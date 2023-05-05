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

#include <cmath>
#include <limits>
#include "GLSLTestDefs.h"

constexpr float F_INF = std::numeric_limits<float>::infinity();
constexpr float F_NAN = std::numeric_limits<float>::quiet_NaN();

class op_glsl_PackSnorm4x8_vec4
    : public GlslBuiltinTest<glsl::uintTy, glsl::vec4Ty> {
 public:
  op_glsl_PackSnorm4x8_vec4()
      : GlslBuiltinTest<glsl::uintTy, glsl::vec4Ty>(
            uvk::Shader::op_glsl_PackSnorm4x8_vec4) {}
};

TEST_F(op_glsl_PackSnorm4x8_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

// Tests that PackSnorm4x8 is correctly implemented
TEST_F(op_glsl_PackSnorm4x8_vec4, BasicCorrectnessTest) {
  // From specification:
  //   First, converts each component of the normalized floating-point value v
  //   into 8-bit integer values. These are then packed into the result.
  //   The instruction performs the following conversion per element:
  //       v = round(clamp(c, -1, +1) * 127)
  //   Where C is a normalized floating point number.
  //
  //   The first component of the vector will correspond to the least signifcant
  //   bits of the output and the last component will correspond to the most
  //   significand bits. The result of the instruction is a 32 bit integer type.
  // Additional:
  //   The standard states that clamp is undefined if c is NaN, and round does
  //   not specify NaN behaviour, so the result is undefined for that particular
  //   component. round is allowed in an implementation defined direction when
  //   fract(c) == 0.5.
  //
  //   Curiously, the standard specifies no required precision for this
  //   instruction.
  // Expected results:
  //   PackSnorm4x8(<-100/127, -10/127, 10/127, 100/127>) = 0x640af69c
  //   PackSnorm4x8(<-inf, -NaN, +NaN, +inf>)             = 0xff????81
  //   PackSnorm4x8(<-1, -0, 0, 1>)                       = 0x7f000081
  //   PackSnorm4x8(<-0.5f, 0.5f, 0, 0>)                  = 0x00003FC0
  //                                                     or 0x00003FC1
  //                                                     or 0x000040C0
  //                                                     or 0x000040C1

  // Test behaviour over expected input range [-1, +1]
  auto result = RunWithArgs(
      {-100.0f / 127.0f, -10.0f / 127.0f, 10.0f / 127.0f, 100.0f / 127.0f});
  EXPECT_EQ(0x640af69cU, result);
  // Test behaviour at floating point edge cases
  result = RunWithArgs({-F_INF, -F_NAN, F_NAN, F_INF});
  EXPECT_EQ(0x7f000081U, result & 0xff0000ff);
  // Test behaviour at edge of input range at for +/- 0
  result = RunWithArgs({-1.0f, -0.0f, 0.0f, 1.0f});
  EXPECT_EQ(0x7f000081U, result);
  // Test rounding behaviour
  result = RunWithArgs({-0.5f, 0.5f, 0.0f, 0.0f});
  EXPECT_TRUE(result == 0x00003FC0U || result == 0x00003FC1U ||
              result == 0x000040C0U || result == 0x000040C1U);
}

class op_glsl_PackUnorm4x8_vec4
    : public GlslBuiltinTest<glsl::uintTy, glsl::vec4Ty> {
 public:
  op_glsl_PackUnorm4x8_vec4()
      : GlslBuiltinTest<glsl::uintTy, glsl::vec4Ty>(
            uvk::Shader::op_glsl_PackUnorm4x8_vec4) {}
};

TEST_F(op_glsl_PackUnorm4x8_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

// Tests that PackUnorm4x8 is correctly implemented
TEST_F(op_glsl_PackUnorm4x8_vec4, BasicCorrectnessTest) {
  // From specification:
  //   First, converts each component of the normalized floating-point value v
  //   into 8-bit integer values. These are then packed into the result.
  //   The instruction performs the following conversion per element:
  //       v = round(clamp(c, 0, +1) * 255)
  //   Where C is a normalized floating point number.
  //
  //   The first component of the vector will correspond to the least signifcant
  //   bits of the output and the last component will correspond to the most
  //   significand bits. The result of the instruction is a 32 bit integer type.
  // Additional:
  //   The standard states that clamp is undefined if c is NaN, and round does
  //   not specify NaN behaviour, so the result is undefined for that particular
  //   component. round is allowed in an implementation defined direction when
  //   fract(c) == 0.5.
  // Expected results:
  //   PackUnorm4x8(<0, 0.33, 0.55, 1>)       = 0xff8c5400
  //   PackUnorm4x8(<-inf, -NaN, +NaN, +inf>) = 0xff????00
  //   PackUnorm4x8(<-0.5, -0.0 , 0.10, 0.5>) = 0x7F190000
  //                                         or 0x80190000
  //                                         or 0x7F1a0000
  //                                         or 0x801a0000

  // Test behaviour at endpoints of range and for normalized values:
  auto result = RunWithArgs({0.0f, 0.33f, 0.55f, 1.0f});
  EXPECT_EQ(0xff8c5400U, result);
  // Test behaviour for floating point edge cases
  result = RunWithArgs({-F_INF, -F_NAN, F_NAN, F_INF});
  EXPECT_EQ(0xff000000U, result & 0xff0000ff);
  // Test round behaviours of floats, negative numbers and handling of -0
  result = RunWithArgs({-0.5f, -0.0f, 0.1f, 0.5f});
  EXPECT_TRUE(result == 0x7F190000U || result == 0x80190000U ||
              result == 0x7F1a0000U || result == 0x801a0000U);
}

class op_glsl_PackSnorm2x16_vec2
    : public GlslBuiltinTest<glsl::uintTy, glsl::vec2Ty> {
 public:
  op_glsl_PackSnorm2x16_vec2()
      : GlslBuiltinTest<glsl::uintTy, glsl::vec2Ty>(
            uvk::Shader::op_glsl_PackSnorm2x16_vec2) {}
};

TEST_F(op_glsl_PackSnorm2x16_vec2, Smoke) { RunWithArgs({2, 2}); }

// Tests that PackSnorm2x16 is correctly implemented
TEST_F(op_glsl_PackSnorm2x16_vec2, BasicCorrectnessTest) {
  // From specification:
  //   First, converts each component of the normalized floating-point value v
  //   into 16-bit integer values. These are then packed into the result.
  //   The conversion for component c of v to fixed point is done as follows:
  //       round(clamp(c, -1, +1) * 32767.0)
  //   The first component of the vector will be written to the least
  //   significant bits of the output; the last component will be written to the
  //   most significant bits.
  // Expected results:
  //   PackSnorm2x16(<-20000/32767, 200000/32767>) = 0x4e20b1e0
  //   PackSnorm2x16(<-inf, inf>)                  = 0x7fff8001
  //   PackSnorm2x16(<-1, 1>)                      = 0x7fff8001

  // Test behaviour over expected input range
  auto result = RunWithArgs({-20000.0f / 32767.0f, 20000.0f / 32767.0f});
  EXPECT_EQ(0x4e20b1e0U, result);
  // Test behaviour at floating point edge cases
  result = RunWithArgs({-F_INF, F_INF});
  EXPECT_EQ(0x7fff8001U, result);
  // (NaN behaviour is undefined)
  // Test behaviour at edge of input range
  result = RunWithArgs({-1.0f, 1.0f});
  EXPECT_EQ(0x7fff8001U, result);
}

class op_glsl_PackUnorm2x16_vec2
    : public GlslBuiltinTest<glsl::uintTy, glsl::vec2Ty> {
 public:
  op_glsl_PackUnorm2x16_vec2()
      : GlslBuiltinTest<glsl::uintTy, glsl::vec2Ty>(
            uvk::Shader::op_glsl_PackUnorm2x16_vec2) {}
};

TEST_F(op_glsl_PackUnorm2x16_vec2, Smoke) { RunWithArgs({2, 2}); }

// Tests that PackUnorm2x16 is correctly implemented
TEST_F(op_glsl_PackUnorm2x16_vec2, BasicCorrectnessTest) {
  // From specification:
  //   First, converts each component of the normalized floating-point value v
  //   into 16-bit integer values. These are then packed into the result.
  //   The conversion for component c of v to fixed point is done as follows:
  //       round(clamp(c, 0, +1) * 65535.0)
  //   The first component of the vector will be written to the least
  //   significant bits of the output; the last component will be written to the
  //   most significant bits.
  // Expected results:
  //   PackUnorm2x16(<20000/65535, 40000/65536>) = 0x9c404e20
  //   PackUnorm2x16(<-10, 30>)                  = 0xffff0000
  //   PackUnorm2x16(<  0,  1>)                  = 0xffff0000
  //   PackUnorm2x16(<-inf, inf>)                = 0xffff0000

  // Test behaviour over expected input range
  // (division here is to avoid floating pointer rounding errors)
  auto result = RunWithArgs({20000.0f / 65535.0f, 40000.0f / 65535.0f});
  EXPECT_EQ(0x9c404e20, result);
  // Test clamping behaviour
  result = RunWithArgs({-10.0f, 30.0f});
  EXPECT_EQ(0xffff0000, result);
  // Test behaviour at edges of input range
  result = RunWithArgs({0.0f, 1.0f});
  EXPECT_EQ(0xffff0000, result);
  // Test behaviour at floating point edge cases
  result = RunWithArgs({-F_INF, F_INF});
  EXPECT_EQ(0xffff0000, result);
  // (NaN behaviour is undefined)
}

class op_glsl_PackHalf2x16_vec2
    : public GlslBuiltinTest<glsl::uintTy, glsl::vec2Ty> {
 public:
  op_glsl_PackHalf2x16_vec2()
      : GlslBuiltinTest<glsl::uintTy, glsl::vec2Ty>(
            uvk::Shader::op_glsl_PackHalf2x16_vec2) {}
};

TEST_F(op_glsl_PackHalf2x16_vec2, Smoke) { RunWithArgs({2, 2}); }

// Tests that PackHalf2x16 is correctly implemented
TEST_F(op_glsl_PackHalf2x16_vec2, BasicCorrectnessTest) {
  // From specification:
  //   Result is the unsigned integer obtained by converting the components of a
  //   two-component floating-point vector to the 16-bit OpTypeFloat, and then
  //   packing these two 16-bit integers into a 32-bit unsigned integer. The
  //   first vector component specifies the 16 least-significant bits of the
  //   result; the second component specifies the 16 most-significant bits.
  //
  //   The v operand must be a vector of 2 components whose type is a 32-bit
  //   floating-point.
  //
  //   Result Type must be a 32-bit integer type.
  // Expected results:
  //   PackHalf2x16(<5.5, 0.05>) = 0xAA664580
  //   PackHalf2x16(<inf, 2.0E-39>) = 0x00007C00
  //   PackHalf2x16(<0.0, -inf>) = 0xFC000000
  //   PackHalf2x16(<NaN, -1.0>) = 0xBC00000 | Half-NaN

  auto result = RunWithArgs({5.5f, -0.05f});
  EXPECT_EQ(0xAA664580, result);

  result = RunWithArgs({F_INF, 2.0E-39f});
  EXPECT_EQ(0x00007C00, result);

  result = RunWithArgs({0.0f, -F_INF});
  EXPECT_EQ(0xFC000000, result);

  result = RunWithArgs({F_NAN, -1.0f});
  // Check -1.0
  EXPECT_EQ(0xBC000000, result & 0xFFFF0000);
  // Check NaN
  EXPECT_TRUE((result & 0x7C00) == 0x7C00 && (result & 0x3FF) != 0);
}

class op_glsl_PackDouble2x32_uvec2
    : public GlslBuiltinTest<glsl::doubleTy, glsl::uvec2Ty> {
 public:
  op_glsl_PackDouble2x32_uvec2()
      : GlslBuiltinTest<glsl::doubleTy, glsl::uvec2Ty>(
            uvk::Shader::op_glsl_PackDouble2x32_uvec2) {}
};

TEST_F(op_glsl_PackDouble2x32_uvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2});
  }
}

