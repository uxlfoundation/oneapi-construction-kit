// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "GLSLTestDefs.h"

class op_glsl_FMix_float_float_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy,
                             glsl::floatTy> {
 public:
  op_glsl_FMix_float_float_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy,
                        glsl::floatTy>(
            uvk::Shader::op_glsl_FMix_float_float_float) {}
};

TEST_F(op_glsl_FMix_float_float_float, Smoke) { RunWithArgs(2, 2, 2); }

class op_glsl_FMix_vec2_vec2_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty,
                             glsl::vec2Ty> {
 public:
  op_glsl_FMix_vec2_vec2_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_FMix_vec2_vec2_vec2) {}
};

TEST_F(op_glsl_FMix_vec2_vec2_vec2, Smoke) {
  RunWithArgs({2, 2}, {2, 2}, {2, 2});
}

class op_glsl_FMix_vec3_vec3_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty,
                             glsl::vec3Ty> {
 public:
  op_glsl_FMix_vec3_vec3_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_FMix_vec3_vec3_vec3) {}
};

TEST_F(op_glsl_FMix_vec3_vec3_vec3, Smoke) {
  RunWithArgs({2, 2, 2}, {2, 2, 2}, {2, 2, 2});
}

class op_glsl_FMix_vec4_vec4_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty,
                             glsl::vec4Ty> {
 public:
  op_glsl_FMix_vec4_vec4_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_FMix_vec4_vec4_vec4) {}
};

TEST_F(op_glsl_FMix_vec4_vec4_vec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2});
}

class op_glsl_FMix_double_double_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy,
                             glsl::doubleTy> {
 public:
  op_glsl_FMix_double_double_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy,
                        glsl::doubleTy>(
            uvk::Shader::op_glsl_FMix_double_double_double) {}
};

TEST_F(op_glsl_FMix_double_double_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2, 2, 2);
  }
}

class op_glsl_FMix_dvec2_dvec2_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty,
                             glsl::dvec2Ty> {
 public:
  op_glsl_FMix_dvec2_dvec2_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty,
                        glsl::dvec2Ty>(
            uvk::Shader::op_glsl_FMix_dvec2_dvec2_dvec2) {}
};

TEST_F(op_glsl_FMix_dvec2_dvec2_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2}, {2, 2}, {2, 2});
  }
}

class op_glsl_FMix_dvec3_dvec3_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty,
                             glsl::dvec3Ty> {
 public:
  op_glsl_FMix_dvec3_dvec3_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty,
                        glsl::dvec3Ty>(
            uvk::Shader::op_glsl_FMix_dvec3_dvec3_dvec3) {}
};

TEST_F(op_glsl_FMix_dvec3_dvec3_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2}, {2, 2, 2}, {2, 2, 2});
  }
}

class op_glsl_FMix_dvec4_dvec4_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty,
                             glsl::dvec4Ty> {
 public:
  op_glsl_FMix_dvec4_dvec4_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty,
                        glsl::dvec4Ty>(
            uvk::Shader::op_glsl_FMix_dvec4_dvec4_dvec4) {}
};

TEST_F(op_glsl_FMix_dvec4_dvec4_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2});
  }
}

TEST_F(op_glsl_FMix_float_float_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is the linear blend of x and y, i.e., x * (1 - a) + y * a.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   FMix(2.3, 4.5, 8.3) = 20.56

  auto result = RunWithArgs(2.3f, 4.5f, 8.3f);
  EXPECT_TRUE(glsl::fuzzyEq(20.56f, result));
}

TEST_F(op_glsl_FMix_vec4_vec4_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the linear blend of x and y, i.e., x * (1 - a) + y * a.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   FMix(<0.0, -0.99, 50.25, -5.45>, <0.5, 0.99, 0.001, -2.23>,
  //          <0.8, 2.02, 25.0f, 0.0>)
  //        = <0.4, 0.99, -1207.005, -5.45>

  auto result =
      RunWithArgs({0.0f, -0.99f, 50.25f, -5.45f}, {0.5f, 0.99f, 0.0f, -2.23f},
                  {0.8f, 1.0f, 25.02f, 0.0f});
  EXPECT_TRUE(glsl::fuzzyEq({0.4f, 0.99f, -1207.005f, -5.45f}, result));
}

TEST_F(op_glsl_FMix_double_double_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is the linear blend of x and y, i.e., x * (1 - a) + y * a.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   FMix(2.3, 4.5, 8.3) = 3.4
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(2.3, 4.5, 0.5);
    EXPECT_TRUE(glsl::fuzzyEq(3.4, result));
  }
}

TEST_F(op_glsl_FMix_dvec4_dvec4_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the linear blend of x and y, i.e., x * (1 - a) + y * a.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   FMix(<1.0, -0.99, 50.25, -5.45>, <0.5, 0.99, 0.001, -2.23>,
  //          <0.8, 2.02, 25.0f, 0.0>)
  //        = <0.4, 0.792, -1207.005, 0.0>
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({0.0, -0.99, 50.25, 0.0}, {0.5, 0.99, 0.0, -2.23},
                              {0.8, 0.9, 25.02, 0.0});
    EXPECT_TRUE(glsl::fuzzyEq({0.4, 0.792, -1207.005, 0.0}, result));
  }
}
