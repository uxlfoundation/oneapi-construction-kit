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

#include "compiler/module.h"

#include <cstdint>
#include <cstring>

#include "cargo/array_view.h"
#include "cargo/small_vector.h"
#include "cargo/string_algorithm.h"
#include "cargo/string_view.h"
#include "common.h"
#include "mux/utils/helpers.h"

/// @file This file contains all tests for the compiler::Module object.

/// @brief Test fixture for testing behaviour of the
/// compiler::Module::createBinary API.
struct CreateBinaryTest : OpenCLCModuleTest {};

TEST_P(CreateBinaryTest, CreateBinary) {
  cargo::array_view<std::uint8_t> buffer{};
  ASSERT_EQ(compiler::Result::SUCCESS, module->createBinary(buffer));

  // If this is a cross compiler (and we have no associated device), stop the
  // test here.
  if (!optional_device.has_value()) {
    return;
  }

  // Otherwise, we can verify that the executable can actually be created.
  auto device = *optional_device;

  mux_executable_t executable;
  ASSERT_EQ(mux_success,
            muxCreateExecutable(device, buffer.data(), buffer.size(), allocator,
                                &executable));

  mux_kernel_t kernel;
  ASSERT_EQ(mux_success,
            muxCreateKernel(device, executable, "nop", std::strlen("nop"),
                            allocator, &kernel));

  mux_ndrange_options_t nd_range_options{};
  nd_range_options.descriptors = nullptr;
  nd_range_options.descriptors_length = 0;
  size_t local_size[]{1, 1, 1};
  std::memcpy(nd_range_options.local_size, local_size, sizeof(local_size));
  const size_t global_offset = 1;
  nd_range_options.global_offset = &global_offset;
  const size_t global_size = 1;
  nd_range_options.global_size = &global_size;
  nd_range_options.dimensions = 1;

  mux_command_buffer_t command_buffer;
  ASSERT_EQ(mux_success, muxCreateCommandBuffer(device, nullptr, allocator,
                                                &command_buffer));

  ASSERT_EQ(mux_success,
            muxCommandNDRange(command_buffer, kernel, nd_range_options, 0,
                              nullptr, nullptr));

  muxDestroyCommandBuffer(device, command_buffer, allocator);
  muxDestroyKernel(device, kernel, allocator);
  muxDestroyExecutable(device, executable, allocator);
}

INSTANTIATE_COMPILER_TARGET_TEST_SUITE_P(CreateBinaryTest);

/// @brief Test fixture for testing behaviour of the
/// compiler::Module::getOptions API.
struct CompileOptionsTest : CompilerModuleTest {
  /// @brief Helper type holding an option name and a bool indicating whether
  /// the option takes an argument.
  struct BuildOption {
    std::string option_name;
    bool takes_argument;
  };

  /// @brief All options supported by device.
  cargo::small_vector<BuildOption, 2> compiler_build_options;

  /// @brief Virtual override to set up resources used by this fixture.
  void SetUp() override {
    RETURN_ON_SKIP_OR_FATAL_FAILURE(CompilerModuleTest::SetUp());
    // Options are semi-colon separated
    auto options_split = cargo::split(compiler_info->compilation_options, ";");
    // Verify options reported by device are valid according to the compiler
    // spec before testing them
    for (auto option : options_split) {
      auto tuple = cargo::split(option, ",");
      ASSERT_EQ(3, tuple.size());

      const cargo::string_view arg_name = tuple[0];
      ASSERT_TRUE(arg_name.starts_with("--"));
      ASSERT_EQ(cargo::string_view::npos,
                arg_name.find_first_of(" \t\n\v\f\r"));

      const cargo::string_view takes_value = tuple[1];
      ASSERT_TRUE(takes_value == "1" || takes_value == "0");

      const cargo::string_view help = tuple[2];
      ASSERT_EQ(cargo::string_view::npos, help.find_first_of("\t\n\v\f\r"));

      BuildOption build_option = {
          std::string(arg_name.data(), arg_name.length()),
          takes_value[0] == '1'};
      ASSERT_EQ(cargo::success,
                compiler_build_options.push_back(std::move(build_option)));
    }
  }

