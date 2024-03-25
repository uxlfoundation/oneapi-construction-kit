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
#include "Common.h"

class FeatureMacroTest : public ucl::CommandQueueTest,
                         public testing::WithParamInterface<cl_name_version> {
 protected:
  virtual void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ucl::CommandQueueTest::SetUp());
    // Skip for non OpenCL-3.0 implementations.
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
    // Requires a compiler to compile the kernel.
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
  }

  // This function takes the is_defined out parameter rather than returning a
  // bool so that it can take advantage of the gtest macros to fail whilst
  // avoiding the convoluted logic involved in returning an error and checking
  // it.
  void isFeatureDefinedBySpecializedQuery(const std::string &macro,
                                          bool &is_defined) {
    if ("__opencl_c_atomic_order_acq_rel" == macro) {
      cl_device_atomic_capabilities supported{};
      ASSERT_SUCCESS(clGetDeviceInfo(device,
                                     CL_DEVICE_ATOMIC_MEMORY_CAPABILITIES,
                                     sizeof(supported), &supported, nullptr));
      is_defined = !!(supported & CL_DEVICE_ATOMIC_ORDER_ACQ_REL);
    } else if ("__opencl_c_atomic_order_seq_cst" == macro) {
      cl_device_atomic_capabilities supported{};
      ASSERT_SUCCESS(clGetDeviceInfo(device,
                                     CL_DEVICE_ATOMIC_MEMORY_CAPABILITIES,
                                     sizeof(supported), &supported, nullptr));
      is_defined = !!(supported & CL_DEVICE_ATOMIC_ORDER_SEQ_CST);
    } else if ("__opencl_c_atomic_scope_device" == macro) {
      cl_device_atomic_capabilities supported{};
      ASSERT_SUCCESS(clGetDeviceInfo(device,
                                     CL_DEVICE_ATOMIC_MEMORY_CAPABILITIES,
                                     sizeof(supported), &supported, nullptr));
      is_defined = !!(supported & CL_DEVICE_ATOMIC_SCOPE_DEVICE);
    } else if ("__opencl_c_atomic_scope_all_devices" == macro) {
      cl_device_atomic_capabilities supported{};
      ASSERT_SUCCESS(clGetDeviceInfo(device,
                                     CL_DEVICE_ATOMIC_MEMORY_CAPABILITIES,
                                     sizeof(supported), &supported, nullptr));
      is_defined = !!(supported & CL_DEVICE_ATOMIC_SCOPE_ALL_DEVICES);
    } else if ("__opencl_c_program_scope_global_variables" == macro) {
      size_t max{};
      ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE,
                                     sizeof(max), &max, nullptr));
      is_defined = max != 0;
    } else if ("__opencl_c_device_enqueue" == macro) {
      cl_device_device_enqueue_capabilities capabilities{};
      ASSERT_SUCCESS(
          clGetDeviceInfo(device, CL_DEVICE_DEVICE_ENQUEUE_CAPABILITIES,
                          sizeof(capabilities), &capabilities, nullptr));
      is_defined = !!(capabilities & CL_DEVICE_QUEUE_SUPPORTED);
      return;
    } else if ("__opencl_c_generic_address_space" == macro) {
      cl_bool supported{};
      ASSERT_SUCCESS(clGetDeviceInfo(device,
                                     CL_DEVICE_GENERIC_ADDRESS_SPACE_SUPPORT,
                                     sizeof(supported), &supported, nullptr));
      is_defined = (CL_TRUE == supported);
      return;
    } else if ("__opencl_c_read_write_images" == macro) {
      cl_uint max{};
      ASSERT_SUCCESS(clGetDeviceInfo(device,
                                     CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS,
                                     sizeof(max), &max, nullptr));
      is_defined = max != 0;
      return;
    } else if ("__opencl_c_atomic_order_acq_rel" == macro) {
      cl_bool supported{};
      ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_ATOMIC_ORDER_ACQ_REL,
                                     sizeof(supported), &supported, nullptr));
      is_defined = (CL_TRUE == supported);
      return;
    } else if ("__opencl_c_pipes" == macro) {
      cl_bool supported{};
      ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PIPE_SUPPORT,
                                     sizeof(supported), &supported, nullptr));
      is_defined = (CL_TRUE == supported);
      return;
    } else if ("__opencl_c_subgroups" == macro) {
      cl_uint max{};
      ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_NUM_SUB_GROUPS,
                                     sizeof(max), &max, nullptr));
      is_defined = max > 0;
      return;
    } else if ("__opencl_c_work_group_collective_functions" == macro) {
      cl_bool supported{};
      ASSERT_SUCCESS(clGetDeviceInfo(
          device, CL_DEVICE_WORK_GROUP_COLLECTIVE_FUNCTIONS_SUPPORT,
          sizeof(supported), &supported, nullptr));
      is_defined = supported != CL_FALSE;
      return;
    } else if ("__opencl_c_3d_image_writes" == macro) {
      size_t extensions_size_in_bytes{};
      ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, 0, nullptr,
                                     &extensions_size_in_bytes));
      std::string extensions(extensions_size_in_bytes, '\0');
      ASSERT_SUCCESS(clGetDeviceInfo(
          device, CL_DEVICE_EXTENSIONS,
          sizeof(decltype(extensions)::value_type) * extensions.size(),
          extensions.data(), nullptr));
      is_defined =
          extensions.find("cl_khr_3d_image_writes") != std::string::npos;
      return;
    } else if ("__opencl_c_images" == macro) {
      // Support for the image built-in functions is optional. If a device
      // supports images then the value of the CL_DEVICE_IMAGE_SUPPORT device
      // query) is CL_TRUE and the OpenCL C 3.0 compiler must define the
      // __opencl_c_images feature macro.
      cl_bool image_support{};
      ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT,
                                     sizeof(image_support), &image_support,
                                     nullptr));
      is_defined = (CL_TRUE == image_support);
      return;
    } else if ("__opencl_c_fp64" == macro) {
      // The double scalar/vector type is an optional type that is supported if
      // the value of the CL_DEVICE_DOUBLE_FP_CONFIG device query is not zero.
      // If this is the case then an OpenCL C 3.0 compiler must also define the
      // __opencl_c_fp64 feature macro.
      cl_device_fp_config fp_config{};
      ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_DOUBLE_FP_CONFIG,
                                     sizeof(fp_config), &fp_config, nullptr));
      is_defined = (0 != fp_config);
      return;
    } else if ("__opencl_c_int64" == macro) {
      // The long, unsigned long and ulong scalar/vector types are optional
      // types for EMBEDDED profile devices that are supported if the value of
      // the CL_DEVICE_EXTENSIONS device query contains cles_khr_int64. An
      // OpenCL C 3.0 compiler must also define the __opencl_c_int64 feature
      // macro unconditionally for FULL profile devices, or for EMBEDDED profile
      // devices that support these types.
      size_t profile_size_in_bytes{};
      ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_PROFILE, 0, nullptr,
                                     &profile_size_in_bytes));
      std::string profile(profile_size_in_bytes, '\0');
      ASSERT_SUCCESS(clGetDeviceInfo(
          device, CL_DEVICE_PROFILE,
          sizeof(decltype(profile)::value_type) * profile.size(),
          profile.data(), nullptr));
      if (0 == std::strcmp(profile.c_str(), "FULL_PROFILE")) {
        is_defined = true;
        return;
      } else if (0 == std::strcmp(profile.c_str(), "EMBEDDED_PROFILE")) {
        size_t extensions_size_in_bytes{};
        ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, 0, nullptr,
                                       &extensions_size_in_bytes));
        std::string extensions(extensions_size_in_bytes, '\0');
        ASSERT_SUCCESS(clGetDeviceInfo(
            device, CL_DEVICE_EXTENSIONS,
            sizeof(decltype(extensions)::value_type) * extensions.size(),
            extensions.data(), nullptr));
        if (extensions.find("cles_khr_int64")) {
          is_defined = true;
          return;
        } else {
          is_defined = false;
          return;
        }
      } else {
        FAIL() << "unhandled device profile " << profile << '\n';
      }
    } else {
      FAIL() << "unhandled feature macro " << macro << '\n';
    }
  }

  void getDeviceOpenCLCFeatures(std::vector<cl_name_version> &name_versions) {
    size_t data_size_in_bytes{};
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_FEATURES, 0,
                                   nullptr, &data_size_in_bytes));
    std::vector<cl_name_version> device_opencl_c_features(
        data_size_in_bytes / sizeof(cl_name_version), {0, {0}});
    EXPECT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_FEATURES,
                                   data_size_in_bytes,
                                   device_opencl_c_features.data(), nullptr));
    name_versions = device_opencl_c_features;
  }

  bool isFeatureInOpenCLCFeaturesQuery(
      const cl_name_version &queried_name_version) {
    // Check whether the given macro is defined in the list returned from
    // CL_DEVICE_OPENCL_C_FEATURES.
    std::vector<cl_name_version> name_versions;
    getDeviceOpenCLCFeatures(name_versions);
    for (const auto &name_version : name_versions) {
      if (0 == std::strncmp(name_version.name, queried_name_version.name,
                            std ::strlen(name_version.name))) {
        // Check that the version is correct.
        EXPECT_EQ(name_version.version, queried_name_version.version);
        return true;
      }
    }
    return false;
  }

  void isFeatureDefinedByCompiler(const std::string &macro_name,
                                  bool &is_defined) {
    // Create two kernels, one that will compile and one that won't, this allows
    // us to be sure that the compile fail is due to the macro definition and
    // not something else.
    const std::string fail_on_defined = "#ifdef " + macro_name + "\n#error " +
                                        macro_name +
                                        " should not be defined.\n#endif";
    const char *fail_on_defined_kernel_source = fail_on_defined.c_str();
    const size_t fail_on_defined_kernel_length = fail_on_defined.length();

    const std::string fail_on_undefined = "#ifndef " + macro_name +
                                          "\n#error " + macro_name +
                                          " should be defined.\n#endif";
    const char *fail_on_undefined_kernel_source = fail_on_undefined.c_str();
    const size_t fail_on_undefined_kernel_length = fail_on_undefined.length();

    // Build kernels.
    cl_int error{};
    cl_program fail_on_defined_program =
        clCreateProgramWithSource(context, 1, &fail_on_defined_kernel_source,
                                  &fail_on_defined_kernel_length, &error);
    ASSERT_SUCCESS(error);
    cl_program fail_on_undefined_program =
        clCreateProgramWithSource(context, 1, &fail_on_undefined_kernel_source,
                                  &fail_on_undefined_kernel_length, &error);
    ASSERT_SUCCESS(error);

    // Try to compile them.
    const bool should_fail_if_defined =
        (CL_BUILD_PROGRAM_FAILURE == clBuildProgram(fail_on_defined_program, 1,
                                                    &device, "-cl-std=CL3.0",
                                                    nullptr, nullptr));
    const bool should_fail_if_undefined =
        (CL_SUCCESS == clBuildProgram(fail_on_undefined_program, 1, &device,
                                      "-cl-std=CL3.0", nullptr, nullptr));

    // Sanity check the results.
    if (should_fail_if_defined && should_fail_if_undefined) {
      is_defined = true;
    } else if (!should_fail_if_defined && !should_fail_if_undefined) {
      is_defined = false;
    } else {
      FAIL() << macro_name
             << " simultaneously defined and undefined by compiler!\n";
    }

    // Cleanup.
    if (fail_on_defined_program) {
      EXPECT_SUCCESS(clReleaseProgram(fail_on_defined_program));
    }
    if (fail_on_undefined_program) {
      EXPECT_SUCCESS(clReleaseProgram(fail_on_undefined_program));
    }
  }
};

