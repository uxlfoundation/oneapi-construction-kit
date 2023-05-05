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

// None of the tests in this file check the precision of the operations, rather
// they check that the function acts as expected for a limited number of
// argument combinations. Some tests do also verify results when the function
// is passed edge case values such as infinity and NaNs.

class op_glsl_Round_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Round_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Round_float) {}
};

TEST_F(op_glsl_Round_float, Smoke) { RunWithArgs(2); }

class op_glsl_Round_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Round_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Round_vec2) {}
};

TEST_F(op_glsl_Round_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Round_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Round_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Round_vec3) {}
};

TEST_F(op_glsl_Round_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Round_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Round_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Round_vec4) {}
};

TEST_F(op_glsl_Round_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

class op_glsl_Round_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_Round_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_Round_double) {}
};

TEST_F(op_glsl_Round_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2);
  }
}

class op_glsl_Round_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_Round_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_Round_dvec2) {}
};

TEST_F(op_glsl_Round_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2});
  }
}

class op_glsl_Round_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_Round_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_Round_dvec3) {}
};

TEST_F(op_glsl_Round_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2});
  }
}

class op_glsl_Round_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_Round_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_Round_dvec4) {}
};

TEST_F(op_glsl_Round_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2});
  }
}

TEST_F(op_glsl_Round_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number to x.
  //   The fraction 0.5 will round in a direction chosen by the implementation,
  //   presumably the direction that is fastest. This includes the possibility
  //   that Round x is the same value as RoundEven x for all values of x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Round(5.01) = 5.0

  auto result = RunWithArgs(5.01f);
  EXPECT_TRUE(glsl::fuzzyEq(5.0f, result));
}

TEST_F(op_glsl_Round_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number to x.
  //   The fraction 0.5 will round in a direction chosen by the implementation,
  //   presumably the direction that is fastest. This includes the possibility
  //   that Round x is the same value as RoundEven x for all values of x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Round(<0.01, 0.99, -0.5, -5.0>) = <0.0, 1.0, -1, -5.0>

  auto result = RunWithArgs({0.01f, 0.99f, -0.5f, -5.0f});
  EXPECT_TRUE(glsl::fuzzyEq({0.0f, 1.0f, -1.0f, -5.0f}, result));
}

TEST_F(op_glsl_Round_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number to x.
  //   The fraction 0.5 will round in a direction chosen by the implementation,
  //   presumably the direction that is fastest. This includes the possibility
  //   that Round x is the same value as RoundEven x for all values of x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Round(4.99) = 5.0
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(4.99);
    EXPECT_TRUE(glsl::fuzzyEq(5.0, result));
  }
}

TEST_F(op_glsl_Round_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number to x.
  //   The fraction 0.5 will round in a direction chosen by the implementation,
  //   presumably the direction that is fastest. This includes the possibility
  //   that Round x is the same value as RoundEven x for all values of x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Round(<1000000.01, -10000000.99, 0.5, 5.0>) = <1000000.0, -10000001.0,
  //   1, 5.0>
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({1000000.01, -10000000.99, 0.5, 5.0});
    EXPECT_TRUE(glsl::fuzzyEq({1000000.0, -10000001.0, 1.0, 5.0}, result));
  }
}

class op_glsl_RoundEven_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_RoundEven_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_RoundEven_float) {}
};

TEST_F(op_glsl_RoundEven_float, Smoke) { RunWithArgs(2); }

class op_glsl_RoundEven_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_RoundEven_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_RoundEven_vec2) {}
};

TEST_F(op_glsl_RoundEven_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_RoundEven_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_RoundEven_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_RoundEven_vec3) {}
};

TEST_F(op_glsl_RoundEven_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_RoundEven_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_RoundEven_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_RoundEven_vec4) {}
};

TEST_F(op_glsl_RoundEven_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

class op_glsl_RoundEven_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_RoundEven_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_RoundEven_double) {}
};

TEST_F(op_glsl_RoundEven_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2);
  }
}

class op_glsl_RoundEven_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_RoundEven_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_RoundEven_dvec2) {}
};

