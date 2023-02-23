// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <limits>
#include "GLSLTestDefs.h"

constexpr glsl::intTy I_MIN = std::numeric_limits<glsl::intTy>::min();
constexpr glsl::intTy I_MAX = std::numeric_limits<glsl::intTy>::max();

class op_glsl_FAbs_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_FAbs_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_FAbs_float) {}
};

TEST_F(op_glsl_FAbs_float, Smoke) { RunWithArgs(2); }

class op_glsl_FAbs_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_FAbs_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_FAbs_vec2) {}
};

TEST_F(op_glsl_FAbs_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_FAbs_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_FAbs_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_FAbs_vec3) {}
};

TEST_F(op_glsl_FAbs_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_FAbs_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_FAbs_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_FAbs_vec4) {}
};

TEST_F(op_glsl_FAbs_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

class op_glsl_FAbs_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_FAbs_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_FAbs_double) {}
};

TEST_F(op_glsl_FAbs_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2);
  }
}

class op_glsl_FAbs_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_FAbs_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_FAbs_dvec2) {}
};

TEST_F(op_glsl_FAbs_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2});
  }
}

class op_glsl_FAbs_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_FAbs_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_FAbs_dvec3) {}
};

TEST_F(op_glsl_FAbs_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2});
  }
}

class op_glsl_FAbs_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_FAbs_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_FAbs_dvec4) {}
};

TEST_F(op_glsl_FAbs_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2});
  }
}

TEST_F(op_glsl_FAbs_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is x if x ≥ 0; otherwise result is -x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   FAbs(4.5) = 4.5

  auto result = RunWithArgs(4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(4.5f, result));
}

TEST_F(op_glsl_FAbs_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is x if x ≥ 0; otherwise result is -x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   FAbs(<0.0, -0.0, 0.1, -0.01>) = <0.0, 0.0, 0.1, 0.01>

  auto result = RunWithArgs({0.0f, -0.0f, 0.1f, -0.01f});
  EXPECT_TRUE(glsl::fuzzyEq({0.0f, 0.0f, 0.1f, 0.01f}, result));
}

TEST_F(op_glsl_FAbs_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is x if x ≥ 0; otherwise result is -x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   FAbs(-10000.5) = 10000.5
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(-10000.5);
    EXPECT_TRUE(glsl::fuzzyEq(10000.5, result));
  }
}

TEST_F(op_glsl_FAbs_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is x if x ≥ 0; otherwise result is -x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   FAbs(<-0.0, -10000000.99, 0.5, -4.5>) = <0.0, 10000000.99, 0.5, 4.5>
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({-0.0, -10000000.99, 0.5, -4.5});
    EXPECT_TRUE(glsl::fuzzyEq({0.0, 10000000.99, 0.5, 4.5}, result));
  }
}

class op_glsl_SAbs_int : public GlslBuiltinTest<glsl::intTy, glsl::intTy> {
 public:
  op_glsl_SAbs_int()
      : GlslBuiltinTest<glsl::intTy, glsl::intTy>(
            uvk::Shader::op_glsl_SAbs_int) {}
};

TEST_F(op_glsl_SAbs_int, Smoke) { RunWithArgs(2); }

class op_glsl_SAbs_ivec2
    : public GlslBuiltinTest<glsl::ivec2Ty, glsl::ivec2Ty> {
 public:
  op_glsl_SAbs_ivec2()
      : GlslBuiltinTest<glsl::ivec2Ty, glsl::ivec2Ty>(
            uvk::Shader::op_glsl_SAbs_ivec2) {}
};

TEST_F(op_glsl_SAbs_ivec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_SAbs_ivec3
    : public GlslBuiltinTest<glsl::ivec3Ty, glsl::ivec3Ty> {
 public:
  op_glsl_SAbs_ivec3()
      : GlslBuiltinTest<glsl::ivec3Ty, glsl::ivec3Ty>(
            uvk::Shader::op_glsl_SAbs_ivec3) {}
};

TEST_F(op_glsl_SAbs_ivec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_SAbs_ivec4
    : public GlslBuiltinTest<glsl::ivec4Ty, glsl::ivec4Ty> {
 public:
  op_glsl_SAbs_ivec4()
      : GlslBuiltinTest<glsl::ivec4Ty, glsl::ivec4Ty>(
            uvk::Shader::op_glsl_SAbs_ivec4) {}
};

TEST_F(op_glsl_SAbs_ivec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_SAbs_int, BasicCorrectnessTest) {
  // From specification:
  //   Result is x if x ≥ 0; otherwise result is -x, where x is interpreted as a
  //   signed integer.
  //
  //   Result Type and the type of x must both be integer scalar or integer
  //   vector types. Result Type and operand types must have the same number of
  //   components with the same component width. Results are computed per
  //   component.
  // Expected results:
  //   SAbs(-1) = 1

  auto result = RunWithArgs(-1);
  EXPECT_EQ(1, result);
}

TEST_F(op_glsl_SAbs_ivec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is x if x ≥ 0; otherwise result is -x, where x is interpreted as a
  //   signed integer.
  //
  //   Result Type and the type of x must both be integer scalar or integer
  //   vector types. Result Type and operand types must have the same number of
  //   components with the same component width. Results are computed per
  //   component.
  // Expected results:
  //   SAbs(<20, 2 147 483 647, -2 147 483 647, 0>) = <20, 2 147 483 647,
  //   2 147 483 647, 0>

  auto result = RunWithArgs({20, I_MAX, I_MIN + 1, 0});
  EXPECT_EQ(result, glsl::ivec4Ty(20, I_MAX, I_MAX, 0));
}
