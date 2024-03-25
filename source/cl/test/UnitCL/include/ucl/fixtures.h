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

#ifndef UNITCL_FIXTURES_H_INCLUDED
#define UNITCL_FIXTURES_H_INCLUDED

#include <CL/cl.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "ucl/callbacks.h"
#include "ucl/checks.h"
#include "ucl/environment.h"

// UnitCL assertions can only be triggered from the outer test scope. This
// macro is intended to be used inside functions, where it prints the error and
// returns.
#define UCL_SUCCESS_OR_RETURN_ERR(ERRCODE)                         \
  if ((ERRCODE) != CL_SUCCESS) {                                   \
    (void)std::fprintf(stderr, "%s: %d: %s\n", __FILE__, __LINE__, \
                       ucl::Error(ERRCODE).description().c_str()); \
    return ERRCODE;                                                \
  }

/// @brief Return if a fatal failure or skip occured invoking an expression.
///
/// Intended for use in test fixture `SetUp()` calls which explicitly call the
/// base class `SetUp()`, if a fatal error or skip occurs in the base class
/// immediately return to avoid crashing the test suite by using uninitialized
/// state.
///
/// @param ... Expression to invoke.
#define UCL_RETURN_ON_FATAL_FAILURE(...)              \
  __VA_ARGS__;                                        \
  if (this->HasFatalFailure() || this->IsSkipped()) { \
    return;                                           \
  }                                                   \
  (void)0

namespace kts {
namespace ucl {
class InputGenerator;
}  // namespace ucl
}  // namespace kts

namespace ucl {
struct BaseTest : testing::Test {
  ucl::Environment *getEnvironment() const {
    return ucl::Environment::instance;
  }

  kts::ucl::InputGenerator &getInputGenerator() const {
    return getEnvironment()->GetInputGenerator();
  }
};

struct PlatformTest : BaseTest {
  cl_platform_id platform = nullptr;

  void SetUp() override { platform = getEnvironment()->GetPlatform(); }

  void TearDown() override { platform = nullptr; }

  std::string getPlatformProfile() const;
  std::string getPlatformVersion() const;
#ifdef CL_VERSION_3_0
  cl_version getPlatformNumericVersion() const;
#endif
  std::string getPlatformName() const;
  std::string getPlatformVendor() const;
  std::string getPlatformExtensions() const;
#ifdef CL_VERSION_3_0
  std::vector<cl_name_version> getPlatformExtensionsWithVersion() const;
#endif
#ifdef CL_VERISON_2_1
  cl_ulong getPlatformHostTimerResolution() const;
#endif

  bool isPlatformExtensionSupported(std::string name) const;
};

struct DeviceTest : PlatformTest {
  cl_device_id device = nullptr;

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(PlatformTest::SetUp());
    device = getEnvironment()->GetDevice();
    ASSERT_SUCCESS(clRetainDevice(device));
  }

  void TearDown() override {
    if (device) {
      EXPECT_SUCCESS(clReleaseDevice(device));
    }
    PlatformTest::TearDown();
  }

