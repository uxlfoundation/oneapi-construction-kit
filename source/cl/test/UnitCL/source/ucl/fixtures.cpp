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

#include "ucl/fixtures.h"

#include "cargo/string_algorithm.h"
#include "ucl/file.h"

namespace {
std::string getPlatformInfo(cl_platform_id platform, cl_platform_info info) {
  size_t size;
  if (auto error = clGetPlatformInfo(platform, info, 0, nullptr, &size)) {
    UCL_ABORT("clGetPlatformInfo failed: %d", error);
  }
  std::string value(size, '\0');
  if (auto error =
          clGetPlatformInfo(platform, info, size, value.data(), nullptr)) {
    UCL_ABORT("clGetPlatformInfo failed: %d", error);
  }
  return value;
}
}  // namespace

std::string ucl::PlatformTest::getPlatformProfile() const {
  return getPlatformInfo(platform, CL_PLATFORM_PROFILE);
}

std::string ucl::PlatformTest::getPlatformVersion() const {
  return getPlatformInfo(platform, CL_PLATFORM_VERSION);
}

#if CL_VERSION_3_0
cl_version ucl::PlatformTest::getPlatformNumericVersion() const {
  cl_version version;
  if (auto error = clGetPlatformInfo(platform, CL_PLATFORM_NUMERIC_VERSION,
                                     sizeof(version), &version, nullptr)) {
    UCL_ABORT("clGetPlatformInfo failed: %d", error);
  }
  return version;
}
#endif

std::string ucl::PlatformTest::getPlatformName() const {
  return getPlatformInfo(platform, CL_PLATFORM_NAME);
}

std::string ucl::PlatformTest::getPlatformExtensions() const {
  return getPlatformInfo(platform, CL_PLATFORM_EXTENSIONS);
}

bool ucl::PlatformTest::isPlatformExtensionSupported(std::string name) const {
  return getPlatformExtensions().find(name) != std::string::npos;
}

#if CL_VERSION_3_0
std::vector<cl_name_version>
ucl::PlatformTest::getPlatformExtensionsWithVersion() const {
  size_t size;
  if (auto error = clGetPlatformInfo(
          platform, CL_PLATFORM_EXTENSIONS_WITH_VERSION, 0, nullptr, &size)) {
    UCL_ABORT("clGetPlatformInfo failed: %d", error);
  }
  std::vector<cl_name_version> extensions_with_version(size /
                                                       sizeof(cl_name_version));
  if (auto error =
          clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS_WITH_VERSION, size,
                            extensions_with_version.data(), nullptr)) {
    UCL_ABORT("clGetPlatformInfo failed: %d", error);
  }
  return extensions_with_version;
}
#endif

#if CL_VERISON_2_1
cl_ulong ucl::PlatformTest::getPlatformHostTimerResolution() const {
  cl_ulong host_timer_resolution;
  if (auto error =
          clGetPlatformInfo(platform, info, sizeof(host_timer_resolution),
                            &host_timer_resolution, nullptr)) {
    UCL_ABORT("clGetPlatformInfo failed: %d", error);
  }
  return value;
}
#endif

namespace {
template <class T>
T getDeviceInfo(cl_device_id device, cl_device_info info) {
  T value;
  if (auto error = clGetDeviceInfo(device, info, sizeof(value),
                                   static_cast<void *>(&value), nullptr)) {
    UCL_ABORT("clGetDeviceInfo failed: %d", error);
  }
  return value;
}

template <>
std::string getDeviceInfo<std::string>(cl_device_id device,
                                       cl_device_info info) {
  size_t size;
  if (auto error = clGetDeviceInfo(device, info, 0, nullptr, &size)) {
    UCL_ABORT("clGetDeviceInfo failed: %d", error);
  }
  std::string value(size - 1, '\0');  // account for null terminator
  if (auto error = clGetDeviceInfo(device, info, size, value.data(), nullptr)) {
    UCL_ABORT("clGetDeviceInfo failed: %d", error);
  }
  return value;
}

#ifdef CL_VERSION_3_0
template <>
std::vector<cl_name_version> getDeviceInfo<std::vector<cl_name_version>>(
    cl_device_id device, cl_device_info info) {
  size_t size;
  if (auto error = clGetDeviceInfo(device, info, 0, nullptr, &size)) {
    UCL_ABORT("clGetDeviceInfo failed: %d", error);
  }
  std::vector<cl_name_version> versions(size / sizeof(cl_name_version));
  if (auto error =
          clGetDeviceInfo(device, info, size, versions.data(), nullptr)) {
    UCL_ABORT("clGetDeviceInfo failed: %d", error);
  }
  return versions;
}
#endif
}  // namespace

