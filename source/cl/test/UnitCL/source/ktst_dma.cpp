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

#include <cargo/utility.h>
#include <gtest/gtest.h>

#include <algorithm>

#include "Common.h"
#include "kts/execution.h"
#include "kts/precision.h"
#include "kts/reference_functions.h"

// This is needed for 'TEST_F'.
using namespace kts::ucl;

namespace {
const size_t local_wg_size = 16;

// Vector addition: C[x] = A[x] + B[x];
kts::Reference1D<cl_int> vaddInA = [](size_t x) {
  return (kts::Ref_Identity(x) * 3) + 27;
};

kts::Reference1D<cl_int> vaddInB = [](size_t x) {
  return (kts::Ref_Identity(x) * 7) + 41;
};

kts::Reference1D<cl_int> vaddOutC = [](size_t x) {
  return vaddInA(x) + vaddInB(x);
};

// Max: C[x] = max(A[x], B[x])
// Choose values of A and B such that if `cl_half` on host is actually an
// integral type, then interpreting those bits as a `half` on the device will
// never result in a denormal number.  Modulo 256 to provide numbers such that
// sometimes A is actually larger than B.  Use the `max` (or `fmax` on device)
// operator rather than some arithmetic as it will be consistent between host
// and device for deciding whether A or B is larger for a given `x` (even if it
// is an integral on host).
struct HalfTypeParam final {
  std::string type_str;
  const size_t vec_width;
  const size_t type_size;

  HalfTypeParam(size_t size, size_t width)
      : type_str("half"), vec_width(width), type_size(size) {
    if (vec_width != 1) {
      type_str.append(std::to_string(vec_width));
    }
  }

  static cl_half InA(size_t);
  static cl_half InB(size_t);
  static cl_half OutC(size_t);
};

cl_half HalfTypeParam::InA(size_t x) {
  const cl_ushort id =
      static_cast<cl_ushort>((kts::Ref_Identity(x) * 3 + 27) % 256);
  const cl_ushort as_ushort = TypeInfo<cl_half>::low_exp_mask + id;
  return cargo::bit_cast<cl_half>(as_ushort);
}

cl_half HalfTypeParam::InB(size_t x) {
  const cl_ushort id =
      static_cast<cl_ushort>((kts::Ref_Identity(x) * 7 + 41) % 256);
  const cl_ushort as_ushort = TypeInfo<cl_half>::low_exp_mask + id;
  return cargo::bit_cast<cl_half>(as_ushort);
}

cl_half HalfTypeParam::OutC(size_t x) {
  return std::max(HalfTypeParam::InA(x), HalfTypeParam::InB(x));
}

// Validates results from an output buffer contain half3 types. Since the
// `cl_half3` typedef aliases `cl_half4`, it's useful to define this without
// templates to avoid conflating the vector widths.
struct Half3Validator {
  bool validate(cl_half3 &expected, cl_half3 &actual) {
    kts::Validator<cl_half> v;
    return v.validate(expected.s[0], actual.s[0]) &&
           v.validate(expected.s[1], actual.s[1]) &&
           v.validate(expected.s[2], actual.s[2]);
  }

  void print(std::stringstream &s, const cl_half3 &value) {
    kts::Validator<cl_half> v;
    s << "<";
    v.print(s, value.s[0]);
    s << ",";
    v.print(s, value.s[1]);
    s << ",";
    v.print(s, value.s[2]);
    s << ">";
  }
};
using Half3StreamerTy = kts::GenericStreamer<cl_half3, Half3Validator>;

template <typename F>
std::shared_ptr<Half3StreamerTy> makeHalf3Streamer(F &&f) {
  auto ref = kts::BuildVec3Reference1D<cl_half3>(std::forward<F>(f));
  return std::make_shared<Half3StreamerTy>(ref);
}

}  // namespace

TEST_P(Execution, Dma_01_Direct) {
  AddInputBuffer(kts::N, vaddInA);
  AddInputBuffer(kts::N, vaddInB);
  AddOutputBuffer(kts::N, vaddOutC);
  RunGeneric1D(kts::N, local_wg_size);
}

TEST_P(Execution, Dma_02_Explicit_Copy) {
  AddLocalBuffer<cl_int>(local_wg_size);
  AddLocalBuffer<cl_int>(local_wg_size);
  AddInputBuffer(kts::N, vaddInA);
  AddInputBuffer(kts::N, vaddInB);
  AddOutputBuffer(kts::N, vaddOutC);
  RunGeneric1D(kts::N, local_wg_size);
}

