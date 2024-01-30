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

#include <cargo/allocator.h>
#include <gtest/gtest.h>

#include <memory>

namespace {
struct RootType {
  virtual ~RootType(){};
};

struct LeafType : public RootType {
  LeafType(bool &was_destructed_) : was_destructed(was_destructed_) {}
  virtual ~LeafType() { was_destructed = true; };

 private:
  bool &was_destructed;
};
}  // namespace

TEST(allocator, unique_ptr_deleter) {
  bool was_destructed = false;

  {
    void *raw = cargo::alloc(sizeof(LeafType), alignof(LeafType));
    ASSERT_TRUE(raw);
    LeafType *leaf = new (raw) LeafType(was_destructed);
    ASSERT_TRUE(leaf);
    const std::unique_ptr<RootType, cargo::deleter<RootType>> ptr(leaf);
  }

  EXPECT_TRUE(was_destructed);
}