// Tests that PackDouble2x32 is correctly implemented
TEST_F(op_glsl_PackDouble2x32_uvec2, BasicCorrectnessTest) {
  // From specification:
  //   Result is the double-precision value obtained by packing the components
  //   of v into a 64-bit value. If an IEEE 754 Inf or NaN is created, it will
  //   not signal, and the resulting floating-point value is unspecified.
  //   Otherwise, the bit-level representation of v is preserved. The first
  //   vector component specifies the 32 least significant bits; the second
  //   component specifies the 32 most significant bits.
  // Expected results:
  //   PackDouble2x32(<0x00000000, 0x4059a000>) = 102.5

  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({0x00000000, 0x4059a000});
    EXPECT_EQ(102.5, result);
  }
}

class op_glsl_UnpackSnorm2x16_uint
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::uintTy> {
 public:
  op_glsl_UnpackSnorm2x16_uint()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::uintTy>(
            uvk::Shader::op_glsl_UnpackSnorm2x16_uint) {}
};

TEST_F(op_glsl_UnpackSnorm2x16_uint, Smoke) { RunWithArgs(2); }

// Tests that UnpackSnorm2x16 is correctly implemented
TEST_F(op_glsl_UnpackSnorm2x16_uint, BasicCorrectnessTest) {
  // From specification:
  //   First, unpacks a single 32-bit unsigned integer p into a pair of 16-bit
  //   signed integers. Then, each component is converted to a normalized
  //   floating-point value to generate the result. The conversion for unpacked
  //   fixed-point value f to floating point is done as follows:
  //       clamp(f / 32767.0, -1, +1)
  //   The first component of the result will be extracted from the least
  //   significant bits of the input; the last component will be extracted from
  //   the most significant bits.
  // Expected results:
  //   UnpackSnorm2x16(0xCCCD3333) = <0.40001, -0.40001>
  //   UnpackSnorm2x16(0x00008000) = <-1.0, 0>

  // Test for correct handling of positive and negative numbers
  auto result = RunWithArgs(0xCCCD3333);
  ASSERT_TRUE(glsl::fuzzyEq(result, glsl::vec2Ty(0.40001f, -0.40001f))) << result;
  // Test for correct handling of 0 and clamping of f = -32768
  result = RunWithArgs(0x00008000);
  ASSERT_TRUE(glsl::fuzzyEq(result, glsl::vec2Ty(-1.0f, 0.0f))) << result;
}

