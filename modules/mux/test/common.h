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
/// @brief Common utilities for UnitMux tests.

#ifndef MUX_UNITMUX_COMMON_H_INCLUDED
#define MUX_UNITMUX_COMMON_H_INCLUDED

#include <cargo/array_view.h>
#include <cargo/string_view.h>
#include <compiler/context.h>
#include <compiler/loader.h>
#include <compiler/module.h>
#include <compiler/target.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-port.h>
#include <mux/mux.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "mux/utils/helpers.h"

/// @brief Print a `mux_error_e` value in human readable form.
struct PrintError {
  PrintError(mux_result_t error) : error(error) {}

  std::string description() const {
    switch (error) {
#define CASE(ERROR) \
  case ERROR:       \
    return #ERROR;

      CASE(mux_success)
      CASE(mux_error_failure)
      CASE(mux_error_null_out_parameter)
      CASE(mux_error_invalid_value)
      CASE(mux_error_out_of_memory)
      CASE(mux_error_null_allocator_callback)
      CASE(mux_error_device_entry_hook_failed)
      CASE(mux_error_invalid_binary)
      CASE(mux_error_feature_unsupported)
      CASE(mux_error_missing_kernel)
      CASE(mux_error_internal)
      CASE(mux_error_fence_failure)
      CASE(mux_fence_not_ready)

#undef CASE

      default:
        return "unknown mux_result_t: " + std::to_string(error);
    }
  }

  bool operator==(const PrintError &other) const {
    return error == other.error;
  }

 private:
  mux_result_t error;
};

inline std::ostream &operator<<(std::ostream &out, const PrintError &error) {
  return out << error.description();
}

#ifndef ASSERT_SUCCESS
#define ASSERT_SUCCESS(actual) \
  ASSERT_EQ(PrintError(mux_success), PrintError(actual))
#endif
#ifndef EXPECT_SUCCESS
#define EXPECT_SUCCESS(actual) \
  EXPECT_EQ(PrintError(mux_success), PrintError(actual))
#endif

#ifndef ASSERT_ERROR_EQ
#define ASSERT_ERROR_EQ(expected, actual) \
  ASSERT_EQ(PrintError(expected), PrintError(actual))
#endif
#ifndef EXPECT_ERROR_EQ
#define EXPECT_ERROR_EQ(expected, actual) \
  EXPECT_EQ(PrintError(expected), PrintError(actual))
#endif

/// @brief Return if a fatal failure or skip occured invoking an expression.
///
/// Intended for use in test fixture `SetUp()` calls which explicitly call the
/// base class `SetUp()`, if a fatal error or skip occurs in the base class
/// immediately return to avoid crashing the test suite by using uninitialized
/// state.
///
/// @param ... Expression to invoke.
#define RETURN_ON_FATAL_FAILURE(...)      \
  __VA_ARGS__;                            \
  if (HasFatalFailure() || IsSkipped()) { \
    return;                               \
  }                                       \
  (void)0

/// @brief Get the number of devices.
///
/// @return Returns the number of devices.
inline uint64_t getNumDevices() {
  uint64_t num_devices;
  muxGetDeviceInfos(mux_device_type_all, 0, nullptr, &num_devices);
  return num_devices;
}

/// @brief Get the list of all non-compiler `mux_device_info_t`s.
///
/// @return Returns a vector of device infos.
inline std::vector<mux_device_info_t> getDeviceInfos() {
  std::vector<mux_device_info_t> device_infos(getNumDevices());
  muxGetDeviceInfos(mux_device_type_all, device_infos.size(),
                    device_infos.data(), nullptr);
  return device_infos;
}

