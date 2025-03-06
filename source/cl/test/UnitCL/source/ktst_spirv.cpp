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

#include "Common.h"
#include "kts/execution.h"
#include "kts/reference_functions.h"

using namespace kts::ucl;

// Used for tests which can only be tested with SPIR-V input.
using SpirvExecution = Execution;
UCL_EXECUTION_TEST_SUITE(SpirvExecution, testing::Values(SPIRV, OFFLINESPIRV))

TEST_P(Execution, Spirv_01_Copy) {
  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Spirv_02_Async_Copy) {
  AddInputBuffer(kts::N, kts::Ref_A);
  AddLocalBuffer<cl_uint>(kts::localN);
  AddOutputBuffer(kts::N, kts::Ref_A);

  RunGeneric1D(kts::N, kts::localN);
}

TEST_P(Execution, Spirv_03_Test_Atomic_Add) {
  AddInOutBuffer(kts::N, kts::Ref_Identity, kts::Ref_PlusOne);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Spirv_04_Test_Atomic_Sub) {
  AddInOutBuffer(kts::N, kts::Ref_PlusOne, kts::Ref_Identity);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Spirv_05_Test_Atomic_Min) {
  AddInputBuffer(kts::N, kts::Ref_Identity);
  AddInOutBuffer(kts::N, kts::Ref_PlusOne, kts::Ref_Identity);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Spirv_06_Test_Atomic_Max) {
  AddInputBuffer(kts::N, kts::Ref_Identity);
  AddInOutBuffer(kts::N, kts::Ref_PlusOne, kts::Ref_PlusOne);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Spirv_07_Test_Atomic_And) {
  kts::Reference1D<cl_int> refZero = [](size_t) { return 0; };
  AddInOutBuffer(kts::N, kts::Ref_Identity, refZero);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Spirv_08_Test_Atomic_Or) {
  AddInOutBuffer(kts::N, kts::Ref_Identity, kts::Ref_Identity);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Spirv_09_Test_Atomic_Xor) {
  AddInOutBuffer(kts::N, kts::Ref_Identity, kts::Ref_Identity);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Spirv_10_Test_Atomic_Exchange) {
  AddInOutBuffer(kts::N, kts::Ref_Identity, kts::Ref_PlusOne);
  RunGeneric1D(kts::N);
}

TEST_P(Execution, Spirv_11_Test_Atomic_Compare_Exchange) {
  kts::Reference1D<cl_int> refCompare = [](size_t x) {
    return x == 0 ? 42 : x;
  };
  AddInOutBuffer(kts::N, kts::Ref_Identity, refCompare);
  RunGeneric1D(kts::N);
}

struct Simple {
  cl_int a;
  cl_char b;
};

// SPIR-V CTS tests copied into UnitCL to test regression on CA-1526
static std::ostream &operator<<(std::ostream &stream, const Simple &data) {
  stream << "{" << data.a << ", " << data.b << "}";
  return stream;
}

static bool operator==(const Simple &lhs, const Simple &rhs) {
  return lhs.a == rhs.a && lhs.b == rhs.b;
}

