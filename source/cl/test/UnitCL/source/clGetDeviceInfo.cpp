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

#include <cargo/dynamic_array.h>
#include <cargo/small_vector.h>
#include <cargo/string_algorithm.h>
#include <cargo/string_view.h>

#include <algorithm>
#include <limits>
#include <regex>
#include <tuple>

#include "Common.h"

namespace cl {
enum profile {
  FULL_PROFILE,
  EMBEDDED_PROFILE,
};
}  // namespace cl

class clGetDeviceInfoTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    auto profile_string = getDeviceProfile();
    if (profile_string == "FULL_PROFILE") {
      profile = cl::FULL_PROFILE;
    } else if (profile_string == "EMBEDDED_PROFILE") {
      profile = cl::EMBEDDED_PROFILE;
    } else {
      ASSERT_TRUE(false) << "Unknown OpenCL device profile string!";
    }
  }

  cl::profile profile;
};

TEST_F(clGetDeviceInfoTest, Default) {
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_AVAILABLE, 0, nullptr, nullptr));
}

TEST_F(clGetDeviceInfoTest, BadDevice) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_DEVICE,
      clGetDeviceInfo(nullptr, CL_DEVICE_AVAILABLE, 0, nullptr, nullptr));
}

TEST_F(clGetDeviceInfoTest, BadParam) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetDeviceInfo(device, 0, 0, nullptr, nullptr));
}

TEST_F(clGetDeviceInfoTest, BadSize) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_AVAILABLE, 0, nullptr, &size));
  UCL::Buffer<char> payload(size);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetDeviceInfo(device, CL_DEVICE_AVAILABLE, size - 1, payload, nullptr));
}

TEST_F(clGetDeviceInfoTest, BadSizeWithNullBuffer) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_AVAILABLE, 0, nullptr, &size));
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_AVAILABLE, size - 1, nullptr, nullptr));
}

TEST_F(clGetDeviceInfoTest, ADDRESS_BITS) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_ADDRESS_BITS, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_ADDRESS_BITS, size, &payload, nullptr));
  ASSERT_EQ(true, (32 == payload) || (64 == payload));
}

TEST_F(clGetDeviceInfoTest, AVAILABLE) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_AVAILABLE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_bool), size);
  cl_bool payload;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_AVAILABLE, size, &payload, nullptr));
  ASSERT_EQ(true, (CL_TRUE == payload) || (CL_FALSE == payload));
}

TEST_F(clGetDeviceInfoTest, BUILT_IN_KERNELS) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, 0, nullptr, &size));
  UCL::Buffer<char> payload(size);
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, size,
                                 payload, nullptr));
  ASSERT_TRUE(size ? size == strlen(payload) + 1 : true);

  auto name_list = cargo::split_all(
      cargo::string_view{static_cast<const char *>(&payload[0])}, ";");

  for (auto name_view : name_list) {
    std::string name(std::begin(name_view), std::end(name_view));

    std::regex name_pattern{R"(^[_a-zA-Z][_a-zA-Z0-9]*$)"};
    std::smatch result;
    ASSERT_TRUE(std::regex_search(name, result, name_pattern));

    std::regex keywords{
        R"(\b[(kernel|__kernel|global|__global|local|__local|constant|__constant|read_only|__read_only|write_only|__write_only|read_write|__read_write|auto|break|case|char|const|continue|default|do|double|else|enum|extern|float|for|goto|if|inline|int|long|register|restrict|return|short|signed|sizeof|static|struct|switch|typedef|union|unsigned|void|volatile|while|_Bool|_Complex|_Imaginary|_Pragma|asm|fortran)]\b)"};
    ASSERT_FALSE(std::regex_search(name, result, keywords));
  }
}

TEST_F(clGetDeviceInfoTest, COMPILER_AVAILABLE) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_COMPILER_AVAILABLE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_bool), size);
  cl_bool payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_COMPILER_AVAILABLE, size,
                                 &payload, nullptr));
}

TEST_F(clGetDeviceInfoTest, DOUBLE_FP_CONFIG) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_DOUBLE_FP_CONFIG, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_device_fp_config), size);
  cl_device_fp_config payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_DOUBLE_FP_CONFIG, size,
                                 &payload, nullptr));
  if (UCL::hasDeviceExtensionSupport(device, "cl_khr_fp64")) {
    // Device reports that it support double precision floating-point, check
    // for minimal capabilities that are required to be supported.
    EXPECT_TRUE(CL_FP_FMA & payload);
    EXPECT_TRUE(CL_FP_ROUND_TO_NEAREST & payload);
    EXPECT_TRUE(CL_FP_ROUND_TO_ZERO & payload);
    EXPECT_TRUE(CL_FP_ROUND_TO_INF & payload);
    EXPECT_TRUE(CL_FP_INF_NAN & payload);
    EXPECT_TRUE(CL_FP_DENORM & payload);
  }

  // Ensure that only known option flags are set.
  const cl_device_fp_config allLegalValuesMask =
      CL_FP_DENORM | CL_FP_INF_NAN | CL_FP_ROUND_TO_NEAREST |
      CL_FP_ROUND_TO_ZERO | CL_FP_ROUND_TO_INF | CL_FP_FMA | CL_FP_SOFT_FLOAT;
  ASSERT_FALSE(~allLegalValuesMask & payload) << "Non-spec-conform options.";
}

TEST_F(clGetDeviceInfoTest, ENDIAN_LITTLE) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_ENDIAN_LITTLE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_bool), size);
  cl_bool payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_ENDIAN_LITTLE, size,
                                 &payload, nullptr));
  ASSERT_EQ(true, (CL_TRUE == payload) || (CL_FALSE == payload));
}

TEST_F(clGetDeviceInfoTest, ERROR_CORRECTION_SUPPORT) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_ERROR_CORRECTION_SUPPORT, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_bool), size);
  cl_bool payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_ERROR_CORRECTION_SUPPORT,
                                 size, &payload, nullptr));
  ASSERT_EQ(true, (CL_TRUE == payload) || (CL_FALSE == payload));
}

TEST_F(clGetDeviceInfoTest, EXECUTION_CAPABILITIES) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_EXECUTION_CAPABILITIES, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_device_exec_capabilities), size);
  cl_device_exec_capabilities payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_EXECUTION_CAPABILITIES, size,
                                 &payload, nullptr));
  ASSERT_TRUE(CL_EXEC_KERNEL & payload);
  ASSERT_FALSE(~(CL_EXEC_KERNEL | CL_EXEC_NATIVE_KERNEL) & payload);
}

TEST_F(clGetDeviceInfoTest, EXTENSIONS) {
  if (UCL::hasSupportForOpenCL_C_1_1(device)) {
    // Check for spec required OpenCL 1.2 and OpenCL C 1.1 extensions.
    size_t size = 0;
    ASSERT_SUCCESS(
        clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, 0, nullptr, &size));
    UCL::Buffer<char> payload(size);
    ASSERT_SUCCESS(
        clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, size, payload, nullptr));

    ASSERT_EQ(size, strlen(payload) + 1);  // +1 for terminating zero.
    ASSERT_NE(nullptr, strstr(payload, "cl_khr_global_int32_base_atomics"));
    ASSERT_NE(nullptr, strstr(payload, "cl_khr_global_int32_extended_atomics"));
    ASSERT_NE(nullptr, strstr(payload, "cl_khr_local_int32_base_atomics"));
    ASSERT_NE(nullptr, strstr(payload, "cl_khr_local_int32_extended_atomics"));
    ASSERT_NE(nullptr, strstr(payload, "cl_khr_byte_addressable_store"));

    // Longs are optional via extension on embedded profile
    // but the extension string shouldn't appear in full profile.
    size_t profile_str_size = 0;
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PROFILE, 0, nullptr,
                                   &profile_str_size));

    UCL::Buffer<char> name(profile_str_size);
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PROFILE, profile_str_size,
                                   name, nullptr));
    ASSERT_EQ(profile_str_size, strlen(name) + 1);  // +1 for terminating zero.

    if (strcmp(name.data(), "FULL_PROFILE") == 0) {
      ASSERT_EQ(nullptr, strstr(payload, "cles_khr_int64"));
    } else if (strcmp(name.data(), "EMBEDDED_PROFILE") == 0) {
      cl_int address_bits = 0;
      size_t addr_size = 0;
      clGetDeviceInfo(device, CL_DEVICE_ADDRESS_BITS, 0, nullptr, &addr_size);
      clGetDeviceInfo(device, CL_DEVICE_ADDRESS_BITS, addr_size, &address_bits,
                      nullptr);

      // If 64 addressable bits then cles_khr_int64 is also supported
      // in embedded mode.
      if (address_bits == 64) {
        ASSERT_NE(nullptr, strstr(payload, "cles_khr_int64"));
      }

      cl_device_fp_config fp_config;
      size_t fp_conf_size = 0;
      clGetDeviceInfo(device, CL_DEVICE_DOUBLE_FP_CONFIG, 0, nullptr,
                      &fp_conf_size);
      clGetDeviceInfo(device, CL_DEVICE_DOUBLE_FP_CONFIG, fp_conf_size,
                      &fp_config, nullptr);

      // If doubles are supported then cles_khr_int64 is also supported
      // in embedded mode.
      if (fp_config > 0) {
        ASSERT_NE(nullptr, strstr(payload, "cles_khr_int64"));
      }
    } else {
      // Unknown profile - this shouldn't happen
      ASSERT_TRUE(false);
    }

    // Image2D writes are available by extension in embedded mode.
    if (strcmp(name.data(), "FULL_PROFILE") == 0) {
      ASSERT_EQ(nullptr, strstr(payload, "cles_khr_2d_image_array_writes"));
    } else if (strcmp(name.data(), "EMBEDDED_PROFILE") == 0) {
      cl_bool image_support;
      size_t image_size = 0;
      clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT, 0, nullptr, &image_size);
      clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT, image_size,
                      &image_support, nullptr);

      if (image_support == CL_TRUE) {
        ASSERT_NE(nullptr, strstr(payload, "cles_khr_2d_image_array_writes"));
      }
    } else {
      // Unknown profile - this shouldn't happen
      ASSERT_TRUE(false);
    }
  }
}

