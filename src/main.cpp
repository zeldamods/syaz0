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

#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>
#include <span.hpp>
#include <vector>

#include "common/binary_reader.h"
#include "common/types.h"
#include "yaz0.h"

namespace py = pybind11;
using namespace py::literals;

namespace detail {
static tcb::span<u8> PyBytesToSpan(py::bytes b) {
  return {reinterpret_cast<u8*>(PYBIND11_BYTES_AS_STRING(b.ptr())),
          size_t(PYBIND11_BYTES_SIZE(b.ptr()))};
}

static tcb::span<u8> PyBufferToSpan(py::buffer b) {
  const py::buffer_info buffer = b.request();
  if (buffer.itemsize != 1 || buffer.ndim != 1 || buffer.size <= 0)
    throw py::value_error("Expected a non-empty bytes-like object");
  return {static_cast<u8*>(buffer.ptr), size_t(buffer.size)};
}

template <typename C, typename D, common::Endianness E>
void Bind(py::class_<C>& cl, const char* name, common::EndianInt<D, E> C::*pm) {
  cl.def_property(
      name, [pm](const C& c) { return D(c.*pm); }, [pm](C& c, D value) { c.*pm = value; });
}
}  // namespace detail

PYBIND11_MAKE_OPAQUE(std::vector<u8>);

PYBIND11_MODULE(syaz0, m) {
  py::bind_vector<std::vector<u8>>(m, "Bytes", py::buffer_protocol());

  auto header_cl = py::class_<syaz0::Header>(m, "Header");
  header_cl.def_readwrite("magic", &syaz0::Header::magic);
  detail::Bind(header_cl, "uncompressed_size", &syaz0::Header::uncompressed_size);
  detail::Bind(header_cl, "data_alignment", &syaz0::Header::data_alignment);
  header_cl.def_readwrite("reserved", &syaz0::Header::reserved);

  m.def(
      "get_header",
      [](py::buffer data_py) { return syaz0::GetHeader(detail::PyBufferToSpan(data_py)).value(); },
      "data"_a);

  m.def(
      "decompress",
      [](py::buffer src_py) {
        const auto src = detail::PyBufferToSpan(src_py);
        const auto header = syaz0::GetHeader(src);
        if (!header)
          throw py::value_error("Invalid Yaz0 header");
        py::bytes dst_py{nullptr, header->uncompressed_size};
        syaz0::Decompress(src, detail::PyBytesToSpan(dst_py));
        return dst_py;
      },
      "data"_a);

  m.def(
      "decompress_unsafe",
      [](py::bytes src_py) {
        const auto src = detail::PyBytesToSpan(src_py);
        py::bytes dst_py{nullptr, syaz0::GetHeader(src)->uncompressed_size};
        syaz0::DecompressUnsafe(src, detail::PyBytesToSpan(dst_py));
        return dst_py;
      },
      "data"_a);

  m.def(
      "compress",
      [](py::buffer src_py, u32 data_alignment, int level) {
        const auto src = detail::PyBufferToSpan(src_py);
        return syaz0::Compress(src, data_alignment, level);
      },
      "data"_a, "data_alignment"_a = 0, "level"_a = 7);
}