TEST_P(SpirvExecution, Spirv_12_Const_Struct_Int_Char) {
  kts::Reference1D<Simple> refOut = [](size_t) -> Simple {
    return Simple{cl_int(2100483600), cl_char(128)};
  };

  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

TEST_P(SpirvExecution, Spirv_12_Copy_Struct_Int_Char) {
  kts::Reference1D<Simple> refOut = [](size_t) -> Simple {
    return Simple{cl_int(2100483600), cl_char(128)};
  };

  AddOutputBuffer(kts::N, refOut);
  RunGeneric1D(kts::N);
}

// TODO: Disabled due to CA-2844
TEST_P(SpirvExecution, DISABLED_Spirv_13_Write_Image_Array) {
  // This test checks that a specific call to write_image is mangled correctly,
  // it's a tricky one because there are no signed integers in OpenCL SPIR-V
  // so just using the types given by SPIR-V the signature looks like:
  //
  // write_imageui(image2d_array_t, uint_4, uint_4)
  //
  // which isn't a valid write_image signature, and itanium mangling will
  // attempt to susbstitute the second uint4. There is no need to validate
  // output here, if the kernel runs without a segfault then the mangling is
  // correct.
  if (!UCL::hasImageSupport(device)) {
    GTEST_SKIP();
  }

  const uint32_t size = 1;

  cl_image_format format = {};
  format.image_channel_order = CL_RGBA;
  format.image_channel_data_type = CL_FLOAT;

  cl_image_desc desc = {};
  desc.image_type = CL_MEM_OBJECT_IMAGE2D_ARRAY;
  desc.image_width = size;
  desc.image_height = size;
  desc.image_array_size = size;

  kts::Reference1D<cl_int4> refIn = [](size_t s) -> cl_int4 {
    cl_int i = static_cast<cl_int>(s);
    return cl_int4({{i, i, i, i}});
  };

  AddInputImage(format, desc, size, refIn);

  RunGeneric1D(size);
}

// Tests the OpImageQuerySizeLod instruction, which is effectively just calls a
// bunch of the get_image_* functions.
// TODO: Disabled due to CA-2844
TEST_P(SpirvExecution, DISABLED_Spirv_14_Query_Image_Size) {
  if (!UCL::hasImageSupport(device)) {
    GTEST_SKIP();
  }

  static constexpr size_t width = 1;
  static constexpr size_t height = 2;
  static constexpr size_t depth = 3;
  static constexpr size_t size = 1;

  cl_image_format format = {};
  format.image_channel_order = CL_RGBA;
  format.image_channel_data_type = CL_FLOAT;

  cl_image_desc array_2d_desc = {};
  array_2d_desc.image_type = CL_MEM_OBJECT_IMAGE2D_ARRAY;
  array_2d_desc.image_width = width;
  array_2d_desc.image_height = height;
  array_2d_desc.image_array_size = depth;

  cl_image_desc image_3d_desc = {};
  image_3d_desc.image_type = CL_MEM_OBJECT_IMAGE3D;
  image_3d_desc.image_width = width;
  image_3d_desc.image_height = height;
  image_3d_desc.image_depth = depth;

  kts::Reference1D<cl_int4> refImageIn = [](size_t) -> cl_int4 {
    return cl_int4({{0, 0, 0, 0}});
  };

  kts::Reference1D<cl_int3> refBufferIn = [](size_t) -> cl_int3 {
    return cl_int3({{0, 0, 0}});
  };

  kts::Reference1D<cl_int3> refBufferOut = [](size_t) -> cl_int3 {
    return cl_int3({{static_cast<cl_int>(width), static_cast<cl_int>(height),
                     static_cast<cl_int>(depth)}});
  };

  AddInputImage(format, array_2d_desc, width * height * depth, refImageIn);
  AddInputImage(format, image_3d_desc, width * height * depth, refImageIn);
  AddInOutBuffer(size * 2, refBufferIn, refBufferOut);

  RunGeneric1D(size);
}

TEST_P(SpirvExecution, Spirv_15_Work_Dim) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  // Running this work item function from a SPIR-V module must be tested with
  // the CL runtime as it doesn't have a glsl equivalent.
  static constexpr size_t dimensions = 2;
  static constexpr size_t size = 1;
  static constexpr size_t global_dim[] = {size, size};
  static constexpr size_t local_dim[] = {size, size};

  kts::Reference1D<cl_uint> refBufferIn = [](size_t) -> cl_uint { return 0; };

  kts::Reference1D<cl_uint> refBufferOut = [](size_t) -> cl_uint {
    return dimensions;
  };

  AddInOutBuffer(size, refBufferIn, refBufferOut);

  RunGenericND(dimensions, global_dim, local_dim);
}

TEST_P(Execution, Spirv_16_Frexp_Smoke) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  // This is a regression/smoke test to ensure we generate the correct mangling
  // for this builtin, which isn't as easy as it sounds because the builtin only
  // takes signed int for a parameter while OpenCL SPIR-V can only encode
  // unsigned int.
  const size_t size = 1;

  // In this kernel we returnt the int* result, which is the exp component of
  // the input float. Since the input is 42.42 our exponent is 6.
  kts::Reference1D<cl_int> refBufferOut = [](size_t) -> cl_uint { return 6; };

  AddOutputBuffer(size, refBufferOut);

  RunGeneric1D(size);
}

TEST_P(Execution, Spirv_17_Ldexp_Smoke) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  // This is a regression/smoke test to ensure we generate the correct mangling
  // for this builtin, which isn't as easy as it sounds because the builtin only
  // takes signed int for a parameter while OpenCL SPIR-V can only encode
  // unsigned int.
  const size_t size = 1;

  // The ldexp function constructs a float from the given exponent and
  // significand, in this case our inputs are 1.0 and 5, so our result should be
  // 32.
  kts::Reference1D<cl_float> refBufferOut = [](size_t) -> cl_float {
    return 32.0;
  };

  AddOutputBuffer(size, refBufferOut);

  RunGeneric1D(size);
}

