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

#include <cargo/string_algorithm.h>
#include <cargo/string_view.h>

#include <vector>

#include "Common.h"
#include "Device.h"

using host_clGetDeviceInfoTest = ucl::DeviceTest;

TEST_F(host_clGetDeviceInfoTest, Name) {
  // Since this is a host-specific test we want to skip it if we aren't running
  // on host.
  if (!UCL::isDevice_host(device)) {
    GTEST_SKIP() << "Not running on host device, skipping test.\n";
  }
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr, &size));
  ASSERT_GE(size, 0u);
  UCL::Buffer<char> payload(size);
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_NAME, size, payload, nullptr));

  // Device name structure: Prefix Arch[ Windows]
  auto words = cargo::split(payload.data(), " ");
  ASSERT_TRUE(2 == words.size() || 3 == words.size())
      << "expected host device CL_DEVICE_NAME structure: Prefix Arch[ "
         "Windows]";

  // Does the first word match CA_HOST_CL_DEVICE_NAME_PREFIX?
  ASSERT_TRUE(words[0] == CA_HOST_CL_DEVICE_NAME_PREFIX);

#if defined(__arm__) || defined(__thumb__) || defined(_M_ARM) || \
    defined(_M_ARMT)
  const char *arch = "Arm";
#elif defined(__aarch64__)
  const char *arch = "AArch64";
#elif defined(__i386__) || defined(_M_IX86)
  const char *arch = "x86";
#elif defined(__x86_64__) || defined(_M_X64)
  const char *arch = "x86_64";
#elif defined(__riscv)
#if __riscv_xlen == 64
  const char *arch = "riscv64";
#elif __riscv_xlen == 32
  const char *arch = "riscv32";
#else
#error "32 bit or 64 bit xlen not defined for RISC-V"
#endif
#else
#error Unknown host system being compiled for!
#endif
  // Is the second word the expected architecture?
  ASSERT_TRUE(words[1] == arch);

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
  // On Windows, is the third word "Windows"?
  ASSERT_EQ(words.size(), 3);
  ASSERT_TRUE(words[2] == "Windows");
#endif
}

TEST_F(host_clGetDeviceInfoTest, VendorID) {
  // Since this is a host-specific test we want to skip it if we aren't running
  // on host.
  if (!UCL::isDevice_host(device)) {
    GTEST_SKIP() << "Not running on host device, skipping test.\n";
  }
  cl_uint vendor_id;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_VENDOR_ID, sizeof(cl_uint),
                                 &vendor_id, nullptr));

  // 0x10004 is our vendor ID CL_KHRONOS_VENDOR_ID_CODEPLAY
  ASSERT_EQ(0x10004, vendor_id);
  ASSERT_EQ(CL_KHRONOS_VENDOR_ID_CODEPLAY, vendor_id);
}
