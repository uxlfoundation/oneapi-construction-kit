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

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <numeric>

#include "Common.h"
#include "Device.h"
#include "cargo/utility.h"
#include "kts/execution.h"
#include "kts/precision.h"
#include "kts/reference_functions.h"

using namespace kts::ucl;

TEST_P(Execution, Regression_51_Local_phi) {
  AddMacro("SIZE", (unsigned int)kts::localN);
  kts::Reference1D<cl_int> refOut = [](size_t x) {
    return static_cast<cl_int>(x);
  };

  AddOutputBuffer(kts::N / kts::localN, refOut);
  RunGeneric1D(kts::N, kts::localN);
}

TEST_P(Execution, Regression_52_Nested_Loop_Using_Kernel_Arg) {
  kts::Reference1D<cl_int> refIn = [](size_t) { return 42; };

  kts::Reference1D<cl_int> refOut = [](size_t) { return 42; };

  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_53_Kernel_arg_phi) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  const size_t row_dim = 9;
  const size_t col_dim = 23;
  const size_t global_range[] = {col_dim, row_dim};
  const size_t local_range[] = {1, row_dim};

  // If the value of columns changes from 45, recompile the Offline tests
  const size_t columns = 45;
  const size_t loops = columns / row_dim;
  const cl_int step = 16;

  AddMacro("SIZE", columns);
  AddMacro("LOOPS", loops);

  kts::Reference1D<cl_uchar> refOut = [](size_t x) {
    if (0 == (x % 8)) {
      return static_cast<cl_uchar>('A');
    } else if (0 == (x % 4)) {
      return static_cast<cl_uchar>('B');
    }
    return static_cast<cl_uchar>(0);
  };

  // initial offset to dst_ptr before loop
  const size_t initial_offset =
      (col_dim * sizeof(cl_int2)) + ((row_dim - 1) * step);
  // byte offset for all loop iterations updating dst
  const size_t buffer_size = initial_offset + ((loops - 1) * row_dim * step);
  AddOutputBuffer(buffer_size, refOut);
  AddPrimitive(step);
  RunGenericND(2, global_range, local_range);
}

TEST_P(Execution, Regression_54_Negative_Comparison) {
  kts::Reference1D<cl_float> outRef = [](size_t x) -> cl_float {
    return 4.0f * x;
  };

  AddOutputBuffer(4, outRef);
  AddPrimitive(10);
  AddPrimitive(10);
  RunGeneric1D(4, 4);
}

// Spirv_Regression_55_Float_Memcpy tests something that is not valid OpenCL so
// there is no Execution variant available for this test. Building the test
// requires the legacy Khronos SPIR 3.2 generator, which is no longer standard
// for building SPIR-V. As a result, the SPIR-V version of the test has a
// different name and uses .spvasm{32|64} files built with legacy tools.
TEST_P(ExecutionSPIRV, Spirv_Regression_55_Float_Memcpy) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  kts::Reference1D<cl_float> refIn = [](size_t) { return 3.14f; };

  kts::Reference1D<cl_float> refOut = [](size_t) { return 3.14f; };

  // Enqueue single work-item
  AddInputBuffer(1, refIn);
  AddOutputBuffer(1, refOut);
  const cl_int copy_size = sizeof(cl_float);
  AddPrimitive(copy_size);
  RunGeneric1D(1);
}

TEST_P(Execution, Regression_56_Local_Vec_Mem) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  const cl_float input = 2.0f;

  kts::Reference1D<cl_float> refIn = [input](size_t) { return input; };
  kts::Reference1D<cl_float> refOut = [input](size_t) { return input; };

  // Only want one thread
  AddOutputBuffer(1, refOut);
  AddLocalBuffer<cl_float4>(1);
  AddInputBuffer(1, refIn);
  RunGeneric1D(1, 1);
}

