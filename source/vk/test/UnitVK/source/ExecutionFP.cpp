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

#include "GLSLTestDefs.h"

// None of the tests in this file check the precision of the operations, rather
// they check that the function acts as expected for a limited number of
// argument combinations. Some tests do also verify results when the function
// is passed edge case values such as infinity and NaNs.

#ifndef IGNORE_SPIRV_TESTS

// Note: All pointer arguments point to within the results buffer.
// The result type FrexpStructfloatTy allows access to all pointed-to arguments.
class op_glsl_Frexp_float_intPtr
    : public GlslBuiltinTest<glsl::FrexpStructfloatTy, glsl::floatTy> {
 public:
  op_glsl_Frexp_float_intPtr()
      : GlslBuiltinTest<glsl::FrexpStructfloatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Frexp_float_intPtr) {}
};

TEST_F(op_glsl_Frexp_float_intPtr, Smoke) { RunWithArgs(2); }

// Note: All pointer arguments point to within the results buffer.
// The result type FrexpStructvec2Ty allows access to all pointed-to arguments.
class op_glsl_Frexp_vec2_ivec2Ptr
    : public GlslBuiltinTest<glsl::FrexpStructvec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Frexp_vec2_ivec2Ptr()
      : GlslBuiltinTest<glsl::FrexpStructvec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Frexp_vec2_ivec2Ptr) {}
};

TEST_F(op_glsl_Frexp_vec2_ivec2Ptr, Smoke) { RunWithArgs({2, 2}); }

// Note: All pointer arguments point to within the results buffer.
// The result type FrexpStructvec3Ty allows access to all pointed-to arguments.
class op_glsl_Frexp_vec3_ivec3Ptr
    : public GlslBuiltinTest<glsl::FrexpStructvec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Frexp_vec3_ivec3Ptr()
      : GlslBuiltinTest<glsl::FrexpStructvec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Frexp_vec3_ivec3Ptr) {}
};

TEST_F(op_glsl_Frexp_vec3_ivec3Ptr, Smoke) { RunWithArgs({2, 2, 2}); }

// Note: All pointer arguments point to within the results buffer.
// The result type FrexpStructvec4Ty allows access to all pointed-to arguments.
class op_glsl_Frexp_vec4_ivec4Ptr
    : public GlslBuiltinTest<glsl::FrexpStructvec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Frexp_vec4_ivec4Ptr()
      : GlslBuiltinTest<glsl::FrexpStructvec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Frexp_vec4_ivec4Ptr) {}
};

TEST_F(op_glsl_Frexp_vec4_ivec4Ptr, Smoke) { RunWithArgs({2, 2, 2, 2}); }

// Note: All pointer arguments point to within the results buffer.
// The result type FrexpStructdoubleTy allows access to all pointed-to
// arguments.
class op_glsl_Frexp_double_intPtr
    : public GlslBuiltinTest<glsl::FrexpStructdoubleTy, glsl::doubleTy> {
 public:
  op_glsl_Frexp_double_intPtr()
      : GlslBuiltinTest<glsl::FrexpStructdoubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_Frexp_double_intPtr) {}
};

TEST_F(op_glsl_Frexp_double_intPtr, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2);
  }
}

// Note: All pointer arguments point to within the results buffer.
// The result type FrexpStructdvec2Ty allows access to all pointed-to arguments.
class op_glsl_Frexp_dvec2_ivec2Ptr
    : public GlslBuiltinTest<glsl::FrexpStructdvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_Frexp_dvec2_ivec2Ptr()
      : GlslBuiltinTest<glsl::FrexpStructdvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_Frexp_dvec2_ivec2Ptr) {}
};

TEST_F(op_glsl_Frexp_dvec2_ivec2Ptr, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2});
  }
}

// Note: All pointer arguments point to within the results buffer.
// The result type FrexpStructdvec3Ty allows access to all pointed-to arguments.
class op_glsl_Frexp_dvec3_ivec3Ptr
    : public GlslBuiltinTest<glsl::FrexpStructdvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_Frexp_dvec3_ivec3Ptr()
      : GlslBuiltinTest<glsl::FrexpStructdvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_Frexp_dvec3_ivec3Ptr) {}
};

