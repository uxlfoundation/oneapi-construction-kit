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

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(ComputeAorta VERSION 4.0.0 LANGUAGES C CXX ASM)

# There's no situation where it's necessary to *not* generate this file and the
# time difference to configure is about 0.1s
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "oneAPI Construction Kit does not support in-tree builds!")
endif()

# GCC 4.8 is known to have bugs that cause test failures, we require at least
# GCC 5.1.  Minimum to build LLVM-10 is 5.1.
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.1)
  message(FATAL_ERROR
    "GNU Compiler version ${CMAKE_CXX_COMPILER_VERSION} not supported")
endif()

# Older versions of Clang may work, but this is the oldest version that we
# test.  Minimum to build LLVM-10 is 3.5.
if(CMAKE_CXX_COMPILER_ID STREQUAL Clang AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
  message(FATAL_ERROR
    "Clang version ${CMAKE_CXX_COMPILER_VERSION} not supported")
endif()

# oneAPI Construction Kit minimum MSVC is 19.0 (2015).  Minimum to build LLVM-10 is 19.10
# (MSVC 2017).
if(CMAKE_CXX_COMPILER_ID STREQUAL MSVC AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.0)
  message(FATAL_ERROR
    "MSVC version ${CMAKE_CXX_COMPILER_VERSION} not supported")
endif()

if(MSVC AND (CMAKE_SIZEOF_VOID_P EQUAL 4) AND
    (CMAKE_SYSTEM_PROCESSOR STREQUAL x86 OR
        CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64 OR
        CMAKE_SYSTEM_PROCESSOR STREQUAL AMD64))
  message(FATAL_ERROR "Windows 32 bit is no longer supported by oneAPI Construction Kit")
endif()

# Add cmake modules directory to the path.
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Bring in our ca_option macro for declaring options.
include(CAOption)

# Bring in various settings based on the host or target platform.
include(CAPlatform)

message(STATUS "oneAPI Construction Kit version: ${PROJECT_VERSION}")
find_program(GIT_EXECUTABLE git${CA_HOST_EXECUTABLE_SUFFIX})
if(NOT GIT_EXECUTABLE STREQUAL GIT_EXECUTABLE-NOTFOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} log -1 --no-show-signature --format=%h
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE CA_GIT_COMMIT)
  message(STATUS "oneAPI Construction Kit Git Commit: ${CA_GIT_COMMIT}")
endif()

ca_option(CA_LLVM_INSTALL_DIR STRING
  "LLVM install directory" "")
ca_option(CA_ENABLE_API STRING
  "Semi-colon separated list of APIs to enable, empty enables cl and vk" "cl;vk")
ca_option(CA_ENABLE_TESTS BOOL
  "Enable building tests" ON)
ca_option(CA_ENABLE_EXAMPLES BOOL
  "Enable building example programs" ON)
ca_option(CA_ENABLE_HOST_IMAGE_SUPPORT BOOL
  "Enable host image support" OFF)
ca_option(CA_ENABLE_OUT_OF_ORDER_EXEC_MODE BOOL
  "Enable out of order execution mode" ON)
ca_immutable_option(CA_ENABLE_DEBUG_SUPPORT BOOL
  "Enable debug support regardless of build type" OFF)
ca_option(CA_ENABLE_DEBUG_BACKTRACE BOOL
  "Enable debug backtrace support" OFF)
ca_option(CA_ENABLE_LLVM_OPTIONS_IN_RELEASE BOOL
  "Enable support for CA_LLVM_OPTIONS regardless of build type" OFF)
ca_option(CA_ENABLE_COVERAGE BOOL
  "Enable driver source coverage" OFF)
ca_option(CA_EXTERNAL_BUILTINS BOOL
  "Enable external builtins, always on during cross compilation"
  CMAKE_CROSSCOMPILING)
if(CA_PLATFORM_WINDOWS)
  ca_option(CA_HOST_ENABLE_FP64 BOOL "Enable host cl_khr_fp64" OFF)
