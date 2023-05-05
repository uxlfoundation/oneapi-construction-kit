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

class op_glsl_FaceForward_float_float_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy,
                             glsl::floatTy> {
 public:
  op_glsl_FaceForward_float_float_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy,
                        glsl::floatTy>(
            uvk::Shader::op_glsl_FaceForward_float_float_float) {}
};

TEST_F(op_glsl_FaceForward_float_float_float, Smoke) { RunWithArgs(2, 2, 2); }

class op_glsl_FaceForward_vec2_vec2_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty,
                             glsl::vec2Ty> {
 public:
  op_glsl_FaceForward_vec2_vec2_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_FaceForward_vec2_vec2_vec2) {}
};

TEST_F(op_glsl_FaceForward_vec2_vec2_vec2, Smoke) {
  RunWithArgs({2, 2}, {2, 2}, {2, 2});
}

class op_glsl_FaceForward_vec3_vec3_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty,
                             glsl::vec3Ty> {
 public:
  op_glsl_FaceForward_vec3_vec3_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_FaceForward_vec3_vec3_vec3) {}
};

TEST_F(op_glsl_FaceForward_vec3_vec3_vec3, Smoke) {
  RunWithArgs({2, 2, 2}, {2, 2, 2}, {2, 2, 2});
}

class op_glsl_FaceForward_vec4_vec4_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty,
                             glsl::vec4Ty> {
 public:
  op_glsl_FaceForward_vec4_vec4_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_FaceForward_vec4_vec4_vec4) {}
};

TEST_F(op_glsl_FaceForward_vec4_vec4_vec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2});
}

class op_glsl_FaceForward_double_double_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy,
                             glsl::doubleTy> {
 public:
  op_glsl_FaceForward_double_double_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy,
                        glsl::doubleTy>(
            uvk::Shader::op_glsl_FaceForward_double_double_double) {}
};

TEST_F(op_glsl_FaceForward_double_double_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2, 2, 2);
  }
}

class op_glsl_FaceForward_dvec2_dvec2_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty,
                             glsl::dvec2Ty> {
 public:
  op_glsl_FaceForward_dvec2_dvec2_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty,
                        glsl::dvec2Ty>(
            uvk::Shader::op_glsl_FaceForward_dvec2_dvec2_dvec2) {}
};

TEST_F(op_glsl_FaceForward_dvec2_dvec2_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2}, {2, 2}, {2, 2});
  }
}

class op_glsl_FaceForward_dvec3_dvec3_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty,
                             glsl::dvec3Ty> {
 public:
  op_glsl_FaceForward_dvec3_dvec3_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty,
                        glsl::dvec3Ty>(
            uvk::Shader::op_glsl_FaceForward_dvec3_dvec3_dvec3) {}
};

TEST_F(op_glsl_FaceForward_dvec3_dvec3_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2}, {2, 2, 2}, {2, 2, 2});
  }
}

class op_glsl_FaceForward_dvec4_dvec4_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty,
                             glsl::dvec4Ty> {
 public:
  op_glsl_FaceForward_dvec4_dvec4_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty,
                        glsl::dvec4Ty>(
            uvk::Shader::op_glsl_FaceForward_dvec4_dvec4_dvec4) {}
};

TEST_F(op_glsl_FaceForward_dvec4_dvec4_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2});
  }
}

TEST_F(op_glsl_FaceForward_float_float_float, BasicCorrectnessTest) {
  // From specification:
  //   If the dot product of Nref and I is negative, the result is N, otherwise
  //   it is -N.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  // Expected results:
  //   FaceForward(2.3, 4.5, -8.3) = 2.3

  auto result = RunWithArgs(2.3f, 4.5f, -8.3f);
  EXPECT_TRUE(glsl::fuzzyEq(2.3f, result));
}

