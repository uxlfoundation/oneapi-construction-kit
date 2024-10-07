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

TEST_P(Execution, Regression_26_Predeclared_Internal_Builtins) {
  if (!UCL::hasImageSupport(this->device)) {
    GTEST_SKIP();
  }

  // This bug caused a compilation failure, so the results are not too
  // important.
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  AddPrimitive(0);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_27_Divergent_Atomics) {
  fail_if_not_vectorized_ = false;
  // The output buffers are default-initialized, so integers they are
  // zeroed-out.
  kts::Reference1D<cl_uint> refOut = [](size_t) {
    return static_cast<cl_uint>(kts::localN * 2);
  };
  AddOutputBuffer(1, refOut);
  RunGeneric1D(kts::localN, kts::localN);
}

TEST_P(Execution, Regression_28_Uniform_Atomics) {
  // The output buffers are default-initialized, so integers they are
  // zeroed-out.
  kts::Reference1D<cl_int> refOut = [](size_t) {
    return static_cast<cl_int>(kts::localN);
  };
  AddOutputBuffer(1, refOut);
  RunGeneric1D(kts::localN, kts::localN);
}

TEST_P(Execution, Regression_29_Divergent_Memfence) {
  // Inputs/outputs are unimportant
  AddPrimitive(1);
  AddOutputBuffer(kts::N, kts::Ref_Identity);
  RunGeneric1D(kts::N);
}

struct ArraySizeAndTypeParam final {
  const unsigned int arraySize;
  const std::string varType;

  ArraySizeAndTypeParam(unsigned int size, std::string type)
      : arraySize(size), varType(type) {}
};

static std::ostream &operator<<(std::ostream &out,
                                const ArraySizeAndTypeParam &param) {
  out << "ArraySizeAndTypeParam{.arraySize{" << param.arraySize
      << ", .varType{\"" << param.varType << "\"}}";
  return out;
}

using LocalAlignmentTests = ExecutionWithParam<ArraySizeAndTypeParam>;

TEST_P(LocalAlignmentTests, Regression_30_Local_Alignment) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection does not support rebuilding a program.
  }
  // Test for __local variable alignment
  const auto Param = getParam();
  const int arraySize = Param.arraySize;
  std::string localType = Param.varType;

  // Don't assume we support doubles, fallback to long since it has the
  // same alignment requirements
  if (0 == localType.compare("double") && !UCL::hasDoubleSupport(device)) {
    localType = "long";
  }

  const unsigned int outputSize = 7u;
  std::map<std::string, std::array<cl_ulong, outputSize>> alignments = {
      {"short", {{1u, 3u, 7u, 15u, 31u}}},
      {"int", {{3u, 7u, 15u, 31u, 63u}}},
      {"long", {{7u, 15u, 31u, 63u, 127u}}},
      {"double", {{7u, 15u, 31u, 63u, 127u}}}};

  const auto &typeAlignment = alignments[localType];

  kts::Reference1D<cl_ulong> refIn = [&typeAlignment](size_t x) {
    switch (x) {
      default:
        break;
      case 0:  // __local TYPE
      case 1:  // __local TYPE[]
        return typeAlignment[0];
      case 2:  // __local TYPE2[]
        return typeAlignment[1];
      case 3:  // __local TYPE3[]
      case 4:  // __local TYPE4[]
        return typeAlignment[2];
      case 5:  // __local TYPE8[]
        return typeAlignment[3];
      case 6:  // __local TYPE16[]
        return typeAlignment[4];
    }
    // shouldn't get here, test will fail
    return std::numeric_limits<cl_ulong>::max();
  };

  // We bitwise and the __local mem address with the input mask,
  // which is expected to be zero.
  kts::Reference1D<cl_ulong> refOutZero = [](size_t) { return 0; };

  AddInputBuffer(outputSize, refIn);
  AddOutputBuffer(outputSize, refOutZero);

  AddMacro("SIZE", arraySize);
  AddMacro("TYPE", localType);
  AddMacro("TYPE2", localType + '2');
  AddMacro("TYPE3", localType + '3');
  AddMacro("TYPE4", localType + '4');
  AddMacro("TYPE8", localType + '8');
  AddMacro("TYPE16", localType + "16");
  RunGeneric1D(kts::N);
}

