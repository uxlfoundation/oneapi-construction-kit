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
.. cmake:variable:: CA_HOST_ENABLED

  Option for enabling host Mux target, defaults to on.
#]=======================================================================]
ca_option(CA_HOST_ENABLED BOOL "Enable host mux target" ON)
if(NOT CA_HOST_ENABLED)
  return()
endif()

#[=======================================================================[.rst:
.. cmake:variable:: CA_HOST_ENABLE_BUILTIN_KERNEL

  Enable host reporting of builtin kernels. Defaults to the value of
  :cmake:variable:`CA_ENABLE_DEBUG_SUPPORT` as builtin kernels are implemented
  in software on host for testing purposes.
#]=======================================================================]
ca_option(CA_HOST_ENABLE_BUILTIN_KERNEL BOOL
  "Enable host builtin kernel support" ${CA_ENABLE_DEBUG_SUPPORT})

#[=======================================================================[.rst:
.. cmake:variable:: CA_HOST_ENABLE_BUILTINS_EXTENSION

  Enable host specific extension ``cl_codeplay_host_builtins`` for builds
  where a compiler is available. Defaults to the value of
  :cmake:variable:`CA_ENABLE_DEBUG_SUPPORT` as the extension is primarily
  for testing the target specific CL extension mechanism.
#]=======================================================================]
if(CA_RUNTIME_COMPILER_ENABLED)
  ca_option(CA_HOST_ENABLE_BUILTINS_EXTENSION BOOL
    "Enable cl_codeplay_host_builtins extension" ${CA_ENABLE_DEBUG_SUPPORT})
else()
  # Override even if the user has set the option with -D
  set(CA_HOST_ENABLE_BUILTINS_EXTENSION FALSE CACHE BOOL
    "cl_codeplay_host_builtins is disabled in offline-only builds" FORCE)
endif()

#[=======================================================================[.rst:
.. cmake:variable:: CA_HOST_ENABLE_PAPI_COUNTERS

  Enable counter type query pools in Host, supported with PAPI. Requires an
  installation of the PAPI library and headers.
#]=======================================================================]
ca_option(CA_HOST_ENABLE_PAPI_COUNTERS BOOL
  "Enable PAPI counter based queries in host." OFF)

# If the online coverage is enabled we add the modules so that the XML file
# can be generated automatically.
if(${CA_ENABLE_COVERAGE} AND ${CA_RUNTIME_COMPILER_ENABLED})
  set(COVERAGE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/host.dir/source
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/host.dir/source)
  add_coverage_modules(${COVERAGE_DIRS})
endif()

add_ca_library(host STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/host/buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/host/builtin_kernel.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/host/command_buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/host/device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/host/executable.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/host/fence.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/host/host.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/host/image.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/host/kernel.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/host/memory.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/host/metadata_hooks.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/host/query_pool.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/host/queue.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/host/semaphore.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/host/thread_pool.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/builtin_kernel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/command_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/executable.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/fence.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/image.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/kernel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/memory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/metadata_hooks.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/query_pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/queue.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/semaphore.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/thread_pool.cpp
  DEPENDS mux-config abacus_generate)

target_include_directories(host PUBLIC
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_definitions(host PUBLIC
  $<$<BOOL:${CA_HOST_ENABLE_BUILTIN_KERNEL}>:CA_HOST_ENABLE_BUILTIN_KERNEL>)
target_compile_options(host PRIVATE
  $<$<BOOL:${UNIX}>:-Wno-unused-parameter>)
target_link_libraries(host PUBLIC
  cargo host-utils loader mux-headers mux-utils tracer md_handler
  utils  # TODO: remove this dependency
  Threads::Threads)

if(CA_HOST_ENABLE_PAPI_COUNTERS)
  if(CA_PLATFORM_WINDOWS)
    message(FATAL_ERROR "PAPI counters are not supported on Windows!")
  endif()
  find_package(PAPI REQUIRED)
  target_link_libraries(host PRIVATE papi::papi)
  target_compile_definitions(host PRIVATE
    CA_HOST_ENABLE_PAPI_COUNTERS)
  target_sources(host PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host/papi_error_codes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host/papi_counter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/papi_counter.cpp)
endif()

# List of capabilities that host has.
set(hostCapabilities)

# Host is set to have whatever bit width the host machine has, but a Linux user
# can force a 32-bit build.
if(CA_BUILD_32_BITS OR ${CMAKE_SIZEOF_VOID_P} EQUAL 4)
  list(APPEND hostCapabilities "32bit")
else()
  list(APPEND hostCapabilities "64bit" "atomic64")
endif()

if(CA_ENABLE_HOST_IMAGE_SUPPORT)
  message(WARNING "ComputeAorta supports images in a way that is conformant "
                  "only to OpenCL 1.2. Support for other versions 3.0 is "
                  "known not to be conformant.")
  target_link_libraries(host PUBLIC image_library_host)
  target_compile_definitions(host PRIVATE HOST_IMAGE_SUPPORT)
endif()

if(CA_HOST_ENABLE_FP64)
  list(APPEND hostCapabilities "fp64")
  target_compile_definitions(host PRIVATE CA_HOST_ENABLE_FP64)
endif()

if(CA_HOST_ENABLE_FP16)
  list(APPEND hostCapabilities "fp16")
  target_compile_definitions(host PRIVATE CA_HOST_ENABLE_FP16)
endif()

set(host_CA_HOST_CL_DEVICE_NAME)
get_ca_host_device_name(host_CA_HOST_CL_DEVICE_NAME)

# Provide CA_HOST_CL_DEVICE_NAME to the host device implementation.
target_compile_definitions(host PRIVATE
  CA_HOST_CL_DEVICE_NAME="${host_CA_HOST_CL_DEVICE_NAME}")

add_mux_target(host CAPABILITIES ${hostCapabilities}
  HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/host
  DEVICE_NAMES "${host_CA_HOST_CL_DEVICE_NAME}")

if(CA_ENABLE_API MATCHES "(^$|cl)")
  add_subdirectory(extension/cl_ext_codeplay)
  if(CA_ENABLE_EXAMPLES)
    add_ca_example_subdirectory(extension/example)
  endif()
endif()

if(CA_ENABLE_TESTS)
  # Include the test subdirectory for any additional Host tests
  add_subdirectory(test)
endif()