cl_device_type ucl::DeviceTest::getDeviceType() const {
  return getDeviceInfo<cl_device_type>(device, CL_DEVICE_TYPE);
}

cl_uint ucl::DeviceTest::getDeviceVendorId() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_VENDOR_ID);
}

cl_uint ucl::DeviceTest::getDeviceMaxComputeUnits() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_MAX_COMPUTE_UNITS);
}

cl_uint ucl::DeviceTest::getDeviceMaxWorkItemDimensions() const {
  return getDeviceInfo<size_t>(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS);
}

std::vector<size_t> ucl::DeviceTest::getDeviceMaxWorkItemSizes() const {
  size_t size;
  if (auto error = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, 0,
                                   nullptr, &size)) {
    UCL_ABORT("clGetDeviceInfo failed: %d", error);
  }
  std::vector<size_t> max_work_item_sizes(size);
  if (auto error = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES,
                                   sizeof(size_t) * max_work_item_sizes.size(),
                                   max_work_item_sizes.data(), nullptr)) {
    UCL_ABORT("clGetDeviceInfo failed: %d", error);
  }
  return max_work_item_sizes;
}

size_t ucl::DeviceTest::getDeviceMaxWorkGroupSize() const {
  return getDeviceInfo<size_t>(device, CL_DEVICE_MAX_WORK_GROUP_SIZE);
}

cl_uint ucl::DeviceTest::getDevicePreferredVectorWidthChar() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR);
}

cl_uint ucl::DeviceTest::getDevicePreferredVectorWidthShort() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT);
}

cl_uint ucl::DeviceTest::getDevicePreferredVectorWidthInt() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT);
}

cl_uint ucl::DeviceTest::getDevicePreferredVectorWidthLong() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG);
}

cl_uint ucl::DeviceTest::getDevicePreferredVectorWidthFloat() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT);
}

cl_uint ucl::DeviceTest::getDevicePreferredVectorWidthDouble() const {
  return getDeviceInfo<cl_uint>(device,
                                CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE);
}

#ifdef CL_VERSION_1_1
cl_uint ucl::DeviceTest::getDevicePreferredVectorWidthHalf() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF);
}
#endif

cl_uint ucl::DeviceTest::getDeviceNativeVectorWidthChar() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR);
}

cl_uint ucl::DeviceTest::getDeviceNativeVectorWidthShort() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT);
}

cl_uint ucl::DeviceTest::getDeviceNativeVectorWidthInt() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_INT);
}

cl_uint ucl::DeviceTest::getDeviceNativeVectorWidthLong() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG);
}

cl_uint ucl::DeviceTest::getDeviceNativeVectorWidthFloat() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT);
}

cl_uint ucl::DeviceTest::getDeviceNativeVectorWidthDouble() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE);
}

#ifdef CL_VERSION_1_1
cl_uint ucl::DeviceTest::getDeviceNativeVectorWidthHalf() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF);
}
#endif

cl_uint ucl::DeviceTest::getDeviceMaxClockFrequency() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_MAX_CLOCK_FREQUENCY);
}

cl_uint ucl::DeviceTest::getDeviceAddressBits() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_ADDRESS_BITS);
}

