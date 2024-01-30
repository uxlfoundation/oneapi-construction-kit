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

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
// This has to come before Common.h due to MinGW-w64 header limitations:
// * https://sourceforge.net/p/mingw-w64/wiki2/Missing%20_aligned_malloc/
#include <malloc.h>
#endif

#include <CL/cl_ext.h>
#include <cargo/string_algorithm.h>
#include <cargo/utility.h>

#include <cstring>
#include <limits>
#include <mutex>
#include <regex>
#include <utility>

#include "Common.h"
#ifdef __ANDROID__
#include <malloc.h>
#endif

#define PRINT_PLATFORM_INFO(platform, param_name) \
  print_platform_info(platform, param_name, #param_name)

namespace {
void print_platform_info(cl_platform_id platform,
                         const cl_platform_info param_name,
                         const char *param_name_str) {
  std::size_t param_value_size_ret = 0;
  cl_int errcode = clGetPlatformInfo(platform, param_name, 0, nullptr,
                                     &param_value_size_ret);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetPlatformInfo failed");

  std::vector<char> param_value(param_value_size_ret, '\0');
  errcode = clGetPlatformInfo(platform, param_name, param_value_size_ret,
                              &param_value[0], nullptr);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetPlatformInfo failed");

  fprintf(stdout, "%s : %s\n", param_name_str, &param_value[0]);
}

#define PRINT_DEVICE_INFO(device, param_name, print_fn) \
  print_device_info_##print_fn(device, param_name, #param_name)

void print_device_info_cl_device_type(cl_device_id device,
                                      const cl_device_info param_name,
                                      const char *param_name_str) {
  cl_device_type param_value = 0;
  const cl_int errcode = clGetDeviceInfo(
      device, param_name, sizeof(param_value), &param_value, nullptr);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetDeviceInfo failed");

  fprintf(stdout, "%s : ", param_name_str);

  bool matched = false;

#define PRINT_MATCH(pname)     \
  if ((pname) & param_value) { \
    if (matched) {             \
      fprintf(stdout, " | ");  \
    }                          \
    matched = true;            \
    fprintf(stdout, #pname);   \
  }

  PRINT_MATCH(CL_DEVICE_TYPE_CPU);
  PRINT_MATCH(CL_DEVICE_TYPE_GPU);
  PRINT_MATCH(CL_DEVICE_TYPE_ACCELERATOR);
  PRINT_MATCH(CL_DEVICE_TYPE_DEFAULT);
  PRINT_MATCH(CL_DEVICE_TYPE_CUSTOM);
#undef PRINT_MATCH

  if (!matched) {
    fprintf(stdout, "UNKNOWN");
  }

  fprintf(stdout, "\n");
}

void print_device_info_cl_uint(cl_device_id device,
                               const cl_device_info param_name,
                               const char *param_name_str) {
  cl_uint param_value = 0;
  const cl_int errcode = clGetDeviceInfo(
      device, param_name, sizeof(param_value), &param_value, nullptr);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetDeviceInfo failed");

  fprintf(stdout, "%s : %llu\n", param_name_str,
          (unsigned long long)param_value);
}

void print_device_info_size_t_array(cl_device_id device,
                                    const cl_device_info param_name,
                                    const char *param_name_str) {
  size_t param_value_size_ret = 0;
  cl_int errcode =
      clGetDeviceInfo(device, param_name, 0, nullptr, &param_value_size_ret);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetDeviceInfo failed");

  const std::size_t elem_count = param_value_size_ret / sizeof(std::size_t);
  std::vector<std::size_t> param_value(elem_count, 0);
  errcode = clGetDeviceInfo(device, param_name, param_value_size_ret,
                            &param_value[0], nullptr);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetDeviceInfo failed");

  fprintf(stdout, "%s : {", param_name_str);

  for (std::size_t i = 0; i < elem_count; ++i) {
    if (0u != i) {
      fprintf(stdout, ", ");
    }
    fprintf(stdout, "%llu", (unsigned long long)param_value[i]);
  }

  fprintf(stdout, "}\n");
}

void print_device_info_size_t(cl_device_id device,
                              const cl_device_info param_name,
                              const char *param_name_str) {
  size_t param_value = 0;
  const cl_int errcode = clGetDeviceInfo(
      device, param_name, sizeof(param_value), &param_value, nullptr);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetDeviceInfo failed");

  fprintf(stdout, "%s : %llu\n", param_name_str,
          (unsigned long long)param_value);
}

void print_device_info_cl_ulong(cl_device_id device,
                                const cl_device_info param_name,
                                const char *param_name_str) {
  cl_ulong param_value = 0;
  const cl_int errcode = clGetDeviceInfo(
      device, param_name, sizeof(param_value), &param_value, nullptr);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetDeviceInfo failed");

  fprintf(stdout, "%s : %llu\n", param_name_str,
          (unsigned long long)param_value);
}

void print_device_info_cl_bool(cl_device_id device,
                               const cl_device_info param_name,
                               const char *param_name_str) {
  cl_bool param_value = 0;
  const cl_int errcode = clGetDeviceInfo(
      device, param_name, sizeof(param_value), &param_value, nullptr);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetDeviceInfo failed");

  fprintf(stdout, "%s : %s\n", param_name_str,
          (param_value ? "CL_TRUE" : "CL_FALSE"));
}

void print_device_info_cl_device_fp_config(cl_device_id device,
                                           const cl_device_info param_name,
                                           const char *param_name_str) {
  cl_device_fp_config param_value = 0;
  const cl_int errcode = clGetDeviceInfo(
      device, param_name, sizeof(param_value), &param_value, nullptr);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetDeviceInfo failed");

  fprintf(stdout, "%s : ", param_name_str);

  bool matched = false;

#define PRINT_MATCH(pname)     \
  if ((pname) & param_value) { \
    if (matched) {             \
      fprintf(stdout, " | ");  \
    }                          \
    matched = true;            \
    fprintf(stdout, #pname);   \
  }

  PRINT_MATCH(CL_FP_DENORM);
  PRINT_MATCH(CL_FP_INF_NAN);
  PRINT_MATCH(CL_FP_ROUND_TO_NEAREST);
  PRINT_MATCH(CL_FP_ROUND_TO_ZERO);
  PRINT_MATCH(CL_FP_ROUND_TO_INF);
  PRINT_MATCH(CL_FP_FMA);
  PRINT_MATCH(CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT);
  PRINT_MATCH(CL_FP_SOFT_FLOAT);
#undef PRINT_MATCH

  if (!matched) {
    fprintf(stdout, "0 - UNSUPPORTED");
  }

  fprintf(stdout, "\n");
}

void print_device_info_cl_device_mem_cache_type(cl_device_id device,
                                                const cl_device_info param_name,
                                                const char *param_name_str) {
  cl_device_mem_cache_type param_value = 0;
  const cl_int errcode = clGetDeviceInfo(
      device, param_name, sizeof(param_value), &param_value, nullptr);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetDeviceInfo failed");

  fprintf(stdout, "%s : ", param_name_str);

  switch (param_value) {
    case CL_NONE: {
      fprintf(stdout, "CL_NONE");
      break;
    }
    case CL_READ_ONLY_CACHE: {
      fprintf(stdout, "CL_READ_ONLY_CACHE");
      break;
    }
    case CL_READ_WRITE_CACHE: {
      fprintf(stdout, "CL_READ_WRITE_CACHE");
      break;
    }
    default: {
      fprintf(stdout, "UNKNOWN");
      break;
    }
  }

  fprintf(stdout, "\n");
}

void print_device_info_cl_device_local_mem_type(cl_device_id device,
                                                const cl_device_info param_name,
                                                const char *param_name_str) {
  cl_device_local_mem_type param_value = 0;
  const cl_int errcode = clGetDeviceInfo(
      device, param_name, sizeof(param_value), &param_value, nullptr);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetDeviceInfo failed");

  fprintf(stdout, "%s : ", param_name_str);

  switch (param_value) {
    case CL_LOCAL: {
      fprintf(stdout, "CL_LOCAL");
      break;
    }
    case CL_GLOBAL: {
      fprintf(stdout, "CL_GLOBAL");
      break;
    }
    case CL_NONE: {
      fprintf(stdout, "CL_NONE");
      break;
    }
    default: {
      fprintf(stdout, "UNKNOWN");
      break;
    }
  }

  fprintf(stdout, "\n");
}

void print_device_info_cl_device_exec_capabilities(
    cl_device_id device, const cl_device_info param_name,
    const char *param_name_str) {
  cl_device_exec_capabilities param_value = 0;
  const cl_int errcode = clGetDeviceInfo(
      device, param_name, sizeof(param_value), &param_value, nullptr);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetDeviceInfo failed");

  fprintf(stdout, "%s : ", param_name_str);

  bool matched = false;

#define PRINT_MATCH(pname)     \
  if ((pname) & param_value) { \
    if (matched) {             \
      fprintf(stdout, " | ");  \
    }                          \
    matched = true;            \
    fprintf(stdout, #pname);   \
  }

  PRINT_MATCH(CL_EXEC_KERNEL);
  PRINT_MATCH(CL_EXEC_NATIVE_KERNEL);
#undef PRINT_MATCH

  if (!matched) {
    fprintf(stdout, "UNKNOWN");
  }

  fprintf(stdout, "\n");
}

void print_device_info_cl_command_queue_properties(
    cl_device_id device, const cl_device_info param_name,
    const char *param_name_str) {
  cl_command_queue_properties param_value = 0;
  const cl_int errcode = clGetDeviceInfo(
      device, param_name, sizeof(param_value), &param_value, nullptr);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetDeviceInfo failed");

  fprintf(stdout, "%s : ", param_name_str);

  bool matched = false;

#define PRINT_MATCH(pname)     \
  if ((pname) & param_value) { \
    if (matched) {             \
      fprintf(stdout, " | ");  \
    }                          \
    matched = true;            \
    fprintf(stdout, #pname);   \
  }

  PRINT_MATCH(CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
  PRINT_MATCH(CL_QUEUE_PROFILING_ENABLE);
#undef PRINT_MATCH

  if (!matched) {
    fprintf(stdout, "UNKNOWN");
  }

  fprintf(stdout, "\n");
}

void print_device_info_char_array(cl_device_id device,
                                  const cl_device_info param_name,
                                  const char *param_name_str) {
  size_t param_value_size_ret = 0;
  cl_int errcode =
      clGetDeviceInfo(device, param_name, 0, nullptr, &param_value_size_ret);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetDeviceInfo failed");

  std::vector<char> param_value(param_value_size_ret, '\0');
  errcode = clGetDeviceInfo(device, param_name, param_value_size_ret,
                            &param_value[0], nullptr);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetDeviceInfo failed");

  fprintf(stdout, "%s : %s\n", param_name_str, &param_value[0]);
}

void print_device_info_cl_device_partition_property_array(
    cl_device_id device, const cl_device_info param_name,
    const char *param_name_str) {
  size_t param_value_size_ret = 0;
  cl_int errcode =
      clGetDeviceInfo(device, param_name, 0, nullptr, &param_value_size_ret);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetDeviceInfo failed");

  const std::size_t elem_count =
      param_value_size_ret / sizeof(cl_device_partition_property);
  std::vector<cl_device_partition_property> param_value(elem_count, 0);
  errcode = clGetDeviceInfo(device, param_name, param_value_size_ret,
                            &param_value[0], nullptr);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetDeviceInfo failed");

  fprintf(stdout, "%s : {", param_name_str);

  for (std::size_t i = 0; i < elem_count; ++i) {
    if (0u != i) {
      fprintf(stdout, ", ");
    }

    const cl_device_partition_property property = param_value[i];
    switch (property) {
      case CL_DEVICE_PARTITION_EQUALLY: {
        fprintf(stdout, "CL_DEVICE_PARTITION_EQUALLY");
        break;
      }
      case CL_DEVICE_PARTITION_BY_COUNTS: {
        fprintf(stdout, "CL_DEVICE_PARTITION_BY_COUNTS");
        break;
      }
      case CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN: {
        fprintf(stdout, "CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN");
        break;
      }
      case 0: {
        fprintf(stdout, "UNSUPPORTED");
        break;
      }
      default: {
        fprintf(stdout, "UNKNOWN");
        break;
      }
    }
  }
  fprintf(stdout, "}\n");
}

void print_device_info_cl_device_affinity_domain(
    cl_device_id device, const cl_device_info param_name,
    const char *param_name_str) {
  cl_device_affinity_domain param_value = 0;
  const cl_int errcode = clGetDeviceInfo(
      device, param_name, sizeof(param_value), &param_value, nullptr);
  UCL_ASSERT(CL_SUCCESS == errcode, "clGetDeviceInfo failed");

  fprintf(stdout, "%s : ", param_name_str);

  bool matched = false;

#define PRINT_MATCH(pname)     \
  if ((pname) & param_value) { \
    if (matched) {             \
      fprintf(stdout, " | ");  \
    }                          \
    matched = true;            \
    fprintf(stdout, #pname);   \
  }

  PRINT_MATCH(CL_DEVICE_AFFINITY_DOMAIN_NUMA);
  PRINT_MATCH(CL_DEVICE_AFFINITY_DOMAIN_L4_CACHE);
  PRINT_MATCH(CL_DEVICE_AFFINITY_DOMAIN_L3_CACHE);
  PRINT_MATCH(CL_DEVICE_AFFINITY_DOMAIN_L2_CACHE);
  PRINT_MATCH(CL_DEVICE_AFFINITY_DOMAIN_L1_CACHE);
  PRINT_MATCH(CL_DEVICE_AFFINITY_DOMAIN_NEXT_PARTITIONABLE);
#undef PRINT_MATCH

  if (!matched) {
    fprintf(stdout, "UNSUPPORTED");
  }

  fprintf(stdout, "\n");
}
}  // namespace

bool UCL::print_opencl_platform_and_device_info(
    const cl_device_type device_type) {
  cl_uint num_platforms = 0;
  cl_int errcode = clGetPlatformIDs(0, NULL, &num_platforms);
  if (CL_SUCCESS != errcode) {
    return false;
  }

  std::vector<cl_platform_id> platforms(num_platforms, nullptr);
  errcode = clGetPlatformIDs(num_platforms, &platforms[0], nullptr);
  if (CL_SUCCESS != errcode) {
    return false;
  }

  for (std::size_t platform_idx = 0; platform_idx < num_platforms;
       ++platform_idx) {
    cl_platform_id platform = platforms[platform_idx];
    if (nullptr == platform) {
      return false;
    }

    PRINT_PLATFORM_INFO(platform, CL_PLATFORM_PROFILE);
    PRINT_PLATFORM_INFO(platform, CL_PLATFORM_VERSION);
    PRINT_PLATFORM_INFO(platform, CL_PLATFORM_NAME);
    PRINT_PLATFORM_INFO(platform, CL_PLATFORM_VENDOR);
    PRINT_PLATFORM_INFO(platform, CL_PLATFORM_EXTENSIONS);

    cl_uint num_devices = 0;
    errcode = clGetDeviceIDs(platform, device_type, 0, nullptr, &num_devices);
    if (CL_SUCCESS != errcode) {
      return false;
    }

    std::vector<cl_device_id> devices(num_devices, nullptr);
    errcode = clGetDeviceIDs(platform, device_type, num_devices, &devices[0],
                             nullptr);
    if (CL_SUCCESS != errcode) {
      return false;
    }

    for (std::size_t device_idx = 0; device_idx < num_devices; ++device_idx) {
      cl_device_id device = devices[device_idx];
      if (nullptr == device) {
        return false;
      }

      PRINT_DEVICE_INFO(device, CL_DEVICE_TYPE, cl_device_type);
      PRINT_DEVICE_INFO(device, CL_DEVICE_VENDOR_ID, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_MAX_COMPUTE_UNITS, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, size_t_array);
      PRINT_DEVICE_INFO(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, size_t);
      PRINT_DEVICE_INFO(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT,
                        cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT,
                        cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE,
                        cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_ADDRESS_BITS, cl_uint);

      PRINT_DEVICE_INFO(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, cl_ulong);

      PRINT_DEVICE_INFO(device, CL_DEVICE_IMAGE_SUPPORT, cl_bool);
      PRINT_DEVICE_INFO(device, CL_DEVICE_MAX_READ_IMAGE_ARGS, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_MAX_WRITE_IMAGE_ARGS, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_IMAGE2D_MAX_WIDTH, size_t);
      PRINT_DEVICE_INFO(device, CL_DEVICE_IMAGE2D_MAX_HEIGHT, size_t);
      PRINT_DEVICE_INFO(device, CL_DEVICE_IMAGE3D_MAX_WIDTH, size_t);
      PRINT_DEVICE_INFO(device, CL_DEVICE_IMAGE3D_MAX_HEIGHT, size_t);
      PRINT_DEVICE_INFO(device, CL_DEVICE_IMAGE3D_MAX_DEPTH, size_t);
      PRINT_DEVICE_INFO(device, CL_DEVICE_IMAGE_MAX_BUFFER_SIZE, size_t);
      PRINT_DEVICE_INFO(device, CL_DEVICE_IMAGE_MAX_ARRAY_SIZE, size_t);
      PRINT_DEVICE_INFO(device, CL_DEVICE_MAX_SAMPLERS, cl_uint);

      PRINT_DEVICE_INFO(device, CL_DEVICE_MAX_PARAMETER_SIZE, size_t);

      PRINT_DEVICE_INFO(device, CL_DEVICE_MEM_BASE_ADDR_ALIGN, cl_uint);

      PRINT_DEVICE_INFO(device, CL_DEVICE_SINGLE_FP_CONFIG,
                        cl_device_fp_config);
      PRINT_DEVICE_INFO(device, CL_DEVICE_DOUBLE_FP_CONFIG,
                        cl_device_fp_config);

      PRINT_DEVICE_INFO(device, CL_DEVICE_GLOBAL_MEM_CACHE_TYPE,
                        cl_device_mem_cache_type);
      PRINT_DEVICE_INFO(device, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, cl_ulong);
      PRINT_DEVICE_INFO(device, CL_DEVICE_GLOBAL_MEM_SIZE, cl_ulong);

      PRINT_DEVICE_INFO(device, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, cl_ulong);
      PRINT_DEVICE_INFO(device, CL_DEVICE_MAX_CONSTANT_ARGS, cl_uint);

      PRINT_DEVICE_INFO(device, CL_DEVICE_LOCAL_MEM_TYPE,
                        cl_device_local_mem_type);
      PRINT_DEVICE_INFO(device, CL_DEVICE_LOCAL_MEM_SIZE, cl_ulong);
      PRINT_DEVICE_INFO(device, CL_DEVICE_ERROR_CORRECTION_SUPPORT, cl_bool);

      PRINT_DEVICE_INFO(device, CL_DEVICE_HOST_UNIFIED_MEMORY, cl_bool);

      PRINT_DEVICE_INFO(device, CL_DEVICE_PROFILING_TIMER_RESOLUTION, size_t);

      PRINT_DEVICE_INFO(device, CL_DEVICE_ENDIAN_LITTLE, cl_bool);
      PRINT_DEVICE_INFO(device, CL_DEVICE_AVAILABLE, cl_bool);

      PRINT_DEVICE_INFO(device, CL_DEVICE_COMPILER_AVAILABLE, cl_bool);
      PRINT_DEVICE_INFO(device, CL_DEVICE_LINKER_AVAILABLE, cl_bool);

      PRINT_DEVICE_INFO(device, CL_DEVICE_EXECUTION_CAPABILITIES,
                        cl_device_exec_capabilities);

      PRINT_DEVICE_INFO(device, CL_DEVICE_QUEUE_PROPERTIES,
                        cl_command_queue_properties);

      PRINT_DEVICE_INFO(device, CL_DEVICE_BUILT_IN_KERNELS, char_array);

      // We already printed the devices platform, no point in repeating it.
      // PRINT_DEVICE_INFO(device, CL_DEVICE_PLATFORM, cl_platform_id);

      PRINT_DEVICE_INFO(device, CL_DEVICE_NAME, char_array);
      PRINT_DEVICE_INFO(device, CL_DEVICE_VENDOR, char_array);
      PRINT_DEVICE_INFO(device, CL_DEVICE_PROFILE, char_array);
      PRINT_DEVICE_INFO(device, CL_DEVICE_VERSION, char_array);
      PRINT_DEVICE_INFO(device, CL_DEVICE_OPENCL_C_VERSION, char_array);
      PRINT_DEVICE_INFO(device, CL_DRIVER_VERSION, char_array);
      PRINT_DEVICE_INFO(device, CL_DEVICE_EXTENSIONS, char_array);

      PRINT_DEVICE_INFO(device, CL_DEVICE_PRINTF_BUFFER_SIZE, size_t);

      PRINT_DEVICE_INFO(device, CL_DEVICE_PREFERRED_INTEROP_USER_SYNC, cl_bool);

      // As we only print info for built-in it makes no sense to print the
      // parent device.
      // PRINT_DEVICE_INFO(device, CL_DEVICE_PARENT_DEVICE, cl_device_id);
      PRINT_DEVICE_INFO(device, CL_DEVICE_PARTITION_MAX_SUB_DEVICES, cl_uint);
      PRINT_DEVICE_INFO(device, CL_DEVICE_PARTITION_PROPERTIES,
                        cl_device_partition_property_array);
      PRINT_DEVICE_INFO(device, CL_DEVICE_PARTITION_AFFINITY_DOMAIN,
                        cl_device_affinity_domain);
      PRINT_DEVICE_INFO(device, CL_DEVICE_PARTITION_TYPE,
                        cl_device_partition_property_array);

      // As we only print info for built-in devices which cannot be retained
      // this makes no sense.
      // PRINT_DEVICE_INFO(device, CL_DEVICE_REFERENCE_COUNT, cl_uint);
    }
  }

  return true;
}

// END TNEX

void UCL::printDevicesImageSupport(const unsigned numDevices,
                                   cl_device_id *devices) {
  for (unsigned i = 0; i < numDevices; ++i) {
    cl_device_id device = devices[i];
    cl_uint deviceVendorId = 0;
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_VENDOR_ID,
                                   sizeof(deviceVendorId), &deviceVendorId,
                                   nullptr));

    cl_bool hasImageSupport = CL_FALSE;
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT,
                                   sizeof(hasImageSupport), &hasImageSupport,
                                   nullptr));
    const char *imageSupport = hasImageSupport ? "ON" : "OFF";
    std::fprintf(stdout,
                 "UnitCL device : %u CL_DEVICE_VENDOR_ID : %u "
                 "CL_DEVICE_IMAGE_SUPPORT : %s\n",
                 i, deviceVendorId, imageSupport);
  }
}

cl_uint UCL::getNumPlatforms() {
  return static_cast<cl_uint>(ucl::Environment::instance->platforms.size());
}

cl_platform_id *UCL::getPlatforms() {
  return ucl::Environment::instance->platforms.data();
}

cl_uint UCL::getNumDevices() {
  return static_cast<cl_uint>(ucl::Environment::instance->devices.size());
}

cl_device_id *UCL::getDevices() {
  return ucl::Environment::instance->devices.data();
}

std::string UCL::getTestIncludePath() {
  return ucl::Environment::instance->testIncludePath;
}

std::string UCL::getTestIncludePathWithQuotedSpaces() {
  return ucl::Environment::instance->testIncludePath +
         "/\"directory with spaces\"";
}

std::string UCL::getTestIncludePathWithBackslashedSpaces() {
  return ucl::Environment::instance->testIncludePath +
         "/directory\\ with\\ spaces";
}

bool UCL::hasTestIncludePath() { return UCL::getTestIncludePath().size() > 0; }

void UCL::checkTestIncludePath() {
  // Ensure that the test programs involving includes will be able to compile.
  // This uses some internal Google Test API's, but as we manually update
  // Google Test this won't break by surprise.
  typedef testing::internal::FilePath Path;
  std::string test_include = UCL::getTestIncludePath();
  std::string msg =
      "You must set --unitcl_test_include to the supplied 'test_include/' "
      "directory.";

  // Check the main directory and files are where we will be looking for them.
  Path dir1(test_include);
  Path file1(test_include + "/test_include.h");
  Path file2(test_include + "/test_empty_include.h");
  Path file3(test_include + "/test_declare_only_include.h");
  ASSERT_EQ(true, UCL::hasTestIncludePath()) << msg;
  ASSERT_EQ(true, dir1.DirectoryExists())
      << msg << std::endl
      << "'" << dir1.string() << "' is missing.";
  ASSERT_EQ(true, file1.FileOrDirectoryExists())
      << msg << "'" << std::endl
      << file1.string() << "' is missing.";
  ASSERT_EQ(true, file2.FileOrDirectoryExists())
      << msg << "'" << std::endl
      << file2.string() << "' is missing.";
  ASSERT_EQ(true, file3.FileOrDirectoryExists())
      << msg << "'" << std::endl
      << file3.string() << "' is missing.";

  // Also check that the test include directory contains the with-spaces test
  // directory and file.
  Path dir2(test_include + "/directory with spaces");
  Path file4(test_include + "/directory with spaces/test_include.h");
  ASSERT_EQ(true, dir2.DirectoryExists())
      << msg << std::endl
      << "'" << dir2.string() << "' is missing.";
  ASSERT_EQ(true, file4.FileOrDirectoryExists())
      << msg << "'" << std::endl
      << file4.string() << "' is missing.";
}

bool UCL::isDeviceName(cl_device_id device, const char *name) {
  size_t size;
  EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr, &size));
  // ASSERT_EQ(size, 0u);
  UCL::Buffer<char> payload(size);
  EXPECT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_NAME, size, payload, nullptr));

  if (cargo::string_view(payload.data()) != name) {
    return false;
  }
  return true;
}

