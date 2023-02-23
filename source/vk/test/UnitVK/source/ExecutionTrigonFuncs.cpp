// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "GLSLTestDefs.h"

class op_glsl_Sin_float : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Sin_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Sin_float) {}
};

TEST_F(op_glsl_Sin_float, Smoke) { RunWithArgs(2); }

class op_glsl_Sin_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Sin_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Sin_vec2) {}
};

TEST_F(op_glsl_Sin_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Sin_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Sin_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Sin_vec3) {}
};

TEST_F(op_glsl_Sin_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Sin_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Sin_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Sin_vec4) {}
};

TEST_F(op_glsl_Sin_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Sin_float, BasicCorrectnessTest) {
  // From specification:
  //   The standard trigonometric sine of x radians.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Sin(4.5) = -0.977530118

  auto result = RunWithArgs(4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(-0.977530118f, result));
}

TEST_F(op_glsl_Sin_vec4, BasicCorrectnessTest) {
  // From specification:
  //   The standard trigonometric sine of x radians.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Sin(<0.0f, 3.14159265359, -1.5, 8.56>) = <0.0, 0.0, -0.997494987,
  //   0.760951221>

  auto result = RunWithArgs({0.0f, 3.14159265359f, -1.5f, 8.56f});
  EXPECT_TRUE(glsl::fuzzyEq({0.0f, 0.0f, -0.997494987f, 0.760951221f}, result));
}

class op_glsl_Cos_float : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Cos_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Cos_float) {}
};

TEST_F(op_glsl_Cos_float, Smoke) { RunWithArgs(2); }

class op_glsl_Cos_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Cos_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Cos_vec2) {}
};

TEST_F(op_glsl_Cos_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Cos_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Cos_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Cos_vec3) {}
};

TEST_F(op_glsl_Cos_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Cos_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Cos_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Cos_vec4) {}
};

TEST_F(op_glsl_Cos_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Cos_float, BasicCorrectnessTest) {
  // From specification:
  //   The standard trigonometric cosine of x radians.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Cos(4.5) = -0.210795799

  auto result = RunWithArgs(4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(-0.210795799f, result));
}

TEST_F(op_glsl_Cos_vec4, BasicCorrectnessTest) {
  // From specification:
  //   The standard trigonometric cosine of x radians.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Cos(<0.0, 3.14159265359, -1.5, 8.56>) = <1.0, -1.0, 0.070737202,
  //   -0.648809093>

  auto result = RunWithArgs({0.0f, 3.14159265359f, -1.5f, 8.56f});
  EXPECT_TRUE(
      glsl::fuzzyEq({1.0f, -1.0f, 0.070737202f, -0.648809093f}, result));
}

class op_glsl_Tan_float : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Tan_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Tan_float) {}
};

TEST_F(op_glsl_Tan_float, Smoke) { RunWithArgs(2); }

class op_glsl_Tan_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Tan_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Tan_vec2) {}
};

TEST_F(op_glsl_Tan_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Tan_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Tan_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Tan_vec3) {}
};

TEST_F(op_glsl_Tan_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Tan_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Tan_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Tan_vec4) {}
};

TEST_F(op_glsl_Tan_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Tan_float, BasicCorrectnessTest) {
  // From specification:
  //   The standard trigonometric tangent of x radians.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Tan(4.5) = 4.637332055

  auto result = RunWithArgs(4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(4.637332055f, result));
}

TEST_F(op_glsl_Tan_vec4, BasicCorrectnessTest) {
  // From specification:
  //   The standard trigonometric tangent of x radians.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Tan(<0.0, 3.14159265359, -1.5, 4.711592654>) = <0.0, 0.0, -14.101419947,
  //   1255.766238376>

  auto result = RunWithArgs({0.0f, 3.14159265359f, -1.5f, 4.711592654f});
  EXPECT_TRUE(glsl::fuzzyEq({0.0f, 0.0f, -14.101419947f, 1255.766238376f},
                            result, 0.1f));
}

class op_glsl_Asin_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Asin_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Asin_float) {}
};

TEST_F(op_glsl_Asin_float, Smoke) { RunWithArgs(2); }

class op_glsl_Asin_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Asin_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Asin_vec2) {}
};

TEST_F(op_glsl_Asin_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Asin_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Asin_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Asin_vec3) {}
};

TEST_F(op_glsl_Asin_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Asin_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Asin_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Asin_vec4) {}
};

TEST_F(op_glsl_Asin_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Asin_float, BasicCorrectnessTest) {
  // From specification:
  //   Arc sine. Result is an angle, in radians, whose sine is x.
  //   The range of result values is [-π / 2, π / 2]. Result is undefined if abs
  //   x > 1.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Asin(0.5) = 0.523598776

  auto result = RunWithArgs(0.5f);
  EXPECT_TRUE(glsl::fuzzyEq(0.523598776f, result));
}