UCL_EXECUTION_TEST_SUITE_P(
    LocalAlignmentTests, testing::Values(OPENCL_C),
    testing::Values(
        ArraySizeAndTypeParam(2, "short"), ArraySizeAndTypeParam(2, "int"),
        ArraySizeAndTypeParam(2, "double"), ArraySizeAndTypeParam(3, "short"),
        ArraySizeAndTypeParam(3, "int"), ArraySizeAndTypeParam(3, "double"),
        ArraySizeAndTypeParam(4, "short"), ArraySizeAndTypeParam(4, "int"),
        ArraySizeAndTypeParam(4, "double"), ArraySizeAndTypeParam(5, "short"),
        ArraySizeAndTypeParam(5, "int"), ArraySizeAndTypeParam(5, "double"),
        ArraySizeAndTypeParam(6, "short"), ArraySizeAndTypeParam(6, "int"),
        ArraySizeAndTypeParam(6, "double"), ArraySizeAndTypeParam(7, "short"),
        ArraySizeAndTypeParam(7, "int"), ArraySizeAndTypeParam(7, "double"),
        ArraySizeAndTypeParam(8, "short"), ArraySizeAndTypeParam(8, "int"),
        ArraySizeAndTypeParam(8, "double"), ArraySizeAndTypeParam(9, "short"),
        ArraySizeAndTypeParam(9, "int"), ArraySizeAndTypeParam(9, "double"),
        ArraySizeAndTypeParam(10, "short"), ArraySizeAndTypeParam(10, "int"),
        ArraySizeAndTypeParam(10, "double")))

using AlignmentParam = unsigned int;
using LocalStructAlignmentTests = ExecutionWithParam<AlignmentParam>;

TEST_P(LocalStructAlignmentTests, Regression_31_Local_Struct_Alignment) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection does not support rebuilding a program.
  }
  // Test for __local variable alignment
  const unsigned int alignment = getParam();
  const unsigned int outputSize = 3;

  // Kernel contains double type, fallback to long if not supported
  if (!UCL::hasDoubleSupport(device)) {
    AddMacro("NO_DOUBLES", 1);
  }

  kts::Reference1D<cl_ulong> refIn = [&alignment](size_t) {
    return (cl_ulong)(alignment - 1);
  };

  // We bitwise and the __local mem address with the input mask,
  // which is expected to be zero.
  kts::Reference1D<cl_ulong> refOutZero = [](size_t) { return 0; };

  AddInputBuffer(outputSize, refIn);
  AddOutputBuffer(outputSize, refOutZero);

  AddMacro("ALIGN", alignment);
  RunGeneric1D(kts::N);
}

TEST_P(LocalStructAlignmentTests, Regression_31_Local_Struct_Alignment2) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection does not support rebuilding a program.
  }
  // Test for __local variable alignment
  const unsigned int alignment = getParam();
  const unsigned int outputSize = 3;

  kts::Reference1D<cl_ulong> refIn = [&alignment](size_t) {
    return (cl_ulong)(alignment - 1);
  };

  // We bitwise and the __local mem address with the input mask,
  // which is expected to be zero.
  kts::Reference1D<cl_ulong> refOutZero = [](size_t) { return 0; };

  AddInputBuffer(outputSize, refIn);
  AddOutputBuffer(outputSize, refOutZero);

  AddMacro("ALIGN", alignment);
  RunGeneric1D(kts::N);
}

TEST_P(LocalStructAlignmentTests, Regression_31_Local_Struct_Alignment3) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection does not support rebuilding a program.
  }
  // Test for __local variable alignment.
  const unsigned int alignment = getParam();
  const unsigned int outputSize = 3;

  kts::Reference1D<cl_ulong> refIn = [&alignment](size_t) {
    return (cl_ulong)(alignment - 1);
  };

  // We bitwise and the __local mem address with the input mask,
  // which is expected to be zero.
  kts::Reference1D<cl_ulong> refOutZero = [](size_t) { return 0; };

  AddInputBuffer(outputSize, refIn);
  AddOutputBuffer(outputSize, refOutZero);

  AddMacro("ALIGN", alignment);
  RunGeneric1D(kts::N);
}

UCL_EXECUTION_TEST_SUITE_P(
    LocalStructAlignmentTests, testing::Values(OPENCL_C),
    testing::Values(AlignmentParam(8), AlignmentParam(16), AlignmentParam(32),
                    AlignmentParam(64), AlignmentParam(128),
                    AlignmentParam(256)))

using TypeParam = std::string;
using StructMemberAlignmentTests = ExecutionWithParam<TypeParam>;

