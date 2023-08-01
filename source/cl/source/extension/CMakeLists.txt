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
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/extension.dir/source
    ${CMAKE_CURRENT_SOURCE_DIR}/include/extension
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/extension.dir/source
    ${CMAKE_CURRENT_SOURCE_DIR}/include/CL
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/extension.dir/source)
endif()

# List of all supported runtime extensions.
set(RUNTIME_EXTENSIONS
  codeplay_kernel_exec_info
  codeplay_performance_counters
  codeplay_soft_math
  intel_required_subgroup_size
  intel_unified_shared_memory
  khr_command_buffer
  khr_command_buffer_mutable_dispatch
  khr_create_command_queue
  khr_fp16
  khr_icd
  khr_opencl_c_1_2)
# For each external runtime extension, add the extension names.
foreach(tag ${CA_CL_RUNTIME_EXTENSION_TAGS})
  list(APPEND RUNTIME_EXTENSIONS ${${tag}_RUNTIME_EXTENSIONS})
endforeach()

# List of all supported compiler extensions.
set(COMPILER_EXTENSIONS
  codeplay_extra_build_options
  codeplay_kernel_debug
  codeplay_wfv
  khr_extended_async_copies
  khr_il_program)
# For each external compiler extension, all the extension names.
foreach(tag ${CA_CL_COMPILER_EXTENSION_TAGS})
  list(APPEND COMPILER_EXTENSIONS ${${tag}_COMPILER_EXTENSIONS})
endforeach()

# Create an individual option for each extension in the list.
foreach(extension ${RUNTIME_EXTENSIONS} ${COMPILER_EXTENSIONS})
  if(${extension} STREQUAL "khr_command_buffer"
    OR ${extension} STREQUAL "khr_command_buffer_mutable_dispatch")
    # TODO Enable `khr_command_buffer` and any layered extensions
    # by default once complete.
    ca_option(OCL_EXTENSION_cl_${extension} BOOL
        "OpenCL extension cl_${extension}" OFF)
  else()
    ca_option(OCL_EXTENSION_cl_${extension} BOOL
      "OpenCL extension cl_${extension}" ON)
  endif()
endforeach()

if(OCL_EXTENSION_cl_khr_command_buffer_mutable_dispatch
  AND NOT OCL_EXTENSION_cl_khr_command_buffer)
  # The cl_khr_command_buffer_mutable_dispatch extension has a dependency on
  # cl_khr_command_buffer.
  message(FATAL_ERROR
    "cl_khr_command_buffer_mutable_dispatch requires cl_khr_command_buffer to"
    " be enabled")
endif()

# Collect all the extension headers that need to be installed.
set(CL_EXTENSION_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/include/CL/cl_ext_codeplay.h)

foreach(tag ${CA_CL_RUNTIME_EXTENSION_TAGS})
  list(APPEND CL_EXTENSION_HEADERS ${${tag}_RUNTIME_HEADER})
endforeach()
foreach(tag ${CA_CL_COMPILER_EXTENSION_TAGS})
  list(APPEND CL_EXTENSION_HEADERS ${${tag}_COMPILER_HEADER})
endforeach()
list(REMOVE_DUPLICATES CL_EXTENSION_HEADERS)

# Generate config.h and extension.cpp
set(generate_extension_defines)
set(generate_includes)
set(generate_declarations)
set(generate_alloc_error)

