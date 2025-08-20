// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "Common.h"

using clCreateSubDevicesTest = ucl::DeviceTest;

TEST_F(clCreateSubDevicesTest, InvalidDevice) {
  cl_device_partition_property properties[] = {CL_DEVICE_PARTITION_EQUALLY, 1,
                                               0};
  cl_device_id subDevice = nullptr;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_DEVICE,
      clCreateSubDevices(nullptr, properties, 1, &subDevice, nullptr));
  ASSERT_FALSE(subDevice);
}

TEST_F(clCreateSubDevicesTest, InvalidNullProperties) {
  if (!UCL::hasSubDeviceSupport(device)) {
    GTEST_SKIP();
  }
  cl_device_id subDevice = nullptr;
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE, clCreateSubDevices(device, nullptr, 1,
                                                         &subDevice, nullptr));
  ASSERT_FALSE(subDevice);
}

TEST_F(clCreateSubDevicesTest, DevicePartitionEqually) {
  if (!UCL::hasDevicePartitionSupport(device, CL_DEVICE_PARTITION_EQUALLY)) {
    GTEST_SKIP();
  }
  cl_device_partition_property properties[] = {
      CL_DEVICE_PARTITION_EQUALLY, static_cast<cl_device_partition_property>(1),
      0};
  cl_uint numSubDevices = 0;
  EXPECT_SUCCESS(
      clCreateSubDevices(device, properties, 0, nullptr, &numSubDevices));
  ASSERT_GT(numSubDevices, 0u);
  UCL::vector<cl_device_id> subDevices(numSubDevices);
  ASSERT_SUCCESS(clCreateSubDevices(device, properties, numSubDevices,
                                    subDevices.data(), nullptr));
  for (UCL::vector<cl_device_id>::iterator iter = subDevices.begin(),
                                           iter_end = subDevices.end();
       iter != iter_end; ++iter) {
    ASSERT_SUCCESS(clReleaseDevice(*iter));
  }
}

TEST_F(clCreateSubDevicesTest, DevicePartitionByCounts) {
  if (!UCL::hasDevicePartitionSupport(device, CL_DEVICE_PARTITION_BY_COUNTS)) {
    GTEST_SKIP();
  }
  cl_device_partition_property properties[] = {
      CL_DEVICE_PARTITION_BY_COUNTS,
      static_cast<cl_device_partition_property>(1),
      static_cast<cl_device_partition_property>(1),
      CL_DEVICE_PARTITION_BY_COUNTS_LIST_END, 0};
  cl_uint numSubDevices = 0;
  EXPECT_SUCCESS(
      clCreateSubDevices(device, properties, 0, nullptr, &numSubDevices));
  ASSERT_GT(numSubDevices, 0u);
  UCL::vector<cl_device_id> subDevices(numSubDevices);
  ASSERT_SUCCESS(clCreateSubDevices(device, properties, numSubDevices,
                                    subDevices.data(), nullptr));
  for (UCL::vector<cl_device_id>::iterator iter = subDevices.begin(),
                                           iter_end = subDevices.end();
       iter != iter_end; ++iter) {
    ASSERT_SUCCESS(clReleaseDevice(*iter));
  }
}
