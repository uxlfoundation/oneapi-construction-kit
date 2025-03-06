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

// This test contains tests for various vectorization failures

#include <string>
#include <utility>

#include "Common.h"
#include "Device.h"
#include "kts/vecz_tasks_common.h"

using namespace kts::ucl;

TEST_P(Execution, Task_10_01_Shuffle_Constant) {
  // TODO: CA-2214: Remove when fixed upstream
  if (UCL::isDevice_Oclgrind(this->device) ||
      UCL::isDevice_IntelNeo(this->device)) {
    GTEST_SKIP();
  }
  auto refIn1 = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
  auto refIn2 = kts::BuildVec2Reference1D<cl_int2>(kts::Ref_A);

  kts::Reference1D<cl_int2> refOut = [](size_t x) {
    return cl_int2{{kts::Ref_A((2 * x) + 1), kts::Ref_A((4 * x) + 2)}};
  };

  AddInputBuffer(kts::N, refIn1);
  AddInputBuffer(kts::N, refIn2);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_10_02_Shuffle_Runtime) {
  // TODO: CA-2214: Remove when fixed upstream
  if (UCL::isDevice_Oclgrind(this->device) ||
      UCL::isDevice_IntelNeo(this->device)) {
    GTEST_SKIP();
  }
  auto refIn1 = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
  auto refIn2 = kts::BuildVec2Reference1D<cl_int2>(kts::Ref_A);

  kts::Reference1D<cl_int2> refOut = [](size_t x) {
    return cl_int2{{kts::Ref_A((2 * x) + 1), kts::Ref_A((4 * x) + 2)}};
  };

  AddInputBuffer(kts::N, refIn1);
  AddInputBuffer(kts::N, refIn2);
  AddPrimitive(cl_uint4{{0, 1, 1, 0}});
  // The large index is used to test if only the correct bits are taken into
  // consideration for the mask.
  AddPrimitive(cl_uint2{{11098, 6}});
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_10_03_Vector_Loop) {
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  RunGeneric1D(kts::N);
}

using TypeAndFunctionParam = std::pair<std::string, std::string>;
using OneArgRelationals = ExecutionWithParam<TypeAndFunctionParam>;

