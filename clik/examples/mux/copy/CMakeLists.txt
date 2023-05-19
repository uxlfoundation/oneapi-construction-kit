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

add_mux_example(copy_mux
  copy.cpp
)

# Compile device_copy.c for the device and generate a C header
# file containing the compiled kernel executable using Bin2H.
add_baked_kernel(copy_mux copy_mux_kernel kernel_binary.h device_copy.c)

# Generate a C header containing SPIR bitcode for the kernel using Bin2H.
if (${DEFAULT_ISA} MATCHES "^RV64")
  set(BITCODE_SRC device_copy64.bc)
else()
  set(BITCODE_SRC device_copy32.bc)
endif()
hal_add_baked_data(copy_mux copy_kernel_bitcode kernel_bitcode.h ${BITCODE_SRC})
