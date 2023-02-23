// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "GLSLTestDefs.h"

class op_glsl_Sqrt_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Sqrt_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Sqrt_float) {}
};

TEST_F(op_glsl_Sqrt_float, Smoke) { RunWithArgs(2); }

class op_glsl_Sqrt_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Sqrt_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Sqrt_vec2) {}
};

TEST_F(op_glsl_Sqrt_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Sqrt_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Sqrt_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Sqrt_vec3) {}
};

TEST_F(op_glsl_Sqrt_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Sqrt_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Sqrt_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Sqrt_vec4) {}
};

TEST_F(op_glsl_Sqrt_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

class op_glsl_Sqrt_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_Sqrt_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_Sqrt_double) {}
};

TEST_F(op_glsl_Sqrt_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2);
  }
}

class op_glsl_Sqrt_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_Sqrt_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_Sqrt_dvec2) {}
};

TEST_F(op_glsl_Sqrt_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2});
  }
}

class op_glsl_Sqrt_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_Sqrt_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_Sqrt_dvec3) {}
};

TEST_F(op_glsl_Sqrt_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2});
  }
}

class op_glsl_Sqrt_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_Sqrt_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_Sqrt_dvec4) {}
};

TEST_F(op_glsl_Sqrt_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2});
  }
}

TEST_F(op_glsl_Sqrt_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is the square root of x. Result is undefined if x < 0.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Sqrt(4.5) = 2.121320344

  auto result = RunWithArgs(4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(2.121320344f, result));
}

TEST_F(op_glsl_Sqrt_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the square root of x. Result is undefined if x < 0.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Sqrt(<1.0, 0.99, 10000.0, 0.0>) = <1.0, 0.994987437, 100.0, 0.0>

  auto result = RunWithArgs({1.0f, 0.99f, 10000.0f, 0.0f});
  EXPECT_TRUE(glsl::fuzzyEq({1.0f, 0.994987437f, 100.0f, 0.0f}, result));
}

TEST_F(op_glsl_Sqrt_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is the square root of x. Result is undefined if x < 0.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Sqrt(4.5) = 2.121320344
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(4.5);
    EXPECT_TRUE(glsl::fuzzyEq(2.121320344, result));
  }
}

TEST_F(op_glsl_Sqrt_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the square root of x. Result is undefined if x < 0.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Sqrt(<100.0, 0.125, 0.5, -0.0>) = <10.0, 0.353553391, 0.707106781, 0.0>
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({100.0, 0.125, 0.5, -0.0});
    EXPECT_TRUE(glsl::fuzzyEq({10.0, 0.353553391, 0.707106781, 0.0}, result));
  }
}

class op_glsl_InverseSqrt_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_InverseSqrt_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_InverseSqrt_float) {}
};

TEST_F(op_glsl_InverseSqrt_float, Smoke) { RunWithArgs(2); }

class op_glsl_InverseSqrt_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_InverseSqrt_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_InverseSqrt_vec2) {}
};

TEST_F(op_glsl_InverseSqrt_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_InverseSqrt_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_InverseSqrt_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_InverseSqrt_vec3) {}
};

TEST_F(op_glsl_InverseSqrt_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_InverseSqrt_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_InverseSqrt_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_InverseSqrt_vec4) {}
};

TEST_F(op_glsl_InverseSqrt_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

class op_glsl_InverseSqrt_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_InverseSqrt_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_InverseSqrt_double) {}
};

TEST_F(op_glsl_InverseSqrt_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2);
  }
}

class op_glsl_InverseSqrt_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_InverseSqrt_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_InverseSqrt_dvec2) {}
};

TEST_F(op_glsl_InverseSqrt_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2});
  }
}

class op_glsl_InverseSqrt_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_InverseSqrt_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_InverseSqrt_dvec3) {}
};

TEST_F(op_glsl_InverseSqrt_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2});
  }
}

class op_glsl_InverseSqrt_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_InverseSqrt_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_InverseSqrt_dvec4) {}
};

TEST_F(op_glsl_InverseSqrt_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2});
  }
}

TEST_F(op_glsl_InverseSqrt_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is the reciprocal of sqrt x. Result is undefined if x ≤ 0.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   InverseSqrt(4.5) = 0.471404521

  auto result = RunWithArgs(4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(0.471404521f, result));
}

TEST_F(op_glsl_InverseSqrt_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the reciprocal of sqrt x. Result is undefined if x ≤ 0.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   InverseSqrt(<1.0, 0.99, 10000.0, 0.01>) = <1.0, 1.005037815, 0.01, 10.0>

  auto result = RunWithArgs({1.0f, 0.99f, 10000.0f, 0.01f});
  EXPECT_TRUE(glsl::fuzzyEq({1.0f, 1.005037815f, 0.01f, 10.0f}, result));
}

TEST_F(op_glsl_InverseSqrt_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is the reciprocal of sqrt x. Result is undefined if x ≤ 0.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   InverseSqrt(4.5) = 0.471404521
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(4.5);
    EXPECT_TRUE(glsl::fuzzyEq(0.471404521, result));
  }
}

TEST_F(op_glsl_InverseSqrt_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the reciprocal of sqrt x. Result is undefined if x ≤ 0.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   InverseSqrt(<100.0, 0.125, 0.5, 0.01>) = <0.1, 2.828427121,
  //   1.414213563, 10.0>
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({100.0, 0.125, 0.5, 0.01});
    EXPECT_TRUE(glsl::fuzzyEq({0.1, 2.828427121, 1.414213563, 10.0}, result));
  }
}
