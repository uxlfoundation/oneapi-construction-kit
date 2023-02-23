// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#include <algorithm>
#include <limits>

#include "Common.h"
#include "ucl/checks.h"

using namespace ucl;

TEST_F(ContextTest, WorkGroupCollectiveFunctionsFeatureMacroTest) {
  // Work-group collectives are an optional 3.0 features.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Check whether we have a compiler to compile our OpenCL C.
  if (!UCL::hasCompilerSupport(device)) {
    GTEST_SKIP();
  }

  const char *feature_macro_defined = R"(
      #if !defined(__opencl_c_work_group_collective_functions)
      #error __opencl_c_work_group_collective_functions not defined
      #endif
    )";
  const auto feature_macro_defined_size = std::strlen(feature_macro_defined);

  const char *feature_macro_undefined = R"(
      #if defined(__opencl_c_work_group_collective_functions)
      #error __opencl_c_work_group_collective_functions is defined
      #endif
    )";
  const auto feature_macro_undefined_size =
      std::strlen(feature_macro_undefined);

  cl_bool supports_work_group_collectives = CL_FALSE;

  ASSERT_SUCCESS(clGetDeviceInfo(
      device, CL_DEVICE_WORK_GROUP_COLLECTIVE_FUNCTIONS_SUPPORT,
      sizeof(cl_bool), &supports_work_group_collectives, nullptr));

  cl_int error = CL_SUCCESS;
  const cl_program feature_macro_defined_program = clCreateProgramWithSource(
      context, 1, &feature_macro_defined, &feature_macro_defined_size, &error);
  ASSERT_SUCCESS(error);

  const cl_program feature_macro_undefined_program =
      clCreateProgramWithSource(context, 1, &feature_macro_undefined,
                                &feature_macro_undefined_size, &error);
  EXPECT_SUCCESS(error);

  const char *build_options = "-cl-std=CL3.0";
  if (supports_work_group_collectives) {
    EXPECT_SUCCESS(clBuildProgram(feature_macro_defined_program, 1, &device,
                                  build_options, nullptr, nullptr));
    EXPECT_EQ_ERRCODE(CL_BUILD_PROGRAM_FAILURE,
                      clBuildProgram(feature_macro_undefined_program, 1,
                                     &device, build_options, nullptr, nullptr));
  } else {
    EXPECT_SUCCESS(clBuildProgram(feature_macro_undefined_program, 1, &device,
                                  build_options, nullptr, nullptr));
    EXPECT_EQ_ERRCODE(CL_BUILD_PROGRAM_FAILURE,
                      clBuildProgram(feature_macro_defined_program, 1, &device,
                                     build_options, nullptr, nullptr));
  }

  EXPECT_SUCCESS(clReleaseProgram(feature_macro_defined_program));
  EXPECT_SUCCESS(clReleaseProgram(feature_macro_undefined_program));
}

struct WorkGroupCollectiveFunctionsTest : public kts::ucl::BaseExecution {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(kts::ucl::BaseExecution::SetUp());
    // Work-group collectives are a 3.0 feature.
    if (!UCL::isDeviceVersionAtLeast({3, 0}) ||
        !UCL::hasCompilerSupport(device)) {
      GTEST_SKIP();
    }

    // Some of these tests run small local sizes, which we don't vectorize.
    // This is too coarse-grained, as there are some NDRanges which we can
    // vectorize.
    fail_if_not_vectorized_ = false;

    cl_bool supports_work_group_collectives = CL_FALSE;
    ASSERT_SUCCESS(clGetDeviceInfo(
        device, CL_DEVICE_WORK_GROUP_COLLECTIVE_FUNCTIONS_SUPPORT,
        sizeof(cl_bool), &supports_work_group_collectives, nullptr));

    if (CL_FALSE == supports_work_group_collectives) {
      GTEST_SKIP();
    }

    AddBuildOption("-cl-std=CL3.0");
  }
};

static std::array<size_t, 3> globalLinearIdToGlobalId(size_t global_linear_id,
                                                      size_t global_size_x,
                                                      size_t global_size_y) {
  const auto x = global_linear_id % global_size_x;
  const auto y = ((global_linear_id - x) / global_size_x) % global_size_y;
  const auto z = (global_linear_id - x - global_size_x * y) /
                 (global_size_x * global_size_y);
  return {x, y, z};
}

static size_t globalIdToGlobalLinearId(std::array<size_t, 3> global_ids,
                                       size_t global_size_x,
                                       size_t global_size_y) {
  return global_ids[0] + global_size_x * global_ids[1] +
         global_size_x * global_size_y * global_ids[2];
}

struct ReductionRange {
  size_t x_start, x_end;
  size_t y_start, y_end;
  size_t z_start, z_end;
};

static ReductionRange getReductionRange(
    size_t global_linear_id, const std::array<size_t, 3> &global_sizes,
    const std::array<size_t, 3> &local_sizes) {
  const auto global_range = globalLinearIdToGlobalId(
      global_linear_id, global_sizes[0], global_sizes[1]);
  const auto x_start = (global_range[0] / local_sizes[0]) * local_sizes[0];
  const auto x_end = x_start + local_sizes[0];
  const auto y_start = (global_range[1] / local_sizes[1]) * local_sizes[1];
  const auto y_end = y_start + local_sizes[1];
  const auto z_start = (global_range[2] / local_sizes[2]) * local_sizes[2];
  const auto z_end = z_start + local_sizes[2];
  return {x_start, x_end, y_start, y_end, z_start, z_end};
}

