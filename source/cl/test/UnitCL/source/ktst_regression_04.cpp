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

// This is needed for 'TEST_F'.
using namespace kts::ucl;

TEST_P(Execution, Regression_76_Boscc_Nested_Loops) {
  const cl_uint global = 16;
  const cl_uint read_local = 16;
  const cl_uint n = 16;

  AddOutputBuffer(global, kts::Reference1D<cl_int>([=](size_t gid) {
                    int ret = 1;
                    if (gid < n) {
                      for (size_t i = 0; i < gid; ++i) {
                        const size_t x = n * gid;
                        for (size_t j = 0; j < gid; ++j) {
                          ret += x * j;
                        }
                      }
                    }
                    return ret;
                  }));
  AddPrimitive(n);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_77_Masked_Interleaved_Group) {
  const cl_uint global = 16;
  const cl_uint read_local = 4;

  // it is just a bunch of "random" numbers
  char InBuffer[] = {54, 61, 29, 76, 56, 26, 75, 63,  //
                     29, 86, 57, 34, 37, 15, 91, 56,  //
                     51, 48, 19, 95, 20, 78, 73, 32,  //
                     75, 51, 8,  29, 56, 34, 85, 45};

  kts::Reference1D<cl_uchar> refIn = [=, &InBuffer](size_t x) -> char {
    return InBuffer[x];
  };

  kts::Reference1D<cl_uchar> refOut = [=, &InBuffer](size_t x) -> char {
    if (x & 1) {
      if (InBuffer[x - 1] + InBuffer[x] < 0) {
        return InBuffer[x - 1];
      } else {
        return 0;
      }
    } else {
      if (!(InBuffer[x] + InBuffer[x + 1] < 0)) {
        return InBuffer[x + 1];
      } else {
        return 0;
      }
    }
  };

  const size_t N = sizeof(InBuffer) / sizeof(cl_uchar);
  AddOutputBuffer(N, refOut);
  AddInputBuffer(N, refIn);

  RunGeneric1D(global, read_local);
}

using BuiltinIDParameterTests = ExecutionWithParam<cl_uint>;
TEST_P(BuiltinIDParameterTests, Regression_78_Global_ID_Parameter) {
  cl_uint dim = getParam();
  const size_t global_range[] = {32, 16, 4};
  const size_t local_range[] = {8, 4, 2};

  kts::Reference1D<cl_uint> refOut = [&dim, &global_range](size_t x) {
    if (dim == 0) {
      return static_cast<cl_uint>(x / (global_range[1] * global_range[2]) %
                                  global_range[0]);
    } else if (dim == 1) {
      return static_cast<cl_uint>((x / global_range[2]) % global_range[1]);
    } else if (dim == 2) {
      return static_cast<cl_uint>(x % global_range[2]);
    } else {
      return 0u;
    }
  };

  AddPrimitive<cl_uint>(dim);
  AddOutputBuffer(global_range[0] * global_range[1] * global_range[2], refOut);
  RunGenericND(3, global_range, local_range);
}

TEST_P(BuiltinIDParameterTests, Regression_78_Local_ID_Parameter) {
  cl_uint dim = getParam();
  const size_t global_range[] = {32, 16, 4};
  const size_t local_range[] = {8, 4, 2};

  kts::Reference1D<cl_uint> refOut = [&dim, &global_range,
                                      &local_range](size_t x) {
    if (dim == 0) {
      return static_cast<cl_uint>(x / (global_range[1] * global_range[2]) %
                                  local_range[0]);
    } else if (dim == 1) {
      return static_cast<cl_uint>((x / global_range[2]) % local_range[1]);
    } else if (dim == 2) {
      return static_cast<cl_uint>(x % local_range[2]);
    } else {
      return 0u;
    }
  };

  AddPrimitive<cl_uint>(dim);
  AddOutputBuffer(global_range[0] * global_range[1] * global_range[2], refOut);
  RunGenericND(3, global_range, local_range);
}