TEST_P(Execution, Regression_57_Attribute_Aligned) {
  static constexpr size_t global = 32;
  static constexpr size_t read_local = 4;
  static constexpr size_t num_out_per_id = 2;
  AddPrimitive(0x3ff);
  AddOutputBuffer(global * num_out_per_id,
                  kts::Reference1D<cl_uint>([=](size_t i) {
                    const cl_uint id = i;
                    const cl_uint global_id = id / num_out_per_id;
                    const cl_uint sub_index = id % num_out_per_id;
                    switch (sub_index) {
                      case 0:
                        // 1024 byte align - bottom 10 bits set as we invert
                        return 0x3ffu;
                      case 1:
                        return static_cast<cl_uint>(0xdeadbeef & global_id);
                      default:
                        return 0u;  // Shouldn't hit here.
                    }
                  }));

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_58_Nested_Structs) {
  // This test uses the same nested struct in host and device code, pack in both
  // cases to guarantee that it will end up being the same size.  Unfortunately
  // GCC/Clang and MSVC handle packing quite differently.

#if defined(__GNUC__) || defined(__clang__)
#define PACKED __attribute__((packed))
#else
#define PACKED /* deliberately blank */
#endif

#if defined(_MSC_VER)
  __pragma(pack(push, 1))
#endif

      struct PACKED long_array {
    cl_long data[1];
  };

  struct PACKED s_loops {
    struct long_array loops;
    struct PACKED {
      cl_char unused;
    } unused2;
    cl_char unused3[7];
  };

  struct PACKED s_step {
    struct long_array step;
    struct s_loops x;
  };

  struct PACKED s_scheduling {
    struct long_array stride;
    struct s_step x;
  };

  struct PACKED s_wrapper {
    struct PACKED {
      struct long_array unused;
    } unused2;
    struct s_scheduling sched;
  };

  struct PACKED s_top_level {
    struct PACKED {
      cl_char unused[2];
    } unused2;
    struct s_wrapper wrap;
  };

#if defined(_MSC_VER)
  __pragma(pack(pop))
#endif

#undef PACKED

      kts::Reference1D<cl_int>
          refOut = [](size_t i) {
            if (i == 0) {
              // First work-item should do '0+1'
              return 1;
            } else if (i == 1) {
              // Second work-item should do '2+3'
              return 5;
            } else {
              return -1;
            }
          };

  kts::Reference1D<cl_uint> refIn = [](size_t i) { return i; };

  s_top_level unused_struct;
  memset(&unused_struct, 0xAF, sizeof(unused_struct));

  const size_t num_threads = 2;
  // If the work_per_thread changes from 2, recompile Offline
  const size_t work_per_thread = 2;

  AddMacro("NUM_ELEMENTS", work_per_thread);
  AddOutputBuffer(num_threads, refOut);
  AddPrimitive(unused_struct);  // Pass struct by value
  AddInputBuffer(num_threads * work_per_thread, refIn);

  RunGeneric1D(num_threads);
}

TEST_P(Execution, Regression_59_Right_Shift) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  kts::Reference1D<cl_long> refOut = [](size_t) { return -5; };

  kts::Reference1D<cl_long> refLHS = [](size_t) { return -5; };

  kts::Reference1D<cl_long> refRHS = [](size_t) { return 0; };

  // Only need a single thread
  AddOutputBuffer(1, refOut);
  AddInputBuffer(1, refLHS);
  AddInputBuffer(1, refRHS);

  RunGeneric1D(1);
}

struct GlobalRangeAndLocalRange final {
  const size_t global_range[2];
  const size_t local_range[2];

  GlobalRangeAndLocalRange(size_t global_1d, size_t global_2d, size_t local_1d,
                           size_t local_2d)
      : global_range{global_1d, global_2d}, local_range{local_1d, local_2d} {}
};

static std::ostream &operator<<(std::ostream &out,
                                const GlobalRangeAndLocalRange &ranges) {
  out << "GlobalRangeAndLocalRange{.global_range{" << ranges.global_range[0]
      << ", " << ranges.global_range[1] << "}, local_range{"
      << ranges.local_range[0] << ", " << ranges.local_range[1] << "}}";
  return out;
}

using MultipleDimensionsTests = ExecutionWithParam<GlobalRangeAndLocalRange>;

TEST_P(MultipleDimensionsTests, Regression_60_Multiple_Dimensions_0) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  AddOutputBuffer(12, kts::Ref_Identity);
  const auto Param = getParam();
  RunGenericND(2, Param.global_range, Param.local_range);
}

TEST_P(MultipleDimensionsTests, Regression_60_Multiple_Dimensions_1) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  AddOutputBuffer(12, kts::Ref_Identity);
  const auto Param = getParam();
  RunGenericND(2, Param.global_range, Param.local_range);
}

UCL_EXECUTION_TEST_SUITE_P(
    MultipleDimensionsTests, testing::ValuesIn(getSourceTypes()),
    testing::Values(GlobalRangeAndLocalRange(12, 1, 2, 1),
                    GlobalRangeAndLocalRange(6, 2, 2, 1),
                    GlobalRangeAndLocalRange(2, 6, 1, 2),
                    GlobalRangeAndLocalRange(6, 2, 3, 2),
                    GlobalRangeAndLocalRange(4, 3, 4, 1),
                    GlobalRangeAndLocalRange(4, 3, 2, 1)))

