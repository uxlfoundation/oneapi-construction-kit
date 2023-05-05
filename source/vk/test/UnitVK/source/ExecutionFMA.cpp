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

class op_glsl_Fma_float_float_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy,
                             glsl::floatTy> {
 public:
  op_glsl_Fma_float_float_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy,
                        glsl::floatTy>(
            uvk::Shader::op_glsl_Fma_float_float_float) {}
};

TEST_F(op_glsl_Fma_float_float_float, Smoke) { RunWithArgs(2, 2, 2); }

class op_glsl_Fma_vec2_vec2_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty,
                             glsl::vec2Ty> {
 public:
  op_glsl_Fma_vec2_vec2_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Fma_vec2_vec2_vec2) {}
};

TEST_F(op_glsl_Fma_vec2_vec2_vec2, Smoke) {
  RunWithArgs({2, 2}, {2, 2}, {2, 2});
}

class op_glsl_Fma_vec3_vec3_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty,
                             glsl::vec3Ty> {
 public:
  op_glsl_Fma_vec3_vec3_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Fma_vec3_vec3_vec3) {}
};

TEST_F(op_glsl_Fma_vec3_vec3_vec3, Smoke) {
  RunWithArgs({2, 2, 2}, {2, 2, 2}, {2, 2, 2});
}

class op_glsl_Fma_vec4_vec4_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty,
                             glsl::vec4Ty> {
 public:
  op_glsl_Fma_vec4_vec4_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Fma_vec4_vec4_vec4) {}
};

TEST_F(op_glsl_Fma_vec4_vec4_vec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2});
}

class op_glsl_Fma_double_double_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy,
                             glsl::doubleTy> {
 public:
  op_glsl_Fma_double_double_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy,
                        glsl::doubleTy>(
            uvk::Shader::op_glsl_Fma_double_double_double) {}
};

TEST_F(op_glsl_Fma_double_double_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2, 2, 2);
  }
}

class op_glsl_Fma_dvec2_dvec2_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty,
                             glsl::dvec2Ty> {
 public:
  op_glsl_Fma_dvec2_dvec2_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty,
                        glsl::dvec2Ty>(
            uvk::Shader::op_glsl_Fma_dvec2_dvec2_dvec2) {}
};

TEST_F(op_glsl_Fma_dvec2_dvec2_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2}, {2, 2}, {2, 2});
  }
}

class op_glsl_Fma_dvec3_dvec3_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty,
                             glsl::dvec3Ty> {
 public:
  op_glsl_Fma_dvec3_dvec3_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty,
                        glsl::dvec3Ty>(
            uvk::Shader::op_glsl_Fma_dvec3_dvec3_dvec3) {}
};

TEST_F(op_glsl_Fma_dvec3_dvec3_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2}, {2, 2, 2}, {2, 2, 2});
  }
}

class op_glsl_Fma_dvec4_dvec4_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty,
                             glsl::dvec4Ty> {
 public:
  op_glsl_Fma_dvec4_dvec4_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty,
                        glsl::dvec4Ty>(
            uvk::Shader::op_glsl_Fma_dvec4_dvec4_dvec4) {}
};

TEST_F(op_glsl_Fma_dvec4_dvec4_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2});
  }
}

TEST_F(op_glsl_Fma_float_float_float, BasicCorrectnessTest) {
  // From specification:
  //   Computes a * b + c. In uses where this operation is decorated with
  //   NoContraction:
  //
  //   - fma is considered a single operation, whereas the expression
  //     a * b + c is considered two operations.
  //
  //   - The precision of fma can differ from the precision of the expression a
  //   * b + c.
  //
  //   - fma will be computed with the same precision as any other fma decorated
  //   with NoContraction, giving invariant results for the same input
  //   values of a, b, and c.
  //
  //   Otherwise, in the absence of a NoContraction decoration,
  //   there are no special constraints on the number of operations or
  //   difference in precision between fma and the expression a * b +c.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Fma(2.3, 4.5, 3.3) = 13.65

  auto result = RunWithArgs(2.3f, 4.5f, 3.3f);
  EXPECT_TRUE(glsl::fuzzyEq(13.65f, result));
}