struct ScanRange {
  size_t x_work_group_start, x_end, x_work_group_end;
  size_t y_work_group_start, y_end, y_work_group_end;
  size_t z_work_group_start, z_end;
};

static ScanRange getScanRange(size_t global_linear_id,
                              const std::array<size_t, 3> &global_sizes,
                              const std::array<size_t, 3> &local_sizes,
                              bool is_inclusive) {
  const auto global_range = globalLinearIdToGlobalId(
      global_linear_id, global_sizes[0], global_sizes[1]);
  const auto x_work_group_start =
      (global_range[0] / local_sizes[0]) * local_sizes[0];
  const auto x_end = global_range[0] + (is_inclusive ? 1 : 0);
  const auto x_work_group_end = x_work_group_start + local_sizes[0];
  const auto y_work_group_start =
      (global_range[1] / local_sizes[1]) * local_sizes[1];
  const auto y_end = global_range[1] + 1;
  const auto y_work_group_end = y_work_group_start + local_sizes[1];
  const auto z_work_group_start =
      (global_range[2] / local_sizes[2]) * local_sizes[2];
  const auto z_end = global_range[2] + 1;

  return {x_work_group_start, x_end, x_work_group_end,
          y_work_group_start, y_end, y_work_group_end,
          z_work_group_start, z_end};
}

TEST_F(WorkGroupCollectiveFunctionsTest,
       Work_Group_Collective_Functions_01_All) {
  auto runTestCase = [this](size_t local_size_x, size_t local_size_y,
                            size_t local_size_z) {
    const std::array<size_t, 3> global_sizes{local_size_x * 4, local_size_y,
                                             local_size_z};
    const std::array<size_t, 3> local_sizes{local_size_x, local_size_y,
                                            local_size_z};
    const auto global_size =
        global_sizes[0] * global_sizes[1] * global_sizes[2];

    std::vector<cl_int> input_data(global_size);
    std::vector<cl_int> output_data(global_size);

    kts::Reference1D<cl_uint> input_ref = [&input_data](size_t id) {
      return input_data[id];
    };

    kts::Reference1D<cl_int> output_ref =
        [&input_data, &global_sizes, &local_sizes](size_t global_linear_id,
                                                   cl_int result) {
          cl_int expected = 1;
          const auto range =
              getReductionRange(global_linear_id, global_sizes, local_sizes);

          for (auto x = range.x_start; x < range.x_end; ++x) {
            for (auto y = range.y_start; y < range.y_end; ++y) {
              for (auto z = range.z_start; z < range.z_end; ++z) {
                const auto linear_id = globalIdToGlobalLinearId(
                    {x, y, z}, global_sizes[0], global_sizes[1]);
                expected &= !!input_data[linear_id];
              }
            }
          }
          return !!result == !!expected;
        };

    // First an input of all false.
    input_data.assign(global_size, 0);
    AddInputBuffer(global_size, input_ref);
    AddOutputBuffer(global_size, output_ref);
    RunGenericND(3, global_sizes.data(), local_sizes.data());

    // Then an input of all true.
    input_data.assign(global_size, 42);
    AddInputBuffer(global_size, input_ref);
    AddOutputBuffer(global_size, output_ref);
    RunGenericND(3, global_sizes.data(), local_sizes.data());

    // Then a mix.
    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
    AddInputBuffer(global_size, input_ref);
    AddOutputBuffer(global_size, output_ref);
    RunGenericND(3, global_sizes.data(), local_sizes.data());
  };

  runTestCase(64, 1, 1);
  runTestCase(1, 64, 1);
  runTestCase(1, 1, 64);
  runTestCase(67, 1, 1);
  runTestCase(67, 5, 1);
  runTestCase(67, 2, 3);
}

TEST_F(WorkGroupCollectiveFunctionsTest,
       Work_Group_Collective_Functions_02_Any) {
  auto runTestCase = [this](size_t local_size_x, size_t local_size_y,
                            size_t local_size_z) {
    const std::array<size_t, 3> global_sizes{local_size_x * 4, local_size_y,
                                             local_size_z};
    const std::array<size_t, 3> local_sizes{local_size_x, local_size_y,
                                            local_size_z};
    const auto global_size =
        global_sizes[0] * global_sizes[1] * global_sizes[2];
    std::vector<cl_int> input_data(global_size);

    kts::Reference1D<cl_uint> input_ref = [&input_data](size_t id) {
      return input_data[id];
    };

    kts::Reference1D<cl_int> output_ref =
        [&input_data, &global_sizes, &local_sizes](size_t global_linear_id,
                                                   cl_int result) {
          cl_int expected = 0;
          const auto range =
              getReductionRange(global_linear_id, global_sizes, local_sizes);

          for (auto x = range.x_start; x < range.x_end; ++x) {
            for (auto y = range.y_start; y < range.y_end; ++y) {
              for (auto z = range.z_start; z < range.z_end; ++z) {
                const auto linear_id = globalIdToGlobalLinearId(
                    {x, y, z}, global_sizes[0], global_sizes[1]);
                expected |= !!input_data[linear_id];
              }
            }
          }
          return !!result == !!expected;
        };

    // First an input of all false.
    input_data.assign(global_size, 0);
    AddInputBuffer(global_size, input_ref);
    AddOutputBuffer(global_size, output_ref);
    RunGenericND(3, global_sizes.data(), local_sizes.data());

    // Then an input of all true.
    input_data.assign(global_size, 42);
    AddInputBuffer(global_size, input_ref);
    AddOutputBuffer(global_size, output_ref);
    RunGenericND(3, global_sizes.data(), local_sizes.data());

    // Then a mix.
    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
    AddInputBuffer(global_size, input_ref);
    AddOutputBuffer(global_size, output_ref);
    RunGenericND(3, global_sizes.data(), local_sizes.data());
  };

  runTestCase(64, 1, 1);
  runTestCase(1, 64, 1);
  runTestCase(1, 1, 64);
  runTestCase(67, 1, 1);
  runTestCase(67, 5, 1);
  runTestCase(67, 2, 3);
}

