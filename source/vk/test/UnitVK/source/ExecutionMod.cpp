// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "GLSLTestDefs.h"

// None of the tests in this file check the precision of the operations, rather
// they check that the function acts as expected for a limited number of
// argument combinations. Some tests do also verify results when the function
// is passed edge case values such as infinity and NaNs.

#ifndef IGNORE_SPIRV_TESTS

// Note: All pointer arguments point to within the results buffer.
// The result type ModfStructfloatTy allows access to all pointed-to arguments.
class op_glsl_Modf_float_floatPtr
    : public GlslBuiltinTest<glsl::ModfStructfloatTy, glsl::floatTy> {
 public:
  op_glsl_Modf_float_floatPtr()
      : GlslBuiltinTest<glsl::ModfStructfloatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_Modf_float_floatPtr) {}
};

TEST_F(op_glsl_Modf_float_floatPtr, Smoke) { RunWithArgs(2); }

// Note: All pointer arguments point to within the results buffer.
// The result type ModfStructvec2Ty allows access to all pointed-to arguments.
class op_glsl_Modf_vec2_vec2Ptr
    : public GlslBuiltinTest<glsl::ModfStructvec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_Modf_vec2_vec2Ptr()
      : GlslBuiltinTest<glsl::ModfStructvec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_Modf_vec2_vec2Ptr) {}
};

TEST_F(op_glsl_Modf_vec2_vec2Ptr, Smoke) { RunWithArgs({2, 2}); }

// Note: All pointer arguments point to within the results buffer.
// The result type ModfStructvec3Ty allows access to all pointed-to arguments.
class op_glsl_Modf_vec3_vec3Ptr
    : public GlslBuiltinTest<glsl::ModfStructvec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_Modf_vec3_vec3Ptr()
      : GlslBuiltinTest<glsl::ModfStructvec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_Modf_vec3_vec3Ptr) {}
};

TEST_F(op_glsl_Modf_vec3_vec3Ptr, Smoke) { RunWithArgs({2, 2, 2}); }

// Note: All pointer arguments point to within the results buffer.
// The result type ModfStructvec4Ty allows access to all pointed-to arguments.
class op_glsl_Modf_vec4_vec4Ptr
    : public GlslBuiltinTest<glsl::ModfStructvec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_Modf_vec4_vec4Ptr()
      : GlslBuiltinTest<glsl::ModfStructvec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_Modf_vec4_vec4Ptr) {}
};

TEST_F(op_glsl_Modf_vec4_vec4Ptr, Smoke) { RunWithArgs({2, 2, 2, 2}); }

// Note: All pointer arguments point to within the results buffer.
// The result type ModfStructdoubleTy allows access to all pointed-to arguments.
class op_glsl_Modf_double_doublePtr
    : public GlslBuiltinTest<glsl::ModfStructdoubleTy, glsl::doubleTy> {
 public:
  op_glsl_Modf_double_doublePtr()
      : GlslBuiltinTest<glsl::ModfStructdoubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_Modf_double_doublePtr) {}
};

TEST_F(op_glsl_Modf_double_doublePtr, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2);
  }
}

// Note: All pointer arguments point to within the results buffer.
// The result type ModfStructdvec2Ty allows access to all pointed-to arguments.
class op_glsl_Modf_dvec2_dvec2Ptr
    : public GlslBuiltinTest<glsl::ModfStructdvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_Modf_dvec2_dvec2Ptr()
      : GlslBuiltinTest<glsl::ModfStructdvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_Modf_dvec2_dvec2Ptr) {}
};

TEST_F(op_glsl_Modf_dvec2_dvec2Ptr, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2});
  }
}

// Note: All pointer arguments point to within the results buffer.
// The result type ModfStructdvec3Ty allows access to all pointed-to arguments.
class op_glsl_Modf_dvec3_dvec3Ptr
    : public GlslBuiltinTest<glsl::ModfStructdvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_Modf_dvec3_dvec3Ptr()
      : GlslBuiltinTest<glsl::ModfStructdvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_Modf_dvec3_dvec3Ptr) {}
};

TEST_F(op_glsl_Modf_dvec3_dvec3Ptr, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2});
  }
}