/// @brief Replicates the pre-device_info_t API for creating and initializing
/// all devices present on the system.
///
/// @param[in] devices_length The length of out_devices. Must be 0, if
/// out_devices is NULL.
/// @param[in] allocator_info Allocator information.
/// @param[out] out_devices Array of devices mux knows about, or NULL if an
/// error occurred. Can be NULL, if out_devices_length is non-NULL.
/// @param[out] out_devices_length The total number of devices we are returning,
/// or 0 if an error occurred. Can be NULL, if out_devices is non-NULL.
///
/// @return mux_success, or a mux_error_* errorcode if an error
/// occurred.
inline mux_result_t createAllDevices(uint64_t devices_length,
                                     mux_allocator_info_t allocator_info,
                                     mux_device_t *out_devices,
                                     uint64_t *out_devices_length) {
  if (out_devices == nullptr && devices_length > 0) {
    return mux_error_null_out_parameter;
  }
  if (out_devices == nullptr && out_devices_length == nullptr) {
    return mux_error_null_out_parameter;
  }
  uint64_t devCount;
  if (auto error =
          muxGetDeviceInfos(mux_device_type_all, 0, nullptr, &devCount)) {
    return error;
  }
  if (out_devices != nullptr && devices_length != devCount) {
    return mux_error_invalid_value;
  }
  if (out_devices_length != nullptr) {
    *out_devices_length = devCount;
  }
  if (out_devices == nullptr) {
    return mux_success;
  }
  cargo::small_vector<mux_device_info_t, 4> devInfos;
  if (devInfos.resize(devCount)) {
    return mux_error_out_of_memory;
  }
  if (auto error = muxGetDeviceInfos(mux_device_type_all, devInfos.size(),
                                     devInfos.data(), nullptr)) {
    return error;
  }
  return muxCreateDevices(devices_length, devInfos.data(), allocator_info,
                          out_devices);
}

/// @brief Detect the builtin capabilities of a mux device.
///
/// @param[in] device_info Device info to detect capabilities for.
///
/// @return Capabilities of device.
inline uint32_t detectBuiltinCapabilities(mux_device_info_t device_info) {
  uint32_t caps = 0;
  if (device_info->address_capabilities & mux_address_capabilities_bits32) {
    caps |= compiler::CAPS_32BIT;
  }
  if (device_info->double_capabilities) {
    caps |= compiler::CAPS_FP64;
  }
  if (device_info->half_capabilities) {
    caps |= compiler::CAPS_FP16;
  }
  return caps;
}

/// @brief Pretty the device name from a `DeviceTest` index param.
///
/// @param info Information about the test, used to get the device name.
///
/// @return Returns a valid name for the device test param.
inline std::string printDeviceParamName(
    const testing::TestParamInfo<uint64_t> &info) {
  std::string name = getDeviceInfos()[info.param]->device_name;
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
  return name;
}

/// @brief Fixture for testing all devices.
///
/// This fixture provides `callback`, `allocator` and `device` members which
/// may be used to implement test cases.
///
/// Example usage:
///
/// ```cpp
/// struct myTest : DeviceTest {
///   virtual void SetUp() override {
///     RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
///     // Set up specific to myTest.
///   }
///
///   virtual void TearDown() override {
///     if (device) {
///       // Tear down specific to myTest, always use EXPECT macros.
///     }
///     DeviceTest::TearDown();
///   }
/// }
///
/// TEST_P(myTest, Default) {
///   // The body of the device test.
/// }
///
/// INSTANTIATE_DEVICE_TEST_SUITE_P(myTest);
/// ```
struct DeviceTest : testing::TestWithParam<uint64_t> {
  mux_callback_info_t callback = nullptr;
  mux_allocator_info_t allocator = {mux::alloc, mux::free, nullptr};
  mux_device_t device = nullptr;

  void SetUp() override {
    auto device_infos = getDeviceInfos();
    ASSERT_SUCCESS(
        muxCreateDevices(1, &device_infos[GetParam()], allocator, &device));
  }

  void TearDown() override { muxDestroyDevice(device, allocator); }
};

/// @brief Helper fixture for tests that require a compiler.
///
/// This test fixture will skip any tests derived from it when a compiler isn't
/// present.
/// It also supports a helper function for creating a mux_executable_t from an
/// OpenCL C string.
struct DeviceCompilerTest : DeviceTest {
  /// @brief Handle on the compiler library.
  std::unique_ptr<compiler::Library> library;
  /// @brief Compiler info.
  const compiler::Info *compiler_info;
  /// @brief Context used by the compiler for resource allocation.
  std::unique_ptr<compiler::Context> context;
  /// @brief Target to compile for.
  std::unique_ptr<compiler::Target> target;
  /// @brief Error counter to pass to module.
  uint32_t num_errors;
  /// @brief log to pass to module.
  std::string log;
  /// @brief Module to load source into and compile.
  std::unique_ptr<compiler::Module> module;
  /// @brief OpenCL C profile of the mux_device_info_t, used to compile the
  /// source.
  cargo::string_view profile;

