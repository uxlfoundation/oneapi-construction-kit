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
{% if cookiecutter.copyright_name != "" -%}
# Additional changes Copyright (C) {{cookiecutter.copyright_name}}. All Rights
# Reserved.
{% endif -%}

ca_option(CA_{{cookiecutter.target_name_capitals}}_ENABLED BOOL "Enable {{cookiecutter.target_name}} mux target" OFF)
if(NOT ${CA_{{cookiecutter.target_name_capitals}}_ENABLED})
  return()
endif()

if (DEFINED CA_EXTERNAL_{{cookiecutter.target_name_capitals}}_HAL_DIR)
  # Get the name of only the final part of the directory
  get_filename_component(EXTERNAL_HAL_NAME ${CA_EXTERNAL_{{cookiecutter.target_name_capitals}}_HAL_DIR} NAME)
  # Remove the hal component
  string(REPLACE "hal_" "" EXTERNAL_HAL_NAME ${EXTERNAL_HAL_NAME} )
  # Set this as the default HAL Name
  ca_option(CA_HAL_{{cookiecutter.target_name_capitals}}_NAME STRING "Name of the HAL library to link with"  ${EXTERNAL_HAL_NAME})
else()
  ca_option(CA_HAL_{{cookiecutter.target_name_capitals}}_NAME STRING "Name of the HAL library to link with" {{cookiecutter.target_name}})
endif()

{% if cookiecutter.llvm_name == "RISCV" -%}
# RISCV_TOOLCHAIN_DIR can be useful for runtime library generation and some Hal variants
set(RISCV_TOOLCHAIN_DIR "${CA_LLVM_INSTALL_DIR}" CACHE PATH
  "Path to the clang RISC-V toolchain")
{% endif -%}

# Ensure that HAL targets are processed before the target.
add_subdirectory(external)

# Try to query the HAL for its description string, which might be non-configurable.
string(TOUPPER ${CA_HAL_{{cookiecutter.target_name_capitals}}_NAME} CA_HAL_{{cookiecutter.target_name_capitals}}_NAME_UPPER)
get_property(REPORTED_HAL_{{cookiecutter.target_name_capitals}}_DESCRIPTION GLOBAL
  PROPERTY HAL_{{cookiecutter.target_name_capitals}}_${CA_HAL_{{cookiecutter.target_name_capitals}}_NAME_UPPER}_DESCRIPTION)
if (REPORTED_HAL_{{cookiecutter.target_name_capitals}}_DESCRIPTION)
  set(HAL_{{cookiecutter.target_name_capitals}}_DESCRIPTION "${REPORTED_HAL_{{cookiecutter.target_name_capitals}}_DESCRIPTION}")
endif()
message("-- ComputeAorta {{cookiecutter.target_name}} HAL description: ${HAL_{{cookiecutter.target_name_capitals}}_DESCRIPTION}")

# This is needed as cmake uses a known device name for calling clc for offline
# kernels
set(DEFAULT_{{cookiecutter.target_name_capitals}}_DEVICE_NAME "")
get_property(REPORTED_{{cookiecutter.target_name_capitals}}_HAL_DEVICE_NAME GLOBAL
  PROPERTY HAL_${CA_HAL_{{cookiecutter.target_name_capitals}}_NAME_UPPER}_DEVICE_NAME)
if (REPORTED_{{cookiecutter.target_name_capitals}}_HAL_DEVICE_NAME)
  set(DEFAULT_{{cookiecutter.target_name_capitals}}_DEVICE_NAME "${REPORTED_{{cookiecutter.target_name_capitals}}_HAL_DEVICE_NAME}")
endif()

# This is needed as cmake uses a known device name for calling clc for offline
# kernels
ca_option(CA_{{cookiecutter.target_name_capitals}}_DEVICE STRING 
  "Default {{cookiecutter.target_name_capitals}} device name"
  "${DEFAULT_{{cookiecutter.target_name_capitals}}_DEVICE_NAME}")

{%- if cookiecutter.bit_width == "32" %}
set({{cookiecutter.target_name_capitals}}_32_BITS ON)
{%- else %}
set({{cookiecutter.target_name_capitals}}_32_BITS OFF)
{%- endif %}

{%- if cookiecutter.capabilities_fp64 == "true" %}
set({{cookiecutter.target_name_capitals}}_FP64 ON)
{%- else %}
set({{cookiecutter.target_name_capitals}}_FP64 OFF)
{%- endif %}