TEST_P(StructMemberAlignmentTests, Regression_32_Struct_Member_Alignment) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection does not support rebuilding a program.
  }
  // Test for struct member alignment
  std::string localType = getParam();

  cl_uint address_bits = UCL::getDeviceAddressBits(device);
  ASSERT_TRUE((address_bits == 32) || (address_bits == 64));

  // Don't assume we support doubles, fallback to long since it has the
  // same alignment requirements
  const std::string doubleStr("double");
  if (0 == localType.compare(0, doubleStr.length(), doubleStr) &&
      !UCL::hasDoubleSupport(device)) {
    localType.replace(0, doubleStr.size(), "long");
  }

  const unsigned int outputSize = 3;
  std::map<std::string, cl_ulong> alignments = {
      {"char16", 15u}, {"char8", 7u}, {"short3", 7u},   {"short4", 7u},
      {"int3", 11u},   {"int8", 31u}, {"double3", 31u}, {"double8", 63u},
      {"long3", 31u},  {"long8", 63u}};

  cl_ulong &align = alignments[localType];

  kts::Reference1D<cl_ulong> refIn = [&align, &address_bits](size_t x) {
    switch (x) {
      default:
        break;
      case 0:  // alignment of test param
        return align;
      case 1:  // pointer alignment
        return (cl_ulong)((address_bits / 8) - 1);
      case 2:  // alignment of cl_int16
        return (cl_ulong)63u;
    }
    // shouldn't get here, test will fail
    return std::numeric_limits<cl_ulong>::max();
  };

  kts::Reference1D<cl_ulong> refOutZero = [](size_t) { return 0; };

  AddInputBuffer(outputSize, refIn);
  AddOutputBuffer(outputSize, refOutZero);

  AddMacro("TYPE", localType);
  RunGeneric1D(kts::N);
}

UCL_EXECUTION_TEST_SUITE_P(
    StructMemberAlignmentTests, testing::Values(OPENCL_C),
    testing::Values(TypeParam("char16"), TypeParam("char8"),
                    TypeParam("short3"), TypeParam("short4"), TypeParam("int3"),
                    TypeParam("int8"), TypeParam("double3"),
                    TypeParam("double8")))

// The following struct template triggers clang-tidy data layout warnings which
// are being used deliberately to test the interoperation with a kernel so mark
// it NOLINT to disable the check.
template <typename DevicePtrT>
struct UserStruct {  // NOLINT
  cl_char a;
  cl_short3 b;
  DevicePtrT c;
  cl_float d[3];
  cl_long4 e;
};

namespace kts {

template <typename DevicePtrT>
struct Validator<UserStruct<DevicePtrT>> {
  bool validate(UserStruct<DevicePtrT> &expected,
                UserStruct<DevicePtrT> &actual) {
    Validator<cl_char> vChar;
    Validator<cl_short3> vShort;
    Validator<DevicePtrT> vInt;
    Validator<cl_float> vFloat;
    Validator<cl_long4> vLong;

    bool success = vChar.validate(expected.a, actual.a);
    success = success && vShort.validate(expected.b, actual.b);
    success = success && vInt.validate(expected.c, actual.c);
    success = success && vFloat.validate(expected.d[0], actual.d[0]);
    success = success && vFloat.validate(expected.d[1], actual.d[1]);
    success = success && vFloat.validate(expected.d[2], actual.d[2]);
    success = success && vLong.validate(expected.e, actual.e);
    return success;
  }

  void print(std::stringstream &s, const UserStruct<DevicePtrT> &value) {
    Validator<cl_char> vChar;
    Validator<cl_short3> vShort;
    Validator<DevicePtrT> vInt;
    Validator<cl_float> vFloat;
    Validator<cl_long4> vLong;
    s << "{ a = ";
    vChar.print(s, value.a);
    s << ", b = ";
    vShort.print(s, value.b);
    s << ", c = ";
    vInt.print(s, value.c);
    s << ", d = [";
    vFloat.print(s, value.d[0]);
    s << ", ";
    vFloat.print(s, value.d[1]);
    s << ", ";
    vFloat.print(s, value.d[2]);
    s << "], e = ";
    vLong.print(s, value.e);
    s << " };";
  }
};
}  // namespace kts

