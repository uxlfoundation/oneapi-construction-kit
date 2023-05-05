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

class op_glsl_Pow_float_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Pow_float_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Pow_float_float) {}
};

TEST_F(op_glsl_Pow_float_float, Smoke) { RunWithArgs(2, 2); }

class op_glsl_Pow_vec2_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Pow_vec2_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Pow_vec2_vec2) {}
};

TEST_F(op_glsl_Pow_vec2_vec2, Smoke) { RunWithArgs({2, 2}, {2, 2}); }

class op_glsl_Pow_vec3_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Pow_vec3_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Pow_vec3_vec3) {}
};

TEST_F(op_glsl_Pow_vec3_vec3, Smoke) { RunWithArgs({2, 2, 2}, {2, 2, 2}); }

class op_glsl_Pow_vec4_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Pow_vec4_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Pow_vec4_vec4) {}
};

TEST_F(op_glsl_Pow_vec4_vec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
}

TEST_F(op_glsl_Pow_float_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is x raised to the y power.
  //   Result is undefined if x < 0. Result is undefined if x = 0 and y ≤ 0.
  //
  //   The operand x and y must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Pow(2.3, 4.5) = 42.439988943

  auto result = RunWithArgs(2.3f, 4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(42.439988943f, result));
}

TEST_F(op_glsl_Pow_vec4_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is x raised to the y power.
  //   Result is undefined if x < 0. Result is undefined if x = 0 and y ≤ 0.
  //
  //   The operand x and y must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Pow(<0.0f, 0.99f, 50.25f, 1.0f>, <0.5f, 2.0f, 0.0f, 1000.0f>) =
  //     <0.0f, 0.9801, 1.0f, 1.0f>

  auto result =
      RunWithArgs({0.0f, 0.99f, 50.25f, 1.0f}, {0.5f, 2.0f, 0.0f, 1000.0f});
  EXPECT_TRUE(glsl::fuzzyEq({0.0f, 0.9801f, 1.0f, 1.0f}, result));
}