bool UCL::isDeviceVersionAtLeast(ucl::Version version) {
  return ucl::Environment::instance->deviceOpenCLVersion >= version;
}

bool UCL::isPlatformVersion(const char *version_string) {
  if (ucl::Environment::instance->platformOCLVersion.find(version_string) !=
      std::string::npos) {
    return true;
  }
  return false;
}

bool UCL::hasImageSupport(cl_device_id device) {
  cl_bool imageSupport = CL_FALSE;
  EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT,
                                 sizeof(cl_bool), &imageSupport, nullptr));
  return (CL_TRUE == imageSupport) ? true : false;
}

bool UCL::isImageFormatSupported(cl_context context,
                                 const UCL::vector<cl_mem_flags> &flags_list,
                                 cl_mem_object_type type,
                                 const cl_image_format &format) {
  UCL_ASSERT(CL_MEM_OBJECT_BUFFER != type,
             "type must not be CL_MEM_OBJECT_BUFFER");

  bool isSupported = false;
  for (const auto &flags : flags_list) {
    cl_uint supportedFormatCount = 0;
    cl_int error = clGetSupportedImageFormats(context, flags, type, 0, nullptr,
                                              &supportedFormatCount);
    UCL_ASSERT(!error, "clGetSupportedImageFormats failed");
    if (0 == supportedFormatCount) {
      isSupported = false;
      break;
    }

    UCL::vector<cl_image_format> supportedFormats(supportedFormatCount);
    error =
        clGetSupportedImageFormats(context, flags, type, supportedFormatCount,
                                   supportedFormats.data(), nullptr);
    UCL_ASSERT(!error, "clGetSupportedImageFormats failed");
    for (const auto &supportedFormat : supportedFormats) {
      if (supportedFormat.image_channel_data_type ==
              format.image_channel_data_type &&
          supportedFormat.image_channel_order == format.image_channel_order) {
        isSupported = true;
        break;
      }
    }
  }

#define CASE(VALUE) \
  case VALUE:       \
    return #VALUE

  auto channelOrderStr = [](cl_channel_order order) {
    switch (order) {
      CASE(CL_R);
      CASE(CL_A);
      CASE(CL_RG);
      CASE(CL_RA);
      CASE(CL_RGB);
      CASE(CL_RGBA);
      CASE(CL_BGRA);
      CASE(CL_ARGB);
      CASE(CL_INTENSITY);
      CASE(CL_LUMINANCE);
      CASE(CL_Rx);
      CASE(CL_RGx);
      CASE(CL_RGBx);
      CASE(CL_DEPTH);
      CASE(CL_DEPTH_STENCIL);
      default:
        return "UNKNOWN CHANNEL ORDER";
    }
  };

  auto channelDataTypeStr = [](cl_channel_type type) {
    switch (type) {
      CASE(CL_SNORM_INT8);
      CASE(CL_SNORM_INT16);
      CASE(CL_UNORM_INT8);
      CASE(CL_UNORM_INT16);
      CASE(CL_UNORM_SHORT_565);
      CASE(CL_UNORM_SHORT_555);
      CASE(CL_UNORM_INT_101010);
      CASE(CL_SIGNED_INT8);
      CASE(CL_SIGNED_INT16);
      CASE(CL_SIGNED_INT32);
      CASE(CL_UNSIGNED_INT8);
      CASE(CL_UNSIGNED_INT16);
      CASE(CL_UNSIGNED_INT32);
      CASE(CL_HALF_FLOAT);
      CASE(CL_FLOAT);
      CASE(CL_UNORM_INT24);
      default:
        return "UNKNOWN CHANNEL DATA TYPE";
    }
  };

#undef CASE

  if (!isSupported) {
    printf("Image format { %s, %s } not supported, skipping...\n",
           channelOrderStr(format.image_channel_order),
           channelDataTypeStr(format.image_channel_data_type));
  }

  return isSupported;
}

