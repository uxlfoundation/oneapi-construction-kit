// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

/// @file metadata_impl.h
///
/// @brief Implementation details of the Metadata API.

#ifndef MD_DETAIL_METADATA_IMPL_H_INCLUDED
#define MD_DETAIL_METADATA_IMPL_H_INCLUDED
#include <metadata/metadata.h>
#include <stdint.h>

#include <limits>

namespace md {
/// @addtogroup md
/// @{

constexpr auto MD_BLOCK_INFO_SIZE = 24;
constexpr auto MD_HEADER_SIZE = 16;

constexpr uint8_t MD_MAGIC_0 = 0x43;
constexpr uint8_t MD_MAGIC_1 = 0x41;
constexpr uint8_t MD_MAGIC_2 = 0x4D;
constexpr uint8_t MD_MAGIC_3 = 0x44;

/// @brief Represents a Compute Aorta metadata header.
struct CAMD_Header {
  /// @brief Default Constructor for a new metadata header object
  CAMD_Header() = default;

  /// @brief A 4-byte magic number, must be ["C", "A", "M", "D"] to be valid.
  uint8_t magic[4];

  /// @brief The endianness of the encoded binary 0x1 or 0x2 for big & little
  /// endian respectively.
  uint8_t endianness;

  /// @brief The version of the metadata binary format we are using.
  uint8_t version;

  /// @brief These padding bytes are unused in the present version and MUST be
  /// set to zero.Consumers of this version MUST ignore them.
  uint8_t pad_unused_[2];

  /// @brief An index from [0x0] to the start of the block list. Since the
  /// header is fixed-size we are also able to deduce the length of the
  /// string-table from this value.
  uint32_t block_list_offset;

  /// @brief The number of blocks in the binary.
  uint32_t n_blocks;
};

/// @brief Represents a Compute Aorta block info.
struct CAMD_BlockInfo {
  /// @brief Default Constructor for a new metadata block info object
  CAMD_BlockInfo() = default;

  /// @brief The offset in bytes from [0x0] of the start of the block.
  uint64_t offset;

  /// @brief The size in bytes of the block.
  uint64_t size;

  /// @brief An index from the start of the string-table which indicates the
  /// name of this block. Note: in the binary implementation terms we *can*
  /// support multiple blocks of the same name, but to simplify the user
  /// interface, we don't allow such things to happen.
  uint32_t name_idx;

  /// @brief Serialization flags indicate the serialization format and encoding
  /// used of the block data.
  uint32_t flags;
};

/// @}
} // namespace md

#endif // MD_DETAIL_METADATA_IMPL_H_INCLUDED
