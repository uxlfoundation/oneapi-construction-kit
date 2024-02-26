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

if(CA_ENABLE_DEBUG_BACKTRACE)
  # Only build with debug backtrace when enabled.
  add_ca_library(debug-backtrace STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/debug/backtrace.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/backtrace.cpp)
  target_include_directories(debug-backtrace PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
  target_compile_definitions(debug-backtrace PUBLIC
    DEBUG_BACKTRACE_ENABLED)
  target_link_libraries(debug-backtrace PUBLIC cargo)
endif()
