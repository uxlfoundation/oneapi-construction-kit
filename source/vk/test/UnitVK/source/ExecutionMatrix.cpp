// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "GLSLTestDefs.h"

class op_glsl_Determinant_mat2
    : public GlslBuiltinTest<glsl::floatTy, glsl::mat2Ty> {
 public:
  op_glsl_Determinant_mat2()
      : GlslBuiltinTest<glsl::floatTy, glsl::mat2Ty>(
            uvk::Shader::op_glsl_Determinant_mat2) {}
};

TEST_F(op_glsl_Determinant_mat2, DISABLED_Smoke) {
  RunWithArgs({{2, 2}, {2, 2}});
}

class op_glsl_Determinant_mat3
    : public GlslBuiltinTest<glsl::floatTy, glsl::mat3Ty> {
 public:
  op_glsl_Determinant_mat3()
      : GlslBuiltinTest<glsl::floatTy, glsl::mat3Ty>(
            uvk::Shader::op_glsl_Determinant_mat3) {}
};

TEST_F(op_glsl_Determinant_mat3, DISABLED_Smoke) {
  RunWithArgs({{2, 2, 2}, {2, 2, 2}, {2, 2, 2}});
}

class op_glsl_Determinant_mat4
    : public GlslBuiltinTest<glsl::floatTy, glsl::mat4Ty> {
 public:
  op_glsl_Determinant_mat4()
      : GlslBuiltinTest<glsl::floatTy, glsl::mat4Ty>(
            uvk::Shader::op_glsl_Determinant_mat4) {}
};

TEST_F(op_glsl_Determinant_mat4, DISABLED_Smoke) {
  RunWithArgs({{2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2}});
}

class op_glsl_Determinant_dmat2
    : public GlslBuiltinTest<glsl::doubleTy, glsl::dmat2Ty> {
 public:
  op_glsl_Determinant_dmat2()
      : GlslBuiltinTest<glsl::doubleTy, glsl::dmat2Ty>(
            uvk::Shader::op_glsl_Determinant_dmat2) {}
};

TEST_F(op_glsl_Determinant_dmat2, DISABLED_Smoke) {
  RunWithArgs({{2, 2}, {2, 2}});
}

class op_glsl_Determinant_dmat3
    : public GlslBuiltinTest<glsl::doubleTy, glsl::dmat3Ty> {
 public:
  op_glsl_Determinant_dmat3()
      : GlslBuiltinTest<glsl::doubleTy, glsl::dmat3Ty>(
            uvk::Shader::op_glsl_Determinant_dmat3) {}
};

TEST_F(op_glsl_Determinant_dmat3, DISABLED_Smoke) {
  RunWithArgs({{2, 2, 2}, {2, 2, 2}, {2, 2, 2}});
}

class op_glsl_Determinant_dmat4
    : public GlslBuiltinTest<glsl::doubleTy, glsl::dmat4Ty> {
 public:
  op_glsl_Determinant_dmat4()
      : GlslBuiltinTest<glsl::doubleTy, glsl::dmat4Ty>(
            uvk::Shader::op_glsl_Determinant_dmat4) {}
};

TEST_F(op_glsl_Determinant_dmat4, DISABLED_Smoke) {
  RunWithArgs({{2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2}});
}

TEST_F(op_glsl_Determinant_mat2, DISABLED_BasicCorrectnessTest) {
  // From specification:
  //   Result is the determinant of x.
  //
  //   The operand x must be a square matrix.
  //
  //   Result Type and the type of x must be the same type. Results are computed
  //   per component.
  // Expected results:
  //   Determinant(<5.0, 20.0>,
  //               <2.0, 63.0>) = 275.0

  auto result = RunWithArgs({{5.0f, 2.0f}, {20.0f, 63.0f}});
  EXPECT_TRUE(glsl::fuzzyEq(275.0f, result));
}

TEST_F(op_glsl_Determinant_mat4, DISABLED_BasicCorrectnessTest) {
  // From specification:
  //   Result is the determinant of x.
  //
  //   The operand x must be a square matrix.
  //
  //   Result Type must be the same type as the component type in the columns of
  //   x.
  // Expected results:
  //   Determinant(<5.0, 20.0, 0.232, 23>
  //               <2.0, 63.0, -0.899, 4.5656>
  //               <-23, 36, -89.0, 8.0f>
  //               <0.001, 2.45, 4, 789.0f>) = -18721698.390455812

  auto result = RunWithArgs({{5.0f, 2.0f, -23.0f, 0.001f},
                             {20.0f, 63.0f, 36.0f, 2.45f},
                             {0.232f, -0.899f, -89.0f, 4.0f},
                             {23.0f, 4.5656f, 8.0f, 789.0f}});
  EXPECT_TRUE(glsl::fuzzyEq(-18721698.390455812f, result));
}

