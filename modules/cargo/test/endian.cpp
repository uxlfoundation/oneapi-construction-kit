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

#include <cargo/endian.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

TEST(endian, bswaps) {
  ASSERT_EQ(0x01, cargo::byte_swap(uint8_t(0x01)));
  ASSERT_EQ(0x0201, cargo::byte_swap(uint16_t(0x0102)));
  ASSERT_EQ(0x04030201, cargo::byte_swap(uint32_t(0x01020304)));
  ASSERT_EQ(0x0807060504030201ULL,
            cargo::byte_swap(uint64_t(0x0102030405060708ULL)));
}

TEST(endian, reads) {
  std::array<uint8_t, 9> bytes;
  {
    std::copy_n(reinterpret_cast<const uint8_t*>(
                    "\x01\x00\x00\x00\x00\x00\x00\x00\x00"),
                bytes.size(), bytes.begin());
    uint8_t val = 0;
    cargo::read_little_endian(&val, bytes.begin());
    ASSERT_EQ(uint8_t(0x01), val);
    std::copy_n(reinterpret_cast<const uint8_t*>(
                    "\x01\x00\x00\x00\x00\x00\x00\x00\x00"),
                bytes.size(), bytes.begin());
    val = 0;
    cargo::read_big_endian(&val, bytes.begin());
    ASSERT_EQ(uint8_t(0x01), val);
  }

  {
    std::copy_n(reinterpret_cast<const uint8_t*>(
                    "\x02\x01\x00\x00\x00\x00\x00\x00\x00"),
                bytes.size(), bytes.begin());
    uint16_t val = 0;
    cargo::read_little_endian(&val, bytes.begin());
    ASSERT_EQ(uint16_t(0x0102), val);
    std::copy_n(reinterpret_cast<const uint8_t*>(
                    "\x01\x02\x00\x00\x00\x00\x00\x00\x00"),
                bytes.size(), bytes.begin());
    val = 0;
    cargo::read_big_endian(&val, bytes.begin());
    ASSERT_EQ(uint16_t(0x0102), val);
  }

  {
    std::copy_n(reinterpret_cast<const uint8_t*>(
                    "\x04\x03\x02\x01\x00\x00\x00\x00\x00"),
                bytes.size(), bytes.begin());
    uint32_t val = 0;
    cargo::read_little_endian(&val, bytes.begin());
    ASSERT_EQ(uint32_t(0x01020304), val);
    std::copy_n(reinterpret_cast<const uint8_t*>(
                    "\x01\x02\x03\x04\x00\x00\x00\x00\x00"),
                bytes.size(), bytes.begin());
    val = 0;
    cargo::read_big_endian(&val, bytes.begin());
    ASSERT_EQ(uint32_t(0x01020304), val);
  }

  {
    std::copy_n(reinterpret_cast<const uint8_t*>(
                    "\x08\x07\x06\x05\x04\x03\x02\x01\x00"),
                bytes.size(), bytes.begin());
    uint64_t val = 0;
    cargo::read_little_endian(&val, bytes.begin());
    ASSERT_EQ(uint64_t(0x0102030405060708ULL), val);
    std::copy_n(reinterpret_cast<const uint8_t*>(
                    "\x01\x02\x03\x04\x05\x06\x07\x08\x00"),
                bytes.size(), bytes.begin());
    val = 0;
    cargo::read_big_endian(&val, bytes.begin());
    ASSERT_EQ(uint64_t(0x0102030405060708ULL), val);
  }
}

TEST(endian, consecutive_reads) {
  std::array<uint8_t, 8> bytes = {{2, 1, 3, 4, 6, 5, 7, 8}};

  auto it = bytes.begin();
  uint16_t val = 0;
  it = cargo::read_little_endian(&val, it);
  ASSERT_EQ(uint16_t(0x0102), val);
  it = cargo::read_big_endian(&val, it);
  ASSERT_EQ(uint16_t(0x0304), val);
  it = cargo::read_little_endian(&val, it);
  ASSERT_EQ(uint16_t(0x0506), val);
  it = cargo::read_big_endian(&val, it);
  ASSERT_EQ(uint16_t(0x0708), val);
  ASSERT_EQ(it, bytes.end());
}

