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

# If the riscv target is disabled, then the compiler should be too.
if(NOT TARGET riscv)
  return()
endif()

if(CA_RUNTIME_COMPILER_ENABLED)
  set(RISCV_SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/get_info.h
  )

  add_ca_library(compiler-riscv STATIC ${RISCV_SOURCES})

  ca_option(CA_RISCV_DEMO_MODE BOOL "Enable oneAPI Construction Kit RISC-V demo mode" ON)
  if(CA_RISCV_DEMO_MODE)
    target_compile_definitions(compiler-riscv PRIVATE CA_RISCV_DEMO_MODE=$<BOOL:${CA_RISCV_DEMO_MODE}>)
  endif()

  target_include_directories(compiler-riscv PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  )

  target_link_libraries(compiler-riscv PUBLIC compiler-riscv-utils)

  add_mux_compiler_target(compiler-riscv
     COMPILER_INFO riscv::getInfos
     HEADER_FILE riscv/get_info.h)

  if(CA_ENABLE_TESTS)
    # Include the test subdirectory for any additional riscv tests
    add_subdirectory(test)
  endif()
endif()
