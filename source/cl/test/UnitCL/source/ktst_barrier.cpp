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

#include "kts/execution.h"
#include "kts/reference_functions.h"

// This is needed for 'TEST_F'.
using namespace kts::ucl;

TEST_P(Execution, Barrier_01_Barrier_In_Function) { RunGeneric1D(kts::N, 2); }

namespace {
cl_int refOut(size_t x) {
  const cl_int idx = kts::Ref_Identity(x);
  const cl_int pos = idx & 1;

  if (pos) {
    return idx - 1;
  } else {
    return idx + 1;
  }
}
}  // namespace

TEST_P(Execution, Barrier_02_Barrier_No_Duplicates) {
  AddInputBuffer(kts::N, kts::Reference1D<cl_int>(kts::Ref_Identity));
  AddOutputBuffer(kts::N, kts::Reference1D<cl_int>(refOut));

  RunGeneric1D(kts::N, 2);
}

TEST_P(Execution, Barrier_03_Barrier_Noinline) {
  AddInputBuffer(kts::N, kts::Reference1D<cl_int>(kts::Ref_Identity));
  AddOutputBuffer(kts::N, kts::Reference1D<cl_int>(refOut));

  RunGeneric1D(kts::N, 2);
}

TEST_P(Execution, Barrier_04_Barrier_Local_Mem) {
  const cl_int global = 64;
  // If these change regenerate SPIR-V
  const cl_int read_local = 16;
  const cl_int read_local_id = 1;
  const cl_int global_id = 0;

  AddInputBuffer(kts::N, kts::Ref_Identity);
  AddOutputBuffer(
      1, kts::Reference1D<cl_int>([=](size_t) { return read_local_id; }));

  // These macros will not affect SPIR-V or OFFLINE tests
  AddMacro("GLOBAL_ID", global_id);
  AddMacro("READ_LOCAL_ID", read_local_id);
  AddMacro("READ_LOCAL_SIZE", read_local);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Barrier_05_Barrier_In_Loop) {
  const cl_int global = 64;
  // If these change regenerate SPIR-V
  const cl_int read_local = 16;
  const cl_int outer_loop_size = 1;
  const cl_int global_id = 0;

  AddOutputBuffer(1, kts::Reference1D<cl_int>([=](size_t) {
                    cl_int total = 0;
                    for (int i = 0; i < read_local; i++) {
                      total += i;
                    }
                    return total * outer_loop_size;
                  }));

  // These macros will not affect SPIR-V or OFFLINE tests
  AddMacro("GLOBAL_ID", global_id);
  AddMacro("OUTER_LOOP_SIZE", outer_loop_size);
  AddMacro("READ_LOCAL_SIZE", read_local);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Barrier_06_Barrier_With_Ifs) {
  // FIXME: An issue in control-flow conversion prevents this test from
  // vectorizing under certain optimizations. See CA-4419.
  fail_if_not_vectorized_ = false;

  const cl_int global = 32;
  // If these change regenerate SPIR-V
  const cl_int read_local = 4;
  const cl_int global_id = 1;
  const cl_int local_id = 0;

  AddOutputBuffer(1, kts::Reference1D<cl_int>([=](size_t) {
                    cl_int total = 0;
                    for (int i = 0; i < read_local; i++) {
                      total += i;
                    }
                    return total * global;
                  }));

  // These macros will not affect SPIR-V or OFFLINE tests
  AddMacro("GLOBAL_ID", global_id);
  AddMacro("LOCAL_ID", local_id);
  AddMacro("READ_LOCAL_SIZE", read_local);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Barrier_07_Barrier_In_Loop_2) {
  const cl_int global = 32;
  // If these change regenerate SPIR-V
  const cl_int read_local = 4;
  const cl_int global_id = 1;
  const cl_int local_id = 0;

  AddOutputBuffer(1, kts::Reference1D<cl_int>([=](size_t) {
                    int total = 0;
                    for (int i = 0; i < read_local; i++) {
                      for (int j = 0; j < read_local; j++) {
                        total += i;
                      }
                    }
                    return total;
                  }));

  // These macros will not affect SPIR-V or OFFLINE tests
  AddMacro("GLOBAL_ID", global_id);
  AddMacro("LOCAL_ID", local_id);
  AddMacro("READ_LOCAL_SIZE", read_local);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Barrier_07_Barrier_In_Loop_3) {
  const cl_int global = 32;
  // If these change regenerate SPIR-V
  const cl_int read_local = 4;
  const cl_int global_id = 1;
  const cl_int local_id = 0;

  AddOutputBuffer(1, kts::Reference1D<cl_int>([=](size_t) {
                    int total = 0;
                    for (int i = 0; i < read_local; i++) {
                      for (int j = 0; j < read_local; j++) {
                        total += i;
                      }
                    }
                    return total;
                  }));

  // These macros will not affect SPIR-V or OFFLINE tests
  AddMacro("GLOBAL_ID", global_id);
  AddMacro("LOCAL_ID", local_id);
  AddMacro("READ_LOCAL_SIZE", read_local);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Barrier_07_Barrier_In_Loop_4) {
  const cl_int global = 32;
  // If these change regenerate SPIR-V
  const cl_int read_local = 4;
  const cl_int global_id = 1;
  const cl_int local_id = 0;

  AddOutputBuffer(1, kts::Reference1D<cl_int>([=](size_t) {
                    int total = 0;
                    for (int i = 0; i < read_local; i++) {
                      for (int j = 0; j < read_local; j++) {
                        total += i;
                      }
                    }
                    return total;
                  }));

  // These macros will not affect SPIR-V or OFFLINE tests
  AddMacro("GLOBAL_ID", global_id);
  AddMacro("LOCAL_ID", local_id);
  AddMacro("READ_LOCAL_SIZE", read_local);

  RunGeneric1D(global, read_local);
}