  /// @brief Virtual method used to setup resources required by this fixture.
  void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    // Get the compiler library.
    auto library_expected = compiler::loadLibrary();
    if (!library_expected.has_value()) {
      GTEST_FAIL() << "Unable to load compiler library: "
                   << library_expected.error();
    }
    library = std::move(*library_expected);

    // If there is no compiler library skip the test since it requires a
    // compiler.
    if (nullptr == library) {
      GTEST_SKIP();
    };

    // Get the compiler for this device.
    compiler_info = compiler::getCompilerForDevice(library.get(), device->info);
    if (!compiler_info) {
      GTEST_FAIL() << "No compiler associated with Mux device "
                   << device->info->device_name;
    }

    // Create a context for compilation.
    context = compiler::createContext(library.get());
    ASSERT_NE(context, nullptr);

    // Create the target.
    target = compiler_info->createTarget(context.get(), nullptr);
    ASSERT_NE(target, nullptr);
    ASSERT_EQ(target->init(detectBuiltinCapabilities(device->info)),
              compiler::Result::SUCCESS);

    // Create the module.
    module = target->createModule(num_errors, log);
    ASSERT_NE(module, nullptr);

    // Detect the profile.
    profile = mux::detectOpenCLProfile(device->info);
  }

  void TearDown() override {
    module.reset();
    DeviceTest::TearDown();
  }

  /// @brief Helper function for compiling an OpenCL C source string and
  /// producing a binary.
  ///
  /// @param[in] clc_source The kernel source code in the OpenCL C language.
  /// @param[out] buffer The compiled binary owned by the module.
  //
  /// @return Error indicating success of function.
  compiler::Result createBinary(cargo::string_view clc_source,
                                cargo::array_view<std::uint8_t> &buffer) {
    // Load in and compile the OpenCL C.
    auto error = module->compileOpenCLC(profile, clc_source, {});
    if (compiler::Result::SUCCESS != error) {
      return error;
    };

    std::vector<builtins::printf::descriptor> printf_calls;
    error = module->finalize([](compiler::KernelInfo) {}, printf_calls);
    if (compiler::Result::SUCCESS != error) {
      return error;
    }

    error = module->createBinary(buffer);
    if (compiler::Result::SUCCESS != error) {
      return error;
    }

    return compiler::Result::SUCCESS;
  }

  /// @brief Helper function for compiling an OpenCL C source string and
  /// producing a mux_executable_t object.
  ///
  /// @brief[in] clc_source The kernel source code in the OpenCL C language.
  /// @brief[out] executable The mux_executable_t produced by the compiler.
  ///
  /// @return Returns mux_success on success, or the result of
  /// muxCreateExecutable if creating the executable failed. If compilation
  /// failed, mux_error_invalid_value is returned.
  mux_result_t createMuxExecutable(cargo::string_view clc_source,
                                   mux_executable_t *executable) {
    cargo::array_view<std::uint8_t> buffer;
    if (compiler::Result::SUCCESS != createBinary(clc_source, buffer)) {
      return mux_error_invalid_value;
    }

    return muxCreateExecutable(device, buffer.data(), buffer.size(), allocator,
                               executable);
  }
};

/// @def INSTANTIATE_DEVICE_TEST_SUITE_P
///
/// @brief Instantiate a DeviceTest suite to test all devices.
///
/// @param FIXTURE A test fixture type which inherits from DeviceTest.
#define INSTANTIATE_DEVICE_TEST_SUITE_P(FIXTURE)                         \
  INSTANTIATE_TEST_SUITE_P(, FIXTURE,                                    \
                           testing::Range(uint64_t(0), getNumDevices()), \
                           printDeviceParamName)
#endif  // MUX_UNITMUX_COMMON_H_INCLUDED