TEST_F(op_glsl_Asin_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Arc sine. Result is an angle, in radians, whose sine is x.
  //   The range of result values is [-π / 2, π / 2]. Result is undefined if abs
  //   x > 1.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Asin(<0.0, 1.0, -1.0, -0.0>) = <0.0, 1.570796327, -1.570796327, 0.0>

  auto result = RunWithArgs({0.0f, 1.0f, -1.0f, -0.0f});
  EXPECT_TRUE(glsl::fuzzyEq({0.0f, 1.570796327f, -1.570796327f, 0.0f}, result));
}

class op_glsl_Acos_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Acos_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Acos_float) {}
};

TEST_F(op_glsl_Acos_float, Smoke) { RunWithArgs(2); }

class op_glsl_Acos_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Acos_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Acos_vec2) {}
};

TEST_F(op_glsl_Acos_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Acos_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Acos_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Acos_vec3) {}
};

TEST_F(op_glsl_Acos_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Acos_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Acos_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Acos_vec4) {}
};

TEST_F(op_glsl_Acos_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Acos_float, BasicCorrectnessTest) {
  // From specification:
  //   Arc cosine. Result is an angle, in radians, whose cosine is x.
  //   The range of result values is [0, π]. Result is undefined if abs x > 1.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Acos(0.5) = 1.047197551

  auto result = RunWithArgs(0.5f);
  EXPECT_TRUE(glsl::fuzzyEq(1.047197551f, result));
}

TEST_F(op_glsl_Acos_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Arc cosine. Result is an angle, in radians, whose cosine is x.
  //   The range of result values is [0, π]. Result is undefined if abs x > 1.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Acos(<0.0, 1.0, -1.0, -0.0>) = <1.570796327, 0.0,
  //   3.141592654, 1.570796327>

  auto result = RunWithArgs({0.0f, 1.0f, -1.0f, -0.0f});
  EXPECT_TRUE(
      glsl::fuzzyEq({1.570796327f, 0.0f, 3.141592654f, 1.570796327f}, result));
}

class op_glsl_Atan_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Atan_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Atan_float) {}
};

TEST_F(op_glsl_Atan_float, Smoke) { RunWithArgs(2); }

class op_glsl_Atan_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Atan_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Atan_vec2) {}
};

TEST_F(op_glsl_Atan_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Atan_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Atan_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Atan_vec3) {}
};

TEST_F(op_glsl_Atan_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Atan_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Atan_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Atan_vec4) {}
};

TEST_F(op_glsl_Atan_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Atan_float, BasicCorrectnessTest) {
  // From specification:
  //   Arc tangent. Result is an angle, in radians, whose tangent is y_over_x.
  //   The range of result values is [-π, π].
  //
  //   The operand y_over_x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of y_over_x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Atan(4.5) = 1.352127381

  auto result = RunWithArgs(4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(1.352127381f, result));
}

TEST_F(op_glsl_Atan_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Arc tangent. Result is an angle, in radians, whose tangent is y_over_x.
  //   The range of result values is [-π, π].
  //
  //   The operand y_over_x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of y_over_x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Atan(<0.0, 1.0, -1.0, 1000.0>) = <0.0, 0.785398163,
  //   -0.785398163, 1.569796327>

  auto result = RunWithArgs({0.0f, 1.0f, -1.0f, 1000.0f});
  EXPECT_TRUE(
      glsl::fuzzyEq({0.0f, 0.785398163f, -0.785398163f, 1.569796327f}, result));
}

class op_glsl_Sinh_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Sinh_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Sinh_float) {}
};

TEST_F(op_glsl_Sinh_float, Smoke) { RunWithArgs(2); }

class op_glsl_Sinh_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Sinh_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Sinh_vec2) {}
};

TEST_F(op_glsl_Sinh_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Sinh_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Sinh_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Sinh_vec3) {}
};

TEST_F(op_glsl_Sinh_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Sinh_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Sinh_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Sinh_vec4) {}
};

TEST_F(op_glsl_Sinh_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Sinh_float, BasicCorrectnessTest) {
  // From specification:
  //   Hyperbolic sine of x radians.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Sinh(0.5) = 0.521095305

  auto result = RunWithArgs(0.5f);
  EXPECT_TRUE(glsl::fuzzyEq(0.521095305f, result));
}

TEST_F(op_glsl_Sinh_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Hyperbolic sine of x radians.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Sinh(<0.0f, 1.0f, 3.14159265359f, -1.570796327f>) =
  //     <0.0f, 1.175201194f, 11.548739357f, -2.301298903f>

  auto result = RunWithArgs({0.0f, 1.0f, 3.14159265359f, -1.570796327f});
  EXPECT_TRUE(glsl::fuzzyEq({0.0f, 1.175201194f, 11.548739357f, -2.301298903f},
                            result));
}

