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

#include "GLSLTestDefs.h"

class op_glsl_Length_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Length_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Length_float) {}
};

TEST_F(op_glsl_Length_float, Smoke) { RunWithArgs(2); }

class op_glsl_Length_vec2
    : public GlslBuiltinTest<glsl::floatTy, glsl::vec2Ty> {
 public:
  op_glsl_Length_vec2()
      : GlslBuiltinTest<glsl::floatTy, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Length_vec2) {}
};

TEST_F(op_glsl_Length_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Length_vec3
    : public GlslBuiltinTest<glsl::floatTy, glsl::vec3Ty> {
 public:
  op_glsl_Length_vec3()
      : GlslBuiltinTest<glsl::floatTy, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Length_vec3) {}
};

TEST_F(op_glsl_Length_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Length_vec4
    : public GlslBuiltinTest<glsl::floatTy, glsl::vec4Ty> {
 public:
  op_glsl_Length_vec4()
      : GlslBuiltinTest<glsl::floatTy, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Length_vec4) {}
};

TEST_F(op_glsl_Length_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

class op_glsl_Length_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_Length_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_Length_double) {}
};

TEST_F(op_glsl_Length_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2);
  }
}

class op_glsl_Length_dvec2
    : public GlslBuiltinTest<glsl::doubleTy, glsl::dvec2Ty> {
 public:
  op_glsl_Length_dvec2()
      : GlslBuiltinTest<glsl::doubleTy, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_Length_dvec2) {}
};

TEST_F(op_glsl_Length_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2});
  }
}

class op_glsl_Length_dvec3
    : public GlslBuiltinTest<glsl::doubleTy, glsl::dvec3Ty> {
 public:
  op_glsl_Length_dvec3()
      : GlslBuiltinTest<glsl::doubleTy, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_Length_dvec3) {}
};

TEST_F(op_glsl_Length_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2});
  }
}

class op_glsl_Length_dvec4
    : public GlslBuiltinTest<glsl::doubleTy, glsl::dvec4Ty> {
 public:
  op_glsl_Length_dvec4()
      : GlslBuiltinTest<glsl::doubleTy, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_Length_dvec4) {}
};

TEST_F(op_glsl_Length_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2});
  }
}

TEST_F(op_glsl_Length_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is the length of vector x, i.e., sqrt(x [0]^2 + x [1]^2 + …).
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type must be a scalar of the same type as the component type of x.
  // Expected results:
  //   Length(-4.5) = 4.5

  auto result = RunWithArgs(-4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(4.5f, result));
}

TEST_F(op_glsl_Length_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the length of vector x, i.e., sqrt(x [0]^2 + x [1]^2 + …).
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type must be a scalar of the same type as the component type of x.
  // Expected results:
  //   Length(<0.0, 5.05, 0.01, -100.02>) = 100.147406357

  auto result = RunWithArgs({0.0f, 5.05f, 0.01f, -100.02f});
  EXPECT_TRUE(glsl::fuzzyEq(100.147406357f, result));
}

TEST_F(op_glsl_Length_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is the length of vector x, i.e., sqrt(x [0]^2 + x [1]^2 + …).
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type must be a scalar of the same type as the component type of x.
  // Expected results:
  //   Length(0.0) = 0.0
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(0.0);
    EXPECT_TRUE(glsl::fuzzyEq(0.0, result));
  }
}

TEST_F(op_glsl_Length_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the length of vector x, i.e., sqrt(x [0]^2 + x [1]^2 + …).
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type must be a scalar of the same type as the component type of x.
  // Expected results:
  //   Length(<100.0, 0.125, -0.5, -668.001>) = 675.444743577
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({100.0, 0.125, -0.5, -668.001});
    EXPECT_TRUE(glsl::fuzzyEq(675.444743577, result));
  }
}

class op_glsl_Distance_float_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Distance_float_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Distance_float_float) {}
};

TEST_F(op_glsl_Distance_float_float, Smoke) { RunWithArgs(2, 2); }

class op_glsl_Distance_vec2_vec2
    : public GlslBuiltinTest<glsl::floatTy, glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Distance_vec2_vec2()
      : GlslBuiltinTest<glsl::floatTy, glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Distance_vec2_vec2) {}
};

TEST_F(op_glsl_Distance_vec2_vec2, Smoke) { RunWithArgs({2, 2}, {2, 2}); }

class op_glsl_Distance_vec3_vec3
    : public GlslBuiltinTest<glsl::floatTy, glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Distance_vec3_vec3()
      : GlslBuiltinTest<glsl::floatTy, glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Distance_vec3_vec3) {}
};

TEST_F(op_glsl_Distance_vec3_vec3, Smoke) { RunWithArgs({2, 2, 2}, {2, 2, 2}); }

class op_glsl_Distance_vec4_vec4
    : public GlslBuiltinTest<glsl::floatTy, glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Distance_vec4_vec4()
      : GlslBuiltinTest<glsl::floatTy, glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Distance_vec4_vec4) {}
};

TEST_F(op_glsl_Distance_vec4_vec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
}

class op_glsl_Distance_double_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_Distance_double_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_Distance_double_double) {}
};

TEST_F(op_glsl_Distance_double_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2, 2);
  }
}

class op_glsl_Distance_dvec2_dvec2
    : public GlslBuiltinTest<glsl::doubleTy, glsl::dvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_Distance_dvec2_dvec2()
      : GlslBuiltinTest<glsl::doubleTy, glsl::dvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_Distance_dvec2_dvec2) {}
};

