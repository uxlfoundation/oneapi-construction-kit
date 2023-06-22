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

#include <cmath>
#include <limits>

#include "GLSLTestDefs.h"

constexpr float F_NAN = std::numeric_limits<float>::quiet_NaN();
constexpr float D_NAN = std::numeric_limits<double>::quiet_NaN();
constexpr glsl::intTy I_MIN = std::numeric_limits<glsl::intTy>::min();
constexpr glsl::intTy I_MAX = std::numeric_limits<glsl::intTy>::max();

class op_glsl_FMin_float_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_FMin_float_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_FMin_float_float) {}
};

TEST_F(op_glsl_FMin_float_float, Smoke) { RunWithArgs(2, 2); }

class op_glsl_FMin_vec2_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_FMin_vec2_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_FMin_vec2_vec2) {}
};

TEST_F(op_glsl_FMin_vec2_vec2, Smoke) { RunWithArgs({2, 2}, {2, 2}); }

class op_glsl_FMin_vec3_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_FMin_vec3_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_FMin_vec3_vec3) {}
};

TEST_F(op_glsl_FMin_vec3_vec3, Smoke) { RunWithArgs({2, 2, 2}, {2, 2, 2}); }

class op_glsl_FMin_vec4_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_FMin_vec4_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_FMin_vec4_vec4) {}
};

TEST_F(op_glsl_FMin_vec4_vec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
}

class op_glsl_FMin_double_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_FMin_double_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_FMin_double_double) {}
};

TEST_F(op_glsl_FMin_double_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2, 2);
  }
}

class op_glsl_FMin_dvec2_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_FMin_dvec2_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_FMin_dvec2_dvec2) {}
};

TEST_F(op_glsl_FMin_dvec2_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2}, {2, 2});
  }
}

class op_glsl_FMin_dvec3_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_FMin_dvec3_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_FMin_dvec3_dvec3) {}
};

TEST_F(op_glsl_FMin_dvec3_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2}, {2, 2, 2});
  }
}

class op_glsl_FMin_dvec4_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_FMin_dvec4_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_FMin_dvec4_dvec4) {}
};

TEST_F(op_glsl_FMin_dvec4_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
  }
}

TEST_F(op_glsl_FMin_float_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if y < x; otherwise result is x.
  //   Which operand is the result is undefined if one of the operands is a NaN.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   FMin(2.3, 4.5) = 2.3

  auto result = RunWithArgs(2.3f, 4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(2.3f, result));
}

TEST_F(op_glsl_FMin_vec4_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if y < x; otherwise result is x.
  //   Which operand is the result is undefined if one of the operands is a NaN.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   FMin(<0.0, -0.99, 50.25, -5.45>, <0.5, 0.99, 0.001, -2.23>) =
  //     <0.0, -0.99, 0.001, -5.45>

  auto result = RunWithArgs({0.0f, -0.99f, 50.25f, -5.45f},
                            {0.5f, 0.99f, 0.001f, -2.23f});
  EXPECT_TRUE(glsl::fuzzyEq({0.0f, -0.99f, 0.001f, -5.45f}, result));
}

TEST_F(op_glsl_FMin_double_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if y < x; otherwise result is x.
  //   Which operand is the result is undefined if one of the operands is a NaN.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   FMin(2.3, 0.001) = 0.001
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(2.3, 0.001);
    EXPECT_TRUE(glsl::fuzzyEq(0.001, result));
  }
}

TEST_F(op_glsl_FMin_dvec4_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if y < x; otherwise result is x.
  //   Which operand is the result is undefined if one of the operands is a NaN.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   FMin(<0.499, -0.99, 0.0, -5.45>, <0.5, 0.99, 0.001, 2.23>) =
  //     <0.499, -0.99, 0.0, -5.45>
  if (deviceFeatures.shaderFloat64) {
    auto result =
        RunWithArgs({0.499, -0.99, 0.0, -5.45}, {0.5, 0.99, 0.001, 2.23});
    EXPECT_TRUE(glsl::fuzzyEq({0.499, -0.99, 0.0, -5.45}, result));
  }
}

