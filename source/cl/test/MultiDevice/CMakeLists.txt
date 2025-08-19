# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

add_ca_cl_executable(MultiDevice
  ${CMAKE_CURRENT_SOURCE_DIR}/include/common.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/command_queue.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/context.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/main.cpp)

target_include_directories(MultiDevice PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(MultiDevice PRIVATE
  CA_VERSION="${PROJECT_VERSION}")
target_link_libraries(MultiDevice PRIVATE ca_gtest)

add_ca_cl_check(MultiDevice GTEST
  COMMAND MultiDevice --opencl-platform=ComputeAorta
    --gtest_output=xml:${PROJECT_BINARY_DIR}/MultiDevice.xml
  CLEAN ${PROJECT_BINARY_DIR}/MultiDevice.xml
  DEPENDS MultiDevice)

install(TARGETS MultiDevice RUNTIME DESTINATION bin COMPONENT MultiDevice)
