// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "GLSLTestDefs.h"

class op_glsl_Step_float_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Step_float_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Step_float_float) {}
};

TEST_F(op_glsl_Step_float_float, Smoke) { RunWithArgs(2, 2); }

class op_glsl_Step_vec2_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Step_vec2_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Step_vec2_vec2) {}
};

TEST_F(op_glsl_Step_vec2_vec2, Smoke) { RunWithArgs({2, 2}, {2, 2}); }

class op_glsl_Step_vec3_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Step_vec3_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Step_vec3_vec3) {}
};

TEST_F(op_glsl_Step_vec3_vec3, Smoke) { RunWithArgs({2, 2, 2}, {2, 2, 2}); }

class op_glsl_Step_vec4_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Step_vec4_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Step_vec4_vec4) {}
};

TEST_F(op_glsl_Step_vec4_vec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
}

class op_glsl_Step_double_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_Step_double_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_Step_double_double) {}
};

TEST_F(op_glsl_Step_double_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2, 2);
  }
}

class op_glsl_Step_dvec2_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_Step_dvec2_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_Step_dvec2_dvec2) {}
};

TEST_F(op_glsl_Step_dvec2_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2}, {2, 2});
  }
}

class op_glsl_Step_dvec3_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_Step_dvec3_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_Step_dvec3_dvec3) {}
};

TEST_F(op_glsl_Step_dvec3_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2}, {2, 2, 2});
  }
}

class op_glsl_Step_dvec4_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_Step_dvec4_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_Step_dvec4_dvec4) {}
};

TEST_F(op_glsl_Step_dvec4_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
  }
}

TEST_F(op_glsl_Step_float_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is 0.0 if x < edge; otherwise result is 1.0.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Step(2.3, 4.5) = 1.0

  auto result = RunWithArgs(2.3f, 4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(1.0f, result));
}

TEST_F(op_glsl_Step_vec4_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is 0.0 if x < edge; otherwise result is 1.0.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Step(<0.0, -5.5, -5.5, 100.0>, <0.0, 0.0, -6.0, 99.0>) =
  //     <1.0, 1.0, 0.0, 0.0>

  auto result =
      RunWithArgs({0.0f, -5.5f, -5.5f, 100.0f}, {0.0f, 0.0f, -6.0f, 99.0f});
  EXPECT_TRUE(glsl::fuzzyEq({1.0f, 1.0f, 0.0f, 0.0f}, result));
}

TEST_F(op_glsl_Step_double_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is 0.0 if x < edge; otherwise result is 1.0.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Step(2.3, 0.001) = 0.0
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(2.3, 0.001);
    EXPECT_TRUE(glsl::fuzzyEq(0.0, result));
  }
}

TEST_F(op_glsl_Step_dvec4_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is 0.0 if x < edge; otherwise result is 1.0.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Step(<0.499, -0.99, 0.0, -5.45>, <0.5, -0.99, 0.001, 2.23>) =
  //     <1.0, 1.0, 1.0, 1.0>
  if (deviceFeatures.shaderFloat64) {
    auto result =
        RunWithArgs({0.499, -0.99, 0.0, -5.45}, {0.5, -0.99, 0.001, 2.23});
    EXPECT_TRUE(glsl::fuzzyEq({1.0, 1.0, 1.0, 1.0}, result));
  }
}

class op_glsl_SmoothStep_float_float_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy,
                             glsl::floatTy> {
 public:
  op_glsl_SmoothStep_float_float_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy,
                        glsl::floatTy>(
            uvk::Shader::op_glsl_SmoothStep_float_float_float) {}
};

TEST_F(op_glsl_SmoothStep_float_float_float, Smoke) { RunWithArgs(2, 2, 2); }

class op_glsl_SmoothStep_vec2_vec2_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty,
                             glsl::vec2Ty> {
 public:
  op_glsl_SmoothStep_vec2_vec2_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_SmoothStep_vec2_vec2_vec2) {}
};

TEST_F(op_glsl_SmoothStep_vec2_vec2_vec2, Smoke) {
  RunWithArgs({2, 2}, {2, 2}, {2, 2});
}

class op_glsl_SmoothStep_vec3_vec3_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty,
                             glsl::vec3Ty> {
 public:
  op_glsl_SmoothStep_vec3_vec3_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_SmoothStep_vec3_vec3_vec3) {}
};

TEST_F(op_glsl_SmoothStep_vec3_vec3_vec3, Smoke) {
  RunWithArgs({2, 2, 2}, {2, 2, 2}, {2, 2, 2});
}

class op_glsl_SmoothStep_vec4_vec4_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty,
                             glsl::vec4Ty> {
 public:
  op_glsl_SmoothStep_vec4_vec4_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_SmoothStep_vec4_vec4_vec4) {}
};

TEST_F(op_glsl_SmoothStep_vec4_vec4_vec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2});
}

class op_glsl_SmoothStep_double_double_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy,
                             glsl::doubleTy> {
 public:
  op_glsl_SmoothStep_double_double_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy,
                        glsl::doubleTy>(
            uvk::Shader::op_glsl_SmoothStep_double_double_double) {}
};

