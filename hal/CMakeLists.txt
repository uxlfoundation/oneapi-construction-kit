# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.4.3 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set_property(GLOBAL PROPERTY HAL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

include(HALBin2H)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(HAL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(HAL_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)

set(HAL_SOURCE
  ${HAL_INCLUDE_DIR}/hal.h
  ${HAL_INCLUDE_DIR}/hal_types.h
  ${HAL_INCLUDE_DIR}/hal_riscv.h
  ${HAL_INCLUDE_DIR}/allocator.h
  ${HAL_INCLUDE_DIR}/arg_pack.h
  ${HAL_INCLUDE_DIR}/hal_library.h
  ${HAL_INCLUDE_DIR}/hal_riscv_common.h
  ${HAL_INCLUDE_DIR}/hal_profiler.h
  ${HAL_INCLUDE_DIR}/program.h
  ${HAL_SOURCE_DIR}/program.cpp
  ${HAL_SOURCE_DIR}/profiler.cpp
  ${HAL_SOURCE_DIR}/hal_riscv_common.cpp
  ${HAL_SOURCE_DIR}/arg_pack.cpp
  ${HAL_SOURCE_DIR}/hal_library.cpp)

add_library(
  hal_common STATIC ${HAL_SOURCE})
target_include_directories(
  hal_common PUBLIC ${HAL_INCLUDE_DIR})
set_target_properties(
  hal_common PROPERTIES LINKER_LANGUAGE CXX)

if (MSVC)
target_compile_options(hal_common PRIVATE
  /GR-      # Disable Run-Time Type Information
  /EHs      # Enables standard C++ stack unwinding
  /EHc      # Assume extern "C" functions will not throw
  /WX       # Treat warnings as errors
)
endif()

target_compile_definitions(hal_common PRIVATE
  $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN>)

target_link_libraries(hal_common PUBLIC $<$<PLATFORM_ID:Linux>:dl>)

add_subdirectory(hal_remote)
add_subdirectory(source/hal_null)