bool UCL::hasCorrectlyRoundedDivideSqrtSupport(cl_device_id device) {
  cl_device_fp_config fpConfig = CL_FALSE;
  EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_SINGLE_FP_CONFIG,
                                 sizeof(cl_device_fp_config), &fpConfig,
                                 nullptr));
  return CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT & fpConfig;
}

bool UCL::hasSubDeviceSupport(cl_device_id device) {
  return hasDevicePartitionSupport(device, CL_DEVICE_PARTITION_EQUALLY) ||
         hasDevicePartitionSupport(device, CL_DEVICE_PARTITION_BY_COUNTS) ||
         hasDevicePartitionSupport(device,
                                   CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN);
}

bool UCL::hasNativeKernelSupport(cl_device_id device) {
  cl_device_exec_capabilities exec = 0;
  EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_EXECUTION_CAPABILITIES,
                                 sizeof(exec), &exec, nullptr));
  return CL_EXEC_NATIVE_KERNEL & exec;
}

bool UCL::hasDevicePartitionSupport(
    cl_device_id device, const cl_device_partition_property property) {
  size_t size = 0;

  if (CL_SUCCESS != clGetDeviceInfo(device, CL_DEVICE_PARTITION_PROPERTIES, 0,
                                    nullptr, &size)) {
    return false;
  }

  if (0 == size) {
    return false;
  }

  UCL::vector<cl_device_partition_property> properties(
      size / sizeof(cl_device_partition_property));

  if (CL_SUCCESS != clGetDeviceInfo(device, CL_DEVICE_PARTITION_PROPERTIES,
                                    size, properties.data(), nullptr)) {
    return false;
  }

  for (UCL::vector<cl_device_partition_property>::iterator
           iter = properties.begin(),
           iter_end = properties.end();
       iter != iter_end; ++iter) {
    if (property == *iter) {
      return true;
    }
  }

  return false;
}