TEST_F(op_glsl_Frexp_dvec3_ivec3Ptr, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2});
  }
}

// Note: All pointer arguments point to within the results buffer.
// The result type FrexpStructdvec4Ty allows access to all pointed-to arguments.
class op_glsl_Frexp_dvec4_ivec4Ptr
    : public GlslBuiltinTest<glsl::FrexpStructdvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_Frexp_dvec4_ivec4Ptr()
      : GlslBuiltinTest<glsl::FrexpStructdvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_Frexp_dvec4_ivec4Ptr) {}
};

TEST_F(op_glsl_Frexp_dvec4_ivec4Ptr, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2});
  }
}

// The following test that the Frexp instruction is correctly implemented.
// This is not a test for precision, rather this is to ensure that the pointers
// passed to the instruction are correctly handled. As a result, a limited
// of argument combinations are tested, and the argument values were chosen
// completely arbitrarily.

// NOTE that frexp operates directly on the floating point IEE754
// representation and hence there is no need to do a fuzzy comparison.
TEST_F(op_glsl_Frexp_float_intPtr, ArgumentsPassedCorrectly) {
  // Expected results (significand,exponent):
  //    Frexp(10) = (0.625, 4)

  auto result = RunWithArgs(10);
  EXPECT_EQ(result.significand, 0.625f);
  EXPECT_EQ(result.exponent, 4);
}

TEST_F(op_glsl_Frexp_vec2_ivec2Ptr, ArgumentsPassedCorrectly) {
  // Expected results (significand,exponent):
  //    Frexp(0.1) = (  0.8, -3)
  //    Frexp(20)  = ( 0.625, 5)

  auto result = RunWithArgs({0.1f, 20});
  EXPECT_EQ(result.significand, glsl::vec2Ty(0.8f, 0.625f));
  EXPECT_EQ(result.exponent, glsl::ivec2Ty(-3, 5));
}

TEST_F(op_glsl_Frexp_vec4_ivec4Ptr, ArgumentsPassedCorrectly) {
  // Expected results:
  //    Frexp(0.1) = (  0.8, -3)
  //    Frexp(1)   = (  0.5,  1)
  //    Frexp(10)  = ( 0.625, 4)
  //    Frexp(20)  = ( 0.625, 5)

  auto result = RunWithArgs({0.1f, 1, 10, 20});
  EXPECT_EQ(result.significand, glsl::vec4Ty(0.800f, 0.500f, 0.625f, 0.625f));
  EXPECT_EQ(result.exponent, glsl::ivec4Ty(-3, 1, 4, 5));
}

TEST_F(op_glsl_Frexp_double_intPtr, ArgumentsPassedCorrectly) {
  // Expected result:
  //    Frexp(1) = (0.5, 1)
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(10);
    EXPECT_EQ(0.625, result.significand);
    EXPECT_EQ(4, result.exponent);
  }
}

TEST_F(op_glsl_Frexp_dvec3_ivec3Ptr, ArgumentsPassedCorrectly) {
  // Expected results:
  //    Frexp(1) = (  0.5,  1)
  //    Frexp(2) = (  0.5,  2)
  //    Frexp(3) = (  0.75, 2)
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({1, 2, 3});
    EXPECT_EQ(result.significand, glsl::dvec3Ty(0.5, 0.5, 0.75));
    EXPECT_EQ(result.exponent, glsl::ivec3Ty(1, 2, 2));
  }
}

class op_glsl_FrexpStruct_float
    : public GlslBuiltinTest<glsl::FrexpStructfloatTy, glsl::floatTy> {
 public:
  op_glsl_FrexpStruct_float()
      : GlslBuiltinTest<glsl::FrexpStructfloatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_FrexpStruct_float) {}
};

TEST_F(op_glsl_FrexpStruct_float, Smoke) { RunWithArgs(2); }

class op_glsl_FrexpStruct_vec2
    : public GlslBuiltinTest<glsl::FrexpStructvec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_FrexpStruct_vec2()
      : GlslBuiltinTest<glsl::FrexpStructvec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_FrexpStruct_vec2) {}
};

