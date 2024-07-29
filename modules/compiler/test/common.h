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
/// @brief Common utilities for UnitCompiler tests.

#ifndef COMPILER_UNITCOMPILER_COMMON_H_INCLUDED
#define COMPILER_UNITCOMPILER_COMMON_H_INCLUDED

#include <cargo/array_view.h>
#include <compiler/context.h>
#include <compiler/info.h>
#include <compiler/library.h>
#include <compiler/module.h>
#include <compiler/target.h>
#include <gtest/gtest.h>
#include <llvm/AsmParser/Parser.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/SourceMgr.h>
#include <mux/mux.h>
#include <mux/utils/helpers.h>

#include <memory>
#include <mutex>

#include "cargo/string_view.h"

// TODO: Add helpers to print error codes and helper macros that use them (see
// CA-3594).

/// @brief Return if a fatal failure or skip occurred invoking an expression.
///
/// Intended for use in test fixture `SetUp()` calls which explicitly call the
/// base class `SetUp()`, if a fatal error or skip occurs in the base class
/// immediately return to avoid crashing the test suite by using uninitialized
/// state.
///
/// TODO: Merge with version in UnitMux (see CA-3593).
///
/// @param ... Expression to invoke.
#define RETURN_ON_SKIP_OR_FATAL_FAILURE(...) \
  __VA_ARGS__;                               \
  if (HasFatalFailure() || IsSkipped()) {    \
    return;                                  \
  }                                          \
  (void)0

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

/// @brief Fixture for testing behavior of compiler::Info objects.
///
/// Tests based on this fixture should test the behavior of
/// compiler::Info objects.
///
/// Fixtures should derive from this class if they want to assume the presence
/// of a valid compiler.
struct CompilerInfoTest : ::testing::Test,
                          testing::WithParamInterface<const compiler::Info *> {
  /// @brief Compiler info (the variable over which this fixture is
  /// parameterized).
  const compiler::Info *compiler_info = nullptr;

  /// @brief Virtual method to setup any resources this fixture requires.
  void SetUp() override { compiler_info = GetParam(); }
};

/// @brief Fixture for testing behavior of compiler::Context objects.
///
/// Tests based on this fixture should test the behavior of
/// compiler::Context objects.
///
/// Fixtures should derive from this class if they want to assume the presence
/// of a valid compiler context.
struct CompilerContextTest : CompilerInfoTest {
  /// @brief Context used by the compiler API.
  std::unique_ptr<compiler::Context> context = nullptr;

  /// @brief Virtual method to setup any resources this fixture requires.
  void SetUp() override {
    RETURN_ON_SKIP_OR_FATAL_FAILURE(CompilerInfoTest::SetUp());
    context = compiler::createContext();
    ASSERT_NE(context, nullptr);
  }

  /// @brief Virtual method to clean up any resources this fixture allocated.
  void TearDown() override { context.reset(); }
};

/// @brief Parameterized fixture for testing behavior of compiler::Target
/// objects.
///
/// Tests based on this fixture should test the behavior of
/// compiler::Target objects.
///
/// Fixtures should derive from this class if they want to assume the presence
/// of a valid compiler target.
///
/// This fixture is parameterized on all mux device infos on the platform which
/// are used to create the targets.
struct CompilerTargetTest : CompilerContextTest {
  /// @brief Device to create kernels for, if the compiler is not a cross
  /// compiler.
  cargo::optional<mux_device_t> optional_device;
  /// @brief Target to compile for.
  std::unique_ptr<compiler::Target> target = nullptr;
  /// @brief Allocator used for custom device specific allocations. Here we
  /// default to the platform's system calls.
  mux_allocator_info_t allocator = {mux::alloc, mux::free, nullptr};