using BarrierDebugTests = kts::ucl::ExecutionWithParam<bool>;
UCL_EXECUTION_TEST_SUITE_P(BarrierDebugTests, testing::Values(OPENCL_C),
                           testing::Values(true, false))

TEST_P(BarrierDebugTests, Barrier_08_Barrier_Debug) {
  fail_if_not_vectorized_ = false;
  // This test is designed for running under a debugger to test debugability
  // of kernels with barriers. Therefore we sometimes want to run a single
  // workgroup to prevent switching between threads.
  const bool singleWorkgroup = getParam();
  AddInputBuffer(kts::N, kts::Ref_Identity);

  const size_t local_size = singleWorkgroup ? kts::N : 4;

  kts::Reference1D<cl_int> refOut = [local_size](size_t x) {
    return static_cast<cl_int>((x * (x % local_size)) + (2 * x));
  };

  AddOutputBuffer(kts::N, refOut);

  RunGeneric1D(kts::N, local_size);
}

TEST_P(Execution, Barrier_09_Barrier_With_Alias) {
  const cl_int global = 32;
  const cl_int read_local = 4;
  AddOutputBuffer(global, kts::Reference1D<cl_int>([](size_t i) {
                    const cl_int global_id = static_cast<cl_int>(i);
                    const cl_int res = 4 + global_id;
                    return res;
                  }));

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Barrier_10_Barriers_With_Alias) {
  const cl_int global = 32;
  const cl_int read_local = 4;
  AddOutputBuffer(global, kts::Reference1D<cl_int>([](size_t i) {
                    const cl_int global_id = static_cast<cl_int>(i);

                    cl_int res = global_id;
                    res = res + ((global_id & 1) ? 22 : 1);
                    res = res + ((global_id & 1) ? 14 : 12);
                    return res;
                  }));

  RunGeneric1D(global, read_local);
}

using MultipleLocalDimensions = ExecutionWithParam<size_t>;
UCL_EXECUTION_TEST_SUITE_P(MultipleLocalDimensions, testing::Values(OPENCL_C),
                           testing::Values(2u, 4u, 8u, 16u, 32u))

TEST_P(MultipleLocalDimensions, Barrier_10_Barriers_With_Alias) {
  const cl_int global = 32;
  const cl_int read_local = getParam();
  AddOutputBuffer(global, kts::Reference1D<cl_int>([](size_t i) {
                    const cl_int global_id = static_cast<cl_int>(i);

                    cl_int res = global_id;
                    res = res + ((global_id & 1) ? 22 : 1);
                    res = res + ((global_id & 1) ? 14 : 12);
                    return res;
                  }));

  RunGeneric1D(global, read_local);
}

