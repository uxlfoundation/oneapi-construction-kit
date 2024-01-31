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
#include <metadata/detail/allocator_helper.h>

#include "fixtures.h"

template <class>
struct MDAllocatorTypeTest : MDAllocatorTest {};

TYPED_TEST_SUITE_P(MDAllocatorTypeTest);

TYPED_TEST_P(MDAllocatorTypeTest, ManualAllocation) {
  const md::AllocatorHelper<> helper(&this->hooks, &this->userdata);
  EXPECT_FALSE(this->userdata.allocated);

  auto alloc = helper.get_allocator<TypeParam>();
  TypeParam *i = alloc.allocate(1);
  EXPECT_NE(i, nullptr);
  EXPECT_TRUE(this->userdata.allocated);
  EXPECT_FALSE(this->userdata.deallocated);

  alloc.deallocate(i, 1);
  EXPECT_TRUE(this->userdata.deallocated);
}

TYPED_TEST_P(MDAllocatorTypeTest, ManualArrayAllocation) {
  const md::AllocatorHelper<> helper(&this->hooks, &this->userdata);
  EXPECT_FALSE(this->userdata.allocated);

  auto alloc = helper.get_allocator<TypeParam>();
  const size_t arr_size = 8;
  TypeParam *i = alloc.allocate(arr_size);
  EXPECT_NE(i, nullptr);
  EXPECT_TRUE(this->userdata.allocated);
  EXPECT_FALSE(this->userdata.deallocated);

  alloc.deallocate(i, arr_size);
  EXPECT_TRUE(this->userdata.deallocated);
}

TYPED_TEST_P(MDAllocatorTypeTest, DefaultCallback) {
  this->hooks.allocate = nullptr;
  this->hooks.deallocate = nullptr;
  const md::AllocatorHelper<> default_alloc(&this->hooks, nullptr);
  auto alloc = default_alloc.get_allocator<TypeParam>();
  TypeParam *i = alloc.allocate(1);
  EXPECT_NE(i, nullptr);
  alloc.deallocate(i, 1);
}

TYPED_TEST_P(MDAllocatorTypeTest, AllocateShared) {
  md::AllocatorHelper<> helper(&this->hooks, &this->userdata);
  {
    EXPECT_FALSE(this->userdata.allocated);
    const std::shared_ptr<TypeParam> intptr =
        helper.allocate_shared<TypeParam>(22);
    EXPECT_TRUE(this->userdata.allocated);
    EXPECT_FALSE(this->userdata.deallocated);
  }
  EXPECT_TRUE(this->userdata.deallocated);
}

using AllocTypes = ::testing::Types<int, char, uint64_t>;
REGISTER_TYPED_TEST_SUITE_P(MDAllocatorTypeTest, ManualAllocation,
                            ManualArrayAllocation, DefaultCallback,
                            AllocateShared);
INSTANTIATE_TYPED_TEST_SUITE_P(AllocationHelperTests, MDAllocatorTypeTest,
                               AllocTypes);
