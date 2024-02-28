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

if(${CA_ENABLE_COVERAGE} AND ${CA_RUNTIME_COMPILER_ENABLED})
  add_coverage_modules(${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/loader.dir/source
    ${CMAKE_CURRENT_SOURCE_DIR}/include/loader
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/loader.dir/source)
endif()

add_ca_library(loader STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/loader/elf.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/loader/mapper.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/loader/relocation_types.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/loader/relocations.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/elf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/mapper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/relocations.cpp)

target_include_directories(loader PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(loader PUBLIC cargo)