TEST_P(MultipleLocalDimensions, Barrier_10_Barriers_With_Alias_2) {
  const cl_int global = 32;
  const cl_int read_local = getParam();
  AddOutputBuffer(global, kts::Reference1D<cl_int>([](size_t i) {
                    const cl_int global_id = static_cast<cl_int>(i);

                    cl_int res = global_id;
                    res = res + ((global_id & 1) ? 22 : 1);
                    res = res + ((global_id & 1) ? 14 : 12);
                    return res;
                  }));

  RunGeneric1D(global, read_local);
}

TEST_P(MultipleLocalDimensions, Barrier_11_Barrier_With_Align) {
  static constexpr size_t global = 32;
  static constexpr size_t num_out_per_id = 6;
  const size_t read_local = getParam();
  AddPrimitive(0x3);
  AddPrimitive(0x7);
  AddPrimitive(0x3ff);
  AddOutputBuffer(global * num_out_per_id,
                  kts::Reference1D<cl_uint>([=](size_t i) {
                    const cl_uint id = static_cast<cl_uint>(i);
                    const cl_uint global_id = id / num_out_per_id;
                    const cl_uint sub_index = id % num_out_per_id;
                    switch (sub_index) {
                      case 0:
                        // 32 bit align - bottom 2 bits set as we invert
                        return static_cast<cl_uint>(0x3);
                      case 1:
                        // 64 bit align - bottom 3 bits set as we invert
                        return static_cast<cl_uint>(0x7);
                      case 2:
                        // 1024 byte align - bottom 10 bits set as we invert
                        return static_cast<cl_uint>(0x3ff);
                      case 3:
                        return static_cast<cl_uint>(global_id + 12);
                      case 4:
                        return static_cast<cl_uint>(global_id + 54);
                      case 5:
                        return static_cast<cl_uint>(0xdeadbeef & global_id);
                      default:
                        assert(false);
                        return static_cast<cl_uint>(0);  // Shouldn't hit here.
                    }
                  }));

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Barrier_12_Barrier_In_Sub_Function_Called_Twice) {
  fail_if_not_vectorized_ = false;

  AddInputBuffer(kts::N, kts::Reference1D<cl_int>(kts::Ref_Identity));
  AddOutputBuffer(kts::N, kts::Reference1D<cl_int>(refOut));

  RunGeneric1D(kts::N, 2);
}

TEST_P(Execution, Barrier_13_Barrier_Shift_loop) {
  const size_t block_size = 16;
  const size_t local_size =
      block_size * block_size;  // If this changes regenerate SPIR-V
  const size_t global_size = block_size * local_size;
  const size_t global_range[] = {global_size, block_size};
  const size_t local_range[] = {local_size, 1};

  const size_t blocks = block_size * 2;  // If this changes regenerate SPIR-V
  // These macros will not affect SPIR-V or OFFLINE tests
  AddMacro("BLOCK_COLS", blocks);
  AddMacro("BLOCK_ROWS", blocks);
  AddMacro("LOCAL_SIZE", local_size);

  kts::Reference1D<cl_uchar> refOut = [](size_t) { return 'A'; };

  const size_t columns = local_size * 2;
  const size_t rows = local_size * 2;
  const size_t buffer_size = rows * columns;

  AddOutputBuffer(buffer_size, refOut);
  AddPrimitive(cl_int(rows));
  AddPrimitive(cl_int(columns));
  RunGenericND(2, global_range, local_range);
}

TEST_P(Execution, Barrier_14_Barrier_In_Reduce) {
  kts::Reference1D<cl_int> refIn = [](size_t x) {
    return static_cast<cl_int>(x % kts::localN);
  };

  kts::Reference1D<cl_int> refOut = [](size_t) {
    auto n = kts::localN - 1;
    auto gauss = (n * (n + 1)) / 2;
    return static_cast<cl_int>(gauss);
  };

  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N / kts::localN, refOut);
  AddLocalBuffer<cl_int>(kts::localN);
  RunGeneric1D(kts::N, kts::localN);
}

TEST_P(Execution, Barrier_15_Vector_Barriers_With_Alias) {
  const cl_int global = 32;
  const cl_int read_local = 4;

  AddOutputBuffer(global, kts::Reference1D<cl_uint>([=](size_t i) {
                    const cl_uint global_id = i;
                    cl_int res = global_id;
                    res = res + ((global_id & 1) ? 22 : 1);
                    return res;
                  }));

  RunGeneric1D(global / 4, read_local);
}