bool UCL::isExtraCompileOptEnabled(std::string option) {
  const char *value = std::getenv("CA_EXTRA_COMPILE_OPTS");
  return value &&
         cargo::string_view{value}.find(option) != cargo::string_view::npos;
}

bool UCL::isInterceptLayerPresent() {
  return std::getenv("CLI_OpenCLFileName") != nullptr;
}

bool UCL::isInterceptLayerControlEnabled(std::string control) {
  if (!cargo::string_view{control}.starts_with("CLI_")) {
    control = "CLI_" + control;
  }
  const char *value = std::getenv(control.c_str());
  return value && value[0] != '0';
}

bool UCL::isQueueInOrder(cl_command_queue command_queue) {
  cl_command_queue_properties command_queue_properties;
  EXPECT_SUCCESS(clGetCommandQueueInfo(command_queue, CL_QUEUE_PROPERTIES,
                                       sizeof(cl_command_queue_properties),
                                       &command_queue_properties, nullptr));
  return !(command_queue_properties & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
}

size_t UCL::getTypeSize(const char *const type) {
  if (0 == strcmp(type, "char")) {
    return sizeof(cl_char);
  } else if (0 == strcmp(type, "char2")) {
    return sizeof(cl_char2);
  } else if (0 == strcmp(type, "char3")) {
    return 3 * sizeof(cl_char);
  } else if (0 == strcmp(type, "char4")) {
    return sizeof(cl_char4);
  } else if (0 == strcmp(type, "char8")) {
    return sizeof(cl_char8);
  } else if (0 == strcmp(type, "char16")) {
    return sizeof(cl_char16);
  } else if (0 == strcmp(type, "uchar")) {
    return sizeof(cl_uchar);
  } else if (0 == strcmp(type, "uchar2")) {
    return sizeof(cl_uchar2);
  } else if (0 == strcmp(type, "uchar3")) {
    return 3 * sizeof(cl_uchar);
  } else if (0 == strcmp(type, "uchar4")) {
    return sizeof(cl_uchar4);
  } else if (0 == strcmp(type, "uchar8")) {
    return sizeof(cl_uchar8);
  } else if (0 == strcmp(type, "uchar16")) {
    return sizeof(cl_uchar16);
  } else if (0 == strcmp(type, "short")) {
    return sizeof(cl_short);
  } else if (0 == strcmp(type, "short2")) {
    return sizeof(cl_short2);
  } else if (0 == strcmp(type, "short3")) {
    return 3 * sizeof(cl_short);
  } else if (0 == strcmp(type, "short4")) {
    return sizeof(cl_short4);
  } else if (0 == strcmp(type, "short8")) {
    return sizeof(cl_short8);
  } else if (0 == strcmp(type, "short16")) {
    return sizeof(cl_short16);
  } else if (0 == strcmp(type, "ushort")) {
    return sizeof(cl_ushort);
  } else if (0 == strcmp(type, "ushort2")) {
    return sizeof(cl_ushort2);
  } else if (0 == strcmp(type, "ushort3")) {
    return 3 * sizeof(cl_ushort);
  } else if (0 == strcmp(type, "ushort4")) {
    return sizeof(cl_ushort4);
  } else if (0 == strcmp(type, "ushort8")) {
    return sizeof(cl_ushort8);
  } else if (0 == strcmp(type, "ushort16")) {
    return sizeof(cl_ushort16);
  } else if (0 == strcmp(type, "int")) {
    return sizeof(cl_int);
  } else if (0 == strcmp(type, "int2")) {
    return sizeof(cl_int2);
  } else if (0 == strcmp(type, "int3")) {
    return 3 * sizeof(cl_int);
  } else if (0 == strcmp(type, "int4")) {
    return sizeof(cl_int4);
  } else if (0 == strcmp(type, "int8")) {
    return sizeof(cl_int8);
  } else if (0 == strcmp(type, "int16")) {
    return sizeof(cl_int16);
  } else if (0 == strcmp(type, "uint")) {
    return sizeof(cl_uint);
  } else if (0 == strcmp(type, "uint2")) {
    return sizeof(cl_uint2);
  } else if (0 == strcmp(type, "uint3")) {
    return 3 * sizeof(cl_uint);
  } else if (0 == strcmp(type, "uint4")) {
    return sizeof(cl_uint4);
  } else if (0 == strcmp(type, "uint8")) {
    return sizeof(cl_uint8);
  } else if (0 == strcmp(type, "uint16")) {
    return sizeof(cl_uint16);
  } else if (0 == strcmp(type, "long")) {
    return sizeof(cl_long);
  } else if (0 == strcmp(type, "long2")) {
    return sizeof(cl_long2);
  } else if (0 == strcmp(type, "long3")) {
    return 3 * sizeof(cl_long);
  } else if (0 == strcmp(type, "long4")) {
    return sizeof(cl_long4);
  } else if (0 == strcmp(type, "long8")) {
    return sizeof(cl_long8);
  } else if (0 == strcmp(type, "long16")) {
    return sizeof(cl_long16);
  } else if (0 == strcmp(type, "ulong")) {
    return sizeof(cl_ulong);
  } else if (0 == strcmp(type, "ulong2")) {
    return sizeof(cl_ulong2);
  } else if (0 == strcmp(type, "ulong3")) {
    return 3 * sizeof(cl_ulong);
  } else if (0 == strcmp(type, "ulong4")) {
    return sizeof(cl_ulong4);
  } else if (0 == strcmp(type, "ulong8")) {
    return sizeof(cl_ulong8);
  } else if (0 == strcmp(type, "ulong16")) {
    return sizeof(cl_ulong16);
  } else if (0 == strcmp(type, "float")) {
    return sizeof(cl_float);
  } else if (0 == strcmp(type, "float2")) {
    return sizeof(cl_float2);
  } else if (0 == strcmp(type, "float3")) {
    return sizeof(cl_float3);
  } else if (0 == strcmp(type, "float4")) {
    return sizeof(cl_float4);
  } else if (0 == strcmp(type, "float8")) {
    return sizeof(cl_float8);
  } else if (0 == strcmp(type, "float16")) {
    return sizeof(cl_float16);
  } else if (0 == strcmp(type, "double")) {
    return sizeof(cl_double);
  } else if (0 == strcmp(type, "double2")) {
    return sizeof(cl_double2);
  } else if (0 == strcmp(type, "double3")) {
    return sizeof(cl_double3);
  } else if (0 == strcmp(type, "double4")) {
    return sizeof(cl_double4);
  } else if (0 == strcmp(type, "double8")) {
    return sizeof(cl_double8);
  } else if (0 == strcmp(type, "double16")) {
    return sizeof(cl_double16);
  } else {
    UCL_ASSERT(false, "Unknown type!");
  }
}

bool UCL::hasPlatformExtensionSupport(const char *extension_name) {
  cl_platform_id *platforms = getPlatforms();
  cl_platform_id platform = platforms[0];

  size_t extension_names_size = 0;
  EXPECT_SUCCESS(clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, 0, nullptr,
                                   &extension_names_size));

  UCL::Buffer<char> extension_names(extension_names_size);
  EXPECT_SUCCESS(clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS,
                                   extension_names.size(), extension_names,
                                   nullptr));

  return nullptr != std::strstr(extension_names, extension_name);
}