class op_glsl_UMin_uint_uint
    : public GlslBuiltinTest<glsl::uintTy, glsl::uintTy, glsl::uintTy> {
 public:
  op_glsl_UMin_uint_uint()
      : GlslBuiltinTest<glsl::uintTy, glsl::uintTy, glsl::uintTy>(
            uvk::Shader::op_glsl_UMin_uint_uint) {}
};

TEST_F(op_glsl_UMin_uint_uint, Smoke) { RunWithArgs(2, 2); }

class op_glsl_UMin_uvec2_uvec2
    : public GlslBuiltinTest<glsl::uvec2Ty, glsl::uvec2Ty, glsl::uvec2Ty> {
 public:
  op_glsl_UMin_uvec2_uvec2()
      : GlslBuiltinTest<glsl::uvec2Ty, glsl::uvec2Ty, glsl::uvec2Ty>(
            uvk::Shader::op_glsl_UMin_uvec2_uvec2) {}
};

TEST_F(op_glsl_UMin_uvec2_uvec2, Smoke) { RunWithArgs({2, 2}, {2, 2}); }

class op_glsl_UMin_uvec3_uvec3
    : public GlslBuiltinTest<glsl::uvec3Ty, glsl::uvec3Ty, glsl::uvec3Ty> {
 public:
  op_glsl_UMin_uvec3_uvec3()
      : GlslBuiltinTest<glsl::uvec3Ty, glsl::uvec3Ty, glsl::uvec3Ty>(
            uvk::Shader::op_glsl_UMin_uvec3_uvec3) {}
};

TEST_F(op_glsl_UMin_uvec3_uvec3, Smoke) { RunWithArgs({2, 2, 2}, {2, 2, 2}); }

class op_glsl_UMin_uvec4_uvec4
    : public GlslBuiltinTest<glsl::uvec4Ty, glsl::uvec4Ty, glsl::uvec4Ty> {
 public:
  op_glsl_UMin_uvec4_uvec4()
      : GlslBuiltinTest<glsl::uvec4Ty, glsl::uvec4Ty, glsl::uvec4Ty>(
            uvk::Shader::op_glsl_UMin_uvec4_uvec4) {}
};

TEST_F(op_glsl_UMin_uvec4_uvec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
}

TEST_F(op_glsl_UMin_uint_uint, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if y < x; otherwise result is x,
  //   where x and y are interpreted as unsigned integers.
  //
  //   Result Type and the type of x and y must both be integer scalar or
  //   integer vector types. Result Type and operand types must have the same
  //   number of components with the same component width. Results are computed
  //   per component.
  // Expected results:
  //   UMin(2, 0) = 0

  auto result = RunWithArgs(2, 0);
  EXPECT_EQ(0, result);
}

TEST_F(op_glsl_UMin_uvec4_uvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if y < x; otherwise result is x,
  //   where x and y are interpreted as unsigned integers.
  //
  //   Result Type and the type of x and y must both be integer scalar or
  //   integer vector types. Result Type and operand types must have the same
  //   number of components with the same component width. Results are computed
  //   per component.
  // Expected results:
  //   UMin(<5, 68, 1, 2147483647>, <2000, 67, 10, 2147483646>) =
  //     <5, 67, 1, 2147483646>

  auto result = RunWithArgs({5, 68, 1, (glsl::uintTy)I_MAX},
                            {2000, 67, 10, (glsl::uintTy)I_MAX - 1});
  EXPECT_EQ(glsl::uvec4Ty(5U, 67U, 1U, (glsl::uintTy)I_MAX - 1), result);
}

class op_glsl_SMin_int_int
    : public GlslBuiltinTest<glsl::intTy, glsl::intTy, glsl::intTy> {
 public:
  op_glsl_SMin_int_int()
      : GlslBuiltinTest<glsl::intTy, glsl::intTy, glsl::intTy>(
            uvk::Shader::op_glsl_SMin_int_int) {}
};

TEST_F(op_glsl_SMin_int_int, Smoke) { RunWithArgs(2, 2); }

class op_glsl_SMin_ivec2_ivec2
    : public GlslBuiltinTest<glsl::ivec2Ty, glsl::ivec2Ty, glsl::ivec2Ty> {
 public:
  op_glsl_SMin_ivec2_ivec2()
      : GlslBuiltinTest<glsl::ivec2Ty, glsl::ivec2Ty, glsl::ivec2Ty>(
            uvk::Shader::op_glsl_SMin_ivec2_ivec2) {}
};

