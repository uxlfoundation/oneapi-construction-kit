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

#include <metadata/detail/utils.h>
#include <metadata/handler/vectorize_info_metadata.h>

#include <iterator>

namespace {
bool push(md_stack stack, FixedOrScalableQuantity<uint32_t> q) {
  int err = md_push_uint(stack, q.getKnownMinValue());
  if (MD_CHECK_ERR(err)) {
    return false;
  }

  err = md_push_uint(stack, q.isScalable() ? 1 : 0);
  if (MD_CHECK_ERR(err)) {
    return false;
  }

  return true;
}

FixedOrScalableQuantity<uint32_t> read_quantity(uint8_t *&data,
                                                MD_ENDIAN endianness) {
  const uint32_t quantity = md::utils::read_value<uint64_t>(data, endianness);
  data += sizeof(uint64_t);

  const bool is_scalable =
      md::utils::read_value<uint64_t>(data, endianness) == 1;
  data += sizeof(uint64_t);

  return FixedOrScalableQuantity<uint32_t>(quantity, is_scalable);
}
}  // namespace

namespace handler {

VectorizeInfoMetadata::VectorizeInfoMetadata(
    std::string kernel_name, std::string source_name,
    uint64_t local_memory_usage,
    FixedOrScalableQuantity<uint32_t> sub_group_size,
    FixedOrScalableQuantity<uint32_t> min_wi_factor,
    FixedOrScalableQuantity<uint32_t> pref_wi_factor)
    : GenericMetadata(kernel_name, source_name, local_memory_usage,
                      sub_group_size),
      min_work_item_factor(min_wi_factor),
      pref_work_item_factor(pref_wi_factor) {}

VectorizeInfoMetadataHandler::~VectorizeInfoMetadataHandler() {
  if (vec_data) {
    if (hooks && hooks->deallocate) {
      hooks->deallocate(vec_data, userdata);
    } else {
      std::free(vec_data);
    }
  }
}

bool VectorizeInfoMetadataHandler::init(md_hooks *hooks, void *userdata) {
  if (!GenericMetadataHandler::init(hooks, userdata)) {
    return false;
  }
  md_stack vectorize_stack = md_get_block(ctx, VECTORIZE_MD_BLOCK_NAME);
  if (!vectorize_stack) {
    vectorize_stack = md_create_block(ctx, VECTORIZE_MD_BLOCK_NAME);
    if (!vectorize_stack) {
      return false;
    }
  }

  if (hooks->map) {
    const int err = md_loadf(vectorize_stack, "s", &vec_data_len, &vec_data);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
  }
  return true;
}

bool VectorizeInfoMetadataHandler::finalize() {
  md_stack vectorize_stack = md_get_block(ctx, VECTORIZE_MD_BLOCK_NAME);
  if (!vectorize_stack) {
    return false;
  }
  const int err = md_finalize_block(vectorize_stack);
  if (MD_CHECK_ERR(err)) {
    return false;
  }
  return GenericMetadataHandler::finalize();
}

bool VectorizeInfoMetadataHandler::read(VectorizeInfoMetadata &md) {
  if (!GenericMetadataHandler::read(md)) {
    return false;
  }
  if (vec_offset >= vec_data_len) {
    return 0;
  }

  uint8_t *ptr = (uint8_t *)vec_data + vec_offset;

  md.min_work_item_factor = read_quantity(ptr, md_get_endianness(ctx));

  md.pref_work_item_factor = read_quantity(ptr, md_get_endianness(ctx));

  vec_offset = std::distance((uint8_t *)vec_data, ptr);

  return true;
}

bool VectorizeInfoMetadataHandler::write(const VectorizeInfoMetadata &md) {
  if (!GenericMetadataHandler::write(md)) {
    return false;
  }
  md_stack vectorize_stack = md_get_block(ctx, VECTORIZE_MD_BLOCK_NAME);
  if (!vectorize_stack) {
    return false;
  }

  if (!push(vectorize_stack, md.min_work_item_factor)) {
    return false;
  }

  if (!push(vectorize_stack, md.pref_work_item_factor)) {
    return false;
  }

  return true;
}

}  // namespace handler
