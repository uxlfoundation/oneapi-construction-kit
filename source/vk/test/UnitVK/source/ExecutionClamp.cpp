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

class op_glsl_FClamp_float_float_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy,
                             glsl::floatTy> {
 public:
  op_glsl_FClamp_float_float_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy,
                        glsl::floatTy>(
            uvk::Shader::op_glsl_FClamp_float_float_float) {}
};

TEST_F(op_glsl_FClamp_float_float_float, Smoke) { RunWithArgs(2, 2, 2); }

class op_glsl_FClamp_vec2_vec2_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty,
                             glsl::vec2Ty> {
 public:
  op_glsl_FClamp_vec2_vec2_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_FClamp_vec2_vec2_vec2) {}
};

TEST_F(op_glsl_FClamp_vec2_vec2_vec2, Smoke) {
  RunWithArgs({2, 2}, {2, 2}, {2, 2});
}

class op_glsl_FClamp_vec3_vec3_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty,
                             glsl::vec3Ty> {
 public:
  op_glsl_FClamp_vec3_vec3_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_FClamp_vec3_vec3_vec3) {}
};

TEST_F(op_glsl_FClamp_vec3_vec3_vec3, Smoke) {
  RunWithArgs({2, 2, 2}, {2, 2, 2}, {2, 2, 2});
}

class op_glsl_FClamp_vec4_vec4_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty,
                             glsl::vec4Ty> {
 public:
  op_glsl_FClamp_vec4_vec4_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_FClamp_vec4_vec4_vec4) {}
};

TEST_F(op_glsl_FClamp_vec4_vec4_vec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2});
}

class op_glsl_FClamp_double_double_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy,
                             glsl::doubleTy> {
 public:
  op_glsl_FClamp_double_double_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy,
                        glsl::doubleTy>(
            uvk::Shader::op_glsl_FClamp_double_double_double) {}
};

TEST_F(op_glsl_FClamp_double_double_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2, 2, 2);
  }
}

class op_glsl_FClamp_dvec2_dvec2_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty,
                             glsl::dvec2Ty> {
 public:
  op_glsl_FClamp_dvec2_dvec2_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty,
                        glsl::dvec2Ty>(
            uvk::Shader::op_glsl_FClamp_dvec2_dvec2_dvec2) {}
};

TEST_F(op_glsl_FClamp_dvec2_dvec2_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2}, {2, 2}, {2, 2});
  }
}

class op_glsl_FClamp_dvec3_dvec3_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty,
                             glsl::dvec3Ty> {
 public:
  op_glsl_FClamp_dvec3_dvec3_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty,
                        glsl::dvec3Ty>(
            uvk::Shader::op_glsl_FClamp_dvec3_dvec3_dvec3) {}
};

TEST_F(op_glsl_FClamp_dvec3_dvec3_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2}, {2, 2, 2}, {2, 2, 2});
  }
}

class op_glsl_FClamp_dvec4_dvec4_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty,
                             glsl::dvec4Ty> {
 public:
  op_glsl_FClamp_dvec4_dvec4_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty,
                        glsl::dvec4Ty>(
            uvk::Shader::op_glsl_FClamp_dvec4_dvec4_dvec4) {}
};

TEST_F(op_glsl_FClamp_dvec4_dvec4_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2});
  }
}

TEST_F(op_glsl_FClamp_float_float_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is min(max(x, minVal), maxVal). Result is undefined if minVal >
  //   maxVal. The semantics used by min() and max() are those of FMin and FMax.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   FClamp(2.3, 4.5, 8.3) = 4.5

  auto result = RunWithArgs(2.3f, 4.5f, 8.3f);
  EXPECT_TRUE(glsl::fuzzyEq(4.5f, result));
}

