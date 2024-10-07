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

#include <gtest/gtest.h>

#include <initializer_list>
#include <numeric>
#include <vector>

#include "kts/execution.h"

class NDimensions {
 public:
  // The 'active' work group will write out 1s, all other work groups will write
  // out 0s.
  const std::vector<size_t> global;
  const std::vector<size_t> local;
  const std::vector<cl_uint> active;

  NDimensions(std::initializer_list<size_t> g, std::initializer_list<size_t> l,
              std::initializer_list<cl_uint> a)
      : global(g), local(l), active(a) {};

  // If there is no active work group.
  NDimensions(std::initializer_list<size_t> g, std::initializer_list<size_t> l)
      : global(g), local(l), active({0, 0, 0}) {};

  size_t size() const {
    // Calculate product.
    return std::accumulate(global.begin(), global.end(), static_cast<size_t>(1),
                           std::multiplies<size_t>());
  }
  cl_uint dims() const { return static_cast<cl_uint>(global.size()); }

  cl_int expected(size_t idx) const {
    const size_t idx_x = idx % global[0];
    const size_t group_x = idx_x / local[0];
    if (group_x != active[0]) {
      return 0;
    }

    if (dims() > 1) {
      const size_t idx_y = (idx / global[0]) % global[1];
      const size_t group_y = idx_y / local[1];
      if (group_y != active[1]) {
        return 0;
      }
    }

    if (dims() > 2) {
      const size_t idx_z = (idx / (global[0] * global[1])) % global[2];
      const size_t group_z = idx_z / local[2];
      if (group_z != active[2]) {
        return 0;
      }
    }

    return 1;
  }
};

template <class T>
static std::ostream &operator<<(std::ostream &out, const std::vector<T> &v) {
  out << "{";
  for (size_t index = 0, count = v.size(); index < count; index++) {
    out << v[index];
    if (index < count - 1) {
      out << ", ";
    }
  }
  out << "}";
  return out;
}

static std::ostream &operator<<(std::ostream &out, const NDimensions &nd) {
  out << "NDimensions{"
      << ".global" << nd.global << ", "
      << ".local" << nd.local << ", "
      << ".active" << nd.active << "}";
  return out;
}

using LocalDimensionTests1D = kts::ucl::ExecutionWithParam<NDimensions>;
TEST_P(LocalDimensionTests1D, Dimension_01_Single_Group_1D) {
  // Whether or not the kernel will be vectorized at a local size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  NDimensions dim = getParam();
  ASSERT_EQ(1u, dim.dims());

  kts::Reference1D<cl_int> refOut = [&dim](size_t x) {
    return dim.expected(x);
  };

  AddOutputBuffer(dim.size(), kts::Reference1D<cl_int>(refOut));
  AddPrimitive<cl_uint>(dim.active[0]);

  RunGenericND(dim.dims(), dim.global.data(), dim.local.data());
}

using LocalDimensionTests2D = kts::ucl::ExecutionWithParam<NDimensions>;
TEST_P(LocalDimensionTests2D, Dimension_02_Single_Group_2D) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  NDimensions dim = getParam();
  ASSERT_EQ(2u, dim.dims());

  kts::Reference1D<cl_int> refOut = [&dim](size_t x) {
    return dim.expected(x);
  };

  AddOutputBuffer(dim.size(), kts::Reference1D<cl_int>(refOut));
  AddPrimitive<cl_uint>(dim.active[0]);
  AddPrimitive<cl_uint>(dim.active[1]);

  RunGenericND(dim.dims(), dim.global.data(), dim.local.data());
}

using LocalDimensionTests3D = kts::ucl::ExecutionWithParam<NDimensions>;
TEST_P(LocalDimensionTests3D, Dimension_03_Single_Group_3D) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  NDimensions dim = getParam();
  ASSERT_EQ(3u, dim.dims());

  kts::Reference1D<cl_int> refOut = [&dim](size_t x) {
    return dim.expected(x);
  };

  AddOutputBuffer(dim.size(), kts::Reference1D<cl_int>(refOut));
  AddPrimitive<cl_uint>(dim.active[0]);
  AddPrimitive<cl_uint>(dim.active[1]);
  AddPrimitive<cl_uint>(dim.active[2]);

  RunGenericND(dim.dims(), dim.global.data(), dim.local.data());
}

