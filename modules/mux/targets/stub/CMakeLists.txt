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

# The stub target is only ever used for manually testing unusual corner cases,
# so it needs to be explicitly enabled. Most of the time we pretend it isn't
# even here. It will probably be removed completely before too long.
ca_option(CA_STUB_ENABLED BOOL "Enable stub mux target" OFF)
if(NOT CA_STUB_ENABLED)
  return()
endif()

add_ca_library(stub STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/stub/stub.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/stub.cpp)
target_include_directories(stub PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_options(stub PRIVATE
  $<$<BOOL:${UNIX}>:-Wno-unused-parameter>)
target_link_libraries(stub PUBLIC mux-headers)

add_mux_target(stub CAPABILITIES "64bit"
  HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/stub
  DEVICE_NAMES "Codeplay Software Ltd. - stub")

