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

option(HAL_CPU_WG_MODE "Build Hal CPU in workgroup mode" ON)

set(HAL_CPU_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND CMAKE_MODULE_PATH ${HAL_CPU_SOURCE_DIR}/cmake)

include(CompileKernel)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -fvisibility=hidden)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fvisibility=hidden)

set_property(GLOBAL APPEND PROPERTY KNOWN_HAL_DEVICES "cpu")
set_property(GLOBAL PROPERTY HAL_CPU_NAME "cpu")
set_property(GLOBAL PROPERTY HAL_CPU_DEVICE_NAME "ock cpu")
set_property(GLOBAL PROPERTY HAL_CPU_DIR "${HAL_CPU_SOURCE_DIR}")
set_property(GLOBAL PROPERTY HAL_CPU_EXAMPLE_DIR "${HAL_CPU_SOURCE_DIR}/examples/wi")

add_subdirectory(source)
