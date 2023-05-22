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

add_subdirectory(common)

add_library(hal_refsi_tutorial SHARED
  hal_main.cpp
  refsi_hal.cpp
)

target_compile_definitions(hal_refsi_tutorial PUBLIC -DBUILD_HAL_DLL)

target_include_directories(hal_refsi_tutorial PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(hal_refsi_tutorial PUBLIC
    ${HAL_REFSI_TUTORIAL_SOURCE_DIR}/include
)

target_link_libraries(hal_refsi_tutorial hal_common dl refsi_common refsidrv)

add_baked_data(hal_refsi_tutorial
    hal_refsi_tutorial_linker_script
    linker_script.h
    ${HAL_REFSI_TUTORIAL_SOURCE_DIR}/include/device/program.lds)

install(TARGETS hal_refsi_tutorial
        LIBRARY DESTINATION lib
)

set_target_properties(hal_refsi_tutorial PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