using MemFenceTests = kts::ucl::ExecutionWithParam<const char *>;
UCL_EXECUTION_TEST_SUITE_P(MemFenceTests, testing::Values(OPENCL_C),
                           testing::Values("mem_fence", "read_mem_fence",
                                           "write_mem_fence", "barrier"))
TEST_P(MemFenceTests, Barrier_16_Memory_Fence_Global) {
  AddMacro("FENCE_OP", getParam());
  AddInputBuffer(kts::N, kts::Ref_Identity);
  AddOutputBuffer(kts::N, kts::Ref_Identity);
  AddOutputBuffer(kts::N, kts::Ref_Identity);
  RunGeneric1D(kts::N);
}

TEST_P(MemFenceTests, Barrier_16_Memory_Fence_Local) {
  AddMacro("FENCE_OP", getParam());
  AddInputBuffer(kts::N, kts::Ref_Identity);
  AddLocalBuffer<cl_int>(kts::localN);
  AddOutputBuffer(kts::N, kts::Ref_Identity);
  RunGeneric1D(kts::N, kts::localN);
}

TEST_P(MultipleLocalDimensions, Barrier_17_Barrier_Store_Mask) {
  const cl_uint global = 32;
  const cl_uint read_local = getParam();
  AddPrimitive(0);
  AddOutputBuffer(global, kts::Reference1D<cl_uint>(
                              [=](size_t) { return static_cast<cl_uint>(0); }));

  RunGeneric1D(global, read_local);
}

TEST_P(MultipleLocalDimensions, Barrier_18_Barrier_Store_Mask) {
  const size_t global = 32;
  const size_t read_local = getParam();
  const size_t num_out_per_id = 2;
  AddPrimitive(0x3);
  AddOutputBuffer(global * num_out_per_id,
                  kts::Reference1D<cl_uint>([=](size_t i) {
                    const cl_uint id = static_cast<cl_uint>(i);
                    const cl_uint global_id = id / num_out_per_id;
                    const cl_uint sub_index = id % num_out_per_id;
                    switch (sub_index) {
                      case 0:
                        return static_cast<cl_uint>(0x3);
                      case 1:
                        return static_cast<cl_uint>(global_id);
                      default:
                        assert(false);
                        return static_cast<cl_uint>(0);  // Shouldn't hit here.
                    }
                  }));

  RunGeneric1D(global, read_local);
}

TEST_P(MultipleLocalDimensions, Barrier_19_Barrier_Store_Mask) {
  const size_t global = 32;
  const size_t read_local = getParam();
  const size_t num_out_per_id = 2;
  AddPrimitive(0x3);
  AddOutputBuffer(global * num_out_per_id,
                  kts::Reference1D<cl_uint>([=](size_t i) {
                    const cl_uint id = static_cast<cl_uint>(i);
                    const cl_uint global_id = id / num_out_per_id;
                    const cl_uint sub_index = id % num_out_per_id;
                    switch (sub_index) {
                      case 0:
                        // 32 bit align - bottom 2 bits set as we invert
                        return static_cast<cl_uint>(0x3);
                      case 1:
                        return static_cast<cl_uint>(global_id + 12);
                      default:
                        assert(false);
                        return static_cast<cl_uint>(0);  // Shouldn't hit here.
                    }
                  }));

  RunGeneric1D(global, read_local);
}

// This test is reduced from barrier_10. Its purpose was to have a more precise
// and smaller generated IR to debug the code that was detected by the
// barrier_10 test.
TEST_P(MultipleLocalDimensions, Barrier_20_Barriers_With_Alias) {
  const cl_int global = 32;
  const cl_int read_local = getParam();
  AddOutputBuffer(global, kts::Reference1D<cl_int>([](size_t i) {
                    const cl_int global_id = static_cast<cl_int>(i);

                    cl_int res = global_id;
                    res = res + ((global_id & 1) ? 22 : 20);
                    return res;
                  }));

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Barrier_21_Barrier_In_loop) {
  const size_t global_size = 32;
  const size_t local_size = 32;

  kts::Reference1D<cl_uchar> refOut = [](size_t) { return 'A'; };

  AddOutputBuffer(global_size, refOut);
  RunGeneric1D(global_size, local_size);
}

