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

set(CLIK_HAL_DIR "" CACHE STRING "Directory to check for the HAL")
if(CLIK_HAL_DIR STREQUAL "")
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/hal)
    set(CLIK_HAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hal)
  else()
    # If not in the external directory default to looking at the level above
    # the project
    set(CLIK_HAL_DIR "${PROJECT_SOURCE_DIR}/../hal")
  endif()
endif()

# Clik currently only works with hal cpu in WI mode so disable WG mode
option(HAL_CPU_WG_MODE "Build Hal CPU in workgroup mode" OFF)

set(HAL_SCRIPT ${CLIK_HAL_DIR}/CMakeLists.txt)
if (NOT EXISTS ${HAL_SCRIPT})
  message(FATAL_ERROR "Could not find ${HAL_SCRIPT}. hal needs to be either in external or passed in via CLIK_HAL_DIR (${CLIK_HAL_DIR}. ")
endif()

add_subdirectory(${CLIK_HAL_DIR} ${CMAKE_CURRENT_BINARY_DIR}/hal)

# Iterate through all 'hal_*' sub-directories.
file(GLOB HAL_DIR_NAMES FOLLOW_SYMLINKS LIST_DIRECTORIES true "hal_*")

if (DEFINED CLIK_EXTERNAL_HAL_DIR)
  list(APPEND HAL_DIR_NAMES ${CLIK_EXTERNAL_HAL_DIR})
endif()

foreach(HAL_PATH ${HAL_DIR_NAMES})
  file(RELATIVE_PATH HAL_DIR ${CMAKE_CURRENT_SOURCE_DIR} ${HAL_PATH})
  string(REGEX REPLACE "^hal_" "" HAL_NAME ${HAL_DIR})

  # Give the option for each HAL that was found to be built or not.
  get_filename_component(HAL_NAME_BASENAME ${HAL_NAME} NAME)    
  string(TOUPPER ${HAL_NAME_BASENAME} HAL_NAME_UPPER)
  set(CLIK_BUILD_HAL_${HAL_NAME_UPPER} TRUE CACHE BOOL "Enable building the '${HAL_NAME}' HAL.")

  # Include the current HAL, if configured by the user.
  if (EXISTS ${HAL_PATH}/CMakeLists.txt AND ${CLIK_BUILD_HAL_${HAL_NAME_UPPER}})
    add_subdirectory(${HAL_PATH} ${CMAKE_CURRENT_BINARY_DIR}/${HAL_NAME_BASENAME})
  endif()
endforeach()
