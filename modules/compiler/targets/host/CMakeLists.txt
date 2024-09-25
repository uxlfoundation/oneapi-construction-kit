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

# If the host target is disabled, then the compiler should be too.
if(NOT TARGET host)
  return()
endif()

#[=======================================================================[.rst:
.. cmake:variable:: CA_HOST_CROSS_COMPILERS

  Semi-colon separated list of host cross-compilers, or ``"all"``, defaults to
  ``"all"``.

  .. todo::
    CA-1865: The AArch64 cross-compiler is built with the wrong builtins on
    x86, so only build it on x86_64. ATM we have to default to enabling
    AArch64 because of CA-1376, but this might change.
#]=======================================================================]
ca_option(CA_HOST_CROSS_COMPILERS STRING
  "Semi-colon separated list of host cross-compilers, or 'all'" "all")

#[=======================================================================[.rst:
.. cmake:variable:: CA_HOST_TARGET_<ARCH>_CPU

  Name of the CPU that the host target `<ARCH>` should optimize for, or
  ``"native"`` to optimize for the CPU that's being used to build it. Defaults
  to unset because setting can break compatibility of the build with other CPUs
  than the one specified. This can be used for all host compiler variants that
  are enabled e.g. X86_64, AARCH64, RISCV64. Note the `<ARCH>` part is in
  capitals.
#]=======================================================================]

add_subdirectory(builtins)

set(HOST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host/compiler_kernel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host/info.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host/host_mux_builtin_info.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host/host_pass_machinery.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host/module.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host/target.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/kernel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/AddEntryHook.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/AddFloatingPointControl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/HostMuxBuiltinInfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/HostPassMachinery.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/RemoveByValAttributes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/module.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/target.cpp
    )

add_ca_library(compiler-host STATIC ${HOST_SOURCES}
  DEPENDS mux-config abacus_generate)

target_include_directories(compiler-host PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/builtins/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/compiler/source/base/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/compiler/builtins/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/utils/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/debug/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/compiler/multi_llvm/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/mux/source/host/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/mux/include>)

target_compile_definitions(compiler-host PUBLIC
  $<$<TARGET_EXISTS:resources-host>:CA_ENABLE_HOST_BUILTINS>)
if(CA_HOST_ENABLE_FP64)
  target_compile_definitions(compiler-host PRIVATE CA_HOST_ENABLE_FP64)
endif()
if(CA_HOST_ENABLE_FP16)
  target_compile_definitions(compiler-host PRIVATE CA_HOST_ENABLE_FP16)
endif()

if(TARGET builtins-host)
  add_dependencies(compiler-host builtins-host)
endif()

target_link_libraries(compiler-host PUBLIC
  compiler-base host-utils host
  LLVMCoverage LLVMDebugInfoCodeView LLVMExecutionEngine
  LLVMOrcShared LLVMOrcJIT LLVMVectorize LLVMipo multi_llvm)

if(TARGET LLVMARMCodeGen)
  # link with LLVM if it was built with the ARM target
  target_link_libraries(compiler-host PUBLIC LLVMARMCodeGen)
  target_compile_definitions(compiler-host PRIVATE HOST_LLVM_ARM)
  # TODO(CA-1894,CA-1865): 32-bit and 64-bit builtins in the same build are not
  # yet supported.
  if(CA_BUILD_32_BITS OR CMAKE_SIZEOF_VOID_P EQUAL 4)
    list(APPEND hostCrossCompilers arm)
  endif()
endif()

if(TARGET LLVMAArch64CodeGen AND TARGET LLVMAArch64AsmParser)
  # link with LLVM if it was built with the AArch64 target
  target_link_libraries(compiler-host PUBLIC LLVMAArch64CodeGen LLVMAArch64AsmParser)
  target_compile_definitions(compiler-host PRIVATE HOST_LLVM_AARCH64)
  # TODO(CA-1894,CA-1865): 32-bit and 64-bit builtins in the same build are not
  # yet supported.
  if(NOT (CA_BUILD_32_BITS OR CMAKE_SIZEOF_VOID_P EQUAL 4))
    list(APPEND hostCrossCompilers aarch64)
  endif()
endif()