  /// @brief Helper function to avoid code duplication.
  ///
  /// Loads a nop OpenCLC kernel into the module, compiles and finalizes it then
  /// creates a binary. This assumes any options that are being tested have
  /// already been set.
  void compileProgram() {
    const cargo::string_view nop_clc_source = "kernel void nop(){}";
    ASSERT_EQ(compiler::Result::SUCCESS,
              module->compileOpenCLC(
                  mux::detectOpenCLProfile(compiler_info->device_info),
                  nop_clc_source, {}));
    std::vector<builtins::printf::descriptor> printf_calls{};
    ASSERT_EQ(compiler::Result::SUCCESS,
              module->finalize(nullptr, printf_calls));
    cargo::array_view<std::uint8_t> buffer;
    ASSERT_EQ(compiler::Result::SUCCESS, module->createBinary(buffer));
  }
};

TEST_P(CompileOptionsTest, NoOptBuildFlag) {
  // Set the options.
  module->getOptions().opt_disable = true;

  // Check we can actually compile some source with this
  ASSERT_NO_FATAL_FAILURE(compileProgram());
}

TEST_P(CompileOptionsTest, EmptyOption) {
  // Set the options.
  module->getOptions().device_args = "";

  // Check we can actually compile some source with this
  ASSERT_NO_FATAL_FAILURE(compileProgram());
}

TEST_P(CompileOptionsTest, IndividualOptions) {
  if (compiler_build_options.empty()) {
    GTEST_SKIP();
  }

  for (const auto &option : compiler_build_options) {
    std::string test_option = option.option_name;
    if (option.takes_argument) {
      test_option.append(",test value");
    }
    module->getOptions().device_args = test_option;

    // Check we can actually compile some source with this
    ASSERT_NO_FATAL_FAILURE(compileProgram());
  }

  for (const auto &option : compiler_build_options) {
    std::string test_option = option.option_name;
    if (option.takes_argument) {
      test_option.append(",test value");
    } else {
      // Pass an empty value after comma separator
      test_option.push_back(',');
    }
    module->getOptions().device_args = test_option;

    // Check we can actually compile some source with this
    ASSERT_NO_FATAL_FAILURE(compileProgram());
  }
}

TEST_P(CompileOptionsTest, CombinedOptions) {
  if (compiler_build_options.empty()) {
    GTEST_SKIP();
  }

  std::string test_options;
  for (const auto &option : compiler_build_options) {
    test_options.append(option.option_name);
    test_options.push_back(',');
    if (option.takes_argument) {
      test_options.append(",test value");
    }
    test_options.push_back(';');
  }
  test_options.pop_back();  // Remove excess trailing ';'
  module->getOptions().device_args = test_options;

  // Check we can actually compile some source with this
  ASSERT_NO_FATAL_FAILURE(compileProgram());
}

INSTANTIATE_COMPILER_TARGET_TEST_SUITE_P(CompileOptionsTest);

/// @brief Test fixture for testing behaviour of the serialization and
/// deserialization API.
struct SerializeModuleTest : CompilerModuleTest {
  /// @brief Virtual method to setup any resources this fixture requires.
  void SetUp() override {
    RETURN_ON_SKIP_OR_FATAL_FAILURE(CompilerModuleTest::SetUp());

    // Compile a program, but don't finalize it yet.
    const cargo::string_view kernel_source = "kernel void nop(){}";
    ASSERT_EQ(compiler::Result::SUCCESS,
              module->compileOpenCLC(
                  mux::detectOpenCLProfile(compiler_info->device_info),
                  kernel_source, {}));
  }

  /// @brief Virtual method to clean up any resources this fixture allocated.
  void TearDown() override {
    module->clear();
    CompilerModuleTest::TearDown();
  }
};

