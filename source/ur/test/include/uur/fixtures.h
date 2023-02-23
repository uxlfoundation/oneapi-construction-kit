// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
  urDeviceGetInfo(device, UR_DEVICE_INFO_NAME, size, &name[0], nullptr);
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
    ASSERT_SUCCESS(urContextCreate(1, &device, &context));
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
    ASSERT_SUCCESS(urContextCreate(1, &this->device, &context));
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

struct ModuleTest : QueueTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(QueueTest::SetUp());
    const auto kernel_source =
        uur::Environment::instance->LoadSource("foo", this->GetParam());
    ASSERT_SUCCESS(kernel_source.status);
    ASSERT_SUCCESS(urModuleCreate(context, kernel_source.source,
                                  kernel_source.source_length, "", nullptr,
                                  nullptr, &module));
    ASSERT_NE(nullptr, module);
  }

  void TearDown() override {
    if (module) {
      EXPECT_SUCCESS(urModuleRelease(module));
    }
    QueueTest::TearDown();
  }

  ur_module_handle_t module = nullptr;
};

struct MultiModuleTest : QueueTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(QueueTest::SetUp());
    const auto kernel_source1 =
        uur::Environment::instance->LoadSource("foo", this->GetParam());
    ASSERT_SUCCESS(kernel_source1.status);
    ASSERT_SUCCESS(urModuleCreate(context, kernel_source1.source,
                                  kernel_source1.source_length, "", nullptr,
                                  nullptr, &module1));
    const auto kernel_source2 =
        uur::Environment::instance->LoadSource("goo", this->GetParam());
    ASSERT_SUCCESS(kernel_source2.status);
    ASSERT_SUCCESS(urModuleCreate(context, kernel_source2.source,
                                  kernel_source2.source_length, "", nullptr,
                                  nullptr, &module2));
    ASSERT_NE(nullptr, module1);
    ASSERT_NE(nullptr, module2);
  }

  void TearDown() override {
    if (module1) {
      EXPECT_SUCCESS(urModuleRelease(module1));
    }
    if (module2) {
      EXPECT_SUCCESS(urModuleRelease(module2));
    }
    QueueTest::TearDown();
  }

  ur_module_handle_t module1 = nullptr;
  ur_module_handle_t module2 = nullptr;
};

struct ProgramTest : ModuleTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(ModuleTest::SetUp());
    ASSERT_SUCCESS(urProgramCreate(context, 1, &module, nullptr, &program));
    ASSERT_NE(nullptr, program);
  }

  void TearDown() override {
    if (program) {
      EXPECT_SUCCESS(urProgramRelease(program));
    }
    ModuleTest::TearDown();
  }
  ur_program_handle_t program = nullptr;
};

struct MultiModuleProgramTest : MultiModuleTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(MultiModuleTest::SetUp());
    std::vector<ur_module_handle_t> modules = {module1, module2};
    ASSERT_SUCCESS(urProgramCreate(context, modules.size(), modules.data(),
                                   nullptr, &program));
    ASSERT_NE(nullptr, program);
  }

  void TearDown() override {
    if (program) {
      EXPECT_SUCCESS(urProgramRelease(program));
    }
    MultiModuleTest::TearDown();
  }
  ur_program_handle_t program = nullptr;
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

struct MultiKernelTest : MultiModuleProgramTest {
  void SetUp() override {
    UUR_RETURN_ON_FATAL_FAILURE(MultiModuleProgramTest::SetUp());
    ASSERT_SUCCESS(urKernelCreate(program, "foo", &kernel1));
    ASSERT_SUCCESS(urKernelCreate(program, "goo", &kernel2));
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
    MultiModuleProgramTest::TearDown();
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
    ASSERT_SUCCESS(urContextCreate(devices.size(), devices.data(), &context));
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
