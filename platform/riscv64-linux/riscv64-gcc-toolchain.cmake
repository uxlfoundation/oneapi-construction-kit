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

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  message(FATAL_ERROR "Cross-compiling for RV64 Linux is not supported on \
    Windows")
endif()

set(TOOLCHAIN_ROOT "" CACHE PATH "path to toolchain root directory")
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES TOOLCHAIN_ROOT)
if (NOT TOOLCHAIN_ROOT)
  message(FATAL_ERROR "TOOLCHAIN_ROOT must be defined when using the riscv64 \
    toolchain!")
endif()

set(TRIPLE riscv64-unknown-linux-gnu)

set(CMAKE_C_COMPILER "${TOOLCHAIN_ROOT}/bin/${TRIPLE}-gcc" CACHE PATH "gcc" FORCE)
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_ROOT}/bin/${TRIPLE}-g++" CACHE PATH "g++" FORCE)
set(CMAKE_AR "${TOOLCHAIN_ROOT}/bin/${TRIPLE}-ar" CACHE PATH "archive" FORCE)
set(CMAKE_LINKER "${TOOLCHAIN_ROOT}/bin/${TRIPLE}-ld" CACHE PATH "linker" FORCE)
set(CMAKE_NM "${TOOLCHAIN_ROOT}/bin/${TRIPLE}-nm" CACHE PATH "nm" FORCE)
set(CMAKE_OBJCOPY "${TOOLCHAIN_ROOT}/bin/${TRIPLE}-objcopy" CACHE PATH "objcopy" FORCE)
set(CMAKE_OBJDUMP "${TOOLCHAIN_ROOT}/bin/${TRIPLE}-objdump" CACHE PATH "objdump" FORCE)
set(CMAKE_STRIP "${TOOLCHAIN_ROOT}/bin/${TRIPLE}-strip" CACHE PATH "strip" FORCE)
set(CMAKE_RANLIB "${TOOLCHAIN_ROOT}/bin/${TRIPLE}-ranlib" CACHE PATH "ranlib" FORCE)
