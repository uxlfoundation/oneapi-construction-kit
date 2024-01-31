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

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  # Support standalone spirv-ll build for ComputeCpp.
  cmake_minimum_required(VERSION 3.4.3 FATAL_ERROR)
  project(spirv-ll)
  list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../cmake)

  include(CAOption)
  ca_option(CA_LLVM_INSTALL_DIR STRING "LLVM install directory" "")
  set(CA_ENABLE_COVERAGE OFF)
  set(CA_RUNTIME_COMPILER_ENABLED ON)

  # Setup LLVM and platform detection.
  include(AddCA)
  include(ImportLLVM)
  include(CAPlatform)

  # Add library dependencies.
  add_subdirectory(
    ${PROJECT_SOURCE_DIR}/../../external ${PROJECT_BINARY_DIR}/external)
  add_subdirectory(
    ${PROJECT_SOURCE_DIR}/../cargo ${PROJECT_BINARY_DIR}/cargo)
endif()

if(${CA_ENABLE_COVERAGE} AND ${CA_RUNTIME_COMPILER_ENABLED})
  add_coverage_modules(${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/spirv-ll.dir/source
    ${CMAKE_CURRENT_SOURCE_DIR}/include/spirv-ll
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/spirv-ll.dir/source)
endif()

add_ca_library(spirv-ll STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/spirv-ll/assert.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/spirv-ll/builder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/spirv-ll/builder_debug_info.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/spirv-ll/builder_glsl.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/spirv-ll/builder_group_async_copies.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/spirv-ll/builder_opencl.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/spirv-ll/context.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/spirv-ll/module.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/spirv-ll/opcodes.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/builder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/builder_core.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/builder_debug_info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/builder_glsl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/builder_group_async_copies.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/builder_opencl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/context.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/module.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opcodes.cpp)

target_include_directories(spirv-ll PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/source)
target_include_directories(spirv-ll SYSTEM PUBLIC
  ${spirv-headers_SOURCE_DIR}/include ${LLVM_INCLUDE_DIR})
target_link_libraries(spirv-ll PUBLIC cargo multi_llvm compiler-pipeline LLVMCore LLVMBitWriter)

add_subdirectory(tools)
if(CA_ENABLE_TESTS)
  add_subdirectory(test)
endif()

# Append to the list of module libraries, the cache MUST be updated.
list(APPEND MODULES_LIBRARIES spirv-ll)
set(MODULES_LIBRARIES ${MODULES_LIBRARIES}
  CACHE INTERNAL "List of module libraries.")
