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
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/cargo.dir/source
    ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/cargo.dir/source)
endif()

include(CheckSymbolExists)

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  # Macros and flags necessary to find the pthread get/set name symbols.
  set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
  set(CMAKE_REQUIRED_FLAGS -pthread)
  # Check for specific symbols in case a platform, e.g. an RTOS, does not
  # support them while otherwise supporting pthreads.
  check_symbol_exists(pthread_setname_np "pthread.h" CARGO_HAS_PTHREAD_SETNAME_NP)
  check_symbol_exists(pthread_getname_np "pthread.h" CARGO_HAS_PTHREAD_GETNAME_NP)
else()
  set(CARGO_HAS_PTHREAD_SETNAME_NP OFF)
  set(CARGO_HAS_PTHREAD_GETNAME_NP OFF)
endif()

add_ca_library(cargo STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/allocator.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/argument_parser.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/array_view.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/attributes.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/detail/expected.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/detail/optional.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/detail/sfinae_bases.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/dynamic_array.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/endian.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/error.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/expected.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/fixed_vector.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/function_ref.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/functional.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/memory.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/mutex.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/optional.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/platform_defines.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/ring_buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/small_vector.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/string_algorithm.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/string_view.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/thread.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/thread_safety.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/type_traits.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/cargo/utility.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/allocator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/endian.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/statics.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/thread.cpp)
if(CA_NATIVE_CPU)
  target_include_directories(cargo PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
else()
  target_include_directories(cargo PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

ca_option(CA_ENABLE_CARGO_INSTRUMENTATION BOOL
  "Enable instrumentation of memory allocation in cargo" OFF)
if(CA_ENABLE_CARGO_INSTRUMENTATION AND NOT CA_ENABLE_DEBUG_BACKTRACE)
  message(FATAL_ERROR "CA_ENABLE_DEBUG_BACKTRACE=1 is required when\
  CA_ENABLE_CARGO_INSTRUMENTATION=1")
endif()
target_compile_definitions(cargo PRIVATE
  CARGO_HAS_PTHREAD_SETNAME_NP=$<BOOL:${CARGO_HAS_PTHREAD_SETNAME_NP}>
  CARGO_HAS_PTHREAD_GETNAME_NP=$<BOOL:${CARGO_HAS_PTHREAD_GETNAME_NP}>)
target_compile_definitions(cargo PUBLIC
  $<$<BOOL:${CA_ENABLE_CARGO_INSTRUMENTATION}>:
  CA_CARGO_INSTRUMENTATION_ENABLED>)

# Append to the list of module libraries, the cache MUST be updated.
list(APPEND MODULES_LIBRARIES cargo)
set(MODULES_LIBRARIES ${MODULES_LIBRARIES}
  CACHE INTERNAL "List of module libraries.")

if(CA_ENABLE_TESTS)
  add_ca_executable(UnitCargo
    ${CMAKE_CURRENT_SOURCE_DIR}/test/allocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/argument_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/array_view.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/bit_cast.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/test/dynamic_array.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/endian.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/expected.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/fixed_vector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/function_ref.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/mutex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/optional.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/ring_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/small_vector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/string_algorithm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/string_view.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/thread.cpp)
  target_link_libraries(UnitCargo PRIVATE cargo ca_gtest_main)

  target_compile_options(UnitCargo PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<AND:$<VERSION_GREATER:$<CXX_COMPILER_VERSION>,6.9>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,11.0>>>:
      -Wno-error=array-bounds
    >)

  add_ca_check(UnitCargo GTEST
    COMMAND UnitCargo --gtest_output=xml:${PROJECT_BINARY_DIR}/UnitCargo.xml
    CLEAN ${PROJECT_BINARY_DIR}/UnitCargo.xml
    DEPENDS UnitCargo)

  install(TARGETS UnitCargo RUNTIME DESTINATION bin COMPONENT Cargo)
endif()
