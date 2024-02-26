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

# If coverage is enabled we add the modules so that the XML file can be
# generated automatically.
if(CA_ENABLE_COVERAGE)
  add_coverage_modules(
    ${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/compiler-loader.dir/source

    ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/compiler-loader.dir/source
  )
endif()

set(COMPILER_HEADERS
  ${PROJECT_SOURCE_DIR}/modules/compiler/include/compiler/context.h
  ${PROJECT_SOURCE_DIR}/modules/compiler/include/compiler/kernel.h
  ${PROJECT_SOURCE_DIR}/modules/compiler/include/compiler/limits.h
  ${PROJECT_SOURCE_DIR}/modules/compiler/include/compiler/module.h
  ${PROJECT_SOURCE_DIR}/modules/compiler/include/compiler/target.h)

set(COMPILER_LOADER_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/loader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/loader.cpp)

add_ca_library(compiler-loader STATIC
  ${COMPILER_HEADERS} ${COMPILER_LOADER_SOURCES})
target_include_directories(compiler-loader PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/compiler/include>)
target_include_directories(compiler-loader SYSTEM PUBLIC
  ${spirv-headers_SOURCE_DIR}/include)
target_link_libraries(compiler-loader PUBLIC mux cargo)
target_compile_definitions(compiler-loader PRIVATE
  $<$<BOOL:${CA_COMPILER_ENABLE_DYNAMIC_LOADER}>:
    CA_COMPILER_ENABLE_DYNAMIC_LOADER
  >)

# If the compiler is enabled.
if (CA_RUNTIME_COMPILER_ENABLED)
  target_link_libraries(compiler-loader PUBLIC
    # Link the dynamic loader on Linux if dynamic loading is enabled.
    $<$<AND:$<PLATFORM_ID:Linux>,$<BOOL:${CA_COMPILER_ENABLE_DYNAMIC_LOADER}>>:dl>
    # Link the static version of the compiler if dynamic loading is disabled.
    $<$<NOT:$<BOOL:${CA_COMPILER_ENABLE_DYNAMIC_LOADER}>>:compiler-static>)
endif()

# We need to directly link against the builtins to ensure header paths are
# satisfied for an offline only build or for a build using the dynamic loader.
target_link_libraries(compiler-loader PUBLIC builtins)
