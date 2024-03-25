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

#include "ucl/environment.h"

#include "cargo/string_algorithm.h"
#include "kts/precision.h"
#include "ucl/checks.h"

namespace {
std::string getPlatformVendor(cl_platform_id platform) {
  size_t size;
  if (auto error =
          clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, 0, nullptr, &size)) {
    (void)std::fprintf(stderr, "ERROR: Getting OpenCL platform vendor: %d\n",
                       error);
    std::exit(-1);
  }
  std::string platformVendor(size, '\0');
  if (auto error = clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, size,
                                     platformVendor.data(), nullptr)) {
    (void)std::fprintf(stderr, "ERROR: Getting OpenCL platform vendor: %d\n",
                       error);
    std::exit(-1);
  }
  return cargo::as<std::string>(cargo::trim(platformVendor));
}

std::string getDeviceName(cl_device_id device) {
  size_t size;
  if (auto error = clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr, &size)) {
    (void)std::fprintf(stderr, "ERROR: Getting OpencL device name: %d\n",
                       error);
    std::exit(-1);
  }
  std::string deviceName(size, '\0');
  if (auto error = clGetDeviceInfo(device, CL_DEVICE_NAME, size,
                                   deviceName.data(), nullptr)) {
    (void)std::fprintf(stderr, "ERROR: Getting OpencL device name: %d\n",
                       error);
    std::exit(-1);
  }
  return cargo::as<std::string>(cargo::trim(deviceName));
}
}  // namespace

ucl::Environment::Environment(
    const std::string &platformVendor, const std::string &deviceName,
    const std::string &includePath, const unsigned randSeed,
    const ucl::MathMode &mathMode, const std::string &build_options,
    const std::string &kernel_directory, const bool vecz_check)
    : platformVendor(platformVendor),
      deviceName(deviceName),
      deviceVersion(),
      deviceOpenCLVersion(),
      platformOCLVersion(),
      platforms(),
      devices(),
      testIncludePath(includePath),
      math_mode(mathMode),
      kernel_dir_path(kernel_directory),
      kernel_build_options(build_options),
      platform(nullptr),
      device(nullptr),
      do_vectorizer_check(vecz_check),
      generator(randSeed) {
  // Get the available platforms.
  cl_uint num_platforms;
  if (auto error = clGetPlatformIDs(0, nullptr, &num_platforms)) {
    (void)std::fprintf(stderr, "ERROR: Getting OpenCL platforms: %d\n", error);
    std::exit(-1);
  }
  if (num_platforms == 0) {
    (void)std::fprintf(stderr, "ERROR: No OpenCL platforms available\n");
    std::exit(-1);
  }
  platforms.resize(num_platforms);
  if (auto error = clGetPlatformIDs(num_platforms, platforms.data(), nullptr)) {
    (void)std::fprintf(stderr, "ERROR: Getting OpenCL platforms: %d\n", error);
    std::exit(-1);
  }

  // Select the platform to test.
  if (num_platforms == 1) {
    if (platformVendor.empty() ||
        platformVendor == getPlatformVendor(platforms[0])) {
      platform = platforms[0];
    }
  } else if (!platformVendor.empty()) {
    for (auto candidatePlatform : platforms) {
      if (platformVendor == getPlatformVendor(candidatePlatform)) {
        platform = candidatePlatform;
        break;
      }
    }
  }
  // Check a platform was actually found.
  if (!platform) {
    (void)std::fprintf(stderr,
                       "ERROR: OpenCL platform vendor not found: \"%s\"\n",
                       platformVendor.c_str());
    (void)std::fprintf(stderr,
                       "HINT: Use --unitcl_platform=VENDOR and choose from:\n");
    for (auto platform : platforms) {
      (void)std::fprintf(stderr, "  \"%s\"\n",
                         getPlatformVendor(platform).c_str());
    }
    std::exit(-1);
  }

  // Get the available devices.
  cl_uint num_devices;
  if (auto error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr,
                                  &num_devices)) {
    (void)std::fprintf(stderr, "ERROR: Getting OpenCL devices: %d\n", error);
    std::exit(-1);
  }
  if (num_devices == 0) {
    (void)std::fprintf(stderr, "ERROR: No OpenCL devices are available\n");
    std::exit(-1);
  }
  devices.resize(num_devices);
  if (auto error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, num_devices,
                                  devices.data(), nullptr)) {
    (void)std::fprintf(stderr, "ERROR: Getting OpenCL devices: %d\n", error);
    std::exit(-1);
  }

  // Select a specific device if the user asked.
  if (!deviceName.empty()) {
    cl_device_id selectedDevice = nullptr;
    for (auto device : devices) {
      if (deviceName == getDeviceName(device)) {
        selectedDevice = device;
        break;
      }
    }
    if (!selectedDevice) {
      (void)std::fprintf(stderr,
                         "ERROR: OpenCL device name not found: \"%s\"\n",
                         deviceName.c_str());
      (void)std::fprintf(stderr,
                         "HINT: Use --unitcl_device=NAME and choose from:\n");
      for (auto device : devices) {
        (void)std::fprintf(stderr, "  \"%s\"\n", getDeviceName(device).c_str());
      }
      std::exit(-1);
    }
    // Replace all available devices with the selected device.
    clRetainDevice(selectedDevice);
    for (auto device : devices) {
      clReleaseDevice(device);
    }
    devices.clear();
    devices.push_back(selectedDevice);
  }
}

void ucl::Environment::SetUp() {
  device = devices[0];

  // TODO(CA-3968): This doesn't make sense in a multi-device testing world
  size_t deviceVersionStringLength{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_VERSION, 0, nullptr,
                                 &deviceVersionStringLength));
  deviceVersion = std::string(deviceVersionStringLength, '\0');
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_VERSION,
                                 deviceVersion.length(), deviceVersion.data(),
                                 nullptr));

  // Device version string must be of the form "OpenCL [0-9]\.[0-9] .*".
  // This will break if major or minor versions ever exceed 10.
  auto dot_position = deviceVersion.find('.');
  deviceOpenCLVersion = ucl::Version(deviceVersion[dot_position - 1] - '0',
                                     deviceVersion[dot_position + 1] - '0');

  for (auto device : devices) {
    // Create a context per-device
    cl_int error;
    auto context = clCreateContext(nullptr, 1, &device, ucl::contextCallback,
                                   nullptr, &error);
    ASSERT_SUCCESS(error);
    contexts[device] = context;
    // Create a command-queue per-context
    auto command_queue = clCreateCommandQueue(context, device, 0, &error);
    ASSERT_SUCCESS(error);
    command_queues[context] = command_queue;
  }
}

void ucl::Environment::TearDown() {
  for (auto device : devices) {
    auto found_context = contexts.find(device);
    if (found_context != contexts.end()) {
      auto context = found_context->second;
      EXPECT_SUCCESS(clReleaseContext(context));
      auto found_command_queue = command_queues.find(context);
      if (found_command_queue != command_queues.end()) {
        auto command_queue = found_command_queue->second;
        EXPECT_SUCCESS(clReleaseCommandQueue(command_queue));
      }
    }
    EXPECT_SUCCESS(clReleaseDevice(device));
  }
}

ucl::Environment *ucl::Environment::instance = nullptr;