template <typename T>
struct WorkGroupCollectiveFunctionsTypeParameterizedTest
    : public WorkGroupCollectiveFunctionsTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(WorkGroupCollectiveFunctionsTest::SetUp());

    const auto clc_type_name = T::source_name();
    AddMacro("TYPE", clc_type_name);
    ASSERT_TRUE(BuildProgram());
  }
};
TYPED_TEST_SUITE_P(WorkGroupCollectiveFunctionsTypeParameterizedTest);

TYPED_TEST_P(WorkGroupCollectiveFunctionsTypeParameterizedTest,
             Work_Group_Collective_Functions_03_Broadcast_1D) {
  using cl_type = typename TypeParam::cl_type;
  auto runTestCase = [this](size_t local_size_x, size_t local_size_y,
                            size_t local_size_z) {
    const std::array<size_t, 3> global_sizes{local_size_x * 4, local_size_y,
                                             local_size_z};
    const auto global_size =
        global_sizes[0] * global_sizes[1] * global_sizes[2];
    const std::array<size_t, 3> local_sizes{local_size_x, local_size_y,
                                            local_size_z};
    const auto work_group_size =
        local_sizes[0] * local_sizes[1] * local_sizes[2];
    const size_t work_group_count = global_sizes[0] / local_sizes[0] *
                                    global_sizes[1] / local_sizes[1] *
                                    global_sizes[2] / local_sizes[2];

    std::vector<cl_type> input_data(global_size);
    std::vector<size_t> broadcast_ids(local_size_x);

    kts::Reference1D<cl_type> input_ref_a = [&input_data](size_t id) {
      return input_data[id];
    };

    kts::Reference1D<size_t> input_ref_b = [&broadcast_ids](size_t id) {
      return broadcast_ids[id];
    };

    kts::Reference1D<cl_type> output_ref =
        [&input_data, &work_group_size, &local_size_x, &broadcast_ids](
            size_t global_linear_id, cl_type value) {
          const auto work_group_linear_id = global_linear_id / work_group_size;
          const auto broadcast_id = work_group_linear_id * local_size_x +
                                    broadcast_ids[work_group_linear_id];
          return value == input_data[broadcast_id];
        };

    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
    ucl::Environment::instance->GetInputGenerator().GenerateData<size_t>(
        broadcast_ids, 0, local_size_x - 1);

    this->AddInputBuffer(global_size, input_ref_a);
    this->AddInputBuffer(work_group_count, input_ref_b);
    this->AddOutputBuffer(global_size, output_ref);
    this->RunGenericND(3, global_sizes.data(), local_sizes.data());
  };

  runTestCase(64, 1, 1);
  runTestCase(67, 1, 1);
}

TYPED_TEST_P(WorkGroupCollectiveFunctionsTypeParameterizedTest,
             Work_Group_Collective_Functions_04_Broadcast_2D) {
  using cl_type = typename TypeParam::cl_type;
  auto runTestCase = [this](size_t local_size_x, size_t local_size_y,
                            size_t local_size_z) {
    const std::array<size_t, 3> global_sizes{local_size_x * 4, local_size_y,
                                             local_size_z};
    const auto global_size =
        global_sizes[0] * global_sizes[1] * global_sizes[2];
    const std::array<size_t, 3> local_sizes{local_size_x, local_size_y,
                                            local_size_z};
    const size_t work_group_count = global_sizes[0] / local_sizes[0] *
                                    global_sizes[1] / local_sizes[1] *
                                    global_sizes[2] / local_sizes[2];

    std::vector<cl_type> input_data(global_size);
    std::vector<size_t> broadcast_x_ids(work_group_count);
    std::vector<size_t> broadcast_y_ids(work_group_count);

    kts::Reference1D<cl_type> input_ref_a = [&input_data](size_t id) {
      return input_data[id];
    };

    kts::Reference1D<cl_uint2> input_ref_b = [&broadcast_x_ids,
                                              &broadcast_y_ids](size_t id) {
      cl_uint2 broadcast_pair;
      broadcast_pair.x = broadcast_x_ids[id];
      broadcast_pair.y = broadcast_y_ids[id];
      return broadcast_pair;
    };

    kts::Reference1D<cl_type> output_ref = [&input_data, &global_sizes,
                                            &local_size_x, &local_size_y,
                                            &broadcast_x_ids, &broadcast_y_ids](
                                               size_t global_linear_id,
                                               cl_type value) {
      const auto global_ids = globalLinearIdToGlobalId(
          global_linear_id, global_sizes[0], global_sizes[1]);

      const auto work_group_id_x = global_ids[0] / local_size_x;
      const auto work_group_id_y = global_ids[1] / local_size_y;

      const auto work_group_linear_id =
          work_group_id_x + work_group_id_y * global_sizes[0] / local_size_x;

      const auto broadcast_x_id = local_size_x * work_group_id_x +
                                  broadcast_x_ids[work_group_linear_id];

      const auto broadcast_y_id = local_size_y * work_group_id_y +
                                  broadcast_y_ids[work_group_linear_id];

      const auto broadcast_linear_id =
          globalIdToGlobalLinearId({broadcast_x_id, broadcast_y_id, 0},
                                   global_sizes[0], global_sizes[1]);

      return value == input_data[broadcast_linear_id];
    };

    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
    ucl::Environment::instance->GetInputGenerator().GenerateData<size_t>(
        broadcast_x_ids, 0, local_size_x - 1);
    ucl::Environment::instance->GetInputGenerator().GenerateData<size_t>(
        broadcast_y_ids, 0, local_size_y - 1);

    this->AddInputBuffer(global_size, input_ref_a);
    this->AddInputBuffer(work_group_count, input_ref_b);
    this->AddOutputBuffer(global_size, output_ref);
    this->RunGenericND(3, global_sizes.data(), local_sizes.data());
  };

  runTestCase(64, 1, 1);
  runTestCase(1, 64, 1);
  runTestCase(67, 1, 1);
  runTestCase(67, 5, 1);
}

