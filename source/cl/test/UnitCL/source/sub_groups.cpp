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

// Contains all tests for the optional sub-group builtins of OpenCL C 3.0.

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>

#include "Common.h"
#include "kts/arguments_shared.h"
#include "kts/execution.h"
#include "kts/precision.h"
#include "kts/sub_group_helpers.h"
#include "ucl/checks.h"

using namespace kts::ucl;

namespace {
void GenerateOrderIndependentFloatData(std::vector<cl_float> &input_data) {
  // Testing work group collective reductions and scans for general floating
  // point input data is very difficult, because the ordering of the summation
  // is not strictly defined, so:
  //  * intermediate results can overflow, even when the total does not,
  //  * there is no analytical error bound on a floating point summation;
  //    pathological input data can give wildly inaccurate results.
  //
  // To work around this, we generate our floating point data from a limited
  // range of integers, which are guaranteed not to lose any bits of precision
  // during addition, and the sum will be exact regardless of
  // ordering.
  const auto data_size = input_data.size();
  std::vector<cl_int> input_ints(data_size);
  ucl::Environment::instance->GetInputGenerator().GenerateIntData<cl_int>(
      input_ints, -65536, 65536);
  for (size_t i = 0; i != data_size; ++i) {
    input_data[i] = float(input_ints[i]) / 256.0f;
  }
}

}  // namespace

// Denotes a local work-group size used to execute sub-group tests below.
struct LocalSizes {
  LocalSizes(size_t _x, size_t _y, size_t _z) {
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
};

// Base class that makes the necessary checks for the existence of sub-groups in
// the OpenCL implementation.
class SubGroupTest : public kts::ucl::ExecutionWithParam<LocalSizes> {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(BaseExecution::SetUp());
    // Subgroups are a 3.0 features.
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }

    // Some of these tests run small local sizes, which we don't vectorize.
    // This is too coarse-grained, as there are some NDRanges which we can
    // vectorize.
    fail_if_not_vectorized_ = false;

    // clGetDeviceInfo may return 0, indicating that the device does not support
    // subgroups.
    cl_uint max_num_subgroups;
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_MAX_NUM_SUB_GROUPS,
                                   sizeof(cl_uint), &max_num_subgroups,
                                   nullptr));
    if (0 == max_num_subgroups) {
      GTEST_SKIP();
    }

    AddBuildOption("-cl-std=CL3.0");
    // We need to force a compilation early to ensure we have a kernel to query
    // sub-groups on.
    if (!BuildProgram()) {
      // Calling clGetKernelInfo without a valid kernel will cause the test to
      // fail before it can be skipped on an offline build (this would normally
      // happen in RunGenericND), so here we skip early.
      GTEST_SKIP();
    }
  }

  size_t getSubGroupCount(size_t local_size_x, size_t local_size_y,
                          size_t local_size_z) {
    size_t local_size[]{local_size_x, local_size_y, local_size_z};
    size_t sub_group_count = 0;
    const cl_int err = clGetKernelSubGroupInfo(
        kernel_, device, CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE,
        sizeof(local_size), local_size, sizeof(sub_group_count),
        &sub_group_count, nullptr);
    if (CL_SUCCESS != err) {
      Fail("Error while querying CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE", err);
      return 1;
    }
    return sub_group_count;
  }

  size_t getMaxSubGroupSize(size_t local_size_x, size_t local_size_y,
                            size_t local_size_z) {
    size_t local_size[]{local_size_x, local_size_y, local_size_z};
    size_t sub_group_count = 0;
    const cl_int err = clGetKernelSubGroupInfo(
        kernel_, device, CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE,
        sizeof(local_size), local_size, sizeof(sub_group_count),
        &sub_group_count, nullptr);
    if (CL_SUCCESS != err) {
      Fail("Error while querying CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE",
           err);
      return 1;
    }
    return sub_group_count;
  }
};