else()
  ca_option(CA_HOST_ENABLE_FP64 BOOL "Enable host cl_khr_fp64" ON)
endif()
ca_option(CA_HOST_ENABLE_FP16 BOOL "Enable host cl_khr_fp16" OFF)
set(fp16_supported_arches ARM64;ARM)
if(CA_HOST_ENABLE_FP16)
  set(arch ${CMAKE_SYSTEM_PROCESSOR})
  if(NOT ${arch} IN_LIST fp16_supported_arches)
    message(WARNING "half-precision is not supported on ${arch} targets")
  endif()
endif()
ca_option(CA_ENABLE_DOCUMENTATION BOOL
  "Enable documentation generation" ON)
ca_option(CA_MUX_TARGETS_TO_ENABLE STRING
  "Semi-colon separated list of ComputeMux targets to enable, empty enables all" "")
ca_option(CA_EXTERNAL_MUX_TARGET_DIRS STRING
  "Semi-colon separated list of external ComputeMux target source directories" "")
ca_option(CA_MUX_COMPILERS_TO_ENABLE STRING
  "Semi-colon separated list of ComputeMux compilers to enable, empty enables all" "")
ca_option(CA_EXTERNAL_MUX_COMPILER_DIRS STRING
  "Semi-colon separated list of external ComputeMux compiler source directories" "")
ca_immutable_option(CA_RUNTIME_COMPILER_ENABLED BOOL
  "Enable runtime compiler support" ON)
ca_option(CA_COMPILER_ENABLE_DYNAMIC_LOADER BOOL
  "Enable compiler library dynamic loader" OFF)
ca_option(CA_EXTERNAL_CLC STRING
  "Location of external clc" "")
ca_option(CA_GTEST_LAUNCHER STRING
  "Semi-color separated command for launching googletest suites" "")

ca_option(CA_EXTERNAL_LLVM_SPIRV STRING
  "Location of external llvm-spirv tool." "")

ca_option_linux(CA_BUILD_32_BITS BOOL
  "Enable building 32 bit Linux binaries" OFF)
ca_option_linux(CA_USE_LINKER STRING
  "Build using a non-standard linker" "")

ca_option_windows(CA_DISABLE_DEBUG_ITERATOR BOOL
  "Disable MSVC debug iterator support" OFF)

# Add support for clang sanitizer.
# TODO: Simplify enabling sanitizer builds by using the CMAKE_BUILD_TYPE
# method, this will also simplify scripts/build.py.
ca_option(CA_USE_SANITIZER STRING
  "Enable sanitizer, supported sanitizers: ${CA_SUPPORTED_SANITIZERS}" "")
ca_option(CA_USE_SPLIT_DWARF BOOL
  "Use split DWARF debuginfo for faster link times and smaller executables"
  ON)

if(CMAKE_MAKE_PROGRAM MATCHES ninja)
  ca_option(CA_PARALLEL_LINK_JOBS STRING
    "Number of parallel link jobs (Ninja only)." "2")
  if(CA_PARALLEL_LINK_JOBS)
    set_property(GLOBAL APPEND PROPERTY
      JOB_POOLS link_job_pool=${CA_PARALLEL_LINK_JOBS})
    set(CMAKE_JOB_POOL_LINK link_job_pool)
  endif()
endif()

set(OPTIONS_MODULES
  ${CMAKE_CURRENT_SOURCE_DIR}/source/cl/cmake/Options.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ur/cmake/Options.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vk/cmake/Options.cmake)
foreach(OPTIONS_MODULE ${OPTIONS_MODULES})
  if(EXISTS ${OPTIONS_MODULE})
    include(${OPTIONS_MODULE})
  endif()
endforeach()

# Check if we are in an LLVM Tree.
# This is done by checking for the LLVMCore target which does not exist
# until we import LLVM.
# This only happens of we have not imported or are fetched by LLVM via
# FetchContent or as an external project. In this case we set a cmake
# variable OCK_IN_LLVM_TREE to TRUE.
set(OCK_IN_LLVM_TREE FALSE)
if (TARGET LLVMCore)
  set(OCK_IN_LLVM_TREE TRUE)