TEST_F(op_glsl_SmoothStep_double_double_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2, 2, 2);
  }
}

class op_glsl_SmoothStep_dvec2_dvec2_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty,
                             glsl::dvec2Ty> {
 public:
  op_glsl_SmoothStep_dvec2_dvec2_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty,
                        glsl::dvec2Ty>(
            uvk::Shader::op_glsl_SmoothStep_dvec2_dvec2_dvec2) {}
};

TEST_F(op_glsl_SmoothStep_dvec2_dvec2_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2}, {2, 2}, {2, 2});
  }
}

class op_glsl_SmoothStep_dvec3_dvec3_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty,
                             glsl::dvec3Ty> {
 public:
  op_glsl_SmoothStep_dvec3_dvec3_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty,
                        glsl::dvec3Ty>(
            uvk::Shader::op_glsl_SmoothStep_dvec3_dvec3_dvec3) {}
};

TEST_F(op_glsl_SmoothStep_dvec3_dvec3_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2}, {2, 2, 2}, {2, 2, 2});
  }
}

class op_glsl_SmoothStep_dvec4_dvec4_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty,
                             glsl::dvec4Ty> {
 public:
  op_glsl_SmoothStep_dvec4_dvec4_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty,
                        glsl::dvec4Ty>(
            uvk::Shader::op_glsl_SmoothStep_dvec4_dvec4_dvec4) {}
};

TEST_F(op_glsl_SmoothStep_dvec4_dvec4_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2});
  }
}

TEST_F(op_glsl_SmoothStep_float_float_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is 0.0 if x ≤ edge0 and 1.0 if x ≥ edge1 and performs smooth
  //   Hermite interpolation between 0 and 1 when edge0 < x < edge1. This is
  //   equivalent to:
  //
  //   t * t * (3 - 2 * t), where t = clamp ((x - edge0) / (edge1 - edge0), 0,
  //   1)
  //
  //   Result is undefined if edge0 ≥ edge1.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   SmoothStep(2.3, 4.5, 3.3) = 0.432006011

  auto result = RunWithArgs(2.3f, 4.5f, 3.3f);
  EXPECT_TRUE(glsl::fuzzyEq(0.432006011f, result));
}

TEST_F(op_glsl_SmoothStep_vec4_vec4_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is 0.0 if x ≤ edge0 and 1.0 if x ≥ edge1 and performs smooth
  //   Hermite interpolation between 0 and 1 when edge0 < x < edge1. This is
  //   equivalent to:
  //
  //   t * t * (3 - 2 * t), where t = clamp ((x - edge0) / (edge1 - edge0), 0,
  //   1)
  //
  //   Result is undefined if edge0 ≥ edge1.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   SmoothStep(<0.0, 0.0, -1.0, 5.0>, <0.5, 0.5, 0.0, 99.0>, <0.0, 0.5, -0.5,
  //   101.0>)
  //     = <0.0, 1.0, 0.5, 0.0>

  auto result =
      RunWithArgs({0.0f, 0.0f, -1.0f, 5.0f}, {0.5f, 0.5f, 0.0f, 99.0f},
                  {0.0f, 0.5f, -0.5f, 101.0f});
  EXPECT_TRUE(glsl::fuzzyEq({0.0f, 1.0f, 0.5f, 1.0f}, result));
}

TEST_F(op_glsl_SmoothStep_double_double_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is 0.0 if x ≤ edge0 and 1.0 if x ≥ edge1 and performs smooth
  //   Hermite interpolation between 0 and 1 when edge0 < x < edge1. This is
  //   equivalent to:
  //
  //   t * t * (3 - 2 * t), where t = clamp ((x - edge0) / (edge1 - edge0), 0,
  //   1)
  //
  //   Result is undefined if edge0 ≥ edge1.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   SmoothStep(2.3, 4.5, 3.3) = 0.432006011
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(2.3, 4.5, 3.3);
    EXPECT_TRUE(glsl::fuzzyEq(0.432006011, result));
  }
}

TEST_F(op_glsl_SmoothStep_dvec4_dvec4_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is 0.0 if x ≤ edge0 and 1.0 if x ≥ edge1 and performs smooth
  //   Hermite interpolation between 0 and 1 when edge0 < x < edge1. This is
  //   equivalent to:
  //
  //   t * t * (3 - 2 * t), where t = clamp ((x - edge0) / (edge1 - edge0), 0,
  //   1)
  //
  //   Result is undefined if edge0 ≥ edge1.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   SmoothStep(<0.0, 0.0, -1.0, 5.0>, <0.5, 0.5, 0.0, 99.0>, <0.499, 0.5,
  //   -0.5, 4.0>)
  //     = <0.999988016, 1.0, 0.5, 0.0>
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({0.0, 0.0, -1.0, 5.0}, {0.5, 0.5, 0.0, 99.0},
                              {0.499, 0.5, -0.5, 4.0});
    EXPECT_TRUE(glsl::fuzzyEq({0.999988016, 1.0, 0.5, 0.0}, result));
  }
}
