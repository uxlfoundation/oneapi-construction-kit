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

#include "uur/environment.h"

#include "cargo/string_view.h"
#include "cargo/utility.h"

// ColoredPrintf is internal to GoogleTest and we have to patch GoogleTest to be
// able to refer to it at all. Still, it is easier than reinventing the wheel.
namespace testing::internal {
enum class GTestColor { kDefault, kRed, kGreen, kYellow };
void ColoredPrintf(GTestColor color, const char *fmt, ...);
}  // namespace testing::internal

// This assumes to be run from build folder
const char *const KERNELS_BIN_RELATIVE_PATH = "../source/ur/test/kernels";

std::ostream &operator<<(std::ostream &out,
                         const std::vector<ur_platform_handle_t> &platforms) {
  for (auto platform : platforms) {
    size_t size;
    urPlatformGetInfo(platform, UR_PLATFORM_INFO_NAME, 0, nullptr, &size);
    std::vector<char> name(size);
    urPlatformGetInfo(platform, UR_PLATFORM_INFO_NAME, size, name.data(),
                      nullptr);
    out << "\n  * \"" << name.data() << "\"";
  }
  return out;
}

uur::Environment::Environment(int argc, char **argv)
    : options{parseOptions(argc, argv)} {
  using namespace std::string_literals;
  Environment::instance = this;
  platform = cargo::make_unexpected("platform uninitialized"s);
  devices = cargo::make_unexpected("devices uninitialized"s);
  const ur_device_init_flags_t device_flags = 0;
  if (urInit(device_flags)) {
    platform = cargo::make_unexpected("urInit() failed"s);
    return;
  }

  // Select platform based on the command-line --platform=NAME option.
  uint32_t count;
  if (urPlatformGet(0, nullptr, &count)) {
    platform = cargo::make_unexpected("urPlatformGet() failed"s);
    return;
  }
  std::vector<ur_platform_handle_t> platforms(count);
  if (urPlatformGet(count, platforms.data(), nullptr)) {
    platform = cargo::make_unexpected("urPlatformGet() failed"s);
    return;
  }
  if (options.platform_name.empty()) {
    if (count == 1) {
      platform = platforms[0];
    } else {
      std::stringstream error;
      error << "Select a single platform from below using the --platform=NAME "
               "command-line option:"
            << platforms;
      platform = cargo::make_unexpected(error.str());
      return;
    }
  } else {
    for (auto candidate : platforms) {
      size_t size;
      if (urPlatformGetInfo(candidate, UR_PLATFORM_INFO_NAME, 0, nullptr,
                            &size)) {
        platform = cargo::make_unexpected("urPlatformGetInfo() failed"s);
        return;
      }
      std::vector<char> name(size);
      if (urPlatformGetInfo(candidate, UR_PLATFORM_INFO_NAME, size, name.data(),
                            nullptr)) {
        platform = cargo::make_unexpected("urPlatformGetInfo() failed"s);
        return;
      }
      if (options.platform_name == name.data()) {
        platform = candidate;
        break;
      }
    }
    if (!platform) {
      std::stringstream error;
      error << "Platform \"" << options.platform_name
            << "\" not found. Select a single platform from below using the "
               "--platform=NAME command-line options:"
            << platforms;
      platform = cargo::make_unexpected(error.str());
      return;
    }
  }

  if (options.kernel_directory.empty()) {
    options.kernel_directory = KERNELS_BIN_RELATIVE_PATH;
  }

  if (urDeviceGet(getPlatform(), UR_DEVICE_TYPE_ALL, 0, nullptr, &count)) {
    devices = cargo::make_unexpected("urDeviceGet failed"s);
    return;
  }
  if (count == 0) {
    devices = cargo::make_unexpected("urDeviceGet no devices found"s);
    return;
  }
  devices = std::vector<ur_device_handle_t>(count);
  if (urDeviceGet(getPlatform(), UR_DEVICE_TYPE_ALL, count, devices->data(),
                  nullptr)) {
    devices = cargo::make_unexpected("urDeviceGet failed"s);
    return;
  }
}

void uur::Environment::SetUp() {
  if (!platform) {
    FAIL() << platform.error();
  }
}

void uur::Environment::TearDown() {
  ur_tear_down_params_t tear_down_params{};
  urTearDown(&tear_down_params);
  clearCachedKernels();
}

uur::Environment *uur::Environment::instance = nullptr;

namespace {
void printHelp(const char *arg0) {
  (void)arg0;
  using namespace testing::internal;

  printf("Platform Selection:\n");

  ColoredPrintf(GTestColor::kGreen, "  --platform=");
  ColoredPrintf(GTestColor::kYellow, "NAME\n");
  printf(
      "      Run the tests on the specified platform, if there are multiple\n"
      "      platforms this option is required to select a single platform.\n");

  ColoredPrintf(GTestColor::kGreen, "  --kernel_directory=");
  ColoredPrintf(GTestColor::kYellow, "PATH\n");
  printf(
      "      Provide the path to the supplied 'kernels' directory. "
      "Default:\n");
  ColoredPrintf(GTestColor::kYellow, "      %s\n", KERNELS_BIN_RELATIVE_PATH);

  printf("\n");
}
}  // namespace