UCL_EXECUTION_TEST_SUITE_P(
    LocalDimensionTests1D, testing::Values(kts::ucl::OPENCL_C),
    testing::Values(
        NDimensions({4u}, {1u}, {0u}), NDimensions({4u}, {1u}, {1u}),
        NDimensions({4u}, {1u}, {2u}), NDimensions({4u}, {1u}, {3u}),
        NDimensions({8u}, {2u}, {1u}), NDimensions({8u}, {4u}, {1u}),
        NDimensions({16u}, {2u}, {4u}), NDimensions({16u}, {4u}, {2u}),
        NDimensions({16u}, {8u}, {0u}), NDimensions({16u}, {8u}, {5u}),
        NDimensions({27u}, {9u}, {0u}), NDimensions({27u}, {9u}, {1u}),
        NDimensions({27u}, {9u}, {2u}), NDimensions({32u}, {1u}, {17u}),
        NDimensions({32u}, {16u}, {1u}), NDimensions({32u}, {32u}, {0u}),
        NDimensions({4096u}, {1u}, {7u}), NDimensions({4096u}, {2u}, {7u}),
        NDimensions({4096u}, {4u}, {7u}), NDimensions({4096u}, {8u}, {7u}),
        NDimensions({4096u}, {16u}, {7u}), NDimensions({4096u}, {32u}, {7u})))

UCL_EXECUTION_TEST_SUITE_P(
    LocalDimensionTests2D, testing::Values(kts::ucl::OPENCL_C),
    testing::Values(NDimensions({32u, 16u}, {16u, 8u}, {0u, 0u}),
                    NDimensions({32u, 16u}, {16u, 8u}, {0u, 1u}),
                    NDimensions({32u, 16u}, {16u, 8u}, {1u, 0u}),
                    NDimensions({32u, 16u}, {16u, 8u}, {1u, 1u}),
                    NDimensions({21u, 27u}, {7u, 9u}, {0u, 0u}),
                    NDimensions({21u, 27u}, {7u, 9u}, {1u, 1u}),
                    NDimensions({21u, 27u}, {7u, 9u}, {2u, 2u}),
                    NDimensions({64u, 64u}, {1u, 32u}, {0u, 0u}),
                    NDimensions({64u, 64u}, {2u, 16u}, {1u, 1u}),
                    NDimensions({64u, 64u}, {4u, 8u}, {1u, 1u}),
                    NDimensions({64u, 64u}, {8u, 4u}, {1u, 1u}),
                    NDimensions({64u, 64u}, {16u, 2u}, {1u, 1u}),
                    NDimensions({64u, 64u}, {32u, 1u}, {0u, 0u}),
                    NDimensions({128u, 64u}, {2u, 1u}, {3u, 3u}),
                    NDimensions({128u, 64u}, {4u, 2u}, {3u, 3u}),
                    NDimensions({128u, 64u}, {8u, 4u}, {3u, 3u}),
                    NDimensions({128u, 64u}, {16u, 8u}, {3u, 3u}),
                    NDimensions({128u, 64u}, {32u, 16u}, {3u, 3u}),
                    NDimensions({16u, 8u}, {2u, 4u}, {1u, 1u})))

UCL_EXECUTION_TEST_SUITE_P(
    LocalDimensionTests3D, testing::Values(kts::ucl::OPENCL_C),
    testing::Values(NDimensions({4u, 4u, 4u}, {1u, 1u, 1u}, {0u, 0u, 0u}),
                    NDimensions({16u, 8u, 4u}, {2u, 4u, 1u}, {1u, 1u, 1u}),
                    NDimensions({8u, 8u, 8u}, {8u, 1u, 1u}, {0u, 1u, 1u}),
                    NDimensions({8u, 8u, 8u}, {4u, 2u, 1u}, {1u, 1u, 1u}),
                    NDimensions({8u, 8u, 8u}, {4u, 1u, 2u}, {1u, 1u, 1u}),
                    NDimensions({8u, 8u, 8u}, {2u, 4u, 1u}, {1u, 1u, 1u}),
                    NDimensions({8u, 8u, 8u}, {2u, 1u, 4u}, {1u, 1u, 1u}),
                    NDimensions({8u, 8u, 8u}, {2u, 2u, 2u}, {1u, 1u, 1u}),
                    NDimensions({8u, 8u, 8u}, {1u, 8u, 1u}, {1u, 0u, 1u}),
                    NDimensions({8u, 8u, 8u}, {1u, 4u, 2u}, {1u, 1u, 1u}),
                    NDimensions({8u, 8u, 8u}, {1u, 2u, 4u}, {1u, 1u, 1u}),
                    NDimensions({8u, 8u, 8u}, {1u, 1u, 8u}, {1u, 1u, 0u}),
                    NDimensions({32u, 32u, 32u}, {1u, 1u, 1u}, {17u, 13u, 21u}),
                    NDimensions({32u, 32u, 32u}, {2u, 2u, 2u}, {1u, 1u, 1u}),
                    NDimensions({32u, 32u, 32u}, {4u, 4u, 4u}, {1u, 1u, 1u}),
                    NDimensions({32u, 32u, 32u}, {8u, 8u, 8u}, {1u, 1u, 1u})))