cl_ulong ucl::DeviceTest::getDeviceMaxMemAllocSize() const {
  return getDeviceInfo<cl_ulong>(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE);
}

cl_bool ucl::DeviceTest::getDeviceImageSupport() const {
  return getDeviceInfo<cl_bool>(device, CL_DEVICE_IMAGE_SUPPORT);
}

cl_uint ucl::DeviceTest::getDeviceMaxReadImageArgs() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_MAX_READ_IMAGE_ARGS);
}

cl_uint ucl::DeviceTest::getDeviceMaxWriteImageArgs() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_MAX_WRITE_IMAGE_ARGS);
}

#ifdef CL_VERSION_2_0
cl_uint ucl::DeviceTest::getDeviceMaxReadWriteImageArgs() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS);
}
#endif

#ifdef CL_VERSION_2_1
std::string ucl::DeviceTest::getDeviceILVersion() const {
  return getDeviceInfo<std::string>(device, CL_DEVICE_IL_VERSION);
}
#endif

#ifdef CL_VERSION_3_0
std::vector<cl_name_version> ucl::DeviceTest::getDeviceILsWithVersion() const {
  return getDeviceInfo<std::vector<cl_name_version>>(
      device, CL_DEVICE_ILS_WITH_VERSION);
}
#endif

size_t ucl::DeviceTest::getDeviceImage2dMaxWidth() const {
  return getDeviceInfo<size_t>(device, CL_DEVICE_IMAGE2D_MAX_WIDTH);
}

size_t ucl::DeviceTest::getDeviceImage2dMaxHeight() const {
  return getDeviceInfo<size_t>(device, CL_DEVICE_IMAGE2D_MAX_HEIGHT);
}

size_t ucl::DeviceTest::getDeviceImage3dMaxWidth() const {
  return getDeviceInfo<size_t>(device, CL_DEVICE_IMAGE3D_MAX_WIDTH);
}

size_t ucl::DeviceTest::getDeviceImage3dMaxHeight() const {
  return getDeviceInfo<size_t>(device, CL_DEVICE_IMAGE3D_MAX_HEIGHT);
}

size_t ucl::DeviceTest::getDeviceImage3dMaxDepth() const {
  return getDeviceInfo<size_t>(device, CL_DEVICE_IMAGE3D_MAX_DEPTH);
}

#ifdef CL_VERSION_1_2
size_t ucl::DeviceTest::getDeviceImageMaxBufferSize() const {
  return getDeviceInfo<size_t>(device, CL_DEVICE_IMAGE_MAX_BUFFER_SIZE);
}

size_t ucl::DeviceTest::getDeviceImageMaxArraySize() const {
  return getDeviceInfo<size_t>(device, CL_DEVICE_IMAGE_MAX_ARRAY_SIZE);
}
#endif

cl_uint ucl::DeviceTest::getDeviceMaxSamplers() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_MAX_SAMPLERS);
}

#ifdef CL_VERSION_2_0
cl_uint ucl::DeviceTest::getDeviceImagePitchAlignment() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_IMAGE_PITCH_ALIGNMENT);
}

cl_uint ucl::DeviceTest::getDeviceImageBaseAddressAlignment() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT);
}

cl_uint ucl::DeviceTest::getDeviceMaxPipeArgs() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_MAX_PIPE_ARGS);
}

cl_uint ucl::DeviceTest::getDevicePipeMaxActiveReservations() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS);
}

cl_uint ucl::DeviceTest::getDevicePipeMaxPacketSize() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_PIPE_MAX_PACKET_SIZE);
}
#endif

size_t ucl::DeviceTest::getDeviceMaxParameterSize() const {
  return getDeviceInfo<size_t>(device, CL_DEVICE_MAX_PARAMETER_SIZE);
}

cl_uint ucl::DeviceTest::getDeviceMemBaseAddrAlign() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_MEM_BASE_ADDR_ALIGN);
}

cl_uint ucl::DeviceTest::getDeviceMinDataTypeAlignSize() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE);
}