TEST_F(op_glsl_SMin_ivec2_ivec2, Smoke) { RunWithArgs({2, 2}, {2, 2}); }

class op_glsl_SMin_ivec3_ivec3
    : public GlslBuiltinTest<glsl::ivec3Ty, glsl::ivec3Ty, glsl::ivec3Ty> {
 public:
  op_glsl_SMin_ivec3_ivec3()
      : GlslBuiltinTest<glsl::ivec3Ty, glsl::ivec3Ty, glsl::ivec3Ty>(
            uvk::Shader::op_glsl_SMin_ivec3_ivec3) {}
};

TEST_F(op_glsl_SMin_ivec3_ivec3, Smoke) { RunWithArgs({2, 2, 2}, {2, 2, 2}); }

class op_glsl_SMin_ivec4_ivec4
    : public GlslBuiltinTest<glsl::ivec4Ty, glsl::ivec4Ty, glsl::ivec4Ty> {
 public:
  op_glsl_SMin_ivec4_ivec4()
      : GlslBuiltinTest<glsl::ivec4Ty, glsl::ivec4Ty, glsl::ivec4Ty>(
            uvk::Shader::op_glsl_SMin_ivec4_ivec4) {}
};

TEST_F(op_glsl_SMin_ivec4_ivec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
}

TEST_F(op_glsl_SMin_int_int, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if y < x; otherwise result is x,
  //   where x and y are interpreted as signed integers.
  //
  //   Result Type and the type of x and y must both be integer scalar or
  //   integer vector types. Result Type and operand types must have the same
  //   number of components with the same component width. Results are computed
  //   per component.
  // Expected results:
  //   SMin(2, 4) = -2

  auto result = RunWithArgs(-2, 4);
  EXPECT_EQ(-2, result);
}

TEST_F(op_glsl_SMin_ivec4_ivec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if y < x; otherwise result is x,
  //   where x and y are interpreted as signed integers.
  //
  //   Result Type and the type of x and y must both be integer scalar or
  //   integer vector types. Result Type and operand types must have the same
  //   number of components with the same component width. Results are computed
  //   per component.
  // Expected results:
  //   SMin(<5, 68, 1, -2147483648>, <2000, 67, 10, 2147483646>) =
  //     <5, -68, 1, -2147483648>

  auto result = RunWithArgs({-5, -68, 1, I_MIN}, {2000, 67, 10, I_MAX});
  EXPECT_EQ(glsl::ivec4Ty(-5, -68, 1, I_MIN), result);
}

class op_glsl_FMax_float_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_FMax_float_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_FMax_float_float) {}
};

TEST_F(op_glsl_FMax_float_float, Smoke) { RunWithArgs(2, 2); }

class op_glsl_FMax_vec2_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_FMax_vec2_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_FMax_vec2_vec2) {}
};

TEST_F(op_glsl_FMax_vec2_vec2, Smoke) { RunWithArgs({2, 2}, {2, 2}); }

class op_glsl_FMax_vec3_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_FMax_vec3_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_FMax_vec3_vec3) {}
};

TEST_F(op_glsl_FMax_vec3_vec3, Smoke) { RunWithArgs({2, 2, 2}, {2, 2, 2}); }

class op_glsl_FMax_vec4_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_FMax_vec4_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_FMax_vec4_vec4) {}
};

TEST_F(op_glsl_FMax_vec4_vec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
}

class op_glsl_FMax_double_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_FMax_double_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_FMax_double_double) {}
};

TEST_F(op_glsl_FMax_double_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2, 2);
  }
}

class op_glsl_FMax_dvec2_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_FMax_dvec2_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_FMax_dvec2_dvec2) {}
};

TEST_F(op_glsl_FMax_dvec2_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2}, {2, 2});
  }
}

class op_glsl_FMax_dvec3_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_FMax_dvec3_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_FMax_dvec3_dvec3) {}
};

TEST_F(op_glsl_FMax_dvec3_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2}, {2, 2, 2});
  }
}

class op_glsl_FMax_dvec4_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_FMax_dvec4_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_FMax_dvec4_dvec4) {}
};

TEST_F(op_glsl_FMax_dvec4_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
  }
}

