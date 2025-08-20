# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set(CMAKE_SYSTEM_NAME                 Linux)
set(CMAKE_SYSTEM_PROCESSOR            aarch64)
set(CMAKE_C_COMPILER                  aarch64-linux-gnu-gcc CACHE STRING "")
set(CMAKE_CXX_COMPILER                aarch64-linux-gnu-g++ CACHE STRING "")
set(PKG_CONFIG_EXECUTABLE             aarch64-linux-gnu-pkg-config)

set(CMAKE_FIND_ROOT_PATH              /usr/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_CROSSCOMPILING_EMULATOR     qemu-aarch64 -L ${CMAKE_FIND_ROOT_PATH} CACHE STRING "")