uur::Environment::Options uur::Environment::parseOptions(int argc,
                                                         char **argv) {
  uur::Environment::Options options;
  for (int argi = 1; argi < argc; argi++) {
    const cargo::string_view arg{argv[argi]};
    if (arg == "-h" || arg == "--help") {
      printHelp(argv[0]);
      break;
    } else if (arg.starts_with("--platform=")) {
      options.platform_name =
          cargo::as<std::string>(*arg.substr(std::strlen("--platform=")));
    } else if (arg.starts_with("--kernel_directory=")) {
      options.kernel_directory = cargo::as<std::string>(
          *arg.substr(std::strlen("--kernel_directory=")));
    }
  }
  return options;
}

std::string uur::Environment::getSupportedILPostfix(uint32_t device_index) {
  std::stringstream IL;

  auto device = instance->getDevices()[device_index];
  size_t size;
  if (urDeviceGetInfo(device, UR_DEVICE_INFO_IL_VERSION, 0, nullptr, &size)) {
    (void)std::fprintf(stderr, "ERROR: Getting device IL version\n");
    return "";
  }
  std::string IL_version(size, '\0');
  if (urDeviceGetInfo(device, UR_DEVICE_INFO_IL_VERSION, size,
                      IL_version.data(), nullptr)) {
    (void)std::fprintf(stderr, "ERROR: Getting device IL version\n");
    return "";
  }

  // Delete the ETX character at the end as it is not part of the name.
  IL_version.pop_back();

  IL << "_" << IL_version;

  // TODO: Add other IL types like ptx when they are defined how they will be
  // reported.
  if (IL_version.find("SPIR-V") != std::string::npos) {
    IL << ".spv";
  } else {
    (void)std::fprintf(stderr, "ERROR: Undefined IL version\n");
    return "";
  }

  return IL.str();
}

std::string uur::Environment::getKernelSourcePath(
    const std::string &kernel_name, uint32_t device_index) {
  std::stringstream path;
  path << instance->getKernelDirectory();
  // il_postfix = supported_IL(SPIRV-PTX-...) + IL_version + extension(.spv -
  // .ptx - ....)
  const std::string il_postfix = getSupportedILPostfix(device_index);

  if (il_postfix.empty()) {
    (void)std::fprintf(stderr, "ERROR: Getting device supported IL\n");
    return "";
  }

  path << "/" << kernel_name << il_postfix;

  uint32_t address_bits;
  auto device = Environment::instance->getDevices()[device_index];
  if (urDeviceGetInfo(device, UR_DEVICE_INFO_ADDRESS_BITS, sizeof(uint32_t),
                      &address_bits, nullptr)) {
    (void)std::fprintf(stderr,
                       "ERROR: Getting device address bits supported\n");
    return "";
  }
  path << address_bits;

  return path.str();
}

uur::Environment::KernelSource uur::Environment::LoadSource(
    std::string kernel_name, uint32_t device_index) {
  std::string source_path =
      instance->getKernelSourcePath(kernel_name, device_index);

  if (source_path.empty()) {
    (void)std::fprintf(stderr,
                       "ERROR: Retrieving kernel source path for kernel: %s\n",
                       kernel_name.data());
    return KernelSource{kernel_name.data(), nullptr, 0,
                        UR_RESULT_ERROR_INVALID_BINARY};
  }

  if (cached_kernels.find(source_path) != cached_kernels.end()) {
    return cached_kernels[source_path];
  }

  FILE *source_file = fopen(source_path.data(), "rb");

  if (!source_file) {
    (void)std::fprintf(stderr, "ERROR: Opening the kernel path: %s\n",
                       source_path.data());
    return KernelSource{kernel_name.data(), nullptr, 0,
                        UR_RESULT_ERROR_INVALID_BINARY};
  }

  const long source_size = [&] {
    if (fseek(source_file, 0, SEEK_END)) return -1L;
    const long source_size = ftell(source_file);
    if (fseek(source_file, 0, SEEK_SET)) return -1L;
    return source_size;
  }();
  if (source_size < 0) {
    (void)std::fprintf(stderr, "ERROR: Determining kernel image size: %s\n",
                       source_path.data());
    return KernelSource{kernel_name.data(), nullptr, 0,
                        UR_RESULT_ERROR_INVALID_BINARY};
  }

  uint32_t *source = static_cast<uint32_t *>(malloc(source_size));
  if (fread(source, sizeof(uint32_t), source_size / sizeof(uint32_t),
            source_file) != source_size / sizeof(uint32_t)) {
    (void)fclose(source_file);
    free(source);
    (void)std::fprintf(stderr,
                       "ERROR: Reading kernel source data from file: %s\n",
                       source_path.data());
    return KernelSource{kernel_name.data(), nullptr, 0,
                        UR_RESULT_ERROR_INVALID_BINARY};
  }
  (void)fclose(source_file);

  const KernelSource kernel_source = KernelSource{
      kernel_name.data(), source, uint32_t(source_size), UR_RESULT_SUCCESS};

  return cached_kernels[source_path] = kernel_source;
}

void uur::Environment::clearCachedKernels() {
  for (const auto &kernel : cached_kernels) {
    free(kernel.second.source);
  }
  cached_kernels.clear();
}