TYPED_TEST_P(WorkGroupCollectiveFunctionsTypeParameterizedTest,
             Work_Group_Collective_Functions_05_Broadcast_3D) {
  using cl_type = typename TypeParam::cl_type;
  auto runTestCase = [this](size_t local_size_x, size_t local_size_y,
                            size_t local_size_z) {
    const std::array<size_t, 3> global_sizes{local_size_x * 4, local_size_y,
                                             local_size_z};
    const auto global_size =
        global_sizes[0] * global_sizes[1] * global_sizes[2];
    const std::array<size_t, 3> local_sizes{local_size_x, local_size_y,
                                            local_size_z};
    const size_t work_group_count = global_sizes[0] / local_sizes[0] *
                                    global_sizes[1] / local_sizes[1] *
                                    global_sizes[2] / local_sizes[2];

    std::vector<cl_type> input_data(global_size);
    std::vector<size_t> broadcast_x_ids(work_group_count);
    std::vector<size_t> broadcast_y_ids(work_group_count);
    std::vector<size_t> broadcast_z_ids(work_group_count);

    kts::Reference1D<cl_type> input_ref_a = [&input_data](size_t id) {
      return input_data[id];
    };

    kts::Reference1D<cl_uint3> input_ref_b =
        [&broadcast_x_ids, &broadcast_y_ids, &broadcast_z_ids](size_t id) {
          cl_uint3 broadcast_triple;
          broadcast_triple.x = broadcast_x_ids[id];
          broadcast_triple.y = broadcast_y_ids[id];
          broadcast_triple.z = broadcast_z_ids[id];
          return broadcast_triple;
        };

    kts::Reference1D<cl_type> output_ref =
        [&input_data, &global_sizes, &local_size_x, &local_size_y,
         &local_size_z, &broadcast_x_ids, &broadcast_y_ids,
         &broadcast_z_ids](size_t global_linear_id, cl_type value) {
          const auto global_ids = globalLinearIdToGlobalId(
              global_linear_id, global_sizes[0], global_sizes[1]);

          const auto work_group_id_x = global_ids[0] / local_size_x;
          const auto work_group_id_y = global_ids[1] / local_size_y;
          const auto work_group_id_z = global_ids[2] / local_size_z;

          const auto work_group_linear_id =
              work_group_id_x +
              work_group_id_y * global_sizes[0] / local_size_x +
              work_group_id_z * global_sizes[0] / local_size_x *
                  global_sizes[1] / local_size_y;

          const auto broadcast_x_id = local_size_x * work_group_id_x +
                                      broadcast_x_ids[work_group_linear_id];

          const auto broadcast_y_id = local_size_y * work_group_id_y +
                                      broadcast_y_ids[work_group_linear_id];

          const auto broadcast_z_id = local_size_z * work_group_id_z +
                                      broadcast_z_ids[work_group_linear_id];

          const auto broadcast_linear_id = globalIdToGlobalLinearId(
              {broadcast_x_id, broadcast_y_id, broadcast_z_id}, global_sizes[0],
              global_sizes[1]);

          return value == input_data[broadcast_linear_id];
        };

    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
    ucl::Environment::instance->GetInputGenerator().GenerateData<size_t>(
        broadcast_x_ids, 0, local_size_x - 1);
    ucl::Environment::instance->GetInputGenerator().GenerateData<size_t>(
        broadcast_y_ids, 0, local_size_y - 1);

    this->AddInputBuffer(global_size, input_ref_a);
    this->AddInputBuffer(work_group_count, input_ref_b);
    this->AddOutputBuffer(global_size, output_ref);
    this->RunGenericND(3, global_sizes.data(), local_sizes.data());
  };

  runTestCase(64, 1, 1);
  runTestCase(1, 64, 1);
  runTestCase(1, 1, 64);
  runTestCase(67, 1, 1);
  runTestCase(67, 5, 1);
  runTestCase(67, 2, 3);
}

