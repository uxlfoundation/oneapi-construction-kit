# Copyright (C) Codeplay Software Limited. All Rights Reserved.

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.4.3)
  project(Abacus)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(ABACUS_BUILD_WITH_RUNTIME_TOOLS
  "Enable building with ComputeAorta runtime tools" OFF)

add_subdirectory(generate)
add_subdirectory(source)

option(ABACUS_BUILD_DOCS "Request that Abacus documentation is built also" ON)
if(ABACUS_BUILD_DOCS)
  add_subdirectory(doxygen)
endif()
