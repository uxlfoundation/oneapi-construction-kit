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

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "Common.h"

// ColoredPrintf is internal to GoogleTest and we have to patch GoogleTest to be
// able to refer to it at all. Still, it is easier than reinventing the wheel.
namespace testing::internal {
enum class GTestColor { kDefault, kRed, kGreen, kYellow };
extern void ColoredPrintf(GTestColor color, const char *fmt, ...);
}  // namespace testing::internal

#ifdef _WIN32
#include <windows.h>

namespace {

const char *const KERNELS_EXE_RELATIVE_PATH = "..\\share\\kernels";
const char *const INCLUDE_EXE_RELATIVE_PATH = "..\\share\\test_include";

std::string get_path_relative_to_exe(const std::string &relative_path) {
  // no way to query the buffer length beforehand
  std::vector<char> lnpath(MAX_PATH + 2);
  // guaranteed to return backslash-separated path
  DWORD sz = GetModuleFileNameA(nullptr, lnpath.data(),
                                static_cast<DWORD>(lnpath.size()));
  if (sz == 0) {
    int err = GetLastError();
    fprintf(stderr,
            "Could not get executable path (GetModuleFileNameA failed with "
            "error %d\n",
            err);
    exit(1);
  }
  // sz does not include the terminating null byte
  if (sz > lnpath.size() - 2) {
    fprintf(stderr, "Executable path longer than %zu bytes\n",
            lnpath.size() - 1);
    exit(1);
  }
  std::string path(lnpath.begin(), lnpath.begin() + sz);
  // trim to last \ (e.g. "C:\a\b\c" to "C:\a\b\"), to get UnitCL's directory
  size_t idx = path.find_last_of('\\');
  if (idx != std::string::npos) {
    path.resize(idx + 1);
  } else {
    path = ".\\";
  }
  return path + relative_path;
}

}  // namespace

#elif defined(__linux__) || defined(__APPLE__) || defined(__QNX__)
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace {

const char *const KERNELS_EXE_RELATIVE_PATH = "../share/kernels";
const char *const INCLUDE_EXE_RELATIVE_PATH = "../share/test_include";

std::string get_path_relative_to_exe(const std::string &relative_path) {
#ifdef __APPLE__
  std::vector<char> lnpath(PATH_MAX + 1);
  std::uint32_t size = lnpath.size();
  if (auto error = _NSGetExecutablePath(lnpath.data(), &size)) {
    perror("Could not get executable path (_NSGetExecutablePath())");
    std::exit(error);
  }
  std::string path(lnpath.data(), std::strlen(lnpath.data()));
#else
  const char *const self_exe = "/proc/self/exe";
  // stat() doesn't report link length, because it is in /proc
  std::vector<char> lnpath(4096);
  auto sz = readlink(self_exe, lnpath.data(), lnpath.size());
  if (sz < 0) {
    perror("Could not get executable path (readlink /proc/self/exe failed)\n");
    exit(1);
  }
  if (sz > static_cast<ssize_t>(lnpath.size() - 1)) {
    (void)fprintf(stderr, "Executable path longer than %zu bytes\n",
                  lnpath.size());
    exit(1);
  }
  std::string path(lnpath.begin(), lnpath.begin() + sz);
#endif
  // trim to last / (e.g. "/a/b/c" to "/a/b/"), to get UnitCL's directory
  const size_t idx = path.find_last_of('/');
  if (idx != std::string::npos) {
    path.resize(idx + 1);
  } else {
    path = "./";
  }
  return path + relative_path;
}

}  // namespace

#else

namespace {

const char *const KERNELS_EXE_RELATIVE_PATH = "source/cl/test/UnitCL/kernels";
const char *const INCLUDE_EXE_RELATIVE_PATH =
    "source/cl/test/UnitCL/test_include";
#warning \
    "Unsupported platform - no code for determining current executable path, always using '.'"
std::string get_path_relative_to_exe(const std::string &relative_path) {
  return relative_path;
}
}  // namespace
#endif

namespace {

bool is_opt(const char *const argv, const char *const opt) {
  return 0 == strncmp(opt, argv, strlen(opt));
}

bool starts_with(const char *const argv, const char *const opt) {
  return 0 == strncmp(opt, argv, strlen(opt));
}

struct ArgumentParser {
  ArgumentParser(int argc, char *argv[])
      : platform(""),
        includePath(get_path_relative_to_exe(INCLUDE_EXE_RELATIVE_PATH)),
        math_mode(ucl::MathMode::WIMPY) {
    for (int i = 1; i < argc; i++) {
      if ((0 == strcmp("-h", argv[i])) || (0 == strcmp("--help", argv[i]))) {
        printHelp(argv[0]);
      } else if (is_opt(argv[i], "--unitcl_test_include=")) {
        includePath = argv[i] + strlen("--unitcl_test_include=");
      } else if (is_opt(argv[i], "--unitcl_platform=")) {
        platform = argv[i] + strlen("--unitcl_platform=");
      } else if (is_opt(argv[i], "--unitcl_device=")) {
        device = argv[i] + strlen("--unitcl_device=");
      } else if (is_opt(argv[i], "--unitcl_kernel_directory=")) {
        kernel_directory = argv[i] + strlen("--unitcl_kernel_directory=");
      } else if (is_opt(argv[i], "--unitcl_build_options=")) {
        build_options = argv[i] + strlen("--unitcl_build_options=");
      } else if (is_opt(argv[i], "--unitcl_seed=")) {
        const std::string seed_str = argv[i] + strlen("--unitcl_seed=");
        rand_seed = std::stoul(seed_str);
      } else if (is_opt(argv[i], "--unitcl_math=")) {
        const std::string value = argv[i] + strlen("--unitcl_math=");
        if (0 == value.compare("quick")) {
          math_mode = ucl::MathMode::QUICK;
        } else if (0 == value.compare("full")) {
          math_mode = ucl::MathMode::FULL;
        } else if (0 != value.compare("wimpy")) {
          (void)fprintf(stderr, "ERROR: invalid math mode selected\n");
          exit(-1);
        }
      } else if (is_opt(argv[i], "--vecz-check")) {
        vecz_check = true;
      } else if (is_opt(argv[i], "--opencl_info")) {
        const bool print_ok = UCL::print_opencl_platform_and_device_info(
            CL_DEVICE_TYPE_ALL | CL_DEVICE_TYPE_CUSTOM);
        if (!print_ok) {
          (void)fprintf(
              stderr,
              "WARNING: Unable to query and print OpenCL platform and "
              "device info.\n");
        }
        exit(0);  // Don't run any tests, just exit now we've dumped the info
      } else if (!starts_with(argv[i], "--gtest")) {
        (void)fprintf(stderr, "ERROR : Unknown argument '%s'.\n", argv[i]);
        printHelp(argv[0]);
        exit(1);  // Don't run any tests, just exit
      }
    }
  }

