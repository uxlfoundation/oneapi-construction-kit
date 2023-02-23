// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#include <gtest/gtest.h>
#include <metadata/detail/allocator_helper.h>

#include "fixtures.h"

template <class>
struct MDAllocatorTypeTest : MDAllocatorTest {};

TYPED_TEST_SUITE_P(MDAllocatorTypeTest);

TYPED_TEST_P(MDAllocatorTypeTest, ManualAllocation) {
  md::AllocatorHelper<> helper(&this->hooks, &this->userdata);
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
  md::AllocatorHelper<> helper(&this->hooks, &this->userdata);
  EXPECT_FALSE(this->userdata.allocated);

  auto alloc = helper.get_allocator<TypeParam>();
  size_t arr_size = 8;
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
  md::AllocatorHelper<> default_alloc(&this->hooks, nullptr);
  auto alloc = default_alloc.get_allocator<TypeParam>();
  TypeParam *i = alloc.allocate(1);
  EXPECT_NE(i, nullptr);
  alloc.deallocate(i, 1);
}

TYPED_TEST_P(MDAllocatorTypeTest, AllocateShared) {
  md::AllocatorHelper<> helper(&this->hooks, &this->userdata);
  {
    EXPECT_FALSE(this->userdata.allocated);
    std::shared_ptr<TypeParam> intptr = helper.allocate_shared<TypeParam>(22);
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
