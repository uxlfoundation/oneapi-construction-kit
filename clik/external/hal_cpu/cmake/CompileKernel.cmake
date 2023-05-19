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

function(hal_cpu_compile_kernel_source OBJECT SRC)
  # Start include directory list with input directory
  set(INCLUDES ${ARGN})
  # get the property for the HAL CPU top level directory
  get_property(ROOT_DIR GLOBAL PROPERTY HAL_CPU_DIR)

  # Add include directory to include/device
  set(DEVICE_INCLUDE_DIR ${ROOT_DIR}/include/device)
  list(APPEND INCLUDES ${DEVICE_INCLUDE_DIR})
  # debug flags
  set(OPT -O2)
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(OPT -O0 -g)
  endif()
  # set up includes
  set(EXTRA_CFLAGS)
  foreach(INCLUDE ${INCLUDES})
    set(EXTRA_CFLAGS ${EXTRA_CFLAGS} -I${INCLUDE})
  endforeach()
  # create custom command
  add_custom_command(OUTPUT ${OBJECT}
                     COMMAND ${CMAKE_C_COMPILER} ${OPT} -c -fPIC -DBUILD_FOR_DEVICE ${EXTRA_CFLAGS} ${SRC} -o ${OBJECT}
                     DEPENDS ${SRC})
endfunction()

function(hal_cpu_link_kernel BINARY)
  set(OBJECTS ${ARGN})
  add_custom_command(OUTPUT ${BINARY}
                     COMMAND ${CMAKE_C_COMPILER} -shared ${OBJECTS} -o ${BINARY}
                     DEPENDS ${OBJECTS})
endfunction()