class op_glsl_UnpackUnorm2x16_uint
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::uintTy> {
 public:
  op_glsl_UnpackUnorm2x16_uint()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::uintTy>(
            uvk::Shader::op_glsl_UnpackUnorm2x16_uint) {}
};

TEST_F(op_glsl_UnpackUnorm2x16_uint, Smoke) { RunWithArgs(2); }

// Tests that UnpackUnorm2x16 is correctly implemented
TEST_F(op_glsl_UnpackUnorm2x16_uint, BasicCorrectnessTest) {
  // From specification:
  //   First, unpacks a single 32-bit unsigned integer p into a pair of 16-bit
  //   unsigned integers. Then, each component is converted to a normalized
  //   floating-point value to generate the result. The conversion for unpacked
  //   fixed-point value f to floating point is done as follows:
  //       f / 65535.0
  //   The first component of the result will be extracted from the least
  //   significant bits of the input; the last component will be extracted from
  //   the most significant bits.
  // Expected results:
  //   UnpackUnorm2x16(0xff010101) = <0.00392, 0.99612>

  // Test with two random values
  auto result = RunWithArgs(0xff010101);
  ASSERT_TRUE(glsl::fuzzyEq(result, glsl::vec2Ty(0.00392f, 0.99612f)));
}

class op_glsl_UnpackHalf2x16_uint
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::uintTy> {
 public:
  op_glsl_UnpackHalf2x16_uint()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::uintTy>(
            uvk::Shader::op_glsl_UnpackHalf2x16_uint) {}
};

