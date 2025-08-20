# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.20)

if(CMAKE_BUILD_TYPE)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CC_BUILD_TYPE "-DNDEBUG")
  else()
    set(CC_BUILD_TYPE "")
  endif()
endif()

get_property(RISCV_CC_FLAGS GLOBAL PROPERTY RISCV_CC_FLAGS)
get_property(RISCV_LINKER_FLAGS GLOBAL PROPERTY RISCV_LINKER_FLAGS)
get_target_property(REFSIDRV_SRC_DIR refsidrv SOURCE_DIR)

set(RISCV_CC_FLAGS
  ${RISCV_CC_FLAGS}
  -mcmodel=medany
  -fno-PIE
  -fno-stack-protector
  -O3
  ${CC_BUILD_TYPE}
  -DBUILD_FOR_DEVICE
  -DNUM_HARTS_FOR_CA_MODE=${NUM_HARTS_FOR_CA_MODE}
  -DHAL_REFSI_TARGET_G1=1
  -static)

set(RISCV_LINKER_FLAGS
  -Wl,--build-id=none
  ${RISCV_LINKER_FLAGS}
  -nostdlib
  -T${CMAKE_CURRENT_SOURCE_DIR}/loader.lds)

set(REFSI_G1_LOADER_BIN ${PROJECT_BINARY_DIR}/bin/refsi_g1_loader)

set(LOADER_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/loader.c
  ${CMAKE_CURRENT_SOURCE_DIR}/io.c
  ${CMAKE_CURRENT_SOURCE_DIR}/machine.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mentry.S)
set(LOADER_INCLUDES
  -I${CMAKE_CURRENT_SOURCE_DIR}
  -I${HAL_REFSI_SOURCE_DIR}/include
  -I${REFSIDRV_SRC_DIR}/../include
  -I${REFSIDRV_SRC_DIR}/../include/device)

add_custom_command(OUTPUT ${REFSI_G1_LOADER_BIN} COMMAND
  ${RISCV_CC}
  ${LOADER_SOURCES}
  ${LOADER_INCLUDES}
  ${RISCV_CC_FLAGS}
  ${RISCV_LINKER_FLAGS}
  -o ${REFSI_G1_LOADER_BIN}
  DEPENDS ${LOADER_SOURCES})

add_custom_target(refsi_g1_loader DEPENDS ${REFSI_G1_LOADER_BIN})

add_bin2h_target(refsi_g1_loader_binary
  ${REFSI_G1_LOADER_BIN}
  ${CMAKE_CURRENT_BINARY_DIR}/refsi_g1_loader_binary.h)

add_dependencies(refsi_g1_loader_binary refsi_g1_loader)