TEST_F(clGetDeviceInfoTest, GLOBAL_MEM_CACHE_SIZE) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_ulong), size);
  cl_ulong payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, size,
                                 &payload, nullptr));
}

TEST_F(clGetDeviceInfoTest, GLOBAL_MEM_CACHE_TYPE) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_device_mem_cache_type), size);
  cl_device_mem_cache_type payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, size,
                                 &payload, nullptr));
  ASSERT_EQ(true, (CL_NONE == payload) || (CL_READ_ONLY_CACHE == payload) ||
                      (CL_READ_WRITE_CACHE == payload));
}

TEST_F(clGetDeviceInfoTest, GLOBAL_MEM_CACHELINE_SIZE) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE,
                                 size, &payload, nullptr));
}

TEST_F(clGetDeviceInfoTest, GLOBAL_MEM_SIZE) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_ulong), size);
  cl_ulong payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, size,
                                 &payload, nullptr));
}

TEST_F(clGetDeviceInfoTest, HALF_FP_CONFIG) {
  size_t size;
  const cl_int status =
      clGetDeviceInfo(device, CL_DEVICE_HALF_FP_CONFIG, 0, nullptr, &size);

  if (!UCL::hasDeviceExtensionSupport(device, "cl_khr_fp16")) {
    // device doesn't support half config
    ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, status);
  } else {
    ASSERT_SUCCESS(status);
    ASSERT_EQ(sizeof(cl_device_fp_config), size);

    cl_device_fp_config payload;
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_HALF_FP_CONFIG, size,
                                   &payload, nullptr));

    // Minimum required half precision capabilities
    ASSERT_TRUE(payload &
                (CL_FP_ROUND_TO_ZERO | CL_FP_ROUND_TO_NEAREST | CL_FP_INF_NAN));

    // Mask out all valid bit to ensure there are no invalid bits.
    ASSERT_FALSE(payload &
                 ~(CL_FP_DENORM | CL_FP_INF_NAN | CL_FP_ROUND_TO_NEAREST |
                   CL_FP_ROUND_TO_ZERO | CL_FP_ROUND_TO_INF | CL_FP_FMA |
                   CL_FP_SOFT_FLOAT));
  }
}

TEST_F(clGetDeviceInfoTest, HOST_UNIFIED_MEMORY) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_HOST_UNIFIED_MEMORY, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_bool), size);
  cl_bool payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_HOST_UNIFIED_MEMORY, size,
                                 &payload, nullptr));
  ASSERT_EQ(true, (CL_TRUE == payload) || (CL_FALSE == payload));
}

TEST_F(clGetDeviceInfoTest, IMAGE_SUPPORT) {
  // This test needs to compile a kernel as part of the test
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_bool), size);
  cl_bool image_support;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT, size,
                                 &image_support, nullptr));

  // ensure that __IMAGE_SUPPORT__ is defined accordingly in the kernel language
  cl_int errorcode = !CL_SUCCESS;
  cl_context context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errorcode);
  EXPECT_TRUE(context);
  ASSERT_SUCCESS(errorcode);

  const char *src =
      "#ifdef __IMAGE_SUPPORT__\n"
      "void kernel foo() {}\n"
      "#endif\n";
  cl_program program =
      clCreateProgramWithSource(context, 1, &src, nullptr, &errorcode);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));

  cl_kernel kernel = clCreateKernel(program, "foo", &errorcode);

  if (image_support) {
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(errorcode);

    ASSERT_SUCCESS(clReleaseKernel(kernel));
  } else {
    EXPECT_FALSE(kernel);
    ASSERT_EQ_ERRCODE(CL_INVALID_KERNEL_NAME, errorcode);
  }

  ASSERT_SUCCESS(clReleaseProgram(program));
  ASSERT_SUCCESS(clReleaseContext(context));
}

TEST_F(clGetDeviceInfoTest, IMAGE2D_MAX_WIDTH) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_WIDTH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t image2d_max_width;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_WIDTH, size,
                                 &image2d_max_width, nullptr));

  if (UCL::hasImageSupport(device)) {
    switch (profile) {
      case cl::FULL_PROFILE: {
        ASSERT_LE(8192u, image2d_max_width);
      } break;
      case cl::EMBEDDED_PROFILE: {
        ASSERT_LE(2048u, image2d_max_width);
      } break;
    }
  } else {
    ASSERT_EQ(0u, image2d_max_width);
  }
}

TEST_F(clGetDeviceInfoTest, IMAGE2D_MAX_HEIGHT) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_HEIGHT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t image2d_max_height;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_HEIGHT, size,
                                 &image2d_max_height, nullptr));

  if (UCL::hasImageSupport(device)) {
    switch (profile) {
      case cl::FULL_PROFILE: {
        ASSERT_LE(8192u, image2d_max_height);
      } break;
      case cl::EMBEDDED_PROFILE: {
        ASSERT_LE(2048u, image2d_max_height);
      } break;
    }
  } else {
    ASSERT_EQ(0u, image2d_max_height);
  }
}

TEST_F(clGetDeviceInfoTest, IMAGE3D_MAX_WIDTH) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_WIDTH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t image3d_max_width;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_WIDTH, size,
                                 &image3d_max_width, nullptr));

  if (UCL::hasImageSupport(device)) {
    switch (profile) {
      case cl::FULL_PROFILE: {
        ASSERT_LE(2048u, image3d_max_width);
      } break;
      case cl::EMBEDDED_PROFILE: {
        ASSERT_LE(0u, image3d_max_width);
      } break;
    }
  } else {
    ASSERT_EQ(0u, image3d_max_width);
  }
}

TEST_F(clGetDeviceInfoTest, IMAGE3D_MAX_HEIGHT) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_HEIGHT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t image3d_max_height;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_HEIGHT, size,
                                 &image3d_max_height, nullptr));

  if (UCL::hasImageSupport(device)) {
    switch (profile) {
      case cl::FULL_PROFILE: {
        ASSERT_LE(2048u, image3d_max_height);
      } break;
      case cl::EMBEDDED_PROFILE: {
        ASSERT_LE(0u, image3d_max_height);
      } break;
    }
  } else {
    ASSERT_EQ(0u, image3d_max_height);
  }
}

TEST_F(clGetDeviceInfoTest, IMAGE3D_MAX_DEPTH) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_DEPTH, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t image3d_max_depth;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_DEPTH, size,
                                 &image3d_max_depth, nullptr));

  if (UCL::hasImageSupport(device)) {
    switch (profile) {
      case cl::FULL_PROFILE: {
        ASSERT_LE(2048u, image3d_max_depth);
      } break;
      case cl::EMBEDDED_PROFILE: {
        ASSERT_LE(0u, image3d_max_depth);
      } break;
    }
  } else {
    ASSERT_EQ(0u, image3d_max_depth);
  }
}

TEST_F(clGetDeviceInfoTest, IMAGE_MAX_BUFFER_SIZE) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE_MAX_BUFFER_SIZE, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t image_max_buffer_size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE_MAX_BUFFER_SIZE, size,
                                 &image_max_buffer_size, nullptr));

  if (UCL::hasImageSupport(device)) {
    switch (profile) {
      case cl::FULL_PROFILE: {
        ASSERT_LE(65523u, image_max_buffer_size);
      } break;
      case cl::EMBEDDED_PROFILE: {
        ASSERT_LE(2048u, image_max_buffer_size);
      } break;
    }
  } else {
    ASSERT_EQ(0u, image_max_buffer_size);
  }
}

TEST_F(clGetDeviceInfoTest, IMAGE_MAX_ARRAY_SIZE) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE_MAX_ARRAY_SIZE, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t image_max_array_size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE_MAX_ARRAY_SIZE, size,
                                 &image_max_array_size, nullptr));

  if (UCL::hasImageSupport(device)) {
    switch (profile) {
      case cl::FULL_PROFILE: {
        ASSERT_LE(2048u, image_max_array_size);
      } break;
      case cl::EMBEDDED_PROFILE: {
        ASSERT_LE(256u, image_max_array_size);
      } break;
    }
  } else {
    ASSERT_EQ(0u, image_max_array_size);
  }
}

TEST_F(clGetDeviceInfoTest, LINKER_AVAILABLE) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_LINKER_AVAILABLE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_bool), size);
  cl_bool payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_LINKER_AVAILABLE, size,
                                 &payload, nullptr));
  ASSERT_EQ(true, (CL_TRUE == payload) || (CL_FALSE == payload));
}

TEST_F(clGetDeviceInfoTest, LOCAL_MEM_SIZE) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_ulong), size);
  cl_ulong payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, size,
                                 &payload, nullptr));
  ASSERT_GE(payload, 32ul * 1024ul);
}

