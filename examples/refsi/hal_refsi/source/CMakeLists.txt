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

add_subdirectory(loader)
add_subdirectory(common)

add_library(hal_refsi SHARED
  hal_main.cpp
  refsi_hal.cpp
  refsi_hal_m1.cpp
  refsi_hal_g1.cpp
  refsi_command_buffer.cpp
  riscv_encoder.cpp
)

target_compile_definitions(hal_refsi PUBLIC -DBUILD_HAL_DLL)
target_compile_definitions(hal_refsi PUBLIC -DNUM_HARTS_FOR_CA_MODE=${NUM_HARTS_FOR_CA_MODE})
target_compile_definitions(hal_refsi PRIVATE -DREFSI_SIM_MAX_HARTS=${REFSIDRV_SIM_MAX_HARTS})
if(HAL_REFSI_SOC STREQUAL "M1")
  target_compile_definitions(hal_refsi PUBLIC -DHAL_REFSI_TARGET_M1 -DHAL_REFSI_MODE_WG=1)
elseif(HAL_REFSI_SOC STREQUAL "G1")
  target_compile_definitions(hal_refsi PUBLIC -DHAL_REFSI_TARGET_G1)
  if (${HAL_REFSI_THREAD_MODE} STREQUAL "WG")
    target_compile_definitions(hal_refsi PUBLIC -DHAL_REFSI_MODE_WG=1)
  else()
    target_compile_definitions(hal_refsi PUBLIC -DHAL_REFSI_MODE_WI=1)
  endif()
endif()

target_include_directories(hal_refsi PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/loader
)

target_include_directories(hal_refsi PUBLIC
    ${HAL_REFSI_SOURCE_DIR}/include
)

target_link_libraries(hal_refsi hal_common dl refsi_common refsidrv)

add_dependencies(hal_refsi refsi_g1_loader_binary)

add_baked_data(hal_refsi
    hal_refsi_linker_script
    linker_script.h
    ${HAL_REFSI_SOURCE_DIR}/include/device/program.lds)

install(TARGETS hal_refsi
        LIBRARY DESTINATION lib
)

set_target_properties(hal_refsi PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