// This test is intended to check that the total number of work-items executed
// is as it should be.
//
// Note that this tests assumes that atomics really are globally 'atomic'.
// Some interpretations of the OpenCL 1.2 spec suggest that this is not
// required.
using TotalWorkTests = kts::ucl::ExecutionWithParam<NDimensions>;
TEST_P(TotalWorkTests, Dimension_04_Total_Work_Single_Atomic) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  NDimensions dim = getParam();

  kts::Reference1D<cl_uint> refOut = [&](size_t) {
    return static_cast<cl_uint>(dim.size());
  };

  AddOutputBuffer(1, refOut);
  RunGenericND(dim.dims(), dim.global.data(), dim.local.data());
}

// This test is similar to the above, but for the [probably not conformant]
// case that atomics are not globally atomic we have a counter per work-item,
// and provide the expected total number of work-items as a parameter so that
// the list of counters can be accessed modulo the total.
//
// If too few work items are executed then some counters will be '0'.  If too
// many work items are executed then some work items will be '2' or higher.
// However, in the case that too many work items are executed (and only in the
// presense of that bug) this test is assuming that atomic operations are truly
// globally atomic across work group.  Thus it is concievable that this test
// may sometimes spuriously pass when it should fail if the OpenCL
// implementation executes too many work items on a hardware platform where
// atomics are not globally consistent.
TEST_P(TotalWorkTests, Dimension_05_Total_Work_Many_Atomics) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  const NDimensions dim = getParam();

  kts::Reference1D<cl_uint> refOut = [&](size_t) {
    return static_cast<cl_uint>(1);
  };

  AddOutputBuffer(dim.size(), refOut);
  AddPrimitive<cl_ulong>(dim.size());
  RunGenericND(dim.dims(), dim.global.data(), dim.local.data());
}

UCL_EXECUTION_TEST_SUITE_P(
    TotalWorkTests, testing::Values(kts::ucl::OPENCL_C),
    testing::Values(
        NDimensions({8192}, {1}), NDimensions({8192 / 4}, {4}),
        NDimensions({17L * 19L}, {1}), NDimensions({17L * 19L}, {17}),
        NDimensions({8192, 1}, {1, 1}), NDimensions({8192, 1}, {4, 1}),
        NDimensions({8192 / 4, 4}, {1, 1}), NDimensions({8192 / 4, 4}, {1, 2}),
        NDimensions({8192 / 4, 4}, {4, 1}), NDimensions({8192 / 4, 4}, {4, 2}),
        NDimensions({17L * 19L, 1}, {17, 1}),
        NDimensions({17L * 19L, 1}, {1, 1}), NDimensions({17, 19}, {17, 1}),
        NDimensions({17, 19}, {17, 19}), NDimensions({8192, 1, 1}, {1, 1, 1}),
        NDimensions({8192, 1, 1}, {8, 1, 1}),
        NDimensions({8192 / 2, 2, 1}, {1, 1, 1}),
        NDimensions({8192 / 2, 2, 1}, {1, 2, 1}),
        NDimensions({8192 / 2, 2, 1}, {8, 1, 1}),
        NDimensions({8192 / 2, 2, 1}, {8, 2, 1}),
        NDimensions({8192 / 4, 2, 2}, {1, 1, 1}),
        NDimensions({8192 / 4, 2, 2}, {1, 1, 2}),
        NDimensions({8192 / 4, 2, 2}, {1, 2, 1}),
        NDimensions({8192 / 4, 2, 2}, {1, 2, 2}),
        NDimensions({8192 / 4, 2, 2}, {8, 1, 1}),
        NDimensions({8192 / 4, 2, 2}, {8, 1, 2}),
        NDimensions({8192 / 4, 2, 2}, {8, 2, 1}),
        NDimensions({8192 / 4, 2, 2}, {8, 2, 2}),
        NDimensions({17L * 19L * 23L, 1, 1}, {1, 1, 1}),
        NDimensions({17L * 19L * 23L, 1, 1}, {17, 1, 1}),
        NDimensions({17L * 23L, 19, 1}, {1, 1, 1}),
        NDimensions({17L * 23L, 19, 1}, {1, 19, 1}),
        NDimensions({23, 19L * 17, 1}, {1, 19, 1}),
        NDimensions({17, 19, 23}, {1, 1, 1}),
        NDimensions({17, 19, 23}, {1, 1, 23}),
        NDimensions({1, 19, 23L * 17L}, {1, 1, 23})))