TEST_F(clGetDeviceInfoTest, LOCAL_MEM_TYPE) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_TYPE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_device_local_mem_type), size);
  cl_device_local_mem_type payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_TYPE, size,
                                 &payload, nullptr));
  ASSERT_EQ(true, (CL_LOCAL == payload) || (CL_GLOBAL == payload) ||
                      (CL_NONE == payload));
}

// CA-3108: Seems to report 0 on ARM.
#if defined(__arm__) || defined(__aarch64__)
TEST_F(clGetDeviceInfoTest, DISABLED_MAX_CLOCK_FREQUENCY) {
#else
TEST_F(clGetDeviceInfoTest, MAX_CLOCK_FREQUENCY) {
#endif
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, size,
                                 &payload, nullptr));
  ASSERT_GT(payload, 0u);
}

TEST_F(clGetDeviceInfoTest, MAX_COMPUTE_UNITS) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, size,
                                 &payload, nullptr));
  ASSERT_GE(payload, 1u);
}

TEST_F(clGetDeviceInfoTest, MAX_CONSTANT_ARGS) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_MAX_CONSTANT_ARGS, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint max_constant_args;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_CONSTANT_ARGS, size,
                                 &max_constant_args, nullptr));
  switch (profile) {
    case cl::FULL_PROFILE: {
      ASSERT_LE(8u, max_constant_args);
    } break;
    case cl::EMBEDDED_PROFILE: {
      ASSERT_LE(4u, max_constant_args);
    }
  }
}

TEST_F(clGetDeviceInfoTest, MAX_CONSTANT_BUFFER_SIZE) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_ulong), size);
  cl_ulong max_constant_buffer_size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE,
                                 size, &max_constant_buffer_size, nullptr));
  switch (profile) {
    case cl::FULL_PROFILE: {
      ASSERT_LE(64ul * 1024ul, max_constant_buffer_size);
    } break;
    case cl::EMBEDDED_PROFILE: {
      ASSERT_LE(1ul * 1024ul, max_constant_buffer_size);
    } break;
  }
}

TEST_F(clGetDeviceInfoTest, MAX_MEM_ALLOC_SIZE) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_ulong), size);
  cl_ulong max_mem_alloc_size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, size,
                                 &max_mem_alloc_size, nullptr));

  // The device can't report more available memory than what can be addressed on
  // the system.
  ASSERT_GE(std::numeric_limits<size_t>::max(), max_mem_alloc_size);

  cl_ulong memsize;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE,
                                 sizeof(cl_ulong), &memsize, nullptr));
  ASSERT_GE(memsize, 1u);  // There must be at least 1 byte of memory.

  // Max allocation can't exceed total amount of memory.
  ASSERT_LE(max_mem_alloc_size, memsize);

  switch (profile) {
    case cl::FULL_PROFILE:
      // The OpenCL 1.2 spec requires that CL_DEVICE_MAX_MEM_ALLOC_SIZE be at
      // least max(memsize/4, 128MiB).
      ASSERT_GE(max_mem_alloc_size,
                std::max<cl_ulong>(memsize / 4u, 128u * 1024u * 1024u));
      break;
    case cl::EMBEDDED_PROFILE:
      // Embedded profile changes the 128MiB to 1MiB.
      ASSERT_GE(max_mem_alloc_size,
                std::max<cl_ulong>(memsize / 4u, 1u * 1024u * 1024u));
      break;
  }
}

TEST_F(clGetDeviceInfoTest, MAX_PARAMETER_SIZE) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_MAX_PARAMETER_SIZE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t max_parameter_size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_PARAMETER_SIZE, size,
                                 &max_parameter_size, nullptr));

  switch (profile) {
    case cl::FULL_PROFILE: {
      ASSERT_LE(1024u, max_parameter_size);
    } break;
    case cl::EMBEDDED_PROFILE: {
      ASSERT_LE(256u, max_parameter_size);
    } break;
  }
}

TEST_F(clGetDeviceInfoTest, MAX_READ_IMAGE_ARGS) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_READ_IMAGE_ARGS, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);

  cl_uint max_read_image_args;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_READ_IMAGE_ARGS, size,
                                 &max_read_image_args, nullptr));

  if (UCL::hasImageSupport(device)) {
    switch (profile) {
      case cl::FULL_PROFILE:
        ASSERT_LE(128u, max_read_image_args);
        break;
      case cl::EMBEDDED_PROFILE:
        ASSERT_LE(8u, max_read_image_args);
        break;
    }
  } else {
    ASSERT_EQ(0u, max_read_image_args);
  }
}

TEST_F(clGetDeviceInfoTest, MAX_WRITE_IMAGE_ARGS) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WRITE_IMAGE_ARGS, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint max_write_image_args;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WRITE_IMAGE_ARGS, size,
                                 &max_write_image_args, nullptr));

  if (UCL::hasImageSupport(device)) {
    switch (profile) {
      case cl::FULL_PROFILE: {
        ASSERT_LE(8u, max_write_image_args);
      } break;
      case cl::EMBEDDED_PROFILE: {
        ASSERT_LE(1u, max_write_image_args);
      } break;
    }
  } else {
    ASSERT_EQ(0u, max_write_image_args);
  }
}

TEST_F(clGetDeviceInfoTest, MAX_SAMPLERS) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_MAX_SAMPLERS, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint max_samplers;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_SAMPLERS, size,
                                 &max_samplers, nullptr));

  if (UCL::hasImageSupport(device)) {
    switch (profile) {
      case cl::FULL_PROFILE: {
        ASSERT_LE(16u, max_samplers);
      } break;
      case cl::EMBEDDED_PROFILE: {
        ASSERT_LE(8u, max_samplers);
      } break;
    }
  } else {
    ASSERT_EQ(0u, max_samplers);
  }
}

TEST_F(clGetDeviceInfoTest, MAX_WORK_GROUP_SIZE) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, size,
                                 &payload, nullptr));
  ASSERT_GE(payload, 1u);
}

TEST_F(clGetDeviceInfoTest, MAX_WORK_ITEM_DIMENSIONS) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
                                 size, &payload, nullptr));
  ASSERT_GE(payload, 3u);
}

TEST_F(clGetDeviceInfoTest, MAX_WORK_ITEM_SIZES) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint numDimensions;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
                                 size, &numDimensions, nullptr));
  ASSERT_GE(numDimensions, 3u);

  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, 0,
                                 nullptr, &size));
  ASSERT_EQ(numDimensions * sizeof(size_t), size);
  UCL::Buffer<size_t> payload(numDimensions);
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, size,
                                 payload, nullptr));
  for (cl_uint i = 0; i < numDimensions; i++) {
    ASSERT_GE(payload[i], 1u);
  }
}

TEST_F(clGetDeviceInfoTest, MEM_BASE_ADDR_ALIGN) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MEM_BASE_ADDR_ALIGN, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MEM_BASE_ADDR_ALIGN, size,
                                 &payload, nullptr));
  ASSERT_GE(payload, sizeof(cl_ulong16));
}

TEST_F(clGetDeviceInfoTest, MIN_DATA_TYPE_ALIGN_SIZE) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE,
                                 size, &payload, nullptr));
}

TEST_F(clGetDeviceInfoTest, NAME) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr, &size));
  ASSERT_GE(size, 0u);
  UCL::Buffer<char> payload(size);
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_NAME, size, payload, nullptr));
}

TEST_F(clGetDeviceInfoTest, NATIVE_VECTOR_WIDTH_CHAR) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR,
                                 size, &payload, nullptr));
  ASSERT_NE(0u, payload);
}

TEST_F(clGetDeviceInfoTest, NATIVE_VECTOR_WIDTH_SHORT) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT,
                                 size, &payload, nullptr));
  ASSERT_NE(0u, payload);
}

TEST_F(clGetDeviceInfoTest, NATIVE_VECTOR_WIDTH_INT) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_INT,
                                 size, &payload, nullptr));
  ASSERT_NE(0u, payload);
}

TEST_F(clGetDeviceInfoTest, NATIVE_VECTOR_WIDTH_LONG) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG,
                                 size, &payload, nullptr));
  ASSERT_NE(0u, payload);
}

TEST_F(clGetDeviceInfoTest, NATIVE_VECTOR_WIDTH_FLOAT) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT,
                                 size, &payload, nullptr));
  ASSERT_NE(0u, payload);
}

TEST_F(clGetDeviceInfoTest, NATIVE_VECTOR_WIDTH_DOUBLE) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE,
                                 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE,
                                 size, &payload, nullptr));

  if (UCL::hasDoubleSupport(device)) {
    ASSERT_NE(0u, payload);
  } else {
    ASSERT_EQ(0u, payload);
  }
}

TEST_F(clGetDeviceInfoTest, NATIVE_VECTOR_WIDTH_HALF) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF,
                                 size, &payload, nullptr));

  if (UCL::hasHalfSupport(device)) {
    ASSERT_NE(0u, payload);
  } else {
    ASSERT_EQ(0u, payload);
  }
}

TEST_F(clGetDeviceInfoTest, OPENCL_C_VERSION) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_VERSION, 0, nullptr, &size));
  UCL::Buffer<char> payload(size);
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_VERSION, size,
                                 payload, nullptr));
  ASSERT_TRUE(size ? size == strlen(payload) + 1 : true);
  // Redmine #5127: Check use str functions to check that the version string
  // returned is not malformed
}