if(TARGET LLVMRISCVCodeGen)
  # link with LLVM if it was built with the RISCV target
  target_link_libraries(compiler-host PUBLIC LLVMRISCVCodeGen)
  target_compile_definitions(compiler-host PRIVATE HOST_LLVM_RISCV)
  # TODO(CA-1894,CA-1865): 32-bit and 64-bit builtins in the same build are not
  # yet supported.
  if(CA_BUILD_32_BITS OR CMAKE_SIZEOF_VOID_P EQUAL 4)
    list(APPEND hostCrossCompilers riscv32)
  else()
    list(APPEND hostCrossCompilers riscv64)
  endif()
endif()

if(TARGET LLVMX86CodeGen)
  # link with LLVM if it was built with the X86 target
  target_link_libraries(compiler-host PUBLIC LLVMX86CodeGen)
  target_compile_definitions(compiler-host PRIVATE HOST_LLVM_X86)
  # TODO(CA-1894,CA-1865): 32-bit and 64-bit builtins in the same build are not
  # yet supported.
  if(CA_BUILD_32_BITS OR CMAKE_SIZEOF_VOID_P EQUAL 4)
    list(APPEND hostCrossCompilers x86)
  else()
    list(APPEND hostCrossCompilers x86_64)
  endif()
endif()

