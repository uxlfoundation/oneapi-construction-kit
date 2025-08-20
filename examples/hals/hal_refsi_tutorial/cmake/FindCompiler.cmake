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

function(find_clang TRIPLE)

  # if DEFAULT_RISCV_TOOLCHAIN_TRIPLE is not set, 
  if(NOT DEFAULT_RISCV_TOOLCHAIN_TRIPLE)
    set(DEFAULT_RISCV_TOOLCHAIN_TRIPLE ${TRIPLE})
  endif()

  # set cache variable, will be used by find_cc()
  set(RISCV_TOOLCHAIN_TRIPLE "${DEFAULT_RISCV_TOOLCHAIN_TRIPLE}" CACHE PATH "Triple use for compilation target")

  find_program(RISCV_CC NAMES clang clang-12 clang-13
    PATHS "${RISCV_TOOLCHAIN_DIR}/bin/" DOC "clang"
    NO_DEFAULT_PATH)
endfunction()

function(find_gcc DEFAULT_TRIPLE)
  # if RISCV_GNU_TOOLCHAIN_DIR is not set, default to "/usr/"
  if(NOT RISCV_GNU_TOOLCHAIN_DIR)
    set(RISCV_GNU_TOOLCHAIN_DIR "/usr/")
  endif()

  # if RISCV_GNU_TOOLCHAIN_TRIPLE is not set, default to "riscv64-linux-gnu"
  if(NOT RISCV_GNU_TOOLCHAIN_TRIPLE)
    set(RISCV_GNU_TOOLCHAIN_TRIPLE "${DEFAULT_TRIPLE}-linux-gnu")
  endif()

  find_program(RISCV_CC NAMES
    "${RISCV_GNU_TOOLCHAIN_TRIPLE}-gcc"
    "${RISCV_GNU_TOOLCHAIN_TRIPLE}-gcc-10.1.0"
    "${RISCV_GNU_TOOLCHAIN_TRIPLE}-gcc-9"
    "${RISCV_GNU_TOOLCHAIN_TRIPLE}-gcc-8"
    "gcc-${RISCV_GNU_TOOLCHAIN_TRIPLE}"
    "gcc-10.1.0-${RISCV_GNU_TOOLCHAIN_TRIPLE}"
    "gcc-9-${RISCV_GNU_TOOLCHAIN_TRIPLE}"
    "gcc-8-${RISCV_GNU_TOOLCHAIN_TRIPLE}"
    PATHS "${RISCV_GNU_TOOLCHAIN_DIR}/bin/" DOC "gcc")
endfunction()

function(find_cc TRIPLE)
  # Find a compiler, giving priority to clang over GCC.
  if(NOT EXISTS "${RISCV_CC}")
    find_clang(${TRIPLE})
    if(NOT EXISTS "${RISCV_CC}")
      find_gcc(${TRIPLE})
      if(NOT EXISTS "${RISCV_CC}")
        return()
      endif()
    endif()
  endif()

  # Determine whether the compiler is clang or GCC.
  execute_process(COMMAND ${RISCV_CC} --version
                  OUTPUT_VARIABLE RISCV_CC_OUTPUT)
  if (RISCV_CC_OUTPUT MATCHES "clang")
    message(STATUS "Found CC: clang")

    if(TRIPLE STREQUAL "riscv32")
      set_property(GLOBAL PROPERTY RISCV_CC_FLAGS
        --target=${RISCV_TOOLCHAIN_TRIPLE}
        -fuse-ld=lld
        -march=rv32gc
        -mno-relax)
    else()
      set_property(GLOBAL PROPERTY RISCV_CC_FLAGS
        --target=${RISCV_TOOLCHAIN_TRIPLE}
        -fuse-ld=lld
        -march=rv64gc
        -mno-relax)
    endif()

    set_property(GLOBAL PROPERTY RISCV_LINKER_FLAGS "")
  else()
    message(STATUS "Found CC: gcc")
    set_property(GLOBAL PROPERTY RISCV_CC_FLAGS "")
    set_property(GLOBAL PROPERTY RISCV_LINKER_FLAGS
      -nostartfiles
      -rdynamic)
  endif()
endfunction()
