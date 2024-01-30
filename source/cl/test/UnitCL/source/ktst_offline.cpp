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

// Some functions are handled differently between MSVC and other compilers. We
// use this for the printf tests
#if defined(_MSC_VER)
#include <io.h>
#define DUP _dup
#define DUP2 _dup2
#define SNPRINTF _snprintf
#else
#include <unistd.h>
#define DUP dup
#define DUP2 dup2
#define SNPRINTF snprintf
#endif

#include <cargo/string_view.h>
#include <gtest/gtest.h>

#include <fstream>
#include <iterator>
#include <numeric>
#include <string>

#include "Common.h"
#include "kts/execution.h"
#include "kts/reference_functions.h"

// This is needed for 'TEST_F'.
using namespace kts::ucl;

TEST_P(Execution, Offline_01_NoOp) { RunGeneric1D(kts::N, 2); }

TEST_P(Execution, Offline_02_Add) {
  AddInputBuffer(kts::N, kts::Ref_A);
  AddInputBuffer(kts::N, kts::Ref_B);
  AddOutputBuffer(kts::N, kts::Ref_Add);
  RunGeneric1D(kts::N);
}

namespace {
float Ref_float_A(size_t x) { return 2.0f * x; }
float Ref_float_B(size_t x) { return 4.0f * x; }
float Ref_float_Add(size_t x) { return 6.0f * x; }
}  // namespace

TEST_P(Execution, Offline_02_AddF) {
  AddInputBuffer(kts::N, Ref_float_A);
  AddInputBuffer(kts::N, Ref_float_B);
  AddOutputBuffer(kts::N, Ref_float_Add);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Offline_03_Add4) {
  auto refIn1 = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_A);
  auto refIn2 = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_B);
  auto refOut = kts::BuildVec4Reference1D<cl_int4>(kts::Ref_Add);
  AddInputBuffer(kts::N, refIn1);
  AddInputBuffer(kts::N, refIn2);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Offline_03_Add4F) {
  auto refIn1 = kts::BuildVec4Reference1D<cl_float4>(Ref_float_A);
  auto refIn2 = kts::BuildVec4Reference1D<cl_float4>(Ref_float_B);
  auto refOut = kts::BuildVec4Reference1D<cl_float4>(Ref_float_Add);
  AddInputBuffer(kts::N, refIn1);
  AddInputBuffer(kts::N, refIn2);
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Offline_04_MultiKernel) {
  AddInputBuffer(kts::N, kts::Ref_A);
  AddInputBuffer(kts::N, kts::Ref_B);
  AddOutputBuffer(kts::N, kts::Ref_Add);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Offline_04_MultiKernel_WGS) {
  const size_t N = 8 * 64;
  AddInputBuffer(N, kts::Ref_A);
  AddInputBuffer(N, kts::Ref_B);
  AddOutputBuffer(N, kts::Ref_Add);
  RunGeneric1D(N, 8);
}