bool UCL::hasDeviceExtensionSupport(cl_device_id device,
                                    const char *extension_name) {
  size_t extension_names_size = 0;
  EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, 0, nullptr,
                                 &extension_names_size));
  UCL::Buffer<char> extension_names(extension_names_size);

  EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS,
                                 extension_names.size(), extension_names,
                                 nullptr));

  return nullptr != std::strstr(extension_names, extension_name);
}

bool UCL::hasSupportForOpenCL_C_1_0(cl_device_id device) {
  // Assuming that OpenCL C 1.0 is always supported.
  (void)device;  // Suppress unused warning.
  return true;
}

bool UCL::hasSupportForOpenCL_C_1_1(cl_device_id device) {
  size_t size = 0;
  EXPECT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_VERSION, 0, nullptr, &size));
  UCL::Buffer<char> payload(size);
  EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_VERSION, size,
                                 payload, nullptr));
  return !std::strstr(payload, "OpenCL C 1.0");
}

bool UCL::hasSupportForOpenCL_C_1_2(cl_device_id device) {
  size_t size = 0;
  EXPECT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_VERSION, 0, nullptr, &size));
  UCL::Buffer<char> payload(size);
  EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_VERSION, size,
                                 payload, nullptr));
  return !std::strstr(payload, "OpenCL C 1.1") &&
         !std::strstr(payload, "OpenCL C 1.0");
}

cl_int UCL::buildProgram(cl_program program, cl_device_id device,
                         const char *options) {
  auto error = clBuildProgram(program, 1, &device, options, nullptr, nullptr);
  if (error == CL_BUILD_PROGRAM_FAILURE) {
    size_t size;
    if (clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr,
                              &size)) {
      return error;
    }
    std::vector<char> log(size);
    if (clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, size,
                              log.data(), nullptr)) {
      return error;
    }
    EXPECT_SUCCESS(error) << log.data() << "\n";
  }
  return error;
}

void *UCL::aligned_alloc(uint32_t align, size_t bytes) {
  UCL_ASSERT(0 != align, "UCL::aligned_alloc align must not be zero!");
  // NOTE: Due to the platform specific aligned allocation constraints stated
  // below the alignment should be a power of two, also the new alignment must
  // be a multiple of align and also a multiple of sizeof(void*) so keep raising
  // to the power two until this holds true.
  UCL_ASSERT(0 == (align & (align - 1)),
             "UCL::algned_alloc must be a power of two");
  uint32_t alignment = std::max(align, static_cast<uint32_t>(sizeof(void *)));
  void *ptr = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
  // NOTE: The MSVC documentation for _aligned_alloc states that the alignment
  // must be an integral power of 2.
  ptr = _aligned_malloc(bytes, alignment);
  UCL_ASSERT(ENOMEM != errno, "_aligned_malloc failed!");
#elif defined __ANDROID__
  ptr = memalign(alignment, bytes);
  UCL_ASSERT(ptr, "memalign failed!");
#else
  // NOTE: The Open Group documentation for posix_memalign states that the
  // alignment must be a multiple of sizeof(void*).
  int error = posix_memalign(&ptr, alignment, bytes);
  UCL_ASSERT(!error, "posix_memalign failed!");
#endif
  UCL_ASSERT(ptr, "UCL::aligned_alloc failed!");
  return ptr;
}

void UCL::aligned_free(void *ptr) {
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
  _aligned_free(ptr);
#else
  free(ptr);
#endif
}

size_t UCL::getPixelSize(const cl_image_format &format) {
#define SIZE(ELEMENTS, DATA_TYPE) \
  switch (DATA_TYPE) {            \
    case CL_SNORM_INT8:           \
      return (ELEMENTS) * 1;      \
    case CL_SNORM_INT16:          \
      return (ELEMENTS) * 2;      \
    case CL_UNORM_INT8:           \
      return (ELEMENTS) * 1;      \
    case CL_UNORM_INT16:          \
      return (ELEMENTS) * 2;      \
    case CL_UNORM_SHORT_565:      \
      return 2;                   \
    case CL_UNORM_SHORT_555:      \
      return 2;                   \
    case CL_UNORM_INT_101010:     \
      return 4;                   \
    case CL_SIGNED_INT8:          \
      return (ELEMENTS) * 1;      \
    case CL_SIGNED_INT16:         \
      return (ELEMENTS) * 2;      \
    case CL_SIGNED_INT32:         \
      return (ELEMENTS) * 4;      \
    case CL_UNSIGNED_INT8:        \
      return (ELEMENTS) * 1;      \
    case CL_UNSIGNED_INT16:       \
      return (ELEMENTS) * 2;      \
    case CL_UNSIGNED_INT32:       \
      return (ELEMENTS) * 4;      \
    case CL_HALF_FLOAT:           \
      return (ELEMENTS) * 2;      \
    case CL_FLOAT:                \
      return (ELEMENTS) * 4;      \
    default:                      \
      return (ELEMENTS) * 0;      \
  }
  switch (format.image_channel_order) {
    case CL_R:
      SIZE(1, format.image_channel_data_type)
    case CL_Rx:
      SIZE(1, format.image_channel_data_type)
    case CL_A:
      SIZE(1, format.image_channel_data_type)
    case CL_INTENSITY:
      SIZE(1, format.image_channel_data_type)
    case CL_LUMINANCE:
      SIZE(1, format.image_channel_data_type)
    case CL_RG:
      SIZE(2, format.image_channel_data_type)
    case CL_RGx:
      SIZE(2, format.image_channel_data_type)
    case CL_RA:
      SIZE(2, format.image_channel_data_type)
    case CL_RGB:
      SIZE(3, format.image_channel_data_type)
    case CL_RGBx:
      SIZE(3, format.image_channel_data_type)
    case CL_RGBA:
      SIZE(4, format.image_channel_data_type)
    case CL_ARGB:
      SIZE(4, format.image_channel_data_type)
    case CL_BGRA:
      SIZE(4, format.image_channel_data_type)
    default:
      break;
  }
#undef SIZE
  return 0;
}

