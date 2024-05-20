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

project(hal_cpu_remote_server VERSION 1.0.0 LANGUAGES C CXX ASM)
set(CMAKE_CXX_STANDARD 17)            # Enable C++17 mode
set(HAL_CPU_REMOTE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../clik/external/hal_cpu)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../hal ${CMAKE_CURRENT_BINARY_DIR}/hal)

# Add the files in directly, rather than drag in as a shared library or take
# all the extra clik code
add_executable(hal_cpu_server_bin 
  hal_cpu_executable.cpp
  ${HAL_CPU_REMOTE_SOURCE_DIR}/source/cpu_hal.cpp
  ${HAL_CPU_REMOTE_SOURCE_DIR}/source/hal_main.cpp
)

target_compile_definitions(hal_cpu_server_bin PUBLIC -DHAL_CPU_MODE=HAL_CPU_WG_MODE)
target_include_directories(hal_cpu_server_bin PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${HAL_CPU_REMOTE_SOURCE_DIR}/include
)

target_link_libraries(hal_cpu_server_bin hal_remote hal_common dl pthread)


install(TARGETS hal_cpu_server_bin
        LIBRARY DESTINATION bin
)
