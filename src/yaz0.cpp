/**
 * Copyright (C) 2019 leoetlino <leo@leolam.fr>
 *
 * This file is part of syaz0.
 *
 * syaz0 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * syaz0 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with syaz0.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "yaz0.h"

#include <algorithm>
#include <bitset>
#include <cstring>
#include <list>
#include <unordered_map>

#include "common/binary_reader.h"

namespace syaz0 {

constexpr std::array<char, 4> Magic = {'Y', 'a', 'z', '0'};
constexpr size_t ChunksPerGroup = 8;
constexpr size_t MaximumMatchLength = 0xFF + 0x12;
constexpr size_t WindowSize = 0x1000;

static std::optional<Header> GetHeader(common::BinaryReader& reader) {
  const auto header = reader.Read<Header>();
  if (!header)
    return std::nullopt;
  if (header->magic != Magic)
    return std::nullopt;
  return header;
}

std::optional<Header> GetHeader(tcb::span<const u8> data) {
  common::BinaryReader reader{data, common::Endianness::Big};
  return GetHeader(reader);
}

namespace {
struct Match {
  size_t offset;
  size_t length;
};

size_t GetWindowBeginOffset(size_t offset) {
  return std::max(0, int(offset) - int(WindowSize));
}

// Naive implementation which scans the sliding window directly.
// Results in an O(n^2) complexity. Vectorized memchr helps keep this usable.
[[maybe_unused]] Match FindMatch(tcb::span<const u8> src, size_t offset) {
  const size_t window_begin_offset = GetWindowBeginOffset(offset);
  Match best_match{};
  for (const u8* pos = src.data() + window_begin_offset; pos < src.data() + offset; ++pos) {
    // memchr is much faster than a handrolled loop for at least GCC.
    pos = static_cast<const u8*>(std::memchr(pos, src[offset], &src[offset] - pos));
    if (!pos)
      break;

    size_t match_length = 0;
    while (match_length < src.size() - offset && match_length < MaximumMatchLength) {
      if (pos[match_length] != src[offset + match_length])
        break;
      match_length += 1;
    }
    if (match_length >= best_match.length) {
      best_match.length = match_length;
      best_match.offset = pos - src.data();
    }
  }
  return best_match;
}

inline u32 StrToKey(const u8* key) {
  return key[0] << 16 | key[1] << 8 | key[2];
}

using HashTable = std::unordered_map<u32, std::list<size_t>>;

Match FindMatchWithTable(tcb::span<const u8> src, size_t offset, HashTable& table) {
  Match best_match{};
  const auto it = table.find(StrToKey(&src[offset]));
  if (it == table.end())
    return best_match;
  for (const size_t match_pos : it->second) {
    size_t match_length = 0;
    while (match_length < src.size() - offset && match_length < MaximumMatchLength) {
      if (src[match_pos + match_length] != src[offset + match_length])
        break;
      match_length += 1;
    }
    if (match_length > best_match.length) {
      best_match.length = match_length;
      best_match.offset = match_pos;
    }
  }
  return best_match;
}

size_t WriteMatch(const Match& match, size_t offset, std::vector<u8>& result) {
  const size_t distance = offset - match.offset - 1;

  if (match.length < 18) {
    result.push_back(((match.length - 2) << 4) | u8(distance >> 8));
    result.push_back(u8(distance));
    return match.length;
  }

  // If the match is longer than 18 bytes, 3 bytes are needed to write the match.
  const size_t actual_length = std::min(MaximumMatchLength, match.length);
  result.push_back(u8(distance >> 8));
  result.push_back(u8(distance));
  result.push_back(u8(actual_length - 0x12));
  return actual_length;
}

void UpdateHashTable(tcb::span<const u8> src, size_t offset, size_t processed, HashTable& table) {
  const size_t window_begin_offset = GetWindowBeginOffset(offset);
  const size_t new_window_begin_offset = window_begin_offset + processed;

  // Remove elements that will be moved out of the window.
  for (size_t i = 0; i < processed; ++i) {
    const u8* data = &src[window_begin_offset + i];
    if (data + 2 >= src.end())
      break;
    const auto it = table.find(StrToKey(data));
    if (it == table.end())
      continue;
    while (!it->second.empty() && it->second.back() < new_window_begin_offset)
      it->second.pop_back();
  }

  // Add elements that will be moved into the window.
  for (size_t i = 0; i < processed; ++i) {
    const u8* data = &src[offset + i];
    if (data + 2 >= src.end())
      break;
    auto& list = table[StrToKey(data)];
    if (list.size() > 128)
      list.pop_back();
    list.push_front(offset + i);
  }
}
}  // namespace

std::vector<u8> Compress(tcb::span<const u8> src, u32 data_alignment) {
  std::vector<u8> result(sizeof(Header));
  result.reserve(src.size());

  // Write the header.
  Header header;
  header.magic = Magic;
  header.uncompressed_size = u32(src.size());
  header.data_alignment = data_alignment;
  header.reserved.fill(0);
  std::memcpy(result.data(), &header, sizeof(header));

  // Each deque is kept sorted, with the most recent entries
  // (those closest to the offset) at the front.
  HashTable table;

  size_t offset = 0;
  while (offset < src.size()) {
    std::bitset<8> group_header;
    // Do not keep a reference because it could be invalidated.
    const size_t header_offset = result.size();
    result.emplace_back(0xFF);

    for (size_t chunk_idx = 0; chunk_idx < ChunksPerGroup && offset < src.size(); ++chunk_idx) {
      const Match match = FindMatchWithTable(src, offset, table);
      size_t processed;
      if (match.length <= 2) {
        group_header.set(7 - chunk_idx);
        result.push_back(src[offset]);
        processed = 1;
      } else {
        processed = WriteMatch(match, offset, result);
      }

      UpdateHashTable(src, offset, processed, table);
      offset += processed;
    }

    result[header_offset] = u8(group_header.to_ulong());
  }

  return result;
}

std::vector<u8> Decompress(tcb::span<const u8> src) {
  const auto header = GetHeader(src);
  if (!header)
    return {};
  std::vector<u8> result(header->uncompressed_size);
  Decompress(src, result);
  return result;
}

void Decompress(tcb::span<const u8> src, tcb::span<u8> dst) {
  common::BinaryReader reader{src, common::Endianness::Big};
  reader.Seek(sizeof(Header));

  u8 group_header = 0;
  size_t remaining_chunks = 0;
  for (auto dst_it = dst.begin(); dst_it < dst.end();) {
    if (remaining_chunks == 0) {
      group_header = reader.Read<u8>().value();
      remaining_chunks = ChunksPerGroup;
    }

    if (group_header & 0x80) {
      *dst_it++ = reader.Read<u8>().value();
    } else {
      const u16 pair = reader.Read<u16>().value();
      const size_t distance = (pair & 0x0FFF) + 1;
      const size_t length = ((pair >> 12) ? (pair >> 12) : (reader.Read<u8>().value() + 16)) + 2;

      const u8* base = dst_it - distance;
      if (base < dst.begin() || dst_it + length > dst.end()) {
        throw std::invalid_argument("Copy is out of bounds");
      }
#pragma GCC unroll 0
      for (size_t i = 0; i < length; ++i)
        *dst_it++ = base[i];
    }

    group_header <<= 1;
    remaining_chunks -= 1;
  }
}

}  // namespace syaz0
