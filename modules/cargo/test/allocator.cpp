// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cargo/allocator.h>
#include <gtest/gtest.h>

#include <memory>

namespace {
struct RootType {
  virtual ~RootType(){};
};

struct LeafType : public RootType {
  LeafType(bool& was_destructed_) : was_destructed(was_destructed_) {}
  virtual ~LeafType() { was_destructed = true; };

 private:
  bool& was_destructed;
};
}  // namespace

TEST(allocator, unique_ptr_deleter) {
  bool was_destructed = false;

  {
    void* raw = cargo::alloc(sizeof(LeafType), alignof(LeafType));
    ASSERT_TRUE(raw);
    LeafType* leaf = new (raw) LeafType(was_destructed);
    ASSERT_TRUE(leaf);
    std::unique_ptr<RootType, cargo::deleter<RootType>> ptr(leaf);
  }

  EXPECT_TRUE(was_destructed);
}