add_ca_library({{cookiecutter.target_name}} STATIC
"${CMAKE_CURRENT_SOURCE_DIR}/source/hal.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/queue.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/device.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/device_info.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/device_info_get.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/fence.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/semaphore.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/buffer.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/command_buffer.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/query_pool.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/memory.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/executable.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/kernel.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/source/image.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/buffer.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/device.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/device_info.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/hal.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/memory.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/queue.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/fence.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/semaphore.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/command_buffer.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/device_info_get.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/executable.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/command_buffer.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/query_pool.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}/{{cookiecutter.target_name}}.h"
)

target_include_directories({{cookiecutter.target_name}} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_options({{cookiecutter.target_name}} PRIVATE
  $<$<BOOL:${UNIX}>:-Wno-unused-parameter>)
target_link_libraries({{cookiecutter.target_name}} PUBLIC
  cargo loader mux-hal mux-headers mux-utils tracer)

ca_option(CA_{{cookiecutter.target_name_capitals}}_DEMO_MODE BOOL "Enable ComputeAorta {{cookiecutter.target_name_capitals}} debug mode" OFF)
if(CA_{{cookiecutter.target_name_capitals}}_DEMO_MODE)
  target_compile_definitions({{cookiecutter.target_name}} PRIVATE CA_{{cookiecutter.target_name_capitals}}_DEMO_MODE=$<BOOL:${CA_{{cookiecutter.target_name_capitals}}_DEMO_MODE}>)
endif()

target_compile_definitions({{cookiecutter.target_name}} PRIVATE
  CA_HAL_DEFAULT_DEVICE="${CA_HAL_{{cookiecutter.target_name_capitals}}_NAME}")

target_link_libraries({{cookiecutter.target_name}} PUBLIC hal_common)
target_link_libraries({{cookiecutter.target_name}}
  PUBLIC
  cargo
  tracer
  compiler-pipeline
  compiler-binary-metadata
  utils
)
target_link_libraries({{cookiecutter.target_name}} PUBLIC loader)
if(NOT CA_HAL_{{cookiecutter.target_name_capitals}}_NAME STREQUAL "" AND TARGET "hal_${CA_HAL_{{cookiecutter.target_name_capitals}}_NAME}")
  target_link_libraries({{cookiecutter.target_name}} PUBLIC "hal_${CA_HAL_{{cookiecutter.target_name_capitals}}_NAME}")
endif()

ca_option(CA_HAL_{{cookiecutter.target_name_capitals}}_LOCK_DEVICE_NAME BOOL
    "Prevent dynamic loading of HAL libraries" FALSE)
target_compile_definitions({{cookiecutter.target_name}} PRIVATE
  CA_HAL_{{cookiecutter.target_name_capitals}}_LOCK_DEVICE_NAME=$<BOOL:${CA_HAL_{{cookiecutter.target_name_capitals}}_LOCK_DEVICE_NAME}>)

# List of capabilities that {{cookiecutter.target_name}} has.
set({{cookiecutter.target_name}}Capabilities)

if({{cookiecutter.target_name_capitals}}_32_BITS)
  list(APPEND {{cookiecutter.target_name}}Capabilities "32bit")
else()
  list(APPEND {{cookiecutter.target_name}}Capabilities "64bit")
endif()

if({{cookiecutter.target_name_capitals}}_FP64)
  list(APPEND {{cookiecutter.target_name}}Capabilities "fp64")
endif()

target_compile_definitions({{cookiecutter.target_name}} PRIVATE
  CA_{{cookiecutter.target_name_capitals}}_FP_64=$<BOOL:${{"{"}}{{cookiecutter.target_name_capitals}}_FP64}>)

target_compile_definitions({{cookiecutter.target_name}} PRIVATE
  CA_{{cookiecutter.target_name_capitals}}_32_BIT=$<BOOL:${{"{"}}{{cookiecutter.target_name_capitals}}_32_BITS}>)

add_mux_target({{cookiecutter.target_name}} CAPABILITIES ${{"{"}}{{cookiecutter.target_name}}Capabilities}
  HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/{{cookiecutter.target_name}}
  DEVICE_NAMES "${CA_{{cookiecutter.target_name_capitals}}_DEVICE}")

if(NOT CA_HAL_{{cookiecutter.target_name_capitals}}_NAME STREQUAL "" AND TARGET "hal_${CA_HAL_{{cookiecutter.target_name_capitals}}_NAME}")
  target_link_libraries({{cookiecutter.target_name}} PUBLIC "hal_${CA_HAL_{{cookiecutter.target_name_capitals}}_NAME}")
endif()