  void printHelp(const char *arg0) {
    using namespace testing::internal;

    printf("%s (v%s)\n\n", arg0, STR(CA_VERSION));

    printf("UnitCL Options:\n");
    ColoredPrintf(GTestColor::kGreen, "  --unitcl_test_include=");
    ColoredPrintf(GTestColor::kYellow, "DIRECTORY_PATH\n");
    printf(
        "      Provide the path to the supplied 'test_include' directory. "
        "Default:\n");
    ColoredPrintf(GTestColor::kYellow, "      %s\n", includePath.c_str());

    ColoredPrintf(GTestColor::kGreen, "  --unitcl_platform=");
    ColoredPrintf(GTestColor::kYellow, "VENDOR\n");
    printf("      Provide an OpenCL platform vendor to use for testing.\n");

    ColoredPrintf(GTestColor::kGreen, "  --unitcl_device=");
    ColoredPrintf(GTestColor::kYellow, "DEVICE_NAME\n");
    printf("      Provide an OpenCL device name to use for testing.\n");

    ColoredPrintf(GTestColor::kGreen, "  --unitcl_kernel_directory=");
    ColoredPrintf(GTestColor::kYellow, "PATH\n");
    printf(
        "      Provide the path to the supplied 'kernels' directory. "
        "Default:\n");
    ColoredPrintf(GTestColor::kYellow, "      %s\n",
                  get_path_relative_to_exe(KERNELS_EXE_RELATIVE_PATH).c_str());

    ColoredPrintf(GTestColor::kGreen, "  --unitcl_build_options=");
    ColoredPrintf(GTestColor::kYellow, "OPTION_STRING\n");
    printf(
        "      Provide compilation options to pass to clBuildProgram() when\n"
        "      compiling kernels in the 'kernels' directory.\n");

    ColoredPrintf(GTestColor::kGreen, "  --unitcl_seed=");
    ColoredPrintf(GTestColor::kYellow, "NUMBER\n");
    printf(
        "      Provide an unsigned integer to seed the random number "
        "generator with.\n");

    ColoredPrintf(GTestColor::kGreen, "  --unitcl_math=");
    ColoredPrintf(GTestColor::kYellow, "(");
    ColoredPrintf(GTestColor::kGreen, "quick");
    ColoredPrintf(GTestColor::kYellow, "|");
    ColoredPrintf(GTestColor::kGreen, "wimpy");
    ColoredPrintf(GTestColor::kYellow, "|");
    ColoredPrintf(GTestColor::kGreen, "full");
    ColoredPrintf(GTestColor::kYellow, ")\n");
    printf(
        "      Run math builtins tests over an increasing data size, "
        "defaults to wimpy.\n");

    ColoredPrintf(GTestColor::kGreen, "  --vecz-check\n");
    printf(
        "      Mark tests as failed if the vectorizer did not vectorize "
        "them.\n");

    ColoredPrintf(GTestColor::kGreen, "  --opencl_info\n");
    printf("      Print OpenCL platform and platform devices info.\n\n");
  }

  std::string platform;
  std::string device;
  std::string includePath;
  std::string build_options;

  // KTS
  std::string kernel_directory =
      get_path_relative_to_exe(KERNELS_EXE_RELATIVE_PATH);
  ucl::MathMode math_mode;
  bool vecz_check = false;
  unsigned rand_seed = 0;  // InputGenerator picks a random value if this is 0.
};
}  // namespace

int main(int argc, char **argv) {
  // First parse UnitCL specific arguments.
  const ArgumentParser parser(argc, argv);

  // Create our global environment, this must happen before
  // testing::InitGoogleTest() in order to support multi-device testing.
  ucl::Environment::instance = new ucl::Environment(
      parser.platform, parser.device, parser.includePath, parser.rand_seed,
      parser.math_mode, parser.build_options, parser.kernel_directory,
      parser.vecz_check);
  if (!(ucl::Environment::instance)) {
    (void)fprintf(stderr,
                  "ERROR: Could not initialize UnitCL global environment!\n");
    return -1;
  }

  // Let Google Test parse the arguments first
  testing::InitGoogleTest(&argc, argv);

  testing::Environment *const environment =
      testing::AddGlobalTestEnvironment(ucl::Environment::instance);

  if (environment != ucl::Environment::instance) {
    (void)fprintf(stderr,
                  "ERROR: UnitCL global environment did not match GoogleTest's "
                  "returned environment!\n");
    return -1;
  }

  return RUN_ALL_TESTS();
}
