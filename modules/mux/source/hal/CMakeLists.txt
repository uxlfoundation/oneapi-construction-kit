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

if(NOT TARGET hal_common)
  return()
endif()

add_ca_library(mux-hal STATIC
  include/mux/hal/buffer.h source/buffer.cpp
  include/mux/hal/device.h source/device.cpp
  include/mux/hal/executable.h source/executable.cpp
  include/mux/hal/fence.h source/fence.cpp
  include/mux/hal/kernel.h
  include/mux/hal/memory.h source/memory.cpp
  include/mux/hal/query_pool.h source/query_pool.cpp
  include/mux/hal/semaphore.h source/semaphore.cpp)

target_include_directories(mux-hal PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(mux-hal PUBLIC
  cargo hal_common mux-headers mux-utils utils)
