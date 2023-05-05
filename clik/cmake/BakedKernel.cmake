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

#[=======================================================================[.rst:
.. cmake:command:: add_baked_kernel

  The ``add_baked_kernel`` macro compiles and links a kernel based on a header and source,
  producing the result in a header file using hal_add_bin2h_target.

  Arguments:
    * ``kernel``: The name of the kernel to compile e.g. blur
    * ``header``: The binary output header filepath
    * ``src``: The source file containing the kernel entry
#]=======================================================================]
function(add_baked_kernel KERNEL HEADER SRC)
  set(KERNEL_SRC ${SRC})
  if (NOT IS_ABSOLUTE ${KERNEL_SRC})
    set(KERNEL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/${KERNEL_SRC})
  endif()
  set(KERNEL_EXE ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL})
  set(KERNEL_B2H ${CMAKE_CURRENT_BINARY_DIR}/${HEADER})
  set(B2H_TARGET ${KERNEL}_binary)

  get_filename_component(SRC_DIR ${KERNEL_SRC} DIRECTORY)
  list(APPEND INCLUDES ${SRC_DIR})

  # Compile the kernel source file to an object.
  get_filename_component(SRC_NAME_WE ${KERNEL_SRC} NAME_WE)
  set(KERNEL_OBJ ${CMAKE_CURRENT_BINARY_DIR}/${SRC_NAME_WE}.o)
  list(APPEND OBJECTS ${KERNEL_OBJ})
  hal_compile_kernel_source(${KERNEL_OBJ} ${KERNEL_SRC} ${INCLUDES})

  # Compile the entry point file for the kernel, if it exists.
  set(KERNEL_ENTRY_OBJ ${CMAKE_CURRENT_BINARY_DIR}/${SRC_NAME_WE}_entry.o)
  string(REPLACE "${CLIK_SOURCE_DIR}/examples/" "" REL_SRC_DIR ${SRC_DIR})
  if(IS_ABSOLUTE ${REL_SRC_DIR})
    message(WARNING "Couldn't locate kernel source file ${KERNEL_SRC} in the clik examples directory. It will be compiled without a separate entry point file.")
  else()
    # Try to locate a device-specific entry point file provided for the kernel by the HAL.
    string(TOUPPER ${CLIK_HAL_NAME} HAL_NAME_UPPER)
    get_property(HAL_EXAMPLE_DIR GLOBAL PROPERTY HAL_${HAL_NAME_UPPER}_EXAMPLE_DIR)
    set(EXT_KERNEL_ENTRY_SRC "${HAL_EXAMPLE_DIR}/${REL_SRC_DIR}/${SRC_NAME_WE}_entry.c")
    if(EXISTS ${EXT_KERNEL_ENTRY_SRC})
      hal_compile_kernel_source(${KERNEL_ENTRY_OBJ} ${EXT_KERNEL_ENTRY_SRC} ${INCLUDES})
      list(APPEND OBJECTS ${KERNEL_ENTRY_OBJ})
      # Set this to allow clik to know whether the kernel entry file has been found
      set(FOUND_KERNEL_ENTRY TRUE PARENT_SCOPE)
    else()
      message(WARNING "Not compiling kernel: '${KERNEL}'. No kernel entry point file found.")
      set(FOUND_KERNEL_ENTRY FALSE PARENT_SCOPE)
      return()
    endif()
  endif()

  # Link all the objects into the kernel executable.
  hal_link_kernel(${KERNEL_EXE} ${OBJECTS})

  hal_add_bin2h_target(${B2H_TARGET} ${KERNEL_EXE} ${KERNEL_B2H})
endfunction()
