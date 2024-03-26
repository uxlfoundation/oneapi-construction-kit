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

cmake_minimum_required(VERSION 3.2.2 FATAL_ERROR)
project(image_library VERSION 1.0.0)

# If the online coverage is enabled we add the modules so that the XML file
# can be generated automatically.
if(${CA_ENABLE_COVERAGE} AND ${CA_RUNTIME_COMPILER_ENABLED})
  add_coverage_modules(${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/image_library_host.dir/source
    ${CMAKE_CURRENT_SOURCE_DIR}/include/libimg
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/image_library_host.dir/source)
endif()

# The following options are used to control the outputs of builds.
option(CODEPLAY_IMG_BUILD_KERNEL_LIBRARY
  "Enable 'image_library_kernel' static library." OFF)
option(CODEPLAY_IMG_EXPORT_KERNEL_SOURCES
  "Enable export of CMake variables pointing to kernel source files." OFF)
option(CODEPLAY_IMG_ENABLE_DOCUMENTATION
  "Enable generation of image library documentation." OFF)
option(CODEPLAY_IMG_ENABLE_INSTALL "Enable libimg install target." ON)

# Ensure that all required options have been set by the client.
if("${CODEPLAY_IMG_INTEGRATION_INCLUDE_DIRS}" STREQUAL "")
  message(FATAL_ERROR
    "CODEPLAY_IMG_REQUIRED_INCLUDE_DIRS must point to the list of required "
    "include directories.")
endif()
foreach(INTEGRATION_INCLUDE_DIR IN LISTS CODEPLAY_IMG_INTEGRATION_INCLUDE_DIRS)
  if(EXISTS ${INTEGRATION_INCLUDE_DIR}/image_library_integration.h)
    set(CODEPLAY_IMG_INTEGRATION_HEADER_PATH
      ${INTEGRATION_INCLUDE_DIR}/image_library_integration.h)
  endif()
endforeach()
if("${CODEPLAY_IMG_INTEGRATION_HEADER_PATH}" STREQUAL "")
  message(FATAL_ERROR
    "The 'image_library_integration.h' header was not found on any of the "
    "include directories passed to CODEPLAY_IMG_INTEGRATION_INCLUDE_DIRS "
    "please make sure the path to this file is correctly set. The integration "
    "headers contents are described in the README.md")
endif()
# TODO(CA-600): libimg should not depend on the OpenCL headers
if("${CODEPLAY_IMG_OPENCL_INCLUDE_DIR}" STREQUAL "")
  message(FATAL_ERROR
    "CODEPLAY_IMG_OPENCL_INCLUDE_DIR must point to the OpenCL header "
    "directory.")
endif()

# Specify common source files
set(CODEPLAY_IMG_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CODEPLAY_IMG_INTEGRATION_INCLUDE_DIRS})
set(CODEPLAY_IMG_HOST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/libimg/shared.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/libimg/host.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/libimg/validate.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/host.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/validate.cpp)

# Default host static library.
add_library(image_library_host STATIC ${CODEPLAY_IMG_HOST_SOURCES})

# If extra ComputeAorta commands exist, use them.
if(COMMAND add_ca_tidy)
  add_ca_tidy(image_library_host ${CODEPLAY_IMG_HOST_SOURCES} DEPENDS abacus_generate)
endif()

target_include_directories(image_library_host PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  ${CODEPLAY_IMG_INTEGRATION_INCLUDE_DIRS}
  "${CODEPLAY_IMG_INCLUDE_DIRS}")
target_include_directories(image_library_host SYSTEM PUBLIC
  "${CODEPLAY_IMG_OPENCL_INCLUDE_DIR}")
target_compile_options(image_library_host
  PRIVATE ${CA_COMPILE_OPTIONS})

target_compile_definitions(image_library_host
  PUBLIC CL_TARGET_OPENCL_VERSION=${CA_CL_STANDARD_INTERNAL}
  PRIVATE ${CA_COMPILE_DEFINITIONS})
target_link_libraries(image_library_host PRIVATE abacus_static cargo)
if(${CODEPLAY_IMG_ENABLE_INSTALL})
  install(TARGETS image_library_host ARCHIVE DESTINATION lib)
endif()

# Kernel static library.
if(${CODEPLAY_IMG_BUILD_KERNEL_LIB})
  message(STATUS
    "Image library building 'image_library_kernel' static library.")
  add_library(image_library_kernel STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/libimg/shared.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/libimg/kernel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/kernel.cpp)
  set_target_properties(image_library_kernel PROPERTIES
    INCLUDE_DIRECTORIES "${CODEPLAY_IMG_INCLUDE_DIRS}")
  set_property(TARGET image_library_kernel APPEND PROPERTY
    COMPILE_FLAGS "-fPIC -std=c++11")
  if(${CODEPLAY_IMG_ENABLE_INSTALL})
    install(TARGETS image_library_kernel ARCHIVE DESTINATION lib)
  endif()
endif()

# Kernel source files in cached CMake strings.
if(${CODEPLAY_IMG_EXPORT_KERNEL_SOURCES})
  set(CODEPLAY_IMG_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    CACHE PATH "Path to image library header include directory.")
  set(CODEPLAY_IMG_KERNEL_HEADER_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/include/libimg/kernel.h
    CACHE STRING "Path to image library kernel header file.")
  set(CODEPLAY_IMG_KERNEL_SOURCE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/source/kernel.cpp
    CACHE PATH "Path to image library kernel source file.")
endif()

# Generate documentation with Doxygen
if(${CODEPLAY_IMG_ENABLE_DOCUMENTATION})
  find_package(Doxygen)
  if(${DOXYGEN_FOUND})
    message(STATUS
      "Image library documentation will be generated with Doxygen.")
    configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/documentation/Doxyfile.in
      ${CMAKE_CURRENT_BINARY_DIR}/documentation/Doxyfile @ONLY)
    add_custom_target(image_library_documentation
      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/documentation/Doxyfile
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating image library API documentation with Doxygen."
      VERBATIM)
  else()
    message(WARNING "Image library documentation has been enabled but Doxygen "
      "was not found, no documentation will be generated!")
  endif()
endif()