bool UCL::hasLocalWorkSizeSupport(cl_device_id device, cl_uint work_dim,
                                  const size_t *local_work_size) {
  // If the runtime is picking local work size then it will pick a legal one.
  if (nullptr == local_work_size) {
    return true;
  }

  cl_int err = !CL_SUCCESS;
  cl_uint max_work_item_dimensions;
  err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
                        sizeof(cl_uint), &max_work_item_dimensions, nullptr);
  if (err != CL_SUCCESS) {
    // We can't determine device info, lets just let the test continue and see
    // what happens.
    return true;
  }

  size_t max_work_group_size;
  err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t),
                        &max_work_group_size, nullptr);
  if (err != CL_SUCCESS) {
    // We can't determine device info, lets just let the test continue and see
    // what happens.
    return true;
  }

  std::vector<size_t> max_work_item_sizes(max_work_item_dimensions);
  err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES,
                        sizeof(size_t) * max_work_item_dimensions,
                        max_work_item_sizes.data(), nullptr);
  if (err != CL_SUCCESS) {
    // We can't determine device info, lets just let the test continue and see
    // what happens.
    return true;
  }

  if (work_dim > max_work_item_dimensions) {
    return false;
  }

  size_t total_work_size = 1;
  for (cl_uint i = 0; i < work_dim; i++) {
    total_work_size *= local_work_size[i];
  }

  if (total_work_size > max_work_group_size) {
    return false;
  }

  for (cl_uint i = 0; i < work_dim; i++) {
    if (local_work_size[i] > max_work_item_sizes[i]) {
      return false;
    }
  }

  return true;
}

bool UCL::hasDenormSupport(cl_device_id device, cl_uint precision) {
  cl_device_fp_config config;

  switch (precision) {
    case CL_DEVICE_HALF_FP_CONFIG:
    case CL_DEVICE_SINGLE_FP_CONFIG:
    case CL_DEVICE_DOUBLE_FP_CONFIG:
      break;
    default:
      return false;
  }

  if (CL_SUCCESS !=
      clGetDeviceInfo(device, precision, sizeof(config), &config, nullptr)) {
    return false;
  }
  return CL_FP_DENORM & config;
}

bool UCL::hasHalfSupport(cl_device_id device) {
  // Check if the fp16 extension is enabled
  return UCL::hasDeviceExtensionSupport(device, "cl_khr_fp16");
}

bool UCL::hasDoubleSupport(cl_device_id device) {
  // check if device supports double
  cl_device_fp_config config;

  // if CL failed for some reason, we assume there is no double support
  if (CL_SUCCESS != clGetDeviceInfo(device, CL_DEVICE_DOUBLE_FP_CONFIG,
                                    sizeof(config), &config, nullptr)) {
    return false;
  }

  // if config is 0, then device doesn't support double
  return 0 != config;
}

bool UCL::hasAtomic64Support(cl_device_id device) {
  return UCL::hasDeviceExtensionSupport(device, "cl_khr_int64_base_atomics") &&
         UCL::hasDeviceExtensionSupport(device,
                                        "cl_khr_int64_extended_atomics");
}

bool UCL::hasCompilerSupport(cl_device_id device) {
  cl_bool has_compiler;
  UCL_ASSERT(
      CL_SUCCESS == clGetDeviceInfo(device, CL_DEVICE_COMPILER_AVAILABLE,
                                    sizeof(cl_bool), &has_compiler, nullptr),
      "clGetDeviceInfo() failed.");
  return has_compiler;
}

std::string UCL::getDeviceName(cl_device_id device) {
  size_t size;
  cl_int error = clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr, &size);
  if (error) {
    std::cerr << "error: failed to get device name\n";
    std::abort();
  }
  std::vector<char> name(size);
  error = clGetDeviceInfo(device, CL_DEVICE_NAME, size, name.data(), nullptr);
  if (error) {
    std::cerr << "error: failed to get device name\n";
    std::abort();
  }
  return cargo::as<std::string>(cargo::trim(name));
}

cl_uint UCL::getDeviceAddressBits(cl_device_id device) {
  cl_uint address_bits;

  // If CL failed for some reason, return an impossibly large value.
  if (CL_SUCCESS != clGetDeviceInfo(device, CL_DEVICE_ADDRESS_BITS,
                                    sizeof(address_bits), &address_bits,
                                    nullptr)) {
    return std::numeric_limits<cl_uint>::max();
  }

  return address_bits;
}

#if defined(_MSC_VER)
/// @brief Parse hexadecimal floats. This is only needed for Visual Studio,
///        as it does not support hexadecimal floating point notation.
///        Currently this only supports floats of the form 0x0p0f, 0x1pEf, and
///        [-]0x1.MpEf, where M is a hexadecimal number consisting of at most 6
///        characters and where E is the exponent; this suffices for our
///        purposes.
float UCL::parse_hexfloat(const char *str) {
  EXPECT_TRUE(std::strchr(str, 'p') != nullptr);

  // 0x0p0f is special
  if (std::strcmp(str, "0x0p0f") == 0) {
    return 0.0f;
  }

  // Look for minus sign, which goes in the high bit.
  uint32_t float_bits = 0u;
  if (str[0] == '-') {
    float_bits |= 1u << 31;
    str++;
  }

  // Check for that we have the correct prefix
  const char prefix[] = "0x1";
  EXPECT_EQ(0, std::strncmp(str, prefix, std::strlen(prefix)));
  str += std::strlen(prefix);

  if (str[0] == '.') {
    str++;
  }

  // Parse the mantissa taking into account that the number of digits we have
  // might vary between 0 and 6.
  uint32_t mantissa = 0u;
  unsigned num_count = 0;
  for (; str[0] != 'p'; ++str, ++num_count) {
    uint32_t val = 0;
    if (str[0] >= '0' && str[0] <= '9') {
      val = str[0] - '0';
    } else if (str[0] >= 'a' && str[0] <= 'f') {
      val = str[0] - 'a' + 10;
    } else {
      EXPECT_TRUE(false);
    }
    mantissa = mantissa << 4 | val;
  }

  const unsigned max_count = 6;
  EXPECT_TRUE(num_count <= max_count);
  // Shift if we had found less than 6 digits in the mantissa.
  mantissa <<= (4 * (max_count - num_count));
  // The last bit will always be 0, and is not part of the representation.
  mantissa >>= 1;

  int exponent;
  int ret = std::sscanf(str, "p%df", &exponent);
  EXPECT_EQ(1, ret);
  exponent += 127;

  // If the exponent is negative we have a denormal
  if (exponent >= 0) {
    float_bits |= (exponent & 0xff) << 23;  // exponent starts at bit 23.
  } else {
    // For denormals we need the 1 from the prefix we skipped over above
    mantissa |= 1 << 23;
    mantissa >>= -exponent + 1;
  }

  float_bits |= mantissa;

  return cargo::bit_cast<float>(float_bits);
}
#endif  // _MSC_VER

bool UCL::isDevice(cl_device_id device, const char *check_device_prefix,
                   cl_device_type check_device_type) {
  size_t device_name_len = 0;
  EXPECT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr, &device_name_len));

  std::string device_name(device_name_len, '\0');
  EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NAME, device_name_len,
                                 &device_name[0], nullptr));

  cl_device_type device_type;
  EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(device_type),
                                 &device_type, nullptr));

  return ((0 == device_name.compare(0, std::strlen(check_device_prefix),
                                    check_device_prefix)) &&
          (device_type & check_device_type));
}

bool UCL::hasCommandExecutionCompleted(cl_event event) {
  cl_int read_status;
  EXPECT_SUCCESS(clGetEventInfo(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                sizeof(cl_int), &read_status, nullptr));
  return read_status == CL_COMPLETE;
}

std::string UCL::handleUnhandledCase(const std::string &type,
                                     cl_int enum_value) {
  std::stringstream strstr;
  strstr.str("");
  strstr << "Unknown " << type << ": " << enum_value;
  return strstr.str();
}

