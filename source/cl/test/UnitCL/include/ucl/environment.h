// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef UNITCL_ENVIRONMENT_H_INCLUDED
#define UNITCL_ENVIRONMENT_H_INCLUDED

#include <CL/cl.h>
#include <CL/cl_ext_codeplay.h>
#include <gtest/gtest.h>

#include <array>
#include <string>
#include <unordered_map>

#include "kts/generator.h"
#include "ucl/enums.h"
#include "ucl/version.h"

namespace ucl {
struct Environment : public testing::Environment {
  Environment(const std::string &platformVendor, const std::string &deviceName,
              const std::string &includePath, const unsigned randSeed,
              const ucl::MathMode &mathMode, const std::string &build_options,
              const std::string &kernel_directory, const bool vecz_check);

  void SetUp() override;
  void TearDown() override;

  std::string GetKernelDirectory() const { return kernel_dir_path; }
  std::string GetKernelBuildOptions() const { return kernel_build_options; }
  bool GetDoVectorizerCheck() const { return do_vectorizer_check; }

  cl_platform_id &GetPlatform() { return platform; }
  cl_device_id &GetDevice() { return device; }
  kts::ucl::InputGenerator &GetInputGenerator() { return generator; }

  std::string platformVendor;
  std::string deviceName;
  std::string deviceVersion;
  ucl::Version deviceOpenCLVersion;
  std::string platformOCLVersion;
  std::vector<cl_platform_id> platforms;
  std::vector<cl_device_id> devices;
  std::unordered_map<cl_device_id, cl_context> contexts;
  std::unordered_map<cl_context, cl_command_queue> command_queues;
  std::string testIncludePath;
  const ucl::MathMode math_mode;

  static Environment *instance;

 private:
  std::string kernel_dir_path;
  std::string kernel_build_options;
  cl_platform_id platform;
  cl_device_id device;
  bool do_vectorizer_check;
  kts::ucl::InputGenerator generator;
};
}  // namespace ucl

#endif  // UNITCL_ENVIRONMENT_H_INCLUDED
