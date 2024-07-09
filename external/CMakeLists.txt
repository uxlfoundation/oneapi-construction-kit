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

include(FetchContent)

# Enable C and C++ language extensions which we otherwise disable, as
# external projects are entitled to assume CMake defaults.
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_EXTENSIONS ON)

# OpenCL Headers are used outside of source/cl, mostly in libimg, so make
# them available here rather here rather than in source/cl/external.
FetchContent_Declare(
  OpenCLHeaders
  GIT_REPOSITORY https://github.com/KhronosGroup/OpenCL-Headers.git
  GIT_TAG        v2024.05.08
)
FetchContent_MakeAvailable(OpenCLHeaders)

# TODO We should be using the OpenCL::Headers library rather than manually
# adding the include directories everywhere.
set(CL_INCLUDE_DIR "${openclheaders_SOURCE_DIR}" PARENT_SCOPE)

# The googletest CMake does not set the project VERSION which causes verbose
# warnings about CMake policies, instead create our own targets for googletest.

if(CA_ENABLE_TESTS)
  # Link against ca_gtest if the test suite defines it's own main function, e.g.
  # if custom command line arguments are required.
  add_ca_library(ca_gtest STATIC googletest/src/gtest-all.cc)
  set_target_properties(ca_gtest PROPERTIES CXX_STANDARD 14)
  target_include_directories(ca_gtest SYSTEM
    PUBLIC googletest/include PRIVATE googletest)
  target_compile_definitions(ca_gtest PUBLIC GTEST_HAS_TR1_TUPLE=0)
  if(UNIX AND NOT (APPLE OR ANDROID OR CA_PLATFORM_QNX))
    target_link_libraries(ca_gtest PUBLIC pthread dl)
  endif()

  # Link against ca_gtest_main if the test suite doesn't need customization.
  add_ca_library(ca_gtest_main STATIC googletest/src/gtest_main.cc)
  set_target_properties(ca_gtest_main PROPERTIES CXX_STANDARD 14)
  target_link_libraries(ca_gtest_main PUBLIC ca_gtest)
endif()

# If we're cross-compiling, targeting Windows, and using a GNU compiler then we
# are using MinGW.  google-benchmark uses 'versionhelpers.h' that MinGW does
# not provide until version 6.0, we currently have people using 5.3.  Native
# MinGW compile works because presumably a native header is available.  If
# MinGW 5.3 is no longer used then google-benchmark can become unconditional,
# though the case of some header names may need changed within the source
# (again, this is not an issue for a native MinGW compile).
if(NOT (CMAKE_CROSSCOMPILING AND CA_PLATFORM_WINDOWS AND
    CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
  # Ignore benchmarks outdated CMake as it generates warnings in our CMake and
  # breaks cross-compilation by using try_run to determine the regex backend to
  # use, all our supported platforms should support C++11 <regex> header so with
  # that assumption it is unconditionally included here.
  find_package(Threads REQUIRED)
  add_ca_library(ca-benchmark STATIC
    benchmark/src/benchmark.cc
    benchmark/src/benchmark_api_internal.cc
    benchmark/src/benchmark_main.cc
    benchmark/src/benchmark_name.cc
    benchmark/src/benchmark_register.cc
    benchmark/src/benchmark_runner.cc
    benchmark/src/colorprint.cc
    benchmark/src/commandlineflags.cc
    benchmark/src/complexity.cc
    benchmark/src/console_reporter.cc
    benchmark/src/counter.cc
    benchmark/src/csv_reporter.cc
    benchmark/src/json_reporter.cc
    benchmark/src/perf_counters.cc
    benchmark/src/reporter.cc
    benchmark/src/sleep.cc
    benchmark/src/statistics.cc
    benchmark/src/string_util.cc
    benchmark/src/sysinfo.cc
    benchmark/src/timers.cc)
  set_target_properties(ca-benchmark PROPERTIES
    CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
  target_include_directories(ca-benchmark SYSTEM
    PUBLIC benchmark/include PRIVATE benchmark/src)
  target_compile_definitions(ca-benchmark PRIVATE HAVE_STD_REGEX)
  target_compile_options(ca-benchmark PRIVATE
    $<$<PLATFORM_ID:Linux>:-fstrict-aliasing>
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:QCC>>:
      -Wno-deprecated-declarations
    >)
  target_link_libraries(ca-benchmark PUBLIC
    ${CMAKE_THREAD_LIBS_INIT} $<$<PLATFORM_ID:Windows>:Shlwapi>)
endif()
