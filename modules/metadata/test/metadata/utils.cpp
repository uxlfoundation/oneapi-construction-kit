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
#include <gtest/gtest.h>
#include <metadata/detail/utils.h>

#include <cstring>

#include "fixtures.h"

TEST(UtilsTest, ReadValueBigEndianUint32) {
  // 4321 in BIG-ENDIAN -> uint32
  uint8_t val[] = {0x00, 0x00, 0x10, 0xe1};
  const uint32_t read_val =
      md::utils::read_value<uint32_t>(&val[0], MD_ENDIAN::BIG);
  EXPECT_EQ(read_val, 4321);
}

TEST(UtilsTest, ReadValueLittleEndianUint32) {
  // 15432 in LITTLE-ENDIAN -> uint32
  uint8_t val[] = {0x48, 0x3c, 0x00, 0x00};
  const uint32_t read_val =
      md::utils::read_value<uint32_t>(&val[0], MD_ENDIAN::LITTLE);
  EXPECT_EQ(read_val, 15432);
}

struct DecodeTest : ::testing::Test {
  uint8_t *get_start() { return const_cast<uint8_t *>(&example_md_bin[0]); }
  size_t get_bin_size() {
    return sizeof(example_md_bin) / sizeof(example_md_bin[0]);
  }
  md::CAMD_Header header;
  std::vector<md::CAMD_BlockInfo> infos;
};

TEST_F(DecodeTest, DecodeMDHeader) {
  auto decoded =
      md::utils::decode_md_header(get_start(), header, get_bin_size());
  EXPECT_TRUE(decoded.has_value());
  EXPECT_EQ(std::strncmp((const char *)&header.magic[0], "CAMD", 4), 0);
  EXPECT_EQ(header.endianness, MD_ENDIAN::BIG);
  EXPECT_EQ(header.version, 1);
  EXPECT_EQ(header.block_list_offset, 40);
  EXPECT_EQ(header.n_blocks, 2);
}

TEST_F(DecodeTest, GetBLockListStart) {
  auto decoded =
      md::utils::decode_md_header(get_start(), header, get_bin_size());
  EXPECT_TRUE(decoded.has_value());
  uint8_t *block_list_start =
      md::utils::get_block_list_start(get_start(), header);
  EXPECT_EQ(block_list_start, &example_md_bin[40]);
}

TEST_F(DecodeTest, DecodeBlockInfo) {
  auto decoded =
      md::utils::decode_md_header(get_start(), header, get_bin_size());
  EXPECT_TRUE(decoded.has_value());

  uint8_t *block_list_start =
      md::utils::get_block_list_start(get_start(), header);
  md::CAMD_BlockInfo info;
  decoded = md::utils::decode_md_block_info(block_list_start, header, info,
                                            get_bin_size());
  EXPECT_TRUE(decoded.has_value());

  EXPECT_EQ(info.offset, 88);
  EXPECT_EQ(info.size, 20);
  EXPECT_EQ(info.name_idx, 16);
  EXPECT_EQ(info.flags, 0);
}

TEST_F(DecodeTest, DecodeBlockInfoWithInvalidBlockSize) {
  const md::CAMD_Header header{
      {md::MD_MAGIC_0, md::MD_MAGIC_1, md::MD_MAGIC_2, md::MD_MAGIC_3},
      MD_ENDIAN::BIG,
      1,
      {0x00, 0x00},
      md::MD_HEADER_SIZE,
      1};
  constexpr const size_t binary_len = 100;
  uint8_t block_info_data[] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30,  // offset
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,  // size (too long 255)
      0x00, 0x00, 0x00, 0x10,                          // name_idx
      0x00, 0x00, 0x00, 0x00                           // flags
  };
  md::CAMD_BlockInfo info;
  auto decoded_info = md::utils::decode_md_block_info(block_info_data, header,
                                                      info, binary_len);
  ASSERT_FALSE(decoded_info.has_value());
  ASSERT_STREQ(decoded_info.error().c_str(), "Invalid Block size");
}

