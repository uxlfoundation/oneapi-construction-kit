// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "GLSLTestDefs.h"

class op_glsl_Cross_vec3_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Cross_vec3_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Cross_vec3_vec3) {}
};

TEST_F(op_glsl_Cross_vec3_vec3, Smoke) { RunWithArgs({2, 2, 2}, {2, 2, 2}); }

class op_glsl_Cross_dvec3_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_Cross_dvec3_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_Cross_dvec3_dvec3) {}
};

TEST_F(op_glsl_Cross_dvec3_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2}, {2, 2, 2});
  }
}

TEST_F(op_glsl_Cross_vec3_vec3, BasicCorrectnessTest) {
  // From specification:
  //   Result is the cross product of x and y, i.e., the resulting components
  //   are, in order:
  //
  //   x[1] * y[2] - y[1] * x[2]
  //
  //   x[2] * y[0] - y[2] * x[0]
  //
  //   x[0] * y[1] - y[0] * x[1]
  //
  //   All the operands must be vectors of 3 components of a floating-point
  //   type.
  //
  //   Result Type and the type of all operands must be the same type.
  // Expected results:
  //   Cross(<0.0, -0.99, 50.25>, <0.5, 5.99, 0.001>) =
  //     <-300.99849, 25.125, 0.495>

  auto result = RunWithArgs({0.0f, -0.99f, 50.25f}, {0.5f, 5.99f, 0.001f});
  EXPECT_TRUE(glsl::fuzzyEq({-300.99849f, 25.125f, 0.495f}, result));
}

TEST_F(op_glsl_Cross_dvec3_dvec3, BasicCorrectnessTest) {
  // From specification:
  //   Result is the cross product of x and y, i.e., the resulting components
  //   are, in order:
  //
  //   x[1] * y[2] - y[1] * x[2]
  //
  //   x[2] * y[0] - y[2] * x[0]
  //
  //   x[0] * y[1] - y[0] * x[1]
  //
  //   All the operands must be vectors of 3 components of a floating-point
  //   type.
  //
  //   Result Type and the type of all operands must be the same type.
  // Expected results:
  //   Cross(<6.23, -8.0005, 0.0>, <0.5, 0.0, 10000.001>) =
  //     <-80005.00800, -62300.00623, 4.00025>

  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({6.23, -8.0005, 0.0}, {0.5, 0.0, 10000.001});
    EXPECT_TRUE(glsl::fuzzyEq({-80005.00800, -62300.00623, 4.00025}, result));
  }
}