TYPED_TEST_P(WorkGroupCollectiveFunctionsTypeParameterizedTest,
             Work_Group_Collective_Functions_06_Reduce_Add) {
  using cl_type = typename TypeParam::cl_type;
  auto runTestCase = [this](size_t local_size_x, size_t local_size_y,
                            size_t local_size_z) {
    const std::array<size_t, 3> global_sizes{local_size_x * 4, local_size_y,
                                             local_size_z};
    const std::array<size_t, 3> local_sizes{local_size_x, local_size_y,
                                            local_size_z};
    const auto global_size =
        global_sizes[0] * global_sizes[1] * global_sizes[2];

    std::vector<cl_type> input_data(global_size);
    std::vector<cl_type> output_data(global_size);

    kts::Reference1D<cl_type> input_ref = [&input_data](size_t id) {
      return input_data[id];
    };

    kts::Reference1D<cl_type> output_ref =
        [&input_data, &global_sizes, &local_sizes](size_t global_linear_id,
                                                   cl_type result) {
          cl_type expected = 0;
          const auto range =
              getReductionRange(global_linear_id, global_sizes, local_sizes);

          for (auto x = range.x_start; x < range.x_end; ++x) {
            for (auto y = range.y_start; y < range.y_end; ++y) {
              for (auto z = range.z_start; z < range.z_end; ++z) {
                const auto linear_id = globalIdToGlobalLinearId(
                    {x, y, z}, global_sizes[0], global_sizes[1]);
                expected += input_data[linear_id];
              }
            }
          }
          return result == expected;
        };

    const auto work_group_size = local_size_x * local_size_y * local_size_z;
    const auto min = std::numeric_limits<cl_type>::min() /
                     static_cast<cl_type>(work_group_size);
    const auto max = std::numeric_limits<cl_type>::max() /
                     static_cast<cl_type>(work_group_size);
    ucl::Environment::instance->GetInputGenerator().GenerateData<cl_type>(
        input_data, min, max);
    this->AddInputBuffer(global_size, input_ref);
    this->AddOutputBuffer(global_size, output_ref);
    this->RunGenericND(3, global_sizes.data(), local_sizes.data());
  };

  runTestCase(64, 1, 1);
  runTestCase(1, 64, 1);
  runTestCase(1, 1, 64);
  runTestCase(67, 1, 1);
  runTestCase(67, 5, 1);
  runTestCase(67, 2, 3);
}

TYPED_TEST_P(WorkGroupCollectiveFunctionsTypeParameterizedTest,
             Work_Group_Collective_Functions_07_Reduce_Min) {
  using cl_type = typename TypeParam::cl_type;
  auto runTestCase = [this](size_t local_size_x, size_t local_size_y,
                            size_t local_size_z) {
    const std::array<size_t, 3> global_sizes{local_size_x * 1, local_size_y,
                                             local_size_z};
    const std::array<size_t, 3> local_sizes{local_size_x, local_size_y,
                                            local_size_z};
    const auto global_size =
        global_sizes[0] * global_sizes[1] * global_sizes[2];

    std::vector<cl_type> input_data(global_size);
    std::vector<cl_type> output_data(global_size);

    kts::Reference1D<cl_type> input_ref = [&input_data](size_t id) {
      return input_data[id];
    };

    kts::Reference1D<cl_type> output_ref =
        [&input_data, &global_sizes, &local_sizes](size_t global_linear_id,
                                                   cl_type result) {
          cl_type expected = std::numeric_limits<cl_type>::max();
          const auto range =
              getReductionRange(global_linear_id, global_sizes, local_sizes);

          for (auto x = range.x_start; x < range.x_end; ++x) {
            for (auto y = range.y_start; y < range.y_end; ++y) {
              for (auto z = range.z_start; z < range.z_end; ++z) {
                const auto linear_id = globalIdToGlobalLinearId(
                    {x, y, z}, global_sizes[0], global_sizes[1]);
                expected = std::min(expected, input_data[linear_id]);
              }
            }
          }
          return result == expected;
        };

    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
    this->AddInputBuffer(global_size, input_ref);
    this->AddOutputBuffer(global_size, output_ref);
    this->RunGenericND(3, global_sizes.data(), local_sizes.data());
  };

  runTestCase(3, 1, 1);
  runTestCase(1, 64, 1);
  runTestCase(1, 1, 64);
  runTestCase(67, 1, 1);
  runTestCase(67, 5, 1);
  runTestCase(67, 2, 3);
}

TYPED_TEST_P(WorkGroupCollectiveFunctionsTypeParameterizedTest,
             Work_Group_Collective_Functions_08_Reduce_Max) {
  using cl_type = typename TypeParam::cl_type;
  auto runTestCase = [this](size_t local_size_x, size_t local_size_y,
                            size_t local_size_z) {
    const std::array<size_t, 3> global_sizes{local_size_x * 4, local_size_y,
                                             local_size_z};
    const std::array<size_t, 3> local_sizes{local_size_x, local_size_y,
                                            local_size_z};
    const auto global_size =
        global_sizes[0] * global_sizes[1] * global_sizes[2];

    std::vector<cl_type> input_data(global_size);
    std::vector<cl_type> output_data(global_size);

    kts::Reference1D<cl_type> input_ref = [&input_data](size_t id) {
      return input_data[id];
    };

    kts::Reference1D<cl_type> output_ref =
        [&input_data, &global_sizes, &local_sizes](size_t global_linear_id,
                                                   cl_type result) {
          cl_type expected = std::numeric_limits<cl_type>::min();
          const auto range =
              getReductionRange(global_linear_id, global_sizes, local_sizes);

          for (auto x = range.x_start; x < range.x_end; ++x) {
            for (auto y = range.y_start; y < range.y_end; ++y) {
              for (auto z = range.z_start; z < range.z_end; ++z) {
                const auto linear_id = globalIdToGlobalLinearId(
                    {x, y, z}, global_sizes[0], global_sizes[1]);
                expected = std::max(expected, input_data[linear_id]);
              }
            }
          }
          return result == expected;
        };

    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
    this->AddInputBuffer(global_size, input_ref);
    this->AddOutputBuffer(global_size, output_ref);
    this->RunGenericND(3, global_sizes.data(), local_sizes.data());
  };

  runTestCase(64, 1, 1);
  runTestCase(1, 64, 1);
  runTestCase(1, 1, 64);
  runTestCase(67, 1, 1);
  runTestCase(67, 5, 1);
  runTestCase(67, 2, 3);
}