TEST_P(OneArgRelationals, Task_10_04_OneArg_Relationals) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection does not support rebuilding a program.
  }
  auto Param = getParam();
  const std::string InTy = Param.first;
  const std::string Function = Param.second;

  // Skip the double tests if we don't have doubles support
  if (InTy.find("double") != InTy.npos && !UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  // Expected inputs and outputs
  cl_float InF[] = {0.0f, -1.0f, std::numeric_limits<cl_float>::infinity(),
                    stdcompat::nanf(""), 0.0f};
  cl_double InD[] = {0.0, -1.0, std::numeric_limits<cl_double>::infinity(),
                     stdcompat::nan(""), 0.0};

  // Create denormals at the fifth position of the inputs
  char *data = reinterpret_cast<char *>(&InF[4]);
  int32_t denormal_f = 12;
  memcpy(data, &denormal_f, 4);
  data = reinterpret_cast<char *>(&InD[4]);
  int64_t denormal_d = 12;
  memcpy(data, &denormal_d, 8);

  // Note: Make sure all the inputs have the same size
  const cl_int OutIsFinite[] = {1, 1, 0, 0, 1};
  const cl_int OutIsInf[] = {0, 0, 1, 0, 0};
  const cl_int OutIsNormal[] = {0, 1, 0, 0, 0};
  const cl_int OutIsNan[] = {0, 0, 0, 1, 0};
  const cl_int OutSignBit[] = {0, 1, 0, 0, 0};
  const int InputSize = sizeof(OutIsFinite) / sizeof(cl_int);

  // Determine the output based on the function being tested
  AddMacro("FUNC", Function);
  const cl_int *Out = nullptr;

  if (Function == "isfinite") {
    Out = OutIsFinite;
  } else if (Function == "isinf") {
    Out = OutIsInf;
  } else if (Function == "isnormal") {
    Out = OutIsNormal;
  } else if (Function == "isnan") {
    Out = OutIsNan;
  } else if (Function == "signbit") {
    Out = OutSignBit;
  } else {
    // Unsupported test case
    FAIL();
  }

  // Create the input and output buffers based on the function and type being
  // tested
  AddMacro("IN_TY", InTy);

  if (InTy == "float") {
    AddMacro("OUT_TY", "int");
    // Construct the input and expected output
    kts::Reference1D<cl_float> RefIn = [=, &InF](size_t x) {
      return InF[x % InputSize];
    };
    kts::Reference1D<cl_int> RefOut = [=, &Out](size_t x) {
      return Out[x % InputSize];
    };

    AddInputBuffer(kts::N, RefIn);
    AddOutputBuffer(kts::N, RefOut);
  } else if (InTy == "float4") {
    AddMacro("OUT_TY", "int4");
    // Construct the input and expected output
    kts::Reference1D<cl_float4> RefIn = [=, &InF](size_t x) {
      return cl_float4{{InF[x % InputSize], InF[(x + 1) % InputSize],
                        InF[(x + 2) % InputSize], InF[(x + 3) % InputSize]}};
    };
    kts::Reference1D<cl_int4> RefOut = [=, &Out](size_t x) {
      auto Out0 = Out[x % InputSize] ? -1 : 0;
      auto Out1 = Out[(x + 1) % InputSize] ? -1 : 0;
      auto Out2 = Out[(x + 2) % InputSize] ? -1 : 0;
      auto Out3 = Out[(x + 3) % InputSize] ? -1 : 0;
      return cl_int4{{Out0, Out1, Out2, Out3}};
    };

    AddInputBuffer(kts::N, RefIn);
    AddOutputBuffer(kts::N, RefOut);
  } else if (InTy == "double") {
    AddMacro("OUT_TY", "int");
    // Construct the input and expected output
    kts::Reference1D<cl_double> RefIn = [=, &InD](size_t x) {
      return InD[x % InputSize];
    };
    kts::Reference1D<cl_int> RefOut = [=, &Out](size_t x) {
      return Out[x % InputSize];
    };

    AddInputBuffer(kts::N, RefIn);
    AddOutputBuffer(kts::N, RefOut);
  } else if (InTy == "double4") {
    AddMacro("OUT_TY", "long4");
    // Construct the input and expected output
    kts::Reference1D<cl_double4> RefIn = [=, &InD](size_t x) {
      return cl_double4{{InD[x % InputSize], InD[(x + 1) % InputSize],
                         InD[(x + 2) % InputSize], InD[(x + 3) % InputSize]}};
    };
    kts::Reference1D<cl_long4> RefOut = [=, &Out](size_t x) {
      long Out0 = Out[x % InputSize] ? -1 : 0;
      long Out1 = Out[(x + 1) % InputSize] ? -1 : 0;
      long Out2 = Out[(x + 2) % InputSize] ? -1 : 0;
      long Out3 = Out[(x + 3) % InputSize] ? -1 : 0;
      return cl_long4{{Out0, Out1, Out2, Out3}};
    };

    AddInputBuffer(kts::N, RefIn);
    AddOutputBuffer(kts::N, RefOut);
  } else {
    // Unsupported test case
    FAIL();
  }

  // Execute the kernel
  RunGeneric1D(kts::N);
}