TEST_P(Execution, Dma_03_Explicit_Copy_Rotate) {
  AddLocalBuffer<cl_int>(local_wg_size);
  AddLocalBuffer<cl_int>(local_wg_size);
  AddInputBuffer(kts::N, vaddInA);
  AddInputBuffer(kts::N, vaddInB);
  AddOutputBuffer(kts::N, vaddOutC);
  RunGeneric1D(kts::N, local_wg_size);
}

TEST_P(Execution, Dma_04_async_copy) {
  AddLocalBuffer<cl_int>(local_wg_size);
  AddLocalBuffer<cl_int>(local_wg_size);
  AddLocalBuffer<cl_int>(local_wg_size);
  AddInputBuffer(kts::N, vaddInA);
  AddInputBuffer(kts::N, vaddInB);
  AddOutputBuffer(kts::N, vaddOutC);
  RunGeneric1D(kts::N, local_wg_size);
}

// Each work item calculations the result for 'iterations' number of results,
// overlap compute and data-transfer using async_work_group_copy so double the
// size of the local buffers.
TEST_P(Execution, Dma_05_async_double_buffer) {
  const cl_int iterations = 16;
  AddLocalBuffer<cl_int>(local_wg_size);
  AddLocalBuffer<cl_int>(local_wg_size);
  AddLocalBuffer<cl_int>(local_wg_size);
  AddLocalBuffer<cl_int>(local_wg_size);
  AddLocalBuffer<cl_int>(local_wg_size);
  AddLocalBuffer<cl_int>(local_wg_size);
  AddInputBuffer(kts::N * iterations, vaddInA);
  AddInputBuffer(kts::N * iterations, vaddInB);
  AddOutputBuffer(kts::N * iterations, vaddOutC);
  AddPrimitive(iterations);
  RunGeneric1D(kts::N, local_wg_size);
}

static constexpr size_t GLOBAL_ITEMS_1D = 4;
static constexpr size_t GLOBAL_ITEMS_2D = 4;
static constexpr size_t LOCAL_ITEMS_1D = 2;
static constexpr size_t LOCAL_ITEMS_2D = 2;
static constexpr size_t GLOBAL_ITEMS_TOTAL = GLOBAL_ITEMS_1D * GLOBAL_ITEMS_2D;

class DmaAutoConvolutionExecute : public Execution {
 public:
  void DmaAutoConvolution(bool includeMiddle, cl_uint totalStart,
                          uint32_t maskLoop1, uint32_t maskLoop2,
                          bool extraParam) {
    const size_t global_range[] = {GLOBAL_ITEMS_1D, GLOBAL_ITEMS_2D};
    const size_t local_range[] = {LOCAL_ITEMS_1D, LOCAL_ITEMS_2D};

    const size_t srcWidth = GLOBAL_ITEMS_1D + 16;
    const size_t srcHeight = GLOBAL_ITEMS_2D + 8;
    kts::Reference1D<cl_uint> inA = [](size_t x) {
      return kts::Ref_Identity(x);
    };
    kts::Reference1D<cl_uint> RefOutput = [&](size_t x) {
      // First of all work out gidY and gidX
      const cl_uint gidX = kts::Ref_Identity(x) % GLOBAL_ITEMS_1D;
      const cl_uint gidY = kts::Ref_Identity(x) / GLOBAL_ITEMS_1D;
      const cl_uint gsizeX = GLOBAL_ITEMS_1D;
      cl_uint total = totalStart;
      const cl_uint dstYStride = gsizeX;
      const cl_uint srcYStride = dstYStride + 16;
      cl_uint srcIndex = (gidY * srcYStride) + gidX + 8;
      srcIndex += srcYStride;
      for (uint32_t yy = 0; yy < 3; yy++) {
        for (uint32_t xx = 0; xx < 3; xx++) {
          if (!includeMiddle && xx == 1 && yy == 1) {
            continue;
          }
          if (((1 << xx) & maskLoop1) && ((1 << xx) & maskLoop2)) {
            const cl_uint srcIndexLoop = (yy * srcYStride) + srcIndex + xx - 1;
            total = total + inA(srcIndexLoop);
          }
        }
      }
      total = total / (8 + (includeMiddle ? 1 : 0));

      return total;
    };
    AddInputBuffer(srcWidth * srcHeight, inA);
    AddOutputBuffer(GLOBAL_ITEMS_TOTAL, RefOutput);
    if (extraParam) {
      AddPrimitive(10);
    }
    RunGenericND(2, global_range, local_range);
  }
};