TEST_F(op_glsl_FMax_float_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if x < y; otherwise result is x.
  //   Which operand is the result is undefined if one of the operands is a NaN.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   FMax(2.3, 4.5) = 4.5

  auto result = RunWithArgs(2.3f, 4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(4.5f, result));
}

TEST_F(op_glsl_FMax_vec4_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if x < y; otherwise result is x.
  //   Which operand is the result is undefined if one of the operands is a NaN.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   FMax({0.0, -0.99, 50.25, -5.45}, {0.5, 0.99, 0.001, -2.23}) =
  //     <0.5, 0.99, 50.25, -2.23>

  auto result = RunWithArgs({0.0f, -0.99f, 50.25f, -5.45f},
                            {0.5f, 0.99f, 0.001f, -2.23f});
  EXPECT_TRUE(glsl::fuzzyEq({0.5f, 0.99f, 50.25f, -2.23f}, result));
}

TEST_F(op_glsl_FMax_double_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if x < y; otherwise result is x.
  //   Which operand is the result is undefined if one of the operands is a NaN.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   FMax(2.3, 0.001) = 2.3
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(2.3, 0.001);
    EXPECT_TRUE(glsl::fuzzyEq(2.3, result));
  }
}

TEST_F(op_glsl_FMax_dvec4_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if x < y; otherwise result is x.
  //   Which operand is the result is undefined if one of the operands is a NaN.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   FMax(<0.499, -0.99, 0.0, -5.45>, <0.5, 0.99, 0.001, 2.23>) =
  //     <0.5, 0.99, 0.001, 2.23>
  if (deviceFeatures.shaderFloat64) {
    auto result =
        RunWithArgs({0.499, -0.99, 0.0, -5.45}, {0.5, 0.99, 0.001, 2.23});
    EXPECT_TRUE(glsl::fuzzyEq({0.5, 0.99, 0.001, 2.23}, result));
  }
}

class op_glsl_UMax_uint_uint
    : public GlslBuiltinTest<glsl::uintTy, glsl::uintTy, glsl::uintTy> {
 public:
  op_glsl_UMax_uint_uint()
      : GlslBuiltinTest<glsl::uintTy, glsl::uintTy, glsl::uintTy>(
            uvk::Shader::op_glsl_UMax_uint_uint) {}
};

TEST_F(op_glsl_UMax_uint_uint, Smoke) { RunWithArgs(2, 2); }

class op_glsl_UMax_uvec2_uvec2
    : public GlslBuiltinTest<glsl::uvec2Ty, glsl::uvec2Ty, glsl::uvec2Ty> {
 public:
  op_glsl_UMax_uvec2_uvec2()
      : GlslBuiltinTest<glsl::uvec2Ty, glsl::uvec2Ty, glsl::uvec2Ty>(
            uvk::Shader::op_glsl_UMax_uvec2_uvec2) {}
};

TEST_F(op_glsl_UMax_uvec2_uvec2, Smoke) { RunWithArgs({2, 2}, {2, 2}); }

class op_glsl_UMax_uvec3_uvec3
    : public GlslBuiltinTest<glsl::uvec3Ty, glsl::uvec3Ty, glsl::uvec3Ty> {
 public:
  op_glsl_UMax_uvec3_uvec3()
      : GlslBuiltinTest<glsl::uvec3Ty, glsl::uvec3Ty, glsl::uvec3Ty>(
            uvk::Shader::op_glsl_UMax_uvec3_uvec3) {}
};

TEST_F(op_glsl_UMax_uvec3_uvec3, Smoke) { RunWithArgs({2, 2, 2}, {2, 2, 2}); }

class op_glsl_UMax_uvec4_uvec4
    : public GlslBuiltinTest<glsl::uvec4Ty, glsl::uvec4Ty, glsl::uvec4Ty> {
 public:
  op_glsl_UMax_uvec4_uvec4()
      : GlslBuiltinTest<glsl::uvec4Ty, glsl::uvec4Ty, glsl::uvec4Ty>(
            uvk::Shader::op_glsl_UMax_uvec4_uvec4) {}
};

TEST_F(op_glsl_UMax_uvec4_uvec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
}