TEST_F(op_glsl_FaceForward_vec4_vec4_vec4, BasicCorrectnessTest) {
  // From specification:
  //   If the dot product of Nref and I is negative, the result is N, otherwise
  //   it is -N.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  // Expected results:
  //   FaceForward(<0.0, -0.99, 50.25, -5.45>, <0.5, 0.99, 0.001, -2.23>,
  //          <0.8, 2.02, 25.02, 1.0>)
  //        = <0.0, 0.99, -50.25, 5.45>

  auto result =
      RunWithArgs({0.0f, -0.99f, 50.25f, -5.45f}, {0.5f, 0.99f, 0.001f, -2.23f},
                  {0.8f, 2.02f, 25.02f, 1.0f});
  EXPECT_TRUE(glsl::fuzzyEq({0.0f, 0.99f, -50.25f, 5.45f}, result));
}

TEST_F(op_glsl_FaceForward_double_double_double, BasicCorrectnessTest) {
  // From specification:
  //   If the dot product of Nref and I is negative, the result is N, otherwise
  //   it is -N.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  // Expected results:
  //   FaceForward(36.3, 4.5, 8.3) = -36.3
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(36.3, 4.5, 8.3);
    EXPECT_TRUE(glsl::fuzzyEq(-36.3, result));
  }
}

TEST_F(op_glsl_FaceForward_dvec4_dvec4_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   If the dot product of Nref and I is negative, the result is N, otherwise
  //   it is -N.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  // Expected results:
  //   FaceForward(<1.00001, -0.99, 50.25, -5.45>, <2000.001, 0.99, 0.001,
  //   -2.23>,
  //          <0.8, 2.02, 25.02, 0.0>)
  //        = <1.00001, -0.99, 50.25, -5.45>
  if (deviceFeatures.shaderFloat64) {
    auto result =
        RunWithArgs({1.00001, -0.99, 50.25, -5.45},
                    {-2000.001, 0.99, 0.001, -2.23}, {0.8, 2.02, 25.02, 0.0});
    EXPECT_TRUE(glsl::fuzzyEq({1.00001, -0.99, 50.25, -5.45}, result));
  }
}

class op_glsl_Reflect_float_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy> {
 public:
  op_glsl_Reflect_float_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Reflect_float_float) {}
};

TEST_F(op_glsl_Reflect_float_float, Smoke) { RunWithArgs(2, 2); }

class op_glsl_Reflect_vec2_vec2
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Reflect_vec2_vec2()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Reflect_vec2_vec2) {}
};

TEST_F(op_glsl_Reflect_vec2_vec2, Smoke) { RunWithArgs({2, 2}, {2, 2}); }

class op_glsl_Reflect_vec3_vec3
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Reflect_vec3_vec3()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Reflect_vec3_vec3) {}
};

TEST_F(op_glsl_Reflect_vec3_vec3, Smoke) { RunWithArgs({2, 2, 2}, {2, 2, 2}); }

class op_glsl_Reflect_vec4_vec4
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Reflect_vec4_vec4()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Reflect_vec4_vec4) {}
};

TEST_F(op_glsl_Reflect_vec4_vec4, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
}

class op_glsl_Reflect_double_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy> {
 public:
  op_glsl_Reflect_double_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_Reflect_double_double) {}
};

TEST_F(op_glsl_Reflect_double_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2, 2);
  }
}

class op_glsl_Reflect_dvec2_dvec2
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_Reflect_dvec2_dvec2()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_Reflect_dvec2_dvec2) {}
};

TEST_F(op_glsl_Reflect_dvec2_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2}, {2, 2});
  }
}

class op_glsl_Reflect_dvec3_dvec3
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_Reflect_dvec3_dvec3()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_Reflect_dvec3_dvec3) {}
};

TEST_F(op_glsl_Reflect_dvec3_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2}, {2, 2, 2});
  }
}

class op_glsl_Reflect_dvec4_dvec4
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_Reflect_dvec4_dvec4()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_Reflect_dvec4_dvec4) {}
};

TEST_F(op_glsl_Reflect_dvec4_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2});
  }
}

TEST_F(op_glsl_Reflect_float_float, BasicCorrectnessTest) {
  // From specification:
  //   For the incident vector I and surface orientation N, the result is the
  //   reflection direction: I - 2 * dot(N, I) * N N must already be normalized
  //   in order to achieve the desired result.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  // Expected results:
  //   Reflect(2.3, 1.0) = -2.3

  auto result = RunWithArgs(2.3f, 1.0f);
  EXPECT_TRUE(glsl::fuzzyEq(-2.3f, result));
}

