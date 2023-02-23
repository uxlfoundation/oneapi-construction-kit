// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#include <gtest/gtest.h>
#include <metadata/detail/basic_map.h>

#include "fixtures.h"

struct BasicMapTest : MDAllocatorTest {
  std::vector<std::pair<int, int>> data = {
      {1, 3},
      {2, 4},
      {3, 9},
      {4, 1},
  };
};

TEST_F(BasicMapTest, Insert) {
  md::AllocatorHelper<> helper(&hooks, &userdata);
  md::basic_map<int, int> map(helper);

  auto ins_1 = map.insert({1, 4});
  EXPECT_TRUE(ins_1.second);
  auto ins_2 = map.insert({7, 12});
  EXPECT_TRUE(ins_2.second);

  // attempt to insert a key that already exists
  auto ins_fail = map.insert({7, 14});
  EXPECT_FALSE(ins_fail.second);
  // returns the conflicting key/value pair
  EXPECT_EQ(ins_fail.first->first, 7);
  EXPECT_EQ(ins_fail.first->second, 12);
}

TEST_F(BasicMapTest, Erase) {
  md::AllocatorHelper<> helper(&hooks, &userdata);
  md::basic_map<int, int> map(helper);

  for (const auto &el : data) {
    auto ins = map.insert(el);
    EXPECT_TRUE(ins.second);
  }

  for (const auto &el : data) {
    size_t erased = map.erase(el.first);
    EXPECT_EQ(erased, 1);
  }

  // attempt to erase a non-existant value
  EXPECT_EQ(map.erase(101), 0);
}

TEST_F(BasicMapTest, Find) {
  md::AllocatorHelper<> helper(&hooks, &userdata);
  md::basic_map<int, int> map(helper);

  for (const auto &el : data) {
    auto ins = map.insert(el);
    EXPECT_TRUE(ins.second);
  }

  for (const auto &el : data) {
    auto it = map.find(el.first);
    EXPECT_NE(it, map.end());
  }

  // attempt to find a non-existant value
  auto it_not_found = map.find(101);
  EXPECT_EQ(it_not_found, map.end());
}