class op_glsl_Cosh_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Cosh_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Cosh_float) {}
};

TEST_F(op_glsl_Cosh_float, Smoke) { RunWithArgs(2); }

class op_glsl_Cosh_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Cosh_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Cosh_vec2) {}
};

TEST_F(op_glsl_Cosh_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Cosh_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Cosh_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Cosh_vec3) {}
};

TEST_F(op_glsl_Cosh_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Cosh_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Cosh_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Cosh_vec4) {}
};

TEST_F(op_glsl_Cosh_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Cosh_float, BasicCorrectnessTest) {
  // From specification:
  //   Hyperbolic cosine of x radians.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Cosh(0.5) = 1.127625965

  auto result = RunWithArgs(0.5f);
  EXPECT_TRUE(glsl::fuzzyEq(1.127625965f, result));
}

TEST_F(op_glsl_Cosh_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Hyperbolic sine of x radians.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Cosh(<0.0, 1.0, 3.14159265359, -1.570796327>) =
  //     <1.0, 1.543080635, 11.591953276, 2.509178479>

  auto result = RunWithArgs({0.0f, 1.0f, 3.14159265359f, -1.570796327f});
  EXPECT_TRUE(
      glsl::fuzzyEq({1.0f, 1.543080635f, 11.591953276f, 2.509178479f}, result));
}

class op_glsl_Tanh_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Tanh_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Tanh_float) {}
};

TEST_F(op_glsl_Tanh_float, Smoke) { RunWithArgs(2); }

class op_glsl_Tanh_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Tanh_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Tanh_vec2) {}
};

TEST_F(op_glsl_Tanh_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Tanh_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Tanh_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Tanh_vec3) {}
};

TEST_F(op_glsl_Tanh_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Tanh_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Tanh_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Tanh_vec4) {}
};

TEST_F(op_glsl_Tanh_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Tanh_float, BasicCorrectnessTest) {
  // From specification:
  //   Hyperbolic tangent of x radians.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Tanh(0.5) = 0.462117157

  auto result = RunWithArgs(0.5f);
  EXPECT_TRUE(glsl::fuzzyEq(0.462117157f, result));
}

TEST_F(op_glsl_Tanh_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Hyperbolic tangent of x radians.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Tanh(<0.0, 1.0, 3.14159265359, -1.570796327>) =
  //     <0.0, 0.761594156, 0.996272076, -0.917152336>

  auto result = RunWithArgs({0.0f, 1.0f, 3.14159265359f, -1.570796327f});
  EXPECT_TRUE(
      glsl::fuzzyEq({0.0f, 0.761594156f, 0.996272076f, -0.917152336f}, result));
}

class op_glsl_Asinh_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Asinh_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Asinh_float) {}
};

TEST_F(op_glsl_Asinh_float, Smoke) { RunWithArgs(2); }

class op_glsl_Asinh_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Asinh_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Asinh_vec2) {}
};

TEST_F(op_glsl_Asinh_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Asinh_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Asinh_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Asinh_vec3) {}
};

TEST_F(op_glsl_Asinh_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Asinh_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Asinh_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Asinh_vec4) {}
};

TEST_F(op_glsl_Asinh_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Asinh_float, BasicCorrectnessTest) {
  // From specification:
  //    Arc hyperbolic sine; result is the inverse of sinh.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Asinh(0.5) = 0.481211825

  auto result = RunWithArgs(0.5f);
  EXPECT_TRUE(glsl::fuzzyEq(0.481211825f, result));
}

TEST_F(op_glsl_Asinh_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Arc hyperbolic sine; result is the inverse of sinh.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Asinh(<0.0, 1.0, 3.14159265359, -4.712388981>) =
  //     <0.0, 0.881373587, 1.862295743, -2.254414593>

  auto result = RunWithArgs({0.0f, 1.0f, 3.14159265359f, -4.712388981f});
  EXPECT_TRUE(
      glsl::fuzzyEq({0.0f, 0.881373587f, 1.862295743f, -2.254414593f}, result));
}

class op_glsl_Acosh_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Acosh_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Acosh_float) {}
};

TEST_F(op_glsl_Acosh_float, Smoke) { RunWithArgs(2); }

class op_glsl_Acosh_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Acosh_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Acosh_vec2) {}
};

TEST_F(op_glsl_Acosh_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Acosh_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Acosh_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Acosh_vec3) {}
};

TEST_F(op_glsl_Acosh_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Acosh_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Acosh_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Acosh_vec4) {}
};

