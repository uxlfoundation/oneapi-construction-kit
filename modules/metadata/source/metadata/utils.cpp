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

#include <cstring>

namespace md {
namespace utils {

cargo::expected<int, std::string> decode_md_header(uint8_t *header_start,
                                                   CAMD_Header &header,
                                                   size_t bin_size) {
  // check the binary size is sensible
  if (bin_size < MD_HEADER_SIZE) {
    return cargo::make_unexpected("Invalid Binary Size.");
  }

  // Decode the Magic Number: i.e. "CAMD"
  if (std::strncmp(reinterpret_cast<const char *>(header_start), "CAMD", 4) !=
      0) {
    return cargo::make_unexpected("Invalid Magic Number.");
  }
  std::memcpy(&header.magic, header_start, 4);

  // endianness
  // 1 = Little Endian
  // 2 = Big Endian
  const uint8_t endianness = header_start[4];
  if (!(endianness == MD_ENDIAN::LITTLE || endianness == MD_ENDIAN::BIG)) {
    return cargo::make_unexpected("Invalid Endian Format.");
  }
  header.endianness = endianness;

  // Version - only v1 is supported
  const uint8_t version = header_start[5];
  if (version != 1) {
    return cargo::make_unexpected("Invalid Version.");
  }
  header.version = version;

  // // 2 empty padding bytes
  header.pad_unused_[0] = 0x00;
  header.pad_unused_[1] = 0x00;

  // Block List Offset
  const uint32_t block_list_offset =
      read_value<uint32_t>(&header_start[8], header.endianness);
  if (block_list_offset < 16 || block_list_offset >= bin_size) {
    return cargo::make_unexpected("Invalid block-list offset.");
  }
  header.block_list_offset = block_list_offset;

  // N_blocks
  const uint32_t n_blocks =
      read_value<uint32_t>(&header_start[12], header.endianness);
  header.n_blocks = n_blocks;
  return md_err::MD_SUCCESS;
}

uint8_t *get_block_list_start(uint8_t *data, const CAMD_Header &header) {
  return data + header.block_list_offset;
}

uint8_t *get_block_start(uint8_t *binary, const CAMD_BlockInfo &info) {
  return binary + info.offset;
}

uint8_t *get_block_end(uint8_t *binary, const CAMD_BlockInfo &info) {
  return binary + info.offset + info.size;
}

cargo::expected<int, std::string> decode_md_block_info(
    uint8_t *block_info_start, const CAMD_Header &header,
    CAMD_BlockInfo &block_info, size_t bin_size) {
  // read the offset
  const uint64_t offset =
      read_value<uint64_t>(block_info_start, header.endianness);
  const size_t min_valid_block_offset =
      header.block_list_offset + (MD_BLOCK_INFO_SIZE * header.n_blocks);
  if (offset < min_valid_block_offset) {
    return cargo::make_unexpected("Invalid block offset value.");
  }
  block_info.offset = offset;

  // read the size
  const uint64_t block_size =
      read_value<uint64_t>(&block_info_start[8], header.endianness);
  if (block_info.offset + block_size > bin_size) {
    return cargo::make_unexpected("Invalid Block size");
  }
  block_info.size = block_size;

  // read the name_idx
  const uint32_t name_idx =
      read_value<uint32_t>(&block_info_start[16], header.endianness);
  if (name_idx > header.block_list_offset) {
    return cargo::make_unexpected("Invalid name index value.");
  }
  block_info.name_idx = name_idx;

  // read the flags
  const uint32_t flags =
      read_value<uint32_t>(&block_info_start[20], header.endianness);
  if (!(get_enc(flags).has_value() || get_fmt(flags).has_value())) {
    return cargo::make_unexpected("Invalid flags field.");
  }
  block_info.flags = flags;

  return md_err::MD_SUCCESS;
}

cargo::expected<int, std::string> decode_md_block_info_list(
    uint8_t *block_list_start, const CAMD_Header &header,
    std::vector<CAMD_BlockInfo> &blocks, size_t bin_size) {
  for (size_t i = 0; i < header.n_blocks; ++i) {
    CAMD_BlockInfo block_info;
    uint8_t *block_info_start = &block_list_start[i * MD_BLOCK_INFO_SIZE];
    auto decoded =
        decode_md_block_info(block_info_start, header, block_info, bin_size);
    if (!decoded.has_value()) {
      blocks.clear();
      return cargo::make_unexpected(decoded.error());
    }
    blocks.emplace_back(std::move(block_info));
  }
  return md_err::MD_SUCCESS;
}

const char *get_block_info_name(uint8_t *binary_data,
                                const CAMD_BlockInfo &bi) {
  return reinterpret_cast<const char *>(binary_data + bi.name_idx);
}

namespace {

/// @brief serializes integral types into raw bytes and writes them into an
/// output buffer.
///
/// @tparam T The specific integral type.
/// @param val The value to be serialized.
/// @param output The output byte-array to which the bytes are written.
template <class T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
void serialize_int(T val, std::vector<uint8_t> &output, uint8_t endianness) {
  T out_val = read_value<T>(reinterpret_cast<uint8_t *>(&val), endianness);
  auto width = sizeof(T);
  auto *start = reinterpret_cast<uint8_t *>(&out_val);
  output.insert(output.end(), start, &start[width]);
}
}  // namespace

void serialize_md_header(const CAMD_Header &header,
                         std::vector<uint8_t> &output) {
  for (size_t i = 0; i < 4; ++i) {
    serialize_int(header.magic[i], output, header.endianness);
  }
  serialize_int(header.endianness, output, header.endianness);
  serialize_int(header.version, output, header.endianness);
  serialize_int(header.pad_unused_[0], output, header.endianness);
  serialize_int(header.pad_unused_[1], output, header.endianness);
  serialize_int(header.block_list_offset, output, header.endianness);
  serialize_int(header.n_blocks, output, header.endianness);
}

void serialize_block_info(const CAMD_BlockInfo &block_info, uint8_t endianness,
                          std::vector<uint8_t> &output) {
  serialize_int(block_info.offset, output, endianness);
  serialize_int(block_info.size, output, endianness);
  serialize_int(block_info.name_idx, output, endianness);
  serialize_int(block_info.flags, output, endianness);
}

cargo::expected<md_enc, md_err> get_enc(uint32_t flags) {
  auto enc = static_cast<uint8_t>((flags >> 8) & 0xff);
  if (enc >= md_enc::MD_ENC_MAX_) {
    return cargo::make_unexpected(md_err::MD_E_INVALID_FLAGS);
  }
  return static_cast<md_enc>(enc);
}

cargo::expected<md_fmt, md_err> get_fmt(uint32_t flags) {
  auto fmt = static_cast<uint8_t>(flags & 0xff);
  if (fmt >= md_fmt::MD_FMT_MAX_) {
    return cargo::make_unexpected(md_err::MD_E_INVALID_FLAGS);
  }
  return static_cast<md_fmt>(fmt);
}

uint32_t get_flags(md_fmt fmt, md_enc enc) {
  return (fmt | (enc << 8)) & 0x0000ffff;
}

void pad_to_alignment(std::vector<uint8_t> &binary, size_t alignment,
                      uint8_t padding_byte) {
  while (binary.size() % alignment != 0) {
    binary.push_back(padding_byte);
  }
}
}  // namespace utils
}  // namespace md