TEST_F(op_glsl_FClamp_vec4_vec4_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is min(max(x, minVal), maxVal). Result is undefined if minVal >
  //   maxVal. The semantics used by min() and max() are those of FMin and FMax.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   FClamp(<0.0, -0.99, 50.25, -5.45>, <0.5, 0.99, 0.001, -2.23>,
  //          <0.8, 2.02, 25.02, 0.0>)
  //        = <0.5, 0.99, 25.02, -2.23>

  auto result =
      RunWithArgs({0.0f, -0.99f, 50.25f, -5.45f}, {0.5f, 0.99f, 0.001f, -2.23f},
                  {0.8f, 2.02f, 25.02f, 0.0f});
  EXPECT_TRUE(glsl::fuzzyEq({0.5f, 0.99f, 25.02f, -2.23f}, result));
}

TEST_F(op_glsl_FClamp_double_double_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is min(max(x, minVal), maxVal). Result is undefined if minVal >
  //   maxVal. The semantics used by min() and max() are those of FMin and FMax.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   FClamp(2.3, 4.5, 8.3) = 4.5
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(2.3, 4.5, 8.3);
    EXPECT_TRUE(glsl::fuzzyEq(4.5, result));
  }
}

TEST_F(op_glsl_FClamp_dvec4_dvec4_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is min(max(x, minVal), maxVal). Result is undefined if minVal >
  //   maxVal. The semantics used by min() and max() are those of FMin and FMax.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   FClamp(<1.0, -0.99, 50.25, -5.45>, <0.5, 0.99, 0.001, -2.23>,
  //          <0.8, 2.02, 25.02, 0.0>)
  //        = <0.8, 0.99, 25.02, -2.23>
  if (deviceFeatures.shaderFloat64) {
    auto result =
        RunWithArgs({1.0, -0.99, 50.25, -5.45}, {0.5, 0.99, 0.001, -2.23},
                    {0.8, 2.02, 25.02, 0.0});
    EXPECT_TRUE(glsl::fuzzyEq({0.8, 0.99, 25.02, -2.23}, result));
  }
}

class op_glsl_UClamp_uint_uint_uint
    : public GlslBuiltinTest<glsl::uintTy, glsl::uintTy, glsl::uintTy,
                             glsl::uintTy> {
 public:
  op_glsl_UClamp_uint_uint_uint()
      : GlslBuiltinTest<glsl::uintTy, glsl::uintTy, glsl::uintTy, glsl::uintTy>(
            uvk::Shader::op_glsl_UClamp_uint_uint_uint) {}
};

TEST_F(op_glsl_UClamp_uint_uint_uint, Smoke) { RunWithArgs(2, 2, 2); }

class op_glsl_UClamp_uvec2_uvec2_uvec2
    : public GlslBuiltinTest<glsl::uvec2Ty, glsl::uvec2Ty, glsl::uvec2Ty,
                             glsl::uvec2Ty> {
 public:
  op_glsl_UClamp_uvec2_uvec2_uvec2()
      : GlslBuiltinTest<glsl::uvec2Ty, glsl::uvec2Ty, glsl::uvec2Ty,
                        glsl::uvec2Ty>(
            uvk::Shader::op_glsl_UClamp_uvec2_uvec2_uvec2) {}
};

TEST_F(op_glsl_UClamp_uvec2_uvec2_uvec2, Smoke) {
  RunWithArgs({2, 2}, {2, 2}, {2, 2});
}

class op_glsl_UClamp_uvec3_uvec3_uvec3
    : public GlslBuiltinTest<glsl::uvec3Ty, glsl::uvec3Ty, glsl::uvec3Ty,
                             glsl::uvec3Ty> {
 public:
  op_glsl_UClamp_uvec3_uvec3_uvec3()
      : GlslBuiltinTest<glsl::uvec3Ty, glsl::uvec3Ty, glsl::uvec3Ty,
                        glsl::uvec3Ty>(
            uvk::Shader::op_glsl_UClamp_uvec3_uvec3_uvec3) {}
};

TEST_F(op_glsl_UClamp_uvec3_uvec3_uvec3, Smoke) {
  RunWithArgs({2, 2, 2}, {2, 2, 2}, {2, 2, 2});
}

