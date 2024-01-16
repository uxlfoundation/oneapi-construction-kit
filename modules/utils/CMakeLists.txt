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

add_ca_library(utils STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/utils/system.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/system.cpp)

target_include_directories(utils PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_definitions(utils PRIVATE
  $<$<BOOL:${CA_PLATFORM_LINUX}>:CA_PLATFORM_LINUX>
  $<$<BOOL:${CA_PLATFORM_WINDOWS}>:CA_PLATFORM_WINDOWS>
  $<$<BOOL:${CA_PLATFORM_MAC}>:CA_PLATFORM_MAC>
  $<$<BOOL:${CA_PLATFORM_ANDROID}>:CA_PLATFORM_ANDROID>
  $<$<BOOL:${CA_PLATFORM_QNX}>:CA_PLATFORM_QNX>)
target_link_libraries(utils PRIVATE cargo)

# Append to the list of module libraries, the cache MUST be updated.
list(APPEND MODULES_LIBRARIES utils)
set(MODULES_LIBRARIES ${MODULES_LIBRARIES}
  CACHE INTERNAL "List of module libraries.")

# Add any target-specific utils libraries
add_subdirectory(targets)