TEST_F(op_glsl_Reflect_vec4_vec4, BasicCorrectnessTest) {
  // From specification:
  //   For the incident vector I and surface orientation N, the result is the
  //   reflection direction: I - 2 * dot(N, I) * N N must already be normalized
  //   in order to achieve the desired result.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  // Expected results:
  //   Reflect(<0.0, -0.99, 50.25, -5.45>, <0.59757, -0.35854, 0.71709,
  //   0.01195>) =
  //     <-43.41179, 25.05693, -1.84458, -6.31813>

  auto result = RunWithArgs({0.0f, -0.99f, 50.25f, -5.45f},
                            {0.59757f, -0.35854f, 0.71709f, 0.01195f});
  EXPECT_TRUE(
      glsl::fuzzyEq({-43.41179f, 25.05693f, -1.84458f, -6.31813f}, result));
}

TEST_F(op_glsl_Reflect_double_double, BasicCorrectnessTest) {
  // From specification:
  //   For the incident vector I and surface orientation N, the result is the
  //   reflection direction: I - 2 * dot(N, I) * N N must already be normalized
  //   in order to achieve the desired result.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  // Expected results:
  //   Reflect(-0.0001, 1.0) = 0.0001
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(-0.0001, 1.0);
    EXPECT_TRUE(glsl::fuzzyEq(0.0001, result));
  }
}

TEST_F(op_glsl_Reflect_dvec4_dvec4, BasicCorrectnessTest) {
  // From specification:
  //   For the incident vector I and surface orientation N, the result is the
  //   reflection direction: I - 2 * dot(N, I) * N N must already be normalized
  //   in order to achieve the desired result.
  //
  //   The operands must all be a scalar or vector whose component type is
  //   floating-point.
  //
  //   Result Type and the type of all operands must be the same type.
  // Expected results:
  //   Reflect(<0.499, -0.99, 0.0, 5000.45>, <-0.39238, 0.88527, 0.24934,
  //   -0.01287>) =
  //     <-50.84628, 114.85290, 32.63026, 4998.76588>
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({0.499, -0.99, 0.0, 5000.45},
                              {-0.39238, 0.88527, 0.24934, -0.01287});
    EXPECT_TRUE(glsl::fuzzyEq({-50.84628, 114.85290, 32.63026, 4998.76588},
                              result, 0.01));
  }
}

class op_glsl_Refract_float_float_float
    : public GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy,
                             glsl::floatTy> {
 public:
  op_glsl_Refract_float_float_float()
      : GlslBuiltinTest<glsl::floatTy, glsl::floatTy, glsl::floatTy,
                        glsl::floatTy>(
            uvk::Shader::op_glsl_Refract_float_float_float) {}
};

TEST_F(op_glsl_Refract_float_float_float, Smoke) { RunWithArgs(2, 2, 2); }

class op_glsl_Refract_vec2_vec2_float
    : public GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty,
                             glsl::floatTy> {
 public:
  op_glsl_Refract_vec2_vec2_float()
      : GlslBuiltinTest<glsl::vec2Ty, glsl::vec2Ty, glsl::vec2Ty,
                        glsl::floatTy>(
            uvk::Shader::op_glsl_Refract_vec2_vec2_float) {}
};

TEST_F(op_glsl_Refract_vec2_vec2_float, Smoke) {
  RunWithArgs({2, 2}, {2, 2}, 2);
}

class op_glsl_Refract_vec3_vec3_float
    : public GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty,
                             glsl::floatTy> {
 public:
  op_glsl_Refract_vec3_vec3_float()
      : GlslBuiltinTest<glsl::vec3Ty, glsl::vec3Ty, glsl::vec3Ty,
                        glsl::floatTy>(
            uvk::Shader::op_glsl_Refract_vec3_vec3_float) {}
};

TEST_F(op_glsl_Refract_vec3_vec3_float, Smoke) {
  RunWithArgs({2, 2, 2}, {2, 2, 2}, 2);
}