TEST_P(Execution, Barrier_22_Barrier_Local_Arrays) {
  const size_t global_range[] = {16, 16};
  const size_t local_range[] = {16, 16};

  // it is just a bunch of "random" numbers
  float InBuffer[] = {0.54f, 0.61f, 0.29f, 0.76f, 0.56f, 0.26f, 0.75f, 0.63f,
                      0.29f, 0.86f, 0.57f, 0.34f, 0.37f, 0.15f, 0.91f, 0.56f,
                      0.51f, 0.48f, 0.19f, 0.95f, 0.20f, 0.78f, 0.73f, 0.32f,
                      0.75f, 0.51f, 0.08f, 0.29f, 0.56f, 0.34f, 0.85f, 0.45f};

  float OutBuffer[] = {
      0x1.cef32p+6,  0x1.ee83dp+6,  0x1.4b6e2cp+7, 0x1.2d2e3ep+7, 0x1.09ab88p+7,
      0x1.1581d2p+7, 0x1.e69fbp+6,  0x1.085ae2p+7, 0x1.cef32p+6,  0x1.ee83dp+6,
      0x1.4b6e2cp+7, 0x1.2d2e3ep+7, 0x1.09ab88p+7, 0x1.1581d2p+7, 0x1.e69fbp+6,
      0x1.085ae2p+7, 0x1.cef32p+6,  0x1.ee83dp+6,  0x1.4b6e2cp+7, 0x1.2d2e3ep+7,
      0x1.09ab88p+7, 0x1.1581d2p+7, 0x1.e69fbp+6,  0x1.085ae2p+7, 0x1.cef32p+6,
      0x1.ee83dp+6,  0x1.4b6e2cp+7, 0x1.2d2e3ep+7, 0x1.09ab88p+7, 0x1.1581d2p+7,
      0x1.e69fbp+6,  0x1.085ae2p+7, 0x1.cef32p+6,  0x1.ee83dp+6,  0x1.4b6e2cp+7,
      0x1.2d2e3ep+7, 0x1.09ab88p+7, 0x1.1581d2p+7, 0x1.e69fbp+6,  0x1.085ae2p+7,
      0x1.cef32p+6,  0x1.ee83dp+6,  0x1.4b6e2cp+7, 0x1.2d2e3ep+7, 0x1.09ab88p+7,
      0x1.1581d2p+7, 0x1.e69fbp+6,  0x1.085ae2p+7, 0x1.cef32p+6,  0x1.ee83dp+6,
      0x1.4b6e2cp+7, 0x1.2d2e3ep+7, 0x1.09ab88p+7, 0x1.1581d2p+7, 0x1.e69fbp+6,
      0x1.085ae2p+7, 0x1.cef32p+6,  0x1.ee83dp+6,  0x1.4b6e2cp+7, 0x1.2d2e3ep+7,
      0x1.09ab88p+7, 0x1.1581d2p+7, 0x1.e69fbp+6,  0x1.085ae2p+7};

  kts::Reference1D<cl_float> refIn = [&InBuffer](size_t x) -> float {
    return InBuffer[x & 0x1f];
  };

  kts::Reference1D<cl_float> refOut = [&OutBuffer](size_t x) -> float {
    return OutBuffer[x];
  };

  AddInputBuffer(1024, refIn);
  AddOutputBuffer(64, refOut);

  RunGenericND(2, global_range, local_range);
}

TEST_P(Execution, Barrier_23_Barrier_Inline_Stray_Phi) {
  const size_t global_size = 16;
  const size_t local_size = 16;

  kts::Reference1D<cl_int> refOut = [](size_t) { return 1; };

  AddOutputBuffer(global_size, refOut);
  RunGeneric1D(global_size, local_size);
}

// A jump table is generated in the .rodata of the ELF file when there are three
// or more barriers present in the kernel. If this table is not correctly
// relocated when loaded, then the offline test variants will seg fault.
TEST_P(Execution, Barrier_24_Three_Barriers) {
  AddOutputBuffer(kts::N, kts::Reference1D<cl_int>(kts::Ref_Identity));
  AddPrimitive(2);
  RunGeneric1D(kts::N);
}