TEST_P(FeatureMacroTest, CheckSpecializedQueries) {
  // Unpack parameters.
  const std::string macro_name = GetParam().name;

  // Get all the optional features supported by the device through the
  // specialized queries.
  bool is_defined_by_specialized_query{};
  isFeatureDefinedBySpecializedQuery(macro_name,
                                     is_defined_by_specialized_query);

  // Check whether the given macros is defined in the list
  bool is_feature_defined_by_compiler{};
  isFeatureDefinedByCompiler(macro_name, is_feature_defined_by_compiler);

  // Check that the feature is supported in the query if and only if it is
  // supported in the compiler.
  if (is_defined_by_specialized_query && !is_feature_defined_by_compiler) {
    FAIL() << macro_name
           << " defined by CL_DEVICE_OPENCL_C_FEATURES query but not in "
              "compiler feature macros\n";
  } else if (is_feature_defined_by_compiler &&
             !is_defined_by_specialized_query) {
    FAIL() << macro_name
           << " defined as a compiler feature macro but not by the device "
              "query CL_DEVICE_OPENCL_C_FEATURES\n";
  }
}

TEST_P(FeatureMacroTest, CLDeviceOpenCLCFeatures) {
  // Unpack parameters.
  const std::string macro_name = GetParam().name;

  // Get all the optional features supported by the device through the
  // CL_DEVICE_OPENCL_C_FEATURES.
  const auto is_feature_defined_by_device_query =
      isFeatureInOpenCLCFeaturesQuery(GetParam());

  // Check whether the given macros is defined in the list
  bool is_feature_defined_by_compiler{};
  isFeatureDefinedByCompiler(macro_name, is_feature_defined_by_compiler);

  // Check that the feature is supported in the query iff it is supported in the
  // compiler.
  if (is_feature_defined_by_device_query && !is_feature_defined_by_compiler) {
    FAIL() << macro_name
           << " defined by CL_DEVICE_OPENCL_C_FEATURES query but not in "
              "compiler feature macros\n";
  } else if (is_feature_defined_by_compiler &&
             !is_feature_defined_by_device_query) {
    FAIL() << macro_name
           << " defined as a compiler feature macro but not by the device "
              "query CL_DEVICE_OPENCL_C_FEATURES\n";
  }
}