cl_device_fp_config ucl::DeviceTest::getDeviceSingleFpConfig() const {
  return getDeviceInfo<cl_device_fp_config>(device, CL_DEVICE_SINGLE_FP_CONFIG);
}

#ifdef CL_VERSION_1_2
cl_device_fp_config ucl::DeviceTest::getDeviceDoubleFpConfig() const {
  return getDeviceInfo<cl_device_fp_config>(device, CL_DEVICE_DOUBLE_FP_CONFIG);
}
#endif

cl_device_mem_cache_type ucl::DeviceTest::getDeviceGlobalMemCacheType() const {
  return getDeviceInfo<cl_device_mem_cache_type>(
      device, CL_DEVICE_GLOBAL_MEM_CACHE_TYPE);
}

cl_uint ucl::DeviceTest::getDeviceGlobalMemCachelineSize() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE);
}

cl_ulong ucl::DeviceTest::getDeviceGlobalMemCacheSize() const {
  return getDeviceInfo<cl_ulong>(device, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE);
}

cl_ulong ucl::DeviceTest::getDeviceGlobalMemSize() const {
  return getDeviceInfo<cl_ulong>(device, CL_DEVICE_GLOBAL_MEM_SIZE);
}

cl_ulong ucl::DeviceTest::getDeviceMaxConstantBufferSize() const {
  return getDeviceInfo<cl_ulong>(device, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE);
}

cl_uint ucl::DeviceTest::getDeviceMaxConstantArgs() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_MAX_CONSTANT_ARGS);
}

#ifdef CL_VERSION_2_0
size_t ucl::DeviceTest::getDeviceMaxGlobalVariableSize() const {
  return getDeviceInfo<size_t>(device, CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE);
}

size_t ucl::DeviceTest::getDeviceGlobalVariablePreferredTotalSize() const {
  return getDeviceInfo<size_t>(device,
                               CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE);
}
#endif

cl_device_local_mem_type ucl::DeviceTest::getDeviceLocalMemType() const {
  return getDeviceInfo<cl_device_local_mem_type>(device,
                                                 CL_DEVICE_LOCAL_MEM_TYPE);
}

cl_ulong ucl::DeviceTest::getDeviceLocalMemSize() const {
  return getDeviceInfo<cl_ulong>(device, CL_DEVICE_LOCAL_MEM_SIZE);
}

cl_bool ucl::DeviceTest::getDeviceErrorCorrectionSupport() const {
  return getDeviceInfo<cl_bool>(device, CL_DEVICE_ERROR_CORRECTION_SUPPORT);
}

#ifdef CL_VERSION_1_1
cl_bool ucl::DeviceTest::getDeviceHostUnifiedMemory() const {
  return getDeviceInfo<cl_bool>(device, CL_DEVICE_HOST_UNIFIED_MEMORY);
}
#endif

size_t ucl::DeviceTest::getDeviceProfilingTimerResolution() const {
  return getDeviceInfo<size_t>(device, CL_DEVICE_PROFILING_TIMER_RESOLUTION);
}

cl_bool ucl::DeviceTest::getDeviceEndianLittle() const {
  return getDeviceInfo<cl_bool>(device, CL_DEVICE_ENDIAN_LITTLE);
}

cl_bool ucl::DeviceTest::getDeviceAvailable() const {
  return getDeviceInfo<cl_bool>(device, CL_DEVICE_AVAILABLE);
}

cl_bool ucl::DeviceTest::getDeviceCompilerAvailable() const {
  return getDeviceInfo<cl_bool>(device, CL_DEVICE_COMPILER_AVAILABLE);
}

#ifdef CL_VERSION_1_2
cl_bool ucl::DeviceTest::getDeviceLinkerAvailable() const {
  return getDeviceInfo<cl_bool>(device, CL_DEVICE_LINKER_AVAILABLE);
}
#endif