TEST_F(op_glsl_Fma_vec4_vec4_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Computes a * b + c. In uses where this operation is decorated with
  //   NoContraction:
  //
  //   - fma is considered a single operation, whereas the expression
  //     a * b + c is considered two operations.
  //
  //   - The precision of fma can differ from the precision of the expression a
  //   * b + c.
  //
  //   - fma will be computed with the same precision as any other fma decorated
  //   with NoContraction, giving invariant results for the same input
  //   values of a, b, and c.
  //
  //   Otherwise, in the absence of a NoContraction decoration,
  //   there are no special constraints on the number of operations or
  //   difference in precision between fma and the expression a * b +c.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Fma(<0.0, 0.0, -1.0, 5.0>, <0.5, 0.5, 5.0, 99.0>, <0.0, 0.5, -0.5,
  //   101.001>)
  //     = <0.0, 0.5,-5.5, 596.001>

  auto result =
      RunWithArgs({0.0f, 0.0f, -1.0f, 5.0f}, {0.5f, 0.5f, 5.0f, 99.0f},
                  {0.0f, 0.5f, -0.5f, 101.001f});
  EXPECT_TRUE(glsl::fuzzyEq({0.0f, 0.5f, -5.5f, 596.001f}, result));
}

TEST_F(op_glsl_Fma_double_double_double, BasicCorrectnessTest) {
  // From specification:
  //   Computes a * b + c. In uses where this operation is decorated with
  //   NoContraction:
  //
  //   - fma is considered a single operation, whereas the expression
  //     a * b + c is considered two operations.
  //
  //   - The precision of fma can differ from the precision of the expression a
  //   * b + c.
  //
  //   - fma will be computed with the same precision as any other fma decorated
  //   with NoContraction, giving invariant results for the same input
  //   values of a, b, and c.
  //
  //   Otherwise, in the absence of a NoContraction decoration,
  //   there are no special constraints on the number of operations or
  //   difference in precision between fma and the expression a * b +c.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Fma(2.3, 4.5, -3.3) = 7.05

  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(2.3, 4.5, -3.3);
    EXPECT_TRUE(glsl::fuzzyEq(7.05, result));
  }
}

TEST_F(op_glsl_Fma_dvec4_dvec4_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Computes a * b + c. In uses where this operation is decorated with
  //   NoContraction:
  //
  //   - fma is considered a single operation, whereas the expression
  //     a * b + c is considered two operations.
  //
  //   - The precision of fma can differ from the precision of the expression a
  //   * b + c.
  //
  //   - fma will be computed with the same precision as any other fma decorated
  //   with NoContraction, giving invariant results for the same input
  //   values of a, b, and c.
  //
  //   Otherwise, in the absence of a NoContraction decoration,
  //   there are no special constraints on the number of operations or
  //   difference in precision between fma and the expression a * b +c.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Fma(<20.0, 0.2, -1.0, 5.0>, <0.0, 0.5, 0.0, 99.0>, <0.499, 0.5,
  //   -0.5, 4.0>)
  //     = <0.499, 0.6, -0.5, 499.0>

  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({20.0, 0.2, -1.0, 5.0}, {0.0, 0.5, 0.0, 99.0},
                              {0.499, 0.5, -0.5, 4.0});
    EXPECT_TRUE(glsl::fuzzyEq({0.499, 0.6, -0.5, 499.0}, result));
  }
}

#ifndef IGNORE_SPIRV_TESTS

class op_glsl_Fma_No_Contraction_float_float_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy,
                             glsl::floatTy> {
 public:
  op_glsl_Fma_No_Contraction_float_float_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy,
                        glsl::floatTy>(
            uvk::Shader::op_glsl_Fma_No_Contraction_float_float_float) {}
};

