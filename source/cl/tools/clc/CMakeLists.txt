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

add_ca_executable(clc
  ${CMAKE_CURRENT_SOURCE_DIR}/clc.h
  ${CMAKE_CURRENT_SOURCE_DIR}/clc.cpp
  $<$<PLATFORM_ID:Windows>:${BUILTINS_RC_FILE}>)
target_compile_options(clc PRIVATE
  $<$<CXX_COMPILER_ID:Clang>:-fcolor-diagnostics>)
target_compile_definitions(clc PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
  CLC_VERSION="${PROJECT_VERSION}"
  CLC_LLVM_VERSION="${LLVM_PACKAGE_VERSION}")
target_include_directories(clc PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source/cl/source/compiler/include>)
target_link_libraries(clc PRIVATE CL-binary compiler-static builtins cargo)
target_resources(clc NAMESPACES ${BUILTINS_NAMESPACES})

install(TARGETS clc RUNTIME DESTINATION bin COMPONENT clc)