TYPED_TEST_P(WorkGroupCollectiveFunctionsTypeParameterizedTest,
             Work_Group_Collective_Functions_09_Scan_Exclusive_Add) {
  using cl_type = typename TypeParam::cl_type;
  auto runTestCase = [this](size_t local_size_x, size_t local_size_y,
                            size_t local_size_z) {
    const std::array<size_t, 3> global_sizes{local_size_x * 4, local_size_y,
                                             local_size_z};
    const std::array<size_t, 3> local_sizes{local_size_x, local_size_y,
                                            local_size_z};
    const auto global_size =
        global_sizes[0] * global_sizes[1] * global_sizes[2];

    std::vector<cl_type> input_data(global_size);
    std::vector<cl_type> output_data(global_size);

    kts::Reference1D<cl_type> input_ref = [&input_data](size_t id) {
      return input_data[id];
    };

    kts::Reference1D<cl_type> output_ref =
        [&input_data, &global_sizes, &local_sizes](size_t global_linear_id,
                                                   cl_type result) {
          cl_type expected = 0;
          const auto range =
              getScanRange(global_linear_id, global_sizes, local_sizes,
                           /* is_inclusive */ false);

          for (auto z = range.z_work_group_start; z < range.z_end; ++z) {
            const auto y_finish =
                (z == range.z_end - 1) ? range.y_end : range.y_work_group_end;
            for (auto y = range.y_work_group_start; y < y_finish; ++y) {
              const auto x_finish =
                  ((y == range.y_end - 1) && (z == range.z_end - 1))
                      ? range.x_end
                      : range.x_work_group_end;
              for (auto x = range.x_work_group_start; x < x_finish; ++x) {
                const auto linear_id = globalIdToGlobalLinearId(
                    {x, y, z}, global_sizes[0], global_sizes[1]);
                expected += input_data[linear_id];
              }
            }
          }
          return result == expected;
        };

    const auto work_group_size = local_size_x * local_size_y * local_size_z;
    const auto min = std::numeric_limits<cl_type>::min() /
                     static_cast<cl_type>(work_group_size);
    const auto max = std::numeric_limits<cl_type>::max() /
                     static_cast<cl_type>(work_group_size);
    ucl::Environment::instance->GetInputGenerator().GenerateData<cl_type>(
        input_data, min, max);
    this->AddInputBuffer(global_size, input_ref);
    this->AddOutputBuffer(global_size, output_ref);
    this->RunGenericND(3, global_sizes.data(), local_sizes.data());
  };

  runTestCase(64, 1, 1);
  runTestCase(1, 64, 1);
  runTestCase(1, 1, 64);
  runTestCase(67, 1, 1);
  runTestCase(67, 5, 1);
  runTestCase(67, 2, 3);
}

TYPED_TEST_P(WorkGroupCollectiveFunctionsTypeParameterizedTest,
             Work_Group_Collective_Functions_10_Scan_Exclusive_Min) {
  using cl_type = typename TypeParam::cl_type;
  auto runTestCase = [this](size_t local_size_x, size_t local_size_y,
                            size_t local_size_z) {
    const std::array<size_t, 3> global_sizes{local_size_x * 4, local_size_y,
                                             local_size_z};
    const std::array<size_t, 3> local_sizes{local_size_x, local_size_y,
                                            local_size_z};
    const auto global_size =
        global_sizes[0] * global_sizes[1] * global_sizes[2];

    std::vector<cl_type> input_data(global_size);
    std::vector<cl_type> output_data(global_size);

    kts::Reference1D<cl_type> input_ref = [&input_data](size_t id) {
      return input_data[id];
    };

    kts::Reference1D<cl_type> output_ref =
        [&input_data, &global_sizes, &local_sizes](size_t global_linear_id,
                                                   cl_type result) {
          cl_type expected = std::numeric_limits<cl_type>::max();
          const auto range =
              getScanRange(global_linear_id, global_sizes, local_sizes,
                           /* is_inclusive */ false);

          for (auto z = range.z_work_group_start; z < range.z_end; ++z) {
            const auto y_finish =
                (z == range.z_end - 1) ? range.y_end : range.y_work_group_end;
            for (auto y = range.y_work_group_start; y < y_finish; ++y) {
              const auto x_finish =
                  ((y == range.y_end - 1) && (z == range.z_end - 1))
                      ? range.x_end
                      : range.x_work_group_end;
              for (auto x = range.x_work_group_start; x < x_finish; ++x) {
                const auto linear_id = globalIdToGlobalLinearId(
                    {x, y, z}, global_sizes[0], global_sizes[1]);
                expected = std::min(expected, input_data[linear_id]);
              }
            }
          }
          return result == expected;
        };

    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
    this->AddInputBuffer(global_size, input_ref);
    this->AddOutputBuffer(global_size, output_ref);
    this->RunGenericND(3, global_sizes.data(), local_sizes.data());
  };

  runTestCase(64, 1, 1);
  runTestCase(1, 64, 1);
  runTestCase(1, 1, 64);
  runTestCase(67, 1, 1);
  runTestCase(67, 5, 1);
}