TEST_F(op_glsl_Fma_No_Contraction_float_float_float, Smoke) {
  RunWithArgs(2, 2, 2);
}

class op_glsl_Fma_No_Contraction_vec2_vec2_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty,
                             glsl::vec2Ty> {
 public:
  op_glsl_Fma_No_Contraction_vec2_vec2_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Fma_No_Contraction_vec2_vec2_vec2) {}
};

TEST_F(op_glsl_Fma_No_Contraction_vec2_vec2_vec2, Smoke) {
  RunWithArgs({2, 2}, {2, 2}, {2, 2});
}

class op_glsl_Fma_No_Contraction_vec3_vec3_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty,
                             glsl::vec3Ty> {
 public:
  op_glsl_Fma_No_Contraction_vec3_vec3_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Fma_No_Contraction_vec3_vec3_vec3) {}
};

TEST_F(op_glsl_Fma_No_Contraction_vec3_vec3_vec3, Smoke) {
  RunWithArgs({2, 2, 2}, {2, 2, 2}, {2, 2, 2});
}

class op_glsl_Fma_No_Contraction_vec4_vec4_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty,
                             glsl::vec4Ty> {
 public:
  op_glsl_Fma_No_Contraction_vec4_vec4_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Fma_No_Contraction_vec4_vec4_vec4) {}
};

TEST_F(op_glsl_Fma_No_Contraction_vec4_vec4_vec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2});
}

class op_glsl_Fma_No_Contraction_double_double_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy,
                             glsl::doubleTy> {
 public:
  op_glsl_Fma_No_Contraction_double_double_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy,
                        glsl::doubleTy>(
            uvk::Shader::op_glsl_Fma_No_Contraction_double_double_double) {}
};

TEST_F(op_glsl_Fma_No_Contraction_double_double_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2, 2, 2);
  }
}

class op_glsl_Fma_No_Contraction_dvec2_dvec2_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty,
                             glsl::dvec2Ty> {
 public:
  op_glsl_Fma_No_Contraction_dvec2_dvec2_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty,
                        glsl::dvec2Ty>(
            uvk::Shader::op_glsl_Fma_No_Contraction_dvec2_dvec2_dvec2) {}
};

TEST_F(op_glsl_Fma_No_Contraction_dvec2_dvec2_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2}, {2, 2}, {2, 2});
  }
}

class op_glsl_Fma_No_Contraction_dvec3_dvec3_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty,
                             glsl::dvec3Ty> {
 public:
  op_glsl_Fma_No_Contraction_dvec3_dvec3_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty,
                        glsl::dvec3Ty>(
            uvk::Shader::op_glsl_Fma_No_Contraction_dvec3_dvec3_dvec3) {}
};

TEST_F(op_glsl_Fma_No_Contraction_dvec3_dvec3_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2}, {2, 2, 2}, {2, 2, 2});
  }
}

class op_glsl_Fma_No_Contraction_dvec4_dvec4_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty,
                             glsl::dvec4Ty> {
 public:
  op_glsl_Fma_No_Contraction_dvec4_dvec4_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty,
                        glsl::dvec4Ty>(
            uvk::Shader::op_glsl_Fma_No_Contraction_dvec4_dvec4_dvec4) {}
};

TEST_F(op_glsl_Fma_No_Contraction_dvec4_dvec4_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2});
  }
}

TEST_F(op_glsl_Fma_No_Contraction_float_float_float, BasicCorrectnessTest) {
  // From specification:
  //   Computes a * b + c. In uses where this operation is decorated with
  //   NoContraction:
  //
  //   - fma is considered a single operation, whereas the expression
  //     a * b + c is considered two operations.
  //
  //   - The precision of fma can differ from the precision of the expression a
  //   * b + c.
  //
  //   - fma will be computed with the same precision as any other fma decorated
  //   with NoContraction, giving invariant results for the same input
  //   values of a, b, and c.
  //
  //   Otherwise, in the absence of a NoContraction decoration,
  //   there are no special constraints on the number of operations or
  //   difference in precision between fma and the expression a * b +c.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Fma_No_Contraction(2.3, 4.5, 3.3) = 13.65

  auto result = RunWithArgs(2.3f, 4.5f, 3.3f);
  EXPECT_TRUE(glsl::fuzzyEq(13.65f, result));
}

