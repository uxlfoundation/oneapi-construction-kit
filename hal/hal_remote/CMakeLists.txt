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

set(HAL_SOURCE
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_remote/hal_binary_encoder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_remote/hal_binary_decoder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_remote/hal_client.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_remote/hal_device_client.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_remote/hal_server.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_remote/hal_transmitter.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/hal_device_client.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/hal_server.cpp
)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
  list (APPEND HAL_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_remote/hal_socket_client.h)
  list (APPEND HAL_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_remote/hal_socket_transmitter.h)
  list (APPEND HAL_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/source/hal_socket_transmitter.cpp)
endif()

add_library(
  hal_remote STATIC ${HAL_SOURCE})
target_include_directories(
  hal_remote PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(
  hal_remote PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(hal_remote PUBLIC hal_common)