TEST_F(op_glsl_RoundEven_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2});
  }
}

class op_glsl_RoundEven_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_RoundEven_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_RoundEven_dvec3) {}
};

TEST_F(op_glsl_RoundEven_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2});
  }
}

class op_glsl_RoundEven_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_RoundEven_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_RoundEven_dvec4) {}
};

TEST_F(op_glsl_RoundEven_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2});
  }
}

TEST_F(op_glsl_RoundEven_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number to x.
  //   A fractional part of 0.5 will round toward the nearest even whole number.
  //   (Both 3.5 and 4.5 for x will be 4.0.)
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   RoundEven(4.5) = 4.0

  auto result = RunWithArgs(4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(4.0f, result));
}

TEST_F(op_glsl_RoundEven_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number to x.
  //   A fractional part of 0.5 will round toward the nearest even whole number.
  //   (Both 3.5 and 4.5 for x will be 4.0.)
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   RoundEven(<0.01, 0.99, -0.5, -5.0>) = <0.0, 1.0, 0.0, -5.0>

  auto result = RunWithArgs({0.01f, 0.99f, -0.5f, -5.0f});
  EXPECT_TRUE(glsl::fuzzyEq({0.0f, 1.0f, 0.0f, -5.0f}, result));
}

TEST_F(op_glsl_RoundEven_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number to x.
  //   A fractional part of 0.5 will round toward the nearest even whole number.
  //   (Both 3.5 and 4.5 for x will be 4.0.)
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   RoundEven(5.5) = 6.0
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(5.5);
    EXPECT_TRUE(glsl::fuzzyEq(6.0, result));
  }
}

TEST_F(op_glsl_RoundEven_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number to x.
  //   A fractional part of 0.5 will round toward the nearest even whole number.
  //   (Both 3.5 and 4.5 for x will be 4.0.)
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   RoundEven(<1000000.01, -10000000.99, 0.5, -4.5>) = <1000000.0,
  //   -10000001.0, 0.0, -4.0>
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({1000000.01, -10000000.99, 0.5, -4.5});
    EXPECT_TRUE(glsl::fuzzyEq({1000000.0, -10000001.0, 0.0, -4.0}, result));
  }
}

class op_glsl_Trunc_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Trunc_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Trunc_float) {}
};

TEST_F(op_glsl_Trunc_float, Smoke) { RunWithArgs(2); }

class op_glsl_Trunc_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Trunc_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Trunc_vec2) {}
};

TEST_F(op_glsl_Trunc_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Trunc_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Trunc_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Trunc_vec3) {}
};

TEST_F(op_glsl_Trunc_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Trunc_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Trunc_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Trunc_vec4) {}
};

TEST_F(op_glsl_Trunc_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

class op_glsl_Trunc_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_Trunc_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_Trunc_double) {}
};

TEST_F(op_glsl_Trunc_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2);
  }
}

class op_glsl_Trunc_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_Trunc_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_Trunc_dvec2) {}
};

TEST_F(op_glsl_Trunc_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2});
  }
}

class op_glsl_Trunc_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_Trunc_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_Trunc_dvec3) {}
};

TEST_F(op_glsl_Trunc_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2});
  }
}

class op_glsl_Trunc_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_Trunc_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_Trunc_dvec4) {}
};

TEST_F(op_glsl_Trunc_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2});
  }
}

TEST_F(op_glsl_Trunc_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number to x
  //   whose absolute value is not larger than the absolute value of x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Trunc(4.5) = 4.0

  auto result = RunWithArgs(4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(4.0f, result));
}

TEST_F(op_glsl_Trunc_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number to x
  //   whose absolute value is not larger than the absolute value of x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Trunc(<0.01, 0.99, -0.5, -5.0>) = <0.0, 0.0, 0.0, -5.0>

  auto result = RunWithArgs({0.01f, 0.99f, -0.5f, -5.0f});
  EXPECT_TRUE(glsl::fuzzyEq({0.0f, 0.0f, 0.0f, -5.0f}, result));
}