  cl_device_type getDeviceType() const;
  cl_uint getDeviceVendorId() const;
  cl_uint getDeviceMaxComputeUnits() const;
  cl_uint getDeviceMaxWorkItemDimensions() const;
  std::vector<size_t> getDeviceMaxWorkItemSizes() const;
  size_t getDeviceMaxWorkGroupSize() const;
  cl_uint getDevicePreferredVectorWidthChar() const;
  cl_uint getDevicePreferredVectorWidthShort() const;
  cl_uint getDevicePreferredVectorWidthInt() const;
  cl_uint getDevicePreferredVectorWidthLong() const;
  cl_uint getDevicePreferredVectorWidthFloat() const;
  cl_uint getDevicePreferredVectorWidthDouble() const;
#ifdef CL_VERSION_1_1
  cl_uint getDevicePreferredVectorWidthHalf() const;
#endif
  cl_uint getDeviceNativeVectorWidthChar() const;
  cl_uint getDeviceNativeVectorWidthShort() const;
  cl_uint getDeviceNativeVectorWidthInt() const;
  cl_uint getDeviceNativeVectorWidthLong() const;
  cl_uint getDeviceNativeVectorWidthFloat() const;
  cl_uint getDeviceNativeVectorWidthDouble() const;
#ifdef CL_VERSION_1_1
  cl_uint getDeviceNativeVectorWidthHalf() const;
#endif
  cl_uint getDeviceMaxClockFrequency() const;
  cl_uint getDeviceAddressBits() const;
  cl_ulong getDeviceMaxMemAllocSize() const;
  cl_bool getDeviceImageSupport() const;
  cl_uint getDeviceMaxReadImageArgs() const;
  cl_uint getDeviceMaxWriteImageArgs() const;
#ifdef CL_VERSION_2_0
  cl_uint getDeviceMaxReadWriteImageArgs() const;
#endif
#ifdef CL_VERSION_2_1
  std::string getDeviceILVersion() const;
#endif
#ifdef CL_VERSION_3_0
  std::vector<cl_name_version> getDeviceILsWithVersion() const;
#endif
  size_t getDeviceImage2dMaxWidth() const;
  size_t getDeviceImage2dMaxHeight() const;
  size_t getDeviceImage3dMaxWidth() const;
  size_t getDeviceImage3dMaxHeight() const;
  size_t getDeviceImage3dMaxDepth() const;
#ifdef CL_VERSION_1_2
  size_t getDeviceImageMaxBufferSize() const;
  size_t getDeviceImageMaxArraySize() const;
#endif
  cl_uint getDeviceMaxSamplers() const;
#ifdef CL_VERSION_2_0
  cl_uint getDeviceImagePitchAlignment() const;
  cl_uint getDeviceImageBaseAddressAlignment() const;
  cl_uint getDeviceMaxPipeArgs() const;
  cl_uint getDevicePipeMaxActiveReservations() const;
  cl_uint getDevicePipeMaxPacketSize() const;
#endif
  size_t getDeviceMaxParameterSize() const;
  cl_uint getDeviceMemBaseAddrAlign() const;
  cl_uint getDeviceMinDataTypeAlignSize() const;
  cl_device_fp_config getDeviceSingleFpConfig() const;
#ifdef CL_VERSION_1_2
  cl_device_fp_config getDeviceDoubleFpConfig() const;
#endif
  cl_device_mem_cache_type getDeviceGlobalMemCacheType() const;
  cl_uint getDeviceGlobalMemCachelineSize() const;
  cl_ulong getDeviceGlobalMemCacheSize() const;
  cl_ulong getDeviceGlobalMemSize() const;
  cl_ulong getDeviceMaxConstantBufferSize() const;
  cl_uint getDeviceMaxConstantArgs() const;
#ifdef CL_VERSION_2_0
  size_t getDeviceMaxGlobalVariableSize() const;
  size_t getDeviceGlobalVariablePreferredTotalSize() const;
#endif
  cl_device_local_mem_type getDeviceLocalMemType() const;
  cl_ulong getDeviceLocalMemSize() const;
  cl_bool getDeviceErrorCorrectionSupport() const;
#ifdef CL_VERSION_1_1
  cl_bool getDeviceHostUnifiedMemory() const;
#endif
  size_t getDeviceProfilingTimerResolution() const;
  cl_bool getDeviceEndianLittle() const;
  cl_bool getDeviceAvailable() const;
  cl_bool getDeviceCompilerAvailable() const;
#ifdef CL_VERSION_1_2
  cl_bool getDeviceLinkerAvailable() const;
#endif
  cl_device_exec_capabilities getDeviceExecutionCapabilities() const;
  cl_command_queue_properties getDeviceQueueProperties() const;
#ifdef CL_VERSION_2_0
  cl_command_queue_properties getDeviceQueueOnHostProperties() const;
  cl_command_queue_properties getDeviceQueueOnDeviceProperties() const;
  cl_uint getDeviceQueueOnDevicePreferredSize() const;
  cl_uint getDeviceQueueOnDeviceMaxSize() const;
  cl_uint getDeviceMaxOnDeviceQueues() const;
  cl_uint getDeviceMaxOnDeviceEvents() const;
#endif
#ifdef CL_VERSION_1_2
  std::string getDeviceBuiltInKernels() const;
#endif
#ifdef CL_VERSION_3_0
  std::vector<cl_name_version> getDeviceBuiltInKernelsWithVersion() const;
#endif
  cl_platform_id getDevicePlatform() const;
  std::string getDeviceName() const;
  std::string getDeviceVendor() const;
  std::string getDeviceProfile() const;
  std::string getDeviceVersion() const;
#ifdef CL_VERSION_3_0
  cl_version getDeviceNumericVersion() const;
#endif
#ifdef CL_VERSION_1_1
  std::string getDeviceOpenclCVersion() const;
#endif
#ifdef CL_VERSION_3_0
  std::vector<cl_name_version> getDeviceOpenclCAllVersions() const;
  std::vector<cl_name_version> getDeviceOpenclCFeatures() const;
#endif
  std::string getDeviceExtensions() const;
#ifdef CL_VERSION_3_0
  std::vector<cl_name_version> getDeviceExtensionsWithVersion() const;
#endif
#ifdef CL_VERSION_1_2
  size_t getDevicePrintfBufferSize() const;
  cl_bool getDevicePreferredInteropUserSync() const;
  cl_device_id getDeviceParentDevice() const;
  cl_uint getDevicePartitionMaxSubDevices() const;
  std::vector<cl_device_partition_property> getDevicePartitionProperties()
      const;
  cl_device_affinity_domain getDevicePartitionAffinityDomain() const;
  std::vector<cl_device_partition_property> getDevicePartitionType() const;
  cl_uint getDeviceReferenceCount() const;
#endif
#ifdef CL_VERSION_2_0
  cl_device_svm_capabilities getDeviceSvmCapabilities() const;
  cl_uint getDevicePreferredPlatformAtomicAlignment() const;
  cl_uint getDevicePreferredGlobalAtomicAlignment() const;
  cl_uint getDevicePreferredLocalAtomicAlignment() const;
#endif
#ifdef CL_VERISON_2_1
  cl_uint getDeviceMaxNumSubGroups() const;
  cl_bool getDeviceSubGroupIndependentForwardProgress() const;
#endif
#ifdef CL_VERSION_3_0
  cl_device_atomic_capabilities getDeviceAtomicMemoryCapabilities() const;
  cl_device_atomic_capabilities getDeviceAtomicFenceCapabilities() const;
  cl_bool getDeviceNonUniformWorkGroupSupport() const;
  cl_bool getDeviceWorkGroupCollectiveFunctionsSupport() const;
  cl_bool getDeviceGenericAddressSpaceSupport() const;
  cl_device_device_enqueue_capabilities getDeviceDeviceEnqueueCapabilities()
      const;
  cl_bool getDevicePipeSupport() const;
  size_t getDevicePreferredWorkGroupSizeMultiple() const;
  std::string getDeviceLatestConformanceVersionPassed() const;
#endif

  bool isDeviceExtensionSupported(std::string name) const;

  std::string getOpenCLCSourceFromFile(std::string filename) const;
  std::vector<uint8_t> getDeviceBinaryFromFile(std::string filename) const;
  std::vector<uint32_t> getDeviceSpirvFromFile(std::string filename) const;
};

struct ContextTest : DeviceTest {
  cl_context context = nullptr;

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    context = getEnvironment()->contexts[device];
  }
};

struct CommandQueueTest : virtual ContextTest {
  cl_command_queue command_queue = nullptr;

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    command_queue = getEnvironment()->command_queues[context];
  }
};
}  // namespace ucl

#endif  // UNITCL_FIXTURES_H_INCLUDED