TEST_F(op_glsl_Determinant_dmat2, DISABLED_BasicCorrectnessTest) {
  // From specification:
  //   Result is the determinant of x.
  //
  //   The operand x must be a square matrix.
  //
  //   Result Type must be the same type as the component type in the columns of
  //   x.
  // Expected results:
  //   Determinant(<5.0, 20.0>,
  //               <2.0, 63.0>) = 275.0

  auto result = RunWithArgs({{5.0, 2.0}, {20.0, 63.0}});
  EXPECT_TRUE(glsl::fuzzyEq(275.0, result));
}

TEST_F(op_glsl_Determinant_dmat4, DISABLED_BasicCorrectnessTest) {
  // From specification:
  //   Result is the determinant of x.
  //
  //   The operand x must be a square matrix.
  //
  //    Result Type must be the same type as the component type in the columns
  //    of x.
  // Expected results:
  //   Determinant(<5.0, 20.0, 0.232, 23>
  //               <2.0, 63.0, -0.899, 4.5656>
  //               <-23, 36, -89.0, 8.0f>
  //               <0.001, 2.45, 4, 789.0f>) = -18721698.390455812

  auto result = RunWithArgs({{5.0, 2.0, -23.0, 0.001},
                             {20.0, 63.0, 36.0, 2.45},
                             {0.232, -0.899, -89.0, 4.0},
                             {23.0, 4.5656, 8.0, 789.0}});
  EXPECT_TRUE(glsl::fuzzyEq(-18721698.390455812, result));
}

class op_glsl_MatrixInverse_mat2
    : public GlslBuiltinTest<glsl::mat2Ty, glsl::mat2Ty> {
 public:
  op_glsl_MatrixInverse_mat2()
      : GlslBuiltinTest<glsl::mat2Ty, glsl::mat2Ty>(
            uvk::Shader::op_glsl_MatrixInverse_mat2) {}
};

TEST_F(op_glsl_MatrixInverse_mat2, DISABLED_Smoke) {
  RunWithArgs({{2, 2}, {2, 2}});
}

class op_glsl_MatrixInverse_mat3
    : public GlslBuiltinTest<glsl::mat3Ty, glsl::mat3Ty> {
 public:
  op_glsl_MatrixInverse_mat3()
      : GlslBuiltinTest<glsl::mat3Ty, glsl::mat3Ty>(
            uvk::Shader::op_glsl_MatrixInverse_mat3) {}
};

TEST_F(op_glsl_MatrixInverse_mat3, DISABLED_Smoke) {
  RunWithArgs({{2, 2, 2}, {2, 2, 2}, {2, 2, 2}});
}

class op_glsl_MatrixInverse_mat4
    : public GlslBuiltinTest<glsl::mat4Ty, glsl::mat4Ty> {
 public:
  op_glsl_MatrixInverse_mat4()
      : GlslBuiltinTest<glsl::mat4Ty, glsl::mat4Ty>(
            uvk::Shader::op_glsl_MatrixInverse_mat4) {}
};

TEST_F(op_glsl_MatrixInverse_mat4, DISABLED_Smoke) {
  RunWithArgs({{2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2}});
}

class op_glsl_MatrixInverse_dmat2
    : public GlslBuiltinTest<glsl::dmat2Ty, glsl::dmat2Ty> {
 public:
  op_glsl_MatrixInverse_dmat2()
      : GlslBuiltinTest<glsl::dmat2Ty, glsl::dmat2Ty>(
            uvk::Shader::op_glsl_MatrixInverse_dmat2) {}
};

TEST_F(op_glsl_MatrixInverse_dmat2, DISABLED_Smoke) {
  RunWithArgs({{2, 2}, {2, 2}});
}

class op_glsl_MatrixInverse_dmat3
    : public GlslBuiltinTest<glsl::dmat3Ty, glsl::dmat3Ty> {
 public:
  op_glsl_MatrixInverse_dmat3()
      : GlslBuiltinTest<glsl::dmat3Ty, glsl::dmat3Ty>(
            uvk::Shader::op_glsl_MatrixInverse_dmat3) {}
};

