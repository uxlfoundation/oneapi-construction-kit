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

set(CMAKE_SYSTEM_VERSION 1)

set(TRIPLE "riscv64-linux-gnu")
set(TOOLCHAIN_ROOT /usr)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  set(TOOL_OS_SUFFIX .exe)
else()
  set(TOOL_OS_SUFFIX)
endif()

set(CMAKE_SYSTEM_NAME Linux
  CACHE STRING "operating system" FORCE)
set(CMAKE_SYSTEM_PROCESSOR riscv64
  CACHE STRING "processor architecture" FORCE)

find_program(CMAKE_C_COMPILER NAMES
  "${TRIPLE}-gcc${TOOL_OS_SUFFIX}"
  "${TRIPLE}-gcc-12${TOOL_OS_SUFFIX}"
  "${TRIPLE}-gcc-11${TOOL_OS_SUFFIX}"
  "${TRIPLE}-gcc-10${TOOL_OS_SUFFIX}"
  "${TRIPLE}-gcc-9${TOOL_OS_SUFFIX}"
  "${TRIPLE}-gcc-8${TOOL_OS_SUFFIX}"
  "${TRIPLE}-gcc-7${TOOL_OS_SUFFIX}"
  "${TRIPLE}-gcc-6${TOOL_OS_SUFFIX}"
  "${TRIPLE}-gcc-5${TOOL_OS_SUFFIX}"
  "${TRIPLE}-gcc-4.9${TOOL_OS_SUFFIX}"
  "${TRIPLE}-gcc-4.8${TOOL_OS_SUFFIX}"
  "gcc-${TRIPLE}${TOOL_OS_SUFFIX}"
  "gcc-12-${TRIPLE}${TOOL_OS_SUFFIX}"
  "gcc-11-${TRIPLE}${TOOL_OS_SUFFIX}"
  "gcc-10-${TRIPLE}${TOOL_OS_SUFFIX}"
  "gcc-9-${TRIPLE}${TOOL_OS_SUFFIX}"
  "gcc-8-${TRIPLE}${TOOL_OS_SUFFIX}"
  "gcc-7-${TRIPLE}${TOOL_OS_SUFFIX}"
  "gcc-6-${TRIPLE}${TOOL_OS_SUFFIX}"
  "gcc-5-${TRIPLE}${TOOL_OS_SUFFIX}"
  "gcc-4.9-${TRIPLE}${TOOL_OS_SUFFIX}"
  "gcc-4.8-${TRIPLE}${TOOL_OS_SUFFIX}"
  PATHS "${TOOLCHAIN_ROOT}/bin/" DOC "gcc")

find_program(CMAKE_CXX_COMPILER NAMES
  "${TRIPLE}-g++${TOOL_OS_SUFFIX}"
  "${TRIPLE}-g++-12${TOOL_OS_SUFFIX}"
  "${TRIPLE}-g++-11${TOOL_OS_SUFFIX}"
  "${TRIPLE}-g++-10${TOOL_OS_SUFFIX}"
  "${TRIPLE}-g++-9${TOOL_OS_SUFFIX}"
  "${TRIPLE}-g++-8${TOOL_OS_SUFFIX}"
  "${TRIPLE}-g++-7${TOOL_OS_SUFFIX}"
  "${TRIPLE}-g++-6${TOOL_OS_SUFFIX}"
  "${TRIPLE}-g++-5${TOOL_OS_SUFFIX}"
  "${TRIPLE}-g++-4.9${TOOL_OS_SUFFIX}"
  "${TRIPLE}-g++-4.8${TOOL_OS_SUFFIX}"
  "g++-${TRIPLE}${TOOL_OS_SUFFIX}"
  "g++-12-${TRIPLE}${TOOL_OS_SUFFIX}"
  "g++-11-${TRIPLE}${TOOL_OS_SUFFIX}"
  "g++-10-${TRIPLE}${TOOL_OS_SUFFIX}"
  "g++-9-${TRIPLE}${TOOL_OS_SUFFIX}"
  "g++-8-${TRIPLE}${TOOL_OS_SUFFIX}"
  "g++-7-${TRIPLE}${TOOL_OS_SUFFIX}"
  "g++-6-${TRIPLE}${TOOL_OS_SUFFIX}"
  "g++-5-${TRIPLE}${TOOL_OS_SUFFIX}"
  "g++-4.9-${TRIPLE}${TOOL_OS_SUFFIX}"
  "g++-4.8-${TRIPLE}${TOOL_OS_SUFFIX}"
  PATHS "${TOOLCHAIN_ROOT}/bin/" DOC "g++")

set(CMAKE_AR "${TOOLCHAIN_ROOT}/bin/${TRIPLE}-ar${TOOL_OS_SUFFIX}"
  CACHE PATH "archive" FORCE)
set(CMAKE_LINKER "${TOOLCHAIN_ROOT}/bin/${TRIPLE}-ld${TOOL_OS_SUFFIX}"
  CACHE PATH "linker" FORCE)
set(CMAKE_NM "${TOOLCHAIN_ROOT}/bin/${TRIPLE}-nm${TOOL_OS_SUFFIX}"
  CACHE PATH "nm" FORCE)
set(CMAKE_OBJCOPY "${TOOLCHAIN_ROOT}/bin/${TRIPLE}-objcopy${TOOL_OS_SUFFIX}"
  CACHE PATH "objcopy" FORCE)
set(CMAKE_OBJDUMP "${TOOLCHAIN_ROOT}/bin/${TRIPLE}-objdump${TOOL_OS_SUFFIX}"
  CACHE PATH "objdump" FORCE)
set(CMAKE_STRIP "${TOOLCHAIN_ROOT}/bin/${TRIPLE}-strip${TOOL_OS_SUFFIX}"
  CACHE PATH "strip" FORCE)
set(CMAKE_RANLIB "${TOOLCHAIN_ROOT}/bin/${TRIPLE}-ranlib${TOOL_OS_SUFFIX}"
  CACHE PATH "ranlib" FORCE)

set(CMAKE_FIND_ROOT_PATH /usr/${TRIPLE})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <CMAKE_CXX_LINK_FLAGS> \
<LINK_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> ${LINKER_LIBS}"
  CACHE STRING "Linker command line" FORCE)

find_program(QEMU_RISCV64_EXECUTABLE qemu-riscv64)
if(NOT QEMU_RISCV64_EXECUTABLE MATCHES NOTFOUND)
  set(CMAKE_CROSSCOMPILING_EMULATOR
    ${QEMU_RISCV64_EXECUTABLE} -L ${CMAKE_FIND_ROOT_PATH}
    CACHE STRING "qemu" FORCE)
endif()
