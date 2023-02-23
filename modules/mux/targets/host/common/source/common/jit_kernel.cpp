// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <host/common/jit_kernel.h>

// This can be anything, as long as it does not overlap the first byte from the
// ELF header.
const uint8_t magic_byte = 0xcd;

namespace host {
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
}  // namespace host
