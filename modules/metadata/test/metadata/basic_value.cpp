// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#include <gtest/gtest.h>
#include <metadata/detail/md_value.h>

#include <string>

#include "fixtures.h"

struct MD_ValueTest : public MDAllocatorTest {};

TEST_F(MD_ValueTest, BasicTypes) {
  md::AllocatorHelper<> helper(&hooks, &userdata);
  int64_t signedInt = -44;
  uint64_t unsignedInt = 101;
  md_value_ sintVal(helper, md_value_type::MD_TYPE_SINT, std::move(signedInt));
  md_value_ uintVal(helper, md_value_type::MD_TYPE_UINT,
                    std::move(unsignedInt));
  md_value_ zstrVal(helper, md_value_type::MD_TYPE_ZSTR,
                    std::string("Hello Metadata"));

  EXPECT_EQ(*sintVal.get<int64_t>(), signedInt);
  EXPECT_EQ(*uintVal.get<uint64_t>(), unsignedInt);
  EXPECT_STREQ(zstrVal.get<std::string>()->c_str(), "Hello Metadata");
}

TEST_F(MD_ValueTest, ComplexTypes) {
  md::AllocatorHelper<> helper(&hooks, &userdata);
  std::vector<int, md::callback_allocator<int>> vec(
      helper.template get_allocator<int>());
  md_value_ vecVal(helper, md_value_type::MD_TYPE_ARRAY, std::move(vec));

  auto *vecPtr = vecVal.get<std::vector<int, md::callback_allocator<int>>>();
  vecPtr->emplace_back(1);
  vecPtr->emplace_back(2);
  vecPtr->emplace_back(3);
  vecPtr->emplace_back(4);

  EXPECT_EQ(vecPtr->size(), 4);
}

TEST_F(MD_ValueTest, CopyAssignable) {
  md::AllocatorHelper<> helper(&hooks, &userdata);
  md_value_ value(helper, md_value_type::MD_TYPE_UINT, 3U);
  {
    // We can copy values, which copies the shared_ptr but the underlying data
    // remains the same
    md_value_ val_cpy = value;  // NOLINT: value is not modified so
                                // clang-tidy warns unnecessary copy
    auto *cpy_v = val_cpy.get<unsigned>();
    EXPECT_EQ(*cpy_v, 3);
    *cpy_v = 13;
  }
  auto *v = value.get<unsigned>();
  EXPECT_EQ(*v, 13);
}