TEST_P(Execution, Regression_33_Struct_Param_Alignment) {
  const cl_uint address_bits = UCL::getDeviceAddressBits(this->device);
  ASSERT_TRUE((address_bits == 32) || (address_bits == 64));

  kts::Reference1D<UserStruct<cl_uint>> structIn32 = [](size_t) {
    UserStruct<cl_uint> x;
    x.a = 42;
    x.b = {{42, 42, 42}};
    x.c = 0 /* Effectively nullptr. */;
    x.d[0] = 42;
    x.d[1] = 42;
    x.d[2] = 42;
    x.e = {{42, 42, 42, 42}};
    return x;
  };

  kts::Reference1D<UserStruct<cl_ulong>> structIn64 = [](size_t) {
    UserStruct<cl_ulong> x;
    x.a = 42;
    x.b = {{42, 42, 42}};
    x.c = 0 /* Effectively nullptr. */;
    x.d[0] = 42;
    x.d[1] = 42;
    x.d[2] = 42;
    x.e = {{42, 42, 42, 42}};
    return x;
  };

  kts::Reference1D<cl_ulong> refIn = [address_bits](size_t x) {
    switch (x) {
      default:
        break;
      case 0:
        return (cl_ulong)7u;  // cl_short3
      case 1:
        return (cl_ulong)((address_bits / 8) - 1);  // pointer
      case 2:
      case 3:
      case 4:
        return (cl_ulong)3u;  // cl_float
      case 5:
        return (cl_ulong)31u;  // cl_long4
    }
    return std::numeric_limits<cl_ulong>::max();
  };
  kts::Reference1D<cl_ulong> refOutZero = [](size_t) { return 0; };

  const unsigned int bufferSize = 6u;
  if (address_bits == 32) {
    AddInputBuffer(bufferSize, structIn32);
  } else if (address_bits == 64) {
    AddInputBuffer(bufferSize, structIn64);
  }
  AddInputBuffer(bufferSize, refIn);
  AddOutputBuffer(bufferSize, refOutZero);
  RunGeneric1D(kts::N);
}

// This test was added to trigger assertions and crashes in the X86 LLVM
// backend when we try to vectorize by the entire x-dimension (i.e. potentially
// very wide vectors).
TEST_P(Execution, Regression_34_Codegen_1) {
  // This test particularly needs a local workgroup size of 512, so make sure
  // that the global size can accommodate that.
  const int items = std::max<int>(static_cast<int>(kts::N), 1024);
  const cl_int reps = 4;  // How many entries each work item should process.
  const int size = items * reps;

  kts::Reference1D<cl_int> refSize = [&size](size_t) { return size; };

  auto refIn = kts::Ref_Identity;
  kts::Reference1D<cl_int> refOut = [=, &refIn](size_t x) {
    cl_int sum = 0;
    for (size_t i = x * reps; i < (x + 1) * reps; i++) {
      sum += refIn(i);
    }
    return sum * 3;  // Three for three input arrays.
  };

  AddInputBuffer(refSize(0), refIn);
  AddInputBuffer(refSize(1), refIn);
  AddInputBuffer(refSize(2), refIn);
  AddOutputBuffer(items, refOut);
  AddInputBuffer(3, refSize);
  AddPrimitive(reps);
  RunGeneric1D(items, 512);
}

// This test was added to trigger assertions and crashes in the X86 LLVM
// backend when we try to vectorize by the entire x-dimension (i.e. potentially
// very wide vectors).  Note that this test triggered a different crash than
// Regression_34_Codegen_1.
TEST_P(Execution, Regression_34_Codegen_2) {
  // This test particularly needs a local workgroup size of 256, so make sure
  // that the global size can accommodate that.
  const int items = std::max<int>(static_cast<int>(kts::N), 512);
  const cl_int reps = 4;  // How many entries each work item should process.
  const cl_int size = items * reps;

  auto refIn = kts::Ref_Identity;
  kts::Reference1D<cl_int> refOut = [=, &refIn](size_t x) {
    cl_int sum = 0;
    for (size_t i = x * reps; i < (x + 1) * reps; i++) {
      sum += refIn(i);
    }
    return sum;
  };

  AddInputBuffer(size, refIn);
  AddOutputBuffer(items, refOut);
  AddPrimitive(size);
  AddPrimitive(reps);
  RunGeneric1D(items, 256);
}

TEST_P(Execution, Regression_35_Constant_Struct_Alignment) {
  kts::Reference1D<cl_ulong> refIn = [](size_t x) {
    switch (x) {
      default:
        break;
      case 0:
        return (cl_ulong)3u;  // cl_short2
      case 1:
        return (cl_ulong)7u;  // cl_ulong
      case 2:
        return (cl_ulong)15u;  // cl_float4
    }
    return std::numeric_limits<cl_ulong>::max();
  };
  kts::Reference1D<cl_ulong> refOutZero = [](size_t) { return 0; };

  const unsigned int bufferSize = 3u;
  AddInputBuffer(bufferSize, refIn);
  AddOutputBuffer(bufferSize, refOutZero);
  RunGeneric1D(kts::N);
}