string(APPEND generate_extension_defines "
// OpenCL runtime extensions.")
foreach(extension ${RUNTIME_EXTENSIONS})
  if(OCL_EXTENSION_cl_${extension})
    string(APPEND generate_extension_defines "
#define OCL_EXTENSION_cl_${extension}")
  else()
    string(APPEND generate_extension_defines "
// #undef OCL_EXTENSION_cl_${extension}")
  endif()
  string(APPEND generate_includes "
#include <extension/${extension}.h>")
  string(APPEND generate_declarations "
  static extension::${extension} ${extension};")
  string(APPEND generate_alloc_error "
    error = extensions.push_back(&${extension});
    OCL_ASSERT(!error, \"failed to allocate extension storage\");")
endforeach()

foreach(extension ${COMPILER_EXTENSIONS})
  if(OCL_EXTENSION_cl_${extension})
    string(APPEND generate_extension_defines "
#define OCL_EXTENSION_cl_${extension}")
  else()
    string(APPEND generate_extension_defines "
// #undef OCL_EXTENSION_cl_${extension}")
  endif()
  string(APPEND generate_includes "
#include <extension/${extension}.h>")
  string(APPEND generate_declarations "
  static extension::${extension} ${extension};")
  string(APPEND generate_alloc_error "
    error = extensions.push_back(&${extension});
    OCL_ASSERT(!error, \"failed to allocate extension storage\");")
endforeach()

list(LENGTH RUNTIME_EXTENSIONS CA_CL_RUNTIME_EXTENSION_COUNT)
list(LENGTH COMPILER_EXTENSIONS CA_CL_COMPILER_EXTENSION_COUNT)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/config.h.in
  ${PROJECT_BINARY_DIR}/include/extension/config.h)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/source/extension.cpp.in
  ${PROJECT_BINARY_DIR}/source/cl/source/extension/source/extension.cpp)

set(EXTENSION_RUNTIME_SOURCES
  ${PROJECT_BINARY_DIR}/include/extension/config.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/codeplay_extra_build_options.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/codeplay_kernel_debug.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/codeplay_kernel_exec_info.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/codeplay_performance_counters.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/codeplay_soft_math.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/codeplay_wfv.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/intel_unified_shared_memory.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/extension.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/khr_command_buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/khr_fp16.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/khr_icd.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/khr_command_buffer_mutable_dispatch.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/khr_opencl_c_1_2.h
  ${CMAKE_CURRENT_BINARY_DIR}/source/extension.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/codeplay_extra_build_options.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/codeplay_kernel_debug.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/codeplay_kernel_exec_info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/codeplay_performance_counters.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/codeplay_soft_math.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/codeplay_wfv.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/intel_unified_shared_memory/intel_unified_shared_memory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/intel_unified_shared_memory/usm-exports.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/khr_command_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/khr_create_command_queue.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/khr_fp16.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/khr_icd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/khr_command_buffer_mutable_dispatch.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/khr_opencl_c_1_2.cpp)
foreach(tag ${CA_CL_RUNTIME_EXTENSION_TAGS})
  list(APPEND EXTENSION_RUNTIME_SOURCES ${${tag}_RUNTIME_SOURCES})
endforeach()

set(EXTENSION_COMPILER_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/intel_required_subgroup_size.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/khr_extended_async_copies.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/khr_il_program.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/intel_required_subgroup_size.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/khr_extended_async_copies.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/khr_il_program.cpp)
foreach(tag ${CA_CL_COMPILER_EXTENSION_TAGS})
  list(APPEND EXTENSION_COMPILER_SOURCES ${${tag}_COMPILER_SOURCES})
endforeach()

add_ca_library(extension STATIC
  ${EXTENSION_RUNTIME_SOURCES}
  ${EXTENSION_COMPILER_SOURCES})

# Add the default include directory for use by OpenCL applications.
set(CA_CL_EXTENSION_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Add the include directories for all runtime extensions.
foreach(tag ${CA_CL_RUNTIME_EXTENSION_TAGS})
  list(APPEND CA_CL_EXTENSION_INCLUDE_DIRS ${${tag}_RUNTIME_INCLUDE_DIRS})
endforeach()
# Add the include directories for all compiler extensions.
foreach(tag ${CA_CL_COMPILER_EXTENSION_TAGS})
  list(APPEND CA_CL_EXTENSION_INCLUDE_DIRS ${${tag}_COMPILER_INCLUDE_DIRS})
endforeach()
# Store the include directories in the cache for the CL target to use.
set(CA_CL_EXTENSION_INCLUDE_DIRS ${CA_CL_EXTENSION_INCLUDE_DIRS} CACHE INTERNAL
  "List of OpenCL extension include directories" FORCE)

target_include_directories(extension SYSTEM PUBLIC ${CL_INCLUDE_DIR})
target_include_directories(extension
  PUBLIC ${CA_CL_EXTENSION_INCLUDE_DIRS}
  PRIVATE ${PROJECT_SOURCE_DIR}/source
  ${PROJECT_SOURCE_DIR}/source/cl/include
  ${PROJECT_BINARY_DIR}/source/cl/include)
target_link_libraries(extension PUBLIC
  builtins cargo CL-binary compiler-loader mux)

# Append to the list of module libraries, the cache MUST be updated.
list(APPEND MODULES_LIBRARIES extension)
set(MODULES_LIBRARIES ${MODULES_LIBRARIES}
  CACHE INTERNAL "List of module libraries.")

# Install enabled extension headers.
install(FILES ${CL_EXTENSION_HEADERS} DESTINATION include/CL COMPONENT OCL)
