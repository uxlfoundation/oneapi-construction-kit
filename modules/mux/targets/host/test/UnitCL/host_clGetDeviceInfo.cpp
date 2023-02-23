// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