class op_glsl_UClamp_uvec4_uvec4_uvec4
    : public GlslBuiltinTest<glsl::uvec4Ty, glsl::uvec4Ty, glsl::uvec4Ty,
                             glsl::uvec4Ty> {
 public:
  op_glsl_UClamp_uvec4_uvec4_uvec4()
      : GlslBuiltinTest<glsl::uvec4Ty, glsl::uvec4Ty, glsl::uvec4Ty,
                        glsl::uvec4Ty>(
            uvk::Shader::op_glsl_UClamp_uvec4_uvec4_uvec4) {}
};

TEST_F(op_glsl_UClamp_uvec4_uvec4_uvec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2});
}

TEST_F(op_glsl_UClamp_uint_uint_uint, BasicCorrectnessTest) {
  // From specification:
  //   Result is min(max(x, minVal), maxVal), where x, minVal and maxVal are
  //   interpreted as unsigned integers. Result is undefined if minVal > maxVal.
  //
  //   Result Type and the type of the operands must both be integer scalar or
  //   integer vector types. Result Type and operand types must have the same
  //   number of components with the same component width. Results are computed
  //   per component.
  // Expected results:
  //   UClamp(2, 4, 8) = 4

  auto result = RunWithArgs(2, 4, 8);
  EXPECT_EQ(4, result);
}

TEST_F(op_glsl_UClamp_uvec4_uvec4_uvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is min(max(x, minVal), maxVal), where x, minVal and maxVal are
  //   interpreted as unsigned integers. Result is undefined if minVal > maxVal.
  //
  //   Result Type and the type of the operands must both be integer scalar or
  //   integer vector types. Result Type and operand types must have the same
  //   number of components with the same component width. Results are computed
  //   per component.
  // Expected results:
  //   UClamp(<0, 1, 50, 5>, <0, 0, 1, 2>,
  //          <0, 2, 35, 8>)
  //        = <0, 1, 35, 5>

  auto result = RunWithArgs({0, 1, 50, 5}, {0, 0, 1, 2}, {0, 2, 35, 8});
  EXPECT_EQ(glsl::uvec4Ty(0U, 1U, 35U, 5U), result);
}

class op_glsl_SClamp_int_int_int
    : public GlslBuiltinTest<glsl::intTy, glsl::intTy, glsl::intTy,
                             glsl::intTy> {
 public:
  op_glsl_SClamp_int_int_int()
      : GlslBuiltinTest<glsl::intTy, glsl::intTy, glsl::intTy, glsl::intTy>(
            uvk::Shader::op_glsl_SClamp_int_int_int) {}
};

TEST_F(op_glsl_SClamp_int_int_int, Smoke) { RunWithArgs(2, 2, 2); }

class op_glsl_SClamp_ivec2_ivec2_ivec2
    : public GlslBuiltinTest<glsl::ivec2Ty, glsl::ivec2Ty, glsl::ivec2Ty,
                             glsl::ivec2Ty> {
 public:
  op_glsl_SClamp_ivec2_ivec2_ivec2()
      : GlslBuiltinTest<glsl::ivec2Ty, glsl::ivec2Ty, glsl::ivec2Ty,
                        glsl::ivec2Ty>(
            uvk::Shader::op_glsl_SClamp_ivec2_ivec2_ivec2) {}
};

TEST_F(op_glsl_SClamp_ivec2_ivec2_ivec2, Smoke) {
  RunWithArgs({2, 2}, {2, 2}, {2, 2});
}

class op_glsl_SClamp_ivec3_ivec3_ivec3
    : public GlslBuiltinTest<glsl::ivec3Ty, glsl::ivec3Ty, glsl::ivec3Ty,
                             glsl::ivec3Ty> {
 public:
  op_glsl_SClamp_ivec3_ivec3_ivec3()
      : GlslBuiltinTest<glsl::ivec3Ty, glsl::ivec3Ty, glsl::ivec3Ty,
                        glsl::ivec3Ty>(
            uvk::Shader::op_glsl_SClamp_ivec3_ivec3_ivec3) {}
};