UCL_EXECUTION_TEST_SUITE_P(
    OneArgRelationals, testing::Values(OPENCL_C),
    testing::Values(TypeAndFunctionParam{"float", "isfinite"},
                    TypeAndFunctionParam{"double", "isfinite"},
                    TypeAndFunctionParam{"float4", "isfinite"},
                    TypeAndFunctionParam{"double4", "isfinite"},
                    TypeAndFunctionParam{"float", "isinf"},
                    TypeAndFunctionParam{"double", "isinf"},
                    TypeAndFunctionParam{"float4", "isinf"},
                    TypeAndFunctionParam{"double4", "isinf"},
                    TypeAndFunctionParam{"float", "isnormal"},
                    TypeAndFunctionParam{"double", "isnormal"},
                    TypeAndFunctionParam{"float4", "isnormal"},
                    TypeAndFunctionParam{"double4", "isnormal"},
                    TypeAndFunctionParam{"float", "isnan"},
                    TypeAndFunctionParam{"double", "isnan"},
                    TypeAndFunctionParam{"float4", "isnan"},
                    TypeAndFunctionParam{"double4", "isnan"},
                    TypeAndFunctionParam{"float", "signbit"},
                    TypeAndFunctionParam{"double", "signbit"},
                    TypeAndFunctionParam{"float4", "signbit"},
                    TypeAndFunctionParam{"double4", "signbit"}))

// Set local workgroup size to be the same as global work size, otherwise the
// test is assuming that atomic operations have global scope, which is not
// required by the OpenCL spec.
TEST_P(Execution, Task_10_05_Atomic_CmpXchg_Builtin) {
  fail_if_not_vectorized_ = false;
  auto streamer(std::make_shared<AtomicStreamer<cl_int>>(-1, kts::localN));
  AddOutputBuffer(kts::BufferDesc(1, streamer));
  AddOutputBuffer(kts::BufferDesc(kts::localN, streamer));
  RunGeneric1D(kts::localN, kts::localN);
}

TEST_P(Execution, Task_10_06_NoInline_Kernels) {
  fail_if_not_vectorized_ = false;
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_10_07_Break_Loop) {
  kts::Reference1D<cl_int> Zero = [](size_t) { return 0; };
  AddInputBuffer(kts::N, Zero);
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_10_08_InsertElement_Constant_Index) {
  auto refIn = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
  kts::Reference1D<cl_int4> refOut = [](size_t x) {
    cl_int a = kts::Ref_A(4 * x);
    cl_int b = kts::Ref_A((4 * x) + 1);
    cl_int c = 42;
    cl_int d = kts::Ref_A((4 * x) + 3);
    return cl_int4{{a, b, c, d}};
  };
  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_10_09_InsertElement_Runtime_Index) {
  kts::Reference1D<cl_int> Indices = [](size_t x) {
    return kts::Ref_Identity(x) % 4;
  };
  auto refIn = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
  kts::Reference1D<cl_int4> refOut = [](size_t x) {
    cl_int a = (x % 4 == 0) ? 42 : kts::Ref_A(4 * x);
    cl_int b = (x % 4 == 1) ? 42 : kts::Ref_A((4 * x) + 1);
    cl_int c = (x % 4 == 2) ? 42 : kts::Ref_A((4 * x) + 2);
    cl_int d = (x % 4 == 3) ? 42 : kts::Ref_A((4 * x) + 3);
    return cl_int4{{a, b, c, d}};
  };
  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  AddInputBuffer(kts::N, Indices);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_10_10_ExtractElement_Constant_Index) {
  auto refIn = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
  kts::Reference1D<cl_int4> refOut = [](size_t x) {
    cl_int a = 4;
    cl_int b = 4;
    cl_int c = kts::Ref_A(4 * x);
    ;
    cl_int d = 4;
    return cl_int4{{a, b, c, d}};
  };
  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_10_11_ExtractElement_Runtime_Index) {
  kts::Reference1D<cl_int> Indices = [](size_t x) {
    return kts::Ref_Identity(x) % 4;
  };
  auto refIn = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
  kts::Reference1D<cl_int4> refOut = [](size_t x) {
    cl_int a = 4;
    cl_int b = 4;
    cl_int c = kts::Ref_A(4 * x);
    cl_int d = 4;
    return cl_int4{{a, b, c, d}};
  };
  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N, refOut);
  AddInputBuffer(kts::N, Indices);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_10_12_Intptr_Cast) {
  AddInputBuffer(kts::N * 4, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Task_10_13_Intptr_Cast_Phi) {
  AddInputBuffer(kts::N * 4, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  RunGeneric1D(kts::N / 4);
}