TEST_F(clGetDeviceInfoTest, PARENT_DEVICE) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_PARENT_DEVICE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_device_id), size);
  cl_device_id payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PARENT_DEVICE, size,
                                 &payload, nullptr));
  ASSERT_EQ(0, payload);  // assume we are a parent device
}

TEST_F(clGetDeviceInfoTest, PARTITION_MAX_SUB_DEVICES) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint maxComputeUnits;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, size,
                                 &maxComputeUnits, nullptr));

  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PARTITION_MAX_SUB_DEVICES, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PARTITION_MAX_SUB_DEVICES,
                                 size, &payload, nullptr));
  ASSERT_LE(payload, maxComputeUnits);
}

TEST_F(clGetDeviceInfoTest, PARTITION_PROPERTIES) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PARTITION_PROPERTIES, 0,
                                 nullptr, &size));
  UCL::Buffer<cl_device_partition_property> payload(
      size / sizeof(cl_device_partition_property));
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PARTITION_PROPERTIES, size,
                                 payload, nullptr));

  if (0 == payload[0]) {
    // we don't support any partition types
    ASSERT_EQ(1u, payload.size());
  } else {
    for (unsigned int i = 0; i < payload.size(); i++) {
      ASSERT_TRUE((CL_DEVICE_PARTITION_EQUALLY == payload[i]) ||
                  (CL_DEVICE_PARTITION_BY_COUNTS == payload[i]) ||
                  (CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN == payload[i]))
          << "CL_DEVICE_PARTITION_PROPERTIES returned value '0x" << std::hex
          << payload[i] << "'!";
    }
  }
}

TEST_F(clGetDeviceInfoTest, PARTITION_AFFINITY_DOMAIN) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PARTITION_AFFINITY_DOMAIN, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_device_affinity_domain), size);
  cl_device_affinity_domain payload;

  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PARTITION_AFFINITY_DOMAIN,
                                 size, &payload, nullptr));
  ASSERT_EQ(0u, payload);  // assume we don't support any partition types
}

TEST_F(clGetDeviceInfoTest, PARTITION_TYPE) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_PARTITION_TYPE, 0, nullptr, &size));
  if (0 == size) {
    // implementation is choosing the case 'may ... return a
    // param_value_size_ret of 0'
  } else {
    UCL::Buffer<cl_device_partition_property> payload(
        size / sizeof(cl_device_partition_property));
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PARTITION_TYPE, size,
                                   payload, nullptr));
    ASSERT_EQ(1u, payload.size());
    ASSERT_EQ(0u, payload[0]);  // we are not a sub-device
  }
}

TEST_F(clGetDeviceInfoTest, PLATFORM) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_PLATFORM, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_platform_id), size);
  cl_platform_id payload;

  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_PLATFORM, size, &payload, nullptr));
  ASSERT_EQ(platform, payload);  // assume we don't support any partition types
}

TEST_F(clGetDeviceInfoTest, PREFERRED_VECTOR_WIDTH_CHAR) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR,
                                 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR,
                                 size, &payload, nullptr));
  ASSERT_NE(0u, payload);
}

TEST_F(clGetDeviceInfoTest, PREFERRED_VECTOR_WIDTH_SHORT) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT,
                                 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT,
                                 size, &payload, nullptr));
  ASSERT_NE(0u, payload);
}

TEST_F(clGetDeviceInfoTest, PREFERRED_VECTOR_WIDTH_INT) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT,
                                 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT,
                                 size, &payload, nullptr));
  ASSERT_NE(0u, payload);
}

TEST_F(clGetDeviceInfoTest, PREFERRED_VECTOR_WIDTH_LONG) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG,
                                 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG,
                                 size, &payload, nullptr));
  ASSERT_NE(0u, payload);
}

TEST_F(clGetDeviceInfoTest, PREFERRED_VECTOR_WIDTH_FLOAT) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT,
                                 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT,
                                 size, &payload, nullptr));
  ASSERT_NE(0u, payload);
}

TEST_F(clGetDeviceInfoTest, PREFERRED_VECTOR_WIDTH_DOUBLE) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(
      device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device,
                                 CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, size,
                                 &payload, nullptr));

  if (UCL::hasDoubleSupport(device)) {
    ASSERT_NE(0u, payload);
  } else {
    ASSERT_EQ(0u, payload);
  }
}

TEST_F(clGetDeviceInfoTest, PREFERRED_VECTOR_WIDTH_HALF) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF,
                                 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF,
                                 size, &payload, nullptr));

  if (UCL::hasHalfSupport(device)) {
    ASSERT_NE(0u, payload);
  } else {
    ASSERT_EQ(0u, payload);
  }
}

TEST_F(clGetDeviceInfoTest, PRINTF_BUFFER_SIZE) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_PRINTF_BUFFER_SIZE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t printf_buffer_size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PRINTF_BUFFER_SIZE, size,
                                 &printf_buffer_size, nullptr));
  switch (profile) {
    case cl::FULL_PROFILE: {
      ASSERT_LE(1024u * 1024u, printf_buffer_size);
    } break;
    case cl::EMBEDDED_PROFILE: {
      ASSERT_LE(1024u, printf_buffer_size);
    } break;
  }
}

TEST_F(clGetDeviceInfoTest, PREFERRED_INTEROP_USER_SYNC) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PREFERRED_INTEROP_USER_SYNC,
                                 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_bool), size);
  cl_bool payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PREFERRED_INTEROP_USER_SYNC,
                                 size, &payload, nullptr));
  ASSERT_EQ(true, (CL_TRUE == payload) || (CL_FALSE == payload));
}

TEST_F(clGetDeviceInfoTest, PROFILE) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PROFILE, 0, nullptr, &size));
  std::string profile(size - 1,  // std::string allocates the null terminator
                      '\0');     // if not removed == fails due to wrong size
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_PROFILE, size, &profile[0], nullptr));
  ASSERT_TRUE(size ? size == strlen(profile.data()) + 1 : true);
  ASSERT_TRUE(profile == "FULL_PROFILE" || profile == "EMBEDDED_PROFILE");
}

TEST_F(clGetDeviceInfoTest, PROFILING_TIMER_RESOLUTION) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PROFILING_TIMER_RESOLUTION,
                                 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PROFILING_TIMER_RESOLUTION,
                                 size, &payload, nullptr));
}

TEST_F(clGetDeviceInfoTest, QUEUE_PROPERTIES) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_QUEUE_PROPERTIES, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_command_queue_properties), size);
  cl_command_queue_properties payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_QUEUE_PROPERTIES, size,
                                 &payload, nullptr));

  cl_command_queue_properties expect = CL_QUEUE_PROFILING_ENABLE;
  EXPECT_EQ(expect, CL_QUEUE_PROFILING_ENABLE & payload);
  payload &= ~CL_QUEUE_PROFILING_ENABLE;
  ASSERT_TRUE((0 == payload) ||
              (CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE == payload));
}

#if defined(CL_VERSION_3_0)
TEST_F(clGetDeviceInfoTest, QUEUE_ON_HOST_PROPERTIES) {
  // CL_DEVICE_QUEUE_ON_HOST_PROPERTIES has the same numeric value as
  // CL_DEVICE_QUEUE_PROPERTIES and is semantically identical.
  // CL_DEVICE_QUEUE_PROPERTIES is deprecated by version 2.0.
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_QUEUE_ON_HOST_PROPERTIES, 0,
                                 nullptr, &size));
  ASSERT_EQ(sizeof(cl_command_queue_properties), size);
  cl_command_queue_properties payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_QUEUE_ON_HOST_PROPERTIES,
                                 size, &payload, nullptr));

  cl_command_queue_properties expect = CL_QUEUE_PROFILING_ENABLE;
  EXPECT_EQ(expect, CL_QUEUE_PROFILING_ENABLE & payload);
  payload &= ~CL_QUEUE_PROFILING_ENABLE;
  ASSERT_TRUE((0 == payload) ||
              (CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE == payload));
}
#endif

TEST_F(clGetDeviceInfoTest, REFERENCE_COUNT) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_REFERENCE_COUNT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_REFERENCE_COUNT, size,
                                 &payload, nullptr));
  ASSERT_EQ(1u, payload);
}

TEST_F(clGetDeviceInfoTest, SINGLE_FP_CONFIG) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_SINGLE_FP_CONFIG, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_device_fp_config), size);
  cl_device_fp_config single_fp_config;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_SINGLE_FP_CONFIG, size,
                                 &single_fp_config, nullptr));

  switch (profile) {
    case cl::FULL_PROFILE: {
      ASSERT_TRUE(single_fp_config & (CL_FP_ROUND_TO_NEAREST | CL_FP_INF_NAN));
    } break;
    case cl::EMBEDDED_PROFILE: {
      ASSERT_TRUE(single_fp_config & CL_FP_ROUND_TO_ZERO ||
                  single_fp_config & CL_FP_ROUND_TO_NEAREST);
    } break;
  }

  // Mask out all valid bit to ensure there are no invalid bits.
  ASSERT_FALSE(single_fp_config &
               ~(CL_FP_DENORM | CL_FP_INF_NAN | CL_FP_ROUND_TO_NEAREST |
                 CL_FP_ROUND_TO_ZERO | CL_FP_ROUND_TO_INF | CL_FP_FMA |
                 CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT | CL_FP_SOFT_FLOAT));
}

