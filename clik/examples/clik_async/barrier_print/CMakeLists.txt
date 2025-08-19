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

# Compile device_barrier_print.c for the device and generate a C header
# file containing the compiled kernel executable using Bin2H.
add_baked_kernel(barrier_print_kernel kernel_binary.h device_barrier_print.c)
if (NOT FOUND_KERNEL_ENTRY)
  return()
endif()

add_clik_example(
  TARGET barrier_print
  SOURCES barrier_print.cpp
  KERNELS barrier_print_kernel
)

target_link_libraries(barrier_print PRIVATE clik_runtime_async)
