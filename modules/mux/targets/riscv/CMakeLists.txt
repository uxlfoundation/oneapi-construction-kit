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

ca_option(CA_RISCV_ENABLED BOOL "Enable RISC-V mux target" OFF)
if(NOT ${CA_RISCV_ENABLED})
  return()
endif()

ca_option(CA_HAL_NAME STRING "Name of the HAL library to link with" refsi)

# Ensure that HAL targets are processed before the riscv target.
add_subdirectory(external)

# Try to query the HAL for its description string, which might be non-configurable.
string(TOUPPER ${CA_HAL_NAME} CA_HAL_NAME_UPPER)
get_property(REPORTED_HAL_DESCRIPTION GLOBAL
  PROPERTY HAL_${CA_HAL_NAME_UPPER}_DESCRIPTION)
if (REPORTED_HAL_DESCRIPTION)
  set(HAL_DESCRIPTION "${REPORTED_HAL_DESCRIPTION}")
endif()
message("-- oneAPI Construction Kit riscv HAL description: ${HAL_DESCRIPTION}")

# This is needed as cmake uses a known device name for calling clc for offline
# kernels
set(DEFAULT_DEVICE_NAME "")
get_property(REPORTED_HAL_DEVICE_NAME GLOBAL
  PROPERTY HAL_${CA_HAL_NAME_UPPER}_DEVICE_NAME)
if (REPORTED_HAL_DEVICE_NAME)
  set(DEFAULT_DEVICE_NAME "${REPORTED_HAL_DEVICE_NAME}")
endif()
ca_option(CA_RISCV_DEVICE STRING "Default riscv device name"
  "${DEFAULT_DEVICE_NAME}")

if(HAL_DESCRIPTION MATCHES ".*32.*")
  set(RISCV_32_BITS ON)
else()
  set(RISCV_32_BITS OFF)
endif()

if(HAL_DESCRIPTION MATCHES ".*G.*" OR HAL_DESCRIPTION MATCHES ".*D.*")
  set(RISCV_FP64 ON)
else()
  set(RISCV_FP64 OFF)
endif()

if(HAL_DESCRIPTION MATCHES ".*Zfh.*")
  set(RISCV_FP16 ON)
else()
  set(RISCV_FP16 OFF)
endif()

add_ca_library(riscv STATIC
"${CMAKE_CURRENT_SOURCE_DIR}/source/hal.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/queue.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/device.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/device_info.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/device_info_get.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/semaphore.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/fence.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/buffer.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/command_buffer.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/query_pool.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/memory.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/executable.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/kernel.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/image.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/fence.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/buffer.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/device.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/device_info.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/hal.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/memory.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/queue.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/semaphore.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/command_buffer.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/device_info_get.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/executable.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/command_buffer.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/query_pool.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/riscv.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/fence.h"
)

target_include_directories(riscv PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_compile_options(riscv PRIVATE
  $<$<BOOL:${UNIX}>:-Wno-unused-parameter>)
target_link_libraries(riscv PUBLIC
  cargo loader mux-hal mux-headers mux-utils tracer md_handler)

ca_option(CA_RISCV_DEMO_MODE BOOL "Enable oneAPI Construction Kit RISC-V debug mode" OFF)
if(CA_RISCV_DEMO_MODE)
  target_compile_definitions(riscv PRIVATE CA_RISCV_DEMO_MODE=$<BOOL:${CA_RISCV_DEMO_MODE}>)
endif()

target_compile_definitions(riscv PRIVATE
  CA_HAL_DEFAULT_DEVICE="${CA_HAL_NAME}")

target_link_libraries(riscv PUBLIC hal_common)
target_link_libraries(riscv PUBLIC cargo tracer)
target_link_libraries(riscv PUBLIC loader)
if(NOT CA_HAL_NAME STREQUAL "" AND TARGET "hal_${CA_HAL_NAME}")
  target_link_libraries(riscv PUBLIC "hal_${CA_HAL_NAME}")
endif()

ca_option(CA_HAL_LOCK_DEVICE_NAME BOOL
    "Prevent dynamic loading of HAL libraries" FALSE)
target_compile_definitions(riscv PRIVATE
  CA_HAL_LOCK_DEVICE_NAME=$<BOOL:${CA_HAL_LOCK_DEVICE_NAME}>)

# List of capabilities that riscv has.
set(riscvCapabilities)

# TODO : May need to support both as hal is dynamic = see CA-3149
if(RISCV_32_BITS)
  list(APPEND riscvCapabilities "32bit")
else()
  list(APPEND riscvCapabilities "64bit" "atomic64")
endif()
# This must match the HAL's capabilities i.e. D or G if enabled
if(RISCV_FP64)
  list(APPEND riscvCapabilities "fp64")
endif()
# This must match the HAL's capabilities i.e. Zfh if enabled
if(RISCV_FP16)
  list(APPEND riscvCapabilities "fp16")
endif()


target_compile_definitions(riscv PRIVATE CA_RISCV_FP_64=$<BOOL:${RISCV_FP64}>)

target_compile_definitions(riscv PRIVATE
  CA_RISCV_32_BIT=$<BOOL:${RISCV_32_BITS}>)

add_mux_target(riscv CAPABILITIES ${riscvCapabilities}
  HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/riscv
  DEVICE_NAMES "${CA_RISCV_DEVICE}")
