# Copyright (C) Codeplay Software Limited. All Rights Reserved.

cmake_minimum_required(VERSION 3.0.0)
project(clik VERSION 1.0.0)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-tree builds are not supported")
endif()

set(CLIK_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(BakedKernel)
include(ClikExample)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
string(APPEND CMAKE_INSTALL_RPATH :$ORIGIN/../lib)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find python3 on the host system, this is a required dependency used for
# automated testing.
if(${CMAKE_VERSION} GREATER_EQUAL 3.12.0)
  find_package(Python3 REQUIRED)
else()
  set(Python_ADDITIONAL_VERSIONS 3)
  find_package(PythonInterp)
endif()

add_subdirectory(external)
include(SelectHAL)

add_subdirectory(runtime)
add_subdirectory(examples)
add_subdirectory(test)
