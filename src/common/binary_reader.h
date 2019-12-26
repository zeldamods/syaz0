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

#pragma once

#include <cstring>
#include <optional>
#include <type_traits>

#include <span.hpp>

#include "common/swap.h"

#ifndef _WIN32
#include <arpa/inet.h>
#endif

namespace common {

enum class Endianness {
  Big,
  Little,
};

namespace detail {
inline Endianness GetPlatformEndianness() {
#ifdef _WIN32
  return Endianness::Little;
#else
  return htonl(0x12345678) == 0x12345678 ? Endianness::Big : Endianness::Little;
#endif
}

template <typename T>
T SwapIfNeeded(T value, Endianness endian) {
  return GetPlatformEndianness() == endian ? value : SwapValue(value);
}
}  // namespace detail

/// A simple binary data reader that automatically byteswaps and avoids undefined behaviour.
class BinaryReader final {
public:
  BinaryReader(tcb::span<const u8> data, Endianness endian) : m_data{data}, m_endian{endian} {}

  const auto& span() const { return m_data; }
  size_t Tell() const { return m_offset; }
  void Seek(size_t offset) { m_offset = offset; }

  template <typename T, bool Safe = true>
  std::optional<T> Read() {
    static_assert(std::is_pod_v<T>);
    if constexpr (Safe) {
      if (m_offset + sizeof(T) > m_data.size())
        return std::nullopt;
    }
    T value;
    std::memcpy(&value, &m_data[m_offset], sizeof(T));
    m_offset += sizeof(T);
    if constexpr (std::is_arithmetic_v<T>)
      return detail::SwapIfNeeded(value, m_endian);
    else
      return value;
  }

  template <bool Safe = true>
  std::optional<u32> ReadU24() {
    if constexpr (Safe) {
      if (m_offset + 3 > m_data.size())
        return std::nullopt;
    }
    const size_t offset = m_offset;
    m_offset += 3;
    if (m_endian == Endianness::Big)
      return m_data[offset] << 16 | m_data[offset + 1] << 8 | m_data[offset + 2];
    return m_data[offset + 2] << 16 | m_data[offset + 1] << 8 | m_data[offset];
  }

private:
  tcb::span<const u8> m_data;
  size_t m_offset = 0;
  Endianness m_endian;
};

template <typename T, Endianness Endian>
struct EndianInt {
  static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
  EndianInt() = default;
  explicit EndianInt(T val) { *this = val; }
  operator T() const { return detail::SwapIfNeeded(raw, Endian); }
  EndianInt& operator=(T v) {
    raw = detail::SwapIfNeeded(v, Endian);
    return *this;
  }

private:
  T raw;
};

template <typename T>
using BeInt = EndianInt<T, Endianness::Big>;
template <typename T>
using LeInt = EndianInt<T, Endianness::Little>;

}  // namespace common
