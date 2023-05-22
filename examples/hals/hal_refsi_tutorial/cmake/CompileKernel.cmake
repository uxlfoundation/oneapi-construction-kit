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

function(hal_refsi_tutorial_compile_kernel_source OBJECT SRC)
  set(INCLUDES ${ARGN})
  get_property(RISCV_CC_FLAGS GLOBAL PROPERTY RISCV_CC_FLAGS)
  get_property(RISCV_LINKER_FLAGS GLOBAL PROPERTY RISCV_LINKER_FLAGS)
  get_property(ROOT_DIR GLOBAL PROPERTY HAL_REFSI_TUTORIAL_DIR)
  get_target_property(REFSIDRV_SRC_DIR refsidrv SOURCE_DIR)

  # TODO: Compile a kernel source file (${SRC}) into a kernel object (${OBJECT})
endfunction()

function(hal_refsi_tutorial_link_kernel BINARY)
  set(OBJECTS ${ARGN})
  get_property(EXTRA_OBJECTS GLOBAL PROPERTY HAL_REFSI_TUTORIAL_EXTRA_OBJECTS)
  foreach(OBJECT ${EXTRA_OBJECTS})
    list(APPEND OBJECTS ${OBJECT})
  endforeach()

  get_property(RISCV_CC_FLAGS GLOBAL PROPERTY RISCV_CC_FLAGS)
  get_property(RISCV_LINKER_FLAGS GLOBAL PROPERTY RISCV_LINKER_FLAGS)
  get_property(ROOT_DIR GLOBAL PROPERTY HAL_REFSI_TUTORIAL_DIR)

  # TODO: Link multiple objects (${OBJECTS}) into a kernel executable (${BINARY})
endfunction()
