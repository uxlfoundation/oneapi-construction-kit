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

if(NOT "${CA_RISCV_DEVICE}" MATCHES "RefSi M1.*")
  return()
endif()

if(CA_RUNTIME_COMPILER_ENABLED)
  set(REFSI_M1_SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/source/refsi_mux_builtin_info.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/source/refsi_pass_machinery.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/source/refsi_wrapper_pass.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/source/info.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/source/module.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/include/refsi_m1/refsi_mux_builtin_info.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/refsi_m1/refsi_pass_machinery.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/refsi_m1/refsi_wrapper_pass.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/refsi_m1/info.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/refsi_m1/module.h
  )

  add_ca_library(compiler-refsi-m1 STATIC ${REFSI_M1_SOURCES})

  ca_option(CA_REFSI_M1_DEMO_MODE BOOL "Enable oneAPI Construction Kit REFSI M1 demo mode" ON)
  if(CA_REFSI_M1_DEMO_MODE)
    target_compile_definitions(compiler-refsi-m1 PRIVATE CA_REFSI_M1_DEMO_MODE=$<BOOL:${CA_REFSI_M1_DEMO_MODE}>)
  endif()

  target_include_directories(compiler-refsi-m1 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  target_link_libraries(compiler-refsi-m1 PUBLIC
    compiler-riscv-utils
    compiler-pipeline
    compiler-binary-metadata
    compiler-linker-utils      
    refsidrv
    compiler-base
    LLVMCoverage LLVMDebugInfoCodeView LLVMExecutionEngine
    LLVMVectorize LLVMipo multi_llvm)

   add_mux_compiler_target(compiler-refsi-m1
     COMPILER_INFO refsi_m1::RefSiM1Info::get
     HEADER_FILE refsi_m1/info.h)


  if(CA_ENABLE_TESTS)
    # Include the test subdirectory for any additional refsi tests
    add_subdirectory(test)
  endif()
endif()
