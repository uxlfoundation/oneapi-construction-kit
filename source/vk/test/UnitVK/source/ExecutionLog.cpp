// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "GLSLTestDefs.h"

class op_glsl_Log_float : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Log_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Log_float) {}
};

TEST_F(op_glsl_Log_float, Smoke) { RunWithArgs(2); }

class op_glsl_Log_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Log_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Log_vec2) {}
};

TEST_F(op_glsl_Log_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Log_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Log_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Log_vec3) {}
};

TEST_F(op_glsl_Log_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Log_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Log_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Log_vec4) {}
};

TEST_F(op_glsl_Log_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Log_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is the natural logarithm of x, i.e., the value y which satisfies
  //   the equation x = e^y Result is undefined if x ≤ 0.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Log(2.3) = 0.832909123

  auto result = RunWithArgs(2.3f);
  EXPECT_TRUE(glsl::fuzzyEq(0.832909123f, result));
}

TEST_F(op_glsl_Log_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the natural logarithm of x, i.e., the value y which satisfies
  //   the equation x = e^y Result is undefined if x ≤ 0.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Log(<0.01, 1.0, 2.718281828, 5000.01>) =
  //     <-4.605170186, 0.0, 1.0, 8.517195191>

  auto result = RunWithArgs({0.01f, 1.0f, 2.718281828f, 5000.01f});
  EXPECT_TRUE(glsl::fuzzyEq({-4.605170186f, 0.0f, 1.0f, 8.517195191f}, result));
}

class op_glsl_Log2_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Log2_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Log2_float) {}
};

TEST_F(op_glsl_Log2_float, Smoke) { RunWithArgs(2); }

class op_glsl_Log2_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Log2_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Log2_vec2) {}
};

TEST_F(op_glsl_Log2_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Log2_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Log2_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Log2_vec3) {}
};

TEST_F(op_glsl_Log2_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Log2_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Log2_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Log2_vec4) {}
};

TEST_F(op_glsl_Log2_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Log2_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is the base-2 logarithm of x, i.e., the value y which satisfies
  //   the equation x = 2^y. Result is undefined if x ≤ 0.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Log2(2.3) = 1.201634

  auto result = RunWithArgs(2.3f);
  EXPECT_TRUE(glsl::fuzzyEq(1.201634f, result));
}

TEST_F(op_glsl_Log2_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the base-2 logarithm of x, i.e., the value y which satisfies
  //   the equation x = 2^y. Result is undefined if x ≤ 0.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Log2(<0.01, 1.0, 2.0, 5000.01>) =
  //     <-6.643856, 0.0, 1.0, 12.287715>

  auto result = RunWithArgs({0.01f, 1.0f, 2.0f, 5000.01f});
  EXPECT_TRUE(glsl::fuzzyEq({-6.643856f, 0.0f, 1.0f, 12.287715f}, result));
}