TEST_P(SerializeModuleTest, DeserializeFailure) {
  // Expect that an empty buffer is 'successfully' deserialized.
  std::vector<uint8_t> empty_buffer;
  EXPECT_TRUE(module->deserialize(empty_buffer));

  // Try a buffer that is known not to be a valid LLVM module.
  std::vector<uint8_t> garbage_buffer = {
      5,
      1,
      0,
  };
  EXPECT_FALSE(module->deserialize(garbage_buffer));
}

TEST_P(SerializeModuleTest, SerializeIntermediate) {
  const size_t size = module->size();
  ASSERT_GT(size, 0);

  std::vector<uint8_t> buffer;
  buffer.resize(size);
  ASSERT_EQ(size, module->serialize(buffer.data()));

  // Create a new module by deserializing the buffer.
  uint32_t num_errors = 0;
  std::string log;
  std::unique_ptr<compiler::Module> cloned_module =
      target->createModule(num_errors, log);
  ASSERT_NE(cloned_module, nullptr);

  ASSERT_TRUE(cloned_module->deserialize(buffer));
  ASSERT_EQ(num_errors, 0);
  EXPECT_EQ(module->getState(), cloned_module->getState());

  std::vector<builtins::printf::descriptor> printf_calls;
  ASSERT_EQ(compiler::Result::SUCCESS,
            cloned_module->finalize(nullptr, printf_calls));
  EXPECT_EQ(cloned_module->getState(), compiler::ModuleState::EXECUTABLE);
}

TEST_P(SerializeModuleTest, SerializeLibraryThenLink) {
  // Create a library module and link with it.
  {
    uint32_t num_errors = 0;
    std::string log;
    std::unique_ptr<compiler::Module> library_module =
        target->createModule(num_errors, log);
    ASSERT_NE(library_module, nullptr);

    const cargo::string_view library_source = "void library_func(){}";
    ASSERT_EQ(compiler::Result::SUCCESS,
              library_module->compileOpenCLC(
                  mux::detectOpenCLProfile(compiler_info->device_info),
                  library_source, {}));

    std::vector<compiler::Module *> library_modules = {library_module.get()};
    ASSERT_EQ(compiler::Result::SUCCESS, module->link(library_modules));
    EXPECT_EQ(module->getState(), compiler::ModuleState::LIBRARY);
  }

  const size_t size = module->size();
  ASSERT_GT(size, 0);

  std::vector<uint8_t> buffer;
  buffer.resize(size);
  ASSERT_EQ(size, module->serialize(buffer.data()));

  // Create a new module by deserializing the buffer.
  uint32_t num_errors = 0;
  std::string log;
  std::unique_ptr<compiler::Module> cloned_module =
      target->createModule(num_errors, log);
  ASSERT_NE(cloned_module, nullptr);

  ASSERT_TRUE(cloned_module->deserialize(buffer));
  ASSERT_EQ(num_errors, 0);
  EXPECT_EQ(module->getState(), cloned_module->getState());

  std::vector<builtins::printf::descriptor> printf_calls;
  ASSERT_EQ(compiler::Result::SUCCESS,
            cloned_module->finalize(nullptr, printf_calls));
  EXPECT_EQ(cloned_module->getState(), compiler::ModuleState::EXECUTABLE);
}

TEST_P(SerializeModuleTest, SerializeFinalized) {
  std::vector<builtins::printf::descriptor> printf_calls;
  ASSERT_EQ(compiler::Result::SUCCESS, module->finalize(nullptr, printf_calls));

  const size_t size = module->size();
  ASSERT_GT(size, 0);

  std::vector<uint8_t> buffer;
  buffer.resize(size);
  ASSERT_EQ(size, module->serialize(buffer.data()));

  // Create a new module by deserializing the buffer.
  uint32_t num_errors = 0;
  std::string log;
  std::unique_ptr<compiler::Module> cloned_module =
      target->createModule(num_errors, log);
  ASSERT_NE(cloned_module, nullptr);

  ASSERT_TRUE(cloned_module->deserialize(buffer));
  ASSERT_EQ(num_errors, 0);
  EXPECT_EQ(module->getState(), cloned_module->getState());
}

INSTANTIATE_COMPILER_TARGET_TEST_SUITE_P(SerializeModuleTest);