TEST_F(DecodeTest, GetBlockStartEnd) {
  auto decoded =
      md::utils::decode_md_header(get_start(), header, get_bin_size());
  EXPECT_TRUE(decoded.has_value());
  EXPECT_FALSE(MD_CHECK_ERR(decoded.value()));

  uint8_t *block_list_start =
      md::utils::get_block_list_start(get_start(), header);
  md::CAMD_BlockInfo info;
  decoded = md::utils::decode_md_block_info(block_list_start, header, info,
                                            get_bin_size());
  EXPECT_TRUE(decoded.has_value());
  EXPECT_FALSE(MD_CHECK_ERR(decoded.value()));

  uint8_t *block_start = md::utils::get_block_start(get_start(), info);
  EXPECT_EQ(block_start, &example_md_bin[88]);
  uint8_t *block_end = md::utils::get_block_end(get_start(), info);
  EXPECT_EQ(block_end, &example_md_bin[108]);
}

TEST_F(DecodeTest, DecodeBlockInfoList) {
  auto decoded =
      md::utils::decode_md_header(get_start(), header, get_bin_size());
  EXPECT_TRUE(decoded.has_value());
  EXPECT_FALSE(MD_CHECK_ERR(decoded.value()));
  uint8_t *block_list_start =
      md::utils::get_block_list_start(get_start(), header);
  decoded = md::utils::decode_md_block_info_list(block_list_start, header,
                                                 infos, get_bin_size());
  EXPECT_TRUE(decoded.has_value());
  EXPECT_FALSE(MD_CHECK_ERR(decoded.value()));

  EXPECT_EQ(infos.size(), 2);
}

TEST_F(DecodeTest, GetBlockInfoName) {
  auto decoded =
      md::utils::decode_md_header(get_start(), header, get_bin_size());
  EXPECT_TRUE(decoded.has_value());
  EXPECT_FALSE(MD_CHECK_ERR(decoded.value()));
  uint8_t *block_list_start =
      md::utils::get_block_list_start(get_start(), header);
  decoded = md::utils::decode_md_block_info_list(block_list_start, header,
                                                 infos, get_bin_size());
  EXPECT_TRUE(decoded.has_value());
  EXPECT_FALSE(MD_CHECK_ERR(decoded.value()));

  auto compiler_md = infos.at(0);
  EXPECT_STREQ(md::utils::get_block_info_name(get_start(), compiler_md),
               "compiler");

  auto host_md = infos.at(1);
  EXPECT_STREQ(md::utils::get_block_info_name(get_start(), host_md), "host_md");
}

TEST(UtilsTest, SerializeMDHeader) {
  // Generate a header for this binary
  md::CAMD_Header header{
      {md::MD_MAGIC_0, md::MD_MAGIC_1, md::MD_MAGIC_2,
       md::MD_MAGIC_3} /* MAGIC */,
      cargo::is_little_endian() ? MD_ENDIAN::LITTLE : MD_ENDIAN::BIG,
      0x01 /* version */,
      {0x00, 0x00} /* padding */,
      static_cast<uint32_t>(16) /*block_list_offset*/,
      static_cast<uint32_t>(0) /*n_blocks*/};

  std::vector<uint8_t> output;
  md::utils::serialize_md_header(header, output);

  md::CAMD_Header decoded_header;
  auto decoded = md::utils::decode_md_header(output.data(), decoded_header,
                                             output.size() + 1);
  EXPECT_TRUE(decoded.has_value());
  EXPECT_FALSE(MD_CHECK_ERR(decoded.value()));

  EXPECT_EQ(std::memcmp(&decoded_header.magic[0], &header.magic[0], 4), 0);
  EXPECT_EQ(decoded_header.endianness, header.endianness);
  EXPECT_EQ(decoded_header.version, header.version);
  EXPECT_EQ(
      std::memcmp(&decoded_header.pad_unused_[0], &header.pad_unused_[0], 2),
      0);
  EXPECT_EQ(decoded_header.block_list_offset, header.block_list_offset);
  EXPECT_EQ(decoded_header.n_blocks, header.n_blocks);
}