endif()

if(CA_RUNTIME_COMPILER_ENABLED AND NOT OCK_IN_LLVM_TREE)
  include(ImportLLVM)
endif()

# Bring in our sanitizer support.
include(Sanitizers)
# Add the ability to use CMAKE_BUILD_TYPE=ReleaseAssert enabling assertions
# in Release mode or enable assertions when LLVM_ENABLE_ASSERTIONS is set.
include(ReleaseAssert)

# Find python on the host system, this is a required dependency used for
# testing and generation of source code.
find_package(PythonInterp 3.6 REQUIRED)
# Find clang tools on the host system.
# If clang-format is found this will enable targets, including the format
# target, which generate source code which may be committed to the repo.
# If clang-tidy is found this will enable targets, including the tidy target,
# which perform static analysis and style checking on source files.
# When updating the version here, also update that used in the merge request
# config
find_package(ClangTools 17 COMPONENTS clang-format clang-tidy)
if(TARGET ClangTools::clang-tidy)
  ca_option(CA_CLANG_TIDY_FLAGS STRING
    "Semi-color separated list of clang-tidy flags" "")
endif()

# Find the operating systems threading library.
find_package(Threads REQUIRED)

# Add format target to automatically format modified source files.
include(Format)

if(CA_USE_LINKER)
  message(STATUS "Using linker '${CA_USE_LINKER}'")
endif()

# Set default cmake build type to Release.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()

if(CA_ENABLE_COVERAGE)
  message(STATUS "Enabled Coverage")
  # Include the Coverage CMake module to allow generation of code coverage
  # information.
  include(Coverage)
endif()

# Only enable use of install RPATH on Linux which supports using a relative
# RPATHs. On other platforms which only support absolute RPATHs using the
# install RPATH for the build breaks executables which load shrared libraries.
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

# Include utility functions for adding executables, libraries, etc. with
# consistent compile options, definitions, and output directories.
include(AddCA)

# Check if sanitizer settings are required.
if(NOT CA_USE_SANITIZER STREQUAL "")
  ca_enable_sanitizer("${CA_USE_SANITIZER}")
endif()

# Add the resource compiler module for embedding binary data into executables
# and shared libraries.
include(ResourceCompiler)

# Enable ability to create project sub-folders using source_group()
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# TODO: Everything below here can probably be moved

# For android native API levels under 19, FUTIMENS is not available, but needed
# by LLVM. Not having them, however, is an option.
if(CA_PLATFORM_ANDROID)
  if(ANDROID_NATIVE_API_LEVEL LESS 19)
    set(HAVE_FUTIMENS false)
  endif()

  # This is needed to work around lack of 'log2' on older API levels.
  add_definitions(-D__ANDROID_NDK__)
endif()

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

set(CA_INTERNAL_BUILTINS_DIR ${PROJECT_BINARY_DIR}/modules/compiler/builtins)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/modules/compiler/builtins)

if(CA_EXTERNAL_BUILTINS)
  # Make sure we were given the builtins directories
  if(NOT EXISTS "${CA_EXTERNAL_BUILTINS_DIR}")
    message(FATAL_ERROR "Need to set CA_EXTERNAL_BUILTINS_DIR to a valid "
      "pre-built builtins directory! Actual value: "
      "${CA_EXTERNAL_BUILTINS_DIR}")
  endif()

  set(CA_BUILTINS_DIR ${CA_EXTERNAL_BUILTINS_DIR})
else()
  set(CA_BUILTINS_DIR ${CA_INTERNAL_BUILTINS_DIR})
endif()

# An external clc is required only if offline kernel tests are built. This is
# determined later in UnitCL/CMakeLists.txt.
if(CA_EXTERNAL_CLC)
  if(CA_RUNTIME_COMPILER_ENABLED AND NOT CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR "CA_EXTERNAL_CLC may only be set if "
      "CA_RUNTIME_COMPILER_ENABLED=OFF or when cross-compiling")
  endif()