cl_device_exec_capabilities ucl::DeviceTest::getDeviceExecutionCapabilities()
    const {
  return getDeviceInfo<cl_device_exec_capabilities>(
      device, CL_DEVICE_EXECUTION_CAPABILITIES);
}

cl_command_queue_properties ucl::DeviceTest::getDeviceQueueProperties() const {
  return getDeviceInfo<cl_command_queue_properties>(device,
                                                    CL_DEVICE_QUEUE_PROPERTIES);
}

#ifdef CL_VERSION_2_0
cl_command_queue_properties ucl::DeviceTest::getDeviceQueueOnHostProperties()
    const {
  return getDeviceInfo<cl_command_queue_properties>(
      device, CL_DEVICE_QUEUE_ON_HOST_PROPERTIES);
}

cl_command_queue_properties ucl::DeviceTest::getDeviceQueueOnDeviceProperties()
    const {
  return getDeviceInfo<cl_command_queue_properties>(
      device, CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES);
}

cl_uint ucl::DeviceTest::getDeviceQueueOnDevicePreferredSize() const {
  return getDeviceInfo<cl_uint>(device,
                                CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE);
}

cl_uint ucl::DeviceTest::getDeviceQueueOnDeviceMaxSize() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE);
}

cl_uint ucl::DeviceTest::getDeviceMaxOnDeviceQueues() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_MAX_ON_DEVICE_QUEUES);
}

cl_uint ucl::DeviceTest::getDeviceMaxOnDeviceEvents() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_MAX_ON_DEVICE_EVENTS);
}
#endif

#ifdef CL_VERSION_1_2
std::string ucl::DeviceTest::getDeviceBuiltInKernels() const {
  return getDeviceInfo<std::string>(device, CL_DEVICE_BUILT_IN_KERNELS);
}
#endif

#ifdef CL_VERSION_3_0
std::vector<cl_name_version>
ucl::DeviceTest::getDeviceBuiltInKernelsWithVersion() const {
  return getDeviceInfo<std::vector<cl_name_version>>(
      device, CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION);
}
#endif

cl_platform_id ucl::DeviceTest::getDevicePlatform() const {
  return getDeviceInfo<cl_platform_id>(device, CL_DEVICE_PLATFORM);
}

std::string ucl::DeviceTest::getDeviceName() const {
  return getDeviceInfo<std::string>(device, CL_DEVICE_NAME);
}

std::string ucl::DeviceTest::getDeviceVendor() const {
  return getDeviceInfo<std::string>(device, CL_DEVICE_VENDOR);
}

std::string ucl::DeviceTest::getDeviceProfile() const {
  return getDeviceInfo<std::string>(device, CL_DEVICE_PROFILE);
}

std::string ucl::DeviceTest::getDeviceVersion() const {
  return getDeviceInfo<std::string>(device, CL_DEVICE_VERSION);
}

#ifdef CL_VERSION_3_0
cl_version ucl::DeviceTest::getDeviceNumericVersion() const {
  return getDeviceInfo<cl_version>(device, CL_DEVICE_NUMERIC_VERSION);
}
#endif

#ifdef CL_VERSION_1_1
std::string ucl::DeviceTest::getDeviceOpenclCVersion() const {
  return getDeviceInfo<std::string>(device, CL_DEVICE_OPENCL_C_VERSION);
}
#endif

#ifdef CL_VERSION_3_0
std::vector<cl_name_version> ucl::DeviceTest::getDeviceOpenclCAllVersions()
    const {
  return getDeviceInfo<std::vector<cl_name_version>>(
      device, CL_DEVICE_OPENCL_C_ALL_VERSIONS);
}

std::vector<cl_name_version> ucl::DeviceTest::getDeviceOpenclCFeatures() const {
  return getDeviceInfo<std::vector<cl_name_version>>(
      device, CL_DEVICE_OPENCL_C_FEATURES);
}
#endif

std::string ucl::DeviceTest::getDeviceExtensions() const {
  return getDeviceInfo<std::string>(device, CL_DEVICE_EXTENSIONS);
}

