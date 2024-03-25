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

/// @file
///
/// @brief

#ifndef UUR_FIXTURES_H_INCLUDED
#define UUR_FIXTURES_H_INCLUDED

#include "gtest/gtest.h"
#include "ur_api.h"
#include "uur/checks.h"
#include "uur/environment.h"

#define UUR_RETURN_ON_FATAL_FAILURE(...)              \
  __VA_ARGS__;                                        \
  if (this->HasFatalFailure() || this->IsSkipped()) { \
    return;                                           \
  }                                                   \
  (void)0

namespace uur {
struct PlatformTest : testing::Test {
  void SetUp() override { platform = Environment::instance->getPlatform(); }

  ur_platform_handle_t platform = nullptr;
};

struct DeviceTest : PlatformTest, testing::WithParamInterface<uint32_t> {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(PlatformTest::SetUp());
    device = Environment::instance->getDevices()[GetParam()];
  }

  ur_device_handle_t device = nullptr;
};

inline uint32_t getDeviceCount() {
  return Environment::instance->getDevices().size();
}

inline std::string getDeviceNameFromDeviceIndex(uint32_t device_index) {
  // Get the device by index into all devices.
  auto device = Environment::instance->getDevices()[device_index];
  size_t size;
  urDeviceGetInfo(device, UR_DEVICE_INFO_NAME, 0, nullptr, &size);
  std::string name(size, '\0');
  urDeviceGetInfo(device, UR_DEVICE_INFO_NAME, size, name.data(), nullptr);
  // Strip trailing null terminators.
  name.resize(name.find_first_of('\0'));
  // Make the device name a valid C identifier for use by gtest.
  std::replace_if(
      name.begin(), name.end(),
      [](char c) {
        // Don't replace valid C identifier characters.
        if (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9')) {
          return false;
        }
        return true;
      },
      '_');
  return name.data();
}
}  // namespace uur

#define UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(FIXTURE)                 \
  INSTANTIATE_TEST_SUITE_P(                                          \
      , FIXTURE, testing::Range(uint32_t(0), uur::getDeviceCount()), \
      [](const testing::TestParamInfo<uint32_t> &info) {             \
        return uur::getDeviceNameFromDeviceIndex(info.param);        \
      })

namespace uur {
template <typename T>
struct DeviceTestWithParam
    : PlatformTest,
      testing::WithParamInterface<std::tuple<uint32_t, T>> {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(PlatformTest::SetUp());
    device = Environment::instance->getDevices()[std::get<0>(this->GetParam())];
  }
  const T &getParam() const { return std::get<1>(this->GetParam()); }

  ur_device_handle_t device = nullptr;
};
}  // namespace uur

#define UUR_TEST_SUITE_P(FIXTURE, VALUES, PRINTER)                         \
  INSTANTIATE_TEST_SUITE_P(                                                \
      , FIXTURE,                                                           \
      testing::Combine(testing::Range(uint32_t(0), uur::getDeviceCount()), \
                       VALUES),                                            \
      PRINTER)

namespace uur {
struct ContextTest : DeviceTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    ASSERT_SUCCESS(urContextCreate(1, &device, nullptr, &context));
  }

  void TearDown() override {
    EXPECT_SUCCESS(urContextRelease(context));
    DeviceTest::TearDown();
  }

  ur_context_handle_t context = nullptr;
};

template <typename T>
struct ContextTestWithParam : DeviceTestWithParam<T> {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(DeviceTestWithParam<T>::SetUp());
    ASSERT_SUCCESS(urContextCreate(1, &this->device, nullptr, &context));
  }

  void TearDown() override {
    EXPECT_SUCCESS(urContextRelease(context));
    DeviceTestWithParam<T>::TearDown();
  }

  ur_context_handle_t context = nullptr;
};

struct QueueTest : ContextTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    ASSERT_SUCCESS(urQueueCreate(context, device, nullptr, &queue));
  }

  void TearDown() override {
    EXPECT_SUCCESS(urQueueRelease(queue));
    ContextTest::TearDown();
  }

  ur_queue_handle_t queue = nullptr;
};

template <typename T>
struct QueueTestWithParam : ContextTestWithParam<T> {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(ContextTestWithParam<T>::SetUp());
    ASSERT_SUCCESS(urQueueCreate(this->context, this->device, nullptr, &queue));
  }

  void TearDown() override {
    EXPECT_SUCCESS(urQueueRelease(queue));
    ContextTestWithParam<T>::TearDown();
  }

  ur_queue_handle_t queue = nullptr;
};

struct MultiQueueTest : ContextTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    ASSERT_SUCCESS(urQueueCreate(context, device, nullptr, &queue1));
    ASSERT_SUCCESS(urQueueCreate(context, device, nullptr, &queue2));
  }

  void TearDown() override {
    if (queue1 != nullptr) {
      EXPECT_SUCCESS(urQueueRelease(queue1));
    }
    if (queue2 != nullptr) {
      EXPECT_SUCCESS(urQueueRelease(queue2));
    }
    ContextTest::TearDown();
  }

  ur_queue_handle_t queue1 = nullptr;
  ur_queue_handle_t queue2 = nullptr;
};

struct ProgramTest : QueueTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(QueueTest::SetUp());
    const auto kernel_source =
        uur::Environment::instance->LoadSource("foo", this->GetParam());
    ASSERT_SUCCESS(kernel_source.status);

    ASSERT_SUCCESS(urProgramCreateWithIL(context, kernel_source.source,
                                         kernel_source.source_length, nullptr,
                                         &program));
    ASSERT_NE(nullptr, program);

    ASSERT_SUCCESS(urProgramBuild(context, program, nullptr));
  }

  void TearDown() override {
    if (program) {
      EXPECT_SUCCESS(urProgramRelease(program));
    }
  }

  ur_program_handle_t program = nullptr;
};

