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

# If the online coverage is enabled we add the modules so that the XML file
# can be generated automatically.
if(${CA_ENABLE_COVERAGE} AND ${CA_RUNTIME_COMPILER_ENABLED})
  add_coverage_modules(${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/tracer.dir/source
    ${CMAKE_CURRENT_SOURCE_DIR}/include/tracer
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/tracer.dir/source)
endif()

add_ca_library(tracer STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/tracer/tracer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/tracer.cpp)

target_include_directories(tracer PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# Toggle various tracing options
ca_option(CA_TRACE_CL BOOL "Enable tracing OpenCL entry points" OFF)
ca_option(CA_TRACE_CORE BOOL "Enable tracing Core details" OFF)
ca_option(CA_TRACE_IMPLEMENTATION BOOL "Enable tracing of the Implementation details" OFF)

target_compile_definitions(tracer PUBLIC
  $<$<BOOL:${CA_TRACE_CL}>:CA_TRACE_CL=1>
  $<$<BOOL:${CA_TRACE_CORE}>:CA_TRACE_CORE=1>
  $<$<BOOL:${CA_TRACE_IMPLEMENTATION}>:CA_TRACE_IMPLEMENTATION=1>)

target_link_libraries(tracer PRIVATE utils)

# Append to the list of module libraries, the cache MUST be updated.
list(APPEND MODULES_LIBRARIES tracer)
set(MODULES_LIBRARIES ${MODULES_LIBRARIES}
  CACHE INTERNAL "List of module libraries.")
