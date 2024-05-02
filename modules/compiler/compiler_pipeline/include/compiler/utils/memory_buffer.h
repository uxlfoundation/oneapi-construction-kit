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

/// @file
///
/// @brief Useful specializations of llvm::MemoryBuffer.

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