  /// @brief Virtual method to setup any resources this fixture requires.
  void SetUp() override {
    RETURN_ON_SKIP_OR_FATAL_FAILURE(CompilerContextTest::SetUp());

    // Get all possible Mux devices, and create a device if the compiler's
    // associated device info is a Mux device we can create. If the compiler's
    // associated device info is not returned by muxGetDeviceInfos, then we have
    // a cross compiler and therefore can't create the device right now.
    std::vector<mux_device_info_t> device_infos;
    uint64_t num_device_infos = 0;
    ASSERT_EQ(mux_success, muxGetDeviceInfos(mux_device_type_all, 0, nullptr,
                                             &num_device_infos));
    device_infos.resize(num_device_infos);
    ASSERT_EQ(mux_success,
              muxGetDeviceInfos(mux_device_type_all, num_device_infos,
                                device_infos.data(), nullptr));
    for (mux_device_info_t device_info : device_infos) {
      if (device_info == compiler_info->device_info) {
        mux_device_t mux_device;
        ASSERT_EQ(mux_success,
                  muxCreateDevices(1, &device_info, allocator, &mux_device));
        optional_device = mux_device;
        break;
      }
    }

    // Create compiler target.
    target = compiler_info->createTarget(context.get(), nullptr);
    ASSERT_NE(target, nullptr);
    ASSERT_EQ(
        compiler::Result::SUCCESS,
        target->init(detectBuiltinCapabilities(compiler_info->device_info)));
  }

  /// @brief Virtual method to clean up any resources this fixture allocated.
  void TearDown() override {
    target.reset();
    if (optional_device) {
      muxDestroyDevice(*optional_device, allocator);
    }
    CompilerContextTest::TearDown();
  }
};

/// @brief Parameterized fixture for testing behavior of compiler::Module
/// objects.
///
/// Tests based on this fixture should test the behavior of compiler::Module
/// objects.
///
/// Fixtures should derive from this class if they want to assume the presence
/// of a valid compiler Module.
///
/// This fixture is derived from CompilerTargetTest and therefore parameterized
/// on all targets supported by the platform.
struct CompilerModuleTest : CompilerTargetTest {
  /// @brief Module being created. Note that it is, empty tests or derived
  /// fixtures are responsible for loading and compiling source.
  std::unique_ptr<compiler::Module> module = nullptr;
  /// @brief Error counter used by module.
  uint32_t num_errors = 0;
  /// @brief Error log used by module.
  std::string log;

  /// @brief Virtual method to setup any resources this fixture requires.
  void SetUp() override {
    RETURN_ON_SKIP_OR_FATAL_FAILURE(CompilerTargetTest::SetUp());
    module = target->createModule(num_errors, log);
    ASSERT_NE(module, nullptr);
  }

  /// @brief Virtual method to clean up any resources this fixture allocated.
  void TearDown() override {
    module.reset();
    CompilerTargetTest::TearDown();
  }
};

/// @brief Parameterized fixture for testing behavior of a finalized
/// compiler::Module object which compiles an empty OpenCL C Kernel.
///
/// By default the Module loads and compiles the following no-op kernel:
/// kernel void  nop(){}.
///
/// However, derived fixtures can customize this behavior by overriding the
/// KernelSource() method to return any OpenCLC kernel of their choice that does
/// not contain printf statements.
///
/// Tests based on this fixture should test the behavior of finalized
/// compiler::Module objects.
///
/// Fixtures should derive from this class if they want to assume the presence
/// of a valid finalized compiler Module.
///
/// This fixture is derived from CompilerModuleTest and therefore parameterized
/// on all targets supported by the platform.
struct OpenCLCModuleTest : CompilerModuleTest {
  /// @brief Virtual method to setup any resources this fixture requires.
  void SetUp() override {
    RETURN_ON_SKIP_OR_FATAL_FAILURE(CompilerModuleTest::SetUp());
    ASSERT_EQ(compiler::Result::SUCCESS,
              module->compileOpenCLC(
                  mux::detectOpenCLProfile(compiler_info->device_info),
                  KernelSource(), {}));
    std::vector<builtins::printf::descriptor> printf_calls;
    ASSERT_EQ(compiler::Result::SUCCESS,
              module->finalize(nullptr, printf_calls));
  };

  /// @brief Virtual method to clean up any resources this fixture allocated.
  void TearDown() override {
    module->clear();
    CompilerModuleTest::TearDown();
  }

  /// @brief Virtual method to customize OpenCLC compiled by this module.
  ///
  /// Must not contain printf statements.
  virtual cargo::string_view KernelSource() { return "kernel void nop(){}"; }
};