std::string UCL::deviceQueryToString(cl_device_info query) {
  // Helper macro to case the query using stringification.
#define DEVICE_QUERY_CASE(DEVICE_QUERY) \
  case DEVICE_QUERY: {                  \
    return #DEVICE_QUERY;               \
  } break

  switch (query) {
    DEVICE_QUERY_CASE(CL_DEVICE_TYPE);
    DEVICE_QUERY_CASE(CL_DEVICE_VENDOR_ID);
    DEVICE_QUERY_CASE(CL_DEVICE_MAX_COMPUTE_UNITS);
    DEVICE_QUERY_CASE(CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS);
    DEVICE_QUERY_CASE(CL_DEVICE_MAX_WORK_GROUP_SIZE);
    DEVICE_QUERY_CASE(CL_DEVICE_MAX_WORK_ITEM_SIZES);
    DEVICE_QUERY_CASE(CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR);
    DEVICE_QUERY_CASE(CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT);
    DEVICE_QUERY_CASE(CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT);
    DEVICE_QUERY_CASE(CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG);
    DEVICE_QUERY_CASE(CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT);
    DEVICE_QUERY_CASE(CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE);
    DEVICE_QUERY_CASE(CL_DEVICE_MAX_CLOCK_FREQUENCY);
    DEVICE_QUERY_CASE(CL_DEVICE_ADDRESS_BITS);
    DEVICE_QUERY_CASE(CL_DEVICE_MAX_READ_IMAGE_ARGS);
    DEVICE_QUERY_CASE(CL_DEVICE_MAX_WRITE_IMAGE_ARGS);
    DEVICE_QUERY_CASE(CL_DEVICE_MAX_MEM_ALLOC_SIZE);
    DEVICE_QUERY_CASE(CL_DEVICE_IMAGE2D_MAX_WIDTH);
    DEVICE_QUERY_CASE(CL_DEVICE_IMAGE2D_MAX_HEIGHT);
    DEVICE_QUERY_CASE(CL_DEVICE_IMAGE3D_MAX_WIDTH);
    DEVICE_QUERY_CASE(CL_DEVICE_IMAGE3D_MAX_HEIGHT);
    DEVICE_QUERY_CASE(CL_DEVICE_IMAGE3D_MAX_DEPTH);
    DEVICE_QUERY_CASE(CL_DEVICE_IMAGE_SUPPORT);
    DEVICE_QUERY_CASE(CL_DEVICE_MAX_PARAMETER_SIZE);
    DEVICE_QUERY_CASE(CL_DEVICE_MAX_SAMPLERS);
    DEVICE_QUERY_CASE(CL_DEVICE_MEM_BASE_ADDR_ALIGN);
    DEVICE_QUERY_CASE(CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE);
    DEVICE_QUERY_CASE(CL_DEVICE_SINGLE_FP_CONFIG);
    DEVICE_QUERY_CASE(CL_DEVICE_GLOBAL_MEM_CACHE_TYPE);
    DEVICE_QUERY_CASE(CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE);
    DEVICE_QUERY_CASE(CL_DEVICE_GLOBAL_MEM_CACHE_SIZE);
    DEVICE_QUERY_CASE(CL_DEVICE_GLOBAL_MEM_SIZE);
    DEVICE_QUERY_CASE(CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE);
    DEVICE_QUERY_CASE(CL_DEVICE_MAX_CONSTANT_ARGS);
    DEVICE_QUERY_CASE(CL_DEVICE_LOCAL_MEM_TYPE);
    DEVICE_QUERY_CASE(CL_DEVICE_LOCAL_MEM_SIZE);
    DEVICE_QUERY_CASE(CL_DEVICE_ERROR_CORRECTION_SUPPORT);
    DEVICE_QUERY_CASE(CL_DEVICE_PROFILING_TIMER_RESOLUTION);
    DEVICE_QUERY_CASE(CL_DEVICE_ENDIAN_LITTLE);
    DEVICE_QUERY_CASE(CL_DEVICE_AVAILABLE);
    DEVICE_QUERY_CASE(CL_DEVICE_COMPILER_AVAILABLE);
    DEVICE_QUERY_CASE(CL_DEVICE_EXECUTION_CAPABILITIES);
#if CL_TARGET_OPENCL_VERSION < 200
    // CL_DEVICE_QUEUE_PROPERTIES and CL_DEVICE_QUEUE_ON_HOST_PROPERTIES
    // have the same enum value and therefore cannot both have case
    // statements in the switch statement. CL_DEVICE_QUEUE_PROPERTIES is
    // deprecated after OpenCL-2.0.
    DEVICE_QUERY_CASE(CL_DEVICE_QUEUE_PROPERTIES);
#else
    DEVICE_QUERY_CASE(CL_DEVICE_QUEUE_ON_HOST_PROPERTIES);
#endif
    DEVICE_QUERY_CASE(CL_DEVICE_NAME);
    DEVICE_QUERY_CASE(CL_DEVICE_VENDOR);
    DEVICE_QUERY_CASE(CL_DRIVER_VERSION);
    DEVICE_QUERY_CASE(CL_DEVICE_PROFILE);
    DEVICE_QUERY_CASE(CL_DEVICE_VERSION);
    DEVICE_QUERY_CASE(CL_DEVICE_EXTENSIONS);
    DEVICE_QUERY_CASE(CL_DEVICE_PLATFORM);
#ifdef CL_VERSION_1_2
    DEVICE_QUERY_CASE(CL_DEVICE_DOUBLE_FP_CONFIG);
#endif
#ifdef CL_VERSION_1_1
    DEVICE_QUERY_CASE(CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF);
    DEVICE_QUERY_CASE(CL_DEVICE_HOST_UNIFIED_MEMORY);
    DEVICE_QUERY_CASE(CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR);
    DEVICE_QUERY_CASE(CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT);
    DEVICE_QUERY_CASE(CL_DEVICE_NATIVE_VECTOR_WIDTH_INT);
    DEVICE_QUERY_CASE(CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG);
    DEVICE_QUERY_CASE(CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT);
    DEVICE_QUERY_CASE(CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE);
    DEVICE_QUERY_CASE(CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF);
    DEVICE_QUERY_CASE(CL_DEVICE_OPENCL_C_VERSION);
#endif
#ifdef CL_VERSION_1_2
    DEVICE_QUERY_CASE(CL_DEVICE_LINKER_AVAILABLE);
    DEVICE_QUERY_CASE(CL_DEVICE_BUILT_IN_KERNELS);
    DEVICE_QUERY_CASE(CL_DEVICE_IMAGE_MAX_BUFFER_SIZE);
    DEVICE_QUERY_CASE(CL_DEVICE_IMAGE_MAX_ARRAY_SIZE);
    DEVICE_QUERY_CASE(CL_DEVICE_PARENT_DEVICE);
    DEVICE_QUERY_CASE(CL_DEVICE_PARTITION_MAX_SUB_DEVICES);
    DEVICE_QUERY_CASE(CL_DEVICE_PARTITION_PROPERTIES);
    DEVICE_QUERY_CASE(CL_DEVICE_PARTITION_AFFINITY_DOMAIN);
    DEVICE_QUERY_CASE(CL_DEVICE_PARTITION_TYPE);
    DEVICE_QUERY_CASE(CL_DEVICE_REFERENCE_COUNT);
    DEVICE_QUERY_CASE(CL_DEVICE_PREFERRED_INTEROP_USER_SYNC);
    DEVICE_QUERY_CASE(CL_DEVICE_PRINTF_BUFFER_SIZE);
#endif
#ifdef CL_VERSION_2_0
    DEVICE_QUERY_CASE(CL_DEVICE_IMAGE_PITCH_ALIGNMENT);
    DEVICE_QUERY_CASE(CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT);
    DEVICE_QUERY_CASE(CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS);
    DEVICE_QUERY_CASE(CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE);
    DEVICE_QUERY_CASE(CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES);
    DEVICE_QUERY_CASE(CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE);
    DEVICE_QUERY_CASE(CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE);
    DEVICE_QUERY_CASE(CL_DEVICE_MAX_ON_DEVICE_QUEUES);
    DEVICE_QUERY_CASE(CL_DEVICE_MAX_ON_DEVICE_EVENTS);
    DEVICE_QUERY_CASE(CL_DEVICE_SVM_CAPABILITIES);
    DEVICE_QUERY_CASE(CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE);
    DEVICE_QUERY_CASE(CL_DEVICE_MAX_PIPE_ARGS);
    DEVICE_QUERY_CASE(CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS);
    DEVICE_QUERY_CASE(CL_DEVICE_PIPE_MAX_PACKET_SIZE);
    DEVICE_QUERY_CASE(CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT);
    DEVICE_QUERY_CASE(CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT);
    DEVICE_QUERY_CASE(CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT);
#endif
#ifdef CL_VERSION_2_1
    DEVICE_QUERY_CASE(CL_DEVICE_IL_VERSION);
    DEVICE_QUERY_CASE(CL_DEVICE_MAX_NUM_SUB_GROUPS);
    DEVICE_QUERY_CASE(CL_DEVICE_SUB_GROUP_INDEPENDENT_FORWARD_PROGRESS);
#endif
#if defined(CL_VERSION_3_0)
    DEVICE_QUERY_CASE(CL_DEVICE_NUMERIC_VERSION);
    DEVICE_QUERY_CASE(CL_DEVICE_EXTENSIONS_WITH_VERSION);
    DEVICE_QUERY_CASE(CL_DEVICE_ILS_WITH_VERSION);
    DEVICE_QUERY_CASE(CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION);
#endif
#if defined(CL_VERSION_3_0)
    DEVICE_QUERY_CASE(CL_DEVICE_ATOMIC_MEMORY_CAPABILITIES);
    DEVICE_QUERY_CASE(CL_DEVICE_ATOMIC_FENCE_CAPABILITIES);
    DEVICE_QUERY_CASE(CL_DEVICE_NON_UNIFORM_WORK_GROUP_SUPPORT);
    DEVICE_QUERY_CASE(CL_DEVICE_OPENCL_C_ALL_VERSIONS);
    DEVICE_QUERY_CASE(CL_DEVICE_PREFERRED_WORK_GROUP_SIZE_MULTIPLE);
    DEVICE_QUERY_CASE(CL_DEVICE_WORK_GROUP_COLLECTIVE_FUNCTIONS_SUPPORT);
    DEVICE_QUERY_CASE(CL_DEVICE_GENERIC_ADDRESS_SPACE_SUPPORT);
    DEVICE_QUERY_CASE(CL_DEVICE_OPENCL_C_FEATURES);
    DEVICE_QUERY_CASE(CL_DEVICE_DEVICE_ENQUEUE_CAPABILITIES);
    DEVICE_QUERY_CASE(CL_DEVICE_PIPE_SUPPORT);
    DEVICE_QUERY_CASE(CL_DEVICE_LATEST_CONFORMANCE_VERSION_PASSED);
#endif
    default: {
      return handleUnhandledCase("device query", query);
    } break;
  }
#undef DEVICE_QUERY_CASE
}