TEST_F(op_glsl_Acosh_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Acosh_float, BasicCorrectnessTest) {
  // From specification:
  //   Arc hyperbolic cosine; Result is the non-negative inverse of cosh.
  //   Result is undefined if x < 1.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Acosh(4.5) = 2.184643792

  auto result = RunWithArgs(4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(2.184643792f, result));
}

TEST_F(op_glsl_Acosh_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Arc hyperbolic cosine; Result is the non-negative inverse of cosh.
  //   Result is undefined if x < 1.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Acosh(<9.2, 1.0, 3.14159265359, 4.712388981>) =
  //     <2.909383805, 0.0, 1.811526272, 2.231889253>

  auto result = RunWithArgs({9.2f, 1.0f, 3.14159265359f, 4.712388981f});
  EXPECT_TRUE(
      glsl::fuzzyEq({2.909383805f, 0.0f, 1.811526272f, 2.231889253f}, result));
}

class op_glsl_Atanh_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Atanh_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Atanh_float) {}
};

TEST_F(op_glsl_Atanh_float, Smoke) { RunWithArgs(2); }

class op_glsl_Atanh_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Atanh_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Atanh_vec2) {}
};

TEST_F(op_glsl_Atanh_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Atanh_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Atanh_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Atanh_vec3) {}
};

TEST_F(op_glsl_Atanh_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Atanh_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Atanh_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Atanh_vec4) {}
};

TEST_F(op_glsl_Atanh_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Atanh_float, BasicCorrectnessTest) {
  // From specification:
  //   Arc hyperbolic tangent; result is the inverse of tanh. Result is
  //   undefined if abs x ≥ 1.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Atanh(0.5) = 0.549306144

  auto result = RunWithArgs(0.5f);
  EXPECT_TRUE(glsl::fuzzyEq(0.549306144f, result));
}

TEST_F(op_glsl_Atanh_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Arc hyperbolic tangent; result is the inverse of tanh. Result is
  //   undefined if abs x ≥ 1.
  //
  //   The operand x must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of x must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Atanh(<0.0f, 0.99f, 0.25f, 0.001f>) =
  //     <0.0, 2.646652412, 0.255412812, 0.001>

  auto result = RunWithArgs({0.0f, 0.99f, 0.25f, 0.001f});
  EXPECT_TRUE(
      glsl::fuzzyEq({0.0f, 2.646652412f, 0.255412812f, 0.001f}, result));
}

class op_glsl_Atan2_float_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Atan2_float_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Atan2_float_float) {}
};

TEST_F(op_glsl_Atan2_float_float, Smoke) { RunWithArgs(2, 2); }

class op_glsl_Atan2_vec2_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Atan2_vec2_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Atan2_vec2_vec2) {}
};

TEST_F(op_glsl_Atan2_vec2_vec2, Smoke) { RunWithArgs({2, 2}, {2, 2}); }

class op_glsl_Atan2_vec3_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Atan2_vec3_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Atan2_vec3_vec3) {}
};

TEST_F(op_glsl_Atan2_vec3_vec3, Smoke) { RunWithArgs({2, 2, 2}, {2, 2, 2}); }

class op_glsl_Atan2_vec4_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Atan2_vec4_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Atan2_vec4_vec4) {}
};

TEST_F(op_glsl_Atan2_vec4_vec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
}

TEST_F(op_glsl_Atan2_float_float, BasicCorrectnessTest) {
  // From specification:
  //   Arc tangent. Result is an angle, in radians, whose tangent is y / x.
  //   The signs of x and y are used to determine what quadrant the angle is in.
  //   The range of result values is [-π, π] . Result is undefined if x and y
  //   are both 0.
  //
  //   The operand x and y must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Atan2(2.3, 4.5) = 0.472496935517

  auto result = RunWithArgs(2.3f, 4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(0.472496935517f, result));
}

TEST_F(op_glsl_Atan2_vec4_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Arc tangent. Result is an angle, in radians, whose tangent is y / x.
  //   The signs of x and y are used to determine what quadrant the angle is in.
  //   The range of result values is [-π, π] . Result is undefined if x and y
  //   are both 0.
  //
  //   The operand x and y must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Atan2(<0.0f, -0.99f, 50.25f, -5.45f>, <0.5, 0.99, 0.001, -2.23>) =
  //     <0.0, -0.785398163397, 1.570776426297, -1.959186488848>

  auto result = RunWithArgs({0.0f, -0.99f, 50.25f, -5.45f},
                            {0.5f, 0.99f, 0.001f, -2.23f});
  EXPECT_TRUE(glsl::fuzzyEq(
      {0.0f, -0.785398163397f, 1.570776426297f, -1.959186488848f}, result));
}
