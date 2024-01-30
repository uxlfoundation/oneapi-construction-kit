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

#include <cargo/ring_buffer.h>
#include <gtest/gtest.h>

TEST(ring_buffer, add_some_remove_some) {
  const std::array<int, 16> as{
      {42, 14, 4, 3, 13, 256, 54, -53, 9, 10, 7, 2, 4, 400, 1024, 8}};

  cargo::ring_buffer<int, 16> rb;

  for (const int a : as) {
    ASSERT_EQ(cargo::result::success, rb.enqueue(a));
  }

  for (const int a : as) {
    cargo::error_or<int> eo = rb.dequeue();
    ASSERT_TRUE(eo);
    ASSERT_EQ(a, *eo);
  }
}

TEST(ring_buffer, fill_remove_add_drain) {
  const std::array<int, 4> as{{42, 14, 4, 3}};

  cargo::ring_buffer<int, 4> rb;

  for (const int a : as) {
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

  for (const int a : {3, 125, 350}) {
    auto eo = rb.dequeue();
    ASSERT_TRUE(eo);
    ASSERT_EQ(a, *eo);
  }
}

TEST(ring_buffer, enqueue_when_full) {
  const std::array<int, 4> as{{42, 14, 4, 3}};

  cargo::ring_buffer<int, 4> rb;

  for (const int a : as) {
    ASSERT_EQ(cargo::result::success, rb.enqueue(a));
  }

  ASSERT_EQ(cargo::result::overflow, rb.enqueue(13));
}

TEST(ring_buffer, dequeue_when_empty) {
  cargo::ring_buffer<int, 4> rb;
  ASSERT_FALSE(rb.dequeue());
}
