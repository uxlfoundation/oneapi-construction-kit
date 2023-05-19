// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <host/utils/jit_kernel.h>

// This can be anything, as long as it does not overlap the first byte from the
// ELF header.
const uint8_t magic_byte = 0xcd;

namespace host {
namespace utils {

bool isJITKernel(const void *binary, uint64_t binary_length) {
  if (binary_length != getSizeForJITKernel()) {
    return false;
  }
  return static_cast<const uint8_t *>(binary)[0] == magic_byte;
}

cargo::optional<jit_kernel_s> deserializeJITKernel(const void *binary,
                                                   uint64_t binary_length) {
  (void)binary_length;
  auto *buffer = static_cast<const uint8_t *>(binary);

  // Skip over header.
  buffer++;

  // Get JIT kernel object stored at the pointer contained within the buffer.
  const jit_kernel_s *kernel_ptr;
  std::copy_n(buffer, sizeof(const jit_kernel_s *),
              reinterpret_cast<uint8_t *>(&kernel_ptr));

  // Create a copy and return.
  jit_kernel_s kernel = *kernel_ptr;
  return {std::move(kernel)};
}

size_t getSizeForJITKernel() {
  // 1 byte for the header, and sizeof(const jit_kernel_s *) for the pointer.
  return 1 + sizeof(const jit_kernel_s *);
}

void serializeJITKernel(const jit_kernel_s *jit_kernel, uint8_t *buffer) {
  // Write the magic byte that indicates that this is a JIT binary.
  *buffer++ = magic_byte;

  // Write out the pointer to the JIT kernel data structure.
  std::copy_n(reinterpret_cast<const uint8_t *>(&jit_kernel),
              sizeof(const jit_kernel_s *), buffer);
}

}  // namespace utils
}  // namespace host