TEST_P(Execution, Spirv_18_Lgammar_Smoke) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  // This is a regression/smoke test to ensure we generate the correct mangling
  // for this builtin, which isn't as easy as it sounds because the builtin only
  // takes signed int for a parameter while OpenCL SPIR-V can only encode
  // unsigned int.
  const size_t size = 1;

  // Here we return the int*, which for lgamma_r is the sign of the result,
  // which should be positive for an input of 42.42.
  kts::Reference1D<cl_int> refBufferOut = [](size_t) -> cl_int { return 1; };

  AddOutputBuffer(size, refBufferOut);

  RunGeneric1D(size);
}

TEST_P(Execution, Spirv_19_Pown_Smoke) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  // This is a regression/smoke test to ensure we generate the correct mangling
  // for this builtin, which isn't as easy as it sounds because the builtin only
  // takes signed int for a parameter while OpenCL SPIR-V can only encode
  // unsigned int.
  const size_t size = 1;

  // This raises the first input to the power of the second, our inputs are 0.5
  // and 2 so our result is 0.25.
  kts::Reference1D<cl_float> refBufferOut = [](size_t) -> cl_float {
    return 0.25;
  };

  AddOutputBuffer(size, refBufferOut);

  RunGeneric1D(size);
}

TEST_P(Execution, Spirv_20_Remquo_Smoke) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  // This is a regression/smoke test to ensure we generate the correct mangling
  // for this builtin, which isn't as easy as it sounds because the builtin only
  // takes signed int for a parameter while OpenCL SPIR-V can only encode
  // unsigned int.
  const size_t size = 1;

  // This function returns the remainder from the division between the two
  // inputs as well as returning the actual quotient (rounded result) in an int*
  // in this case our inputs are 42.24 and 2.0 so the int* should contain 21.
  kts::Reference1D<cl_int> refBufferOut = [](size_t) -> cl_int { return 21; };

  AddOutputBuffer(size, refBufferOut);

  RunGeneric1D(size);
}

TEST_P(Execution, Spirv_21_Rootn_Smoke) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  // This is a regression/smoke test to ensure we generate the correct mangling
  // for this builtin, which isn't as easy as it sounds because the builtin only
  // takes signed int for a parameter while OpenCL SPIR-V can only encode
  // unsigned int.
  const size_t size = 1;

  // This function returns x^1/y, our x is 42.42 and our y is 1 so we expect a
  // result of 42.42.
  kts::Reference1D<cl_float> refBufferOut = [](size_t) -> cl_float {
    return 42.42f;
  };

  AddOutputBuffer(size, refBufferOut);

  RunGeneric1D(size);
}

TEST_P(SpirvExecution, Spirv_22_Nameless_Dma) {
  // This test is designed to make sure we can handle function
  // parameters that don't have names. Hard-coding this in a SPIR-V kernel is
  // the only way to reliably generate nameless function parameters.

  kts::Reference1D<cl_uint> refBufferAllOnes = [](size_t) -> cl_uint {
    return 1;
  };

  AddInputBuffer(kts::N, kts::Ref_Identity);
  AddInputBuffer(kts::N, refBufferAllOnes);
  AddOutputBuffer(kts::N, kts::Ref_PlusOne);

  RunGeneric1D(kts::N);
}

TEST_P(SpirvExecution, Spirv_23_Memset_Kernel) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  // Runs a SPIR-V module which ought to be translated to include a memset
  // intrinsic. This test doesn't check that it is translated in that way,
  // that's what the lit test is for, it just makes sure an llvm module with
  // that intrinsic can run on our target device.
  kts::Reference1D<cl_int> refZero = [](size_t) { return 0; };
  AddInOutBuffer(64, kts::Ref_Identity, refZero);
  RunGeneric1D(1);
}

TEST_P(SpirvExecution, Spirv_24_Max_Work_Dim) {
  // TODO(CA-3968): Revert when fixed.
#if UNITCL_CROSSCOMPILING
  GTEST_SKIP();
#endif
  AddInputBuffer(kts::N, kts::Ref_A);
  AddInputBuffer(kts::N, kts::Ref_B);
  AddOutputBuffer(kts::N, kts::Ref_Add);
  RunGeneric1D(kts::N);
}
