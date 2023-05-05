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

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/riscv-isa-sim/CMakeLists.txt)
  add_subdirectory(riscv-isa-sim)
else()
  include(FetchContent)

  # Use upstream github for riscv-isa-sim, but with some local changes to support refsi
  FetchContent_Declare(
    riscv-isa-sim
    GIT_REPOSITORY https://github.com/riscv-software-src/riscv-isa-sim.git
    GIT_TAG        057139ab90b8e03b71825e8f2be7cab58fef0f90
    # Do a check to see if it has been applied first as this can sometimes be an
    # issue when cmake changes are made
    PATCH_COMMAND  git apply --check --reverse ${CMAKE_CURRENT_SOURCE_DIR}/codeplay_riscv_isa_sim.patch || git apply ${CMAKE_CURRENT_SOURCE_DIR}/codeplay_riscv_isa_sim.patch
  )

  FetchContent_MakeAvailable(riscv-isa-sim)
endif()
