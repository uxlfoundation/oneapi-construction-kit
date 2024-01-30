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

#include <cargo/utility.h>
#include <gtest/gtest.h>

#include <array>

TEST(bit_cast, int32_uint32) {
  const int32_t in = -1;
  const uint32_t out = cargo::bit_cast<uint32_t>(in);
  ASSERT_EQ(0xffffffff, out);
}

TEST(bit_cast, float_uint32_float) {
  const float in = 23.0f;
  uint32_t out = cargo::bit_cast<uint32_t>(in);
  out |= 0x80000000;  // flip sign bit
  ASSERT_FLOAT_EQ(-23.0f, cargo::bit_cast<float>(out));
}

TEST(bit_cast, array_stdarray) {
  const uint32_t a[8] = {42, 23, 3, 0, 0, 3, 23, 42};
  auto sa = cargo::bit_cast<std::array<uint32_t, 8>>(a);
  ASSERT_EQ(42, sa[0]);
  ASSERT_EQ(23, sa[1]);
  ASSERT_EQ(3, sa[2]);
  ASSERT_EQ(0, sa[3]);
  ASSERT_EQ(0, sa[4]);
  ASSERT_EQ(23, sa[6]);
  ASSERT_EQ(42, sa[7]);
}