INSTANTIATE_TEST_CASE_P(
    CheckMacros, FeatureMacroTest,
    ::testing::Values(
        cl_name_version{
            CL_MAKE_VERSION(3, 0, 0),
            "__opencl_c_images",
        },
        cl_name_version{CL_MAKE_VERSION(3, 0, 0), "__opencl_c_fp64"},
        cl_name_version{
            CL_MAKE_VERSION(3, 0, 0),
            "__opencl_c_int64",
        },
        cl_name_version{CL_MAKE_VERSION(3, 0, 0),
                        "__opencl_c_atomic_order_acq_rel"},
        cl_name_version{CL_MAKE_VERSION(3, 0, 0),
                        "__opencl_c_atomic_order_seq_cst"},
        cl_name_version{CL_MAKE_VERSION(3, 0, 0),
                        "__opencl_c_atomic_scope_device"},
        cl_name_version{CL_MAKE_VERSION(3, 0, 0), "__opencl_c_device_enqueue"},
        cl_name_version{CL_MAKE_VERSION(3, 0, 0),
                        "__opencl_c_atomic_scope_all_devices"},
        cl_name_version{CL_MAKE_VERSION(3, 0, 0), "__opencl_c_subgroups"},
        cl_name_version{CL_MAKE_VERSION(3, 0, 0),
                        "__opencl_c_work_group_collective_functions"},
        cl_name_version{CL_MAKE_VERSION(3, 0, 0),
                        "__opencl_c_generic_address_space"},
        cl_name_version{CL_MAKE_VERSION(3, 0, 0), "__opencl_c_pipes"},
        cl_name_version{CL_MAKE_VERSION(3, 0, 0),
                        "__opencl_c_read_write_images"},
        cl_name_version{CL_MAKE_VERSION(3, 0, 0), "__opencl_c_3d_image_writes"},
        cl_name_version{CL_MAKE_VERSION(3, 0, 0),
                        "__opencl_c_program_scope_global_variables"}),
    [](const ::testing::TestParamInfo<FeatureMacroTest::ParamType> &info) {
      std::string output_name = info.param.name;
      // Google test doesn't allow for underscores in test names.
      output_name.erase(
          std::remove(std::begin(output_name), std::end(output_name), '_'),
          std::end(output_name));
      return output_name;
    });
