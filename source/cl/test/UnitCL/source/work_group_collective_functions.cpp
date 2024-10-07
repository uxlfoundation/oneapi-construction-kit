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
#include <cargo/optional.h>

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

// Denotes a work-group size used to execute tests.
struct NDRange {
  NDRange(size_t _x, size_t _y, size_t _z) {
    array[0] = _x;
    array[1] = _y;
    array[2] = _z;
  }

  size_t array[3];

  size_t x() const { return array[0]; }
  size_t y() const { return array[1]; }
  size_t z() const { return array[2]; }

  // An implicit conversion to a size_t* allows idiomatic usage like
  // local_sizes[0], or being passed to RunGenericND.
  operator size_t *() { return &array[0]; }
  operator const size_t *() const { return &array[0]; }
};

inline static std::string to_string(const NDRange &ndrange) {
  return std::to_string(ndrange.x()) + 'x' + std::to_string(ndrange.y()) + 'x' +
         std::to_string(ndrange.z());
}

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
  return global_ids[0] + (global_size_x * global_ids[1]) +
         (global_size_x * global_size_y * global_ids[2]);
}

struct ReductionRange {
  size_t x_start, x_end;
  size_t y_start, y_end;
  size_t z_start, z_end;
};

static ReductionRange getReductionRange(size_t global_linear_id,
                                        const size_t *global_sizes,
                                        const size_t *local_sizes) {
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

using WGCAnyAllParams = std::tuple<NDRange, cargo::optional<int>>;

struct WorkGroupCollectiveAnyAll
    : public kts::ucl::ExecutionWithParam<WGCAnyAllParams> {
  static std::string getParamName(
      const testing::TestParamInfo<
          std::tuple<kts::ucl::SourceType, WGCAnyAllParams>> &info) {
    const auto source_ty = std::get<0>(info.param);
    const auto &params = std::get<1>(info.param);
    const auto &ndrange = std::get<0>(params);
    const auto &input_data_val = std::get<1>(params);
    return to_string(source_ty) + '_' + to_string(ndrange) + '_' +
           (input_data_val ? "all" + std::to_string(*input_data_val)
                           : "random") +
           '_' + std::to_string(info.index);
  }
  template <bool IsAll>
  void doTest() {
    const auto &params = std::get<1>(GetParam());
    const auto &local_sizes = std::get<0>(params);
    const auto input_data_val = std::get<1>(params);

    NDRange global_sizes{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
    const auto global_size =
        global_sizes[0] * global_sizes[1] * global_sizes[2];
    std::vector<cl_int> input_data(global_size);

    kts::Reference1D<cl_uint> input_ref = [&input_data](size_t id) {
      return input_data[id];
    };

    kts::Reference1D<cl_int> output_ref =
        [&input_data, &global_sizes, &local_sizes](size_t global_linear_id,
                                                   cl_int result) {
          cl_int expected = IsAll ? 1 : 0;
          const auto range =
              getReductionRange(global_linear_id, global_sizes, local_sizes);

          for (auto x = range.x_start; x < range.x_end; ++x) {
            for (auto y = range.y_start; y < range.y_end; ++y) {
              for (auto z = range.z_start; z < range.z_end; ++z) {
                const auto linear_id = globalIdToGlobalLinearId(
                    {x, y, z}, global_sizes[0], global_sizes[1]);
                if (IsAll) {
                  expected &= !!input_data[linear_id];
                } else {
                  expected |= !!input_data[linear_id];
                }
              }
            }
          }
          return !!result == !!expected;
        };

    // AssignRandomDataSentinel is a special value to these tests, indicating
    // that the buffer should be filled with random data.
    if (input_data_val) {
      ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
    } else {
      input_data.assign(global_size, *input_data_val);
    }

    AddInputBuffer(global_size, input_ref);
    AddOutputBuffer(global_size, output_ref);
    RunGenericND(3, global_sizes, local_sizes);
  }

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

TEST_P(WorkGroupCollectiveAnyAll, Work_Group_Collective_Functions_01_All) {
  doTest</*IsAll*/ true>();
}

TEST_P(WorkGroupCollectiveAnyAll, Work_Group_Collective_Functions_02_Any) {
  doTest</*IsAll*/ false>();
}

using WGCParams = NDRange;

struct WorkGroupCollectiveFunctionsTest
    : public kts::ucl::ExecutionWithParam<WGCParams> {
  static std::string getParamName(
      const testing::TestParamInfo<std::tuple<kts::ucl::SourceType, WGCParams>>
          &info) {
    const auto source_ty = std::get<0>(info.param);
    const auto &ndrange = std::get<1>(info.param);
    return to_string(source_ty) + '_' + to_string(ndrange) + '_' +
           std::to_string(info.index);
  }

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

struct ScanRange {
  size_t x_work_group_start, x_end, x_work_group_end;
  size_t y_work_group_start, y_end, y_work_group_end;
  size_t z_work_group_start, z_end;
};

static ScanRange getScanRange(size_t global_linear_id,
                              const size_t global_sizes[3],
                              const size_t local_sizes[3], bool is_inclusive) {
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

struct WorkGroupCollectiveBroadcast1D
    : public WorkGroupCollectiveFunctionsTest {
  template <typename T>
  void doBroadcast1DTest() {
    const auto &local_sizes = std::get<1>(GetParam());
    NDRange global_sizes{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
    const auto global_size =
        global_sizes.x() * global_sizes.y() * global_sizes.z();
    std::vector<T> input_data(global_size);
    const auto local_size_x = local_sizes.x();
    std::vector<size_t> broadcast_ids(local_size_x);

    kts::Reference1D<T> input_ref_a = [&input_data](size_t id) {
      return input_data[id];
    };

    kts::Reference1D<size_t> input_ref_b = [&broadcast_ids](size_t id) {
      return broadcast_ids[id];
    };

    const auto work_group_size =
        local_sizes.x() * local_sizes.y() * local_sizes.z();

    const size_t work_group_count = global_sizes.x() / local_sizes.x() *
                                    global_sizes.y() / local_sizes.y() *
                                    global_sizes.z() / local_sizes.z();

    kts::Reference1D<T> output_ref = [&input_data, &work_group_size,
                                      local_size_x, &broadcast_ids](
                                         size_t global_linear_id, T value) {
      const auto work_group_linear_id = global_linear_id / work_group_size;
      const auto broadcast_id = (work_group_linear_id * local_size_x) +
                                broadcast_ids[work_group_linear_id];
      return value == input_data[broadcast_id];
    };

    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
    ucl::Environment::instance->GetInputGenerator().GenerateData<size_t>(
        broadcast_ids, 0, local_size_x - 1);

    this->AddInputBuffer(global_size, input_ref_a);
    this->AddInputBuffer(work_group_count, input_ref_b);
    this->AddOutputBuffer(global_size, output_ref);
    this->RunGenericND(3, global_sizes, local_sizes);
  }
};

TEST_P(WorkGroupCollectiveBroadcast1D,
       Work_Group_Collective_Functions_03_Broadcast_1D_Int) {
  doBroadcast1DTest<cl_int>();
}
TEST_P(WorkGroupCollectiveBroadcast1D,
       Work_Group_Collective_Functions_03_Broadcast_1D_Uint) {
  doBroadcast1DTest<cl_uint>();
}
TEST_P(WorkGroupCollectiveBroadcast1D,
       Work_Group_Collective_Functions_03_Broadcast_1D_Long) {
  doBroadcast1DTest<cl_long>();
}
TEST_P(WorkGroupCollectiveBroadcast1D,
       Work_Group_Collective_Functions_03_Broadcast_1D_Ulong) {
  doBroadcast1DTest<cl_ulong>();
}

struct WorkGroupCollectiveBroadcast2D
    : public WorkGroupCollectiveFunctionsTest {
  template <typename T>
  void doBroadcast2DTest() {
    const auto &local_sizes = std::get<1>(GetParam());
    NDRange global_sizes{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
    const auto global_size =
        global_sizes.x() * global_sizes.y() * global_sizes.z();
    const size_t work_group_count = global_sizes.x() / local_sizes.x() *
                                    global_sizes.y() / local_sizes.y() *
                                    global_sizes.z() / local_sizes.z();

    std::vector<T> input_data(global_size);
    const auto local_size_x = local_sizes.x();
    const auto local_size_y = local_sizes.y();
    std::vector<size_t> broadcast_x_ids(work_group_count);
    std::vector<size_t> broadcast_y_ids(work_group_count);

    kts::Reference1D<T> input_ref_a = [&input_data](size_t id) {
      return input_data[id];
    };

    kts::Reference1D<cl_uint2> input_ref_b = [&broadcast_x_ids,
                                              &broadcast_y_ids](size_t id) {
      cl_uint2 broadcast_pair;
      broadcast_pair.x = broadcast_x_ids[id];
      broadcast_pair.y = broadcast_y_ids[id];
      return broadcast_pair;
    };

    kts::Reference1D<T> output_ref = [&input_data, &global_sizes, &local_size_x,
                                      &local_size_y, &broadcast_x_ids,
                                      &broadcast_y_ids](size_t global_linear_id,
                                                        T value) {
      const auto global_ids = globalLinearIdToGlobalId(
          global_linear_id, global_sizes[0], global_sizes[1]);

      const auto work_group_id_x = global_ids[0] / local_size_x;
      const auto work_group_id_y = global_ids[1] / local_size_y;

      const auto work_group_linear_id =
          work_group_id_x + (work_group_id_y * global_sizes[0] / local_size_x);

      const auto broadcast_x_id = (local_size_x * work_group_id_x) +
                                  broadcast_x_ids[work_group_linear_id];

      const auto broadcast_y_id = (local_size_y * work_group_id_y) +
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
    this->RunGenericND(3, global_sizes, local_sizes);
  }
};

TEST_P(WorkGroupCollectiveBroadcast2D,
       Work_Group_Collective_Functions_04_Broadcast_2D_Int) {
  doBroadcast2DTest<cl_int>();
}
TEST_P(WorkGroupCollectiveBroadcast2D,
       Work_Group_Collective_Functions_04_Broadcast_2D_Uint) {
  doBroadcast2DTest<cl_uint>();
}
TEST_P(WorkGroupCollectiveBroadcast2D,
       Work_Group_Collective_Functions_04_Broadcast_2D_Long) {
  doBroadcast2DTest<cl_long>();
}
TEST_P(WorkGroupCollectiveBroadcast2D,
       Work_Group_Collective_Functions_04_Broadcast_2D_Ulong) {
  doBroadcast2DTest<cl_ulong>();
}

struct WorkGroupCollectiveBroadcast3D
    : public WorkGroupCollectiveFunctionsTest {
  template <typename T>
  void doBroadcast3DTest() {
    const auto &local_sizes = std::get<1>(GetParam());
    NDRange global_sizes{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};

    const auto global_size =
        global_sizes[0] * global_sizes[1] * global_sizes[2];
    const size_t work_group_count = global_sizes.x() / local_sizes.x() *
                                    global_sizes.y() / local_sizes.y() *
                                    global_sizes.z() / local_sizes.z();

    std::vector<T> input_data(global_size);
    const auto local_size_x = local_sizes.x();
    const auto local_size_y = local_sizes.y();
    const auto local_size_z = local_sizes.z();
    std::vector<size_t> broadcast_x_ids(work_group_count);
    std::vector<size_t> broadcast_y_ids(work_group_count);
    std::vector<size_t> broadcast_z_ids(work_group_count);

    kts::Reference1D<T> input_ref_a = [&input_data](size_t id) {
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

    kts::Reference1D<T> output_ref = [&input_data, &global_sizes, &local_size_x,
                                      &local_size_y, &local_size_z,
                                      &broadcast_x_ids, &broadcast_y_ids,
                                      &broadcast_z_ids](size_t global_linear_id,
                                                        T value) {
      const auto global_ids = globalLinearIdToGlobalId(
          global_linear_id, global_sizes[0], global_sizes[1]);

      const auto work_group_id_x = global_ids[0] / local_size_x;
      const auto work_group_id_y = global_ids[1] / local_size_y;
      const auto work_group_id_z = global_ids[2] / local_size_z;

      const auto work_group_linear_id =
          work_group_id_x + (work_group_id_y * global_sizes[0] / local_size_x) +
          (work_group_id_z * global_sizes[0] / local_size_x * global_sizes[1] /
           local_size_y);

      const auto broadcast_x_id = (local_size_x * work_group_id_x) +
                                  broadcast_x_ids[work_group_linear_id];

      const auto broadcast_y_id = (local_size_y * work_group_id_y) +
                                  broadcast_y_ids[work_group_linear_id];

      const auto broadcast_z_id = (local_size_z * work_group_id_z) +
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
    this->RunGenericND(3, global_sizes, local_sizes);
  }
};

TEST_P(WorkGroupCollectiveBroadcast3D,
       Work_Group_Collective_Functions_05_Broadcast_3D_Int) {
  doBroadcast3DTest<cl_int>();
}
TEST_P(WorkGroupCollectiveBroadcast3D,
       Work_Group_Collective_Functions_05_Broadcast_3D_Uint) {
  doBroadcast3DTest<cl_uint>();
}
TEST_P(WorkGroupCollectiveBroadcast3D,
       Work_Group_Collective_Functions_05_Broadcast_3D_Long) {
  doBroadcast3DTest<cl_long>();
}
TEST_P(WorkGroupCollectiveBroadcast3D,
       Work_Group_Collective_Functions_05_Broadcast_3D_Ulong) {
  doBroadcast3DTest<cl_ulong>();
}

struct WorkGroupCollectiveScanReductionTestBase
    : public WorkGroupCollectiveFunctionsTest {
  template <typename T, bool ClampInputs = false>
  void doReductionTest(
      const std::function<bool(size_t, T, const std::vector<T> &input_data,
                               const NDRange &global_sizes,
                               const NDRange &local_sizes)> &output_ref_fn) {
    const auto &local_sizes = std::get<1>(GetParam());
    NDRange global_sizes{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};

    const auto global_size =
        global_sizes[0] * global_sizes[1] * global_sizes[2];

    std::vector<T> input_data(global_size);
    const std::vector<T> output_data(global_size);

    kts::Reference1D<T> input_ref = [&input_data](size_t id) {
      return input_data[id];
    };

    kts::Reference1D<T> output_ref = [&input_data, &global_sizes, &local_sizes,
                                      &output_ref_fn](size_t global_linear_id,
                                                      T result) {
      return output_ref_fn(global_linear_id, result, input_data, global_sizes,
                           local_sizes);
    };

    if (!ClampInputs) {
      ucl::Environment::instance->GetInputGenerator().GenerateData<T>(
          input_data);
    } else {
      const auto work_group_size =
          local_sizes.x() * local_sizes.y() * local_sizes.z();
      const auto min =
          std::numeric_limits<T>::min() / static_cast<T>(work_group_size);
      const auto max =
          std::numeric_limits<T>::max() / static_cast<T>(work_group_size);
      ucl::Environment::instance->GetInputGenerator().GenerateData<T>(
          input_data, min, max);
    }
    this->AddInputBuffer(global_size, input_ref);
    this->AddOutputBuffer(global_size, output_ref);
    this->RunGenericND(3, global_sizes, local_sizes);
  }
};

using WorkGroupCollectiveReductions = WorkGroupCollectiveScanReductionTestBase;

template <typename T>
static bool reduceBinOpRefFn(size_t global_linear_id, T result,
                             const std::vector<T> &input_data,
                             const NDRange &global_sizes,
                             const NDRange &local_sizes, T initial_val,
                             const std::function<T(T, T)> &reduce_fn) {
  T expected = initial_val;
  const auto range = getReductionRange(global_linear_id, global_sizes.array,
                                       local_sizes.array);

  for (auto x = range.x_start; x < range.x_end; ++x) {
    for (auto y = range.y_start; y < range.y_end; ++y) {
      for (auto z = range.z_start; z < range.z_end; ++z) {
        const auto linear_id = globalIdToGlobalLinearId(
            {x, y, z}, global_sizes[0], global_sizes[1]);
        expected = reduce_fn(expected, input_data[linear_id]);
      }
    }
  }
  return result == expected;
}

template <typename T>
static bool reduceAddRefFn(size_t global_linear_id, T result,
                           const std::vector<T> &input_data,
                           const NDRange &global_sizes,
                           const NDRange &local_sizes) {
  const std::function<T(T, T)> &reduce_fn = [](T a, T b) { return a + b; };
  return reduceBinOpRefFn(global_linear_id, result, input_data, global_sizes,
                          local_sizes, T{0}, reduce_fn);
}

TEST_P(WorkGroupCollectiveReductions,
       Work_Group_Collective_Functions_06_Reduce_Add_Int) {
  doReductionTest<cl_int, true>(reduceAddRefFn<cl_int>);
}
TEST_P(WorkGroupCollectiveReductions,
       Work_Group_Collective_Functions_06_Reduce_Add_Uint) {
  doReductionTest<cl_uint, true>(reduceAddRefFn<cl_uint>);
}
TEST_P(WorkGroupCollectiveReductions,
       Work_Group_Collective_Functions_06_Reduce_Add_Long) {
  doReductionTest<cl_long, true>(reduceAddRefFn<cl_long>);
}
TEST_P(WorkGroupCollectiveReductions,
       Work_Group_Collective_Functions_06_Reduce_Add_Ulong) {
  doReductionTest<cl_ulong, true>(reduceAddRefFn<cl_ulong>);
}

template <typename T>
static bool reduceMinRefFn(size_t global_linear_id, T result,
                           const std::vector<T> &input_data,
                           const NDRange &global_sizes,
                           const NDRange &local_sizes) {
  const std::function<T(T, T)> &reduce_fn = [](T a, T b) {
    return std::min(a, b);
  };
  return reduceBinOpRefFn(global_linear_id, result, input_data, global_sizes,
                          local_sizes, std::numeric_limits<T>::max(),
                          reduce_fn);
}

TEST_P(WorkGroupCollectiveReductions,
       Work_Group_Collective_Functions_07_Reduce_Min_Int) {
  doReductionTest<cl_int>(reduceMinRefFn<cl_int>);
}
TEST_P(WorkGroupCollectiveReductions,
       Work_Group_Collective_Functions_07_Reduce_Min_Uint) {
  doReductionTest<cl_uint>(reduceMinRefFn<cl_uint>);
}
TEST_P(WorkGroupCollectiveReductions,
       Work_Group_Collective_Functions_07_Reduce_Min_Long) {
  doReductionTest<cl_long>(reduceMinRefFn<cl_long>);
}
TEST_P(WorkGroupCollectiveReductions,
       Work_Group_Collective_Functions_07_Reduce_Min_Ulong) {
  doReductionTest<cl_ulong>(reduceMinRefFn<cl_ulong>);
}

template <typename T>
static bool reduceMaxRefFn(size_t global_linear_id, T result,
                           const std::vector<T> &input_data,
                           const NDRange &global_sizes,
                           const NDRange &local_sizes) {
  const std::function<T(T, T)> &reduce_fn = [](T a, T b) {
    return std::max(a, b);
  };
  return reduceBinOpRefFn(global_linear_id, result, input_data, global_sizes,
                          local_sizes, std::numeric_limits<T>::min(),
                          reduce_fn);
}

TEST_P(WorkGroupCollectiveReductions,
       Work_Group_Collective_Functions_08_Reduce_Max_Int) {
  doReductionTest<cl_int>(reduceMaxRefFn<cl_int>);
}
TEST_P(WorkGroupCollectiveReductions,
       Work_Group_Collective_Functions_08_Reduce_Max_Uint) {
  doReductionTest<cl_uint>(reduceMaxRefFn<cl_uint>);
}
TEST_P(WorkGroupCollectiveReductions,
       Work_Group_Collective_Functions_08_Reduce_Max_Long) {
  doReductionTest<cl_long>(reduceMaxRefFn<cl_long>);
}
TEST_P(WorkGroupCollectiveReductions,
       Work_Group_Collective_Functions_08_Reduce_Max_Ulong) {
  doReductionTest<cl_ulong>(reduceMaxRefFn<cl_ulong>);
}

using WorkGroupCollectiveScans = WorkGroupCollectiveScanReductionTestBase;

template <typename T, bool IsInclusive>
static bool scanBinOpRefFn(size_t global_linear_id, T result,
                           const std::vector<T> &input_data,
                           const NDRange &global_sizes,
                           const NDRange &local_sizes, T initial_val,
                           const std::function<T(T, T)> &scan_fn) {
  T expected = initial_val;

  const auto range =
      getScanRange(global_linear_id, global_sizes, local_sizes, IsInclusive);

  for (auto z = range.z_work_group_start; z < range.z_end; ++z) {
    const auto y_finish =
        (z == range.z_end - 1) ? range.y_end : range.y_work_group_end;
    for (auto y = range.y_work_group_start; y < y_finish; ++y) {
      const auto x_finish = ((y == range.y_end - 1) && (z == range.z_end - 1))
                                ? range.x_end
                                : range.x_work_group_end;
      for (auto x = range.x_work_group_start; x < x_finish; ++x) {
        const auto linear_id = globalIdToGlobalLinearId(
            {x, y, z}, global_sizes[0], global_sizes[1]);
        expected = scan_fn(expected, input_data[linear_id]);
      }
    }
  }
  return result == expected;
}

template <typename T, bool IsInclusive>
static bool scanAddRefFn(size_t global_linear_id, T result,
                         const std::vector<T> &input_data,
                         const NDRange &global_sizes,
                         const NDRange &local_sizes) {
  const std::function<T(T, T)> &scan_fn = [](T a, T b) { return a + b; };
  return scanBinOpRefFn<T, IsInclusive>(global_linear_id, result, input_data,
                                        global_sizes, local_sizes, T{0},
                                        scan_fn);
}

template <typename T, bool IsInclusive>
static bool scanMinRefFn(size_t global_linear_id, T result,
                         const std::vector<T> &input_data,
                         const NDRange &global_sizes,
                         const NDRange &local_sizes) {
  const std::function<T(T, T)> &scan_fn = [](T a, T b) {
    return std::min(a, b);
  };
  return scanBinOpRefFn<T, IsInclusive>(global_linear_id, result, input_data,
                                        global_sizes, local_sizes,
                                        std::numeric_limits<T>::max(), scan_fn);
}

template <typename T, bool IsInclusive>
static bool scanMaxRefFn(size_t global_linear_id, T result,
                         const std::vector<T> &input_data,
                         const NDRange &global_sizes,
                         const NDRange &local_sizes) {
  const std::function<T(T, T)> &scan_fn = [](T a, T b) {
    return std::max(a, b);
  };
  return scanBinOpRefFn<T, IsInclusive>(global_linear_id, result, input_data,
                                        global_sizes, local_sizes,
                                        std::numeric_limits<T>::min(), scan_fn);
}

TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_09_Scan_Exclusive_Add_Int) {
  doReductionTest<cl_int, true>(scanAddRefFn<cl_int, false>);
}
TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_09_Scan_Exclusive_Add_UInt) {
  doReductionTest<cl_uint, true>(scanAddRefFn<cl_uint, false>);
}
TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_09_Scan_Exclusive_Add_Long) {
  doReductionTest<cl_long, true>(scanAddRefFn<cl_long, false>);
}
TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_09_Scan_Exclusive_Add_Ulong) {
  doReductionTest<cl_ulong, true>(scanAddRefFn<cl_ulong, false>);
}

TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_10_Scan_Exclusive_Min_Int) {
  doReductionTest<cl_int>(scanMinRefFn<cl_int, false>);
}
TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_10_Scan_Exclusive_Min_Uint) {
  doReductionTest<cl_uint>(scanMinRefFn<cl_uint, false>);
}
TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_10_Scan_Exclusive_Min_Long) {
  doReductionTest<cl_long>(scanMinRefFn<cl_long, false>);
}
TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_10_Scan_Exclusive_Min_Ulong) {
  doReductionTest<cl_ulong>(scanMinRefFn<cl_ulong, false>);
}

TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_11_Scan_Exclusive_Max_Int) {
  doReductionTest<cl_int>(scanMaxRefFn<cl_int, false>);
}
TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_11_Scan_Exclusive_Max_Uint) {
  doReductionTest<cl_uint>(scanMaxRefFn<cl_uint, false>);
}
TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_11_Scan_Exclusive_Max_Long) {
  doReductionTest<cl_long>(scanMaxRefFn<cl_long, false>);
}
TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_11_Scan_Exclusive_Max_Ulong) {
  doReductionTest<cl_ulong>(scanMaxRefFn<cl_ulong, false>);
}

TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_12_Scan_Inclusive_Add_Int) {
  doReductionTest<cl_int, true>(scanAddRefFn<cl_int, true>);
}
TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_12_Scan_Inclusive_Add_Uint) {
  doReductionTest<cl_uint, true>(scanAddRefFn<cl_uint, true>);
}
TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_12_Scan_Inclusive_Add_Long) {
  doReductionTest<cl_long, true>(scanAddRefFn<cl_long, true>);
}
TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_12_Scan_Inclusive_Add_Ulong) {
  doReductionTest<cl_ulong, true>(scanAddRefFn<cl_ulong, true>);
}

TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_13_Scan_Inclusive_Min_Int) {
  doReductionTest<cl_int>(scanMinRefFn<cl_int, true>);
}
TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_13_Scan_Inclusive_Min_Uint) {
  doReductionTest<cl_uint>(scanMinRefFn<cl_uint, true>);
}
TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_13_Scan_Inclusive_Min_Long) {
  doReductionTest<cl_long>(scanMinRefFn<cl_long, true>);
}
TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_13_Scan_Inclusive_Min_Ulong) {
  doReductionTest<cl_ulong>(scanMinRefFn<cl_ulong, true>);
}

TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_14_Scan_Inclusive_Max_Int) {
  doReductionTest<cl_int>(scanMaxRefFn<cl_int, true>);
}
TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_14_Scan_Inclusive_Max_Uint) {
  doReductionTest<cl_uint>(scanMaxRefFn<cl_uint, true>);
}
TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_14_Scan_Inclusive_Max_Long) {
  doReductionTest<cl_long>(scanMaxRefFn<cl_long, true>);
}
TEST_P(WorkGroupCollectiveScans,
       Work_Group_Collective_Functions_14_Scan_Inclusive_Max_Ulong) {
  doReductionTest<cl_ulong>(scanMaxRefFn<cl_ulong, true>);
}