UCL_EXECUTION_TEST_SUITE(DmaAutoConvolutionExecute,
                         testing::ValuesIn(getSourceTypes()))

TEST_P(DmaAutoConvolutionExecute, Dma_06_auto_dma_convolution) {
  this->DmaAutoConvolution(false, 8, 7, 7, false);
}

TEST_P(DmaAutoConvolutionExecute, Dma_07_auto_dma_loop_convolution) {
  this->DmaAutoConvolution(true, 9, 7, 7, false);
}

TEST_P(DmaAutoConvolutionExecute, Dma_07_auto_dma_loop_convolution_looprotate) {
  this->DmaAutoConvolution(true, 9, 7, 7, false);
}

TEST_P(DmaAutoConvolutionExecute,
       Dma_08_auto_dma_loop_convolution_cond_round_inner_loop) {
  this->DmaAutoConvolution(true, 9, 7, 2, false);
}

TEST_P(DmaAutoConvolutionExecute,
       Dma_09_auto_dma_loop_convolution_cond_not_global_id) {
  this->DmaAutoConvolution(true, 19, 7, 7, true);
}

TEST_P(DmaAutoConvolutionExecute,
       Dma_09_auto_dma_loop_convolution_cond_not_global_id_looprotate) {
  this->DmaAutoConvolution(true, 19, 7, 7, true);
}

using AsyncCopyTests = ExecutionWithParam<HalfTypeParam>;

TEST_P(AsyncCopyTests, Dma_10_half_async_copy) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  // We test `async_workgroup_copy` for the different half types and their
  // respective sizes.
  const auto param = getParam();
  AddMacro("TYPE", param.type_str);

  AddLocalBuffer(local_wg_size, param.type_size);
  AddLocalBuffer(local_wg_size, param.type_size);
  AddLocalBuffer(local_wg_size, param.type_size);

  if (3 == param.vec_width) {
    AddInputBuffer(kts::N, makeHalf3Streamer(HalfTypeParam::InA));
    AddInputBuffer(kts::N, makeHalf3Streamer(HalfTypeParam::InB));
    AddOutputBuffer(kts::N, makeHalf3Streamer(HalfTypeParam::OutC));
  } else {
    const size_t global_buffer_len = kts::N * param.vec_width;
    AddInputBuffer(global_buffer_len,
                   kts::Reference1D<cl_half>(HalfTypeParam::InA));
    AddInputBuffer(global_buffer_len,
                   kts::Reference1D<cl_half>(HalfTypeParam::InB));
    AddOutputBuffer(global_buffer_len,
                    kts::Reference1D<cl_half>(HalfTypeParam::OutC));
  }

  RunGeneric1D(kts::N, local_wg_size);
}

TEST_P(AsyncCopyTests, Dma_11_half_async_strided_copy) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  // We test `async_workgroup_strided_copy` for the different half types and
  // their respective sizes.
  const auto param = getParam();
  AddMacro("TYPE", param.type_str);

  AddLocalBuffer(local_wg_size, param.type_size);
  AddLocalBuffer(local_wg_size, param.type_size);
  AddLocalBuffer(local_wg_size, param.type_size);

  if (3 == param.vec_width) {
    AddInputBuffer(kts::N * 2, makeHalf3Streamer(HalfTypeParam::InA));
    AddInputBuffer(kts::N * 2, makeHalf3Streamer(HalfTypeParam::InB));

    typedef cl_half (*half_ref_functor)(size_t);
    // Like HalfTypeParam::OutC, but aware of a stride of 2 used by the kernel.
    half_ref_functor outC = [](size_t x) {
      const size_t type_width = 3;
      const size_t gid = ((x / type_width) * type_width * 2) + (x % type_width);
      return std::max(HalfTypeParam::InA(gid), HalfTypeParam::InB(gid));
    };
    AddOutputBuffer(kts::N, makeHalf3Streamer(outC));
  } else {
    const size_t type_width = param.vec_width;

    // Like HalfTypeParam::OutC, but aware of a stride of 2 used by the kernel.
    auto outC = [&type_width](size_t x) -> cl_half {
      const size_t gid = ((x / type_width) * type_width * 2) + (x % type_width);
      return std::max(HalfTypeParam::InA(gid), HalfTypeParam::InB(gid));
    };

    const size_t global_buffer_len = kts::N * type_width;
    AddInputBuffer(global_buffer_len * 2,
                   kts::Reference1D<cl_half>(HalfTypeParam::InA));
    AddInputBuffer(global_buffer_len * 2,
                   kts::Reference1D<cl_half>(HalfTypeParam::InB));
    AddOutputBuffer(global_buffer_len, kts::Reference1D<cl_half>(outC));
  }
  RunGeneric1D(kts::N, local_wg_size);
}