TEST(endian, writes) {
  std::vector<uint8_t> bytes;
  bytes.resize(9);
  cargo::write_little_endian(uint8_t(0x01), bytes.begin());
  ASSERT_TRUE(std::equal(bytes.begin(), bytes.end(),
                         reinterpret_cast<const uint8_t*>(
                             "\x01\x00\x00\x00\x00\x00\x00\x00\x00")));
  std::fill(bytes.begin(), bytes.end(), 0);
  cargo::write_big_endian(uint8_t(0x01), bytes.begin());
  ASSERT_TRUE(std::equal(bytes.begin(), bytes.end(),
                         reinterpret_cast<const uint8_t*>(
                             "\x01\x00\x00\x00\x00\x00\x00\x00\x00")));
  std::fill(bytes.begin(), bytes.end(), 0);

  cargo::write_little_endian(uint16_t(0x0102), bytes.begin());
  ASSERT_TRUE(std::equal(bytes.begin(), bytes.end(),
                         reinterpret_cast<const uint8_t*>(
                             "\x02\x01\x00\x00\x00\x00\x00\x00\x00")));
  std::fill(bytes.begin(), bytes.end(), 0);
  cargo::write_big_endian(uint16_t(0x0102), bytes.begin());
  ASSERT_TRUE(std::equal(bytes.begin(), bytes.end(),
                         reinterpret_cast<const uint8_t*>(
                             "\x01\x02\x00\x00\x00\x00\x00\x00\x00")));
  std::fill(bytes.begin(), bytes.end(), 0);

  cargo::write_little_endian(uint32_t(0x01020304), bytes.begin());
  ASSERT_TRUE(std::equal(bytes.begin(), bytes.end(),
                         reinterpret_cast<const uint8_t*>(
                             "\x04\x03\x02\x01\x00\x00\x00\x00\x00")));
  std::fill(bytes.begin(), bytes.end(), 0);
  cargo::write_big_endian(uint32_t(0x01020304), bytes.begin());
  ASSERT_TRUE(std::equal(bytes.begin(), bytes.end(),
                         reinterpret_cast<const uint8_t*>(
                             "\x01\x02\x03\x04\x00\x00\x00\x00\x00")));
  std::fill(bytes.begin(), bytes.end(), 0);

  cargo::write_little_endian(uint64_t(0x0102030405060708ULL), bytes.begin());
  ASSERT_TRUE(std::equal(bytes.begin(), bytes.end(),
                         reinterpret_cast<const uint8_t*>(
                             "\x08\x07\x06\x05\x04\x03\x02\x01\x00")));
  std::fill(bytes.begin(), bytes.end(), 0);
  cargo::write_big_endian(uint64_t(0x0102030405060708ULL), bytes.begin());
  ASSERT_TRUE(std::equal(bytes.begin(), bytes.end(),
                         reinterpret_cast<const uint8_t*>(
                             "\x01\x02\x03\x04\x05\x06\x07\x08\x00")));
  std::fill(bytes.begin(), bytes.end(), 0);
}

TEST(endian, consecutive_writes) {
  std::vector<uint8_t> bytes;
  bytes.resize(8);
  auto it = bytes.begin();
  it = cargo::write_little_endian(uint16_t(0x0102), it);
  it = cargo::write_big_endian(uint16_t(0x0304), it);
  it = cargo::write_little_endian(uint16_t(0x0506), it);
  it = cargo::write_big_endian(uint16_t(0x0708), it);
  ASSERT_EQ(it, bytes.end());
  ASSERT_TRUE(std::equal(
      bytes.begin(), bytes.end(),
      reinterpret_cast<const uint8_t*>("\x02\x01\x03\x04\x06\x05\x07\x08")));
}
