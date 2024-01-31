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

set(BINARY_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/binary/argument.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/binary/binary.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/binary/kernel_info.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/binary/program_info.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cl/binary/spirv.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/argument.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/binary.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/kernel_info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/program_info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/spirv.cpp)

add_ca_library(CL-binary STATIC ${BINARY_SOURCES})

target_include_directories(CL-binary PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source/cl/include>)

target_link_libraries(CL-binary PUBLIC
  builtins cargo compiler-loader mux utils)
