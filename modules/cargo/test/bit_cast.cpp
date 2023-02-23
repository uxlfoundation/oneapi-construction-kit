// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cargo/utility.h>
#include <gtest/gtest.h>

#include <array>

TEST(bit_cast, int32_uint32) {
  int32_t in = -1;
  uint32_t out = cargo::bit_cast<uint32_t>(in);
  ASSERT_EQ(0xffffffff, out);
}

TEST(bit_cast, float_uint32_float) {
  float in = 23.0f;
  uint32_t out = cargo::bit_cast<uint32_t>(in);
  out |= 0x80000000;  // flip sign bit
  ASSERT_FLOAT_EQ(-23.0f, cargo::bit_cast<float>(out));
}

TEST(bit_cast, array_stdarray) {
  uint32_t a[8] = {42, 23, 3, 0, 0, 3, 23, 42};
  auto sa = cargo::bit_cast<std::array<uint32_t, 8>>(a);
  ASSERT_EQ(42, sa[0]);
  ASSERT_EQ(23, sa[1]);
  ASSERT_EQ(3, sa[2]);
  ASSERT_EQ(0, sa[3]);
  ASSERT_EQ(0, sa[4]);
  ASSERT_EQ(23, sa[6]);
  ASSERT_EQ(42, sa[7]);
}