TEST_F(op_glsl_FrexpStruct_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_FrexpStruct_vec3
    : public GlslBuiltinTest<glsl::FrexpStructvec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_FrexpStruct_vec3()
      : GlslBuiltinTest<glsl::FrexpStructvec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_FrexpStruct_vec3) {}
};

TEST_F(op_glsl_FrexpStruct_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_FrexpStruct_vec4
    : public GlslBuiltinTest<glsl::FrexpStructvec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_FrexpStruct_vec4()
      : GlslBuiltinTest<glsl::FrexpStructvec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_FrexpStruct_vec4) {}
};

TEST_F(op_glsl_FrexpStruct_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

class op_glsl_FrexpStruct_double
    : public GlslBuiltinTest<glsl::FrexpStructdoubleTy, glsl::doubleTy> {
 public:
  op_glsl_FrexpStruct_double()
      : GlslBuiltinTest<glsl::FrexpStructdoubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_FrexpStruct_double) {}
};

TEST_F(op_glsl_FrexpStruct_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2);
  }
}

class op_glsl_FrexpStruct_dvec2
    : public GlslBuiltinTest<glsl::FrexpStructdvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_FrexpStruct_dvec2()
      : GlslBuiltinTest<glsl::FrexpStructdvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_FrexpStruct_dvec2) {}
};

TEST_F(op_glsl_FrexpStruct_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2});
  }
}

class op_glsl_FrexpStruct_dvec3
    : public GlslBuiltinTest<glsl::FrexpStructdvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_FrexpStruct_dvec3()
      : GlslBuiltinTest<glsl::FrexpStructdvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_FrexpStruct_dvec3) {}
};

TEST_F(op_glsl_FrexpStruct_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2});
  }
}

class op_glsl_FrexpStruct_dvec4
    : public GlslBuiltinTest<glsl::FrexpStructdvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_FrexpStruct_dvec4()
      : GlslBuiltinTest<glsl::FrexpStructdvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_FrexpStruct_dvec4) {}
};

TEST_F(op_glsl_FrexpStruct_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2});
  }
}

// Identical to above, but this time operating on the struct variations of the
// functions
TEST_F(op_glsl_FrexpStruct_float, ArgumentsPassedCorrectly) {
  // Expected results (significand,exponent):
  //    FrexpStruct(10) = (0.625, 4)

  auto result = RunWithArgs(10);
  EXPECT_EQ(result.significand, 0.625f);
  EXPECT_EQ(result.exponent, 4);
}

TEST_F(op_glsl_FrexpStruct_vec2, ArgumentsPassedCorrectly) {
  // Expected results (significand,exponent):
  //    FrexpStruct(0.1) = (  0.8, -3)
  //    FrexpStruct(20)  = ( 0.625, 5)

  auto result = RunWithArgs({0.1f, 20});
  EXPECT_EQ(result.significand, glsl::vec2Ty(0.8f, 0.625f));
  EXPECT_EQ(result.exponent, glsl::ivec2Ty(-3, 5));
}

TEST_F(op_glsl_FrexpStruct_vec4, ArgumentsPassedCorrectly) {
  // Expected results:
  //    FrexpStruct(0.1) = (  0.8, -3)
  //    FrexpStruct(1)   = (  0.5,  1)
  //    FrexpStruct(10)  = ( 0.625, 4)
  //    FrexpStruct(20)  = ( 0.625, 5)

  auto result = RunWithArgs({0.1f, 1, 10, 20});
  EXPECT_EQ(result.significand, glsl::vec4Ty(0.800f, 0.500f, 0.625f, 0.625f));
  EXPECT_EQ(result.exponent, glsl::ivec4Ty(-3, 1, 4, 5));
}

TEST_F(op_glsl_FrexpStruct_double, ArgumentsPassedCorrectly) {
  // Expected result:
  //    FrexpStruct(10) = (0.625, 4)
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(10);
    EXPECT_EQ(0.625, result.significand);
    EXPECT_EQ(4, result.exponent);
  }
}