// Both Regression_61 and Regression_62 were added when tracking down an issue
// involving barriers in SYCL programs, hence the presense of SPIR-V versions
// of the test.  However, I was not able to reproduce the failures here, so
// these tests have never been known to fail in this exact form.
TEST_P(Execution, Regression_61_Sycl_Barrier) {
  kts::Reference1D<cl_int> refOut = [](size_t x) {
    const bool odd = x % 2;
    return kts::Ref_Identity(odd ? x - 1 : x + 1);
  };

  AddInOutBuffer(kts::N, kts::Ref_Identity, refOut);
  AddLocalBuffer<cl_int>(2);
  RunGeneric1D(kts::N, 2);
}

// See Regression_61 comment.
TEST_P(Execution, Regression_62_Sycl_Barrier) {
  kts::Reference1D<cl_int> refOut = [](size_t x) {
    const bool odd = x % 2;
    return kts::Ref_Identity(odd ? x - 1 : x + 1);
  };

  AddInOutBuffer(kts::N, kts::Ref_Identity, refOut);
  AddLocalBuffer<cl_int>(2);
  AddInOutBuffer(kts::N, kts::Ref_Identity, refOut);
  AddLocalBuffer<cl_int>(2);
  AddInOutBuffer(kts::N, kts::Ref_Identity, refOut);
  AddLocalBuffer<cl_int>(2);
  AddInOutBuffer(kts::N, kts::Ref_Identity, refOut);
  AddLocalBuffer<cl_int>(2);
  RunGeneric1D(kts::N, 2);
}

TEST_P(Execution, Regression_63_Barrier_Shift_Loop_Reduced) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  kts::Reference1D<cl_uchar> refOut = [](size_t) { return 23; };

  AddOutputBuffer(1, refOut);
  RunGeneric1D(1);
}

// This test is similar to DMA_03_Explicit_Copy_Rotate, but is intended to
// differentiate between getting the wrong result due to a barrier, or due to
// failing to flush global memory properly.
using MultipleLocalDimensionsTests = ExecutionWithParam<size_t>;
TEST_P(MultipleLocalDimensionsTests,
       Regression_64_Explicit_Copy_Rotate_Compare) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  size_t local_wg_size = getParam();

  kts::Reference1D<cl_int> refIn = [&local_wg_size](size_t x) {
    return x % local_wg_size;
  };

  kts::Reference1D<cl_int> refOut = [](size_t) { return 1; };

  AddLocalBuffer<cl_int>(local_wg_size);
  AddLocalBuffer<cl_int>(local_wg_size);
  AddInputBuffer(kts::N, refIn);
  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N, local_wg_size);
}

UCL_EXECUTION_TEST_SUITE_P(MultipleLocalDimensionsTests,
                           testing::ValuesIn(getSourceTypes()),
                           testing::Values(1u, 2u, 4u, 8u, 16u, 32u))

TEST_P(Execution, Regression_65_Fract_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }
  double Expected1[] = {0.0,
                        0.10000000000000009,
                        0.20000000000000018,
                        0.30000000000000027,
                        0.40000000000000036,
                        0.5,
                        0.60000000000000053,
                        0.70000000000000107,
                        0.80000000000000071,
                        0.9,
                        0.0,
                        0.10000000000000142};
  double Expected2[] = {0.0, 1.0, 2.0, 3.0, 4.0,  5.0,
                        6.0, 7.0, 8.0, 9.0, 11.0, 12.0};
  const size_t num_expected = sizeof(Expected1) / sizeof(double);
  kts::Reference1D<cl_double> refIn = [=](size_t x) {
    x = x % num_expected;
    return double(x) * 1.1;
  };
  kts::Reference1D<cl_double> refOut1 = [=, &Expected1](size_t x) {
    return Expected1[x];
  };
  kts::Reference1D<cl_double> refOut2 = [=, &Expected2](size_t x) {
    return Expected2[x];
  };

  AddInputBuffer(num_expected, refIn);
  AddOutputBuffer(num_expected, refOut1);
  AddOutputBuffer(num_expected, refOut2);
  RunGeneric1D(num_expected);
}

TEST_P(Execution, Regression_66_Loop_Diverge) {
  const cl_uint global = 16;
  const cl_uint read_local = 4;

  kts::Reference1D<cl_uint> refOut = [](size_t) { return 15; };

  AddOutputBuffer(global, refOut);
  AddPrimitive(1);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_67_Check_Ore_Call) {
  if (!this->getDeviceImageSupport()) {
    GTEST_SKIP();
  }

  const cl_uint global = 4;

  cl_image_desc desc;
  desc.image_type = CL_MEM_OBJECT_IMAGE2D;
  desc.image_width = global;
  desc.image_height = 1;
  desc.image_depth = 1;
  desc.image_array_size = 0;
  desc.image_row_pitch = 0;
  desc.image_slice_pitch = 0;
  desc.num_mip_levels = 0;
  desc.num_samples = 0;
  desc.buffer = nullptr;

  const cl_image_format format = {CL_RGBA, CL_UNSIGNED_INT8};

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([](size_t) { return 0; }));
  AddInputImage(format, desc, global, kts::Reference1D<cl_char4>([](size_t x) {
                  cl_char c = static_cast<cl_char>(x);
                  cl_char4 a4 = {{c, c, c, c}};
                  return a4;
                }));

  RunGeneric1D(global);
}

