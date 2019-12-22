cmake_minimum_required(VERSION 3.10)
project(syaz0 CXX)

if (MSVC)
  set(PYBIND11_CPP_STANDARD /std:c++latest)
else()
  set(PYBIND11_CPP_STANDARD -std=c++17)
endif()

find_package(pybind11 REQUIRED)

pybind11_add_module(syaz0
  common/binary_reader.h
  common/swap.h
  common/types.h
  main.cpp
  yaz0.cpp
  yaz0.h
)

target_include_directories(syaz0 PRIVATE ./)

if (MSVC)
  target_compile_options(syaz0 PRIVATE /W4)
else()
  target_compile_options(syaz0 PRIVATE -Wall -Wextra)
endif()

target_include_directories(syaz0 PUBLIC ./lib/tcb)