TEST_F(op_glsl_UnpackHalf2x16_uint, Smoke) { RunWithArgs(2); }

// Tests that UnpackHalf2x16 is correctly implemented
TEST_F(op_glsl_UnpackHalf2x16_uint, BasicCorrectnessTest) {
  // From specification:
  //   Result is the two-component floating-point vector with components
  //   obtained by unpacking a 32-bit unsigned integer into a pair of 16-bit
  //   values, interpreting those values as 16-bit floating-point numbers
  //   according to the OpenGL Specification, and converting them to 32-bit
  //   floating-point values. Subnormal numbers are either preserved or flushed
  //   to zero, consistently within an implemenation.
  //
  //   The first component of the vector is obtained from the 16
  //   least-significant bits of v; the second component is obtained from the 16
  //   most-significant bits of v.
  //
  //   The v operand must be a scalar with 32-bit integer type.
  //
  //   Result Type must be a vector of 2 components whose type is 32-bit
  //   floating point.
  // Expected results:
  //   UnpackHalf2x16(0xAA664580) = <5.5, -0.05>
  //   UnpackHalf2x16(0x68E7C00) = <inf, 0.0001>
  //   UnpackHalf2x16(0xFC000000) = <0.0, -inf>
  //   UnpackHalf2x16(0xFC02BC00) = <-1.0, NaN>
  //   UnpackHalf2x16(0x3C000045) = <4.1E-6, 1.0>

  auto result = RunWithArgs(0xAA664580);
  EXPECT_TRUE(glsl::fuzzyEq(result, {5.5f, -0.05f})) << result;

  result = RunWithArgs(0x68E7C00);
  EXPECT_TRUE(std::isinf(result.data[0]) && result.data[0] > 0.0f &&
              glsl::fuzzyEq(0.0001f, result.data[1]))
      << result;

  result = RunWithArgs(0xFC000000);
  EXPECT_TRUE(glsl::fuzzyEq(0.0f, result.data[0]) &&
              std::isinf(result.data[1]) && result.data[1] < 0.0f)
      << result;

  result = RunWithArgs(0xFC02BC00);
  EXPECT_TRUE(glsl::fuzzyEq(-1.0f, result.data[0]) &&
              std::isnan(result.data[1]))
      << result;

  result = RunWithArgs(0x3C000045);
  EXPECT_TRUE(glsl::fuzzyEq(result, {4.1E-6f, 1.0f}, 0.0000001f)) << result;
}