TEST_P(BuiltinIDParameterTests, Regression_79_Global_Id_Self_Parameter) {
  cl_uint dim = getParam();

  kts::Reference1D<cl_uint> refOut = [&dim](size_t x) {
    if (dim == 0u) {
      return 0u;
    } else {
      return static_cast<cl_uint>(x);
    }
  };

  AddPrimitive<cl_uint>(0);
  AddPrimitive<cl_uint>(dim);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

UCL_EXECUTION_TEST_SUITE_P(BuiltinIDParameterTests,
                           testing::ValuesIn(getSourceTypes()),
                           testing::Values(0u, 1u, 2u, 3u))

TEST_P(Execution, Regression_79_Global_Id_Zero_Parameter) {
  kts::Reference1D<cl_uint> refOut = [](size_t) { return 0u; };

  AddPrimitive<cl_uint>(0);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_80_Varying_Load) {
  static constexpr cl_uint global = 32;
  static constexpr cl_uint read_local = 16;
  static constexpr cl_int n = 10;
  static constexpr cl_int meta = 1;

  AddOutputBuffer(global, kts::Reference1D<cl_int>([=](size_t id) {
                    int ret = 0;
                    if (id <= 10) {
                      int sum = n;
                      if (meta == 0) {
                        int mul = n * id;
                        const int div = (mul / n) + id;
                        const int shl = div << 3;
                        mul += shl;
                        sum = mul << 3;
                      }
                      if (id % 2 == 0) {
                        sum *= meta + n;
                        ret = sum;
                      }
                    }
                    return ret;
                  }));
  AddPrimitive(n);
  AddInputBuffer(1, kts::Reference1D<cl_int>([](size_t) { return meta; }));

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_81_Boscc_Nested_Loops1) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 5;

  AddOutputBuffer(global, kts::Reference1D<cl_int>([=](size_t id) {
                    int ret = 0;
                    if (id % 2 == 0) {
                      const bool cmp = n == 5;
                      const int mul = n * id;
                      const int div = (mul / n) + id;
                      const int shl = div << 3;
                      const int x = mul + div + shl;
                      for (int i = 0; i < n; ++i) {
                        if (cmp) {
                          ret += x;
                        }
                        if (n % 2 != 0) {
                          if (n > 3) {
                            for (int j = 0; j < n; ++j) {
                              ret++;
                              if (id == 0) {
                                const int mul2 = mul * mul;
                                const int div2 = mul2 / n;
                                const int shl2 = div2 << 3;
                                ret += shl2;
                              }
                              for (int k = 0; k < n; ++k) {
                                ret += x;
                                if (id == 4) {
                                  const int mul2 = mul * mul;
                                  const int div2 = mul2 / n;
                                  const int shl2 = div2 << 3;
                                  ret += shl2;
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                    return ret;
                  }));
  AddPrimitive(n);
  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_81_Boscc_Nested_Loops2) {
  const cl_uint global = 32;
  const cl_uint read_local = 8;
  const cl_int n = 10;

  AddOutputBuffer(global, kts::Reference1D<cl_int>([=](size_t id) {
                    int ret = 0;
                    if (id < 16) {
                      const int mul = n * id;
                      const int div = (mul / n) + id;
                      const int shl = div << 3;
                      const int x = mul + div + shl;
                      for (int i = 0; i < n; ++i) {
                        if (id <= 8) {
                          int j = 0;
                          while (true) {
                            ret++;
                            const int mul2 = mul * mul;
                            const int div2 = mul2 / n;
                            const int shl2 = div2 << 3;
                            ret += shl2 + x;
                            if (id + j++ >= 4) {
                              break;
                            }
                          }
                        }
                      }
                    }
                    return ret;
                  }));
  AddPrimitive(n);
  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_81_Boscc_Nested_Loops3) {
  const cl_uint global = 32;
  const cl_uint read_local = 8;
  const cl_uint n = 16;

  AddOutputBuffer(global, kts::Reference1D<cl_int>([=](size_t id) {
                    int ret = 0;
                    if (id < n) {
                      for (size_t i = 0; i < n; ++i) {
                        const int mul = n * id;
                        const int div = (mul / n) + id;
                        const int shl = div << 3;
                        size_t x = mul + div + shl + i;
                        for (; i < n; ++i) {
                          const int add = x + id;
                          int j = 0;
                          while (true) {
                            ret++;
                            if (x < n) {
                              const int mul2 = mul * mul;
                              const int div2 = mul2 / n;
                              const int shl2 = div2 << 3;
                              ret += shl2 + add;
                            }
                            x++;
                            if (id + j++ >= n) {
                              break;
                            }
                          }
                        }
                      }
                    }
                    return ret;
                  }));
  AddPrimitive(n);
  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_82_Boscc_Merge) {
  const cl_uint global = 32;
  const cl_uint read_local = 4;
  const cl_int n = 1;
  const cl_int m = 0;

  AddOutputBuffer(global, kts::Reference1D<cl_int>([=](size_t id) {
                    int ret = 0;
                    if (id % 2 == 0) {
                      if (n != 0) {
                        int base = 0;
                        if (m == 0) {
                          base = id;
                        } else {
                          if (id % 4 == 0) {
                            base = id;
                          }
                        }
                        ret = base;
                      }
                      ret += 2;
                    }
                    return ret;
                  }));
  AddInputBuffer(global, kts::Ref_Identity);
  AddPrimitive(n);
  AddPrimitive(m);
  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_83_Vecz_Lcssa) {
  const cl_uint global = 4;
  const cl_uint read_local = 4;
  const cl_int n = 5;

  AddOutputBuffer(global, kts::Reference1D<cl_int>([=](size_t id) {
                    int ret = 0;
                    if (id % 2 == 0) {
                      const int mul = n * id;
                      const int div = (mul / n) + id;
                      const int shl = div << 3;
                      const int x = mul + div + shl;
                      for (int i = 0; i < n; ++i) {
                        if (id <= 8) {
                          for (size_t j = 0; j < id; ++j) {
                            ret++;
                            const int mul2 = mul * mul;
                            const int div2 = mul2 / n;
                            const int shl2 = div2 << 3;
                            ret += shl2 + x;
                            if (id >= 4) {
                              break;
                            }
                          }
                        }
                      }
                    }
                    return ret;
                  }));
  AddPrimitive(n);
  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_84_Vecz_Merge) {
  const cl_uint global = 4;
  const cl_uint read_local = 4;
  const cl_uint n = 5;

  AddOutputBuffer(global, kts::Reference1D<cl_int>([=](size_t id) {
                    int ret = 0;
                    while (1) {
                      if (n > 0 && n < 5) {
                        goto f;
                      }
                      while (1) {
                        if (n <= 2) {
                          ret = 5;
                          goto f;
                        } else {
                          if (ret + id >= n) {
                            ret = id;
                            goto d;
                          }
                        }
                        if (n & 1) {
                          ret = 1;
                          goto f;
                        }

                      d:
                        if (n > 3) {
                          ret = n;
                          goto e;
                        }
                      }

                    e:
                      if (n & 1) {
                        ret = n + 2;
                        goto f;
                      }
                    }

                  f:
                    return ret;
                  }));
  AddPrimitive(n);
  RunGeneric1D(global, read_local);
}
TEST_P(Execution, Regression_85_Scan_Fact) {
  const cl_uint global = 8;
  const cl_uint read_local = global / 2;

  std::vector<int64_t> in(global);
  std::iota(in.begin(), in.end(), 1);
  std::vector<int64_t> scan_fact(in.size());
  std::partial_sum(in.begin(), in.end(), scan_fact.begin(),
                   std::multiplies<int64_t>{});

  AddOutputBuffer(global, kts::Reference1D<cl_int>([scan_fact](size_t id) {
                    return scan_fact[id];
                  }));
  AddInputBuffer(global,
                 kts::Reference1D<cl_int>([in](size_t id) { return in[id]; }));

  // Scalar kernel does the work of two work items.
  RunGeneric1D(read_local, read_local);
}

TEST_P(Execution, Regression_86_Store_Local) {
  fail_if_not_vectorized_ = false;
  const cl_uint global = 8;
  const cl_uint local = 2;
  const cl_uint n = 3;

  AddOutputBuffer(global, kts::Reference1D<cl_int>([=](size_t) { return n; }));
  AddPrimitive(n);

  RunGeneric1D(global, local);
}

TEST_P(Execution, Regression_87_Pow_Powr) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  // Tests inputs found from the CTS which we didn't previously meet the 16 ULP
  // precision requirements for.
  const size_t N = 5;
  const std::pair<cl_float, cl_float> inputs[N] = {
      // x: 1.3395461, y: -284.7274
      {cargo::bit_cast<cl_float>(0x3fab763f),
       cargo::bit_cast<cl_float>(0xc38e5d1b)},
      // x: 1.3317101, y: -295.75696
      {cargo::bit_cast<cl_float>(0x3faa757a),
       cargo::bit_cast<cl_float>(0xc393e0e4)},
      // x: 1.3239887, y: -296.94553
      {cargo::bit_cast<cl_float>(0x3fa97876),
       cargo::bit_cast<cl_float>(0xc3947907)},
      // x: 1.3421836, y: -285.04593
      {cargo::bit_cast<cl_float>(0x3fabccac),
       cargo::bit_cast<cl_float>(0xc38e85e1)},
      // x: 1.3375553, y: 304.99103
      {cargo::bit_cast<cl_float>(0x3fab3503),
       cargo::bit_cast<cl_float>(0x43987eda)},
  };

  AddInputBuffer(N, kts::Reference1D<cl_float>([&inputs](size_t i) {
                   return std::get<0>(inputs[i]);
                 }));
  AddInputBuffer(N, kts::Reference1D<cl_float>([&inputs](size_t i) {
                   return std::get<1>(inputs[i]);
                 }));

  const auto validator = makeULPStreamer<cl_float, 16_ULP>(
      [&inputs](size_t i) -> cl_double {
        const cl_double x = static_cast<cl_double>(std::get<0>(inputs[i]));
        const cl_double y = static_cast<cl_double>(std::get<1>(inputs[i]));
        return std::pow(x, y);
      },
      this->device);

  AddOutputBuffer(N, validator);
  AddOutputBuffer(N, validator);

  RunGeneric1D(N);
}

TEST_P(Execution, Regression_88_Vstore_Loop) {
  AddOutputBuffer(kts::N,
                  kts::Reference1D<cl_float>([](size_t) { return 1.f; }));
  AddPrimitive<cl_long>(kts::N);

  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_88_Scalar_Loop_Tail) {
  AddOutputBuffer(kts::N,
                  kts::Reference1D<cl_float>([](size_t) { return 1.f; }));
  AddPrimitive<cl_long>(kts::N);

  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_89_Multiple_Local_Memory_Kernels) {
  // Whether or not the kernel will be vectorized at a local size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  const size_t global_work_x = 16;
  const size_t local_work_x = 1;

  AddMacro("LOCAL_X", local_work_x);

  AddInputBuffer(global_work_x * local_work_x, kts::Ref_Identity);

  AddOutputBuffer(global_work_x * local_work_x, kts::Ref_Identity);

  RunGeneric1D(global_work_x, local_work_x);
}

TEST_P(Execution, Regression_90_Offline_Local_Memcpy) {
  AddLocalBuffer<cl_int>(kts::localN);
  AddOutputBuffer(kts::localN, kts::Ref_Identity);
  RunGeneric1D(kts::localN, kts::localN);  // Only the first WG is valid.
}

TEST_P(Execution, Regression_90_Offline_Local_Memcpy_Fixed) {
  fail_if_not_vectorized_ = false;
  const size_t local_size = 17;  // Kernel uses reqd_work_group_size(17,1,1);
  AddLocalBuffer<cl_int>(local_size);
  AddOutputBuffer(local_size, kts::Ref_Identity);
  RunGeneric1D(local_size, local_size);  // Only the first WG is valid.
}

TEST_P(Execution, Regression_91_Loop_Bypass_Branch) {
  cl_int bound = 16;
  kts::Reference1D<cl_int> refIn = [](size_t x) {
    return kts::Ref_Identity(x) - 33;
  };
  kts::Reference1D<cl_int> refOut = [&refIn, &bound](size_t x) {
    cl_int val = refIn(x);
    if (val >= 4) {
      val += 1;
    }
    while (val < 0) {
      val += bound;
    }
    return val;
  };
  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  AddPrimitive(bound);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_92_Danger_Div_Hoist) {
  const cl_uint global = 256;
  const cl_uint read_local = 16;
  const cl_int r = 1234;

  AddOutputBuffer(global, kts::Reference1D<cl_int>([=](size_t id) {
                    cl_int result = r;
                    const cl_int div = ((id * 237) & 0xF);
                    if (div != 0) {
                      result /= div;
                    }
                    return result;
                  }));
  AddPrimitive(r);

  RunGeneric1D(global, read_local);
}

// Long divisions are executed in software on x86, so make sure that works
TEST_P(Execution, Regression_92_Danger_Div_Hoist_Long) {
  const cl_uint global = 256;
  const cl_uint read_local = 16;
  const cl_long r = 1234;

  AddOutputBuffer(global, kts::Reference1D<cl_int>([=](size_t id) {
                    cl_long result = r;
                    const cl_long div = ((id * 237) & 0xF);
                    if (div != 0) {
                      result /= div;
                    }
                    return result;
                  }));
  AddPrimitive(r);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_93_Ashr_Index_Underflow_1) {
  const cl_uint global = kts::N;
  const cl_uint read_local = 16;

  auto refIn = kts::Reference1D<cl_int2>(
      [=](size_t id) { return cl_int2{{kts::Ref_A(id), 0}}; });

  auto refOut =
      kts::Reference1D<cl_int>([=](size_t id) { return refIn(id >> 1).x; });

  AddInputBuffer(global >> 1, refIn);
  AddOutputBuffer(global, refOut);

  RunGeneric1D(global, read_local);
}

TEST_P(Execution, Regression_93_Ashr_Index_Underflow_2) {
  const cl_uint global = kts::N / 2;
  const cl_uint read_local = 16;

  auto refIn = kts::Reference1D<cl_int2>(
      [=](size_t id) { return cl_int2{{kts::Ref_A(id), 0}}; });

  auto refOut = kts::Reference1D<cl_int>(
      [=](size_t id) { return refIn((id * 3) >> 1).x; });

  AddInputBuffer((global * 3) >> 1, refIn);
  AddOutputBuffer(global, refOut);

  RunGeneric1D(global, read_local);
}

// The test is so-named because the "goto F" sneaks into the BOSCC SESE region
// without passing through that region's divergence-causing entry block.
TEST_P(Execution, Regression_94_Boscc_Sese_Backdoor) {
  const size_t global_range[] = {256, 256};
  const size_t local_range[] = {16, 1};

  auto refOut = kts::Reference1D<cl_uint>([=](size_t id) {
    const size_t x = id & 0xFF;
    const size_t y = id >> 8;
    const cl_ushort scrambled_x = (cl_ushort(x) ^ 0x4785) * 0x8257;
    const cl_ushort scrambled_y = (cl_ushort(y) ^ 0x126C) * 0x1351;

    cl_uint route = 0;
    if (scrambled_y & 1) {
      route |= 1;
      if (scrambled_y & 2) {
        route ^= scrambled_y;
      }
      goto F;
    } else {
      route |= 8;
      if (scrambled_x & 1) {
        route |= 16;
        goto G;
      }
    }

  F:
    route |= 32;

  G:
    return route;
  });

  AddOutputBuffer(global_range[0] * global_range[1], refOut);

  RunGenericND(2, global_range, local_range);
}

TEST_P(Execution, Regression_95_Illegal_Uniform_Stride) {
  const cl_uint global = 256;
  const cl_uint read_local = 16;

  auto refOut = kts::Reference1D<cl_uint>([=](size_t x) {
    const cl_int y = x - 1;
    if (y >= 0) {
      return kts::Ref_A(cl_uint(y));
    } else {
      return 0;
    }
  });

  AddOutputBuffer(global, refOut);
  AddInputBuffer(global, kts::Ref_A);

  RunGeneric1D(global, read_local);
}

// This test primarily exists because `clc` had a bug where it would segfault
// looking for a magic number in a zero byte file, and the Execution framework
// is the best way to exercise `clc`.  The test doesn't actually need to do
// anything to exercise that, and if it tries the framework fails to build the
// program as it expected there to be a kernel called 'zero_byte_file'.
TEST_P(Execution, Regression_96_Zero_Byte_File) {
  // Deliberately empty.
}

TEST_P(Execution, Regression_97_Libm_Functions) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  auto refOne = kts::Reference1D<cl_float>([](size_t) { return 1.0f; });

  const size_t num_functions = 14;
  AddBuildOption("-cl-fast-relaxed-math");
  AddInputBuffer(num_functions, refOne);
  AddOutputBuffer(num_functions, refOne);
  RunGeneric1D(1, 1);
}

TEST_P(Execution, Regression_97_Libm_Functions_Double) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  if (!UCL::hasDoubleSupport(this->device)) {
    GTEST_SKIP();
  }

  auto refOne = kts::Reference1D<cl_double>([](size_t) { return 1.0; });

  const size_t num_functions = 7;
  AddBuildOption("-cl-fast-relaxed-math");
  AddInputBuffer(num_functions, refOne);
  AddOutputBuffer(num_functions, refOne);
  RunGeneric1D(1, 1);
}

// Regression_98_Store_Uniform_Pointer tests that we handle uniform stores with
// varying values correctly according to OpenCL. Work item ordering is undefined
// so for VECZ we just take the first instance of the store and avoid the need
// to instantiate it.
//
// For this test, the stored value depends on the global ID
// multiplied by a scalar kernel argument. We pass this as zero to effectively
// ensure uniform behaviour so that we do not see a difference in result between
// scalar and vectorized variants of the test.
TEST_P(Execution, Regression_98_Store_Uniform_Pointer) {
  kts::Reference1D<cl_int> refIn = [](size_t) { return 42; };

  kts::Reference1D<cl_int> refOut = [](size_t gid) {
    if (gid == 3) {
      return 7;
    } else {
      return 42;
    }
  };

  AddInOutBuffer(8, refIn, refOut);
  AddPrimitive(0);
  RunGeneric1D(8);
}

TEST_P(Execution, Regression_99_As_Double3_Inline) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  // On x86 our implementation of the as_type function for double3 can
  // erroneously flip some bits if it isn't inlined. This was determined to be
  // due to behaviour in llvm which isn't conclusively incorrect, so we have
  // accepted that this builtin must be inlined to function 100% correctly. This
  // test should create the circumstances under which bits will flip, if the
  // builtin fails to inline for some reason.
  if (!UCL::hasDoubleSupport(this->device)) {
    GTEST_SKIP();
  }

  // This value is a minimal bitpattern for a double NaN: all exponent bits set
  // plus the lowest mantissa bit.
  auto ref =
      kts::Reference1D<cl_ulong>([](size_t) { return 0x7ff0000000000001; });
  AddInputBuffer(3, ref);
  AddOutputBuffer(3, ref);
  RunGeneric1D(1, 1);
}

// Dividing an integer by zero may result in an unspecified value, but not an
// exception or undefined behaviour.
TEST_P(Execution, Regression_100_Integer_Zero_Divide) {
  AddInOutBuffer(kts::N, kts::Ref_A, kts::Ref_Identity);
  RunGeneric1D(kts::N);
}

// Do not add tests beyond Regression_100* here, or the file may become too
// large to link. Instead, start a new ktst_regression_${NN}.cpp file.