class op_glsl_Refract_vec4_vec4_float
    : public GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty,
                             glsl::floatTy> {
 public:
  op_glsl_Refract_vec4_vec4_float()
      : GlslBuiltinTest<glsl::vec4Ty, glsl::vec4Ty, glsl::vec4Ty,
                        glsl::floatTy>(
            uvk::Shader::op_glsl_Refract_vec4_vec4_float) {}
};

TEST_F(op_glsl_Refract_vec4_vec4_float, Smoke) {
  RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, 2);
}

// Taking a double as the eta argument is not spec compliant, but we allow
// this as well to maintain compatibility with older glslang versions
class op_glsl_Refract_double_double_double
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy,
                             glsl::doubleTy> {
 public:
  op_glsl_Refract_double_double_double()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy,
                        glsl::doubleTy>(
            uvk::Shader::op_glsl_Refract_double_double_double) {}
};

TEST_F(op_glsl_Refract_double_double_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2, 2, 2);
  }
}

class op_glsl_Refract_dvec2_dvec2_double
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty,
                             glsl::doubleTy> {
 public:
  op_glsl_Refract_dvec2_dvec2_double()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty,
                        glsl::doubleTy>(
            uvk::Shader::op_glsl_Refract_dvec2_dvec2_double) {}
};

TEST_F(op_glsl_Refract_dvec2_dvec2_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2}, {2, 2}, 2);
  }
}

class op_glsl_Refract_dvec3_dvec3_double
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty,
                             glsl::doubleTy> {
 public:
  op_glsl_Refract_dvec3_dvec3_double()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty,
                        glsl::doubleTy>(
            uvk::Shader::op_glsl_Refract_dvec3_dvec3_double) {}
};

TEST_F(op_glsl_Refract_dvec3_dvec3_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2}, {2, 2, 2}, 2);
  }
}

class op_glsl_Refract_dvec4_dvec4_double
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty,
                             glsl::doubleTy> {
 public:
  op_glsl_Refract_dvec4_dvec4_double()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty,
                        glsl::doubleTy>(
            uvk::Shader::op_glsl_Refract_dvec4_dvec4_double) {}
};

TEST_F(op_glsl_Refract_dvec4_dvec4_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, 2);
  }
}

#ifndef IGNORE_SPIRV_TESTS

class op_glsl_Refract_double_double_float
    : public GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy,
                             glsl::floatTy> {
 public:
  op_glsl_Refract_double_double_float()
      : GlslBuiltinTest<glsl::doubleTy, glsl::doubleTy, glsl::doubleTy,
                        glsl::floatTy>(
            uvk::Shader::op_glsl_Refract_double_double_float) {}
};

TEST_F(op_glsl_Refract_double_double_float, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2, 2, 2);
  }
}

class op_glsl_Refract_dvec2_dvec2_float
    : public GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty,
                             glsl::floatTy> {
 public:
  op_glsl_Refract_dvec2_dvec2_float()
      : GlslBuiltinTest<glsl::dvec2Ty, glsl::dvec2Ty, glsl::dvec2Ty,
                        glsl::floatTy>(
            uvk::Shader::op_glsl_Refract_dvec2_dvec2_float) {}
};

TEST_F(op_glsl_Refract_dvec2_dvec2_float, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2}, {2, 2}, 2);
  }
}

class op_glsl_Refract_dvec3_dvec3_float
    : public GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty,
                             glsl::floatTy> {
 public:
  op_glsl_Refract_dvec3_dvec3_float()
      : GlslBuiltinTest<glsl::dvec3Ty, glsl::dvec3Ty, glsl::dvec3Ty,
                        glsl::floatTy>(
            uvk::Shader::op_glsl_Refract_dvec3_dvec3_float) {}
};

TEST_F(op_glsl_Refract_dvec3_dvec3_float, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2}, {2, 2, 2}, 2);
  }
}

