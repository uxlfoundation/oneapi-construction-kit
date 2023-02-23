// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
