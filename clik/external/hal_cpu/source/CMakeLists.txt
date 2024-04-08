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

add_library(hal_cpu SHARED
  hal_main.cpp
  cpu_hal.cpp
)

target_compile_definitions(hal_cpu PUBLIC -DBUILD_HAL_DLL)

if(HAL_CPU_WG_MODE)
  target_compile_definitions(hal_cpu PUBLIC -DHAL_CPU_MODE=HAL_CPU_WG_MODE)
else()
  target_compile_definitions(hal_cpu PUBLIC -DHAL_CPU_MODE=HAL_CPU_WI_MODE)
endif()

target_include_directories(hal_cpu PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${HAL_CPU_SOURCE_DIR}/include
)

target_link_libraries(hal_cpu hal_common dl pthread)

set_target_properties(hal_cpu PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  
install(TARGETS hal_cpu
        LIBRARY DESTINATION lib
)