TEST_P(Execution, Regression_68_Load16) {
  const size_t global_range[] = {4, 4};
  const size_t local_range[] = {4, 4};
  const cl_int Stride = 4;

  // it is just a bunch of "random" numbers
  unsigned char InBuffer[] = {54, 61, 29, 76, 56, 26, 75, 63,  //
                              29, 86, 57, 34, 37, 15, 91, 56,  //
                              51, 48, 19, 95, 20, 78, 73, 32,  //
                              75, 51, 8,  29, 56, 34, 85, 45};

  kts::Reference1D<cl_uchar> refIn = [=, &InBuffer](size_t x) -> unsigned char {
    return InBuffer[x];
  };

  kts::Reference1D<cl_uchar> refOut = [=,
                                       &InBuffer](size_t x) -> unsigned char {
    return InBuffer[x * 2] + InBuffer[(x * 2) + 1];
  };

  const size_t N = sizeof(InBuffer) / sizeof(cl_uchar);
  AddOutputBuffer(N / 2, refOut);
  AddInputBuffer(N, refIn);
  AddPrimitive(Stride);

  RunGenericND(2, global_range, local_range);
}

TEST_P(Execution,
       Regression_69_Partial_Linearization_Varying_Uniform_Condition) {
  const cl_uint global = 4;
  const cl_uint read_local = 4;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t i) {
                    switch (i) {
                      case 0:
                      case 1:
                      case 3:
                        return 0;
                      case 2:
                        return 1;
                      default:
                        return -1;
                    }
                  }));
  AddPrimitive(1);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_70_Kernel_Call_Kernel) {
  AddOutputBuffer(kts::N, kts::Ref_Identity);
  AddInputBuffer(kts::N, kts::Ref_Identity);
  RunGeneric1D(kts::N);
}

