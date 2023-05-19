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
.. cmake:variable:: CA_HOST_TARGET_CPU

  Name of the CPU that host should optimize for, or ``"native"`` to optimize for
  the CPU that's being used to build it. Defaults to unset because setting can
  break compatibility of the build with other CPUs than the one specified.
#]=======================================================================]
ca_option(CA_HOST_TARGET_CPU STRING
  "Name of the CPU that host should optimize for, or 'native'" "")

add_subdirectory(builtins)

set(HOST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host/compiler_kernel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host/info.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host/host_mux_builtin_info.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host/host_pass_machinery.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host/module.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host/passes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host/perf_support.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host/target.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/kernel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/passes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/perf_support.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/AddEntryHook.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/AddFloatingPointControl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/DisableNeonAttribute.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/HostMuxBuiltinInfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/HostPassMachinery.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/RemoveByValAttributes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/module.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/target.cpp
    )

add_ca_library(compiler-host STATIC ${HOST_SOURCES})

target_include_directories(compiler-host PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/builtins/include
    ${CMAKE_SOURCE_DIR}/modules/compiler/source/base/include
    ${CMAKE_SOURCE_DIR}/modules/compiler/builtins/include
    ${CMAKE_SOURCE_DIR}/modules/utils/include
    ${CMAKE_SOURCE_DIR}/modules/debug/include
    ${CMAKE_SOURCE_DIR}/modules/multi_llvm/include
    ${CMAKE_SOURCE_DIR}/modules/mux/source/host/include
    ${CMAKE_SOURCE_DIR}/modules/mux/include)

target_compile_definitions(compiler-host PUBLIC
  $<$<TARGET_EXISTS:resources-host>:CA_ENABLE_HOST_BUILTINS>)

if(TARGET builtins-host)
  add_dependencies(compiler-host builtins-host)
endif()

target_link_libraries(compiler-host PUBLIC
  compiler-base host-common host
  LLVMCoverage LLVMDebugInfoCodeView LLVMExecutionEngine
  LLVMMCJIT LLVMVectorize LLVMipo multi_llvm)

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

if(CA_HOST_TARGET_CPU STREQUAL native)
  target_compile_definitions(compiler-host PRIVATE CA_HOST_TARGET_CPU_NATIVE)
elseif(CA_HOST_TARGET_CPU)
  message(STATUS "CPU name ${CA_HOST_TARGET_CPU}")
  target_compile_definitions(compiler-host PRIVATE
    CA_HOST_TARGET_CPU="${CA_HOST_TARGET_CPU}")
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
         CMAKE_SYSTEM_PROCESSOR_TOUPPER STREQUAL ARM64)
    if(CA_BUILD_32_BITS OR CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(HostArchitecture arm)
    else()
      set(HostArchitecture aarch64)
    endif()
  else()
    message(FATAL_ERROR
      "host: unknown CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}.")
  endif()

  if(hostCrossCompilers)
    # Validate the user specified cross compiler list.
    foreach(CrossCompiler ${hostCrossCompilers})
      set(found FALSE)
      # Loop over all possible host cross-compilers to enable.
      set(ValidCrossCompilers arm aarch64 x86 x86_64)
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
    endforeach()
  endif()
endif()

add_mux_compiler_target(compiler-host
  COMPILER_INFO host::HostInfo::get
  HEADER_FILE host/info.h)

add_subdirectory(test)
