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

add_ca_library(md_api STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/source/metadata/utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/metadata/metadata.cpp)
  add_ca_library(md_handler STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/source/metadata/handler/generic_metadata.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/metadata/handler/vectorize_info_metadata.cpp)
if(CA_NATIVE_CPU)
  target_link_libraries(md_api PUBLIC cargo)
  target_include_directories(md_api
      PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
      PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/metadata/detail)
  
  target_link_libraries(md_handler PUBLIC cargo md_api)
  target_include_directories(md_handler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  # cmake complains about the fact that include paths are in the build directory, so I had to make 
  # the include path PRIVATE, and add it manually here
  target_include_directories(md_handler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../cargo/include)
  target_include_directories(md_api PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../cargo/include)
else()
  target_link_libraries(md_api PUBLIC cargo)
  target_include_directories(md_api
      PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
      PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/metadata/detail)
  
  target_link_libraries(md_handler PUBLIC cargo md_api)
  target_include_directories(md_handler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

if(CA_ENABLE_TESTS)
  # UnitMD
  add_ca_executable(UnitMD
      ${CMAKE_CURRENT_SOURCE_DIR}/test/metadata/allocators.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/test/metadata/basic_map.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/test/metadata/utils.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/test/metadata/basic_value.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/test/metadata/basic_stack.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/test/metadata/basic_ctx.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/test/metadata/stack_serializer.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/test/metadata/api.cpp)
  
  target_include_directories(UnitMD PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}/include/metadata/detail)

  target_link_libraries(UnitMD PRIVATE ca_gtest_main md_api)
  
  add_ca_check(UnitMD GTEST
      COMMAND UnitMD --gtest_output=xml:${PROJECT_BINARY_DIR}/UnitMD.xml
      CLEAN ${PROJECT_BINARY_DIR}/UnitMD.xml
      DEPENDS UnitMD)
endif()