// Test sizeof() operator in case of erroneous padding or alignment from the
// compiler.
TEST_P(Execution, Regression_36_Struct_Sizeof) {
  kts::Reference1D<ucl::PackedFloat3> refOut1 = [](size_t x) {
    return ucl::PackedFloat3{(float)x, x + 0.2f, x + 0.5f};
  };

  // This result should be sizeof(testStruct), calculated using working below:
  // typedef struct {
  //   char c;         Start at offset 0, plus sizeof(char), is 1.
  //
  //   float3 f;       Starts at offset 16 because of alignment,
  //                   plus sizeof(float3) = 16, gives 32 bytes
  //
  //   int i;          32 bytes is already 4 byte aligned,
  //                   plus sizeof(cl_int) = 4, 36 bytes
  //
  //   ulong l[2];     Starts at 40 bytes because of 8 byte alignment,
  //                   plus 2 x sizeof(cl_ulong), gives 56 bytes
  //
  //                   struct is then padded to meet largest member alignment,
  //                   16 bytes, for a total 64 byte size.
  // } testStruct;
  kts::Reference1D<cl_uint> refOut2 = [](size_t) { return 64u; };

  AddOutputBuffer(kts::N, refOut1);
  AddOutputBuffer(kts::N, refOut2);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_37_CFC) {
  const cl_int limit = static_cast<cl_int>(kts::N / 2);
  kts::Reference1D<cl_int> refOut = [limit](size_t x) {
    const cl_int ix = kts::Ref_Identity(x);
    return ix < limit ? ix : kts::Ref_A(ix % 32);
  };
  AddMacro("CHUNK_SIZE", 32);
  AddInputBuffer(limit, kts::Ref_A);
  AddOutputBuffer(kts::N, refOut);
  AddPrimitive(limit);
  RunGeneric1D(kts::N);
}

using StructAttributeAlignmentTests = ExecutionWithParam<AlignmentParam>;
TEST_P(StructAttributeAlignmentTests, Regression_38_Attribute_Align) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection does not support rebuilding a program.
  }
  const unsigned memberAlign = getParam();
  const unsigned structAlign = memberAlign * 2;
  const unsigned int outputSize = 6;  // check 2 alignments for the 3 structs

  kts::Reference1D<cl_ulong> refIn = [&memberAlign, &structAlign](size_t x) {
    switch (x) {
      default:
        break;
      case 0:  // __private
      case 1:  // __local
      case 2:  // __constant
        return (cl_ulong)(memberAlign - 1);
      case 3:  // __private
      case 4:  // __local
      case 5:  // __constant
        return (cl_ulong)(structAlign - 1);
    }
    // shouldn't get here, test will fail
    return std::numeric_limits<cl_ulong>::max();
  };

  kts::Reference1D<cl_ulong> refOutZero = [](size_t) { return 0; };

  AddInputBuffer(outputSize, refIn);
  AddOutputBuffer(outputSize, refOutZero);

  AddMacro("ALIGN1", memberAlign);
  AddMacro("ALIGN2", structAlign);
  RunGeneric1D(kts::N);
}

UCL_EXECUTION_TEST_SUITE_P(
    StructAttributeAlignmentTests, testing::Values(OPENCL_C),
    testing::Values(AlignmentParam(8), AlignmentParam(16), AlignmentParam(32),
                    AlignmentParam(64), AlignmentParam(128),
                    AlignmentParam(256)))

TEST_P(Execution, Regression_39_Struct_Helper_Func) {
  const unsigned int outputSize = 2;  // Two helper functions
  kts::Reference1D<cl_ulong> refIn = [](size_t) {
    return (cl_ulong)7;  // short3
  };

  kts::Reference1D<cl_ulong> refOutZero = [](size_t) { return 0; };

  AddInputBuffer(outputSize, refIn);
  AddOutputBuffer(outputSize, refOutZero);
  RunGeneric1D(kts::N);
}

// TODO CA-1939: Add support for double tests to `clc`
TEST_P(ExecutionOpenCLC, Regression_40_Fract_Double3) {
  if (!UCL::hasDoubleSupport(this->device)) {
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
    return Expected1[x % num_expected];
  };
  kts::Reference1D<cl_double> refOut2 = [=, &Expected2](size_t x) {
    return Expected2[x % num_expected];
  };

  AddInputBuffer(kts::N * 3, refIn);
  AddOutputBuffer(kts::N * 3, refOut1);
  AddOutputBuffer(kts::N * 3, refOut2);
  RunGeneric1D(kts::N);
}