TEST_F(op_glsl_Fma_No_Contraction_vec4_vec4_vec4, BasicCorrectnessTest) {
  // From specification:
  //   Computes a * b + c. In uses where this operation is decorated with
  //   NoContraction:
  //
  //   - fma is considered a single operation, whereas the expression
  //     a * b + c is considered two operations.
  //
  //   - The precision of fma can differ from the precision of the expression a
  //   * b + c.
  //
  //   - fma will be computed with the same precision as any other fma decorated
  //   with NoContraction, giving invariant results for the same input
  //   values of a, b, and c.
  //
  //   Otherwise, in the absence of a NoContraction decoration,
  //   there are no special constraints on the number of operations or
  //   difference in precision between fma and the expression a * b +c.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Fma_No_Contraction(<0.0, 0.0, -1.0, 5.0>, <0.5, 0.5, 5.0, 99.0>, <0.0,
  //   0.5, -0.5, 101.001>)
  //     = <0.0, 0.5,-5.5, 596.001>

  auto result =
      RunWithArgs({0.0f, 0.0f, -1.0f, 5.0f}, {0.5f, 0.5f, 5.0f, 99.0f},
                  {0.0f, 0.5f, -0.5f, 101.001f});
  EXPECT_TRUE(glsl::fuzzyEq({0.0f, 0.5f, -5.5f, 596.001f}, result));
}

TEST_F(op_glsl_Fma_No_Contraction_double_double_double, BasicCorrectnessTest) {
  // From specification:
  //   Computes a * b + c. In uses where this operation is decorated with
  //   NoContraction:
  //
  //   - fma is considered a single operation, whereas the expression
  //     a * b + c is considered two operations.
  //
  //   - The precision of fma can differ from the precision of the expression a
  //   * b + c.
  //
  //   - fma will be computed with the same precision as any other fma decorated
  //   with NoContraction, giving invariant results for the same input
  //   values of a, b, and c.
  //
  //   Otherwise, in the absence of a NoContraction decoration,
  //   there are no special constraints on the number of operations or
  //   difference in precision between fma and the expression a * b +c.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Fma_No_Contraction(2.3, 4.5, -3.3) = 7.05

  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(2.3, 4.5, -3.3);
    EXPECT_TRUE(glsl::fuzzyEq(7.05, result));
  }
}

TEST_F(op_glsl_Fma_No_Contraction_dvec4_dvec4_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   Computes a * b + c. In uses where this operation is decorated with
  //   NoContraction:
  //
  //   - fma is considered a single operation, whereas the expression
  //     a * b + c is considered two operations.
  //
  //   - The precision of fma can differ from the precision of the expression a
  //   * b + c.
  //
  //   - fma will be computed with the same precision as any other fma decorated
  //   with NoContraction, giving invariant results for the same input
  //   values of a, b, and c.
  //
  //   Otherwise, in the absence of a NoContraction decoration,
  //   there are no special constraints on the number of operations or
  //   difference in precision between fma and the expression a * b +c.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  //   Results are computed per component.
  // Expected results:
  //   Fma_No_Contraction(<20.0, 0.2, -1.0, 5.0>, <0.0, 0.5, 0.0, 99.0>, <0.499,
  //   0.5, -0.5, 4.0>)
  //     = <0.499, 0.6, -0.5, 499.0>

  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({20.0, 0.2, -1.0, 5.0}, {0.0, 0.5, 0.0, 99.0},
                              {0.499, 0.5, -0.5, 4.0});
    EXPECT_TRUE(glsl::fuzzyEq({0.499, 0.6, -0.5, 499.0}, result));
  }
}

#endif
