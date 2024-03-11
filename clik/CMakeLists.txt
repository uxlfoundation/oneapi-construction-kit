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

cmake_minimum_required(VERSION 3.0.0)
project(clik VERSION 1.0.0)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-tree builds are not supported")
endif()

set(CMAKE_CXX_STANDARD 17)

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
