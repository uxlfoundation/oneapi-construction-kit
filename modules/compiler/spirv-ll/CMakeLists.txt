# Copyright (C) Codeplay Software Limited. All Rights Reserved.

ca_option(CA_ENABLE_SPIRV_LL_EXPERIMENTAL BOOL
  "Enable experimental changes to spirv-ll." OFF)

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
  ${CMAKE_CURRENT_SOURCE_DIR}/include/spirv-ll/context.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/spirv-ll/module.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/spirv-ll/opcodes.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/builder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/builder_core.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/builder_glsl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/builder_opencl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/builder_group_async_copies.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/context.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/module.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opcodes.cpp)

target_include_directories(spirv-ll PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/source)
target_include_directories(spirv-ll SYSTEM PUBLIC
  ${spirv-headers_SOURCE_DIR}/include ${LLVM_INCLUDE_DIR})
target_link_libraries(spirv-ll PUBLIC cargo multi_llvm LLVMCore LLVMBitWriter)

if(${CA_ENABLE_SPIRV_LL_EXPERIMENTAL})
  target_compile_definitions(spirv-ll PUBLIC SPIRV_LL_EXPERIMENTAL)
endif()

add_subdirectory(tools)
if(CA_ENABLE_TESTS)
  add_subdirectory(test)
endif()

# Append to the list of module libraries, the cache MUST be updated.
list(APPEND MODULES_LIBRARIES spirv-ll)
set(MODULES_LIBRARIES ${MODULES_LIBRARIES}
  CACHE INTERNAL "List of module libraries.")