TYPED_TEST_P(WorkGroupCollectiveFunctionsTypeParameterizedTest,
             Work_Group_Collective_Functions_11_Scan_Exclusive_Max) {
  using cl_type = typename TypeParam::cl_type;
  auto runTestCase = [this](size_t local_size_x, size_t local_size_y,
                            size_t local_size_z) {
    const std::array<size_t, 3> global_sizes{local_size_x * 4, local_size_y,
                                             local_size_z};
    const std::array<size_t, 3> local_sizes{local_size_x, local_size_y,
                                            local_size_z};
    const auto global_size =
        global_sizes[0] * global_sizes[1] * global_sizes[2];

    std::vector<cl_type> input_data(global_size);
    std::vector<cl_type> output_data(global_size);

    kts::Reference1D<cl_type> input_ref = [&input_data](size_t id) {
      return input_data[id];
    };

    kts::Reference1D<cl_type> output_ref =
        [&input_data, &global_sizes, &local_sizes](size_t global_linear_id,
                                                   cl_type result) {
          cl_type expected = std::numeric_limits<cl_type>::min();
          const auto range =
              getScanRange(global_linear_id, global_sizes, local_sizes,
                           /* is_inclusive */ false);

          for (auto z = range.z_work_group_start; z < range.z_end; ++z) {
            const auto y_finish =
                (z == range.z_end - 1) ? range.y_end : range.y_work_group_end;
            for (auto y = range.y_work_group_start; y < y_finish; ++y) {
              const auto x_finish =
                  ((y == range.y_end - 1) && (z == range.z_end - 1))
                      ? range.x_end
                      : range.x_work_group_end;
              for (auto x = range.x_work_group_start; x < x_finish; ++x) {
                const auto linear_id = globalIdToGlobalLinearId(
                    {x, y, z}, global_sizes[0], global_sizes[1]);
                expected = std::max(expected, input_data[linear_id]);
              }
            }
          }
          return result == expected;
        };

    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
    this->AddInputBuffer(global_size, input_ref);
    this->AddOutputBuffer(global_size, output_ref);
    this->RunGenericND(3, global_sizes.data(), local_sizes.data());
  };

  runTestCase(64, 1, 1);
  runTestCase(1, 64, 1);
  runTestCase(1, 1, 64);
  runTestCase(67, 1, 1);
  runTestCase(67, 5, 1);
  runTestCase(67, 2, 3);
}

TYPED_TEST_P(WorkGroupCollectiveFunctionsTypeParameterizedTest,
             Work_Group_Collective_Functions_12_Scan_Inclusive_Add) {
  using cl_type = typename TypeParam::cl_type;
  auto runTestCase = [this](size_t local_size_x, size_t local_size_y,
                            size_t local_size_z) {
    const std::array<size_t, 3> global_sizes{local_size_x * 4, local_size_y,
                                             local_size_z};
    const std::array<size_t, 3> local_sizes{local_size_x, local_size_y,
                                            local_size_z};
    const auto global_size =
        global_sizes[0] * global_sizes[1] * global_sizes[2];

    std::vector<cl_type> input_data(global_size);
    std::vector<cl_type> output_data(global_size);

    kts::Reference1D<cl_type> input_ref = [&input_data](size_t id) {
      return input_data[id];
    };

    kts::Reference1D<cl_type> output_ref =
        [&input_data, &global_sizes, &local_sizes](size_t global_linear_id,
                                                   cl_type result) {
          cl_type expected = 0;
          const auto range =
              getScanRange(global_linear_id, global_sizes, local_sizes,
                           /* is_inclusive */ true);

          for (auto z = range.z_work_group_start; z < range.z_end; ++z) {
            const auto y_finish =
                (z == range.z_end - 1) ? range.y_end : range.y_work_group_end;
            for (auto y = range.y_work_group_start; y < y_finish; ++y) {
              const auto x_finish =
                  ((y == range.y_end - 1) && (z == range.z_end - 1))
                      ? range.x_end
                      : range.x_work_group_end;
              for (auto x = range.x_work_group_start; x < x_finish; ++x) {
                const auto linear_id = globalIdToGlobalLinearId(
                    {x, y, z}, global_sizes[0], global_sizes[1]);
                expected += input_data[linear_id];
              }
            }
          }
          return result == expected;
        };

    const auto work_group_size = local_size_x * local_size_y * local_size_z;
    const auto min = std::numeric_limits<cl_type>::min() /
                     static_cast<cl_type>(work_group_size);
    const auto max = std::numeric_limits<cl_type>::max() /
                     static_cast<cl_type>(work_group_size);
    ucl::Environment::instance->GetInputGenerator().GenerateData<cl_type>(
        input_data, min, max);
    this->AddInputBuffer(global_size, input_ref);
    this->AddOutputBuffer(global_size, output_ref);
    this->RunGenericND(3, global_sizes.data(), local_sizes.data());
  };

  runTestCase(64, 1, 1);
  runTestCase(1, 64, 1);
  runTestCase(1, 1, 64);
  runTestCase(67, 1, 1);
  runTestCase(67, 5, 1);
  runTestCase(67, 2, 3);
}

