// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "GLSLTestDefs.h"

class op_glsl_Radians_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Radians_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Radians_float) {}
};

TEST_F(op_glsl_Radians_float, Smoke) { RunWithArgs(2); }

class op_glsl_Radians_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Radians_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Radians_vec2) {}
};

TEST_F(op_glsl_Radians_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Radians_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Radians_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Radians_vec3) {}
};

TEST_F(op_glsl_Radians_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Radians_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Radians_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Radians_vec4) {}
};

TEST_F(op_glsl_Radians_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Radians_float, BasicCorrectnessTest) {
  // From specification:
  //   Converts degrees to radians, i.e., degrees * π / 180.
  //
  //   The operand degrees must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of degrees must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Radians(4.5) = 0.078539816

  auto result = RunWithArgs(4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(0.078539816f, result));
}

TEST_F(op_glsl_Radians_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Converts degrees to radians, i.e., degrees * π / 180.
  //
  //   The operand degrees must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of degrees must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Radians(<0.0, 180.0, -360.0, 535.0>) = <0.0, 3.14159265359,
  //   -6.283185307, 9.337511498>

  auto result = RunWithArgs({0.0f, 180.0f, -360.0f, 535.0f});
  EXPECT_TRUE(glsl::fuzzyEq({0.0f, 3.14159265359f, -6.283185307f, 9.337511498f},
                            result));
}

class op_glsl_Degrees_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Degrees_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Degrees_float) {}
};

TEST_F(op_glsl_Degrees_float, Smoke) { RunWithArgs(2); }

class op_glsl_Degrees_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Degrees_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Degrees_vec2) {}
};

TEST_F(op_glsl_Degrees_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Degrees_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Degrees_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Degrees_vec3) {}
};

TEST_F(op_glsl_Degrees_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Degrees_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Degrees_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Degrees_vec4) {}
};

TEST_F(op_glsl_Degrees_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_Degrees_float, BasicCorrectnessTest) {
  // From specification:
  //   Converts radians to degrees, i.e., radians * 180 / π.
  //
  //   The operand radians must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of radians must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Degrees(3.14159265359) = 0.078539816

  auto result = RunWithArgs(3.14159265359f);
  EXPECT_TRUE(glsl::fuzzyEq(180.0f, result));
}

TEST_F(op_glsl_Degrees_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Converts radians to degrees, i.e., radians * 180 / π.
  //
  //   The operand radians must be a scalar or vector
  //   whose component type is 16-bit or 32-bit floating-point.
  //
  //   Result Type and the type of radians must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Degrees(<0.0f, 2.25f, 12.56f, -0.2>) = <0.0, 128.915503904,
  //   719.634990684, -11.459155903>

  auto result = RunWithArgs({0.0f, 2.25f, 12.56f, -0.2f});
  EXPECT_TRUE(glsl::fuzzyEq(
      {0.0f, 128.915503904f, 719.634990684f, -11.459155903f}, result));
}