TEST(UtilsTest, SerializeMDBlockInfo) {
  const md::CAMD_BlockInfo info{120 /* offset */, 20 /* size */,
                                16 /* name_idx */, 0 /* flags */};
  const uint8_t mach_endianness =
      cargo::is_little_endian() ? MD_ENDIAN::LITTLE : MD_ENDIAN::BIG;
  std::vector<uint8_t> output;
  md::utils::serialize_block_info(info, mach_endianness, output);

  const md::CAMD_Header test_header{
      {md::MD_MAGIC_0, md::MD_MAGIC_1, md::MD_MAGIC_2,
       md::MD_MAGIC_3} /* MAGIC */,
      cargo::is_little_endian() ? MD_ENDIAN::LITTLE : MD_ENDIAN::BIG,
      0x01 /* version */,
      {0x00, 0x00} /* padding */,
      static_cast<uint32_t>(16) /*block_list_offset*/,
      static_cast<uint32_t>(0) /*n_blocks*/};

  md::CAMD_BlockInfo decoded_info;
  auto decoded = md::utils::decode_md_block_info(output.data(), test_header,
                                                 decoded_info, 200);
  EXPECT_TRUE(decoded.has_value());
  EXPECT_FALSE(MD_CHECK_ERR(decoded.value()));

  EXPECT_EQ(decoded_info.offset, info.offset);
  EXPECT_EQ(decoded_info.size, info.size);
  EXPECT_EQ(decoded_info.name_idx, info.name_idx);
  EXPECT_EQ(decoded_info.flags, info.flags);
}

TEST(UtilsTest, GetFlags) {
  const uint32_t flags =
      md::utils::get_flags(md_fmt::MD_FMT_LLVM_TEXT_MD, md_enc::MD_ENC_ZLIB);
  // LLVM_TEXT  = 0x04
  // ZLIB       = 0x01
  // result should be (BIG_ENDIAN)
  // 0x00, 0x00, 0x01, 0x04 (hex)
  // = 260 (decimal)
  EXPECT_EQ(flags, 260);
}

TEST(UtilsTest, GetEnc) {
  const uint32_t flags =
      md::utils::get_flags(md_fmt::MD_FMT_LLVM_TEXT_MD, md_enc::MD_ENC_ZLIB);
  auto enc = md::utils::get_enc(flags);
  EXPECT_TRUE(enc.has_value());
  EXPECT_EQ(enc, md_enc::MD_ENC_ZLIB);
}

TEST(UtilsTest, GetFmt) {
  const uint32_t flags =
      md::utils::get_flags(md_fmt::MD_FMT_LLVM_TEXT_MD, md_enc::MD_ENC_ZLIB);
  auto fmt = md::utils::get_fmt(flags);
  EXPECT_TRUE(fmt.has_value());
  EXPECT_EQ(fmt, md_fmt::MD_FMT_LLVM_TEXT_MD);
}

TEST(UtilsTest, InvalidFlags) {
  const uint32_t inv_flags =
      md::utils::get_flags(md_fmt::MD_FMT_MAX_, md_enc::MD_ENC_MAX_);

  auto enc = md::utils::get_enc(inv_flags);
  EXPECT_FALSE(enc.has_value());
  EXPECT_EQ(enc.error(), md_err::MD_E_INVALID_FLAGS);

  auto fmt = md::utils::get_fmt(inv_flags);
  EXPECT_FALSE(fmt.has_value());
  EXPECT_EQ(fmt.error(), md_err::MD_E_INVALID_FLAGS);
}

TEST(UtilsTest, PadToAlignment) {
  constexpr uint8_t padding_byte = 0x99;
  std::vector<uint8_t> needs_alignment = {0x01, 0x02, 0x03};
  const auto needs_alignment_org_size = needs_alignment.size();
  std::vector<uint8_t> no_alignment = {0x01, 0x02, 0x03, 0x04};
  const auto no_alignment_original_size = no_alignment.size();

  md::utils::pad_to_alignment(needs_alignment, 8, padding_byte);
  EXPECT_EQ(needs_alignment.size(), 8);
  for (size_t i = needs_alignment_org_size; i < needs_alignment.size(); ++i) {
    EXPECT_EQ(needs_alignment[i], padding_byte);
  }

  md::utils::pad_to_alignment(no_alignment, 4, padding_byte);
  EXPECT_EQ(no_alignment.size(), no_alignment_original_size);
}