TEST_F(op_glsl_UMax_uint_uint, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if x < y; otherwise result is x,
  //   where x and y are interpreted as unsigned integers.
  //
  //   Result Type and the type of x and y must both be integer scalar or
  //   integer vector types. Result Type and operand types must have the same
  //   number of components with the same component width. Results are computed
  //   per component.
  // Expected results:
  //   UMax(2, 0) = 2

  auto result = RunWithArgs(2, 0);
  EXPECT_EQ(2, result);
}

TEST_F(op_glsl_UMax_uvec4_uvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if x < y; otherwise result is x,
  //   where x and y are interpreted as unsigned integers.
  //
  //   Result Type and the type of x and y must both be integer scalar or
  //   integer vector types. Result Type and operand types must have the same
  //   number of components with the same component width. Results are computed
  //   per component.
  // Expected results:
  //   UMax(<5, 68, 1, 2147483647>, <2000, 67, 10, 2147483646>) =
  //     <2000, 68, 10, 2147483647>

  auto result = RunWithArgs({5, 68, 1, (glsl::uintTy)I_MAX},
                            {2000, 67, 10, (glsl::uintTy)I_MAX - 1});
  EXPECT_EQ(glsl::uvec4Ty(2000U, 68U, 10U, (glsl::uintTy)I_MAX), result);
}

class op_glsl_SMax_int_int
    : public GlslBuiltinTest<glsl::intTy, glsl::intTy, glsl::intTy> {
 public:
  op_glsl_SMax_int_int()
      : GlslBuiltinTest<glsl::intTy, glsl::intTy, glsl::intTy>(
            uvk::Shader::op_glsl_SMax_int_int) {}
};

TEST_F(op_glsl_SMax_int_int, Smoke) { RunWithArgs(2, 2); }

class op_glsl_SMax_ivec2_ivec2
    : public GlslBuiltinTest<glsl::ivec2Ty, glsl::ivec2Ty, glsl::ivec2Ty> {
 public:
  op_glsl_SMax_ivec2_ivec2()
      : GlslBuiltinTest<glsl::ivec2Ty, glsl::ivec2Ty, glsl::ivec2Ty>(
            uvk::Shader::op_glsl_SMax_ivec2_ivec2) {}
};

TEST_F(op_glsl_SMax_ivec2_ivec2, Smoke) { RunWithArgs({2, 2}, {2, 2}); }

class op_glsl_SMax_ivec3_ivec3
    : public GlslBuiltinTest<glsl::ivec3Ty, glsl::ivec3Ty, glsl::ivec3Ty> {
 public:
  op_glsl_SMax_ivec3_ivec3()
      : GlslBuiltinTest<glsl::ivec3Ty, glsl::ivec3Ty, glsl::ivec3Ty>(
            uvk::Shader::op_glsl_SMax_ivec3_ivec3) {}
};

TEST_F(op_glsl_SMax_ivec3_ivec3, Smoke) { RunWithArgs({2, 2, 2}, {2, 2, 2}); }

class op_glsl_SMax_ivec4_ivec4
    : public GlslBuiltinTest<glsl::ivec4Ty, glsl::ivec4Ty, glsl::ivec4Ty> {
 public:
  op_glsl_SMax_ivec4_ivec4()
      : GlslBuiltinTest<glsl::ivec4Ty, glsl::ivec4Ty, glsl::ivec4Ty>(
            uvk::Shader::op_glsl_SMax_ivec4_ivec4) {}
};

TEST_F(op_glsl_SMax_ivec4_ivec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
}

TEST_F(op_glsl_SMax_int_int, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if x < y; otherwise result is x,
  //   where x and y are interpreted as signed integers.
  //
  //   Result Type and the type of x and y must both be integer scalar or
  //   integer vector types. Result Type and operand types must have the same
  //   number of components with the same component width. Results are computed
  //   per component.
  // Expected results:
  //   SMax(2, 4) = 4

  auto result = RunWithArgs(-2, 4);
  EXPECT_EQ(4, result);
}

TEST_F(op_glsl_SMax_ivec4_ivec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if x < y; otherwise result is x,
  //   where x and y are interpreted as signed integers.
  //
  //   Result Type and the type of x and y must both be integer scalar or
  //   integer vector types. Result Type and operand types must have the same
  //   number of components with the same component width. Results are computed
  //   per component.
  // Expected results:
  //   SMax(<5, -68, 1, -2147483648>, <2000, 67, 10, 2147483647>) =
  //     <2000, 67, 10, 2147483647>

  auto result = RunWithArgs({-5, -68, 1, I_MIN}, {2000, 67, 10, I_MAX});
  EXPECT_EQ(glsl::ivec4Ty(2000, 67, 10, I_MAX), result);
}

