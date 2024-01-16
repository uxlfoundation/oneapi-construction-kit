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

# If the Mux stub target is not enabled, then don't enable this one either.
if(NOT CA_STUB_ENABLED)
  return()
endif()

add_ca_library(compiler-stub STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/stub/compiler-stub.h)
target_include_directories(compiler-stub PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(compiler-stub PUBLIC compiler-base)

add_mux_compiler_target(compiler-stub
  COMPILER_INFO stub::StubInfo::get
  HEADER_FILE stub/compiler-stub.h)
