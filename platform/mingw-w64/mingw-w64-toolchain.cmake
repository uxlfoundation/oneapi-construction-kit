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

set(CMAKE_SYSTEM_VERSION 1)

set(TRIPLE "x86_64-w64-mingw32")
set(TYPE "posix")  # 'win32' or 'posix'
set(TOOLCHAIN_ROOT "/usr")

if(CMAKE_HOST_WIN32)
  message(FATAL_ERROR "Cross-compiling MinGW on Windows, just use "
                      "'MinGW Makefiles' generator instead.")
endif()

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

set(PREFIX "${TOOLCHAIN_ROOT}/bin/${TRIPLE}-")
set(CMAKE_C_COMPILER "${PREFIX}gcc-${TYPE}" CACHE PATH "gcc" FORCE)
set(CMAKE_CXX_COMPILER "${PREFIX}g++-${TYPE}" CACHE PATH "g++" FORCE)
set(CMAKE_AR "${PREFIX}ar" CACHE PATH "archive" FORCE)
set(CMAKE_LINKER "${PREFIX}ld" CACHE PATH "linker" FORCE)
set(CMAKE_NM "${PREFIX}nm" CACHE PATH "nm" FORCE)
set(CMAKE_OBJCOPY "${PREFIX}objcopy" CACHE PATH "objcopy" FORCE)
set(CMAKE_OBJDUMP "${PREFIX}objdump" CACHE PATH "objdump" FORCE)
set(CMAKE_STRIP "${PREFIX}strip" CACHE PATH "strip" FORCE)
set(CMAKE_RANLIB "${PREFIX}ranlib" CACHE PATH "ranlib" FORCE)
set(CMAKE_RC_COMPILER "${PREFIX}windres" CACHE PATH "windres" FORCE)

set(CMAKE_FIND_ROOT_PATH /usr/${TRIPLE})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(TARGET_FLAGS "")
set(BASE_FLAGS "${TARGET_FLAGS}")

set(CMAKE_C_FLAGS "${BASE_FLAGS} " CACHE STRING "c flags" FORCE)
set(CMAKE_CXX_FLAGS "${BASE_FLAGS}" CACHE STRING "c++ flags" FORCE)

set(LINKER_FLAGS "${TARGET_FLAGS}")
set(LINKER_LIBS "")
set(CMAKE_SHARED_LINKER_FLAGS "${LINKER_FLAGS}" CACHE STRING "linker flags" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS "${LINKER_FLAGS}" CACHE STRING "linker flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "${LINKER_FLAGS}" CACHE STRING "linker flags" FORCE)

set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> -o <TARGET>  <OBJECTS> <LINK_LIBRARIES> ${LINKER_LIBS}" CACHE STRING "Linker command line" FORCE)
