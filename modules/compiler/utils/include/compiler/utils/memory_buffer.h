// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Useful specializations of llvm::MemoryBuffer.
///
/// @copyright
/// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef COMPILER_UTILS_MEMORY_BUFFER_H_INCLUDED
#define COMPILER_UTILS_MEMORY_BUFFER_H_INCLUDED

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>

#include <memory>

#include "cargo/array_view.h"

namespace compiler {
namespace utils {
/// @brief An llvm::MemoryBuffer that loads from non-owning memory.
struct MemoryBuffer : public llvm::MemoryBuffer {
  MemoryBuffer(const void *data, uint64_t data_length) {
    const char *data_as_char_ptr = reinterpret_cast<const char *>(data);
    init(data_as_char_ptr, data_as_char_ptr + data_length,
         /*RequiresNullTerminator*/ false);
  }

  virtual llvm::MemoryBuffer::BufferKind getBufferKind() const override {
    return llvm::MemoryBuffer::MemoryBuffer_Malloc;
  }

  virtual ~MemoryBuffer() override {}
};
}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_MEMORY_BUFFER_H_INCLUDED