endif()

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/include)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/include/ocl)

if(CA_ENABLE_TESTS)
  # Open up an active set of 'all' lit suites
  ca_umbrella_lit_testsuite_open(all)
endif()

add_subdirectory(hal)
add_subdirectory(external)
add_subdirectory(source)
if(CA_ENABLE_DOCUMENTATION)
  add_subdirectory(doc)
endif()

# Add the deferred add_ca_extension_subdirectory() entries until all
# dependencies are available for use in the extension's CMake.
foreach(exampleDir ${CA_EXAMPLE_DIRS})
  add_subdirectory(${exampleDir})
endforeach()

function(add_to_group group)
  if(NOT TARGET ${group})
    add_custom_target(${group})
  endif()
  foreach(target ${ARGN})
    if(TARGET ${target})
      add_dependencies(${group} ${target})
    endif()
  endforeach()
endfunction()

add_to_group(ComputeAorta
  cmakelint host-lit lit-deps oclc OpenCLCTS
  UnitCL UnitCargo UnitCore UnitMux UnitCompiler UnitVK UnitMD FuzzCL
  clVectorAddition vkVectorAddition veczc UnitUR urVectorAddition)

if(CA_ENABLE_TESTS)
  # Ensure we've created the global check target
  if(NOT TARGET ${OCK_CHECK_TARGET})
    message(FATAL_ERROR
      "CA_ENABLE_TESTS is enabled but no OCK check target was created")
  endif()

  # Inject an extra dependency into the OCK global check target so that it
  # builds the ComputeAorta target. This turns it into more of a build+test
  # target than a pure test target.
  add_dependencies(${OCK_CHECK_TARGET} ComputeAorta)
  # Add a legacy target called 'check-ComputeAorta' which aliases the OCK
  # global check target. This should be considered deprecated.
  add_custom_target(check-ComputeAorta DEPENDS ${OCK_CHECK_TARGET})

  if(TARGET tidy)
    # Add an extra 'tidy' target which additionally depends on the tidy target
    add_ca_check_group(tidy NOGLOBAL DEPENDS ${OCK_CHECK_TARGET} tidy)

    # Add a legacy target called 'check-tidy-ComputeAorta' which aliases the OCK
    # tidy target. This should be considered deprecated.
    get_ock_check_name(tidy_check_name tidy)
    add_custom_target(check-tidy-ComputeAorta DEPENDS ${tidy_check_name})
  endif()

  if(CMAKE_CROSSCOMPILING)
    # Create an OCK cross check target to build+test fast subset of checks.
    set(check_deps ComputeAorta)
    foreach(target host-lit;spirv-ll-lit;vecz-lit;UnitCargo;UnitMux;UnitCL-offline)
      if(TARGET ${target})
        get_ock_check_name(check_name ${target})
        list(APPEND check_deps ${check_name})
      endif()
    endforeach()
    add_ca_check_group(cross NOGLOBAL DEPENDS ${check_deps})

    # Add a legacy target called 'check-cross-ComputeAorta' which aliases the OCK
    # cross target. This should be considered deprecated.
    get_ock_check_name(cross_check_name cross)
    add_custom_target(check-cross-ComputeAorta DEPENDS ${cross_check_name})
  endif()

  # Close off the active set of 'all' lit suites
  ca_umbrella_lit_testsuite_close(all)

  # Close off any open target test suites
  get_property(target_testsuites GLOBAL PROPERTY CA_TARGET_LIT_TESTSUITES)
  list(REMOVE_DUPLICATES target_testsuites)
  foreach(name ${target_testsuites})
    ca_umbrella_lit_testsuite_close(${name})
  endforeach()
endif()

if(${CA_ENABLE_COVERAGE})
  # Generate the XML input file for coverage and add the custom coverage target
  add_coverage_xml_input()
  add_coverage_custom_target()
endif()