static const NDRange local_sizes[] = {
    NDRange{64u, 1u, 1u}, NDRange{1u, 64u, 1u}, NDRange{1u, 1u, 64u},
    NDRange{67u, 1u, 1u}, NDRange{67u, 5u, 1u}, NDRange{67u, 2u, 3u},
};

static const kts::ucl::SourceType source_types[] = {
    kts::ucl::OPENCL_C, kts::ucl::OFFLINE, kts::ucl::SPIRV,
    kts::ucl::OFFLINESPIRV};

UCL_EXECUTION_TEST_SUITE_P(
    WorkGroupCollectiveAnyAll, testing::ValuesIn(source_types),
    // Test any/all on all-false, all-true, and random values
    testing::Combine(testing::ValuesIn(local_sizes),
                     testing::Values(0, 42, cargo::nullopt)))

UCL_EXECUTION_TEST_SUITE_P(WorkGroupCollectiveBroadcast1D,
                           testing::ValuesIn(source_types),
                           testing::Values(NDRange{64u, 1u, 1u},
                                           NDRange{67u, 1u, 1u}))

UCL_EXECUTION_TEST_SUITE_P(
    WorkGroupCollectiveBroadcast2D, testing::ValuesIn(source_types),
    testing::Values(NDRange{64u, 1u, 1u}, NDRange{1u, 64u, 1u},
                    NDRange{67u, 1u, 1u}, NDRange{67u, 5u, 1u}))

UCL_EXECUTION_TEST_SUITE_P(WorkGroupCollectiveBroadcast3D,
                           testing::ValuesIn(source_types),
                           testing::ValuesIn(local_sizes))

UCL_EXECUTION_TEST_SUITE_P(WorkGroupCollectiveScans,
                           testing::ValuesIn(source_types),
                           testing::ValuesIn(local_sizes))

UCL_EXECUTION_TEST_SUITE_P(WorkGroupCollectiveReductions,
                           testing::ValuesIn(source_types),
                           testing::ValuesIn(local_sizes))
