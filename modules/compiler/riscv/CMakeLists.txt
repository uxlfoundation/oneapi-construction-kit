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

# Check if riscv is enabled in the llvm install.
# If so we can build this library
find_library(LLVMRISCVCODEGEN LLVMRISCVCodeGen PATHS
            "${LLVM_INSTALL_PREFIX}/lib" NO_DEFAULT_PATH)

if (LLVMRISCVCODEGEN)
  if(CA_RUNTIME_COMPILER_ENABLED)
    add_subdirectory(runtimelib)

    set(RISCV_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/source/passes/IRToBuiltinsPass.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/source/info.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/source/module.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/source/target.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/source/riscv_pass_machinery.cpp  
        ${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/bakery.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/info.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/module.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/target.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/riscv_pass_machinery.h
    )

    add_ca_library(compiler-riscv-utils STATIC ${RISCV_SOURCES})

    ca_option(CA_RISCV_DEMO_MODE BOOL "Enable oneAPI Construction Kit RISC-V demo mode" ON)
    if(CA_RISCV_DEMO_MODE)
      target_compile_definitions(compiler-riscv-utils PRIVATE CA_RISCV_DEMO_MODE=$<BOOL:${CA_RISCV_DEMO_MODE}>)
    endif()


    target_include_directories(compiler-riscv-utils PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )

    target_link_libraries(compiler-riscv-utils PUBLIC
      compiler-base
      hal_common      
      compiler-linker-utils
      LLVMCoverage LLVMDebugInfoCodeView LLVMExecutionEngine
      LLVMVectorize LLVMipo multi_llvm)

    target_link_libraries(compiler-riscv-utils PUBLIC riscvrt)
  endif()
endif()