class op_glsl_NMin_float_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_NMin_float_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_NMin_float_float) {}
};

TEST_F(op_glsl_NMin_float_float, Smoke) { RunWithArgs(2, 2); }

class op_glsl_NMin_vec2_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_NMin_vec2_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_NMin_vec2_vec2) {}
};

TEST_F(op_glsl_NMin_vec2_vec2, Smoke) { RunWithArgs({2, 2}, {2, 2}); }

class op_glsl_NMin_vec3_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_NMin_vec3_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_NMin_vec3_vec3) {}
};

TEST_F(op_glsl_NMin_vec3_vec3, Smoke) { RunWithArgs({2, 2, 2}, {2, 2, 2}); }

class op_glsl_NMin_vec4_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_NMin_vec4_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_NMin_vec4_vec4) {}
};

TEST_F(op_glsl_NMin_vec4_vec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
}

class op_glsl_NMin_double_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_NMin_double_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_NMin_double_double) {}
};

TEST_F(op_glsl_NMin_double_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2, 2);
  }
}

class op_glsl_NMin_dvec2_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_NMin_dvec2_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_NMin_dvec2_dvec2) {}
};

TEST_F(op_glsl_NMin_dvec2_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2}, {2, 2});
  }
}

class op_glsl_NMin_dvec3_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_NMin_dvec3_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_NMin_dvec3_dvec3) {}
};

TEST_F(op_glsl_NMin_dvec3_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2}, {2, 2, 2});
  }
}

class op_glsl_NMin_dvec4_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_NMin_dvec4_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_NMin_dvec4_dvec4) {}
};

TEST_F(op_glsl_NMin_dvec4_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
  }
}

TEST_F(op_glsl_NMin_float_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if y < x; otherwise result is x. If one operand is a NaN,
  //   the other operand is the result. If both operands are NaN, the result is
  //   a NaN.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   NMin(2.3, NaN) = 2.3

  auto result = RunWithArgs(2.3f, F_NAN);
  EXPECT_TRUE(glsl::fuzzyEq(2.3f, result));
}

TEST_F(op_glsl_NMin_vec4_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if y < x; otherwise result is x. If one operand is a NaN,
  //   the other operand is the result. If both operands are NaN, the result is
  //   a NaN.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   NMin({0.0, -0.99, NaN, NaN}, {0.5, 0.99, 0.001, NaN}) =
  //     <0.0, -0.99, 0.001, NaN>

  auto result =
      RunWithArgs({0.0f, -0.99f, F_NAN, F_NAN}, {0.5f, 0.99f, 0.001f, F_NAN});
  EXPECT_TRUE(glsl::fuzzyEq(0.0f, result.data[0]) &&
              glsl::fuzzyEq(-0.99f, result.data[1]) &&
              glsl::fuzzyEq(0.001f, result.data[2]) &&
              std::isnan(result.data[3]));
}

TEST_F(op_glsl_NMin_double_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if y < x; otherwise result is x. If one operand is a NaN,
  //   the other operand is the result. If both operands are NaN, the result is
  //   a NaN.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   NMin(NaN, 0.001) = 0.001
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(D_NAN, 0.001);
    EXPECT_TRUE(glsl::fuzzyEq(0.001, result));
  }
}

TEST_F(op_glsl_NMin_dvec4_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if y < x; otherwise result is x. If one operand is a NaN,
  //   the other operand is the result. If both operands are NaN, the result is
  //   a NaN.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   NMin(<0.499, -0.99, 0.0, NaN>, <NaN, 0.99, 0.001, NaN>) =
  //     <0.499, -0.99, 0.0, NaN>
  if (deviceFeatures.shaderFloat64) {
    auto result =
        RunWithArgs({0.499, -0.99, 0.0, D_NAN}, {D_NAN, 0.99, 0.001, D_NAN});
    EXPECT_TRUE(glsl::fuzzyEq(0.499, result.data[0]) &&
                glsl::fuzzyEq(-0.99, result.data[1]) &&
                glsl::fuzzyEq(0.0, result.data[2]) &&
                std::isnan(result.data[3]));
  }
}

