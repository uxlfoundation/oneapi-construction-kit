# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(refsidrv VERSION 1.0.0)

  if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-tree builds are not supported")
  endif()

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  string(APPEND CMAKE_INSTALL_RPATH :$ORIGIN/../lib)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
string(APPEND CMAKE_C_FLAGS " -fvisibility=hidden")
string(APPEND CMAKE_CXX_FLAGS " -fvisibility=hidden")

set(REFSIDRV_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set_property(GLOBAL PROPERTY REFSIDRV_DIR "${REFSIDRV_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH ${REFSIDRV_SOURCE_DIR}/cmake)

add_subdirectory(external)
add_subdirectory(source)

set(REFSIDRV_G1_ISA RV64GCV)
if(DEFINED HAL_DESCRIPTION)
  set(REFSIDRV_G1_ISA ${HAL_DESCRIPTION})
endif()
target_compile_definitions(refsidrv PRIVATE -DREFSI_G1_ISA="${REFSIDRV_G1_ISA}")
set_property(GLOBAL PROPERTY REFSIDRV_G1_ISA "${REFSIDRV_G1_ISA}")

set(REFSIDRV_M1_ISA RV64GCVZbc)
target_compile_definitions(refsidrv PRIVATE -DREFSI_M1_ISA="${REFSIDRV_M1_ISA}")
set_property(GLOBAL PROPERTY REFSIDRV_M1_ISA "${REFSIDRV_M1_ISA}")

set(REFSIDRV_SIM_MAX_HARTS "64" CACHE STRING "Maximum number of harts that can be simulated for RefSi cores")
target_compile_definitions(refsidrv PRIVATE -DREFSI_SIM_MAX_HARTS=${REFSIDRV_SIM_MAX_HARTS})
