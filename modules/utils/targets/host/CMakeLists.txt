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

set(HOST_UTILS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host/utils/jit_kernel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/host/utils/relocations.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/jit_kernel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/relocations.cpp)

add_ca_library(host-utils STATIC ${HOST_UTILS_SOURCES})

target_include_directories(host-utils PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/utils/include>)

if(CA_HOST_ENABLE_FP64)
  target_compile_definitions(host-utils PRIVATE CA_HOST_ENABLE_FP64)
endif()
if(CA_HOST_ENABLE_FP16)
  target_compile_definitions(host-utils PRIVATE CA_HOST_ENABLE_FP16)
endif()

target_link_libraries(host-utils PUBLIC cargo)