TEST_F(op_glsl_SClamp_ivec3_ivec3_ivec3, Smoke) {
  RunWithArgs({2, 2, 2}, {2, 2, 2}, {2, 2, 2});
}

class op_glsl_SClamp_ivec4_ivec4_ivec4
    : public GlslBuiltinTest<glsl::ivec4Ty, glsl::ivec4Ty, glsl::ivec4Ty,
                             glsl::ivec4Ty> {
 public:
  op_glsl_SClamp_ivec4_ivec4_ivec4()
      : GlslBuiltinTest<glsl::ivec4Ty, glsl::ivec4Ty, glsl::ivec4Ty,
                        glsl::ivec4Ty>(
            uvk::Shader::op_glsl_SClamp_ivec4_ivec4_ivec4) {}
};

TEST_F(op_glsl_SClamp_ivec4_ivec4_ivec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2});
}

TEST_F(op_glsl_SClamp_int_int_int, BasicCorrectnessTest) {
  // From specification:
  //   Result is min(max(x, minVal), maxVal), where x, minVal and maxVal are
  //   interpreted as signed integers. Result is undefined if minVal > maxVal.
  //
  //   Result Type and the type of the operands must both be integer scalar or
  //   integer vector types. Result Type and operand types must have the same
  //   number of components with the same component width. Results are computed
  //   per component.
  // Expected results:
  //   SClamp(2, -4, 8) = 2

  auto result = RunWithArgs(2, -4, 8);
  EXPECT_EQ(2, result);
}

TEST_F(op_glsl_SClamp_ivec4_ivec4_ivec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is min(max(x, minVal), maxVal), where x, minVal and maxVal are
  //   interpreted as signed integers. Result is undefined if minVal > maxVal.
  //
  //   Result Type and the type of the operands must both be integer scalar or
  //   integer vector types. Result Type and operand types must have the same
  //   number of components with the same component width. Results are computed
  //   per component.
  // Expected results:
  //   SClamp(<-3, 1, 50, -5>, <0, -8, -10, 2>,
  //          <0, 2, -5, 8>)
  //        = <0, 1, -5, 2>

  auto result = RunWithArgs({-3, 1, 50, -5}, {0, -8, -10, 2}, {0, 2, -5, 8});
  EXPECT_EQ(glsl::ivec4Ty(0, 1, -5, 2), result);
}

class op_glsl_NClamp_float_float_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy,
                             glsl::floatTy> {
 public:
  op_glsl_NClamp_float_float_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy,
                        glsl::floatTy>(
            uvk::Shader::op_glsl_NClamp_float_float_float) {}
};

TEST_F(op_glsl_NClamp_float_float_float, Smoke) { RunWithArgs(2, 2, 2); }

class op_glsl_NClamp_vec2_vec2_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty,
                             glsl::vec2Ty> {
 public:
  op_glsl_NClamp_vec2_vec2_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_NClamp_vec2_vec2_vec2) {}
};

TEST_F(op_glsl_NClamp_vec2_vec2_vec2, Smoke) {
  RunWithArgs({2, 2}, {2, 2}, {2, 2});
}

class op_glsl_NClamp_vec3_vec3_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty,
                             glsl::vec3Ty> {
 public:
  op_glsl_NClamp_vec3_vec3_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_NClamp_vec3_vec3_vec3) {}
};

TEST_F(op_glsl_NClamp_vec3_vec3_vec3, Smoke) {
  RunWithArgs({2, 2, 2}, {2, 2, 2}, {2, 2, 2});
}

class op_glsl_NClamp_vec4_vec4_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty,
                             glsl::vec4Ty> {
 public:
  op_glsl_NClamp_vec4_vec4_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_NClamp_vec4_vec4_vec4) {}
};

TEST_F(op_glsl_NClamp_vec4_vec4_vec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2});
}

class op_glsl_NClamp_double_double_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy,
                             glsl::doubleTy> {
 public:
  op_glsl_NClamp_double_double_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy,
                        glsl::doubleTy>(
            uvk::Shader::op_glsl_NClamp_double_double_double) {}
};

