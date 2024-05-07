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
project(ock_compiler_utils VERSION 1.0.0 LANGUAGES C CXX ASM)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)

set(OCK_IN_LLVM_TREE FALSE)
if (TARGET LLVMCore)
  set(OCK_IN_LLVM_TREE TRUE)
endif()
include(AddCA)

if(NOT OCK_IN_LLVM_TREE)
  include(../cmake/ImportLLVM.cmake)
  include_directories(SYSTEM ${LLVM_INCLUDE_DIR})
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../modules/compiler/multi_llvm ${CMAKE_CURRENT_BINARY_DIR}/multi_llvm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../modules/compiler/compiler_pipeline ${CMAKE_CURRENT_BINARY_DIR}/compiler_pipeline)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../modules/compiler/vecz ${CMAKE_CURRENT_BINARY_DIR}/vecz)
