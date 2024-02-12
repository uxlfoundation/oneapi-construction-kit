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

add_ca_executable(muxc
  ${CMAKE_CURRENT_SOURCE_DIR}/muxc.h
  ${CMAKE_CURRENT_SOURCE_DIR}/muxc.cpp
  $<$<PLATFORM_ID:Windows>:${BUILTINS_RC_FILE}>)

target_compile_options(muxc PRIVATE
  $<$<CXX_COMPILER_ID:Clang>:-fcolor-diagnostics>)
target_compile_definitions(muxc PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
  muxc_VERSION="${PROJECT_VERSION}"
  muxc_LLVM_VERSION="${LLVM_PACKAGE_VERSION}"
  muxc_CL_STD_30=$<BOOL:${muxc_CL_STD_30}>)
target_include_directories(muxc PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source/cl/source/compiler/include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/compiler/multi_llvm/include>)
target_link_libraries(muxc PRIVATE
  compiler-static compiler-base LLVMBitWriter LLVMIRReader)
target_resources(muxc NAMESPACES ${BUILTINS_NAMESPACES})

install(TARGETS muxc RUNTIME DESTINATION bin COMPONENT muxc)