TEST_F(clGetDeviceInfoTest, TYPE) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_TYPE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_device_type), size);
  cl_device_type payload;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_TYPE, size, &payload, nullptr));

  bool oneOf = false;

  oneOf |= (CL_DEVICE_TYPE_CPU == (CL_DEVICE_TYPE_CPU & payload));
  payload &= ~CL_DEVICE_TYPE_CPU;
  oneOf |= (CL_DEVICE_TYPE_GPU == (CL_DEVICE_TYPE_GPU & payload));
  payload &= ~CL_DEVICE_TYPE_GPU;
  oneOf |=
      (CL_DEVICE_TYPE_ACCELERATOR == (CL_DEVICE_TYPE_ACCELERATOR & payload));
  payload &= ~CL_DEVICE_TYPE_ACCELERATOR;
  oneOf |= (CL_DEVICE_TYPE_DEFAULT == (CL_DEVICE_TYPE_DEFAULT & payload));
  payload &= ~CL_DEVICE_TYPE_DEFAULT;
  oneOf |= (CL_DEVICE_TYPE_CUSTOM == (CL_DEVICE_TYPE_CUSTOM & payload));
  payload &= ~CL_DEVICE_TYPE_CUSTOM;

  EXPECT_TRUE(oneOf);
  ASSERT_EQ(0ul, payload);
}

static cl_device_type single_devices[] = {
    CL_DEVICE_TYPE_DEFAULT, CL_DEVICE_TYPE_CPU, CL_DEVICE_TYPE_GPU,
    CL_DEVICE_TYPE_ACCELERATOR, CL_DEVICE_TYPE_CUSTOM};

TEST_F(clGetDeviceInfoTest, UNSPECIFIED_SINGLE) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_TYPE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_device_type), size);
  cl_device_type payload;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_TYPE, size, &payload, nullptr));

  int num_types = 0;
  for (auto type : single_devices) {
    if (payload & type) {
      num_types++;
    }
  }

  ASSERT_EQ(num_types, 1);
}

TEST_F(clGetDeviceInfoTest, VENDOR) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_VENDOR, 0, nullptr, &size));
  ASSERT_GE(size, 0u);
  UCL::Buffer<char> payload(size);
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_VENDOR, size, payload, nullptr));
  ASSERT_TRUE(size ? size == strlen(payload) + 1 : true);
}

TEST_F(clGetDeviceInfoTest, VENDOR_ID) {
  size_t size;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_VENDOR_ID, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint payload;
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_VENDOR_ID, size, &payload, nullptr));
}

TEST_F(clGetDeviceInfoTest, VERSION) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_VERSION, 0, nullptr, &size));
  ASSERT_GE(size, 0u);
  UCL::Buffer<char> payload(size);
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_VERSION, size, payload, nullptr));
  ASSERT_TRUE(size ? size == strlen(payload) + 1 : true);
  // Redmine #5127: Check check that the string is formed correctly
}

TEST_F(clGetDeviceInfoTest, DRIVER_VERSION) {
  size_t size;
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DRIVER_VERSION, 0, nullptr, &size));
  ASSERT_GE(size, 0u);
  UCL::Buffer<char> payload(size);
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DRIVER_VERSION, size, payload, nullptr));
  ASSERT_TRUE(size ? size == strlen(payload) + 1 : true);
  // Redmine #5127: Check check that the string is formed correctly
}

TEST_F(clGetDeviceInfoTest, VerifyDeviceVersion) {
  size_t version_string_size{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_VERSION, 0, nullptr,
                                 &version_string_size));
  std::string version_string(version_string_size, '\0');
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_VERSION,
                                 version_string.size(), &version_string[0],
                                 nullptr));
  ASSERT_TRUE(UCL::verifyOpenCLVersionString(version_string))
      << "Malformed device OpenCL version, must be of form "
         "\"OpenCL<space><major_version.minor_version>\"";
}

TEST_F(clGetDeviceInfoTest, VerifyDeviceOpenCLCVersion) {
  size_t version_string_size{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_VERSION, 0, nullptr,
                                 &version_string_size));
  std::string version_string(version_string_size, '\0');
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_VERSION,
                                 version_string.size(), &version_string[0],
                                 nullptr));
  ASSERT_TRUE(UCL::verifyOpenCLCVersionString(version_string))
      << "Malformed device OpenCL C version, must be of form "
         "\"OpenCL<space>C<space><major_version.minor_version>\"";
}

#if defined(CL_VERSION_3_0)
class clGetDeviceInfoTestScalarQueryOpenCL30
    : public clGetDeviceInfoTest,
      public ::testing::WithParamInterface<std::tuple<size_t, int>> {
 protected:
  void SetUp() override {
    clGetDeviceInfoTest::SetUp();
    // Skip for non OpenCL-3.0 implementations.
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
  }
};

TEST_P(clGetDeviceInfoTestScalarQueryOpenCL30, CheckSizeQuerySucceeds) {
  // Get the enumeration value.
  auto query_enum_value = std::get<1>(GetParam());
  // Query for size of value.
  size_t size{};
  EXPECT_SUCCESS(clGetDeviceInfo(device, query_enum_value, 0, nullptr, &size));
}

TEST_P(clGetDeviceInfoTestScalarQueryOpenCL30, CheckSizeQueryIsCorrect) {
  // Get the enumeration value.
  auto query_enum_value = std::get<1>(GetParam());
  // Query for size of value.
  size_t size{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, query_enum_value, 0, nullptr, &size));
  // Get the correct size of the query.
  auto value_size_in_bytes = std::get<0>(GetParam());
  // Check the queried value is correct.
  EXPECT_EQ(size, value_size_in_bytes);
}

TEST_P(clGetDeviceInfoTestScalarQueryOpenCL30, CheckQuerySucceeds) {
  // Get the correct size of the query and the query itself.
  auto value_size_in_bytes = std::get<0>(GetParam());
  auto query_enum_value = std::get<1>(GetParam());
  // Query for the value.
  UCL::Buffer<char> value_buffer{value_size_in_bytes};
  EXPECT_SUCCESS(clGetDeviceInfo(device, query_enum_value, value_buffer.size(),
                                 value_buffer.data(), nullptr));
}

TEST_P(clGetDeviceInfoTestScalarQueryOpenCL30, CheckIncorrectSizeQueryFails) {
  // Get the correct size of the query and the query itself.
  auto value_size_in_bytes = std::get<0>(GetParam());
  auto query_enum_value = std::get<1>(GetParam());
  // Query for the value with buffer that is too small.
  UCL::Buffer<char> value_buffer{value_size_in_bytes};
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetDeviceInfo(device, query_enum_value, value_buffer.size() - 1,
                      value_buffer.data(), nullptr));
}

INSTANTIATE_TEST_CASE_P(
    DeviceQuery, clGetDeviceInfoTestScalarQueryOpenCL30,
    ::testing::Values(
        std::make_tuple(sizeof(cl_device_svm_capabilities),
                        CL_DEVICE_SVM_CAPABILITIES),
        std::make_tuple(sizeof(cl_device_atomic_capabilities),
                        CL_DEVICE_ATOMIC_MEMORY_CAPABILITIES),
        std::make_tuple(sizeof(cl_device_atomic_capabilities),
                        CL_DEVICE_ATOMIC_FENCE_CAPABILITIES),
        std::make_tuple(sizeof(cl_device_device_enqueue_capabilities),
                        CL_DEVICE_DEVICE_ENQUEUE_CAPABILITIES),
        std::make_tuple(sizeof(cl_command_queue_properties),
                        CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES),
        std::make_tuple(sizeof(cl_uint),
                        CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE),
        std::make_tuple(sizeof(cl_uint), CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE),
        std::make_tuple(sizeof(cl_uint), CL_DEVICE_MAX_ON_DEVICE_QUEUES),
        std::make_tuple(sizeof(cl_uint), CL_DEVICE_MAX_ON_DEVICE_EVENTS),
        std::make_tuple(sizeof(cl_bool), CL_DEVICE_PIPE_SUPPORT),
        std::make_tuple(sizeof(cl_uint), CL_DEVICE_MAX_PIPE_ARGS),
        std::make_tuple(sizeof(cl_uint),
                        CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS),
        std::make_tuple(sizeof(cl_uint), CL_DEVICE_PIPE_MAX_PACKET_SIZE),
        std::make_tuple(sizeof(size_t), CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE),
        std::make_tuple(sizeof(size_t),
                        CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE),
        std::make_tuple(sizeof(cl_bool),
                        CL_DEVICE_NON_UNIFORM_WORK_GROUP_SUPPORT),
        std::make_tuple(sizeof(cl_uint), CL_DEVICE_MAX_READ_IMAGE_ARGS),
        std::make_tuple(sizeof(cl_uint), CL_DEVICE_IMAGE_PITCH_ALIGNMENT),
        std::make_tuple(sizeof(cl_uint),
                        CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT),
        std::make_tuple(sizeof(cl_uint), CL_DEVICE_MAX_NUM_SUB_GROUPS),
        std::make_tuple(sizeof(cl_bool),
                        CL_DEVICE_SUB_GROUP_INDEPENDENT_FORWARD_PROGRESS),
        std::make_tuple(sizeof(cl_bool),
                        CL_DEVICE_WORK_GROUP_COLLECTIVE_FUNCTIONS_SUPPORT),
        std::make_tuple(sizeof(cl_bool),
                        CL_DEVICE_GENERIC_ADDRESS_SPACE_SUPPORT),
        std::make_tuple(sizeof(cl_version), CL_DEVICE_NUMERIC_VERSION),
        std::make_tuple(sizeof(cl_uint),
                        CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT),
        std::make_tuple(sizeof(cl_uint),
                        CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT),
        std::make_tuple(sizeof(cl_uint),
                        CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT),
        std::make_tuple(sizeof(size_t),
                        CL_DEVICE_PREFERRED_WORK_GROUP_SIZE_MULTIPLE)),
    [](const testing::TestParamInfo<
        clGetDeviceInfoTestScalarQueryOpenCL30::ParamType> &info) {
      return UCL::deviceQueryToString(std::get<1>(info.param));
    });