std::string UCL::programQueryToString(cl_program_info query) {
  // Helper macro to case the query using stringification.
#define PROGRAM_QUERY_CASE(PROGRAM_QUERY) \
  case PROGRAM_QUERY: {                   \
    return #PROGRAM_QUERY;                \
  } break
  switch (query) {
    PROGRAM_QUERY_CASE(CL_PROGRAM_REFERENCE_COUNT);
    PROGRAM_QUERY_CASE(CL_PROGRAM_CONTEXT);
    PROGRAM_QUERY_CASE(CL_PROGRAM_NUM_DEVICES);
    PROGRAM_QUERY_CASE(CL_PROGRAM_DEVICES);
    PROGRAM_QUERY_CASE(CL_PROGRAM_SOURCE);
    PROGRAM_QUERY_CASE(CL_PROGRAM_BINARY_SIZES);
    PROGRAM_QUERY_CASE(CL_PROGRAM_BINARIES);
#ifdef CL_VERSION_1_2
    PROGRAM_QUERY_CASE(CL_PROGRAM_NUM_KERNELS);
    PROGRAM_QUERY_CASE(CL_PROGRAM_KERNEL_NAMES);
#endif
#ifdef CL_VERSION_2_1
    PROGRAM_QUERY_CASE(CL_PROGRAM_IL);
#endif
#ifdef CL_VERSION_2_2
    PROGRAM_QUERY_CASE(CL_PROGRAM_SCOPE_GLOBAL_CTORS_PRESENT);
    PROGRAM_QUERY_CASE(CL_PROGRAM_SCOPE_GLOBAL_DTORS_PRESENT);
#endif
    default: {
      return handleUnhandledCase("program query", query);
    } break;
  }
}
#undef PROGRAM_QUERY_CASE
std::string UCL::platformQueryToString(cl_platform_info query) {
  // Helper macro to case the query using stringification.
#define PLATFORM_QUERY_CASE(PLATFORM_QUERY) \
  case PLATFORM_QUERY: {                    \
    return #PLATFORM_QUERY;                 \
  } break
  switch (query) {
    PLATFORM_QUERY_CASE(CL_PLATFORM_PROFILE);
    PLATFORM_QUERY_CASE(CL_PLATFORM_VERSION);
    PLATFORM_QUERY_CASE(CL_PLATFORM_NAME);
    PLATFORM_QUERY_CASE(CL_PLATFORM_VENDOR);
    PLATFORM_QUERY_CASE(CL_PLATFORM_EXTENSIONS);
#ifdef CL_VERSION_2_1
    PLATFORM_QUERY_CASE(CL_PLATFORM_HOST_TIMER_RESOLUTION);
#endif
#if defined(CL_VERSION_3_0)
    PLATFORM_QUERY_CASE(CL_PLATFORM_NUMERIC_VERSION);
    PLATFORM_QUERY_CASE(CL_PLATFORM_EXTENSIONS_WITH_VERSION);
#endif
    default: {
      return handleUnhandledCase("platform query", query);
    } break;
  }
}
#undef PLATFORM_QUERY_CASE

std::string UCL::programBuildQueryToString(cl_program_build_info query) {
  // Helper Maco to casee the query using stringification.
#define PROGRAM_BUILD_QUERY_CASE(PROGRAM_BUILD_QUERY) \
  case PROGRAM_BUILD_QUERY: {                         \
    return #PROGRAM_BUILD_QUERY;                      \
  } break
  switch (query) {
    PROGRAM_BUILD_QUERY_CASE(CL_PROGRAM_BUILD_STATUS);
    PROGRAM_BUILD_QUERY_CASE(CL_PROGRAM_BUILD_OPTIONS);
    PROGRAM_BUILD_QUERY_CASE(CL_PROGRAM_BUILD_LOG);
#ifdef CL_VERSION_1_2
    PROGRAM_BUILD_QUERY_CASE(CL_PROGRAM_BINARY_TYPE);
#endif
#ifdef CL_VERSION_2_0
    PROGRAM_BUILD_QUERY_CASE(CL_PROGRAM_BUILD_GLOBAL_VARIABLE_TOTAL_SIZE);
#endif
    default: {
      return handleUnhandledCase("program build query", query);
    } break;
  }
}
#undef PROGRAM_BUILD_QUERY_CASE

std::string UCL::memObjectQueryToString(cl_mem_info query) {
  // Helper macro to case the query using stringification.
#define MEM_OBJECT_QUERY_CASE(MEM_OBJECT_QUERY) \
  case MEM_OBJECT_QUERY: {                      \
    return #MEM_OBJECT_QUERY;                   \
  } break
  switch (query) {
    default: {
      return handleUnhandledCase("memobject query", query);
    } break;
      MEM_OBJECT_QUERY_CASE(CL_MEM_TYPE);
      MEM_OBJECT_QUERY_CASE(CL_MEM_FLAGS);
      MEM_OBJECT_QUERY_CASE(CL_MEM_SIZE);
      MEM_OBJECT_QUERY_CASE(CL_MEM_HOST_PTR);
      MEM_OBJECT_QUERY_CASE(CL_MEM_MAP_COUNT);
      MEM_OBJECT_QUERY_CASE(CL_MEM_REFERENCE_COUNT);
      MEM_OBJECT_QUERY_CASE(CL_MEM_CONTEXT);
#ifdef CL_VERSION_1_1
      MEM_OBJECT_QUERY_CASE(CL_MEM_ASSOCIATED_MEMOBJECT);
      MEM_OBJECT_QUERY_CASE(CL_MEM_OFFSET);
#endif
#ifdef CL_VERSION_2_0
      MEM_OBJECT_QUERY_CASE(CL_MEM_USES_SVM_POINTER);
#endif
#if defined(CL_VERSION_3_0)
      MEM_OBJECT_QUERY_CASE(CL_MEM_PROPERTIES);
#endif
  }
}
#undef MEM_OBJECT_QUERY_CASE

std::string UCL::commandQueueQueryToString(cl_command_queue_info query) {
  // Helper macro to case the query using stringification.
#define COMMAND_QUEUE_QUERY_CASE(COMMAND_QUEUE_QUERY) \
  case COMMAND_QUEUE_QUERY: {                         \
    return #COMMAND_QUEUE_QUERY;                      \
  } break
  switch (query) {
    default: {
      return handleUnhandledCase("command queue query", query);
    } break;
      COMMAND_QUEUE_QUERY_CASE(CL_QUEUE_CONTEXT);
      COMMAND_QUEUE_QUERY_CASE(CL_QUEUE_DEVICE);
      COMMAND_QUEUE_QUERY_CASE(CL_QUEUE_REFERENCE_COUNT);
      COMMAND_QUEUE_QUERY_CASE(CL_QUEUE_PROPERTIES);
#ifdef CL_VERSION_2_0
      COMMAND_QUEUE_QUERY_CASE(CL_QUEUE_SIZE);
#endif
#ifdef CL_VERSION_2_1
      COMMAND_QUEUE_QUERY_CASE(CL_QUEUE_DEVICE_DEFAULT);
#endif
#if defined(CL_VERSION_3_0)
      COMMAND_QUEUE_QUERY_CASE(CL_QUEUE_PROPERTIES_ARRAY);
#endif
  }
}
#undef COMMAND_QUEUE_INFO_CASE

std::string UCL::profilingQueryToString(cl_profiling_info query) {
#define PROFILING_QUERY_CASE(PROFILING_QUERY) \
  case PROFILING_QUERY: {                     \
    return #PROFILING_QUERY;                  \
  } break
  switch (query) {
    default: {
      return handleUnhandledCase("profiling query", query);
    } break;
      PROFILING_QUERY_CASE(CL_PROFILING_COMMAND_QUEUED);
      PROFILING_QUERY_CASE(CL_PROFILING_COMMAND_SUBMIT);
      PROFILING_QUERY_CASE(CL_PROFILING_COMMAND_START);
      PROFILING_QUERY_CASE(CL_PROFILING_COMMAND_END);
#ifdef CL_VERSION_2_0
      PROFILING_QUERY_CASE(CL_PROFILING_COMMAND_COMPLETE);
#endif
  }
}
#undef PROFILING_QUERY_CASE

bool UCL::verifyOpenCLVersionString(const std::string &opencl_version_string) {
  const std::regex valid_opencl_version{R"(^OpenCL \d+\.\d+ .*$)"};
  return std::regex_match(opencl_version_string, valid_opencl_version);
}

bool UCL::verifyOpenCLCVersionString(
    const std::string &opencl_c_version_string) {
  const std::regex valid_version{R"(^OpenCL C \d+\.\d+ .*$)"};
  return std::regex_match(opencl_c_version_string, valid_version);
}

cargo::optional<std::pair<int, int>> UCL::parseOpenCLVersionString(
    const std::string &opencl_version_string) {
  if (!UCL::verifyOpenCLVersionString(opencl_version_string)) {
    return {};
  }
  const std::regex search_pattern{R"(^OpenCL (\d+)\.(\d+) .*$)"};
  std::smatch matches{};
  std::regex_search(opencl_version_string, matches, search_pattern);
  return std::make_pair(std::atoi(matches[1].str().c_str()),
                        std::atoi(matches[2].str().c_str()));
}