#ifdef CL_VERSION_3_0
std::vector<cl_name_version> ucl::DeviceTest::getDeviceExtensionsWithVersion()
    const {
  return getDeviceInfo<std::vector<cl_name_version>>(
      device, CL_DEVICE_EXTENSIONS_WITH_VERSION);
}
#endif

#ifdef CL_VERSION_1_2
size_t ucl::DeviceTest::getDevicePrintfBufferSize() const {
  return getDeviceInfo<size_t>(device, CL_DEVICE_PRINTF_BUFFER_SIZE);
}

cl_bool ucl::DeviceTest::getDevicePreferredInteropUserSync() const {
  return getDeviceInfo<cl_bool>(device, CL_DEVICE_PREFERRED_INTEROP_USER_SYNC);
}

cl_device_id ucl::DeviceTest::getDeviceParentDevice() const {
  return getDeviceInfo<cl_device_id>(device, CL_DEVICE_PARENT_DEVICE);
}

cl_uint ucl::DeviceTest::getDevicePartitionMaxSubDevices() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_PARTITION_MAX_SUB_DEVICES);
}

std::vector<cl_device_partition_property>
ucl::DeviceTest::getDevicePartitionProperties() const {
  size_t size;
  if (auto error = clGetDeviceInfo(device, CL_DEVICE_PARTITION_PROPERTIES, 0,
                                   nullptr, &size)) {
    UCL_ABORT("clGetDeviceInfo failed: %d", error);
  }
  std::vector<cl_device_partition_property> partition_properties(
      size / sizeof(cl_device_partition_property));
  if (auto error = clGetDeviceInfo(device, CL_DEVICE_PARTITION_PROPERTIES, size,
                                   partition_properties.data(), nullptr)) {
    UCL_ABORT("clGetDeviceInfo failed: %d", error);
  }
  return partition_properties;
}

cl_device_affinity_domain ucl::DeviceTest::getDevicePartitionAffinityDomain()
    const {
  return getDeviceInfo<cl_device_affinity_domain>(
      device, CL_DEVICE_PARTITION_AFFINITY_DOMAIN);
}

std::vector<cl_device_partition_property>
ucl::DeviceTest::getDevicePartitionType() const {
  size_t size;
  if (auto error = clGetDeviceInfo(device, CL_DEVICE_PARTITION_TYPE, 0, nullptr,
                                   &size)) {
    UCL_ABORT("clGetDeviceInfo failed: %d", error);
  }
  std::vector<cl_device_partition_property> partition_type(
      size / sizeof(cl_device_partition_property));
  if (auto error = clGetDeviceInfo(device, CL_DEVICE_PARTITION_TYPE, size,
                                   partition_type.data(), nullptr)) {
    UCL_ABORT("clGetDeviceInfo failed: %d", error);
  }
  return partition_type;
}

cl_uint ucl::DeviceTest::getDeviceReferenceCount() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_REFERENCE_COUNT);
}
#endif

#ifdef CL_VERSION_2_0
cl_device_svm_capabilities ucl::DeviceTest::getDeviceSvmCapabilities() const {
  return getDeviceInfo<cl_device_svm_capabilities>(device,
                                                   CL_DEVICE_SVM_CAPABILITIES);
}

cl_uint ucl::DeviceTest::getDevicePreferredPlatformAtomicAlignment() const {
  return getDeviceInfo<cl_uint>(device,
                                CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT);
}

cl_uint ucl::DeviceTest::getDevicePreferredGlobalAtomicAlignment() const {
  return getDeviceInfo<cl_uint>(device,
                                CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT);
}

cl_uint ucl::DeviceTest::getDevicePreferredLocalAtomicAlignment() const {
  return getDeviceInfo<cl_uint>(device,
                                CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT);
}
#endif

#ifdef CL_VERISON_2_1
cl_uint ucl::DeviceTest::getDeviceMaxNumSubGroups() const {
  return getDeviceInfo<cl_uint>(device, CL_DEVICE_MAX_NUM_SUB_GROUPS);
}