class clGetDeviceInfoTestVectorQueryOpenCL30
    : public clGetDeviceInfoTest,
      public ::testing::WithParamInterface<std::tuple<int>> {
 protected:
  void SetUp() override {
    clGetDeviceInfoTest::SetUp();
    // Skip for non OpenCL-3.0 implementations.
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
  }
};

TEST_P(clGetDeviceInfoTestVectorQueryOpenCL30, CheckSizeQuerySucceeds) {
  // Get the enumeration value.
  auto query_enum_value = std::get<0>(GetParam());
  // Query for size of value.
  size_t size{};
  EXPECT_SUCCESS(clGetDeviceInfo(device, query_enum_value, 0, nullptr, &size));
}

TEST_P(clGetDeviceInfoTestVectorQueryOpenCL30, CheckQuerySucceeds) {
  // The value returned by the query is a vector so we can't check the size
  // is correct.
  auto query_enum_value = std::get<0>(GetParam());
  // Query for size of value.
  size_t size{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, query_enum_value, 0, nullptr, &size));
  // Query for the value.
  if (size > 0) {
    UCL::Buffer<char> value_buffer{size};
    EXPECT_SUCCESS(clGetDeviceInfo(device, query_enum_value,
                                   value_buffer.size(), value_buffer.data(),
                                   nullptr));
  }
}

TEST_P(clGetDeviceInfoTestVectorQueryOpenCL30, CheckIncorrectSizeQueryFails) {
  // The value returned by the query is a vector so we can't check the size
  // is correct.
  auto query_enum_value = std::get<0>(GetParam());
  // Query for size of value.
  size_t size{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, query_enum_value, 0, nullptr, &size));
  // Query for the value with a size that is too small.
  if (size > 0) {
    UCL::Buffer<char> value_buffer{size};
    EXPECT_EQ_ERRCODE(
        CL_INVALID_VALUE,
        clGetDeviceInfo(device, query_enum_value, value_buffer.size() - 1,
                        value_buffer.data(), nullptr));
  }
}

INSTANTIATE_TEST_CASE_P(
    DeviceQuery, clGetDeviceInfoTestVectorQueryOpenCL30,
    testing::Values(CL_DEVICE_IL_VERSION, CL_DEVICE_ILS_WITH_VERSION,
                    CL_DEVICE_EXTENSIONS_WITH_VERSION,
                    CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION,
                    CL_DEVICE_OPENCL_C_ALL_VERSIONS,
                    CL_DEVICE_OPENCL_C_FEATURES,
                    CL_DEVICE_LATEST_CONFORMANCE_VERSION_PASSED),
    [](const testing::TestParamInfo<
        clGetDeviceInfoTestVectorQueryOpenCL30::ParamType> &info) {
      return UCL::deviceQueryToString(std::get<0>(info.param));
    });

TEST_F(clGetDeviceInfoTest, MinimumRequiredAtomicMemoryCapabilities) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Query for the value.
  cl_device_atomic_capabilities atomic_memory_capabilities{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_ATOMIC_MEMORY_CAPABILITIES,
                                 sizeof(atomic_memory_capabilities),
                                 &atomic_memory_capabilities, nullptr));

  // Check that the minimum capability is reported.
  constexpr auto minimum_required_capability{CL_DEVICE_ATOMIC_ORDER_RELAXED |
                                             CL_DEVICE_ATOMIC_SCOPE_WORK_GROUP};
  ASSERT_EQ(atomic_memory_capabilities & minimum_required_capability,
            minimum_required_capability);
}

TEST_F(clGetDeviceInfoTest, MinimumRequiredAtomicFenceCapabilities) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Query for the value.
  cl_device_atomic_capabilities atomic_fence_capabilities{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_ATOMIC_FENCE_CAPABILITIES,
                                 sizeof(atomic_fence_capabilities),
                                 &atomic_fence_capabilities, nullptr));

  // Check that the minimum capability is reported.
  constexpr auto minimum_required_capability{CL_DEVICE_ATOMIC_ORDER_RELAXED |
                                             CL_DEVICE_ATOMIC_ORDER_ACQ_REL |
                                             CL_DEVICE_ATOMIC_SCOPE_WORK_GROUP};
  ASSERT_EQ(atomic_fence_capabilities & minimum_required_capability,
            minimum_required_capability);
}

TEST_F(clGetDeviceInfoTest, DeviceSideEnqueueUnsupported) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Query for device enqueue support.
  cl_device_device_enqueue_capabilities device_enqueue_capabilities{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_DEVICE_ENQUEUE_CAPABILITIES,
                                 sizeof(device_enqueue_capabilities),
                                 &device_enqueue_capabilities, nullptr));

  // Check that if device enqueue is not supported then the relevant queries are
  // zero.
  if (device_enqueue_capabilities != 0) {
    // Query for properties.
    cl_command_queue_properties enqueue_on_device_properties{};
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES,
                                   sizeof(enqueue_on_device_properties),
                                   &enqueue_on_device_properties, nullptr));
    EXPECT_EQ(0, enqueue_on_device_properties);

    // Query for prefered queue size.
    cl_uint queue_on_device_prefered_queue_size{};
    ASSERT_SUCCESS(
        clGetDeviceInfo(device, CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE,
                        sizeof(queue_on_device_prefered_queue_size),
                        &queue_on_device_prefered_queue_size, nullptr));
    EXPECT_EQ(0, queue_on_device_prefered_queue_size);

    // Query for max queue size.
    cl_uint queue_on_device_max_size{};
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE,
                                   sizeof(queue_on_device_max_size),
                                   &queue_on_device_max_size, nullptr));
    EXPECT_EQ(0, queue_on_device_max_size);

    // Query for max number of queues.
    cl_uint max_on_device_queues{};
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_ON_DEVICE_QUEUES,
                                   sizeof(max_on_device_queues),
                                   &max_on_device_queues, nullptr));
    EXPECT_EQ(0, max_on_device_queues);

    // Query for max number of events.
    cl_uint max_on_device_events{};
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_ON_DEVICE_EVENTS,
                                   sizeof(max_on_device_events),
                                   &max_on_device_events, nullptr));
    EXPECT_EQ(0, max_on_device_events);
  }
}

TEST_F(clGetDeviceInfoTest, PipesUnsupported) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Query for pipe support.
  cl_bool pipe_support{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PIPE_SUPPORT,
                                 sizeof(pipe_support), &pipe_support, nullptr));

  // Check that if pipes are not supported then the relevant queries are
  // zero.
  if (pipe_support == CL_FALSE) {
    // Query for max pipe args.
    cl_uint max_pipe_args{};
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_PIPE_ARGS,
                                   sizeof(max_pipe_args), &max_pipe_args,
                                   nullptr));
    EXPECT_EQ(0, max_pipe_args);

    // Query for max active reservations.
    cl_uint pipe_max_active_reservations{};
    ASSERT_SUCCESS(clGetDeviceInfo(device,
                                   CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS,
                                   sizeof(pipe_max_active_reservations),
                                   &pipe_max_active_reservations, nullptr));
    EXPECT_EQ(0, pipe_max_active_reservations);

    // Query for max packet size.
    cl_uint pipe_max_packet_size{};
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PIPE_MAX_PACKET_SIZE,
                                   sizeof(pipe_max_packet_size),
                                   &pipe_max_packet_size, nullptr));
    EXPECT_EQ(0, pipe_max_packet_size);
  }
}

TEST_F(clGetDeviceInfoTest, ProgramScopeGlobalVariablesUnsupported) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Query for program scope global variable support.
  size_t max_global_variable_size{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE,
                                 sizeof(max_global_variable_size),
                                 &max_global_variable_size, nullptr));
  if (0 == max_global_variable_size) {
    // Query for prefered global variable total size.
    size_t global_variable_prefered_total_size{};
    ASSERT_SUCCESS(
        clGetDeviceInfo(device, CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE,
                        sizeof(global_variable_prefered_total_size),
                        &global_variable_prefered_total_size, nullptr));
    EXPECT_EQ(0, global_variable_prefered_total_size);
  }
}

TEST_F(clGetDeviceInfoTest, Creating2DImageFromBufferUnsupported) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Query for image pitch alignment.
  cl_uint image_pitch_alignment{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE_PITCH_ALIGNMENT,
                                 sizeof(image_pitch_alignment),
                                 &image_pitch_alignment, nullptr));

  // Query for image pitch alignment.
  cl_uint image_base_address_alignment{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT,
                                 sizeof(image_base_address_alignment),
                                 &image_base_address_alignment, nullptr));

  // Check that (image_pitch_alignment = 0) <=> (image_base_address_alignment =
  // 0).
  if (0 != image_pitch_alignment) {
    ASSERT_NE(image_base_address_alignment, 0);
  }
  if (0 != image_base_address_alignment) {
    ASSERT_NE(image_pitch_alignment, 0);
  }
}