// SPIR-V and Offline SPIR-V tests are disabled as this is testing build option
// only supported by the runtime compiler or clc.
// TODO(CA-3992) Fix this test.
TEST_P(Execution, DISABLED_Offline_04_MultiKernel_WGS_VecZ) {
  if (!isSourceTypeIn({OPENCL_C, OFFLINE}) ||
      !this->isDeviceExtensionSupported("cl_codeplay_extra_build_options") ||
      !this->getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  this->fail_if_not_vectorized_ = true;
  const size_t N = 8 * 64;
  AddInputBuffer(N, kts::Ref_A);
  AddInputBuffer(N, kts::Ref_B);
  AddOutputBuffer(N, kts::Ref_Add);
  RunGeneric1D(N, 8);
}

TEST_P(Execution, Offline_05_Relocation) {
  AddInputBuffer(kts::N, kts::Ref_A);
  AddInputBuffer(kts::N, kts::Ref_B);
  AddOutputBuffer(kts::N, kts::Ref_Add);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Offline_06_Twokernel) {
  kts::Reference1D<cl_float> refOut = [](size_t) -> cl_float { return 7.4f; };
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Offline_06_Twokernel_Different_Types) {
  kts::Reference1D<cl_int> refOut = [](size_t) -> cl_float { return 8; };
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Offline_06_Twokernel_Different_Number_Types) {
  kts::Reference1D<cl_float> refOut = [](size_t) -> cl_float { return 7.4f; };
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Offline_06_Twokernel_Different_Number_Types_Swapped) {
  kts::Reference1D<cl_float> refOutA = [](size_t) -> cl_float { return 7.4f; };
  kts::Reference1D<cl_float> refOutB = [](size_t) -> cl_float { return 8.4f; };
  AddOutputBuffer(kts::N, refOutA);
  AddOutputBuffer(kts::N, refOutB);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Offline_07_Primitive) {
  cl_float val = 7.4f;
  kts::Reference1D<cl_float> refOut = [&val](size_t) -> cl_float {
    return val;
  };
  AddOutputBuffer(kts::N, refOut);
  AddPrimitive<cl_float>(val);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Offline_07_TwoKernel_Primitive) {
  cl_float val = 7.4f;
  kts::Reference1D<cl_float> refOut = [&val](size_t) -> cl_float {
    return val;
  };
  AddOutputBuffer(kts::N, refOut);
  AddPrimitive<cl_float>(val);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Offline_07_TwoKernel_Int_Primitive) {
  cl_int val = 8;
  kts::Reference1D<cl_float> refOut = [&val](size_t) -> cl_float {
    return (cl_float)val;
  };
  AddOutputBuffer(kts::N, refOut);
  AddPrimitive<cl_int>(val);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Offline_07_TwoKernel_Both_Primitive) {
  cl_float val = 7.4f;
  kts::Reference1D<cl_float> refOut = [&val](size_t) -> cl_float {
    return val;
  };
  AddOutputBuffer(kts::N, refOut);
  AddPrimitive<cl_float>(val);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Offline_07_TwoKernel_Different_Primitive) {
  cl_float val = 7.4f;
  kts::Reference1D<cl_float> refOut = [&val](size_t) -> cl_float {
    return val;
  };
  AddOutputBuffer(kts::N, refOut);
  AddPrimitive<cl_float>(val);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Offline_08_NoKernel) {
  // This is nothing that can go here because the according kernel has no
  // kernel within it.  This test is mostly here as a placeholder, it takes 0ms
  // to run.  The real test is did we manage to build the offline binary when
  // UnitCL was built.
}

class OfflineExecutionType : public BaseExecution {
 public:
  void SetUp() override {
    source_type = OFFLINE;
    UCL_RETURN_ON_FATAL_FAILURE(BaseExecution::SetUp());
    if (!isSourceTypeIn({OFFLINE})) {
      GTEST_SKIP();
    }
  }

  void runTest() {
    // Extract the type name from the test name
    const std::string testName =
        ::testing::UnitTest::GetInstance()->current_test_info()->name();
    auto typeName =
        testName.substr(testName.find_last_of('_') + 1, std::string::npos);

    // Check if the type is supported and skip if not
    const cargo::string_view param{typeName};
    if (param.starts_with("half") && !UCL::hasHalfSupport(device)) {
      GTEST_SKIP();
    }
    if (param.starts_with("double") && !UCL::hasDoubleSupport(device)) {
      GTEST_SKIP();
    }

    if (!BuildProgram()) {
      GTEST_SKIP();
    }

    struct ArgumentDescriptor {
      cargo::string_view name;
      cargo::string_view type;
      cl_kernel_arg_address_qualifier addressQualifier;
      cl_kernel_arg_access_qualifier accessQualifier;
      cl_kernel_arg_type_qualifier typeQualifier;
    };

    // Enumerate the expected results from clGetKernelArgInfo
    auto ptrTypeName = typeName + "*";
    std::array<ArgumentDescriptor, 7> argDescs = {{
        {"in_p", typeName, CL_KERNEL_ARG_ADDRESS_PRIVATE,
         CL_KERNEL_ARG_ACCESS_NONE, CL_KERNEL_ARG_TYPE_NONE},
        {"in_g", ptrTypeName, CL_KERNEL_ARG_ADDRESS_GLOBAL,
         CL_KERNEL_ARG_ACCESS_NONE, CL_KERNEL_ARG_TYPE_NONE},
        {"in_c", ptrTypeName, CL_KERNEL_ARG_ADDRESS_CONSTANT,
         CL_KERNEL_ARG_ACCESS_NONE, CL_KERNEL_ARG_TYPE_CONST},
        {"in_l", ptrTypeName, CL_KERNEL_ARG_ADDRESS_LOCAL,
         CL_KERNEL_ARG_ACCESS_NONE, CL_KERNEL_ARG_TYPE_NONE},
        {"in_gv", ptrTypeName, CL_KERNEL_ARG_ADDRESS_GLOBAL,
         CL_KERNEL_ARG_ACCESS_NONE, CL_KERNEL_ARG_TYPE_VOLATILE},
        {"in_lc", ptrTypeName, CL_KERNEL_ARG_ADDRESS_LOCAL,
         CL_KERNEL_ARG_ACCESS_NONE, CL_KERNEL_ARG_TYPE_CONST},
        {"in_lr", ptrTypeName, CL_KERNEL_ARG_ADDRESS_LOCAL,
         CL_KERNEL_ARG_ACCESS_NONE, CL_KERNEL_ARG_TYPE_RESTRICT},
    }};

    cl_uint num_args;
    ASSERT_SUCCESS(clGetKernelInfo(kernel_, CL_KERNEL_NUM_ARGS,
                                   sizeof(num_args), &num_args, nullptr));
    ASSERT_EQ(argDescs.size(), num_args);

    for (cl_uint argIndex = 0; argIndex < argDescs.size(); argIndex++) {
      auto &argDesc = argDescs[argIndex];

      // Argument name
      size_t size;
      ASSERT_SUCCESS(clGetKernelArgInfo(kernel_, argIndex, CL_KERNEL_ARG_NAME,
                                        0, nullptr, &size));
      std::string name(size, '\0');
      ASSERT_SUCCESS(clGetKernelArgInfo(kernel_, argIndex, CL_KERNEL_ARG_NAME,
                                        size, name.data(), nullptr));
      if (!name.empty()) {
        name.resize(strlen(name.data()));
      }
      ASSERT_EQ(argDesc.name, name);

      // Argument type name
      ASSERT_SUCCESS(clGetKernelArgInfo(
          kernel_, argIndex, CL_KERNEL_ARG_TYPE_NAME, 0, nullptr, &size));
      std::string type(size, '\0');
      ASSERT_SUCCESS(clGetKernelArgInfo(kernel_, argIndex,
                                        CL_KERNEL_ARG_TYPE_NAME, size,
                                        type.data(), nullptr));
      if (!type.empty()) {
        type.resize(strlen(type.data()));
      }
      ASSERT_EQ(argDesc.type, type);

      // Argument address qualifier
      cl_kernel_arg_address_qualifier addressQualifier;
      ASSERT_SUCCESS(clGetKernelArgInfo(
          kernel_, argIndex, CL_KERNEL_ARG_ADDRESS_QUALIFIER,
          sizeof(addressQualifier), &addressQualifier, nullptr));
      ASSERT_EQ(argDesc.addressQualifier, addressQualifier);

      // Argument access qualifier
      cl_kernel_arg_access_qualifier accessQualifier;
      ASSERT_SUCCESS(clGetKernelArgInfo(
          kernel_, argIndex, CL_KERNEL_ARG_ACCESS_QUALIFIER,
          sizeof(accessQualifier), &accessQualifier, nullptr));
      ASSERT_EQ(argDesc.accessQualifier, accessQualifier);

      // Argument type qualifier
      cl_kernel_arg_type_qualifier typeQualifier;
      ASSERT_SUCCESS(
          clGetKernelArgInfo(kernel_, argIndex, CL_KERNEL_ARG_TYPE_QUALIFIER,
                             sizeof(typeQualifier), &typeQualifier, nullptr));
      ASSERT_EQ(argDesc.typeQualifier, typeQualifier);
    }
  }
};

// This is not pretty but in order to take advantage of the Execution framework
// each test must have a different name in order to load the correct program
// from the filesystem. ExecutionWithParam is not suitable in this case as it
// assumes that the same program will be used for all parameterizations.
TEST_F(OfflineExecutionType, Offline_01_type_char) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_char2) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_char3) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_char4) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_char8) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_char16) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_uchar) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_uchar2) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_uchar3) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_uchar4) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_uchar8) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_uchar16) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_short) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_short2) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_short3) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_short4) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_short8) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_short16) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_ushort) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_ushort2) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_ushort3) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_ushort4) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_ushort8) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_ushort16) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_int) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_int2) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_int3) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_int4) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_int8) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_int16) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_uint) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_uint2) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_uint3) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_uint4) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_uint8) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_uint16) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_long) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_long2) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_long3) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_long4) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_long8) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_long16) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_ulong) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_ulong2) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_ulong3) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_ulong4) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_ulong8) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_ulong16) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_float) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_float2) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_float3) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_float4) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_float8) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_float16) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_double) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_double2) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_double3) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_double4) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_double8) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_double16) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_half) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_half2) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_half3) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_half4) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_half8) { runTest(); }
TEST_F(OfflineExecutionType, Offline_01_type_half16) { runTest(); }