TEST_P(AsyncCopyTests, Dma_12_half_prefetch) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  // We test `prefetch` for the different half types and their respective
  // sizes.
  const auto param = getParam();
  AddMacro("TYPE", param.type_str);

  if (3 == param.vec_width) {
    AddInputBuffer(kts::N, makeHalf3Streamer(HalfTypeParam::InA));
    AddInputBuffer(kts::N, makeHalf3Streamer(HalfTypeParam::InB));
    AddOutputBuffer(kts::N, makeHalf3Streamer(HalfTypeParam::OutC));
  } else {
    const size_t global_buffer_len = kts::N * param.vec_width;
    AddInputBuffer(global_buffer_len,
                   kts::Reference1D<cl_half>(HalfTypeParam::InA));
    AddInputBuffer(global_buffer_len,
                   kts::Reference1D<cl_half>(HalfTypeParam::InB));
    AddOutputBuffer(global_buffer_len,
                    kts::Reference1D<cl_half>(HalfTypeParam::OutC));
  }
  RunGeneric1D(kts::N, local_wg_size);
}

UCL_EXECUTION_TEST_SUITE_P(AsyncCopyTests, testing::Values(OPENCL_C),
                           testing::Values(HalfTypeParam(sizeof(cl_half), 1),
                                           HalfTypeParam(sizeof(cl_half2), 2),
                                           HalfTypeParam(sizeof(cl_half3), 3),
                                           HalfTypeParam(sizeof(cl_half4), 4),
                                           HalfTypeParam(sizeof(cl_half8), 8),
                                           HalfTypeParam(sizeof(cl_half16),
                                                         16)))

TEST_P(Execution, Dma_13_wait_event_is_barrier) {
  kts::Reference1D<cl_int> rotateB = [](size_t x) {
    return vaddInB(((x / local_wg_size) * local_wg_size) +
                   (((x % local_wg_size) + 1) % local_wg_size));
  };

  AddLocalBuffer<cl_int>(local_wg_size);
  AddLocalBuffer<cl_int>(local_wg_size);
  AddInputBuffer(kts::N, vaddInA);
  AddInputBuffer(kts::N, vaddInB);
  AddOutputBuffer(kts::N, rotateB);
  RunGeneric1D(kts::N, local_wg_size);
}

TEST_P(Execution, Dma_14_wait_event_is_barrier_overwrite) {
  kts::Reference1D<cl_int> vaddInAPlusOne = [](size_t x) {
    return vaddInA(x) + 1;
  };

  AddLocalBuffer<cl_int>(local_wg_size);
  AddInputBuffer(kts::N, vaddInA);
  AddOutputBuffer(kts::N, vaddInAPlusOne);
  RunGeneric1D(kts::N, local_wg_size);
}

// CA-1816: wait_group_event should be a barrier, and this test will fail until
// it is.  Verified that the test passes on another OpenCL implementation.
TEST_P(Execution, DISABLED_Dma_15_wait_event_is_execution_barrier) {
  kts::Reference1D<cl_int> rotateA = [](size_t x) {
    return vaddInA(((x / local_wg_size) * local_wg_size) +
                   (((x % local_wg_size) + 1) % local_wg_size));
  };

  kts::Reference1D<cl_int> rotateB = [](size_t x) {
    return vaddInB(((x / local_wg_size) * local_wg_size) +
                   (((x % local_wg_size) + 1) % local_wg_size));
  };

  AddLocalBuffer<cl_int>(local_wg_size);
  AddLocalBuffer<cl_int>(local_wg_size);
  AddLocalBuffer<cl_int>(local_wg_size);
  AddLocalBuffer<cl_int>(local_wg_size);
  AddInputBuffer(kts::N, vaddInA);
  AddInputBuffer(kts::N, vaddInB);
  AddOutputBuffer(kts::N, rotateA);
  AddOutputBuffer(kts::N, rotateB);
  RunGeneric1D(kts::N, local_wg_size);
}

TEST_P(Execution, Dma_16_wait_event_is_barrier_strided) {
  kts::Reference1D<cl_int> vaddInAPlusOne = [](size_t x) {
    return vaddInA(x) + 1;
  };

  AddLocalBuffer<cl_int>(local_wg_size);
  AddInputBuffer(kts::N, vaddInA);
  AddOutputBuffer(kts::N, vaddInAPlusOne);
  RunGeneric1D(kts::N, local_wg_size);
}