class op_glsl_NMax_float_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_NMax_float_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_NMax_float_float) {}
};

TEST_F(op_glsl_NMax_float_float, Smoke) { RunWithArgs(2, 2); }

class op_glsl_NMax_vec2_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_NMax_vec2_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_NMax_vec2_vec2) {}
};

TEST_F(op_glsl_NMax_vec2_vec2, Smoke) { RunWithArgs({2, 2}, {2, 2}); }

class op_glsl_NMax_vec3_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_NMax_vec3_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_NMax_vec3_vec3) {}
};

TEST_F(op_glsl_NMax_vec3_vec3, Smoke) { RunWithArgs({2, 2, 2}, {2, 2, 2}); }

class op_glsl_NMax_vec4_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_NMax_vec4_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_NMax_vec4_vec4) {}
};

TEST_F(op_glsl_NMax_vec4_vec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
}

class op_glsl_NMax_double_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_NMax_double_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_NMax_double_double) {}
};

TEST_F(op_glsl_NMax_double_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2, 2);
  }
}

class op_glsl_NMax_dvec2_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_NMax_dvec2_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_NMax_dvec2_dvec2) {}
};

TEST_F(op_glsl_NMax_dvec2_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2}, {2, 2});
  }
}

class op_glsl_NMax_dvec3_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_NMax_dvec3_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_NMax_dvec3_dvec3) {}
};

TEST_F(op_glsl_NMax_dvec3_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2}, {2, 2, 2});
  }
}

class op_glsl_NMax_dvec4_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_NMax_dvec4_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_NMax_dvec4_dvec4) {}
};

TEST_F(op_glsl_NMax_dvec4_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
  }
}

TEST_F(op_glsl_NMax_float_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if x < y; otherwise result is x. If one operand is a NaN,
  //   the other operand is the result. If both operands are NaN, the result is
  //   a NaN.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   NMax(2.3, NaN) = 2.3

  auto result = RunWithArgs(2.3f, F_NAN);
  EXPECT_TRUE(glsl::fuzzyEq(2.3f, result));
}

TEST_F(op_glsl_NMax_vec4_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if x < y; otherwise result is x. If one operand is a NaN,
  //   the other operand is the result. If both operands are NaN, the result is
  //   a NaN.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   NMax({0.0, -0.99, NaN, NaN}, {0.5, 0.99, 0.001, NaN}) =
  //     <0.5, 0.99, 0.001, NaN>

  auto result =
      RunWithArgs({0.0f, -0.99f, F_NAN, F_NAN}, {0.5f, 0.99f, 0.001f, F_NAN});
  EXPECT_TRUE(glsl::fuzzyEq(0.5f, result.data[0]) &&
              glsl::fuzzyEq(0.99f, result.data[1]) &&
              glsl::fuzzyEq(0.001f, result.data[2]) &&
              std::isnan(result.data[3]));
}

TEST_F(op_glsl_NMax_double_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if x < y; otherwise result is x. If one operand is a NaN,
  //   the other operand is the result. If both operands are NaN, the result is
  //   a NaN.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   NMax(NaN, 0.001) = 0.001
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(D_NAN, 0.001);
    EXPECT_TRUE(glsl::fuzzyEq(0.001, result));
  }
}

TEST_F(op_glsl_NMax_dvec4_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is y if x < y; otherwise result is x. If one operand is a NaN,
  //   the other operand is the result. If both operands are NaN, the result is
  //   a NaN.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   NMax(<0.499, -0.99, 0.0, NaN>, <NaN, 0.99, 0.001, NaN>) =
  //     <0.499, 0.99, 0.001, NaN>
  if (deviceFeatures.shaderFloat64) {
    auto result =
        RunWithArgs({0.499, -0.99, 0.0, D_NAN}, {D_NAN, 0.99, 0.001, D_NAN});
    EXPECT_TRUE(glsl::fuzzyEq(0.499, result.data[0]) &&
                glsl::fuzzyEq(0.99, result.data[1]) &&
                glsl::fuzzyEq(0.001, result.data[2]) &&
                std::isnan(result.data[3]));
  }
}