TEST_F(op_glsl_Trunc_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number to x
  //   whose absolute value is not larger than the absolute value of x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Trunc(5.5) = 5.0
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(5.5);
    EXPECT_TRUE(glsl::fuzzyEq(5.0, result));
  }
}

TEST_F(op_glsl_Trunc_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number to x
  //   whose absolute value is not larger than the absolute value of x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Trunc(<1000000.01, -10000000.99, 0.5, -4.5>) = <1000000.0, -10000000.0,
  //   0.0, -4.0>
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({1000000.01, -10000000.99, 0.5, -4.5});
    EXPECT_TRUE(glsl::fuzzyEq({1000000.0, -10000000.0, 0.0, -4.0}, result));
  }
}

class op_glsl_Floor_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Floor_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Floor_float) {}
};

TEST_F(op_glsl_Floor_float, Smoke) { RunWithArgs(2); }

class op_glsl_Floor_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Floor_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Floor_vec2) {}
};

TEST_F(op_glsl_Floor_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Floor_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Floor_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Floor_vec3) {}
};

TEST_F(op_glsl_Floor_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Floor_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Floor_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Floor_vec4) {}
};

TEST_F(op_glsl_Floor_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

class op_glsl_Floor_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_Floor_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_Floor_double) {}
};

TEST_F(op_glsl_Floor_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2);
  }
}

class op_glsl_Floor_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_Floor_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_Floor_dvec2) {}
};

TEST_F(op_glsl_Floor_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2});
  }
}

class op_glsl_Floor_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_Floor_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_Floor_dvec3) {}
};

TEST_F(op_glsl_Floor_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2});
  }
}

class op_glsl_Floor_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_Floor_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_Floor_dvec4) {}
};

TEST_F(op_glsl_Floor_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2});
  }
}

TEST_F(op_glsl_Floor_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number that is less than
  //   or equal to x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Floor(4.5) = 4.0

  auto result = RunWithArgs(4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(4.0f, result));
}

TEST_F(op_glsl_Floor_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number that is less than
  //   or equal to x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Floor(<0.01, 0.99, -0.5, -5.0>) = <0.0, 0.0, -1.0, -5.0>

  auto result = RunWithArgs({0.01f, 0.99f, -0.5f, -5.0f});
  EXPECT_TRUE(glsl::fuzzyEq({0.0f, 0.0f, -1.0f, -5.0f}, result));
}

TEST_F(op_glsl_Floor_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number that is less than
  //   or equal to x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Floor(5.5) = 5.0
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(5.5);
    EXPECT_TRUE(glsl::fuzzyEq(5.0, result));
  }
}

TEST_F(op_glsl_Floor_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number that is less than
  //   or equal to x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Floor(<1000000.01, -10000000.99, 0.5, -4.5>) = <1000000.0, -10000001.0,
  //   0.0, -5.0>
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({1000000.01, -10000000.99, 0.5, -4.5});
    EXPECT_TRUE(glsl::fuzzyEq({1000000.0, -10000001.0, 0.0, -5.0}, result));
  }
}

class op_glsl_Ceil_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Ceil_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Ceil_float) {}
};

TEST_F(op_glsl_Ceil_float, Smoke) { RunWithArgs(2); }

class op_glsl_Ceil_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Ceil_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Ceil_vec2) {}
};

TEST_F(op_glsl_Ceil_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Ceil_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Ceil_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Ceil_vec3) {}
};

TEST_F(op_glsl_Ceil_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Ceil_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Ceil_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Ceil_vec4) {}
};

TEST_F(op_glsl_Ceil_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

class op_glsl_Ceil_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_Ceil_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_Ceil_double) {}
};

TEST_F(op_glsl_Ceil_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2);
  }
}

class op_glsl_Ceil_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_Ceil_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_Ceil_dvec2) {}
};

TEST_F(op_glsl_Ceil_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2});
  }
}

class op_glsl_Ceil_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_Ceil_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_Ceil_dvec3) {}
};

TEST_F(op_glsl_Ceil_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2});
  }
}

class op_glsl_Ceil_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_Ceil_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_Ceil_dvec4) {}
};