TEST_F(op_glsl_NClamp_double_double_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2, 2, 2);
  }
}

class op_glsl_NClamp_dvec2_dvec2_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty,
                             glsl::dvec2Ty> {
 public:
  op_glsl_NClamp_dvec2_dvec2_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty,
                        glsl::dvec2Ty>(
            uvk::Shader::op_glsl_NClamp_dvec2_dvec2_dvec2) {}
};

TEST_F(op_glsl_NClamp_dvec2_dvec2_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2}, {2, 2}, {2, 2});
  }
}

class op_glsl_NClamp_dvec3_dvec3_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty,
                             glsl::dvec3Ty> {
 public:
  op_glsl_NClamp_dvec3_dvec3_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty,
                        glsl::dvec3Ty>(
            uvk::Shader::op_glsl_NClamp_dvec3_dvec3_dvec3) {}
};

TEST_F(op_glsl_NClamp_dvec3_dvec3_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2}, {2, 2, 2}, {2, 2, 2});
  }
}

class op_glsl_NClamp_dvec4_dvec4_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty,
                             glsl::dvec4Ty> {
 public:
  op_glsl_NClamp_dvec4_dvec4_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty,
                        glsl::dvec4Ty>(
            uvk::Shader::op_glsl_NClamp_dvec4_dvec4_dvec4) {}
};

TEST_F(op_glsl_NClamp_dvec4_dvec4_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2});
  }
}

TEST_F(op_glsl_NClamp_float_float_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is min(max(x, minVal), maxVal). Result is undefined if minVal >
  //   maxVal. The semantics used by min() and max() are those of NMin and NMax.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   NClamp(NaN, 2.3, 8.3) = 2.3

  auto result = RunWithArgs(F_NAN, 2.3f, 8.3f);
  EXPECT_TRUE(glsl::fuzzyEq(2.3f, result));
}

TEST_F(op_glsl_NClamp_vec4_vec4_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is min(max(x, minVal), maxVal). Result is undefined if minVal >
  //   maxVal. The semantics used by min() and max() are those of NMin and NMax.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   NClamp(<0.0, -0.99, NaN, NaN>, <0.5, 0.99, -1000.0, 0.001>,
  //          <0.8, 2.02, 25.02, 0.002>)
  //        = <0.5, 0.99, -1000.0, 0.0001>

  auto result =
      RunWithArgs({0.0f, -0.99f, F_NAN, F_NAN}, {0.5f, 0.99f, -1000.0f, 0.001f},
                  {0.8f, 2.02f, 25.02f, 0.002f});
  EXPECT_TRUE(glsl::fuzzyEq({0.5f, 0.99f, -1000.0f, 0.001f}, result));
}

TEST_F(op_glsl_NClamp_double_double_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is min(max(x, minVal), maxVal). Result is undefined if minVal >
  //   maxVal. The semantics used by min() and max() are those of NMin and NMax.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   NClamp(NaN, 2.3, 8.3) = 2.3
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(D_NAN, 2.3, 8.3);
    EXPECT_TRUE(glsl::fuzzyEq(2.3, result));
  }
}

TEST_F(op_glsl_NClamp_dvec4_dvec4_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is min(max(x, minVal), maxVal). Result is undefined if minVal >
  //   maxVal. The semantics used by min() and max() are those of NMin and NMax.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   NClamp(<0.0, -0.99, NaN, NaN>, <0.5, 0.99, -1000.0, 0.001>,
  //          <0.8, 2.02, 25.02, 0.002>)
  //        = <0.5, 0.99, -1000.0, 0.0001>
  if (deviceFeatures.shaderFloat64) {
    auto result =
        RunWithArgs({0.0, -0.99, D_NAN, D_NAN}, {0.5, 0.99, -1000.0, 0.001},
                    {0.8, 2.02, 25.02, 0.002});
    EXPECT_TRUE(glsl::fuzzyEq({0.5, 0.99, -1000.0, 0.001}, result));
  }
}