TEST_F(op_glsl_MatrixInverse_dmat3, DISABLED_Smoke) {
  RunWithArgs({{2, 2, 2}, {2, 2, 2}, {2, 2, 2}});
}

class op_glsl_MatrixInverse_dmat4
    : public GlslBuiltinTest<glsl::dmat4Ty, glsl::dmat4Ty> {
 public:
  op_glsl_MatrixInverse_dmat4()
      : GlslBuiltinTest<glsl::dmat4Ty, glsl::dmat4Ty>(
            uvk::Shader::op_glsl_MatrixInverse_dmat4) {}
};

TEST_F(op_glsl_MatrixInverse_dmat4, DISABLED_Smoke) {
  RunWithArgs({{2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2}, {2, 2, 2, 2}});
}

TEST_F(op_glsl_MatrixInverse_mat2, DISABLED_BasicCorrectnessTest) {
  // From specification:
  //   Result is a matrix that is the inverse of x. The values in the result are
  //   undefined if x is singular or poorly conditioned (nearly singular).
  //
  //   The operand x must be a square matrix.
  //
  //   Result Type and the type of x must be the same type.
  // Expected results:
  //   MatrixInverse(<5.0, 20.0>,
  //                 <2.0, 63.0>)
  //      = <0.2290909, -0.0727273>,
  //        <-0.0727273, 0.0181818>

  auto result = RunWithArgs({{5.0f, 2.0f}, {20.0f, 63.0f}});
  EXPECT_TRUE(glsl::fuzzyEq(
      {{0.2290909f, -0.0072727f}, {-0.0727273f, 0.0181818f}}, result));
}

TEST_F(op_glsl_MatrixInverse_mat4, DISABLED_BasicCorrectnessTest) {
  // From specification:
  //   Result is a matrix that is the inverse of x. The values in the result are
  //   undefined if x is singular or poorly conditioned (nearly singular).
  //
  //   The operand x must be a square matrix.
  //
  //   Result Type and the type of x must be the same type.
  // Expected results:
  //   MatrixInverse(<5.0, 20.0, 0.232, 23>
  //                 <2.0, 63.0, -0.899, 4.5656>
  //                 <-23.0, 36, -89.0, 8>
  //                 <0.001, 2.45, 4.0, 789.0f>)
  //       =
  //                 <0.2349556, -0.0749568, 0.0010808, -0.0064264>,
  //		     <-0.0083988, 0.0186505, -0.000204, 0.000139>,
  //		     <-0.0640846, 0.0268974, -0.0115925, 0.00183>,
  //		     <0.0003507, -0.0001942, 0.0000594, 0.0012577>

  auto result = RunWithArgs({{5.0f, 2.0f, -23.0f, 0.001f},
                             {20.0f, 63.0f, 36.0f, 2.45f},
                             {0.232f, -0.899f, -89.0f, 4.0f},
                             {23.0f, 4.5656f, 8.0f, 789.0f}});
  EXPECT_TRUE(glsl::fuzzyEq({{0.2349556f, -0.0083988f, -0.0640846f, 0.0003507f},
                             {-0.0749568f, 0.0186505f, 0.0268974f, -0.0001942f},
                             {0.0010808f, -0.000204f, -0.0115925f, 0.0000594f},
                             {-0.0064264f, 0.000139f, 0.00183f, 0.0012577f}},
                            result));
}

TEST_F(op_glsl_MatrixInverse_dmat2, DISABLED_BasicCorrectnessTest) {
  // From specification:
  //   Result is a matrix that is the inverse of x. The values in the result are
  //   undefined if x is singular or poorly conditioned (nearly singular).
  //
  //   The operand x must be a square matrix.
  //
  //   Result Type and the type of x must be the same type.
  // Expected results:
  //   MatrixInverse(<5.0, 20.0>,
  //                 <2.0, 63.0>)
  //      = <0.2290909, -0.0727273>,
  //        <-0.0727273, 0.0181818>

  auto result = RunWithArgs({{5.0, 2.0}, {20.0, 63.0}});
  EXPECT_TRUE(glsl::fuzzyEq({{0.2290909, -0.0072727}, {-0.0727273, 0.0181818}},
                            result));
}

