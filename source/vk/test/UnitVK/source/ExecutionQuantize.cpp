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

class op_glsl_Quantize_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Quantize_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Quantize_float) {}
};

TEST_F(op_glsl_Quantize_float, Smoke) { RunWithArgs(2); }

TEST_F(op_glsl_Quantize_float, BasicCorrectnessTest) {
  auto result = RunWithArgs(3.141592f);
  // fuzzEq checks values are equal within an absolute error of 0.001 by
  // default. Absolute error isn't ideal for checking the mantissa bits are
  // correct but it serves this fairly basic use case.
  EXPECT_TRUE(glsl::fuzzyEq(3.14f, result));
}

TEST_F(op_glsl_Quantize_float, InfInInfOut) {
  // Tests the spec rule "If Value is an infinity, the result is the same
  // infinity.".
  const float arg = std::numeric_limits<float>::infinity();
  auto result = RunWithArgs(arg);
  EXPECT_EQ(arg, result);
}

TEST_F(op_glsl_Quantize_float, NanInNanOut) {
  // Tests the spec rule "If Value is a NaN, the result is a NaN, but not
  // necessarily the same NaN.".
  auto result = RunWithArgs(std::nan(""));
  EXPECT_TRUE(std::isnan(result));
}

TEST_F(op_glsl_Quantize_float, HighToInf) {
  // Tests the spec rule "If Value is positive with a magnitude too large to
  // represent as a 16-bit floating-point value, the result is positive
  // infinity."
  auto result = RunWithArgs(100000.f);
  EXPECT_EQ(std::numeric_limits<float>::infinity(), result);
}

TEST_F(op_glsl_Quantize_float, LowToInf) {
  // Tests the spec rule "If Value is negative with a magnitude too large to
  // represent as a 16-bit floating-point value, the result is negative
  // infinity."
  auto result = RunWithArgs(-100000.f);
  EXPECT_EQ(-std::numeric_limits<float>::infinity(), result);
}

TEST_F(op_glsl_Quantize_float, PositiveDenormal) {
  // Anything too small in magnitude to be represented as a normalized half
  // should be rounded to zero.
  auto result = RunWithArgs(0.00001f);
  EXPECT_EQ(0.f, result);
  EXPECT_TRUE(!std::signbit(result));
}

TEST_F(op_glsl_Quantize_float, NegativeDenormal) {
  // Anything too small in magnitude to be represented as a normalized half
  // should be rounded to zero.
  auto result = RunWithArgs(-0.00001f);
  EXPECT_EQ(-0.f, result);
  EXPECT_TRUE(std::signbit(result));
}

class op_glsl_Quantize_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Quantize_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Quantize_vec2) {}
};

TEST_F(op_glsl_Quantize_vec2, Smoke) { RunWithArgs({2, 2}); }

TEST_F(op_glsl_Quantize_vec2, BasicCorrectnessTest) {
  auto result = RunWithArgs({3.141592f, 3.141592f});
  EXPECT_TRUE(glsl::fuzzyEq({3.14f, 3.14f}, result));
}

class op_glsl_Quantize_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Quantize_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Quantize_vec3) {}
};

TEST_F(op_glsl_Quantize_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

TEST_F(op_glsl_Quantize_vec3, BasicCorrectnessTest) {
  auto result = RunWithArgs({3.141592f, 3.141592f, 3.141592f});
  EXPECT_TRUE(glsl::fuzzyEq({3.14f, 3.14f, 3.14f}, result));
}

class op_glsl_Quantize_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Quantize_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Quantize_vec4) {}
};

TEST_F(op_glsl_Quantize_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Quantize_vec4, BasicCorrectnessTest) {
  auto result = RunWithArgs({3.141592f, 3.141592f, 3.141592f, 3.141592f});
  EXPECT_TRUE(glsl::fuzzyEq({3.14f, 3.14f, 3.14f, 3.14f}, result));
}
