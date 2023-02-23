// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#include <gtest/gtest.h>

#include "common.h"

/// @file This file contains all tests for the compiler::Target object.

/// @brief Test fixture for testing behaviour of the compiler::Target::init API.
struct InitTest : CompilerTargetTest {};

TEST_P(InitTest, UnsupportedBuiltinCapabilities) {
  // compiler::Target::init must return compiler::Result::INVALID_VALUE if the
  // requested builtin capabilities are not supported.
  ASSERT_EQ(compiler::Result::INVALID_VALUE, target->init(~0x0));
}

INSTANTIATE_COMPILER_TARGET_TEST_SUITE_P(InitTest);

/// @brief Test fixture for testing behaviour of the
/// compiler::Target::listSnapshotStages API.
struct ListSnapshotStagesTest : CompilerTargetTest {};

TEST_P(ListSnapshotStagesTest, ListNumSnapshots) {
  uint32_t num_stages = 0;
  ASSERT_EQ(compiler::Result::SUCCESS,
            target->listSnapshotStages(0, nullptr, &num_stages));
}

TEST_P(ListSnapshotStagesTest, ListAllSnapshots) {
  std::vector<const char *> stages;
  uint32_t num_stages = 0;
  ASSERT_EQ(compiler::Result::SUCCESS,
            target->listSnapshotStages(0, nullptr, &num_stages));

  if (num_stages > 0) {
    stages.resize(num_stages);
    for (uint32_t j = 0; j < num_stages; ++j) {
      stages[j] = nullptr;
    }
    ASSERT_EQ(compiler::Result::SUCCESS,
              target->listSnapshotStages(num_stages, stages.data(), nullptr));

    for (uint32_t j = 0; j < num_stages; ++j) {
      ASSERT_TRUE(nullptr != stages[j]);
    }
  }
}

TEST_P(ListSnapshotStagesTest, ListSingleSnapshots) {
  uint32_t num_stages = 0;
  EXPECT_EQ(compiler::Result::SUCCESS,
            target->listSnapshotStages(0, nullptr, &num_stages));

  if (num_stages > 1) {
    const char *stages[2] = {nullptr, nullptr};

    EXPECT_EQ(compiler::Result::SUCCESS,
              target->listSnapshotStages(1, stages, nullptr));
    EXPECT_TRUE(nullptr != stages[0]);
    EXPECT_TRUE(nullptr == stages[1]);
  }
}

TEST_P(ListSnapshotStagesTest, ListMoreSnapshots) {
  std::vector<const char *> stages;
  uint32_t num_stages = 0;
  ASSERT_EQ(compiler::Result::SUCCESS,
            target->listSnapshotStages(0, nullptr, &num_stages));

  if (num_stages > 1) {
    // Allocate space for more snapshots than list will return
    const uint32_t allocated_stages = num_stages + 10;
    stages.resize(allocated_stages);

    for (uint32_t j = 0; j < allocated_stages; ++j) {
      stages[j] = nullptr;
    }

    ASSERT_EQ(
        compiler::Result::SUCCESS,
        target->listSnapshotStages(allocated_stages, stages.data(), nullptr));
    for (uint32_t j = 0; j < allocated_stages; ++j) {
      if (j < num_stages) {
        EXPECT_TRUE(nullptr != stages[j]);
      } else {
        EXPECT_TRUE(nullptr == stages[j]);
      }
    }
  }
}

TEST_P(ListSnapshotStagesTest, InvalidListSnapshotsArgs) {
  uint32_t num_stages = 0;
  const char *stages[1] = {nullptr};

  EXPECT_EQ(compiler::Result::SUCCESS,
            target->listSnapshotStages(0, nullptr, nullptr));
  EXPECT_EQ(compiler::Result::INVALID_VALUE,
            target->listSnapshotStages(1, nullptr, nullptr));
  EXPECT_EQ(compiler::Result::INVALID_VALUE,
            target->listSnapshotStages(1, nullptr, &num_stages));
  EXPECT_EQ(compiler::Result::INVALID_VALUE,
            target->listSnapshotStages(0, stages, nullptr));
  EXPECT_EQ(compiler::Result::INVALID_VALUE,
            target->listSnapshotStages(0, stages, &num_stages));
}

INSTANTIATE_COMPILER_TARGET_TEST_SUITE_P(ListSnapshotStagesTest);
