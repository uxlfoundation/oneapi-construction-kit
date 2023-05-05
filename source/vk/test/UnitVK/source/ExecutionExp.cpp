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

class op_glsl_Exp_float : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Exp_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Exp_float) {}
};

TEST_F(op_glsl_Exp_float, Smoke) { RunWithArgs(2); }

class op_glsl_Exp_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Exp_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Exp_vec2) {}
};

TEST_F(op_glsl_Exp_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Exp_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Exp_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Exp_vec3) {}
};

TEST_F(op_glsl_Exp_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Exp_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Exp_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Exp_vec4) {}
};

TEST_F(op_glsl_Exp_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Exp_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is the natural exponentiation of x.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Exp(2.3) = 9.974182455

  auto result = RunWithArgs(2.3f);
  EXPECT_TRUE(glsl::fuzzyEq(9.974182455f, result));
}

TEST_F(op_glsl_Exp_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the natural exponentiation of x.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Exp(<0.0, 1.0, -5.3, 20.0>) =
  //     <1.0, 2.718281828, 0.004991594, 485165195.409790278>

  auto result = RunWithArgs({0.0f, 1.0f, -5.3f, 20.0f});
  EXPECT_TRUE(
      glsl::fuzzyEq({1.0f, 2.718281828f, 0.004991594f, 485165195.409790278f},
                    result, 0.00001f));
}

class op_glsl_Exp2_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Exp2_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Exp2_float) {}
};

TEST_F(op_glsl_Exp2_float, Smoke) { RunWithArgs(2); }

class op_glsl_Exp2_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Exp2_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Exp2_vec2) {}
};

TEST_F(op_glsl_Exp2_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Exp2_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Exp2_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Exp2_vec3) {}
};

TEST_F(op_glsl_Exp2_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Exp2_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Exp2_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Exp2_vec4) {}
};

TEST_F(op_glsl_Exp2_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Exp2_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is 2 raised to the x power.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Exp2(20.2) = 1204497.526289371

  auto result = RunWithArgs(20.2f);
  EXPECT_TRUE(glsl::fuzzyEq(1204497.526289371f, result, 1.0f));
}

TEST_F(op_glsl_Exp2_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is 2 raised to the x power.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Exp2(<0.0, 1.0, -5.3, 2.3>) =
  //     <1.0, 2.0, 0.025382887, 4.924577653>

  auto result = RunWithArgs({0.0f, 1.0f, -5.3f, 2.3f});
  EXPECT_TRUE(glsl::fuzzyEq({1.0f, 2.0f, 0.025382887f, 4.924577653f}, result));
}
