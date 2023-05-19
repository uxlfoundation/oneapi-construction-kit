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
