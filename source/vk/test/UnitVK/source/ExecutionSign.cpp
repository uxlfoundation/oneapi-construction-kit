// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <limits>
#include "GLSLTestDefs.h"

constexpr glsl::intTy I_MIN = std::numeric_limits<glsl::intTy>::min();
constexpr glsl::intTy I_MAX = std::numeric_limits<glsl::intTy>::max();

class op_glsl_FSign_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_FSign_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_FSign_float) {}
};

TEST_F(op_glsl_FSign_float, Smoke) { RunWithArgs(2); }

class op_glsl_FSign_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_FSign_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_FSign_vec2) {}
};

TEST_F(op_glsl_FSign_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_FSign_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_FSign_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_FSign_vec3) {}
};

TEST_F(op_glsl_FSign_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_FSign_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_FSign_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_FSign_vec4) {}
};

TEST_F(op_glsl_FSign_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

class op_glsl_FSign_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_FSign_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_FSign_double) {}
};

TEST_F(op_glsl_FSign_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2);
  }
}

class op_glsl_FSign_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_FSign_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_FSign_dvec2) {}
};

TEST_F(op_glsl_FSign_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2});
  }
}

class op_glsl_FSign_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_FSign_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_FSign_dvec3) {}
};

TEST_F(op_glsl_FSign_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2});
  }
}

class op_glsl_FSign_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_FSign_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_FSign_dvec4) {}
};

TEST_F(op_glsl_FSign_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2});
  }
}

TEST_F(op_glsl_FSign_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is 1.0 if x > 0, 0.0 if x = 0, or -1.0 if x < 0.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   FSign(4.5) = 1.0

  auto result = RunWithArgs(4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(1.0f, result));
}

TEST_F(op_glsl_FSign_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is 1.0 if x > 0, 0.0 if x = 0, or -1.0 if x < 0.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   FSign(<0.0, -0.0, 0.1, -0.01>) = <0.0, 0.0, 1.0, -1.0>

  auto result = RunWithArgs({0.0f, -0.0f, 0.1f, -0.01f});
  EXPECT_TRUE(glsl::fuzzyEq({0.0f, 0.0f, 1.0f, -1.0f}, result));
}

TEST_F(op_glsl_FSign_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is 1.0 if x > 0, 0.0 if x = 0, or -1.0 if x < 0.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   FSign(-10000.5) = -1.0
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(-10000.5);
    EXPECT_TRUE(glsl::fuzzyEq(-1.0, result));
  }
}

TEST_F(op_glsl_FSign_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is 1.0 if x > 0, 0.0 if x = 0, or -1.0 if x < 0.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   FSign(<-0.0, -10000000.99, 0.5, -4.5>) = <0.0, -1.0, 1.0, -1.0>
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({-0.0, -10000000.99, 0.5, -4.5});
    EXPECT_TRUE(glsl::fuzzyEq({0.0, -1.0, 1.0, -1.0}, result));
  }
}

class op_glsl_SSign_int : public GlslBuiltinTest<glsl::intTy, glsl::intTy> {
 public:
  op_glsl_SSign_int()
      : GlslBuiltinTest<glsl::intTy, glsl::intTy>(
            uvk::Shader::op_glsl_SSign_int) {}
};

TEST_F(op_glsl_SSign_int, Smoke) { RunWithArgs(2); }

class op_glsl_SSign_ivec2
    : public GlslBuiltinTest<glsl::ivec2Ty, glsl::ivec2Ty> {
 public:
  op_glsl_SSign_ivec2()
      : GlslBuiltinTest<glsl::ivec2Ty, glsl::ivec2Ty>(
            uvk::Shader::op_glsl_SSign_ivec2) {}
};

TEST_F(op_glsl_SSign_ivec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_SSign_ivec3
    : public GlslBuiltinTest<glsl::ivec3Ty, glsl::ivec3Ty> {
 public:
  op_glsl_SSign_ivec3()
      : GlslBuiltinTest<glsl::ivec3Ty, glsl::ivec3Ty>(
            uvk::Shader::op_glsl_SSign_ivec3) {}
};

TEST_F(op_glsl_SSign_ivec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_SSign_ivec4
    : public GlslBuiltinTest<glsl::ivec4Ty, glsl::ivec4Ty> {
 public:
  op_glsl_SSign_ivec4()
      : GlslBuiltinTest<glsl::ivec4Ty, glsl::ivec4Ty>(
            uvk::Shader::op_glsl_SSign_ivec4) {}
};

TEST_F(op_glsl_SSign_ivec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }


// Tests that SSign is correctly implemented
TEST_F(op_glsl_SSign_int, BasicCorrectnessTest) {
  // From Specification:
  //   Result is 1 if x > 0, 0 if x = 0, or -1 if x < 0, where x is interpreted
  //   as a signed integer.
  // Expected results:
  //   SSign(-100) = -1
  //   SSign(-1)   = -1
  //   SSign(0)    =  0
  //   SSign(1)    =  1
  //   SSign(100)  =  1
  //   SSign(2147483647)  =  1
  //   SSign(-2147483648)  =  -1

  auto result = RunWithArgs(-100);
  EXPECT_EQ(result, -1);
  result = RunWithArgs(-1);
  EXPECT_EQ(result, -1);
  result = RunWithArgs(0);
  EXPECT_EQ(result, 0);
  result = RunWithArgs(1);
  EXPECT_EQ(result, 1);
  result = RunWithArgs(100);
  EXPECT_EQ(result, 1);
  result = RunWithArgs(I_MAX);
  EXPECT_EQ(result, 1);
  result = RunWithArgs(I_MIN);
  EXPECT_EQ(result, -1);
}

// Test SSign is correctly implemented when operating on vectors
TEST_F(op_glsl_SSign_ivec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is 1 if x > 0, 0 if x = 0, or -1 if x < 0, where x is interpreted
  //   as a signed integer. Results are computed per component.
  // Expected results:
  //   SSign(<-100, -1, 0, 100>) = <-1, -1, 0, 1>

  auto result = RunWithArgs({-100, -1, 0, 100});
  EXPECT_EQ(glsl::ivec4Ty(-1, -1, 0, 1), result);
}