// The Regression_71 tests expect uint values where those uint's are actually
// in a packed struct, but that should be fine.
TEST_P(Execution, Regression_71_Global_ID_Array3) {
  kts::Reference1D<cl_uint> refSize = [](size_t) {
    return static_cast<cl_uint>(3 * sizeof(cl_uint));
  };

  kts::Reference1D<cl_uint> refOut = [](size_t x) {
    return (x % 3 == 0) ? x / 3 : 0;
  };

  AddOutputBuffer(kts::N, refSize);
  AddOutputBuffer(kts::N * 3, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_71_Global_ID_Array4) {
  kts::Reference1D<cl_uint> refSize = [](size_t) {
    return static_cast<cl_uint>(4 * sizeof(cl_uint));
  };

  kts::Reference1D<cl_uint> refOut = [](size_t x) {
    return (x % 4 == 0) ? x / 4 : 0;
  };

  AddOutputBuffer(kts::N, refSize);
  AddOutputBuffer(kts::N * 4, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_71_Global_ID_Elements) {
  kts::Reference1D<cl_uint> refSize = [](size_t) {
    return static_cast<cl_uint>(3 * sizeof(cl_uint));
  };

  kts::Reference1D<cl_uint> refOut = [](size_t x) {
    return (x % 3 == 0) ? x / 3 : 0;
  };

  AddOutputBuffer(kts::N, refSize);
  AddOutputBuffer(kts::N * 3, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_72_Rotate_By_Variable) {
  // A number with a complicated bit pattern
  kts::Reference1D<cl_uint> refIn1 = [](size_t) { return 0xA5C30FF4; };

  // A few values to rotate by
  const cl_uint in2[] = {0, 32, 4, 7};
  kts::Reference1D<cl_uint> refIn2 = [&in2](size_t x) { return in2[x]; };

  const cl_uint out[] = {0xA5C30FF4, 0xA5C30FF4, 0x5C30FF4A, 0xE187FA52};
  kts::Reference1D<cl_uint> refOut = [&out](size_t x) { return out[x]; };

  AddInputBuffer(4, refIn1);
  AddInputBuffer(4, refIn2);
  AddOutputBuffer(4, refOut);
  RunGeneric1D(4);
}

// Rotating by a literal allows for compiler optimizations, which might produce
// a poison value
TEST_P(Execution, Regression_73_Rotate_By_Literal) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  // A number with a complicated bit pattern
  kts::Reference1D<cl_uint> refIn = [](size_t) { return 0xA5C30FF4; };

  const cl_uint out[] = {0xA5C30FF4, 0xA5C30FF4, 0x5C30FF4A, 0xE187FA52};
  kts::Reference1D<cl_uint> refOut = [&out](size_t x) { return out[x]; };

  AddInputBuffer(4, refIn);
  AddOutputBuffer(4, refOut);
  RunGeneric1D(1);
}

// Tests for structs with smaller alignment than some of its members,
// where struct size is divisible by alignment of members,
// and also where it isn't:
#pragma pack(4)
struct StrideMisaligned {
  // has to match the one in the kernel source
  cl_ulong4 global_size;
  cl_uint work_dim;

  bool operator==(const StrideMisaligned &rhs) {
    // clang-tidy does not realise that StrideMisaligned has unique object
    // representations because of its global_size member: clang has an open
    // FIXME for supporting vectors. The static_assert below should cover any
    // scenario where the compiler still adds padding inside the struct, so we
    // can suppress the clang-tidy warning.
    return memcmp  // NOLINT(bugprone-suspicious-memory-comparison,cert-exp42-c,cert-flp37-c)
           (this, &rhs, sizeof(StrideMisaligned)) == 0;
  }
};

static_assert(sizeof(StrideMisaligned) ==
              4 * sizeof(cl_ulong) + sizeof(cl_uint));

#pragma pack(4)
struct StrideAligned {
  // has to match the one in the kernel source
  cl_ulong4 global_size;
  cl_uint work_dim;
  cl_uint padding;

  bool operator==(const StrideAligned &rhs) {
    // sizeof(StrideMisaligned) because the value of the padding doesn't matter
    return memcmp(this, &rhs, sizeof(StrideMisaligned)) == 0;
  }
};

static std::stringstream &operator<<(std::stringstream &stream,
                                     const StrideAligned &info) {
  stream << "{\n"
         << "  global_size: " << info.global_size.x << ", "
         << info.global_size.y << ", " << info.global_size.z << ", "
         << info.global_size.w << ")\n"
         << "  work_dim: " << info.work_dim << "\n}\n";
  return stream;
}

static std::stringstream &operator<<(std::stringstream &stream,
                                     const StrideMisaligned &info) {
  stream << "{\n"
         << "  global_size: (" << info.global_size.x << ", "
         << info.global_size.y << ", " << info.global_size.z << ", "
         << info.global_size.w << ")\n"
         << "  work_dim: " << info.work_dim << "\n}\n";
  return stream;
}

TEST_P(Execution, Regression_74_Stride_Aligned) {
  const size_t global_range[] = {24, 16, 4};
  const size_t local_range[] = {4, 4, 4};

  kts::Reference1D<StrideAligned> refOut = [=](size_t) -> StrideAligned {
    StrideAligned result{{{1, 2, 3, 4}}, 5, 0};
    return result;
  };

  const size_t N = global_range[0] * global_range[1] * global_range[2];
  AddOutputBuffer(N, refOut);

  RunGenericND(3, global_range, local_range);
}

TEST_P(Execution, Regression_74_Stride_Misaligned) {
  const size_t global_range[] = {24, 16, 4};
  const size_t local_range[] = {4, 4, 4};

  kts::Reference1D<StrideMisaligned> refOut = [=](size_t) -> StrideMisaligned {
    StrideMisaligned result{{{1, 2, 3, 4}}, 5};
    return result;
  };

  const size_t N = global_range[0] * global_range[1] * global_range[2];
  AddOutputBuffer(N, refOut);

  RunGenericND(3, global_range, local_range);
}

TEST_P(Execution, Regression_75_Partial_Linearization0) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 11;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int ret = 0;
                    if (id % 5 == 0) {
                      for (int i = 0; i < n * 2; i++) ret++;
                    } else {
                      for (int i = 0; i < n / 4; i++) ret++;
                    }
                    if (n > 10) {
                      if (id % 2 == 0) {
                        for (int i = 0; i < n + 10; i++) ret++;
                      } else {
                        for (int i = 0; i < n + 10; i++) ret *= 2;
                      }
                      ret += id * 10;
                    } else {
                      if (id % 2 == 0) {
                        for (int i = 0; i < n + 8; i++) ret++;
                      } else {
                        for (int i = 0; i < n + 8; i++) ret *= 2;
                      }
                      ret += id / 2;
                    }
                    return ret;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization1) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 1;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int ret = 0;
                    int j = 0;
                    while (1) {
                      if (id % 2 == 0) {
                        if (n > 2) {
                          goto e;
                        }
                      } else {
                        for (int i = 0; i < n + 10; i++) ret++;
                      }
                      if (j++ <= 2) break;
                    }
                    ret += n * 2;
                    for (int i = 0; i < n * 2; i++) ret -= i;
                    ret /= n;
                    goto early;
                  e:
                    for (int i = 0; i < n + 5; i++) ret /= 2;
                    ret -= n;
                  early:
                    return ret;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization2) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 12;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int ret = 0;
                    if (n > 10) {
                      if (id % 3 == 0) {
                        for (int i = 0; i < n - 1; i++) {
                          ret++;
                        }
                        goto h;
                      } else {
                        for (int i = 0; i < n / 3; i++) {
                          ret += 2;
                        }
                        goto i;
                      }
                    } else {
                      if (id % 2 == 0) {
                        for (int i = 0; i < n * 2; i++) {
                          ret += 1;
                        }
                        goto h;
                      } else {
                        for (int i = 0; i < n + 5; i++) {
                          ret *= 2;
                        }
                        goto i;
                      }
                    }
                  h:
                    ret += 5;
                    goto end;
                  i:
                    ret *= 10;
                    goto end;
                  end:
                    return ret;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization3) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 12;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int ret = 0;
                    if (n > 10) {
                      if (id % 3 == 0) {
                        for (int i = 0; i < n - 1; i++) {
                          ret++;
                        }
                        goto end;
                      } else {
                        for (int i = 0; i < n / 3; i++) {
                          ret += 2;
                        }
                        goto h;
                      }
                    } else {
                      if (id % 2 == 0) {
                        for (int i = 0; i < n * 2; i++) {
                          ret += 1;
                        }
                        goto h;
                      } else {
                        for (int i = 0; i < n + 5; i++) {
                          ret *= 2;
                        }
                        goto i;
                      }
                    }
                  h:
                    ret += 5;
                  i:
                    ret *= 10;
                  end:
                    return ret;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization4) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 10;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int x = id / n;
                    int y = id % n;
                    int i = 0;
                    for (;;) {
                      if (n > 20) goto e;
                      if (x + y > n) goto f;
                      y++;
                      x++;
                      i++;
                    }
                    goto g;
                  e:
                    i *= 2 + n;
                    goto g;

                  f:
                    i /= i + n;
                  g:
                    return x + y + i;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization5) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 10;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int ret = 0;
                    if (id % 2 == 0) {
                      if (id == 4) {
                        goto g;
                      } else {
                        goto d;
                      }
                    } else {
                      if (n % 2 == 0) {
                        goto d;
                      } else {
                        goto e;
                      }
                    }
                  d:
                    for (int i = 0; i < n; i++) {
                      ret += i - 2;
                    }
                    goto f;
                  e:
                    for (int i = 0; i < n + 5; i++) {
                      ret += i + 5;
                    }
                  f:
                    ret *= ret % n;
                    ret *= ret + 4;
                  g:
                    return ret;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization6) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 10;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int ret = 0;
                    int i = 0;
                    while (1) {
                      if (i++ & 1) {
                        if (n > 2) {
                          goto e;
                        }
                      } else {
                        ret += n + 1;
                      }
                      if (id == static_cast<size_t>(n)) break;
                    }
                    ret += n * 2;
                    ret /= n;
                    goto early;
                  e:
                    ret += n * 4;
                    ret -= n;
                  early:
                    return ret;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization7) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 10;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int i = 0;
                    if (n > 10) {
                      if (n + id > 15) {
                        i = n * 10;
                        goto g;
                      } else {
                        goto e;
                      }
                    } else {
                      if (n < 5) {
                        goto e;
                      } else {
                        for (int j = 0; j < n; j++) {
                          i++;
                        }
                        goto h;
                      }
                    }
                  e:
                    if (n > 5) {
                      goto g;
                    } else {
                      i = n * 3 / 5;
                      goto h;
                    }
                  g:
                    for (int j = 0; j < n; j++) {
                      i++;
                    }
                    goto i;

                  h:
                    i = n + id / 3;
                  i:
                    return i;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization8) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 10;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int x = id / n;
                    int y = id % n;
                    int i = 0;
                    for (;;) {
                      if (i + id > 10) goto e;
                      if (x + y > n) goto f;
                      y++;
                      x++;
                      i++;
                    }
                    goto g;
                  e:
                    i *= 2 + n;
                    goto g;
                  f:
                    i /= i + n;
                  g:
                    return x + y + i;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization9) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 10;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int i = 0;
                    while (1) {
                      int j = 0;
                      for (;; i++) {
                        if (j++ > n) break;
                      }
                      if (i++ + id > 10) break;
                    }
                    return i;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization10) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 11;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int ret = 0;
                    int i = 0;
                    while (1) {
                      if (n > 0) {
                        for (int i = 0; i < n * 2; i++) ret++;
                        if (n <= 10) {
                          goto f;
                        }
                      } else {
                        for (int i = 0; i < n / 4; i++) ret++;
                      }
                      ret++;
                      while (1) {
                        if (n & 1) {
                          if (n == 3) {
                            goto j;
                          }
                        } else {
                          if (ret + id >= 11) {
                            ret /= n * n + ret;
                            goto o;
                          }
                        }
                        if (i++ > 3) {
                          ret += n * ret;
                          goto n;
                        }
                      o:
                        ret++;
                      }
                    j:
                      if (n < 20) {
                        ret += n * 2 + 20;
                        goto p;
                      } else {
                        goto q;
                      }
                    n:
                      ret *= 4;
                    q:
                      if (i > 5) {
                        ret++;
                        goto r;
                      }
                    }
                  r:
                    for (int i = 0; i < n / 4; i++) ret++;
                    goto s;

                  f:
                    ret /= n;
                    goto p;
                  p:
                    for (int i = 0; i < n * 2; i++) ret++;
                  s:
                    return ret;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization11) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 7;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int ret = 0;
                    int i = 0;
                    while (1) {
                      while (1) {
                        if (n > 5) {
                          for (int i = 0; i < n * 2; i++) ret++;
                          if (n == 6) {
                            goto i;
                          }
                        } else {
                          if (ret + id >= 7) {
                            ret /= n * n + ret;
                            if (ret <= 10) {
                              goto k;
                            } else {
                              goto h;
                            }
                          }
                        }
                        ret *= n;
                        if (i++ > 2) {
                          goto j;
                        }
                      h:
                        ret++;
                      }
                    j:
                      ret += n * 2 + 20;
                      goto l;
                    k:
                      ret *= n;
                      goto l;
                    l:
                      if (i > 3) {
                        ret++;
                        goto m;
                      }
                    }
                  m:
                    for (int i = 0; i < n / 4; i++) ret++;
                    goto n;
                  i:
                    ret /= n;
                  n:
                    return ret;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization12) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 7;

  AddOutputBuffer(global, kts::Reference1D<cl_int>([=](size_t id) {
                    int ret = 0;
                    while (1) {
                      if (n > 0) {
                        ret++;
                        if (ret <= 10) {
                          goto f;
                        }
                      } else {
                        ret++;
                      }
                      ret++;
                      while (1) {
                        if (n <= 2) {
                          ret -= n * ret;
                          goto j;
                        } else {
                          if (ret + id >= 7) {
                            ret /= n * n + ret;
                            if (ret < n) {
                              ret -= n;
                              goto m;
                            } else {
                              ret += n;
                              goto n;
                            }
                          } else {
                            if (ret > n) {
                              ret += n;
                              goto m;
                            } else {
                              ret -= n;
                              goto n;
                            }
                          }
                        }
                      m:
                        if (n & ret) {
                          ret *= n;
                          goto q;
                        } else {
                          goto p;
                        }
                      n:
                        ret *= ret;
                      p:
                        if (ret > n) {
                          goto r;
                        }
                        ret++;
                      }
                    r:
                      ret *= 4;
                      ret++;
                      if ((ret + n) & 1) {
                        goto t;
                      }
                      ret++;
                    }
                  f:
                    ret /= n;
                    goto j;
                  j:
                    if (ret <= n) {
                      goto q;
                    } else {
                      goto u;
                    }
                  t:
                    ret++;
                    goto u;
                  q:
                    ret++;
                    goto v;
                  u:
                    ret++;
                  v:
                    return ret;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization13) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 7;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int ret = 0;
                    if (id + 1 < read_local) {
                      ret = n;
                    } else if (id + 1 == read_local) {
                      const size_t leftovers = 1 + (read_local & 1);
                      switch (leftovers) {
                        case 2:
                          ret = 2 * n + 1;
                          [[fallthrough]];
                        case 1:
                          ret += 3 * n - 1;
                          break;
                        default:
                          abort();
                      }
                      switch (leftovers) {
                        case 2:
                          ret /= n;
                          [[fallthrough]];
                        case 1:
                          ret--;
                          break;
                        default:
                          abort();
                      }
                    }
                    return ret;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization14) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 7;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int ret = 0;
                    int i = 0;
                    while (1) {
                      if (n > 0) {
                        for (int i = 0; i < n; i++) ret++;
                      } else {
                        if (id == static_cast<size_t>(n)) {
                          goto k;
                        }
                      }
                      if (i++ >= 2) {
                        goto l;
                      }
                    }
                  k:
                    ret += n;
                  l:
                    return ret;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization15) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const size_t n = 11;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int ret = 0;
                    while (1) {
                      if (n > 0) {
                        for (size_t i = 0; i < n * 2; i++) ret++;
                        if (n <= 10) {
                          goto f;
                        }
                      } else {
                        for (size_t i = 0; i < n / 4; i++) ret++;
                      }
                      ret++;
                      while (1) {
                        if (n & 1) {
                          if (n < 3) {
                            goto l;
                          }
                        } else {
                          if (ret + id >= n) {
                            ret /= n * n + ret;
                            goto m;
                          }
                        }
                        if (n & 1) {
                          goto l;
                        }
                      m:
                        ret++;
                      }
                    l:
                      ret *= 4;
                      if (n & 1) {
                        ret++;
                        goto p;
                      }
                    }
                  p:
                    for (size_t i = 0; i < n / 4; i++) ret++;
                    goto q;
                  f:
                    ret /= n;
                    goto n;
                  n:
                    for (size_t i = 0; i < n * 2; i++) ret++;
                  q:
                    return ret;
                  }));
  AddPrimitive(cl_int(n));

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization16) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 11;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int ret = 0;
                    const int i = 0;
                    if (n < 5) {
                      for (int i = 0; i < n + 10; i++) ret++;
                      goto h;
                    } else {
                      while (1) {
                        if (id + i % 2 == 0) {
                          if (n > 2) {
                            goto f;
                          }
                        } else {
                          for (int i = 0; i < n + 10; i++) ret++;
                        }
                        if (n > 5) break;
                      }
                    }
                    ret += n * 2;
                    for (int i = 0; i < n * 2; i++) ret -= i;
                    ret /= n;
                    goto early;
                  f:
                    for (int i = 0; i < n + 5; i++) ret /= 2;
                    ret -= n;
                  h:
                    for (int i = 0; i < n * 2; i++) ret -= i;
                  early:
                    return ret;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization17) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 11;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int ret = 0;
                    int i = 0;
                    while (1) {
                      if (n > 10) {
                        goto c;
                      } else if (n > 5) {
                        goto f;
                      }
                      if (id + i++ % 2 == 0) {
                        break;
                      }
                    }
                    for (int i = 0; i < n + 10; i++) ret++;
                    goto m;
                  f:
                    ret += n * 2;
                    for (int i = 0; i < n * 2; i++) ret += i;
                    goto m;
                  c:
                    for (int i = 0; i < n + 5; i++) ret += 2;
                    if (id % 2 == 0) {
                      goto h;
                    } else {
                      goto m;
                    }
                  m:
                    ret <<= 2;
                    goto o;
                  h:
                    for (int i = 0; i < n * 2; i++) {
                      if (n > 5) {
                        goto l;
                      }
                    }
                    ret += id << 3;
                    goto p;
                  l:
                    ret += id << 3;
                  o:
                    for (int i = 0; i < n * 2; i++) ret += i;
                  p:
                    return ret;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization18) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 11;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int ret = 0;
                    int i = 0;
                    while (1) {
                      if (n > 5) {
                        if (id + i % 2 == 0) {
                          goto e;
                        } else {
                          goto f;
                        }
                      }
                      if (++i + id > 3) {
                        goto g;
                      }
                    }
                  f:
                    for (int i = 0; i < n + 5; i++) ret += 2;
                    goto g;
                  g:
                    for (int i = 1; i < n * 2; i++) ret -= i;
                    goto h;
                  e:
                    for (int i = 0; i < n + 5; i++) ret++;
                    goto i;
                  h:
                    if (n > 3) {
                    i:
                      ret++;
                    } else {
                      ret *= 3;
                    }
                    return ret;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_75_Partial_Linearization19) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 11;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t id) {
                    int ret = 0;
                    int i = 0;
                    while (1) {
                      if (n > 5) {
                        if (n == 6) {
                          goto d;
                        } else {
                          goto e;
                        }
                      }
                      if (++i + id > 3) {
                        break;
                      }
                    }
                    if (n == 3) {
                      goto h;
                    } else {
                      goto i;
                    }
                  d:
                    for (int i = 0; i < n + 5; i++) ret += 2;
                    goto i;
                  e:
                    for (int i = 1; i < n * 2; i++) ret += i;
                    goto h;
                  i:
                    for (int i = 0; i < n + 5; i++) ret++;
                    goto j;
                  h:
                    for (int i = 0; i < n; i++) ret++;
                    goto j;
                  j:
                    return ret;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

// Do not add additional tests here or this file may become too large to link.
// Instead, extend the newest ktst_regression_${NN}.cpp file.