TEST_F(clGetDeviceInfoTest, SubgroupsUnsupported) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Query for subgroup support.
  cl_uint max_num_sub_groups{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_NUM_SUB_GROUPS,
                                 sizeof(max_num_sub_groups),
                                 &max_num_sub_groups, nullptr));
  if (0 == max_num_sub_groups) {
    cl_bool sub_group_independent_forward_progress{};
    ASSERT_SUCCESS(clGetDeviceInfo(
        device, CL_DEVICE_SUB_GROUP_INDEPENDENT_FORWARD_PROGRESS,
        sizeof(sub_group_independent_forward_progress),
        &sub_group_independent_forward_progress, nullptr));
    EXPECT_EQ(CL_FALSE, sub_group_independent_forward_progress);
  }
}

TEST_F(clGetDeviceInfoTest, ValidateExtensionsWithVersion) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // First query for the CL_DEVICE_EXTENSIONS to get extensions listed
  // as a space separated string.
  size_t device_extensions_size{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, 0, nullptr,
                                 &device_extensions_size));
  UCL::Buffer<char> device_extensions_buffer(device_extensions_size);
  EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS,
                                 device_extensions_buffer.size(),
                                 device_extensions_buffer.data(), nullptr));

  // Now query for the CL_device_EXTENSIONS_WITH_VERSION to get extensions
  // as an array of cl_version objects.
  size_t device_extensions_with_version_size{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS_WITH_VERSION, 0,
                                 nullptr,
                                 &device_extensions_with_version_size));
  cargo::small_vector<cl_name_version_khr, 4> device_extensions_with_version{};
  auto error = device_extensions_with_version.assign(
      device_extensions_with_version_size /
          sizeof(decltype(device_extensions_with_version)::value_type),
      {});
  ASSERT_EQ(error, cargo::success) << "Error: out of memory";
  ASSERT_SUCCESS(clGetDeviceInfo(
      device, CL_DEVICE_EXTENSIONS_WITH_VERSION,
      sizeof(decltype(device_extensions_with_version)::value_type) *
          device_extensions_with_version.size(),
      device_extensions_with_version.data(), nullptr));

  // The list of extensions reported in the array must match the list in the
  // space separated string.

  // Much easier to work with a string view into the space separated string from
  // now on
  cargo::string_view device_extensions(device_extensions_buffer);
  // Construct an array of strings so we can easily traverse the space separated
  // list.
  auto split_extensions = cargo::split(device_extensions, " ");
  // Check that the lists have the same size for an early exit.
  ASSERT_EQ(split_extensions.size(), device_extensions_with_version.size());
  // Construct second array of strings from versioned extensions.
  std::vector<cargo::string_view> split_version_extensions{};
  for (const auto &ext : device_extensions_with_version) {
    split_version_extensions.push_back(ext.name);
  }

  // Sort the two lists and compare to ensure that up to ordering, they are the
  // same.
  std::sort(split_extensions.begin(), split_extensions.end());
  std::sort(split_version_extensions.begin(), split_version_extensions.end());
  ASSERT_EQ(split_extensions, split_version_extensions);
}

TEST_F(clGetDeviceInfoTest, VerifyNumericVersion) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }
  // Query for the string device version.
  size_t device_version_string_size{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_VERSION, 0, nullptr,
                                 &device_version_string_size));
  std::string device_version_string(device_version_string_size, '\0');
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_VERSION,
                                 device_version_string.size(),
                                 &device_version_string[0], nullptr));

  // Query for the value.
  cl_version numeric_version{};
  EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_NUMERIC_VERSION,
                                 sizeof(numeric_version), &numeric_version,
                                 nullptr));

  // Numeric version is required by spec. to match string version.
  int major_version{}, minor_version{};
  auto version = UCL::parseOpenCLVersionString(device_version_string);
  ASSERT_TRUE(version);
  std::tie(major_version, minor_version) = *version;
  EXPECT_EQ(CL_VERSION_MAJOR_KHR(numeric_version), major_version)
      << "Major version mismatch";
  EXPECT_EQ(CL_VERSION_MINOR_KHR(numeric_version), minor_version)
      << "Minor version mismatch";
  // Patch versions are not included in the device version.
}

TEST_F(clGetDeviceInfoTest, ValidateBuiltInKernelsWithVersion) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // First query for the CL_DEVICE_BUILT_IN_KERNELS to get extensions listed
  // as semi-colon space separated string.
  size_t device_built_in_kernels_size{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, 0, nullptr,
                                 &device_built_in_kernels_size));
  UCL::Buffer<char> device_built_in_kernels_buffer(
      device_built_in_kernels_size);
  EXPECT_SUCCESS(clGetDeviceInfo(
      device, CL_DEVICE_BUILT_IN_KERNELS, device_built_in_kernels_buffer.size(),
      device_built_in_kernels_buffer.data(), nullptr));

  // Now query for the CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION to get extensions
  // as an array of cl_version objects.
  size_t device_built_in_kernels_with_version_size{};
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION, 0,
                      nullptr, &device_built_in_kernels_with_version_size));

  // If there are no built-in kernels then CL_DEVICE_BUILT_IN_KERNELS will be an
  // empty string of one NUL character and
  // CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION will be a zero length list, the
  // reported lists must still match.
  if (0 == device_built_in_kernels_with_version_size) {
    EXPECT_EQ(device_built_in_kernels_size, 1);
    ASSERT_EQ(device_built_in_kernels_buffer[0], '\0');
    return;
  }

  cargo::small_vector<cl_name_version_khr, 4>
      device_built_in_kernels_with_version{};
  auto error = device_built_in_kernels_with_version.assign(
      device_built_in_kernels_with_version_size /
          sizeof(decltype(device_built_in_kernels_with_version)::value_type),
      {});
  ASSERT_EQ(error, cargo::success) << "Error: out of memory";
  ASSERT_SUCCESS(clGetDeviceInfo(
      device, CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION,
      sizeof(decltype(device_built_in_kernels_with_version)::value_type) *
          device_built_in_kernels_with_version.size(),
      device_built_in_kernels_with_version.data(), nullptr));

  // The list of extensions reported in the array must match the list in the
  // space separated string.

  // Much easier to work with a string view into the space separated string from
  // now on
  cargo::string_view device_built_in_kernels(device_built_in_kernels_buffer);
  // Construct an array of strings so we can easily traverse the space separated
  // list.
  auto split_built_in_kernels = cargo::split(device_built_in_kernels, ";");
  // Check that the lists have the same size for an early exit.
  ASSERT_EQ(split_built_in_kernels.size(),
            device_built_in_kernels_with_version.size());
  // Construct second array of strings from versioned extensions.
  std::vector<cargo::string_view> split_built_in_kernels_with_version{};
  for (const auto &ext : device_built_in_kernels_with_version) {
    split_built_in_kernels_with_version.push_back(ext.name);
  }

  // Sort the two lists and compare to ensure that up to ordering, they are the
  // same.
  std::sort(split_built_in_kernels_with_version.begin(),
            split_built_in_kernels_with_version.end());
  std::sort(split_built_in_kernels.begin(), split_built_in_kernels.end());
  ASSERT_EQ(split_built_in_kernels_with_version, split_built_in_kernels);
}

TEST_F(clGetDeviceInfoTest, ValidateILVersion) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Query for size of value.
  // Output is a string so can't check for correct size.
  size_t size{};
  ASSERT_SUCCESS(
      clGetDeviceInfo(device, CL_DEVICE_IL_VERSION, 0, nullptr, &size));

  // Query for the value.
  std::string device_il_version(size, '\0');
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IL_VERSION,
                                 device_il_version.length(),
                                 &device_il_version[0], nullptr));
  auto split_ils = cargo::split(device_il_version, " ");
  static const std::regex valid_il{R"(^[\w-]+_\d+\.\d+$)"};
  for (const auto &il : split_ils) {
    EXPECT_TRUE(std::regex_match(std::string(il.data(), il.length()), valid_il))
        << "Incorrectly formated IL reported by CL_DEVICE_IL_VERSION: "
        << device_il_version << std::endl;
  }
}