/// @brief Parameterized fixture for testing behavior of a finalized
/// compiler::Kernel object with an empty kernel originating from the OpenCL C
/// Kernel:
///
/// kernel void nop(){}.
///
/// Tests based on this fixture should test the behavior of compiler::Kernel
/// objects.
///
/// Tests based on this fixture will only be run with runtime compilers that
/// have a valid mux_device associated with it.
///
/// Fixtures should derive from this class if they want to assume the presence
/// of a valid kernel object.
///
/// This fixture is derived from OpenCLCModuleTest and therefore parameterized
/// on all targets supported by the platform.
struct CompilerKernelTest : OpenCLCModuleTest {
  /// @brief Device object.
  mux_device_t device = nullptr;
  /// @brief Kernel object to test.
  compiler::Kernel *kernel = nullptr;

  /// @brief Virtual method to setup any resources required by this fixture.
  void SetUp() override {
    RETURN_ON_SKIP_OR_FATAL_FAILURE(OpenCLCModuleTest::SetUp());
    if (!optional_device.has_value()) {
      GTEST_SKIP();
    }
    device = *optional_device;
    kernel = module->getKernel("nop");
    ASSERT_NE(nullptr, kernel);
  }
};

/// @brief Helper function for printing Mux device names in a way that is
/// compatible with gtest.
///
/// TODO: Merge this with the function in UnitMux (see CA-3593).
static inline std::string printDeviceName(
    const testing::TestParamInfo<const compiler::Info *> &info) {
  std::string name{info.param->device_info->device_name};
  std::replace_if(
      std::begin(name), std::end(name),
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

/// @brief Helper function to list compilers that support deferred compilation.
static inline std::vector<const compiler::Info *> deferrableCompilers() {
  std::vector<const compiler::Info *> deferrable_compilers;
  for (const compiler::Info *compiler : compiler::compilers()) {
    if (compiler->supports_deferred_compilation()) {
      deferrable_compilers.emplace_back(compiler);
    }
  }
  return deferrable_compilers;
}

/// @brief Fixture for testing behavior of the compiler with LLVM modules.
///
/// Tests based on this fixture should test the behavior of
/// LLVM-based APIs and transforms.
struct CompilerLLVMModuleTest : ::testing::Test {
  void SetUp() override {}

  std::unique_ptr<llvm::Module> parseModule(llvm::StringRef Assembly) {
    llvm::SMDiagnostic Error;
    auto M = llvm::parseAssemblyString(Assembly, Error, Context);

    std::string ErrMsg;
    llvm::raw_string_ostream OS(ErrMsg);
    Error.print("", OS);
    EXPECT_TRUE(M) << OS.str();

    return M;
  }

  llvm::LLVMContext Context;
};

/// @brief Macro for instantiating test fixture parameterized over all
/// compiler targets available on the platform.
///
/// @param[in] FIXTURE Test fixture to instantiate, must be derived from
/// CompilerContextTest.
#define INSTANTIATE_COMPILER_TARGET_TEST_SUITE_P(FIXTURE) \
  INSTANTIATE_TEST_SUITE_P(                               \
      , FIXTURE, testing::ValuesIn(compiler::compilers()), printDeviceName)

/// @brief Macro for instantiating test fixture parameterized over all
/// deferrable compiler targets available on the platform.
///
/// The reason for this is that some tests can only be run on compilers that
/// support deferred compilation (i.e. compilers that implement
/// `Module::createKernel`). Rather than skipping those tests and adding noise
/// to UnitCompiler's output, we can use this to never instantiate those tests
/// in the first place.
///
/// @param[in] FIXTURE Test fixture to instantiate, must be derived from
/// CompilerContextTest.
#define INSTANTIATE_DEFERRABLE_COMPILER_TARGET_TEST_SUITE_P(FIXTURE)         \
  INSTANTIATE_TEST_SUITE_P(                                                  \
      , FIXTURE, testing::ValuesIn(deferrableCompilers()), printDeviceName); \
  /* We do not know whether there are any deferrable compilers. */           \
  GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(FIXTURE)

#endif  // COMPILER_UNITCOMPILER_COMMON_H_INCLUDED