TEST_F(op_glsl_FrexpStruct_dvec3, ArgumentsPassedCorrectly) {
  // Expected results:
  //    FrexpStruct(1) = (  0.5,  1)
  //    FrexpStruct(2) = (  0.5,  2)
  //    FrexpStruct(3) = (  0.75, 2)
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({1, 2, 3});
    EXPECT_EQ(result.significand, glsl::dvec3Ty(0.5, 0.5, 0.75));
    EXPECT_EQ(result.exponent, glsl::ivec3Ty(1, 2, 2));
  }
}

#endif

class op_glsl_Ldexp_float_int
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::intTy> {
 public:
  op_glsl_Ldexp_float_int()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::intTy>(
            uvk::Shader::op_glsl_Ldexp_float_int) {}
};

TEST_F(op_glsl_Ldexp_float_int, Smoke) { RunWithArgs(2, 2); }

class op_glsl_Ldexp_vec2_ivec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::ivec2Ty> {
 public:
  op_glsl_Ldexp_vec2_ivec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::ivec2Ty>(
            uvk::Shader::op_glsl_Ldexp_vec2_ivec2) {}
};

TEST_F(op_glsl_Ldexp_vec2_ivec2, Smoke) { RunWithArgs({2, 2}, {2, 2}); }

class op_glsl_Ldexp_vec3_ivec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::ivec3Ty> {
 public:
  op_glsl_Ldexp_vec3_ivec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::ivec3Ty>(
            uvk::Shader::op_glsl_Ldexp_vec3_ivec3) {}
};

TEST_F(op_glsl_Ldexp_vec3_ivec3, Smoke) { RunWithArgs({2, 2, 2}, {2, 2, 2}); }

class op_glsl_Ldexp_vec4_ivec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::ivec4Ty> {
 public:
  op_glsl_Ldexp_vec4_ivec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::ivec4Ty>(
            uvk::Shader::op_glsl_Ldexp_vec4_ivec4) {}
};

TEST_F(op_glsl_Ldexp_vec4_ivec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
}

class op_glsl_Ldexp_double_int
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::intTy> {
 public:
  op_glsl_Ldexp_double_int()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::intTy>(
            uvk::Shader::op_glsl_Ldexp_double_int) {}
};

TEST_F(op_glsl_Ldexp_double_int, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2, 2);
  }
}

class op_glsl_Ldexp_dvec2_ivec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::ivec2Ty> {
 public:
  op_glsl_Ldexp_dvec2_ivec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::ivec2Ty>(
            uvk::Shader::op_glsl_Ldexp_dvec2_ivec2) {}
};

TEST_F(op_glsl_Ldexp_dvec2_ivec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2}, {2, 2});
  }
}

class op_glsl_Ldexp_dvec3_ivec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::ivec3Ty> {
 public:
  op_glsl_Ldexp_dvec3_ivec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::ivec3Ty>(
            uvk::Shader::op_glsl_Ldexp_dvec3_ivec3) {}
};

TEST_F(op_glsl_Ldexp_dvec3_ivec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2}, {2, 2, 2});
  }
}

class op_glsl_Ldexp_dvec4_ivec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::ivec4Ty> {
 public:
  op_glsl_Ldexp_dvec4_ivec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::ivec4Ty>(
            uvk::Shader::op_glsl_Ldexp_dvec4_ivec4) {}
};

TEST_F(op_glsl_Ldexp_dvec4_ivec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
  }
}

TEST_F(op_glsl_Ldexp_float_int, BasicCorrectnessTest) {
  // From specification:
  //   Builds a floating-point number from x and the corresponding integral
  //   exponent of two in exp:
  //
  //   significand * 2^exponent
  //
  //   If this product is too large to be represented in the floating-point
  //   type, the result is undefined. If exp is greater than +128 (single
  //   precision) or +1024 (double precision), the result undefined. If exp is
  //   less than -126 (single precision) or -1022 (double precision), the result
  //   may be flushed to zero. Additionally, splitting the value into a
  //   significand and exponent using frexp and then reconstructing a
  //   floating-point value using ldexp should yield the original input for zero
  //   and all finite non-denormalized values.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   The exp operand must be a scalar or vector with integer component type.
  //   The number of components in x and exp must be the same.
  //
  //   Result Type must be the same type as the type of x. Results are computed
  //   per component.
  // Expected results:
  //   Ldexp(2.3, 4) = 36.8

  auto result = RunWithArgs(2.3f, 4);
  EXPECT_TRUE(glsl::fuzzyEq(36.8f, result));
}