class op_glsl_UnpackSnorm4x8_uint
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::uintTy> {
 public:
  op_glsl_UnpackSnorm4x8_uint()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::uintTy>(
            uvk::Shader::op_glsl_UnpackSnorm4x8_uint) {}
};

TEST_F(op_glsl_UnpackSnorm4x8_uint, Smoke) { RunWithArgs(2); }

// Tests that UnpackSnorm4x8 is correctly implemented
TEST_F(op_glsl_UnpackSnorm4x8_uint, BasicCorrectnessTest) {
  // From specification:
  //   First, unpacks a single 32-bit unsigned integer p into four 8-bit signed
  //   integers. Then, each component is converted to a normalized
  //   floating-point value to generate the result. The conversion for unpacked
  //   fixed-point value f to floating point is done as follows:
  //       clamp(f / 127.0, -1, +1)
  //   The first component of the result will be extracted from the least
  //   significant bits of the input; the last component will be extracted from
  //   the most significant bits.
  // Expected results:
  //   UnpackSnorm4x8_uint(0x1f0080b2) = <-78/127, -1, 0, 31/127>

  // Test for normal behaviour and clamping of f = -128
  auto result = RunWithArgs(0x1f0080b2);
  ASSERT_TRUE(glsl::fuzzyEq(
      result, glsl::vec4Ty(-78.0f / 127.0f, -1.0f, 0.0f, 31.0f / 127.0f)));
}

class op_glsl_UnpackUnorm4x8_uint
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::uintTy> {
 public:
  op_glsl_UnpackUnorm4x8_uint()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::uintTy>(
            uvk::Shader::op_glsl_UnpackUnorm4x8_uint) {}
};

TEST_F(op_glsl_UnpackUnorm4x8_uint, Smoke) { RunWithArgs(2); }

// Tests that UnpackUnorm4x8 is correctly implemented
TEST_F(op_glsl_UnpackUnorm4x8_uint, BasicCorrectnessTest) {
  // From specification:
  //   First, unpacks a single 32-bit unsigned integer p into four 8-bit
  //   unsigned integers. Then, each component is converted to a normalized
  //   floating-point value to generate the result. The conversion for unpacked
  //   fixed-point value f to floating point is done as follows:
  //       f / 255.0
  //   The first component of the result will be extracted from the least
  //   significant bits of the input; the last component will be extracted from
  //   the most significant bits.
  // Expected results:
  //   UnpackUnorm4x8(0x008088ff) = <1, 0.53333, 0.50196, 0>

  auto result = RunWithArgs(0x008088ff);
  ASSERT_TRUE(
      glsl::fuzzyEq(result, glsl::vec4Ty(1.0f, 0.53333f, 0.50196f, 0.0f)));
}

class op_glsl_UnpackDouble2x32_double
    : public GlslBuiltinTest<glsl::uvec2Ty, glsl::doubleTy> {
 public:
  op_glsl_UnpackDouble2x32_double()
      : GlslBuiltinTest<glsl::uvec2Ty, glsl::doubleTy>(
            uvk::Shader::op_glsl_UnpackDouble2x32_double) {}
};

TEST_F(op_glsl_UnpackDouble2x32_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2);
  }
}

// Tests that UnpackDouble2x32 is correctly implemented
TEST_F(op_glsl_UnpackDouble2x32_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is the two-component unsigned integer vector representation of v.
  //   The bit-level representation of v is preserved. The first component of
  //   the vector contains the 32 least significant bits of the double; the
  //   second component consists of the 32 most significant bits.
  // Expected results:
  //   UnpackDouble2x32(102.5) = <0x00000000, 0x4059a000>
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(102.5);
    ASSERT_EQ(result, glsl::uvec2Ty(0x00000000U, 0x4059a000U));
  }
}