TEST_P(SubGroupTest, Sub_Group_01_Get_Sub_Group_Size_Builtin) {
  // The OpenCL spec says:
  //
  // All sub-groups must be the same size, while the last
  // subgroup in any work-group (i.e. the subgroup with the maximum index) could
  // be the same or smaller size.
  //
  // We can't know where the "last" sub-group will be in terms of work-items,
  // but we can know that sub-group size must be equal to the query retured by
  // CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE or local size %
  // CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE.
  //
  // Note: We could do even more here and use the sub_group_ids to check it is
  // actually the last sub-group that has a smaller size, or even check that
  // there is exactly CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE % local size
  // values in the output with the smaller size.
  auto local_sizes = getParam();
  const auto local_size = local_sizes.x() * local_sizes.y() * local_sizes.z();
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  kts::Reference1D<cl_uint> output_ref = [local_size, max_sub_group_size](
                                             size_t id, cl_uint sg_size) {
    (void)id;
    return (sg_size != 0) && ((sg_size == max_sub_group_size) ||
                              (sg_size == local_size % max_sub_group_size));
  };

  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  AddOutputBuffer(global_size, output_ref);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_02_Get_Max_Sub_Group_Size_Builtin) {
  // The OpenCL spec says:
  //
  // This value will be invariant for a given set of dispatch dimensions and a
  // kernel object compiled for a given device.
  //
  // So this value is uniform for all work-items in the nd-range and should
  // match the result returned by CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE.
  auto local_sizes = getParam();
  const auto sub_group_max_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  kts::Reference1D<cl_uint> output_ref = [sub_group_max_size](size_t) {
    return sub_group_max_size;
  };

  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  AddOutputBuffer(global_size, output_ref);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_03_Get_Num_Sub_Groups_Builtin) {
  // Note: This does not currently test non-uniform work-groups.
  auto local_sizes = getParam();
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  kts::Reference1D<cl_uint> output_ref = [sub_group_count](size_t) {
    return sub_group_count;
  };

  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  AddOutputBuffer(global_size, output_ref);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_04_Get_Enqueued_Num_Sub_Groups_Builtin) {
  auto local_sizes = getParam();
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  kts::Reference1D<cl_uint> output_ref = [sub_group_count](size_t) {
    return sub_group_count;
  };

  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  AddOutputBuffer(global_size, output_ref);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_05_Get_Sub_Group_Id_Builtin) {
  // Note: This testing could also be more rigourous. The implemenation is free
  // to map work-items to sub-groups however it pleases, but there are some
  // restrictions, e.g. the there should be sub-group size instances of each
  // sub-group id in each work-group.
  auto local_sizes = getParam();
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  kts::Reference1D<cl_uint> output_ref = [sub_group_count](size_t id,
                                                           cl_uint sgid) {
    (void)id;
    return sgid < sub_group_count;
  };

  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  AddOutputBuffer(global_size, output_ref);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_06_Get_Sub_Group_Local_Id_Builtin) {
  // Note: Similarly here. The local ID of each sub-group element must be unique
  // within a sub-group, here we only test it is in the correct range.
  // We also aren't testing that for the case that there is a non-uniform
  // sub-group where the range of local ids would be reduced.
  auto local_sizes = getParam();
  const auto sub_group_max_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  kts::Reference1D<cl_uint> output_ref = [sub_group_max_size](size_t id,
                                                              cl_uint sgid) {
    (void)id;
    return sgid < sub_group_max_size;
  };

  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  AddOutputBuffer(global_size, output_ref);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_07_Sub_Group_All_Builtin) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto sub_group_max_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  // There is an output buffer for each sub-group in each work-group. The last
  // sub-group may be smaller than the others.
  const auto output_buffer_size =
      (global_size / sub_group_max_size) + !!(global_size % sub_group_max_size);

  std::vector<cl_int> input_data(global_size);

  kts::Reference1D<cl_uint> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  std::map<size_t, std::vector<size_t>> sub_group_map;
  kts::Reference1D<size_t> output_ref_a =
      [&sub_group_map](size_t global_id, size_t sub_group_id) {
        // Record which gids are in each sub-group.
        sub_group_map[sub_group_id].push_back(global_id);
        return true;
      };

  kts::Reference1D<cl_int> output_ref_b = [&sub_group_map, &input_data](
                                              size_t sgid, cl_int result) {
    cl_int expected = 1;
    for (const size_t gid : sub_group_map[sgid]) {
      // Simulate the all operation on host.
      expected &= !!input_data[gid];
    }
    return !!result == !!expected;
  };

  // First an input of all false.
  input_data.assign(global_size, 0);
  sub_group_map.clear();
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(output_buffer_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);

  // Then an input of all true.
  input_data.assign(global_size, 42);
  sub_group_map.clear();
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(output_buffer_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);

  // Then a mix.
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  sub_group_map.clear();
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(output_buffer_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_08_Sub_Group_Any_Builtin) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto sub_group_max_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  // There is an output buffer for each sub-group in each work-group. The last
  // sub-group may be smaller than the others
  const auto output_buffer_size =
      (global_size / sub_group_max_size) + !!(global_size % sub_group_max_size);

  std::vector<cl_int> input_data(global_size);

  kts::Reference1D<cl_uint> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  std::map<size_t, std::vector<size_t>> sub_group_map;
  kts::Reference1D<size_t> output_ref_a =
      [&sub_group_map](size_t global_id, size_t sub_group_id) {
        // Record which gids are in each sub-group.
        sub_group_map[sub_group_id].push_back(global_id);
        return true;
      };

  kts::Reference1D<cl_int> output_ref_b = [&sub_group_map, &input_data](
                                              size_t sgid, cl_int result) {
    cl_int expected = 0;
    for (const size_t gid : sub_group_map[sgid]) {
      // Simulate the all operation on host.
      expected |= !!input_data[gid];
    }
    return !!result == !!expected;
  };

  // First an input of all false.
  input_data.assign(global_size, 0);
  sub_group_map.clear();
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(output_buffer_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);

  // Then an input of all true.
  input_data.assign(global_size, 42);
  sub_group_map.clear();
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(output_buffer_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);

  // Then a mix.
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  sub_group_map.clear();
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(output_buffer_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_09_Sub_Group_Broadcast_Uint) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto group_count = global_sizes[0] / local_sizes[0] * global_sizes[1] /
                           local_sizes[1] * global_sizes[2] / local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = group_count * sub_group_count;

  std::vector<cl_uint> input_data(global_size);
  std::vector<cl_uint> sub_group_local_ids(total_sub_group_count);

  kts::Reference1D<cl_uint> input_ref_a = [&input_data](size_t id) {
    return input_data[id];
  };
  kts::Reference1D<cl_uint> input_ref_b = [&sub_group_local_ids](size_t id) {
    return sub_group_local_ids[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_uint> output_ref_b = [&sub_group_global_id_global_ids_map,
                                            &global_id_sub_group_global_id_map,
                                            &input_data, &sub_group_local_ids](
                                               size_t gid, cl_uint result) {
    const auto sub_group_id = global_id_sub_group_global_id_map[gid];
    const auto &sub_group = sub_group_global_id_global_ids_map[sub_group_id];
    const auto sub_group_local_id = sub_group_local_ids[sub_group_id];
    const auto expected = input_data[sub_group[sub_group_local_id]];
    return result == expected;
  };

  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  std::generate(std::begin(sub_group_local_ids), std::end(sub_group_local_ids),
                [max_sub_group_size]() {
                  return ucl::Environment::instance->GetInputGenerator()
                      .GenerateInt<cl_uint>(0, max_sub_group_size - 1);
                });

  // We need to fill the last sub-group in each work-group seperately incase
  // it's a smaller sub-group because the sub-group size doesn't divide the
  // work-group size.
  if (auto remainder_sub_group_size = local_size % max_sub_group_size) {
    for (unsigned i = sub_group_count - 1; i < total_sub_group_count;
         i += sub_group_count) {
      sub_group_local_ids[i] =
          ucl::Environment::instance->GetInputGenerator().GenerateInt<cl_uint>(
              0, remainder_sub_group_size - 1);
    }
  }

  AddInputBuffer(global_size, input_ref_a);
  AddInputBuffer(total_sub_group_count, input_ref_b);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_09_Sub_Group_Broadcast_Int) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto group_count = global_sizes[0] / local_sizes[0] * global_sizes[1] /
                           local_sizes[1] * global_sizes[2] / local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = group_count * sub_group_count;

  std::vector<cl_int> input_data(global_size);
  std::vector<cl_uint> sub_group_local_ids(total_sub_group_count);

  kts::Reference1D<cl_int> input_ref_a = [&input_data](size_t id) {
    return input_data[id];
  };
  kts::Reference1D<cl_uint> input_ref_b = [&sub_group_local_ids](size_t id) {
    return sub_group_local_ids[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_int> output_ref_b = [&sub_group_global_id_global_ids_map,
                                           &global_id_sub_group_global_id_map,
                                           &input_data, &sub_group_local_ids](
                                              size_t gid, cl_int result) {
    const auto sub_group_id = global_id_sub_group_global_id_map[gid];
    const auto &sub_group = sub_group_global_id_global_ids_map[sub_group_id];
    const auto sub_group_local_id = sub_group_local_ids[sub_group_id];
    const auto expected = input_data[sub_group[sub_group_local_id]];
    return result == expected;
  };

  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  std::generate(std::begin(sub_group_local_ids), std::end(sub_group_local_ids),
                [max_sub_group_size]() {
                  return ucl::Environment::instance->GetInputGenerator()
                      .GenerateInt<cl_uint>(0, max_sub_group_size - 1);
                });

  // We need to fill the last sub-group in each work-group seperately incase
  // it's a smaller sub-group because the sub-group size doesn't divide the
  // work-group size.
  if (auto remainder_sub_group_size = local_size % max_sub_group_size) {
    for (unsigned i = sub_group_count - 1; i < total_sub_group_count;
         i += sub_group_count) {
      sub_group_local_ids[i] =
          ucl::Environment::instance->GetInputGenerator().GenerateInt<cl_uint>(
              0, remainder_sub_group_size - 1);
    }
  }

  AddInputBuffer(global_size, input_ref_a);
  AddInputBuffer(total_sub_group_count, input_ref_b);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_09_Sub_Group_Broadcast_Float) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto group_count = global_sizes[0] / local_sizes[0] * global_sizes[1] /
                           local_sizes[1] * global_sizes[2] / local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = group_count * sub_group_count;

  std::vector<cl_float> input_data(global_size);
  std::vector<cl_uint> sub_group_local_ids(total_sub_group_count);

  kts::Reference1D<cl_float> input_ref_a = [&input_data](size_t id) {
    return input_data[id];
  };
  kts::Reference1D<cl_uint> input_ref_b = [&sub_group_local_ids](size_t id) {
    return sub_group_local_ids[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_float> output_ref_b =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map,
       &input_data, &sub_group_local_ids, this](size_t gid, cl_float result) {
        const auto sub_group_id = global_id_sub_group_global_id_map[gid];
        const auto &sub_group =
            sub_group_global_id_global_ids_map[sub_group_id];
        const auto sub_group_local_id = sub_group_local_ids[sub_group_id];
        const auto expected = input_data[sub_group[sub_group_local_id]];
        return kts::ucl::ULPValidator<cl_float, 1_ULP>(this->device)
            .validate(expected, result);
      };

  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  std::generate(std::begin(sub_group_local_ids), std::end(sub_group_local_ids),
                [max_sub_group_size]() {
                  return ucl::Environment::instance->GetInputGenerator()
                      .GenerateInt<cl_uint>(0, max_sub_group_size - 1);
                });

  // We need to fill the last sub-group in each work-group seperately incase
  // it's a smaller sub-group because the sub-group size doesn't divide the
  // work-group size.
  if (auto remainder_sub_group_size = local_size % max_sub_group_size) {
    for (unsigned i = sub_group_count - 1; i < total_sub_group_count;
         i += sub_group_count) {
      sub_group_local_ids[i] =
          ucl::Environment::instance->GetInputGenerator().GenerateInt<cl_uint>(
              0, remainder_sub_group_size - 1);
    }
  }

  AddInputBuffer(global_size, input_ref_a);
  AddInputBuffer(total_sub_group_count, input_ref_b);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_10_Sub_Group_Reduce_Add_Uint) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto sub_group_max_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  // There is an output buffer for each sub-group in each work-group. The last
  // sub-group may be smaller than the others.
  const auto output_buffer_size =
      (global_size / sub_group_max_size) + !!(global_size % sub_group_max_size);

  std::vector<cl_int> input_data(global_size);

  kts::Reference1D<cl_uint> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  std::map<size_t, std::vector<size_t>> sub_group_map;
  kts::Reference1D<size_t> output_ref_a =
      [&sub_group_map](size_t global_id, size_t sub_group_id) {
        // Record which gids are in each sub-group.
        sub_group_map[sub_group_id].push_back(global_id);
        return true;
      };

  kts::Reference1D<cl_uint> output_ref_b = [&sub_group_map, &input_data](
                                               size_t sgid, cl_uint result) {
    cl_uint expected = 0;
    for (const size_t gid : sub_group_map[sgid]) {
      // Simulate the all operation on host.
      expected += input_data[gid];
    }
    return result == expected;
  };

  // A random selection of values.
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  sub_group_map.clear();
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(output_buffer_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);

  // Unsigned overflow is defined, so check that it has the correct roll over
  // semantics.
  std::fill_n(std::begin(input_data), input_data.size(),
              std::numeric_limits<cl_uint>::max() - 42);
  sub_group_map.clear();
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(output_buffer_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_10_Sub_Group_Reduce_Add_Int) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto sub_group_max_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  // There is an output buffer for each sub-group in each work-group. The last
  // sub-group may be smaller than the others.
  const auto output_buffer_size =
      (global_size / sub_group_max_size) + !!(global_size % sub_group_max_size);

  std::vector<cl_int> input_data(global_size);

  kts::Reference1D<cl_int> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  std::map<size_t, std::vector<size_t>> sub_group_map;
  kts::Reference1D<size_t> output_ref_a =
      [&sub_group_map](size_t global_id, size_t sub_group_id) {
        // Record which gids are in each sub-group.
        sub_group_map[sub_group_id].push_back(global_id);
        return true;
      };

  kts::Reference1D<cl_int> output_ref_b = [&sub_group_map, &input_data](
                                              size_t sgid, cl_int result) {
    cl_int expected = 0;
    for (const size_t gid : sub_group_map[sgid]) {
      // Simulate the all operation on host.
      expected += input_data[gid];
    }
    return result == expected;
  };

  // A random selection of values being careful to avoid the possibility of
  // overflow.
  // Note we need to cast sub_group_max_size to the same type as the numerator
  // here to avoid implicit integer promotion of min to unsigned long which
  // will cause min to exceed max.
  ucl::Environment::instance->GetInputGenerator().GenerateIntData<cl_int>(
      input_data,
      std::numeric_limits<cl_int>::min() /
          static_cast<cl_int>(sub_group_max_size),
      std::numeric_limits<cl_int>::max() /
          static_cast<cl_int>(sub_group_max_size));
  sub_group_map.clear();
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(output_buffer_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_10_Sub_Group_Reduce_Add_Float) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto sub_group_max_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  // There is an output buffer for each sub-group in each work-group. The last
  // sub-group may be smaller than the others
  const auto output_buffer_size =
      (global_size / sub_group_max_size) + !!(global_size % sub_group_max_size);

  std::vector<cl_float> input_data(global_size);

  kts::Reference1D<cl_float> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  std::map<size_t, std::vector<size_t>> sub_group_map;
  kts::Reference1D<size_t> output_ref_a =
      [&sub_group_map](size_t global_id, size_t sub_group_id) {
        // Record which gids are in each sub-group.
        sub_group_map[sub_group_id].push_back(global_id);
        return true;
      };

  kts::Reference1D<cl_float> output_ref_b = [&sub_group_map, &input_data, this](
                                                size_t sgid, cl_float result) {
    cl_float expected = 0;
    for (const size_t gid : sub_group_map[sgid]) {
      // Simulate the all operation on host.
      expected += input_data[gid];
    }
    return kts::ucl::ULPValidator<cl_float, 1_ULP>(this->device)
        .validate(expected, result);
  };

  GenerateOrderIndependentFloatData(input_data);
  sub_group_map.clear();
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(output_buffer_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_11_Sub_Group_Reduce_Min_Uint) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto sub_group_max_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  // There is an output buffer for each sub-group in each work-group. The last
  // sub-group may be smaller than the others.
  const auto output_buffer_size =
      (global_size / sub_group_max_size) + !!(global_size % sub_group_max_size);

  std::vector<cl_uint> input_data(global_size);

  kts::Reference1D<cl_uint> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  std::map<size_t, std::vector<size_t>> sub_group_map;
  kts::Reference1D<size_t> output_ref_a =
      [&sub_group_map](size_t global_id, size_t sub_group_id) {
        // Record which gids are in each sub-group.
        sub_group_map[sub_group_id].push_back(global_id);
        return true;
      };

  kts::Reference1D<cl_uint> output_ref_b = [&sub_group_map, &input_data](
                                               size_t sgid, cl_uint result) {
    cl_uint expected = CL_UINT_MAX;
    for (const size_t gid : sub_group_map[sgid]) {
      // Simulate the all operation on host.
      expected = std::min(expected, input_data[gid]);
    }
    return result == expected;
  };

  ucl::Environment::instance->GetInputGenerator().GenerateData<cl_uint>(
      input_data);
  sub_group_map.clear();
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(output_buffer_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_11_Sub_Group_Reduce_Min_Int) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto sub_group_max_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  // There is an output buffer for each sub-group in each work-group. The last
  // sub-group may be smaller than the others.
  const auto output_buffer_size =
      (global_size / sub_group_max_size) + !!(global_size % sub_group_max_size);

  std::vector<cl_int> input_data(global_size);

  kts::Reference1D<cl_int> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  std::map<size_t, std::vector<size_t>> sub_group_map;
  kts::Reference1D<size_t> output_ref_a =
      [&sub_group_map](size_t global_id, size_t sub_group_id) {
        // Record which gids are in each sub-group.
        sub_group_map[sub_group_id].push_back(global_id);
        return true;
      };

  kts::Reference1D<cl_int> output_ref_b = [&sub_group_map, &input_data](
                                              size_t sgid, cl_int result) {
    cl_int expected = CL_INT_MAX;
    for (const size_t gid : sub_group_map[sgid]) {
      // Simulate the all operation on host.
      expected = std::min(expected, input_data[gid]);
    }
    return result == expected;
  };

  ucl::Environment::instance->GetInputGenerator().GenerateData<cl_int>(
      input_data);
  sub_group_map.clear();
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(output_buffer_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_11_Sub_Group_Reduce_Min_Float) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto sub_group_max_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  // There is an output buffer for each sub-group in each work-group. The last
  // sub-group may be smaller than the others
  const auto output_buffer_size =
      (global_size / sub_group_max_size) + !!(global_size % sub_group_max_size);

  std::vector<cl_float> input_data(global_size);

  kts::Reference1D<cl_float> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  std::map<size_t, std::vector<size_t>> sub_group_map;
  kts::Reference1D<size_t> output_ref_a =
      [&sub_group_map](size_t global_id, size_t sub_group_id) {
        // Record which gids are in each sub-group.
        sub_group_map[sub_group_id].push_back(global_id);
        return true;
      };

  kts::Reference1D<cl_float> output_ref_b = [&sub_group_map, &input_data, this](
                                                size_t sgid, cl_float result) {
    if (sub_group_map[sgid].empty()) {
      return false;
    }
    cl_float expected = input_data[sub_group_map[sgid][0]];
    for (const size_t gid : sub_group_map[sgid]) {
      // Simulate the all operation on host.
      expected = fmin(expected, input_data[gid]);
    }
    return kts::ucl::ULPValidator<cl_float, 1_ULP>(this->device)
        .validate(expected, result);
  };

  {
    // Testing this for general floating point input data is very difficult,
    // because the ordering of the summation is not strictly defined, so:
    //  * intermediate results can overflow, even when the total does not,
    //  * there is no analytical error bound on a floating point summation;
    //    pathological input data can give wildly inaccurate results.
    //
    // To work around this, we generate our floating point data from a limited
    // range of integers, which are guaranteed not to lose any bits of
    // precision during addition, and the sum will be exact regardless of
    // ordering.
    std::vector<cl_int> input_ints(global_size);
    ucl::Environment::instance->GetInputGenerator().GenerateIntData<cl_int>(
        input_ints, -65536, 65536);
    for (size_t i = 0; i != global_size; ++i) {
      input_data[i] = float(input_ints[i]) / 256.0f;
    }
  }
  sub_group_map.clear();
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(output_buffer_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_12_Sub_Group_Reduce_Max_Uint) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto sub_group_max_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  // There is an output buffer for each sub-group in each work-group. The last
  // sub-group may be smaller than the others.
  const auto output_buffer_size =
      (global_size / sub_group_max_size) + !!(global_size % sub_group_max_size);

  std::vector<cl_uint> input_data(global_size);

  kts::Reference1D<cl_uint> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  std::map<size_t, std::vector<size_t>> sub_group_map;
  kts::Reference1D<size_t> output_ref_a =
      [&sub_group_map](size_t global_id, size_t sub_group_id) {
        // Record which gids are in each sub-group.
        sub_group_map[sub_group_id].push_back(global_id);
        return true;
      };

  kts::Reference1D<cl_uint> output_ref_b = [&sub_group_map, &input_data](
                                               size_t sgid, cl_uint result) {
    cl_uint expected = 0;
    for (const size_t gid : sub_group_map[sgid]) {
      // Simulate the all operation on host.
      expected = std::max(expected, input_data[gid]);
    }
    return result == expected;
  };

  ucl::Environment::instance->GetInputGenerator().GenerateData<cl_uint>(
      input_data);
  sub_group_map.clear();
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(output_buffer_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_12_Sub_Group_Reduce_Max_Int) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto sub_group_max_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  // There is an output buffer for each sub-group in each work-group. The last
  // sub-group may be smaller than the others.
  const auto output_buffer_size =
      (global_size / sub_group_max_size) + !!(global_size % sub_group_max_size);

  std::vector<cl_int> input_data(global_size);

  kts::Reference1D<cl_int> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  std::map<size_t, std::vector<size_t>> sub_group_map;
  kts::Reference1D<size_t> output_ref_a =
      [&sub_group_map](size_t global_id, size_t sub_group_id) {
        // Record which gids are in each sub-group.
        sub_group_map[sub_group_id].push_back(global_id);
        return true;
      };

  kts::Reference1D<cl_int> output_ref_b = [&sub_group_map, &input_data](
                                              size_t sgid, cl_int result) {
    cl_int expected = CL_INT_MIN;
    for (const size_t gid : sub_group_map[sgid]) {
      // Simulate the all operation on host.
      expected = std::max(expected, input_data[gid]);
    }
    return result == expected;
  };

  ucl::Environment::instance->GetInputGenerator().GenerateData<cl_int>(
      input_data);
  sub_group_map.clear();
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(output_buffer_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_12_Sub_Group_Reduce_Max_Float) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto sub_group_max_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  // There is an output buffer for each sub-group in each work-group. The last
  // sub-group may be smaller than the others.
  const auto output_buffer_size =
      (global_size / sub_group_max_size) + !!(global_size % sub_group_max_size);

  std::vector<cl_float> input_data(global_size);

  kts::Reference1D<cl_float> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  std::map<size_t, std::vector<size_t>> sub_group_map;
  kts::Reference1D<size_t> output_ref_a =
      [&sub_group_map](size_t global_id, size_t sub_group_id) {
        // Record which gids are in each sub-group.
        sub_group_map[sub_group_id].push_back(global_id);
        return true;
      };

  kts::Reference1D<cl_float> output_ref_b = [&sub_group_map, &input_data, this](
                                                size_t sgid, cl_float result) {
    if (sub_group_map[sgid].empty()) {
      return false;
    }
    cl_float expected = input_data[sub_group_map[sgid][0]];
    for (const size_t gid : sub_group_map[sgid]) {
      // Simulate the all operation on host.
      expected = fmax(expected, input_data[gid]);
    }
    return kts::ucl::ULPValidator<cl_float, 1_ULP>(this->device)
        .validate(expected, result);
  };

  ucl::Environment::instance->GetInputGenerator()
      .GenerateFiniteFloatData<cl_float>(input_data);
  sub_group_map.clear();
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(output_buffer_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_13_Sub_Group_Scan_Exclusive_Add_Uint) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto work_group_count = global_sizes[0] / local_sizes[0] *
                                global_sizes[1] / local_sizes[1] *
                                global_sizes[2] / local_sizes[2];
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = work_group_count * sub_group_count;

  std::vector<cl_uint> input_data(global_size);

  kts::Reference1D<cl_uint> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_uint> output_ref_b = [&sub_group_global_id_global_ids_map,
                                            &global_id_sub_group_global_id_map,
                                            &input_data](size_t gid,
                                                         cl_uint result) {
    const auto sub_group_id = global_id_sub_group_global_id_map[gid];
    const auto &sub_group = sub_group_global_id_global_ids_map[sub_group_id];
    const unsigned sub_group_local_id = std::distance(
        std::begin(sub_group),
        std::find(std::begin(sub_group), std::end(sub_group), gid));

    cl_uint expected = 0;
    for (unsigned i = 0; i < sub_group_local_id; ++i) {
      expected += input_data[sub_group[i]];
    }
    return result == expected;
  };

  // A random selection of values.
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);

  // Unsigned overflow is defined, so check that it has the correct roll over
  // semantics.
  std::fill_n(std::begin(input_data), input_data.size(),
              std::numeric_limits<cl_uint>::max() - 42);
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_13_Sub_Group_Scan_Exclusive_Add_Int) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto work_group_count = global_sizes[0] / local_sizes[0] *
                                global_sizes[1] / local_sizes[1] *
                                global_sizes[2] / local_sizes[2];
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = work_group_count * sub_group_count;

  std::vector<cl_int> input_data(global_size);

  kts::Reference1D<cl_int> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_int> output_ref_b = [&sub_group_global_id_global_ids_map,
                                           &global_id_sub_group_global_id_map,
                                           &input_data](size_t gid,
                                                        cl_int result) {
    const auto sub_group_id = global_id_sub_group_global_id_map[gid];
    const auto &sub_group = sub_group_global_id_global_ids_map[sub_group_id];
    const unsigned sub_group_local_id = std::distance(
        std::begin(sub_group),
        std::find(std::begin(sub_group), std::end(sub_group), gid));

    cl_int expected = 0;
    for (unsigned i = 0; i < sub_group_local_id; ++i) {
      expected += input_data[sub_group[i]];
    }
    return result == expected;
  };

  // A random selection of values being careful to avoid the possibility of
  // overflow.
  auto sub_group_max_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  // Note we need to cast sub_group_max_size to the same type as the numerator
  // here to avoid implicit integer promotion of min to unsigned long which
  // will cause min to exceed max.
  ucl::Environment::instance->GetInputGenerator().GenerateIntData<cl_int>(
      input_data,
      std::numeric_limits<cl_int>::min() /
          static_cast<cl_int>(sub_group_max_size),
      std::numeric_limits<cl_int>::max() /
          static_cast<cl_int>(sub_group_max_size));
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_13_Sub_Group_Scan_Exclusive_Add_Float) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto work_group_count = global_sizes[0] / local_sizes[0] *
                                global_sizes[1] / local_sizes[1] *
                                global_sizes[2] / local_sizes[2];
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = work_group_count * sub_group_count;

  std::vector<cl_float> input_data(global_size);

  kts::Reference1D<cl_float> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_float> output_ref_b =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map,
       &input_data, this](size_t gid, cl_float result) {
        const auto sub_group_id = global_id_sub_group_global_id_map[gid];
        const auto &sub_group =
            sub_group_global_id_global_ids_map[sub_group_id];
        const unsigned sub_group_local_id = std::distance(
            std::begin(sub_group),
            std::find(std::begin(sub_group), std::end(sub_group), gid));

        cl_float expected =
            sub_group_local_id > 0 ? input_data[sub_group[0]] : 0.0f;
        for (unsigned i = 1; i < sub_group_local_id; ++i) {
          expected += input_data[sub_group[i]];
        }
        return kts::ucl::ULPValidator<cl_float, 1_ULP>(this->device)
            .validate(expected, result);
      };

  GenerateOrderIndependentFloatData(input_data);
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_14_Sub_Group_Scan_Exclusive_Min_Uint) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto work_group_count = global_sizes[0] / local_sizes[0] *
                                global_sizes[1] / local_sizes[1] *
                                global_sizes[2] / local_sizes[2];
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = work_group_count * sub_group_count;

  std::vector<cl_uint> input_data(global_size);

  kts::Reference1D<cl_uint> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_uint> output_ref_b = [&sub_group_global_id_global_ids_map,
                                            &global_id_sub_group_global_id_map,
                                            &input_data](size_t gid,
                                                         cl_uint result) {
    const auto sub_group_id = global_id_sub_group_global_id_map[gid];
    const auto &sub_group = sub_group_global_id_global_ids_map[sub_group_id];
    const unsigned sub_group_local_id = std::distance(
        std::begin(sub_group),
        std::find(std::begin(sub_group), std::end(sub_group), gid));

    cl_uint expected = CL_UINT_MAX;
    for (unsigned i = 0; i < sub_group_local_id; ++i) {
      expected = std::min(expected, input_data[sub_group[i]]);
    }
    return result == expected;
  };

  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_14_Sub_Group_Scan_Exclusive_Min_Int) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto work_group_count = global_sizes[0] / local_sizes[0] *
                                global_sizes[1] / local_sizes[1] *
                                global_sizes[2] / local_sizes[2];
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = work_group_count * sub_group_count;

  std::vector<cl_int> input_data(global_size);

  kts::Reference1D<cl_int> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_int> output_ref_b = [&sub_group_global_id_global_ids_map,
                                           &global_id_sub_group_global_id_map,
                                           &input_data](size_t gid,
                                                        cl_int result) {
    const auto sub_group_id = global_id_sub_group_global_id_map[gid];
    const auto &sub_group = sub_group_global_id_global_ids_map[sub_group_id];
    const unsigned sub_group_local_id = std::distance(
        std::begin(sub_group),
        std::find(std::begin(sub_group), std::end(sub_group), gid));

    cl_int expected = CL_INT_MAX;
    for (unsigned i = 0; i < sub_group_local_id; ++i) {
      expected = std::min(expected, input_data[sub_group[i]]);
    }
    return result == expected;
  };

  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_14_Sub_Group_Scan_Exclusive_Min_Float) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto work_group_count = global_sizes[0] / local_sizes[0] *
                                global_sizes[1] / local_sizes[1] *
                                global_sizes[2] / local_sizes[2];
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = work_group_count * sub_group_count;

  std::vector<cl_float> input_data(global_size);

  kts::Reference1D<cl_float> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_float> output_ref_b =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map,
       &input_data, this](size_t gid, cl_float result) {
        const auto sub_group_id = global_id_sub_group_global_id_map[gid];
        const auto &sub_group =
            sub_group_global_id_global_ids_map[sub_group_id];
        const unsigned sub_group_local_id = std::distance(
            std::begin(sub_group),
            std::find(std::begin(sub_group), std::end(sub_group), gid));

        cl_float expected =
            sub_group_local_id > 0 ? input_data[sub_group[0]] : CL_INFINITY;
        for (unsigned i = 1; i < sub_group_local_id; ++i) {
          expected = fmin(expected, input_data[sub_group[i]]);
        }
        return kts::ucl::ULPValidator<cl_float, 1_ULP>(this->device)
            .validate(expected, result);
      };

  ucl::Environment::instance->GetInputGenerator()
      .GenerateFiniteFloatData<cl_float>(input_data);
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_15_Sub_Group_Scan_Exclusive_Max_Uint) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto work_group_count = global_sizes[0] / local_sizes[0] *
                                global_sizes[1] / local_sizes[1] *
                                global_sizes[2] / local_sizes[2];
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = work_group_count * sub_group_count;

  std::vector<cl_uint> input_data(global_size);

  kts::Reference1D<cl_uint> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_uint> output_ref_b = [&sub_group_global_id_global_ids_map,
                                            &global_id_sub_group_global_id_map,
                                            &input_data](size_t gid,
                                                         cl_uint result) {
    const auto sub_group_id = global_id_sub_group_global_id_map[gid];
    const auto &sub_group = sub_group_global_id_global_ids_map[sub_group_id];
    const unsigned sub_group_local_id = std::distance(
        std::begin(sub_group),
        std::find(std::begin(sub_group), std::end(sub_group), gid));

    cl_uint expected = 0;
    for (unsigned i = 0; i < sub_group_local_id; ++i) {
      expected = std::max(expected, input_data[sub_group[i]]);
    }
    return result == expected;
  };

  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_15_Sub_Group_Scan_Exclusive_Max_Int) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto work_group_count = global_sizes[0] / local_sizes[0] *
                                global_sizes[1] / local_sizes[1] *
                                global_sizes[2] / local_sizes[2];
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = work_group_count * sub_group_count;

  std::vector<cl_int> input_data(global_size);

  kts::Reference1D<cl_int> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_int> output_ref_b = [&sub_group_global_id_global_ids_map,
                                           &global_id_sub_group_global_id_map,
                                           &input_data](size_t gid,
                                                        cl_int result) {
    const auto sub_group_id = global_id_sub_group_global_id_map[gid];
    const auto &sub_group = sub_group_global_id_global_ids_map[sub_group_id];
    const unsigned sub_group_local_id = std::distance(
        std::begin(sub_group),
        std::find(std::begin(sub_group), std::end(sub_group), gid));

    cl_int expected = CL_INT_MIN;
    for (unsigned i = 0; i < sub_group_local_id; ++i) {
      expected = std::max(expected, input_data[sub_group[i]]);
    }
    return result == expected;
  };

  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_15_Sub_Group_Scan_Exclusive_Max_Float) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto work_group_count = global_sizes[0] / local_sizes[0] *
                                global_sizes[1] / local_sizes[1] *
                                global_sizes[2] / local_sizes[2];
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = work_group_count * sub_group_count;

  std::vector<cl_float> input_data(global_size);

  kts::Reference1D<cl_float> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_float> output_ref_b =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map,
       &input_data, this](size_t gid, cl_float result) {
        const auto sub_group_id = global_id_sub_group_global_id_map[gid];
        const auto &sub_group =
            sub_group_global_id_global_ids_map[sub_group_id];
        const unsigned sub_group_local_id = std::distance(
            std::begin(sub_group),
            std::find(std::begin(sub_group), std::end(sub_group), gid));

        cl_float expected =
            sub_group_local_id > 0 ? input_data[sub_group[0]] : -CL_INFINITY;
        for (unsigned i = 1; i < sub_group_local_id; ++i) {
          expected = fmax(expected, input_data[sub_group[i]]);
        }
        return kts::ucl::ULPValidator<cl_float, 1_ULP>(this->device)
            .validate(expected, result);
      };

  ucl::Environment::instance->GetInputGenerator()
      .GenerateFiniteFloatData<cl_float>(input_data);
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_16_Sub_Group_Scan_Inclusive_Add_Uint) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto work_group_count = global_sizes[0] / local_sizes[0] *
                                global_sizes[1] / local_sizes[1] *
                                global_sizes[2] / local_sizes[2];
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = work_group_count * sub_group_count;

  std::vector<cl_uint> input_data(global_size);

  kts::Reference1D<cl_uint> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_uint> output_ref_b = [&sub_group_global_id_global_ids_map,
                                            &global_id_sub_group_global_id_map,
                                            &input_data](size_t gid,
                                                         cl_uint result) {
    const auto sub_group_id = global_id_sub_group_global_id_map[gid];
    const auto &sub_group = sub_group_global_id_global_ids_map[sub_group_id];
    const unsigned sub_group_local_id = std::distance(
        std::begin(sub_group),
        std::find(std::begin(sub_group), std::end(sub_group), gid));

    cl_uint expected = 0;
    for (unsigned i = 0; i <= sub_group_local_id; ++i) {
      expected += input_data[sub_group[i]];
    }
    return result == expected;
  };

  // A random selection of values.
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);

  // Unsigned overflow is defined, so check that it has the correct roll over
  // semantics.
  std::fill_n(std::begin(input_data), input_data.size(),
              std::numeric_limits<cl_uint>::max() - 42);
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_16_Sub_Group_Scan_Inclusive_Add_Int) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto work_group_count = global_sizes[0] / local_sizes[0] *
                                global_sizes[1] / local_sizes[1] *
                                global_sizes[2] / local_sizes[2];
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = work_group_count * sub_group_count;

  std::vector<cl_int> input_data(global_size);

  kts::Reference1D<cl_int> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_int> output_ref_b = [&sub_group_global_id_global_ids_map,
                                           &global_id_sub_group_global_id_map,
                                           &input_data](size_t gid,
                                                        cl_int result) {
    const auto sub_group_id = global_id_sub_group_global_id_map[gid];
    const auto &sub_group = sub_group_global_id_global_ids_map[sub_group_id];
    const unsigned sub_group_local_id = std::distance(
        std::begin(sub_group),
        std::find(std::begin(sub_group), std::end(sub_group), gid));

    cl_int expected = 0;
    for (unsigned i = 0; i <= sub_group_local_id; ++i) {
      expected += input_data[sub_group[i]];
    }
    return result == expected;
  };

  // A random selection of values being careful to avoid the possibility of
  // overflow.
  auto sub_group_max_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  // Note we need to cast sub_group_max_size to the same type as the numerator
  // here to avoid implicit integer promotion of min to unsigned long which
  // will cause min to exceed max.
  ucl::Environment::instance->GetInputGenerator().GenerateIntData<cl_int>(
      input_data,
      std::numeric_limits<cl_int>::min() /
          static_cast<cl_int>(sub_group_max_size),
      std::numeric_limits<cl_int>::max() /
          static_cast<cl_int>(sub_group_max_size));
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_16_Sub_Group_Scan_Inclusive_Add_Float) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto work_group_count = global_sizes[0] / local_sizes[0] *
                                global_sizes[1] / local_sizes[1] *
                                global_sizes[2] / local_sizes[2];
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = work_group_count * sub_group_count;

  std::vector<cl_float> input_data(global_size);

  kts::Reference1D<cl_float> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_float> output_ref_b =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map,
       &input_data, this](size_t gid, cl_float result) {
        const auto sub_group_id = global_id_sub_group_global_id_map[gid];
        const auto &sub_group =
            sub_group_global_id_global_ids_map[sub_group_id];
        const unsigned sub_group_local_id = std::distance(
            std::begin(sub_group),
            std::find(std::begin(sub_group), std::end(sub_group), gid));

        cl_float expected = input_data[sub_group[0]];
        for (unsigned i = 1; i <= sub_group_local_id; ++i) {
          expected += input_data[sub_group[i]];
        }
        return kts::ucl::ULPValidator<cl_float, 1_ULP>(this->device)
            .validate(expected, result);
      };

  GenerateOrderIndependentFloatData(input_data);
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_17_Sub_Group_Scan_Inclusive_Min_Uint) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto work_group_count = global_sizes[0] / local_sizes[0] *
                                global_sizes[1] / local_sizes[1] *
                                global_sizes[2] / local_sizes[2];
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = work_group_count * sub_group_count;

  std::vector<cl_uint> input_data(global_size);

  kts::Reference1D<cl_uint> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_uint> output_ref_b = [&sub_group_global_id_global_ids_map,
                                            &global_id_sub_group_global_id_map,
                                            &input_data](size_t gid,
                                                         cl_uint result) {
    const auto sub_group_id = global_id_sub_group_global_id_map[gid];
    const auto &sub_group = sub_group_global_id_global_ids_map[sub_group_id];
    const unsigned sub_group_local_id = std::distance(
        std::begin(sub_group),
        std::find(std::begin(sub_group), std::end(sub_group), gid));

    cl_uint expected = CL_UINT_MAX;
    for (unsigned i = 0; i <= sub_group_local_id; ++i) {
      expected = std::min(expected, input_data[sub_group[i]]);
    }
    return result == expected;
  };

  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_17_Sub_Group_Scan_Inclusive_Min_Int) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto work_group_count = global_sizes[0] / local_sizes[0] *
                                global_sizes[1] / local_sizes[1] *
                                global_sizes[2] / local_sizes[2];
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = work_group_count * sub_group_count;

  std::vector<cl_int> input_data(global_size);

  kts::Reference1D<cl_int> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_int> output_ref_b = [&sub_group_global_id_global_ids_map,
                                           &global_id_sub_group_global_id_map,
                                           &input_data](size_t gid,
                                                        cl_int result) {
    const auto sub_group_id = global_id_sub_group_global_id_map[gid];
    const auto &sub_group = sub_group_global_id_global_ids_map[sub_group_id];
    const unsigned sub_group_local_id = std::distance(
        std::begin(sub_group),
        std::find(std::begin(sub_group), std::end(sub_group), gid));

    cl_int expected = CL_INT_MAX;
    for (unsigned i = 0; i <= sub_group_local_id; ++i) {
      expected = std::min(expected, input_data[sub_group[i]]);
    }
    return result == expected;
  };

  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_17_Sub_Group_Scan_Inclusive_Min_Float) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto work_group_count = global_sizes[0] / local_sizes[0] *
                                global_sizes[1] / local_sizes[1] *
                                global_sizes[2] / local_sizes[2];
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = work_group_count * sub_group_count;

  std::vector<cl_float> input_data(global_size);

  kts::Reference1D<cl_float> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_float> output_ref_b =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map,
       &input_data, this](size_t gid, cl_float result) {
        const auto sub_group_id = global_id_sub_group_global_id_map[gid];
        const auto &sub_group =
            sub_group_global_id_global_ids_map[sub_group_id];
        const unsigned sub_group_local_id = std::distance(
            std::begin(sub_group),
            std::find(std::begin(sub_group), std::end(sub_group), gid));

        cl_float expected = input_data[sub_group[0]];
        for (unsigned i = 1; i <= sub_group_local_id; ++i) {
          expected = fmin(expected, input_data[sub_group[i]]);
        }
        return kts::ucl::ULPValidator<cl_float, 1_ULP>(this->device)
            .validate(expected, result);
      };

  ucl::Environment::instance->GetInputGenerator()
      .GenerateFiniteFloatData<cl_float>(input_data);
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_18_Sub_Group_Scan_Inclusive_Max_Uint) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto work_group_count = global_sizes[0] / local_sizes[0] *
                                global_sizes[1] / local_sizes[1] *
                                global_sizes[2] / local_sizes[2];
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = work_group_count * sub_group_count;

  std::vector<cl_uint> input_data(global_size);

  kts::Reference1D<cl_uint> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_uint> output_ref_b = [&sub_group_global_id_global_ids_map,
                                            &global_id_sub_group_global_id_map,
                                            &input_data](size_t gid,
                                                         cl_uint result) {
    const auto sub_group_id = global_id_sub_group_global_id_map[gid];
    const auto &sub_group = sub_group_global_id_global_ids_map[sub_group_id];
    const unsigned sub_group_local_id = std::distance(
        std::begin(sub_group),
        std::find(std::begin(sub_group), std::end(sub_group), gid));

    cl_uint expected = 0;
    for (unsigned i = 0; i <= sub_group_local_id; ++i) {
      expected = std::max(expected, input_data[sub_group[i]]);
    }
    return result == expected;
  };

  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_18_Sub_Group_Scan_Inclusive_Max_Int) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto work_group_count = global_sizes[0] / local_sizes[0] *
                                global_sizes[1] / local_sizes[1] *
                                global_sizes[2] / local_sizes[2];
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = work_group_count * sub_group_count;

  std::vector<cl_int> input_data(global_size);

  kts::Reference1D<cl_int> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_int> output_ref_b = [&sub_group_global_id_global_ids_map,
                                           &global_id_sub_group_global_id_map,
                                           &input_data](size_t gid,
                                                        cl_int result) {
    const auto sub_group_id = global_id_sub_group_global_id_map[gid];
    const auto &sub_group = sub_group_global_id_global_ids_map[sub_group_id];
    const unsigned sub_group_local_id = std::distance(
        std::begin(sub_group),
        std::find(std::begin(sub_group), std::end(sub_group), gid));

    cl_int expected = CL_INT_MIN;
    for (unsigned i = 0; i <= sub_group_local_id; ++i) {
      expected = std::max(expected, input_data[sub_group[i]]);
    }
    return result == expected;
  };

  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