# A cross-compiler has been requested
if(CA_HOST_CROSS_COMPILERS)
  if(NOT CA_HOST_CROSS_COMPILERS STREQUAL "all")
    set(hostCrossCompilers ${CA_HOST_CROSS_COMPILERS})
  endif()

  # Get the current architecture.
  string(TOUPPER "${CMAKE_SYSTEM_PROCESSOR}" CMAKE_SYSTEM_PROCESSOR_TOUPPER)
  if(CMAKE_SYSTEM_PROCESSOR_TOUPPER STREQUAL X86 OR
    CMAKE_SYSTEM_PROCESSOR_TOUPPER STREQUAL X86_64 OR
    CMAKE_SYSTEM_PROCESSOR_TOUPPER STREQUAL AMD64)
    if(CA_BUILD_32_BITS OR CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(HostArchitecture x86)
    else()
      set(HostArchitecture x86_64)
    endif()
  elseif(CMAKE_SYSTEM_PROCESSOR_TOUPPER STREQUAL ARM OR
         CMAKE_SYSTEM_PROCESSOR_TOUPPER STREQUAL AARCH64 OR
         CMAKE_SYSTEM_PROCESSOR_TOUPPER STREQUAL ARM64)
    if(CA_BUILD_32_BITS OR CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(HostArchitecture arm)
    else()
      set(HostArchitecture aarch64)
    endif()
  elseif(CMAKE_SYSTEM_PROCESSOR_TOUPPER STREQUAL RISCV32 OR
         CMAKE_SYSTEM_PROCESSOR_TOUPPER STREQUAL RISCV64)
      if(CA_BUILD_32_BITS OR CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(HostArchitecture riscv32)
      else()
        set(HostArchitecture riscv64)
      endif()
  else()
    message(FATAL_ERROR
      "host: unknown CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}.")
  endif()

  string(TOUPPER ${HostArchitecture} HOST_ARCH_UPPER)
  ca_option(CA_HOST_TARGET_${HOST_ARCH_UPPER}_CPU STRING
    "Name of the CPU that host ${HOST_ARCH_UPPER} should optimize for, or 'native'" "")  
  if(CA_HOST_TARGET_${HOST_ARCH_UPPER}_CPU)
    message(STATUS "CPU ${HOST_ARCH_UPPER} name ${CA_HOST_TARGET_${HOST_ARCH_UPPER}_CPU}")
    target_compile_definitions(compiler-host PRIVATE
    CA_HOST_TARGET_${HOST_ARCH_UPPER}_CPU="${CA_HOST_TARGET_${HOST_ARCH_UPPER}_CPU}")
  endif()
  set(CA_HOST_TARGET_${HOST_ARCH_UPPER}_FEATURES_DESC
    "Feature list that host ${HOST_ARCH_UPPER} should enable or disable as a comma separated + or - list")
  if(HOST_ARCH_UPPER STREQUAL RISCV32 OR HOST_ARCH_UPPER STREQUAL RISCV64)
    string(APPEND CA_HOST_TARGET_${HOST_ARCH_UPPER}_FEATURES_DESC " e.g. '+v,+zfh'")
  endif()
  ca_option(CA_HOST_TARGET_${HOST_ARCH_UPPER}_FEATURES STRING
    "${CA_HOST_TARGET_${HOST_ARCH_UPPER}_FEATURES_DESC}" "")
  if(CA_HOST_TARGET_${HOST_ARCH_UPPER}_FEATURES)
    message(STATUS "Features ${HOST_ARCH_UPPER} name ${CA_HOST_TARGET_${HOST_ARCH_UPPER}_FEATURES}")
    target_compile_definitions(compiler-host PRIVATE
    CA_HOST_TARGET_${HOST_ARCH_UPPER}_FEATURES="${CA_HOST_TARGET_${HOST_ARCH_UPPER}_FEATURES}")
  endif()
  if(hostCrossCompilers)
    # Validate the user specified cross compiler list.
    foreach(CrossCompiler ${hostCrossCompilers})
      set(found FALSE)
      # Loop over all possible host cross-compilers to enable.
      set(ValidCrossCompilers arm aarch64 x86 x86_64 riscv32 riscv64)
      foreach(ValidName ${ValidCrossCompilers})
        if(CrossCompiler STREQUAL ValidName)
          set(found TRUE)
          break()
        endif()
      endforeach()
      if(NOT ${found})
        message(FATAL_ERROR
          "host: invalid value in hostCrossCompilers: ${CrossCompiler}. "
          "choose from: ${ValidCrossCompilers}")
      endif()

      if(${CrossCompiler} STREQUAL ${HostArchitecture})
        continue()
      endif()

      # Add definitions for the enabled cross-compilers.
      string(TOUPPER ${CrossCompiler} CROSS_COMPILER)
      target_compile_definitions(compiler-host PUBLIC HOST_CROSS_${CROSS_COMPILER})

      # Add the device name associated with the cross compiler.
      # Make this parent scope so we can use it for the lit test inputs
      set(cross_${CrossCompiler}_CA_HOST_CL_DEVICE_NAME PARENT_SCOPE)
      get_ca_host_device_name(cross_${CrossCompiler}_CA_HOST_CL_DEVICE_NAME
        ARCH "${CrossCompiler}")
      set(crossDeviceName ${cross_${CrossCompiler}_CA_HOST_CL_DEVICE_NAME})
      target_compile_definitions(compiler-host PUBLIC
        HOST_CROSS_DEVICE_NAME_${CROSS_COMPILER}="${crossDeviceName}")
        ca_option(CA_HOST_TARGET_${CROSS_COMPILER}_CPU STRING
        "Name of the CPU that host ${CROSS_COMPILER} should optimize for, or 'native'" "")  
        if(CA_HOST_TARGET_${CROSS_COMPILER}_CPU)
          message(STATUS "CPU ${CROSS_COMPILER} name ${CA_HOST_TARGET_${CROSS_COMPILER}_CPU}")
          target_compile_definitions(compiler-host PRIVATE
          CA_HOST_TARGET_${CROSS_COMPILER}_CPU="${CA_HOST_TARGET_${CROSS_COMPILER}_CPU}")
        endif()
        set(CA_HOST_TARGET_${CROSS_COMPILER}_FEATURES_DESC
          "Feature list that host ${CROSS_COMPILER} should enable or disable as a comma separated + or - list")
        if(CROSS_COMPILER STREQUAL "RISCV32" OR CROSS_COMPILER STREQUAL "RISCV64")
          string(APPEND CA_HOST_TARGET_${CROSS_COMPILER}_FEATURES_DESC " e.g. '+v,+zfh'")
        endif()
        ca_option(CA_HOST_TARGET_${CROSS_COMPILER}_FEATURES STRING
          "${CA_HOST_TARGET_${CROSS_COMPILER}_FEATURES_DESC}" "")
        if(CA_HOST_TARGET_${CROSS_COMPILER}_FEATURES)
          message(STATUS "Features ${CROSS_COMPILER} name ${CA_HOST_TARGET_${CROSS_COMPILER}_FEATURES}")
          target_compile_definitions(compiler-host PRIVATE
          CA_HOST_TARGET_${CROSS_COMPILER}_FEATURES="${CA_HOST_TARGET_${CROSS_COMPILER}_FEATURES}")
        endif()
    endforeach()
  endif()
endif()

add_mux_compiler_target(compiler-host
  COMPILER_INFO host::HostInfo::get
  HEADER_FILE host/info.h)

add_subdirectory(test)