TEST_F(op_glsl_MatrixInverse_dmat4, DISABLED_BasicCorrectnessTest) {
  // From specification:
  //   Result is a matrix that is the inverse of x. The values in the result are
  //   undefined if x is singular or poorly conditioned (nearly singular).
  //
  //   The operand x must be a square matrix.
  //
  //   Result Type and the type of x must be the same type.
  // Expected results:
  //   MatrixInverse(<5.0, 20.0, 0.232, 23>
  //                 <2.0, 63.0, -0.899, 4.5656>
  //                 <-23.0, 36, -89.0, 8>
  //                 <0.001, 2.45, 4.0, 789.0f>)
  //       =
  //                 <0.2349556, -0.0749568, 0.0010808, -0.0064264>,
  //		     <-0.0083988, 0.0186505, -0.000204, 0.000139>,
  //		     <-0.0640846, 0.0268974, -0.0115925, 0.00183>,
  //		     <0.0003507, -0.0001942, 0.0000594, 0.0012577>

  auto result = RunWithArgs({{5.0, 2.0, -23.0, 0.001},
                             {20.0, 63.0, 36.0, 2.45},
                             {0.232, -0.899, -89.0, 4.0},
                             {23.0, 4.5656, 8.0, 789.0}});
  EXPECT_TRUE(glsl::fuzzyEq({{0.2349556, -0.0083988, -0.0640846, 0.0003507},
                             {-0.0749568, 0.0186505, 0.0268974, -0.0001942},
                             {0.0010808, -0.000204, -0.0115925, 0.0000594},
                             {-0.0064264, 0.000139, 0.00183, 0.0012577}},
                            result));
}

class op_glsl_Transpose_mat3x2_toRow
    : public GlslBuiltinTest<glsl::glsl_mat<float, 2, 3, glsl::Order::RowMajor>,
                             glsl::glsl_mat<float, 3, 2>> {
 public:
  op_glsl_Transpose_mat3x2_toRow()
      : GlslBuiltinTest<glsl::glsl_mat<float, 2, 3, glsl::Order::RowMajor>,
                        glsl::glsl_mat<float, 3, 2>>(
            uvk::Shader::op_glsl_Transpose_mat3x2_toRow) {}
};

TEST_F(op_glsl_Transpose_mat3x2_toRow, DISABLED_BasicCorrectnessTest) {
  // From specification:
  //   Transpose a matrix.
  //
  //   Result Type must be an OpTypeMatrix, where the number of columns and the
  //   column size is the reverse of those of the type of Matrix.
  //
  //   Matrix must have of type of OpTypeMatrix.
  // Expected results:
  //   MatrixInverse(<4, -1, -4>
  //                 <-4, -5, 4>)
  //       =
  //                 <4, -4>,
  //		     <-1, -5>,
  //		     <-4, 4>

  auto result = RunWithArgs({{4.0f, -4.0f}, {-1.0f, -5.0f}, {-4.0f, 4.0f}});
  EXPECT_TRUE(
      glsl::fuzzyEq({{4.0f, -4.0f}, {-1.0f, -5.0f}, {-4.0f, 4.0f}}, result));
}

class op_glsl_Transpose_mat3_toRow
    : public GlslBuiltinTest<glsl::glsl_mat<float, 3, 3, glsl::Order::RowMajor>,
                             glsl::mat3Ty> {
 public:
  op_glsl_Transpose_mat3_toRow()
      : GlslBuiltinTest<glsl::glsl_mat<float, 3, 3, glsl::Order::RowMajor>,
                        glsl::mat3Ty>(
            uvk::Shader::op_glsl_Transpose_mat3_toRow) {}
};

TEST_F(op_glsl_Transpose_mat3_toRow, DISABLED_BasicCorrectnessTest) {
  // From specification:
  //   Transpose a matrix.
  //
  //   Result Type must be an OpTypeMatrix, where the number of columns and the
  //   column size is the reverse of those of the type of Matrix.
  //
  //   Matrix must have of type of OpTypeMatrix.
  // Expected results:
  //   MatrixInverse(<4, -1, -4>
  //                 <-4, -5, 4>
  //                 <8, -1.5, 0.22>)
  //       =
  //                 <4, -4, 8>,
  //		     <-1, -5, -1.5>,
  //		     <-4, 4, 0.22>

  auto result = RunWithArgs(
      {{4.0f, -4.0f, 8.0f}, {-1.0f, -5.0f, -1.5f}, {-4.0f, 4.0f, 0.22f}});
  EXPECT_TRUE(glsl::fuzzyEq(
      {{4.0f, -4.0f, 8.0f}, {-1.0f, -5.0f, -1.5f}, {-4.0f, 4.0f, 0.22f}},
      result));
}
