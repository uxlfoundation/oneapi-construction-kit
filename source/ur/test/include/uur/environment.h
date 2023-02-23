// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef UUR_ENVIRONMENT_H_INCLUDED
#define UUR_ENVIRONMENT_H_INCLUDED

#include <unordered_map>

#include "cargo/array_view.h"
#include "cargo/expected.h"
#include "gtest/gtest.h"
#include "uur/checks.h"

namespace uur {
struct Environment : testing::Environment {
  struct Options {
    std::string platform_name;
    std::string kernel_directory;
  };

  struct KernelSource {
    const char *kernel_name;
    uint32_t *source;
    uint32_t source_length;
    ur_result_t status;
  };

  Environment(int argc, char **argv);
  virtual ~Environment() {}

  void SetUp() override;
  void TearDown() override;

  static Environment *instance;

  ur_platform_handle_t getPlatform() { return platform.value_or(nullptr); }

  cargo::array_view<ur_device_handle_t> getDevices() {
    if (devices) {
      return *devices;
    }
    return {};
  }

  std::string getKernelDirectory() { return options.kernel_directory; }

  KernelSource LoadSource(std::string kernel_name, uint32_t device_index);

 private:
  Options parseOptions(int argc, char **argv);

  std::string getKernelSourcePath(const std::string &kernel_name,
                                  uint32_t device_index);

  std::string getSupportedILPostfix(uint32_t device_index);

  void clearCachedKernels();

  Options options;
  cargo::expected<ur_platform_handle_t, std::string> platform;
  cargo::expected<std::vector<ur_device_handle_t>, std::string> devices;
  // mapping between kernels (full_path + kernel_name) and their saved source.
  std::unordered_map<std::string, KernelSource> cached_kernels;
};
}  // namespace uur

#endif  // UUR_ENVIRONMENT_H_INCLUDED