cl_bool ucl::DeviceTest::getDeviceSubGroupIndependentForwardProgress() const {
  return getDeviceInfo<cl_bool>(
      device, CL_DEVICE_SUB_GROUP_INDEPENDENT_FORWARD_PROGRESS);
}
#endif

#ifdef CL_VERSION_3_0
cl_device_atomic_capabilities
ucl::DeviceTest::getDeviceAtomicMemoryCapabilities() const {
  return getDeviceInfo<cl_device_atomic_capabilities>(
      device, CL_DEVICE_ATOMIC_MEMORY_CAPABILITIES);
}

cl_device_atomic_capabilities
ucl::DeviceTest::getDeviceAtomicFenceCapabilities() const {
  return getDeviceInfo<cl_device_atomic_capabilities>(
      device, CL_DEVICE_ATOMIC_FENCE_CAPABILITIES);
}

cl_bool ucl::DeviceTest::getDeviceNonUniformWorkGroupSupport() const {
  return getDeviceInfo<cl_bool>(device,
                                CL_DEVICE_NON_UNIFORM_WORK_GROUP_SUPPORT);
}

cl_bool ucl::DeviceTest::getDeviceWorkGroupCollectiveFunctionsSupport() const {
  return getDeviceInfo<cl_bool>(
      device, CL_DEVICE_WORK_GROUP_COLLECTIVE_FUNCTIONS_SUPPORT);
}

cl_bool ucl::DeviceTest::getDeviceGenericAddressSpaceSupport() const {
  return getDeviceInfo<cl_bool>(device,
                                CL_DEVICE_GENERIC_ADDRESS_SPACE_SUPPORT);
}

cl_device_device_enqueue_capabilities
ucl::DeviceTest::getDeviceDeviceEnqueueCapabilities() const {
  return getDeviceInfo<cl_device_device_enqueue_capabilities>(
      device, CL_DEVICE_DEVICE_ENQUEUE_CAPABILITIES);
}

cl_bool ucl::DeviceTest::getDevicePipeSupport() const {
  return getDeviceInfo<cl_bool>(device, CL_DEVICE_PIPE_SUPPORT);
}

size_t ucl::DeviceTest::getDevicePreferredWorkGroupSizeMultiple() const {
  return getDeviceInfo<size_t>(device,
                               CL_DEVICE_PREFERRED_WORK_GROUP_SIZE_MULTIPLE);
}

std::string ucl::DeviceTest::getDeviceLatestConformanceVersionPassed() const {
  return getDeviceInfo<std::string>(
      device, CL_DEVICE_LATEST_CONFORMANCE_VERSION_PASSED);
}
#endif

bool ucl::DeviceTest::isDeviceExtensionSupported(std::string name) const {
  return getDeviceExtensions().find(name) != std::string::npos;
}

std::string ucl::DeviceTest::getOpenCLCSourceFromFile(std::string name) const {
  name = ucl::Environment::instance->GetKernelDirectory() + "/" + name + ".cl";
  return File{name}.read<std::string>();
}

std::vector<uint8_t> ucl::DeviceTest::getDeviceBinaryFromFile(
    std::string name) const {
  name = ucl::Environment::instance->GetKernelDirectory() + "_offline/" +
         getDeviceName() + "/" + name + ".bin";
  return File{name}.read<std::vector<uint8_t>>();
}

std::vector<uint32_t> ucl::DeviceTest::getDeviceSpirvFromFile(
    std::string name) const {
  name = ucl::Environment::instance->GetKernelDirectory() + "/" + name;
  switch (getDeviceAddressBits()) {
    case 32:
      name += ".spv32";
      break;
    case 64:
      name += ".spv64";
      break;
    default:
      UCL_ABORT("Must have either 32 or 64 bits, have %ld",
                (long)getDeviceAddressBits());
  }
  return File{name}.read<std::vector<uint32_t>>();
}