// Tests ARM backend vector shuffle support which fails for this case in LLVM
// versions at least less than 4.0.
TEST_P(Execution, Regression_41_Shuffle_Copy) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  kts::Reference1D<cl_float16> refIn = [](size_t x) {
    // Issue not present when accessing first buffer index, so put test
    // data in second element.
    if (x == 1) {
      return cl_float16{{0.0f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f,
                         -0.0f, -1.0f, -2.0f, -4.0f, -8.0f, -16.0f, -32.0f,
                         -64.0f}};
    }
    return cl_float16{{0.0f}};
  };

  // tmp.S8e42D0Ab = source[gid].s858B6A89;
  // dst index -> src index:
  // A -> 0
  // B -> 2
  // 8 -> 4
  // 8 -> 8
  // 8 -> A
  // 9 -> B
  // 6 -> D
  // 5 -> E
  kts::Reference1D<cl_float16> refOut = [](size_t x) {
    if (x == 1) {
      return cl_float16{{-2.0f, 0.0f, -4.0f, 0.0f, -0.0f, 0.0f, 0.0f, 0.0f,
                         -0.0f, 0.0f, -0.0f, -1.0f, 0.0f, 32.0f, 16.0f, 0.0f}};
    }
    return cl_float16{{0.0f}};
  };

  AddInputBuffer(2, refIn);
  AddOutputBuffer(2, refOut);
  RunGeneric1D(1);  // Run one thread
}

// Tests ARM backend vector shuffle support
TEST_P(Execution, Regression_42_Shuffle_Function_Call) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  kts::Reference1D<cl_char8> refIn = [](size_t) {
    return cl_char8{{10, 11, 12, 13, 14, 15, 16, 17}};
  };

  // refOut.S5B = refIn.s37;
  // dst index -> src index:
  // 3 -> 5
  // 7 -> B
  kts::Reference1D<cl_char16> refOut = [](size_t) {
    return cl_char16{{0, 0, 0, 0, 0, 13, 0, 0, 0, 0, 0, 17, 0, 0, 0, 0}};
  };

  AddInputBuffer(1, refIn);
  AddOutputBuffer(1, refOut);
  RunGeneric1D(1);  // Run one thread
}

TEST_P(Execution, Regression_43_Scatter_Gather) {
  // This test has a kernel that does not handle arbitrary values of N. If its
  // value is changed, this test will need to be updated manually.
  ASSERT_EQ(kts::N, 256);
  kts::Reference1D<cl_int> refOut = [](size_t x) { return (cl_int)(x * 7); };
  kts::Reference1D<cl_int> refIn = [](size_t x) {
    return (cl_int)((x + 1) * 7);
  };
  AddPrimitive(64);
  AddInputBuffer(kts::N, refIn);
  AddOutputBuffer(kts::N + 1, refOut);
  RunGeneric1D(kts::N);
}

// With LLVM 3.9 conversion to short3 vectors from char3 vectors causing
// selection DAG errors, so test all those conversion functions (signed,
// unsigned, saturated, unsatured, all rounding modes).
using Short3CodegenTests = ExecutionWithParam<const char *>;
using Ushort3CodegenTests = ExecutionWithParam<const char *>;

