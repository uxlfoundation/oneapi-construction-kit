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

#include <common.h>

namespace {
static cl_platform_id selectedPlatform = nullptr;
}  // namespace

cl_platform_id getPlatform() { return selectedPlatform; }

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  // Get OpenCL platforms.
  cl_uint numPlatforms = 0;
  if (auto error = clGetPlatformIDs(0, nullptr, &numPlatforms)) {
    std::fprintf(stderr, "error: call to clGetDeviceIDs failed\n");
    return error;
  }
  if (numPlatforms == 0) {
    std::fprintf(stderr, "error: could not find any OpenCL platforms\n");
    return -1;
  }
  std::vector<cl_platform_id> platforms(numPlatforms);
  if (auto error = clGetPlatformIDs(numPlatforms, platforms.data(), nullptr)) {
    std::fprintf(stderr, "error: call to clGetDeviceIDs failed\n");
    return error;
  }

  // Get OpenCL platform names.
  std::vector<std::string> platformNames;
  platformNames.reserve(numPlatforms);
  for (cl_platform_id platform : platforms) {
    size_t size;
    if (auto error =
            clGetPlatformInfo(platform, CL_PLATFORM_NAME, 0, nullptr, &size)) {
      std::fprintf(stderr, "error: could not get platform name\n");
      return error;
    }
    std::vector<char> platformName(size, '\0');
    if (auto error = clGetPlatformInfo(platform, CL_PLATFORM_NAME, size,
                                       platformName.data(), nullptr)) {
      std::fprintf(stderr, "error: could not get platform name\n");
      return error;
    }
    platformNames.emplace_back(
        platformName.begin(),
        std::find(platformName.begin(), platformName.end(), '\0'));
  }

  // Parse additional arguments.
  for (int argi = 1; argi < argc; argi++) {
    std::string arg{argv[argi]};
    if (arg.find("-h") != std::string::npos ||
        arg.find("--help") != std::string::npos) {
      std::printf(R"(
MultiDevice (version %s) - Options:
  --opencl-platform=<name>
      The CL_PLATFORM_NAME of the platform to be tested.
)",
                  CA_VERSION);
      return 0;
    } else if (arg.find("--opencl-platform=") != std::string::npos) {
      const std::string platformName(arg.begin() + 1 + arg.find('='),
                                     arg.end());
      for (cl_uint index = 0; index < numPlatforms; index++) {
        if (platformName == platformNames[index]) {
          selectedPlatform = platforms[index];
        }
      }
      if (selectedPlatform == nullptr) {
        std::fprintf(stderr, "error: could not find platform name: %s\n",
                     platformName.c_str());
        return -1;
      }
    } else {
      std::fprintf(stderr, "error: invalid platform: %s\n", arg.c_str());
      return -1;
    }
  }

  // Handle no selected platform.
  if (!selectedPlatform) {
    if (platforms.size() == 1) {
      selectedPlatform = platforms[0];
    } else if (platforms.size() > 1) {
      std::fprintf(
          stderr,
          "error: multiple OpenCL platforms, use --opencl-platform=<name>\n");
      std::fprintf(stderr, "choose for the following:\n");
      for (const auto &platformName : platformNames) {
        std::fprintf(stderr, "* %s\n", platformName.c_str());
      }
    }
  }

  return RUN_ALL_TESTS();
}
