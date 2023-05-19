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

if(NOT CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64 AND
   NOT CMAKE_SYSTEM_PROCESSOR STREQUAL AMD64)
  message(STATUS
    "host: Target specific builtins disabled for ${CMAKE_SYSTEM_PROCESSOR}")
  return()
endif()

if(NOT CMAKE_SYSTEM_NAME STREQUAL Windows)
  set(fp64 FP64)
endif()

add_target_builtins(host
  TRIPLE x86_64-unknown-unknown 64BIT ${fp64}
  HEADER_FILE ${CMAKE_CURRENT_BINARY_DIR}/include/host/resources.h
  SOURCES source/sqrt.cpp)
