// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cargo/ring_buffer.h>
#include <gtest/gtest.h>

TEST(ring_buffer, add_some_remove_some) {
  std::array<int, 16> as{
      {42, 14, 4, 3, 13, 256, 54, -53, 9, 10, 7, 2, 4, 400, 1024, 8}};

  cargo::ring_buffer<int, 16> rb;

  for (int a : as) {
    ASSERT_EQ(cargo::result::success, rb.enqueue(a));
  }

  for (int a : as) {
    cargo::error_or<int> eo = rb.dequeue();
    ASSERT_TRUE(eo);
    ASSERT_EQ(a, *eo);
  }
}

TEST(ring_buffer, fill_remove_add_drain) {
  std::array<int, 4> as{{42, 14, 4, 3}};

  cargo::ring_buffer<int, 4> rb;

  for (int a : as) {
    ASSERT_EQ(cargo::success, rb.enqueue(a));
  }

  auto eo1 = rb.dequeue();
  ASSERT_TRUE(eo1);
  ASSERT_EQ(42, *eo1);

  ASSERT_EQ(cargo::success, rb.enqueue(125));

  auto eo2 = rb.dequeue();
  ASSERT_TRUE(eo2);
  ASSERT_EQ(14, *eo2);

  auto eo3 = rb.dequeue();
  ASSERT_TRUE(eo3);
  ASSERT_EQ(4, *eo3);
  ASSERT_EQ(cargo::success, rb.enqueue(350));

  for (int a : {3, 125, 350}) {
    auto eo = rb.dequeue();
    ASSERT_TRUE(eo);
    ASSERT_EQ(a, *eo);
  }
}

TEST(ring_buffer, enqueue_when_full) {
  std::array<int, 4> as{{42, 14, 4, 3}};

  cargo::ring_buffer<int, 4> rb;

  for (int a : as) {
    ASSERT_EQ(cargo::result::success, rb.enqueue(a));
  }

  ASSERT_EQ(cargo::result::overflow, rb.enqueue(13));
}

TEST(ring_buffer, dequeue_when_empty) {
  cargo::ring_buffer<int, 4> rb;
  ASSERT_FALSE(rb.dequeue());
}
