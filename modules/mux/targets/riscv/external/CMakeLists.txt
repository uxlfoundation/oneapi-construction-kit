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

if(DEFINED HAL_DESCRIPTION)
  set (DEFAULT_ISA "${HAL_DESCRIPTION}" CACHE STRING "Take DEFAULT_ISA from HAL_DESCRIPTION")
endif()

set(RISCV_TOOLCHAIN_DIR "${CA_LLVM_INSTALL_DIR}" CACHE PATH
  "Path to the clang RISC-V toolchain")

# Iterate through all 'hal_*' sub-directories.
file(GLOB HAL_DIR_NAMES FOLLOW_SYMLINKS LIST_DIRECTORIES true "hal_*"
     ${PROJECT_SOURCE_DIR}/examples/hals/hal_*)

if (DEFINED CA_RISCV_EXTERNAL_HAL_DIR)
  list(APPEND HAL_DIR_NAMES ${CA_RISCV_EXTERNAL_HAL_DIR})
else()
  list(APPEND HAL_DIR_NAMES ${PROJECT_SOURCE_DIR}/examples/refsi/hal_refsi)
endif()

foreach(HAL_PATH ${HAL_DIR_NAMES})
  get_filename_component(HAL_RISCV_BASENAME ${HAL_PATH} NAME)   
  string(REGEX REPLACE "^hal_" "" HAL_NAME ${HAL_RISCV_BASENAME})

  # Give the option for each HAL that was found to be built or not.
  string(TOUPPER ${HAL_NAME} HAL_NAME_UPPER)

  # If this HAL is the one specified in CA_HAL_NAME then we want to build it by
  # default. Otherwise it is disabled unless the user explicitly enables it.
  if (DEFINED CA_HAL_NAME)
    string(TOUPPER ${CA_HAL_NAME} CA_HAL_NAME_UPPER)
    if (CA_HAL_NAME_UPPER STREQUAL HAL_NAME_UPPER)
      set(DEFAULT_VAL ON)
    else()
      set(DEFAULT_VAL OFF)
    endif()
  else()
    set(DEFAULT_VAL OFF)
  endif()
  option(HAL_ENABLE_${HAL_NAME_UPPER} "Enable building the '${HAL_NAME}' HAL."
    ${DEFAULT_VAL})
  # Include the current HAL, if configured by the user.
  if (${HAL_ENABLE_${HAL_NAME_UPPER}})
    if (EXISTS ${HAL_PATH}/CMakeLists.txt)
      add_subdirectory(${HAL_PATH} ${CMAKE_CURRENT_BINARY_DIR}/${HAL_RISCV_BASENAME})
    else()
      # The HAL directory has no CMakeLists.txt so error out as we cannot build it
      message(FATAL_ERROR
        "${HAL_PATH}/CMakeLists.txt not found - unable to build ${HAL_NAME}")
    endif()
  endif()
endforeach()

# If we have no hals here we cannot build so error out
# Note if we have compiled an out of tree hal KNOWN_HAL_DEVICES will be set
get_property(HAL_DEVICES GLOBAL PROPERTY KNOWN_HAL_DEVICES)
if (NOT HAL_DEVICES)
  message(FATAL_ERROR
    "No HAL built (either under ${CMAKE_CURRENT_SOURCE_DIR} or defined by KNOWN_HAL_DEVICES")
endif()