TEST_P(Short3CodegenTests, Regression_44_Short3_Char3_Codegen) {
  if (!isSourceTypeIn({OPENCL_C})) {  // REQUIRES: parameters
    GTEST_SKIP();
  }
  kts::Reference1D<cl_char> refIn = [](size_t x) {
    return static_cast<cl_char>(std::min<size_t>(127, x));
  };
  kts::Reference1D<cl_short> refOut = [&refIn](size_t x) {
    return static_cast<cl_short>(refIn(x));
  };
  AddMacro("CONVERT_FUNCTION", getParam());
  AddInputBuffer(kts::N * 3, refIn);
  AddOutputBuffer(kts::N * 3, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Short3CodegenTests, Regression_44_Short3_Uchar3_Codegen) {
  if (!isSourceTypeIn({OPENCL_C})) {  // REQUIRES: parameters
    GTEST_SKIP();
  }
  kts::Reference1D<cl_uchar> refIn = [](size_t x) {
    return static_cast<cl_uchar>(x);
  };
  kts::Reference1D<cl_short> refOut = [&refIn](size_t x) {
    return static_cast<cl_short>(refIn(x));
  };
  AddMacro("CONVERT_FUNCTION", getParam());
  AddInputBuffer(kts::N * 3, refIn);
  AddOutputBuffer(kts::N * 3, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Ushort3CodegenTests, Regression_44_Ushort3_Char3_Codegen) {
  if (!isSourceTypeIn({OPENCL_C})) {  // REQUIRES: parameters
    GTEST_SKIP();
  }
  kts::Reference1D<cl_char> refIn = [](size_t x) {
    return static_cast<cl_char>(std::min<size_t>(127, x));
  };
  kts::Reference1D<cl_ushort> refOut = [&refIn](size_t x) {
    return static_cast<cl_ushort>(refIn(x));
  };
  AddMacro("CONVERT_FUNCTION", getParam());
  AddInputBuffer(kts::N * 3, refIn);
  AddOutputBuffer(kts::N * 3, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Ushort3CodegenTests, Regression_44_Ushort3_Uchar3_Codegen) {
  if (!isSourceTypeIn({OPENCL_C})) {  // REQUIRES: parameters
    GTEST_SKIP();
  }
  kts::Reference1D<cl_uchar> refIn = [](size_t x) {
    return static_cast<cl_uchar>(x);
  };
  kts::Reference1D<cl_ushort> refOut = [&refIn](size_t x) {
    return static_cast<cl_ushort>(refIn(x));
  };
  AddMacro("CONVERT_FUNCTION", getParam());
  AddInputBuffer(kts::N * 3, refIn);
  AddOutputBuffer(kts::N * 3, refOut);
  RunGeneric1D(kts::N);
}

UCL_EXECUTION_TEST_SUITE_P(
    Short3CodegenTests, testing::Values(OPENCL_C),
    testing::Values("convert_short3", "convert_short3_rte",
                    "convert_short3_rtz", "convert_short3_rtn",
                    "convert_short3_rtp", "convert_short3_sat",
                    "convert_short3_sat_rte", "convert_short3_sat_rtz",
                    "convert_short3_sat_rtn", "convert_short3_sat_rtp"))
UCL_EXECUTION_TEST_SUITE_P(
    Ushort3CodegenTests, testing::Values(OPENCL_C),
    testing::Values("convert_ushort3", "convert_ushort3_rte",
                    "convert_ushort3_rtz", "convert_ushort3_rtn",
                    "convert_ushort3_rtp", "convert_ushort3_sat",
                    "convert_ushort3_sat_rte", "convert_ushort3_sat_rtz",
                    "convert_ushort3_sat_rtn", "convert_ushort3_sat_rtp"))

// With LLVM 3.9 sign extending short3 vectors to int3 vectors is causing
// selection DAG errors, this can't be done directly in OpenCL C but the
// mad_sat short3 functions are doing this, so test those functions.
TEST_P(Execution, Regression_45_Mad_Sat_Short3_Codegen) {
  kts::Reference1D<cl_short> refIn = [](size_t x) {
    return static_cast<cl_short>((x % 65535) - 32768);
  };
  kts::Reference1D<cl_short> refOut = [&refIn](size_t x) {
    const cl_long y = static_cast<cl_long>(refIn(x));
    const cl_long mad = (y * y) + y;  // mad_sat == a*b+c
    const cl_long mad_sat =
        std::min<cl_long>(32767, std::max<cl_long>(-32768, mad));
    return static_cast<cl_short>(mad_sat);
  };
  AddInputBuffer(kts::N * 3, refIn);
  AddInputBuffer(kts::N * 3, refIn);
  AddInputBuffer(kts::N * 3, refIn);
  AddOutputBuffer(kts::N * 3, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Regression_45_Mad_Sat_Ushort3_Codegen) {
  kts::Reference1D<cl_ushort> refIn = [](size_t x) {
    return static_cast<cl_ushort>(x % 65535);
  };
  kts::Reference1D<cl_ushort> refOut = [&refIn](size_t x) {
    const cl_ulong y = static_cast<cl_ulong>(refIn(x));
    const cl_ulong mad = (y * y) + y;  // mad_sat == a*b+c
    const cl_ulong mad_sat = std::min<cl_ulong>(65535, mad);
    return static_cast<cl_ushort>(mad_sat);
  };
  AddInputBuffer(kts::N * 3, refIn);
  AddInputBuffer(kts::N * 3, refIn);
  AddInputBuffer(kts::N * 3, refIn);
  AddOutputBuffer(kts::N * 3, refOut);
  RunGeneric1D(kts::N);
}

// This test checks the alignment of a char2 vector is always a multiple of 2,
// which is not very interesting, but the real point is that taking the address
// of a vector in local memory caused a compile time crash when the vectorizer
// is enabled.  This test attempts to trigger that compile time crash (i.e. the
// code is interesting, the result that it produces less so).
TEST_P(Execution, Regression_46_Local_Vecalign) {
  kts::Reference1D<cl_ulong> refOut = [](size_t) {
    return static_cast<cl_ulong>(0u);
  };
  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

// Tests struct alignment pass on 32-bit systems. Reduced from Eigen code which
// produces packed structs with padded struct members.
TEST_P(Execution, Regression_47_Packed_Struct) {
  kts::Reference1D<cl_ulong> refOut = [](size_t) {
    return static_cast<cl_ulong>(2u);
  };

  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

// Just duplicate these variables and Lambdas into a namespace because these are
// used in four separate tests.
namespace Regression_48 {
constexpr size_t global_size = 4;
constexpr size_t width = global_size;
constexpr size_t height = 1;
constexpr size_t depth = 1;
const cl_image_desc desc = []() {
  cl_image_desc image_desc;
  image_desc.image_type = CL_MEM_OBJECT_IMAGE1D;
  image_desc.image_width = width;
  image_desc.image_height = height;
  image_desc.image_depth = depth;
  image_desc.image_array_size = 0;
  image_desc.image_row_pitch = 0;
  image_desc.image_slice_pitch = 0;
  image_desc.num_mip_levels = 0;
  image_desc.num_samples = 0;
  image_desc.buffer = nullptr;
  return image_desc;
}();
static cl_image_format format = {CL_RGBA, CL_UNSIGNED_INT8};
static cl_bool normalized_coords = CL_TRUE;
static cl_addressing_mode addressing_mode_repeat = CL_ADDRESS_REPEAT;
static cl_addressing_mode addressing_mode_clamp = CL_ADDRESS_CLAMP_TO_EDGE;
static cl_filter_mode filter_mode = CL_FILTER_NEAREST;

static kts::Reference1D<cl_char4> refIn = [](size_t x) {
  cl_char c = static_cast<cl_char>(x);
  cl_char4 a4 = {{c, c, c, c}};
  return a4;
};

static kts::Reference1D<cl_uint> refOut = [](size_t x) {
  const size_t index = x >> 1;

  float normf = (static_cast<float>(index) + 0.05f) /
                (static_cast<float>(global_size) / 2);
  // even indices are repeat, odd are clamp
  if (x & 1) {
    normf = std::min(normf, 0.999f);
    normf = std::max(normf, 0.0f);
  } else {
    float integral = 0.0;
    normf = std::modf(normf, &integral);
  }

  // Calculate unnormalised value
  float coordf = normf * static_cast<float>(width);
  // add 0.5 to get center of pixel
  coordf += 0.5;

  const cl_uint result = static_cast<cl_uint>(std::round(coordf) - 1);
  return result;
};
}  // namespace Regression_48

// TODO CA-1929: Fix OfflineExecution
TEST_P(ExecutionOnline, Regression_48_Image_Sampler) {
  if (!UCL::hasImageSupport(this->device)) {
    GTEST_SKIP();
  }

  using namespace ::Regression_48;

  AddOutputBuffer(global_size * 2, refOut);
  AddInputImage(format, desc, global_size, refIn);
  AddSampler(normalized_coords, addressing_mode_repeat, filter_mode);
  AddSampler(normalized_coords, addressing_mode_clamp, filter_mode);

  RunGeneric1D(global_size);
}

// TODO CA-1929: Fix OfflineExecution
TEST_P(Execution, Regression_48_Image_Sampler_Kernel_Call_Kernel) {
  if (!isSourceTypeIn({OPENCL_C, SPIRV, OFFLINESPIRV}) ||
      !UCL::hasImageSupport(this->device)) {
    GTEST_SKIP();
  }

  using namespace ::Regression_48;

  AddOutputBuffer(global_size * 2, refOut);
  AddInputImage(format, desc, global_size, refIn);
  AddSampler(normalized_coords, addressing_mode_repeat, filter_mode);
  AddSampler(normalized_coords, addressing_mode_clamp, filter_mode);

  RunGeneric1D(global_size);
}

// TODO CA-1930: Generate Spirv/Offline
TEST_P(Execution, Regression_49_Local_Select) {
  if (!isSourceTypeIn({OPENCL_C})) {
    GTEST_SKIP();
  }
  AddMacro("SIZE", (unsigned int)kts::localN);
  kts::Reference1D<cl_bool> refOut = [](size_t) { return true; };

  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N, kts::localN);
}

TEST_P(Execution, Regression_50_Local_atomic) {
  kts::Reference1D<cl_uint> refOut = [](size_t) {
    return static_cast<cl_uint>(kts::localN);
  };

  AddOutputBuffer(kts::N / kts::localN, refOut);
  RunGeneric1D(kts::N, kts::localN);
}

// Do not add additional tests here or this file may become too large to link.
// Instead, extend the newest ktst_regression_${NN}.cpp file.
