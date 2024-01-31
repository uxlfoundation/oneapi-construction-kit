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
#ifndef MD_DETAIL_UTILS_H_INCLUDED
#define MD_DETAIL_UTILS_H_INCLUDED

/// @file utils.h
///
/// @brief Metadata API Utility Functions.

#include <cargo/endian.h>
#include <cargo/expected.h>
#include <metadata/detail/metadata_impl.h>
#include <metadata/metadata.h>

#include <string>
#include <type_traits>
#include <vector>

namespace md {
namespace utils {

/// @addtogroup md
/// @{

/// @brief Read an integer (as an array of bytes) from a desired endian format
/// into the target machine's endian format.
///
/// @tparam T Basic integral type.
/// @param in The value to be written to.
/// @param it An encoded integer byte-array value.
/// @param endianness The source endianness.
template <class T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
T read_value(uint8_t *it, uint8_t endianness) {
  T out;
  if (endianness == MD_ENDIAN::BIG) {
    cargo::read_big_endian(&out, it);
  } else {
    cargo::read_little_endian(&out, it);
  }
  return out;
}

/// @brief Decodes a valid metadata binary header.
///
/// @param header_start Pointer to the start of the header.
/// @param header The header object to be initialized with the header
/// data.
/// @param bin_size The size of the binary.
/// @return MD_SUCCESS or MD_INVALID_BINARY if the header is ill-formed.
cargo::expected<int, std::string> decode_md_header(uint8_t *header_start,
                                                   CAMD_Header &header,
                                                   size_t bin_size);

/// @brief Get a pointer to the start of the block info list.
///
/// @param data Pointer to the start of the binary.
/// @param header Reference to a decoded header.
/// @return Pointer to the start of the block info list.
uint8_t *get_block_list_start(uint8_t *data, const CAMD_Header &header);

/// @brief Decode a binary block info.
///
/// @param block_info_start Pointer to the start of the block info.
/// @param header Reference to a decoded binary header.
/// @param block_info The block info object to be initialized.
/// @param bin_size The size of the binary.
/// @return MD_SUCCESS or MD_INVALID_BINARY if the block info is ill-formed.
cargo::expected<int, std::string> decode_md_block_info(
    uint8_t *block_info_start, const CAMD_Header &header,
    CAMD_BlockInfo &block_info, size_t bin_size);

/// @brief Get a pointer to the start of a block.
///
/// @param binary Pointer to the start of the binary.
/// @param info Reference to the associated block info.
/// @return Pointer to the start of the block.
uint8_t *get_block_start(uint8_t *binary, const CAMD_BlockInfo &info);

/// @brief Get a pointer to the end of a block.
///
/// @param binary Pointer to the start of the binary.
/// @param info Reference to the associated block info.
/// @return Pointer to the end of the block.
uint8_t *get_block_end(uint8_t *binary, const CAMD_BlockInfo &info);

/// @brief Decode the full block info list.
///
/// @param block_list_start Pointer to the start of the block info list.
/// @param header An initialized header reference.
/// @param blocks An output vector to populate with decoded block
/// infos.
/// @param bin_size The size of the binary.
/// @return MD_SUCCESS or MD_INVALID_BINARY if the block info is ill-formed.
cargo::expected<int, std::string> decode_md_block_info_list(
    uint8_t *block_list_start, const CAMD_Header &header,
    std::vector<CAMD_BlockInfo> &blocks, size_t bin_size);

/// @brief Get the name of a block.
///
/// @param binary_data Pointer to the start of the binary.
/// @param bi Initialized block info reference.
/// @return Block info name.
const char *get_block_info_name(uint8_t *binary_data, const CAMD_BlockInfo &bi);

/// @brief Serialize a md header object into raw bytes.
///
/// @param header The header to be serialized.
/// @param output The output vector to which to write the serialized bytes.
void serialize_md_header(const CAMD_Header &header,
                         std::vector<uint8_t> &output);

/// @brief Serialize a block info object into raw bytes.
///
/// @param block_info The block info to be serialized.
/// @param endianness The desired endianness.
/// @param output Output vector to which to write the serialized bytes.
void serialize_block_info(const CAMD_BlockInfo &block_info, uint8_t endianness,
                          std::vector<uint8_t> &output);

/// @brief Retrieve the encoding from flags field.
///
/// @param flags Flags from which to retrieve the encoding.
/// @return If the flags provided include a valid encoding, then it will be
/// returned. If an invalid encoding is present MD_INVALID_FLAGS will be
/// returned.
cargo::expected<md_enc, md_err> get_enc(uint32_t flags);

/// @brief Retrieve the output format from flags field.
///
/// @param flags Flags from which to find format.
/// @return If the flags provided include a valid format, then it will be
/// returned. If an invalid format is present MD_INVALID_FLAGS will be
/// returned.
cargo::expected<md_fmt, md_err> get_fmt(uint32_t flags);

/// @brief Assemble a metadata format and encoding into a flags field.
///
/// @param fmt The desired Format.
/// @param enc The desired Encoding.
/// @return uint32_t
uint32_t get_flags(md_fmt fmt, md_enc enc);

/// @brief Pad a binary with a desired padding byte to the desired alignment
/// boundary.
///
/// @param[in, out] binary The binary to pad.
/// @param alignment The alignment boundary for which to pad.
/// @param padding_byte The desired padding byte to be used.
void pad_to_alignment(std::vector<uint8_t> &binary, size_t alignment = 8,
                      uint8_t padding_byte = 0x00);

/// @brief Get the endianness of the current machine.
///
/// @return constexpr MD_ENDIAN
constexpr MD_ENDIAN get_mach_endianness() {
  return cargo::is_little_endian() ? MD_ENDIAN::LITTLE : MD_ENDIAN::BIG;
}

/// @}
}  // namespace utils
}  // namespace md

#endif  // MD_DETAIL_UTILS_H_INCLUDED
