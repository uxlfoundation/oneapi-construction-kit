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

add_library(refsi_common STATIC
  common/elf_loader.cpp
  common/common_devices.cpp
)

target_link_libraries(refsi_common PUBLIC riscv-sim)

target_include_directories(refsi_common PUBLIC
    ${REFSIDRV_SOURCE_DIR}/include
    ${REFSIDRV_SOURCE_DIR}/include/common
)

add_library(refsidrv SHARED
  refsidrv/refsi_accelerator.cpp
  refsidrv/refsi_command_processor.cpp
  refsidrv/refsi_device.cpp
  refsidrv/refsi_device_m.cpp
  refsidrv/refsi_device_g.cpp
  refsidrv/refsi_memory.cpp
  refsidrv/refsi_memory_window.cpp
  refsidrv/refsi_perf_counters.cpp
  refsidrv/refsidrv.cpp
  refsidrv/kernel_dma.cpp
  refsidrv/slim_sim.cpp
  refsidrv/debugger.cpp
  refsidrv/trap_handlers.cpp
)

target_compile_definitions(refsidrv PRIVATE -DBUILD_REFSI_DLL)

target_link_libraries(refsidrv PRIVATE riscv-sim refsi_common hal_common)

target_include_directories(refsidrv PUBLIC
    ${REFSIDRV_SOURCE_DIR}/include
)

target_include_directories(refsidrv PRIVATE
    ${REFSIDRV_SOURCE_DIR}/include/refsidrv
)

install(TARGETS refsidrv
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  )

set_target_properties(refsidrv PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