class op_glsl_Refract_dvec4_dvec4_float
    : public GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty,
                             glsl::floatTy> {
 public:
  op_glsl_Refract_dvec4_dvec4_float()
      : GlslBuiltinTest<glsl::dvec4Ty, glsl::dvec4Ty, glsl::dvec4Ty,
                        glsl::floatTy>(
            uvk::Shader::op_glsl_Refract_dvec4_dvec4_float) {}
};

TEST_F(op_glsl_Refract_dvec4_dvec4_float, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2}, {2, 2, 2, 2}, 2);
  }
}

#endif

TEST_F(op_glsl_Refract_float_float_float, BasicCorrectnessTest) {
  // From specification:
  //   For the incident vector I and surface normal N, and the ratio of indices
  //   of refraction eta, the result is the refraction vector. The result is
  //   computed by
  //
  //   k = 1.0 - eta * eta * (1.0 - dot(N, I) * dot(N, I))
  //
  //   if k < 0.0 the result is 0.0
  //
  //   otherwise, the result is eta * I - (eta * dot(N, I) + sqrt(k)) * N
  //
  //   The input parameters for the incident vector I and the surface normal N
  //   must already be normalized to get the desired results.
  //
  //   The type of I and N must be a scalar or vector with a floating-point
  //   component type.
  //
  //   The type of eta must be a 16-bit or 32-bit floating-point scalar.
  //
  //   Result Type, the type of I, and the type of N must all be the same type.
  // Expected results:
  //   Refract(1.0, -1.0, 0.25) = 1.0

  auto result = RunWithArgs(1.0f, -1.0f, 0.25f);
  EXPECT_TRUE(glsl::fuzzyEq(1.0f, result));
}

TEST_F(op_glsl_Refract_vec4_vec4_float, BasicCorrectnessTest) {
  // From specification:
  //   For the incident vector I and surface normal N, and the ratio of indices
  //   of refraction eta, the result is the refraction vector. The result is
  //   computed by
  //
  //   k = 1.0 - eta * eta * (1.0 - dot(N, I) * dot(N, I))
  //
  //   if k < 0.0 the result is 0.0
  //
  //   otherwise, the result is eta * I - (eta * dot(N, I) + sqrt(k)) * N
  //
  //   The input parameters for the incident vector I and the surface normal N
  //   must already be normalized to get the desired results.
  //
  //   The type of I and N must be a scalar or vector with a floating-point
  //   component type.
  //
  //   The type of eta must be a 16-bit or 32-bit floating-point scalar.
  //
  //   Result Type, the type of I, and the type of N must all be the same type.
  // Expected results:
  //   Refract(<0.08805f, -0.06339f, 0.88574f, -0.45132f>,
  //           <0.31812f, 0.04772f, -0.71576f, 0.61985f>,
  //           0.23)
  //        = <-0.23107, -0.05228, 0.76918, -0.59349>

  auto result = RunWithArgs({0.08805f, -0.06339f, 0.88574f, -0.45132f},
                            {0.31812f, 0.04772f, -0.71576f, 0.61985f}, 0.23f);
  EXPECT_TRUE(
      glsl::fuzzyEq({-0.23107f, -0.05228f, 0.76918f, -0.59349f}, result, 0.1f));
}

TEST_F(op_glsl_Refract_double_double_double, BasicCorrectnessTest) {
  // From specification:
  //   For the incident vector I and surface normal N, and the ratio of indices
  //   of refraction eta, the result is the refraction vector. The result is
  //   computed by
  //
  //   k = 1.0 - eta * eta * (1.0 - dot(N, I) * dot(N, I))
  //
  //   if k < 0.0 the result is 0.0
  //
  //   otherwise, the result is eta * I - (eta * dot(N, I) + sqrt(k)) * N
  //
  //   The input parameters for the incident vector I and the surface normal N
  //   must already be normalized to get the desired results.
  //
  //   The type of I and N must be a scalar or vector with a floating-point
  //   component type.
  //
  //   The type of eta must be a 16-bit or 32-bit floating-point scalar.
  //
  //   Result Type, the type of I, and the type of N must all be the same type.
  // Expected results:
  //   Refract(0.5, 0.5, 5.0) = 0.0
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(0.5, 0.5, 5.0);
    EXPECT_TRUE(glsl::fuzzyEq(0.0, result));
  }
}