TEST_P(SubGroupTest, Sub_Group_18_Sub_Group_Scan_Inclusive_Max_Float) {
  auto local_sizes = getParam();
  size_t global_sizes[]{local_sizes.x() * 4, local_sizes.y(), local_sizes.z()};
  const auto global_size = global_sizes[0] * global_sizes[1] * global_sizes[2];
  const auto local_size = local_sizes[0] * local_sizes[1] * local_sizes[2];
  const auto max_sub_group_size =
      getMaxSubGroupSize(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto work_group_count = global_sizes[0] / local_sizes[0] *
                                global_sizes[1] / local_sizes[1] *
                                global_sizes[2] / local_sizes[2];
  const auto sub_group_count =
      getSubGroupCount(local_sizes.x(), local_sizes.y(), local_sizes.z());
  const auto total_sub_group_count = work_group_count * sub_group_count;

  std::vector<cl_float> input_data(global_size);

  kts::Reference1D<cl_float> input_ref = [&input_data](size_t id) {
    return input_data[id];
  };

  // Maps the global id of each sub-group to the global ids of work-items in
  // that sub-group.
  SubGroupGlobalIdGlobalIdsMap sub_group_global_id_global_ids_map(
      total_sub_group_count, {{}});
  for (unsigned sub_group = 0; sub_group < total_sub_group_count; ++sub_group) {
    // The last sub-group in each work-group could be smaller.
    size_t sub_group_size = max_sub_group_size;
    if (sub_group == sub_group_count - 1 && local_size % max_sub_group_size) {
      sub_group_size = local_size % max_sub_group_size;
    }
    sub_group_global_id_global_ids_map[sub_group].assign(sub_group_size, 0);
  }

  // Maps the global id of each work-item sub-group to the global ids of its
  // sub-group.
  GlobalIdSubGroupGlobalIdMap global_id_sub_group_global_id_map(global_size, 0);

  kts::Reference1D<cl_uint2> output_ref_a =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map](
          size_t global_id, cl_uint2 sub_group_info) {
        return mapSubGroupIds(global_id, sub_group_info,
                              global_id_sub_group_global_id_map,
                              sub_group_global_id_global_ids_map);
      };

  kts::Reference1D<cl_float> output_ref_b =
      [&sub_group_global_id_global_ids_map, &global_id_sub_group_global_id_map,
       &input_data, this](size_t gid, cl_float result) {
        const auto sub_group_id = global_id_sub_group_global_id_map[gid];
        const auto &sub_group =
            sub_group_global_id_global_ids_map[sub_group_id];
        const unsigned sub_group_local_id = std::distance(
            std::begin(sub_group),
            std::find(std::begin(sub_group), std::end(sub_group), gid));

        cl_float expected = input_data[sub_group[0]];
        for (unsigned i = 1; i <= sub_group_local_id; ++i) {
          expected = fmax(expected, input_data[sub_group[i]]);
        }
        return kts::ucl::ULPValidator<cl_float, 1_ULP>(this->device)
            .validate(expected, result);
      };

  ucl::Environment::instance->GetInputGenerator()
      .GenerateFiniteFloatData<cl_float>(input_data);
  AddInputBuffer(global_size, input_ref);
  AddOutputBuffer(global_size, output_ref_a);
  AddOutputBuffer(global_size, output_ref_b);
  RunGenericND(3, global_sizes, local_sizes);
}

UCL_EXECUTION_TEST_SUITE_P(
    SubGroupTest,
    testing::Values(kts::ucl::OPENCL_C, kts::ucl::OFFLINE, kts::ucl::SPIRV,
                    kts::ucl::OFFLINESPIRV),
    testing::Values(LocalSizes(64, 1, 1), LocalSizes(8, 8, 1),
                    LocalSizes(4, 4, 4),
                    // Local size of 1 on X-dimension won't vectorize
                    LocalSizes(1, 64, 1), LocalSizes(1, 1, 64),
                    // Edge case that local size is prime, in this case there is
                    // either 1 sub-group (sub-group == work-group), or local
                    // size sub-groups (sub-group == work-item) or one sub-group
                    // of size local size % sub group size.
                    LocalSizes(67, 1, 1),
                    // 2D edge case.
                    LocalSizes(67, 3, 1),
                    // 3D edge case.
                    LocalSizes(67, 2, 3)))
