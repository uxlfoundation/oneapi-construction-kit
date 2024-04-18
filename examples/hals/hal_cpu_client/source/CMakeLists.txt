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

set(HAL_CPU_REMOTE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../clik/external/hal_cpu)
add_library(hal_cpu_client SHARED
  # Even though it is remote we need hal_cpu.cpp to get the info
  ${HAL_CPU_REMOTE_SOURCE_DIR}/source/cpu_hal.cpp
  hal_main.cpp
)

set(HAL_CPU_WG_MODE ON)

target_compile_definitions(hal_cpu_client PUBLIC -DBUILD_HAL_DLL)
target_compile_definitions(hal_cpu_client PUBLIC -DHAL_CPU_MODE=HAL_CPU_WG_MODE)

target_include_directories(hal_cpu_client PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${HAL_CPU_REMOTE_SOURCE_DIR}/include
)

target_link_libraries(hal_cpu_client hal_remote dl pthread)

set_target_properties(hal_cpu_client PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  
install(TARGETS hal_cpu_client
        LIBRARY DESTINATION lib
)
