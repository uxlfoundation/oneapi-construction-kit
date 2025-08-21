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

/// @file
///
/// @brief Handle Vectorized Metadata.

#ifndef MD_HANDLER_VECTORIZE_INFO_METADATA_H_INCLUDED
#define MD_HANDLER_VECTORIZE_INFO_METADATA_H_INCLUDED
#include <metadata/handler/generic_metadata.h>

namespace handler {
/// @addtogroup md_handler
/// @{

/// @brief The block name used in the metadata API in which to store vectorize
/// metadata.
constexpr const char VECTORIZE_MD_BLOCK_NAME[] = "VectorizeMetadata";

/// @brief Struct which holds additional vectorization metadata from kernels.
struct VectorizeInfoMetadata : GenericMetadata {
  VectorizeInfoMetadata() = default;
  VectorizeInfoMetadata(
      std::string kernel_name, std::string source_name,
      uint64_t local_memory_usage,
      FixedOrScalableQuantity<uint32_t> sub_group_size,
      FixedOrScalableQuantity<uint32_t> min_work_item_factor,
      FixedOrScalableQuantity<uint32_t> pref_work_item_factor);
  /// @brief The minimum multiple of work-items that this kernel can safely
  /// process.
  FixedOrScalableQuantity<uint32_t> min_work_item_factor;
  /// @brief The preferred multiple of work-items that this kernel can process.
  FixedOrScalableQuantity<uint32_t> pref_work_item_factor;
};

/// @brief VectorizeInfoMetadataHandler handles interacting with the metadata
/// API such that kernel metadata can be correctly read from and written to the
/// binary representation.
class VectorizeInfoMetadataHandler : GenericMetadataHandler {
public:
  virtual ~VectorizeInfoMetadataHandler() override;

  /// @brief Initialize the metadata context.
  ///
  /// @param hooks Hooks forwarded to the context.
  /// @param userdata Userdata passed to the context.
  /// @return true if the context is initialized successfully, false otherwise.
  virtual bool init(md_hooks *hooks, void *userdata) override;

  /// @brief Finalize the metadata context.
  ///
  /// @return true if finalized successfully, false otherwise.
  virtual bool finalize() override;

  /// @brief Read kernel metadata.
  ///
  /// @param[in, out] md Metadata struct to be filled in.
  /// @return true if the data was read successfully, false otherwise.
  bool read(VectorizeInfoMetadata &md);

  /// @brief Write kernel metadata.
  ///
  /// @param md The metadata to be written.
  /// @return true if the write completed successfully, false otherwise.
  bool write(const VectorizeInfoMetadata &md);

private:
  char *vec_data = nullptr;
  size_t vec_data_len = 0;
  size_t vec_offset = 0;
};

/// @}
} // namespace handler

#endif // MD_HANDLER_VECTORIZE_INFO_METADATA_H_INCLUDED