TEST_F(clGetDeviceInfoTest, ValidateILSWithVersion) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // First query for the CL_DEVICE_IL_VERSION to get extensions listed
  // as space separated string.
  size_t device_ils_version{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IL_VERSION, 0, nullptr,
                                 &device_ils_version));
  UCL::Buffer<char> device_il_version_buffer(device_ils_version);
  EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IL_VERSION,
                                 device_il_version_buffer.size(),
                                 device_il_version_buffer.data(), nullptr));

  // Now query for the CL_DEVICE_ILS_WITH_VERSION to get extensions
  // as an array of cl_version objects.
  size_t device_ils_with_version_size{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_ILS_WITH_VERSION, 0, nullptr,
                                 &device_ils_with_version_size));
  cargo::small_vector<cl_name_version_khr, 4> device_ils_with_version{};
  auto error = device_ils_with_version.assign(
      device_ils_with_version_size /
          sizeof(decltype(device_ils_with_version)::value_type),
      {});
  ASSERT_EQ(error, cargo::success) << "Error: out of memory";
  ASSERT_SUCCESS(clGetDeviceInfo(
      device, CL_DEVICE_ILS_WITH_VERSION,
      sizeof(decltype(device_ils_with_version)::value_type) *
          device_ils_with_version.size(),
      device_ils_with_version.size() > 0 ? device_ils_with_version.data()
                                         : nullptr,
      nullptr));

  // The list of extensions reported in the array must match the list in the
  // space separated string.

  // Much easier to work with a string view into the space separated string from
  // now on
  cargo::string_view device_il_version(device_il_version_buffer);
  // Construct an array of strings so we can easily traverse the space separated
  // list.
  auto split_device_il_version = cargo::split(device_il_version, " ");
  // Check that the lists have the same size for an early exit.
  ASSERT_EQ(split_device_il_version.size(), device_ils_with_version.size());
  // Check that every element in IL_VERSION is in ILS_WITH_VERSION.
  for (auto &il_version : split_device_il_version) {
    std::regex ils_version_regex{R"(([\w-]+)_(\d+)\.(\d+))"};
    std::smatch sm{};
    std::string ils_version{il_version.data(), il_version.size()};
    ASSERT_TRUE(std::regex_search(ils_version, sm, ils_version_regex));
    auto il_prefix = sm.str(1);
    auto major_version = sm.str(2);
    auto minor_version = sm.str(3);
    cl_name_version_khr name_version{};
    std::memcpy(name_version.name, il_prefix.data(), il_prefix.size());
    name_version.version = CL_MAKE_VERSION_KHR(
        std::atoi(major_version.c_str()), std::atoi(minor_version.c_str()), 0);
    ASSERT_NE(
        std::find_if(device_ils_with_version.begin(),
                     device_ils_with_version.end(),
                     [&name_version](cl_name_version_khr nv) {
                       return (std::strcmp(name_version.name, nv.name) == 0) &&
                              name_version.version == nv.version;
                     }),
        device_ils_with_version.end())
        << "Missing IL in CL_DEVICE_ILS_WITH_VERSION";
  }
}

TEST_F(clGetDeviceInfoTest, ValidateOpenCLCAllVersions) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }
  // For devices that do not support compilation from OpenCL C source, the
  // CL_DEVICE_OPENCL_C_ALL_VERSIONS query may return an empty array.
  if (!UCL::hasCompilerSupport(device)) {
    GTEST_SKIP();
  }
  // Query for the CL_DEVICE_OPENCL_C_ALL_VERSIONS array of name, version
  // structure.
  size_t opencl_c_all_versions_size{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_ALL_VERSIONS, 0,
                                 nullptr, &opencl_c_all_versions_size));
  cargo::dynamic_array<cl_name_version_khr> opencl_c_all_versions{};
  ASSERT_EQ(opencl_c_all_versions.alloc(
                opencl_c_all_versions_size /
                sizeof(decltype(opencl_c_all_versions)::value_type)),
            cargo::success);
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_ALL_VERSIONS,
                                 opencl_c_all_versions_size,
                                 opencl_c_all_versions.data(), nullptr));
  for (const auto &name_version : opencl_c_all_versions) {
    // In each returned description structure, the name filed is required to be
    // "OpenCL C".
    EXPECT_EQ(std::strcmp(name_version.name, "OpenCL C"), 0);
  }
  // The version returned by CL_DEVICE_OPENCL_C_VERSION is required to be
  // present in the list.
  size_t opencl_c_version_size{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_VERSION, 0, nullptr,
                                 &opencl_c_version_size));
  std::string opencl_c_version(opencl_c_version_size, '\0');
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_VERSION,
                                 opencl_c_version_size, &opencl_c_version[0],
                                 nullptr));
  // Extract the version.
  std::regex version_string{R"(^OpenCL C (\d+)\.(\d+).*$)"};
  std::smatch sm{};
  ASSERT_TRUE(std::regex_search(opencl_c_version, sm, version_string))
      << "Malformed OpenCL C version string";
  ASSERT_EQ(sm.size(), 3) << "Malformed OpenCL C version string";
  cl_version_khr extracted_version = CL_MAKE_VERSION_KHR(
      std::atoi(sm.str(1).c_str()), std::atoi(sm.str(2).c_str()), 0);
  // Check its contained in the array returned by
  // CL_DEVICE_OPENCL_C_ALL_VERSIONS.
  ASSERT_NE(
      std::find_if(std::begin(opencl_c_all_versions),
                   std::end(opencl_c_all_versions),
                   [&extracted_version](cl_name_version_khr name_version) {
                     return name_version.version == extracted_version;
                   }),
      std::end(opencl_c_all_versions));
}

TEST_F(clGetDeviceInfoTest, ValidateOpenCLCAllVersionsCompatibility) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }
  // For devices that do not support compilation from OpenCL C source, the
  // CL_DEVICE_OPENCL_C_ALL_VERSIONS query may return an empty array.
  if (!UCL::hasCompilerSupport(device)) {
    GTEST_SKIP();
  }

  size_t size{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_ALL_VERSIONS, 0,
                                 nullptr, &size));
  cargo::dynamic_array<cl_name_version_khr> opencl_c_all_versions{};
  ASSERT_EQ(opencl_c_all_versions.alloc(
                size / sizeof(decltype(opencl_c_all_versions)::value_type)),
            cargo::success);
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_ALL_VERSIONS, size,
                                 opencl_c_all_versions.data(), nullptr));
  for (const auto &name_version : opencl_c_all_versions) {
    switch (name_version.version) {
      default: {
        FAIL() << "Unhandled OpenCL C Version: " << name_version.version
               << std::endl;
      }
      case CL_MAKE_VERSION_KHR(3, 0, 0): {
        // Because OpenCL 3.0 is backwards compatible with OpenCL C 1.2,
        // support for at least OpenCL C 3.0 and OpenCL C 1.2 is required
        // for an OpenCL 3.0 device.
        EXPECT_NE(std::find_if(std::begin(opencl_c_all_versions),
                               std::end(opencl_c_all_versions),
                               [](cl_name_version_khr nv) {
                                 return nv.version ==
                                        CL_MAKE_VERSION_KHR(1, 2, 0);
                               }),
                  std::end(opencl_c_all_versions));
      } break;
      case CL_MAKE_VERSION_KHR(2, 2, 0):
        [[fallthrough]];
      case CL_MAKE_VERSION_KHR(2, 1, 0): {
        // Support for OpenCL C 2.0, OpenCL C 1.2, OpenCL C 1.1, and OpenCL
        // C 1.0 is required for an OpenCL 2.0, OpenCL 2.1, or OpenCL 2.2
        // device.
        EXPECT_NE(std::find_if(std::begin(opencl_c_all_versions),
                               std::end(opencl_c_all_versions),
                               [](cl_name_version_khr nv) {
                                 return nv.version ==
                                        CL_MAKE_VERSION_KHR(2, 0, 0);
                               }),
                  std::end(opencl_c_all_versions));
        EXPECT_NE(std::find_if(std::begin(opencl_c_all_versions),
                               std::end(opencl_c_all_versions),
                               [](cl_name_version_khr nv) {
                                 return nv.version ==
                                        CL_MAKE_VERSION_KHR(1, 2, 0);
                               }),
                  std::end(opencl_c_all_versions));
        EXPECT_NE(std::find_if(std::begin(opencl_c_all_versions),
                               std::end(opencl_c_all_versions),
                               [](cl_name_version_khr nv) {
                                 return nv.version ==
                                        CL_MAKE_VERSION_KHR(1, 1, 0);
                               }),
                  std::end(opencl_c_all_versions));
        EXPECT_NE(std::find_if(std::begin(opencl_c_all_versions),
                               std::end(opencl_c_all_versions),
                               [](cl_name_version_khr nv) {
                                 return nv.version ==
                                        CL_MAKE_VERSION_KHR(1, 0, 0);
                               }),
                  std::end(opencl_c_all_versions));
      } break;
      case CL_MAKE_VERSION_KHR(1, 2, 0): {
        // Support for OpenCL C 1.2, OpenCL C 1.1, and OpenCL C 1.0 is required
        // for an OpenCL 1.2 device.
        EXPECT_NE(std::find_if(std::begin(opencl_c_all_versions),
                               std::end(opencl_c_all_versions),
                               [](cl_name_version_khr nv) {
                                 return nv.version ==
                                        CL_MAKE_VERSION_KHR(1, 1, 0);
                               }),
                  std::end(opencl_c_all_versions));
        EXPECT_NE(std::find_if(std::begin(opencl_c_all_versions),
                               std::end(opencl_c_all_versions),
                               [](cl_name_version_khr nv) {
                                 return nv.version ==
                                        CL_MAKE_VERSION_KHR(1, 0, 0);
                               }),
                  std::end(opencl_c_all_versions));
      } break;
      case CL_MAKE_VERSION_KHR(1, 1, 0): {
        // Support for OpenCL C 1.1 and OpenCL C 1.0 is required for an
        // OpenCL 1.1 device.
        EXPECT_NE(std::find_if(std::begin(opencl_c_all_versions),
                               std::end(opencl_c_all_versions),
                               [](cl_name_version_khr nv) {
                                 return nv.version ==
                                        CL_MAKE_VERSION_KHR(1, 0, 0);
                               }),
                  std::end(opencl_c_all_versions));
      } break;
      case CL_MAKE_VERSION_KHR(1, 0, 0): {
        // Support for at least OpenCL C 1.0 is required for an OpenCL 1.0
        // device.
      } break;
    }
  }
}
#endif
