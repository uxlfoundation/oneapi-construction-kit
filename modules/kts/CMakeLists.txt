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

add_ca_library(kts STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/kts/arguments_shared.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/kts/execution_shared.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/kts/reference_functions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/kts/stdout_capture.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/kts/vecz_tasks_common.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/arguments_shared.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/stdout_capture.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/reference_functions_shared.cpp)

target_compile_options(kts PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:
    -wd4996  # Disable warning on deprecated declarations
  >)

target_include_directories(kts PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(kts PUBLIC ca_gtest)

# Append to the list of module libraries, the cache MUST be updated.
list(APPEND MODULES_LIBRARIES kts)
set(MODULES_LIBRARIES ${MODULES_LIBRARIES}
  CACHE INTERNAL "List of module libraries.")
