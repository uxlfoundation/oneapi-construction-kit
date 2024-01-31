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
#include <metadata/detail/md_value.h>

#include <string>

#include "fixtures.h"

struct MD_ValueTest : public MDAllocatorTest {};

TEST_F(MD_ValueTest, BasicTypes) {
  const md::AllocatorHelper<> helper(&hooks, &userdata);
  int64_t signedInt = -44;
  uint64_t unsignedInt = 101;
  const md_value_ sintVal(helper, md_value_type::MD_TYPE_SINT,
                          std::move(signedInt));
  const md_value_ uintVal(helper, md_value_type::MD_TYPE_UINT,
                          std::move(unsignedInt));
  const md_value_ zstrVal(helper, md_value_type::MD_TYPE_ZSTR,
                          std::string("Hello Metadata"));

  EXPECT_EQ(*sintVal.get<int64_t>(), signedInt);
  EXPECT_EQ(*uintVal.get<uint64_t>(), unsignedInt);
  EXPECT_STREQ(zstrVal.get<std::string>()->c_str(), "Hello Metadata");
}

TEST_F(MD_ValueTest, ComplexTypes) {
  const md::AllocatorHelper<> helper(&hooks, &userdata);
  std::vector<int, md::callback_allocator<int>> vec(
      helper.template get_allocator<int>());
  const md_value_ vecVal(helper, md_value_type::MD_TYPE_ARRAY, std::move(vec));

  auto *vecPtr = vecVal.get<std::vector<int, md::callback_allocator<int>>>();
  vecPtr->emplace_back(1);
  vecPtr->emplace_back(2);
  vecPtr->emplace_back(3);
  vecPtr->emplace_back(4);

  EXPECT_EQ(vecPtr->size(), 4);
}

TEST_F(MD_ValueTest, CopyAssignable) {
  const md::AllocatorHelper<> helper(&hooks, &userdata);
  const md_value_ value(helper, md_value_type::MD_TYPE_UINT, 3U);
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