// Note: All pointer arguments point to within the results buffer.
// The result type ModfStructdvec4Ty allows access to all pointed-to arguments.
class op_glsl_Modf_dvec4_dvec4Ptr
    : public GlslBuiltinTest<glsl::ModfStructdvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_Modf_dvec4_dvec4Ptr()
      : GlslBuiltinTest<glsl::ModfStructdvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_Modf_dvec4_dvec4Ptr) {}
};

TEST_F(op_glsl_Modf_dvec4_dvec4Ptr, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2});
  }
}

// The following test that the Modf instruction is correctly implemented.
// This is not a test for precision, rather this is to ensure that the pointers
// passed to the instruction are correctly handled. As a result, a limited
// of argument combinations are tested, and the argument values were chosen
// completely arbitrarily.

TEST_F(op_glsl_Modf_float_floatPtr, ArgumentsPassedCorrectly) {
  // Expected results:
  //    Modf(3.14) = ( 0.14, 3)

  auto result = RunWithArgs(3.14f);
  EXPECT_TRUE(glsl::fuzzyEq(result.fract, 0.14f));
  EXPECT_TRUE(glsl::fuzzyEq(result.whole, 3.00f));
}

TEST_F(op_glsl_Modf_vec4_vec4Ptr, ArgumentsPassedCorrectly) {
  // Expected results:
  //    Modf(3.14) = ( 0.14, 3)
  //    Modf(0.01) = ( 0.01, 0)
  //    Modf(2.68) = ( 0.68, 2)
  //    Modf(1.10) = ( 0.10, 1)

  auto result = RunWithArgs({3.14f, 0.01f, 2.68f, 1.10f});
  EXPECT_TRUE(
      glsl::fuzzyEq(result.fract, glsl::vec4Ty(0.14f, 0.01f, 0.68f, 0.10f)));
  EXPECT_TRUE(glsl::fuzzyEq(result.whole, glsl::vec4Ty(3.0f, 0.0f, 2.0f, 1.0f)));
}

TEST_F(op_glsl_Modf_dvec2_dvec2Ptr, ArgumentsPassedCorrectly) {
  // Expected results:
  //    Modf(3.14) = ( 0.14, 3)
  //    Modf(0.01) = ( 0.01, 0)
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({3.14f, 0.01f});
    EXPECT_TRUE(glsl::fuzzyEq(result.fract, glsl::dvec2Ty(0.14, 0.01)));
    EXPECT_TRUE(glsl::fuzzyEq(result.whole, glsl::dvec2Ty(3.0, 0.0)));
  }
}

class op_glsl_ModfStruct_float
    : public GlslBuiltinTest<glsl::ModfStructfloatTy, glsl::floatTy> {
 public:
  op_glsl_ModfStruct_float()
      : GlslBuiltinTest<glsl::ModfStructfloatTy, glsl::floatTy>(
            uvk::Shader::op_glsl_ModfStruct_float) {}
};

TEST_F(op_glsl_ModfStruct_float, Smoke) { RunWithArgs(2); }

class op_glsl_ModfStruct_vec2
    : public GlslBuiltinTest<glsl::ModfStructvec2Ty, glsl::vec2Ty> {
 public:
  op_glsl_ModfStruct_vec2()
      : GlslBuiltinTest<glsl::ModfStructvec2Ty, glsl::vec2Ty>(
            uvk::Shader::op_glsl_ModfStruct_vec2) {}
};