struct MultiProgramTest : QueueTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(QueueTest::SetUp());
    const auto kernel_source1 =
        uur::Environment::instance->LoadSource("foo", this->GetParam());
    ASSERT_SUCCESS(kernel_source1.status);
    ASSERT_SUCCESS(urProgramCreateWithIL(context, kernel_source1.source,
                                         kernel_source1.source_length, nullptr,
                                         &program1));
    const auto kernel_source2 =
        uur::Environment::instance->LoadSource("goo", this->GetParam());
    ASSERT_SUCCESS(kernel_source2.status);
    ASSERT_SUCCESS(urProgramCreateWithIL(context, kernel_source2.source,
                                         kernel_source2.source_length, nullptr,
                                         &program2));
    ASSERT_NE(nullptr, program1);
    ASSERT_NE(nullptr, program2);

    ASSERT_SUCCESS(urProgramBuild(context, program1, nullptr));
    ASSERT_SUCCESS(urProgramBuild(context, program2, nullptr));
  }

  void TearDown() override {
    if (program1) {
      EXPECT_SUCCESS(urProgramRelease(program1));
    }
    if (program2) {
      EXPECT_SUCCESS(urProgramRelease(program2));
    }
    QueueTest::TearDown();
  }

  ur_program_handle_t program1 = nullptr;
  ur_program_handle_t program2 = nullptr;
};

struct KernelTest : ProgramTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(ProgramTest::SetUp());
    ASSERT_SUCCESS(urKernelCreate(program, "foo", &kernel));
    ASSERT_NE(nullptr, kernel);
  }

  void TearDown() override {
    if (kernel) {
      EXPECT_SUCCESS(urKernelRelease(kernel));
    }
    ProgramTest::TearDown();
  }
  ur_kernel_handle_t kernel = nullptr;
};

struct MultiKernelTest : MultiProgramTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(MultiProgramTest::SetUp());
    ASSERT_SUCCESS(urKernelCreate(program1, "foo", &kernel1));
    ASSERT_SUCCESS(urKernelCreate(program2, "goo", &kernel2));
    ASSERT_NE(nullptr, kernel1);
    ASSERT_NE(nullptr, kernel2);
  }

  void TearDown() override {
    if (kernel1) {
      EXPECT_SUCCESS(urKernelRelease(kernel1));
    }
    if (kernel2) {
      EXPECT_SUCCESS(urKernelRelease(kernel2));
    }
    MultiProgramTest::TearDown();
  }

  ur_kernel_handle_t kernel1 = nullptr;
  ur_kernel_handle_t kernel2 = nullptr;
};

struct MultiDeviceContextTest : PlatformTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(PlatformTest::SetUp());
    auto devices = Environment::instance->getDevices();
    if (devices.size() <= 1) {
      GTEST_SKIP();
    }
    ASSERT_SUCCESS(
        urContextCreate(devices.size(), devices.data(), nullptr, &context));
  }

  void TearDown() override {
    if (context) {
      ASSERT_SUCCESS(urContextRelease(context));
    }
    PlatformTest::TearDown();
  }

  ur_context_handle_t context = nullptr;
};

struct MemBufferTest : ContextTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    ASSERT_SUCCESS(urMemBufferCreate(context, UR_MEM_FLAG_READ_WRITE, 4096,
                                     nullptr, &buffer));
    ASSERT_NE(nullptr, buffer);
  }

  void TearDown() override {
    if (buffer) {
      EXPECT_SUCCESS(urMemRelease(buffer));
    }
    ContextTest::TearDown();
  }

  ur_mem_handle_t buffer = nullptr;
};

struct MultiDeviceMemBufferTest : MultiDeviceContextTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(MultiDeviceContextTest::SetUp());
    ASSERT_SUCCESS(urMemBufferCreate(context, UR_MEM_FLAG_READ_WRITE, size,
                                     nullptr, &buffer));
    ASSERT_NE(nullptr, buffer);
  }

  void TearDown() override {
    if (buffer) {
      EXPECT_SUCCESS(urMemRelease(buffer));
    }
    MultiDeviceContextTest::TearDown();
  }

  ur_mem_handle_t buffer = nullptr;
  const size_t count = 1024;
  const size_t size = count * sizeof(uint32_t);
};

struct MultiDeviceMemBufferQueueTest : MultiDeviceMemBufferTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(MultiDeviceMemBufferTest::SetUp());
    queues.reserve(Environment::instance->getDevices().size());
    for (const auto &device : Environment::instance->getDevices()) {
      ur_queue_handle_t queue = nullptr;
      ASSERT_SUCCESS(urQueueCreate(context, device, nullptr, &queue));
      queues.push_back(queue);
    }
  }

  void TearDown() override {
    for (const auto &queue : queues) {
      EXPECT_SUCCESS(urQueueRelease(queue));
    }

    MultiDeviceMemBufferTest::TearDown();
  }

  std::vector<ur_queue_handle_t> queues;
};

struct MemBufferQueueTest : QueueTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(QueueTest::SetUp());
    ASSERT_SUCCESS(urMemBufferCreate(context, UR_MEM_FLAG_READ_WRITE, size,
                                     nullptr, &buffer));
  }

  void TearDown() override {
    if (buffer) {
      EXPECT_SUCCESS(urMemRelease(buffer));
    }
    QueueTest::TearDown();
  }

  const size_t count = 8;
  const size_t size = sizeof(uint32_t) * count;
  ur_mem_handle_t buffer = nullptr;
};
}  // namespace uur

#endif  // UUR_FIXTURES_H_INCLUDED