TEST_F(op_glsl_Ceil_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2});
  }
}

TEST_F(op_glsl_Ceil_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number that is greater
  //   than or equal to x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Ceil(4.5) = 5.0

  auto result = RunWithArgs(4.5f);
  EXPECT_TRUE(glsl::fuzzyEq(5.0f, result));
}

TEST_F(op_glsl_Ceil_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number that is greater
  //   than or equal to x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Ceil(<0.01, 0.99, -0.5, -5.0>) = <1.0, 1.0, 0.0, -5.0>

  auto result = RunWithArgs({0.01f, 0.99f, -0.5f, -5.0f});
  EXPECT_TRUE(glsl::fuzzyEq({1.0f, 1.0f, 0.0f, -5.0f}, result));
}

TEST_F(op_glsl_Ceil_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number that is greater
  //   than or equal to x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Ceil(5.5) = 6.0
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(5.5);
    EXPECT_TRUE(glsl::fuzzyEq(6.0, result));
  }
}

TEST_F(op_glsl_Ceil_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Result is the value equal to the nearest whole number that is greater
  //   than or equal to x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Ceil(<1000000.01, -10000000.99, 0.5, -4.5>) = <1000001.0,
  //   -10000000.0, 1.0, -4.0>
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({1000000.01, -10000000.99, 0.5, -4.5});
    EXPECT_TRUE(glsl::fuzzyEq({1000001.0, -10000000.0, 1.0, -4.0}, result));
  }
}

class op_glsl_Fract_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Fract_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Fract_float) {}
};

TEST_F(op_glsl_Fract_float, Smoke) { RunWithArgs(2); }

class op_glsl_Fract_vec2 : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Fract_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Fract_vec2) {}
};

TEST_F(op_glsl_Fract_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_Fract_vec3 : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Fract_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Fract_vec3) {}
};

TEST_F(op_glsl_Fract_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_Fract_vec4 : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Fract_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Fract_vec4) {}
};

TEST_F(op_glsl_Fract_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

class op_glsl_Fract_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_Fract_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_Fract_double) {}
};

TEST_F(op_glsl_Fract_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2);
  }
}

class op_glsl_Fract_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_Fract_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_Fract_dvec2) {}
};

TEST_F(op_glsl_Fract_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2});
  }
}

class op_glsl_Fract_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_Fract_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_Fract_dvec3) {}
};

TEST_F(op_glsl_Fract_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2});
  }
}

class op_glsl_Fract_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_Fract_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_Fract_dvec4) {}
};

TEST_F(op_glsl_Fract_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2});
  }
}

// Tests that Fract is correctly implemented
TEST_F(op_glsl_Fract_float, BasicCorrectnessTest) {
  // From specification:
  //   Result is x - floor x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Fract(123.456) = 0.456

  auto result = RunWithArgs(123.456f);
  EXPECT_TRUE(glsl::fuzzyEq(0.456f, result));
}

TEST_F(op_glsl_Fract_vec3, BasicCorrectnessTest) {
  // From specification:
  //   Result is x - floor x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Fract(<3.14, 1.23, -4.77>) = <0.14, 0.23, 0.23>

  auto result = RunWithArgs({3.14f, 1.23f, -4.77f});
  EXPECT_TRUE(glsl::fuzzyEq(glsl::vec3Ty(0.14f, 0.23f, 0.23f), result));
}

TEST_F(op_glsl_Fract_double, BasicCorrectnessTest) {
  // From specification:
  //   Result is x - floor x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Fract(3.14) = 0.14
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(3.14);
    EXPECT_TRUE(glsl::fuzzyEq(0.14, result));
  }
}

TEST_F(op_glsl_Fract_dvec3, BasicCorrectnessTest) {
  // From specification:
  //   Result is x - floor x.
  //
  //   The operand x must be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Fract(<3.14, -1.23, 4.77>) = <0.14, 0.77, 0.77>
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({3.14, -1.23, 4.77});
    EXPECT_TRUE(glsl::fuzzyEq(glsl::dvec3Ty(0.14, 0.77, 0.77), result));
  }
}
