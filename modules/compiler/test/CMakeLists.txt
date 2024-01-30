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

add_ca_executable(UnitCompiler
  ${CMAKE_CURRENT_SOURCE_DIR}/common.h
  ${CMAKE_CURRENT_SOURCE_DIR}/group_ops.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kernel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/library.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mangling.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/module.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/target.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/utils.cpp
  $<$<PLATFORM_ID:Windows>:${BUILTINS_RC_FILE}>)

target_include_directories(UnitCompiler PRIVATE
  ${PROJECT_SOURCE_DIR}/modules/mux/include
  ${PROJECT_SOURCE_DIR}/modules/compiler/include)

target_link_libraries(UnitCompiler PRIVATE cargo
  compiler-static mux ca_gtest_main compiler-base compiler-pipeline compiler-binary-metadata)

target_resources(UnitCompiler NAMESPACES ${BUILTINS_NAMESPACES})

add_ca_check(UnitCompiler GTEST
  COMMAND UnitCompiler --gtest_output=xml:${PROJECT_BINARY_DIR}/UnitCompiler.xml
  CLEAN ${PROJECT_BINARY_DIR}/UnitCompiler.xml
  DEPENDS UnitCompiler)

add_subdirectory(lit)

install(TARGETS UnitCompiler RUNTIME DESTINATION bin COMPONENT compiler)