TEST_F(op_glsl_Refract_dvec4_dvec4_double, BasicCorrectnessTest) {
  // From specification:
  //   For the incident vector I and surface normal N, and the ratio of indices
  //   of refraction eta, the result is the refraction vector. The result is
  //   computed by
  //
  //   k = 1.0 - eta * eta * (1.0 - dot(N, I) * dot(N, I))
  //
  //   if k < 0.0 the result is 0.0
  //
  //   otherwise, the result is eta * I - (eta * dot(N, I) + sqrt(k)) * N
  //
  //   The input parameters for the incident vector I and the surface normal N
  //   must already be normalized to get the desired results.
  //
  //   The type of I and N must be a scalar or vector with a floating-point
  //   component type.
  //
  //   The type of eta must be a 16-bit or 32-bit floating-point scalar.
  //
  //   Result Type, the type of I, and the type of N must all be the same type.
  // Expected results:
  //   Refract(<0.08805, -0.06339, 0.88574, -0.45132>, <0.31812, 0.04772,
  //   -0.71576, 0.61985>,
  //          0.23)
  //        = <-0.23107, -0.05228, 0.76918, -0.59349>
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({0.08805, -0.06339, 0.88574, -0.45132},
                              {0.31812, 0.04772, -0.71576, 0.61985}, 0.23);
    EXPECT_TRUE(glsl::fuzzyEq({-0.23107, -0.05228, 0.76918, -0.59349}, result));
  }
}

#ifndef IGNORE_SPIRV_TESTS

TEST_F(op_glsl_Refract_double_double_float, BasicCorrectnessTest) {
  // From specification:
  //   For the incident vector I and surface normal N, and the ratio of indices
  //   of refraction eta, the result is the refraction vector. The result is
  //   computed by
  //
  //   k = 1.0 - eta * eta * (1.0 - dot(N, I) * dot(N, I))
  //
  //   if k < 0.0 the result is 0.0
  //
  //   otherwise, the result is eta * I - (eta * dot(N, I) + sqrt(k)) * N
  //
  //   The input parameters for the incident vector I and the surface normal N
  //   must already be normalized to get the desired results.
  //
  //   The type of I and N must be a scalar or vector with a floating-point
  //   component type.
  //
  //   The type of eta must be a 16-bit or 32-bit floating-point scalar.
  //
  //   Result Type, the type of I, and the type of N must all be the same type.
  // Expected results:
  //   Refract(0.5, 0.5, 5.0) = 0.0
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs(0.5, 0.5, 5.0f);
    EXPECT_TRUE(glsl::fuzzyEq(0.0, result));
  }
}

TEST_F(op_glsl_Refract_dvec4_dvec4_float, BasicCorrectnessTest) {
  // From specification:
  //   For the incident vector I and surface normal N, and the ratio of indices
  //   of refraction eta, the result is the refraction vector. The result is
  //   computed by
  //
  //   k = 1.0 - eta * eta * (1.0 - dot(N, I) * dot(N, I))
  //
  //   if k < 0.0 the result is 0.0
  //
  //   otherwise, the result is eta * I - (eta * dot(N, I) + sqrt(k)) * N
  //
  //   The input parameters for the incident vector I and the surface normal N
  //   must already be normalized to get the desired results.
  //
  //   The type of I and N must be a scalar or vector with a floating-point
  //   component type.
  //
  //   The type of eta must be a 16-bit or 32-bit floating-point scalar.
  //
  //   Result Type, the type of I, and the type of N must all be the same type.
  // Expected results:
  //   Refract(<0.08805, -0.06339, 0.88574, -0.45132>, <0.31812, 0.04772,
  //   -0.71576, 0.61985>,
  //          0.23)
  //        = <-0.23107, -0.05228, 0.76918, -0.59349>
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({0.08805, -0.06339, 0.88574, -0.45132},
                              {0.31812, 0.04772, -0.71576, 0.61985}, 0.23f);
    EXPECT_TRUE(glsl::fuzzyEq({-0.23107, -0.05228, 0.76918, -0.59349}, result));
  }
}

#endif