TEST_F(op_glsl_Ldexp_vec4_ivec4, BasicCorrectnessTest) {
  // From specification:
  //   Builds a floating-point number from x and the corresponding integral
  //   exponent of two in exp:
  //
  //   significand * 2^exponent
  //
  //   If this product is too large to be represented in the floating-point
  //   type, the result is undefined. If exp is greater than +128 (single
  //   precision) or +1024 (double precision), the result undefined. If exp is
  //   less than -126 (single precision) or -1022 (double precision), the result
  //   may be flushed to zero. Additionally, splitting the value into a
  //   significand and exponent using frexp and then reconstructing a
  //   floating-point value using ldexp should yield the original input for zero
  //   and all finite non-denormalized values.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   The exp operand must be a scalar or vector with integer component type.
  //   The number of components in x and exp must be the same.
  //
  //   Result Type must be the same type as the type of x. Results are computed
  //   per component.
  // Expected results:
  //   Ldexp(<0.0, 0.000001, -5.5, 10000.0>, <0, 128, -6, -10>) =
  //     <0.0, 3.402823669e32, -0.0859375, 9.765625>

  auto result =
      RunWithArgs({0.0f, 0.000001f, -5.5f, 10000.0f}, {0, 128, -6, -10});
  EXPECT_TRUE(
      glsl::fuzzyEq({0.0f, 3.402823669e32f, -0.0859375f, 9.765625f}, result));
}

TEST_F(op_glsl_Ldexp_double_int, BasicCorrectnessTest) {
  // From specification:
  //   Builds a floating-point number from x and the corresponding integral
  //   exponent of two in exp:
  //
  //   significand * 2^exponent
  //
  //   If this product is too large to be represented in the floating-point
  //   type, the result is undefined. If exp is greater than +128 (single
  //   precision) or +1024 (double precision), the result undefined. If exp is
  //   less than -126 (single precision) or -1022 (double precision), the result
  //   may be flushed to zero. Additionally, splitting the value into a
  //   significand and exponent using frexp and then reconstructing a
  //   floating-point value using ldexp should yield the original input for zero
  //   and all finite non-denormalized values.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   The exp operand must be a scalar or vector with integer component type.
  //   The number of components in x and exp must be the same.
  //
  //   Result Type must be the same type as the type of x. Results are computed
  //   per component.
  // Expected results:
  //   Ldexp(-0.045, 1024) = -8.089619107*10^306
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(-0.045, 1024);
    EXPECT_TRUE(glsl::fuzzyEq(-8.089619107e306, result, 10.0e296));
  }
}

TEST_F(op_glsl_Ldexp_dvec4_ivec4, BasicCorrectnessTest) {
  // From specification:
  //   Builds a floating-point number from x and the corresponding integral
  //   exponent of two in exp:
  //
  //   significand * 2^exponent
  //
  //   If this product is too large to be represented in the floating-point
  //   type, the result is undefined. If exp is greater than +128 (single
  //   precision) or +1024 (double precision), the result undefined. If exp is
  //   less than -126 (single precision) or -1022 (double precision), the result
  //   may be flushed to zero. Additionally, splitting the value into a
  //   significand and exponent using frexp and then reconstructing a
  //   floating-point value using ldexp should yield the original input for zero
  //   and all finite non-denormalized values.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   The exp operand must be a scalar or vector with integer component type.
  //   The number of components in x and exp must be the same.
  //
  //   Result Type must be the same type as the type of x. Results are computed
  //   per component.
  // Expected results:
  //   Ldexp(<0.499, -0.99, 0.0, -0.045>, <5, -9, 0, -6>) =
  //     <15.968, -0.001933594, 0.0, -0.000703125>
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({0.499, -0.99, 0.0, -0.045}, {5, -9, 0, -6});
    EXPECT_TRUE(
        glsl::fuzzyEq({15.968, -0.001933594, 0.0, -0.000703125}, result));
  }
}