TEST_F(op_glsl_ModfStruct_vec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_ModfStruct_vec3
    : public GlslBuiltinTest<glsl::ModfStructvec3Ty, glsl::vec3Ty> {
 public:
  op_glsl_ModfStruct_vec3()
      : GlslBuiltinTest<glsl::ModfStructvec3Ty, glsl::vec3Ty>(
            uvk::Shader::op_glsl_ModfStruct_vec3) {}
};

TEST_F(op_glsl_ModfStruct_vec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_ModfStruct_vec4
    : public GlslBuiltinTest<glsl::ModfStructvec4Ty, glsl::vec4Ty> {
 public:
  op_glsl_ModfStruct_vec4()
      : GlslBuiltinTest<glsl::ModfStructvec4Ty, glsl::vec4Ty>(
            uvk::Shader::op_glsl_ModfStruct_vec4) {}
};

TEST_F(op_glsl_ModfStruct_vec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

class op_glsl_ModfStruct_double
    : public GlslBuiltinTest<glsl::ModfStructdoubleTy, glsl::doubleTy> {
 public:
  op_glsl_ModfStruct_double()
      : GlslBuiltinTest<glsl::ModfStructdoubleTy, glsl::doubleTy>(
            uvk::Shader::op_glsl_ModfStruct_double) {}
};

TEST_F(op_glsl_ModfStruct_double, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs(2);
  }
}

class op_glsl_ModfStruct_dvec2
    : public GlslBuiltinTest<glsl::ModfStructdvec2Ty, glsl::dvec2Ty> {
 public:
  op_glsl_ModfStruct_dvec2()
      : GlslBuiltinTest<glsl::ModfStructdvec2Ty, glsl::dvec2Ty>(
            uvk::Shader::op_glsl_ModfStruct_dvec2) {}
};

TEST_F(op_glsl_ModfStruct_dvec2, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2});
  }
}

class op_glsl_ModfStruct_dvec3
    : public GlslBuiltinTest<glsl::ModfStructdvec3Ty, glsl::dvec3Ty> {
 public:
  op_glsl_ModfStruct_dvec3()
      : GlslBuiltinTest<glsl::ModfStructdvec3Ty, glsl::dvec3Ty>(
            uvk::Shader::op_glsl_ModfStruct_dvec3) {}
};

TEST_F(op_glsl_ModfStruct_dvec3, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2});
  }
}

class op_glsl_ModfStruct_dvec4
    : public GlslBuiltinTest<glsl::ModfStructdvec4Ty, glsl::dvec4Ty> {
 public:
  op_glsl_ModfStruct_dvec4()
      : GlslBuiltinTest<glsl::ModfStructdvec4Ty, glsl::dvec4Ty>(
            uvk::Shader::op_glsl_ModfStruct_dvec4) {}
};

TEST_F(op_glsl_ModfStruct_dvec4, Smoke) {
  if (deviceFeatures.shaderFloat64) {
    RunWithArgs({2, 2, 2, 2});
  }
}

// Identical to above, but this time operating on the struct variations of the
// functions
TEST_F(op_glsl_ModfStruct_float, ArgumentsPassedCorrectly) {
  // Expected results:
  //    ModfStruct(3.14) = ( 0.14, 3)

  auto result = RunWithArgs(3.14f);
  EXPECT_TRUE(glsl::fuzzyEq(result.fract, 0.14f));
  EXPECT_TRUE(glsl::fuzzyEq(result.whole, 3.00f));
}

TEST_F(op_glsl_ModfStruct_vec4, ArgumentsPassedCorrectly) {
  // Expected results:
  //    ModfStruct(3.14) = ( 0.14, 3)
  //    ModfStruct(0.01) = ( 0.01, 0)
  //    ModfStruct(2.68) = ( 0.68, 2)
  //    ModfStruct(1.10) = ( 0.10, 1)

  auto result = RunWithArgs({3.14f, 0.01f, 2.68f, 1.10f});
  EXPECT_TRUE(
      glsl::fuzzyEq(result.fract, glsl::vec4Ty(0.14f, 0.01f, 0.68f, 0.10f)));
  EXPECT_TRUE(glsl::fuzzyEq(result.whole, glsl::vec4Ty(3.0f, 0.0f, 2.0f, 1.0f)));
}

TEST_F(op_glsl_ModfStruct_dvec2, ArgumentsPassedCorrectly) {
  // Expected results:
  //    ModfStruct(3.14) = ( 0.14, 3)
  //    ModfStruct(0.01) = ( 0.01, 0)
  if (deviceFeatures.shaderFloat64) {
    auto result = RunWithArgs({3.14f, 0.01f});
    EXPECT_TRUE(glsl::fuzzyEq(result.fract, glsl::dvec2Ty(0.14, 0.01)));
    EXPECT_TRUE(glsl::fuzzyEq(result.whole, glsl::dvec2Ty(3.0, 0.0)));
  }
}

#endif