TYPED_TEST_P(WorkGroupCollectiveFunctionsTypeParameterizedTest,
             Work_Group_Collective_Functions_13_Scan_Inclusive_Min) {
  using cl_type = typename TypeParam::cl_type;
  auto runTestCase = [this](size_t local_size_x, size_t local_size_y,
                            size_t local_size_z) {
    const std::array<size_t, 3> global_sizes{local_size_x * 4, local_size_y,
                                             local_size_z};
    const std::array<size_t, 3> local_sizes{local_size_x, local_size_y,
                                            local_size_z};
    const auto global_size =
        global_sizes[0] * global_sizes[1] * global_sizes[2];

    std::vector<cl_type> input_data(global_size);
    std::vector<cl_type> output_data(global_size);

    kts::Reference1D<cl_type> input_ref = [&input_data](size_t id) {
      return input_data[id];
    };

    kts::Reference1D<cl_type> output_ref =
        [&input_data, &global_sizes, &local_sizes](size_t global_linear_id,
                                                   cl_type result) {
          cl_type expected = std::numeric_limits<cl_type>::max();
          const auto range =
              getScanRange(global_linear_id, global_sizes, local_sizes,
                           /* is_inclusive */ true);

          for (auto z = range.z_work_group_start; z < range.z_end; ++z) {
            const auto y_finish =
                (z == range.z_end - 1) ? range.y_end : range.y_work_group_end;
            for (auto y = range.y_work_group_start; y < y_finish; ++y) {
              const auto x_finish =
                  ((y == range.y_end - 1) && (z == range.z_end - 1))
                      ? range.x_end
                      : range.x_work_group_end;
              for (auto x = range.x_work_group_start; x < x_finish; ++x) {
                const auto linear_id = globalIdToGlobalLinearId(
                    {x, y, z}, global_sizes[0], global_sizes[1]);
                expected = std::min(expected, input_data[linear_id]);
              }
            }
          }
          return result == expected;
        };

    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
    this->AddInputBuffer(global_size, input_ref);
    this->AddOutputBuffer(global_size, output_ref);
    this->RunGenericND(3, global_sizes.data(), local_sizes.data());
  };

  runTestCase(64, 1, 1);
  runTestCase(1, 64, 1);
  runTestCase(1, 1, 64);
  runTestCase(67, 1, 1);
  runTestCase(67, 5, 1);
}

TYPED_TEST_P(WorkGroupCollectiveFunctionsTypeParameterizedTest,
             Work_Group_Collective_Functions_14_Scan_Inclusive_Max) {
  using cl_type = typename TypeParam::cl_type;
  auto runTestCase = [this](size_t local_size_x, size_t local_size_y,
                            size_t local_size_z) {
    const std::array<size_t, 3> global_sizes{local_size_x * 4, local_size_y,
                                             local_size_z};
    const std::array<size_t, 3> local_sizes{local_size_x, local_size_y,
                                            local_size_z};
    const auto global_size =
        global_sizes[0] * global_sizes[1] * global_sizes[2];

    std::vector<cl_type> input_data(global_size);
    std::vector<cl_type> output_data(global_size);

    kts::Reference1D<cl_type> input_ref = [&input_data](size_t id) {
      return input_data[id];
    };

    kts::Reference1D<cl_type> output_ref =
        [&input_data, &global_sizes, &local_sizes](size_t global_linear_id,
                                                   cl_type result) {
          cl_type expected = std::numeric_limits<cl_type>::min();
          const auto range =
              getScanRange(global_linear_id, global_sizes, local_sizes,
                           /* is_inclusive */ true);

          for (auto z = range.z_work_group_start; z < range.z_end; ++z) {
            const auto y_finish =
                (z == range.z_end - 1) ? range.y_end : range.y_work_group_end;
            for (auto y = range.y_work_group_start; y < y_finish; ++y) {
              const auto x_finish =
                  ((y == range.y_end - 1) && (z == range.z_end - 1))
                      ? range.x_end
                      : range.x_work_group_end;
              for (auto x = range.x_work_group_start; x < x_finish; ++x) {
                const auto linear_id = globalIdToGlobalLinearId(
                    {x, y, z}, global_sizes[0], global_sizes[1]);
                expected = std::max(expected, input_data[linear_id]);
              }
            }
          }
          return result == expected;
        };

    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
    this->AddInputBuffer(global_size, input_ref);
    this->AddOutputBuffer(global_size, output_ref);
    this->RunGenericND(3, global_sizes.data(), local_sizes.data());
  };

  runTestCase(64, 1, 1);
  runTestCase(1, 64, 1);
  runTestCase(1, 1, 64);
  runTestCase(67, 1, 1);
  runTestCase(67, 5, 1);
  runTestCase(67, 2, 3);
}

REGISTER_TYPED_TEST_CASE_P(
    WorkGroupCollectiveFunctionsTypeParameterizedTest,
    Work_Group_Collective_Functions_03_Broadcast_1D,
    Work_Group_Collective_Functions_04_Broadcast_2D,
    Work_Group_Collective_Functions_05_Broadcast_3D,
    Work_Group_Collective_Functions_06_Reduce_Add,
    Work_Group_Collective_Functions_07_Reduce_Min,
    Work_Group_Collective_Functions_08_Reduce_Max,
    Work_Group_Collective_Functions_09_Scan_Exclusive_Add,
    Work_Group_Collective_Functions_10_Scan_Exclusive_Min,
    Work_Group_Collective_Functions_11_Scan_Exclusive_Max,
    Work_Group_Collective_Functions_12_Scan_Inclusive_Add,
    Work_Group_Collective_Functions_13_Scan_Inclusive_Min,
    Work_Group_Collective_Functions_14_Scan_Inclusive_Max);

#if !defined(__clang_analyzer__)
// TODO: Enable testing for the floating point types (see CA-4558)
using ScalarTypes =
    testing::Types<ucl::Int, ucl::UInt, ucl::Long,
                   ucl::ULong /*, ucl::Float, ucl::Double, ucl::Half */>;
#else
using ScalarTypes = testing::Types<ucl::Int>;
#endif
// Reduce the number of types to test if running clang analyzer (or
// clang-tidy), they'll all result in basically the same code but it takes a
// long time to analyze all of them.
INSTANTIATE_TYPED_TEST_CASE_P(ScalarTypes,
                              WorkGroupCollectiveFunctionsTypeParameterizedTest,
                              ScalarTypes);