TEST_F(op_glsl_Distance_dvec2_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2}, {2, 2});
  }
}

class op_glsl_Distance_dvec3_dvec3
    : public GlslBuiltinTest<glsl::doubleTy, glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_Distance_dvec3_dvec3()
      : GlslBuiltinTest<glsl::doubleTy, glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_Distance_dvec3_dvec3) {}
};

TEST_F(op_glsl_Distance_dvec3_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2}, {2, 2, 2});
  }
}

class op_glsl_Distance_dvec4_dvec4
    : public GlslBuiltinTest<glsl::doubleTy, glsl::dvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_Distance_dvec4_dvec4()
      : GlslBuiltinTest<glsl::doubleTy, glsl::dvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_Distance_dvec4_dvec4) {}
};

TEST_F(op_glsl_Distance_dvec4_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
  }
}

TEST_F(op_glsl_Distance_float_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is the distance between p0 and p1, i.e., length(p0 - p1).
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type must be a scalar of the same type as the component type of
  //   the operands.
  // Expected results:
  //   Distance(2.3, 4.5) = 2.2

  auto result = RunWithArgs(2.3f, 4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(2.2f, result));
}

TEST_F(op_glsl_Distance_vec4_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the distance between p0 and p1, i.e., length(p0 - p1).
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type must be a scalar of the same type as the component type of
  //   the operands.
  // Expected results:
  //   Distance(<0.0, -0.99, 50.25, -5.45>, <0.5, 0.99, 0.001, -2.23>) =
  //     50.393459903

  auto result = RunWithArgs({0.0f, -0.99f, 50.25f, -5.45f},
                            {0.5f, 0.99f, 0.001f, -2.23f});
  EXPECT_TRUE(glsl::fuzzyEq(50.393459903f, result));
}

TEST_F(op_glsl_Distance_double_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is the distance between p0 and p1, i.e., length(p0 - p1).
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type must be a scalar of the same type as the component type of
  //   the operands.
  // Expected results:
  //   Distance(2.3, 0.001) = 2.299
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(2.3, 0.001);
    EXPECT_TRUE(glsl::fuzzyEq(2.299, result));
  }
}

TEST_F(op_glsl_Distance_dvec4_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the distance between p0 and p1, i.e., length(p0 - p1).
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type must be a scalar of the same type as the component type of
  //   the operands.
  // Expected results:
  //   Distance(<0.499, 0.0, -0.0, -5.45>, <0.5, 0.99, 0.001, 2.23>) =
  //     7.743552221
  if (deviceFeatures.shaderFloat64) {
    auto result =
        RunWithArgs({0.499, 0.0, -0.0, -5.45}, {0.5, 0.99, 0.001, 2.23});
    EXPECT_TRUE(glsl::fuzzyEq(7.743552221, result));
  }
}

class op_glsl_Normalize_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Normalize_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Normalize_float) {}
};

TEST_F(op_glsl_Normalize_float, Smoke) { RunWithArgs(2); }

class op_glsl_Normalize_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Normalize_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Normalize_vec2) {}
};

TEST_F(op_glsl_Normalize_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Normalize_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Normalize_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Normalize_vec3) {}
};

TEST_F(op_glsl_Normalize_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Normalize_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Normalize_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Normalize_vec4) {}
};

TEST_F(op_glsl_Normalize_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

class op_glsl_Normalize_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_Normalize_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_Normalize_double) {}
};

TEST_F(op_glsl_Normalize_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2);
  }
}

class op_glsl_Normalize_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_Normalize_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_Normalize_dvec2) {}
};

TEST_F(op_glsl_Normalize_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2});
  }
}

class op_glsl_Normalize_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_Normalize_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_Normalize_dvec3) {}
};

TEST_F(op_glsl_Normalize_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2});
  }
}

class op_glsl_Normalize_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_Normalize_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_Normalize_dvec4) {}
};

TEST_F(op_glsl_Normalize_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2});
  }
}

TEST_F(op_glsl_Normalize_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is the vector in the same direction as x but with a length of 1.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type.
  // Expected results:
  //   Normalize(2.3) = 1.0

  auto result = RunWithArgs(2.3f);
  EXPECT_TRUE(glsl::fuzzyEq(1.0f, result));
}

TEST_F(op_glsl_Normalize_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the vector in the same direction as x but with a length of 1.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type.
  // Expected results:
  //   Normalize(<0.0, -0.99, 50.25, -5.45>) =
  //     <0.0, -0.01958, 0.99398, -0.10780>

  auto result = RunWithArgs({0.0f, -0.99f, 50.25f, -5.45f});
  EXPECT_TRUE(glsl::fuzzyEq({0.0f, -0.01958f, 0.99398f, -0.10780f}, result));
}

TEST_F(op_glsl_Normalize_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is the vector in the same direction as x but with a length of 1.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type.
  // Expected results:
  //   Normalize(1.0) = 1.0
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(1.0);
    EXPECT_TRUE(glsl::fuzzyEq(1.0, result));
  }
}

TEST_F(op_glsl_Normalize_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the vector in the same direction as x but with a length of 1.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type.
  // Expected results:
  //   Normalize(<78.499, -6.99, 0.001, -0.001>) =
  //     <0.99606, -0.08869, 0.00001, -0.00001>
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({78.499, -6.99, 0.001, -0.001});
    EXPECT_TRUE(glsl::fuzzyEq({0.99606, -0.08869, 0.00001, -0.00001}, result));
  }
}
