// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "cargo/thread.h"

#include <gtest/gtest.h>

#include <atomic>

TEST(thread, set_name) {
  std::atomic_bool wait{true};
  cargo::thread thread{[&]() {
    while (wait) {
      ;
    }
  }};
  auto result = thread.set_name("name set");
  if (cargo::unsupported != result) {
    ASSERT_EQ(cargo::success, result);
  }
  wait = false;
  thread.join();
}

TEST(thread, get_name) {
  std::atomic_bool wait{true};
  cargo::thread thread{[&]() {
    while (wait) {
      ;
    }
  }};
  std::string name{"name set"};
  auto result = thread.set_name(name);
  if (cargo::unsupported != result) {
    ASSERT_EQ(cargo::success, result);
    auto error_or_name = thread.get_name();
    ASSERT_EQ(cargo::success, error_or_name.error());
    ASSERT_STREQ(name.c_str(), error_or_name->c_str());
  }
  wait = false;
  thread.join();
}
