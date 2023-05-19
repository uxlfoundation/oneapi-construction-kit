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

# Fetch the list of known HALs.
get_property(HAL_NAMES GLOBAL PROPERTY KNOWN_HAL_DEVICES)
foreach(HAL_NAME ${HAL_NAMES})
  message(STATUS "Found HAL: ${HAL_NAME}")
endforeach()

# Let the user choose the HAL using a list (in the GUI)
set(CLIK_HAL_NAME "cpu" CACHE STRING "Name of the HAL library to use by default")
set_property(CACHE CLIK_HAL_NAME PROPERTY STRINGS ${HAL_NAMES})

# Create a function that compiles a kernel for the selected HAL.
# TODO: Use cmake_language (https://cmake.org/cmake/help/v3.18/command/cmake_language.html)
if(NOT CLIK_HAL_NAME STREQUAL "")
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/compile_kernel.cmake "
function(hal_compile_kernel_source)
  hal_${CLIK_HAL_NAME}_compile_kernel_source(\${ARGN})
endfunction()

function(hal_link_kernel)
  hal_${CLIK_HAL_NAME}_link_kernel(\${ARGN})
endfunction()
"
  )
  include("${CMAKE_CURRENT_BINARY_DIR}/compile_kernel.cmake")
endif()
