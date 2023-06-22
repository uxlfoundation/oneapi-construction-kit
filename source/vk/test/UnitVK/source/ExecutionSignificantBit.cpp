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

#include <limits>

#include "GLSLTestDefs.h"

constexpr glsl::intTy I_MIN = std::numeric_limits<glsl::intTy>::min();
constexpr glsl::intTy I_MAX = std::numeric_limits<glsl::intTy>::max();
constexpr glsl::uintTy U_MAX = std::numeric_limits<glsl::uintTy>::max();

class op_glsl_FindILsb_uint
    : public GlslBuiltinTest<glsl::intTy, glsl::uintTy> {
 public:
  op_glsl_FindILsb_uint()
      : GlslBuiltinTest<glsl::intTy, glsl::uintTy>(
            uvk::Shader::op_glsl_FindILsb_uint) {}
};

TEST_F(op_glsl_FindILsb_uint, Smoke) { RunWithArgs(2); }

class op_glsl_FindILsb_uvec2
    : public GlslBuiltinTest<glsl::ivec2Ty, glsl::uvec2Ty> {
 public:
  op_glsl_FindILsb_uvec2()
      : GlslBuiltinTest<glsl::ivec2Ty, glsl::uvec2Ty>(
            uvk::Shader::op_glsl_FindILsb_uvec2) {}
};

TEST_F(op_glsl_FindILsb_uvec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_FindILsb_uvec3
    : public GlslBuiltinTest<glsl::ivec3Ty, glsl::uvec3Ty> {
 public:
  op_glsl_FindILsb_uvec3()
      : GlslBuiltinTest<glsl::ivec3Ty, glsl::uvec3Ty>(
            uvk::Shader::op_glsl_FindILsb_uvec3) {}
};

TEST_F(op_glsl_FindILsb_uvec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_FindILsb_uvec4
    : public GlslBuiltinTest<glsl::ivec4Ty, glsl::uvec4Ty> {
 public:
  op_glsl_FindILsb_uvec4()
      : GlslBuiltinTest<glsl::ivec4Ty, glsl::uvec4Ty>(
            uvk::Shader::op_glsl_FindILsb_uvec4) {}
};

TEST_F(op_glsl_FindILsb_uvec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

class op_glsl_FindILsb_int : public GlslBuiltinTest<glsl::intTy, glsl::intTy> {
 public:
  op_glsl_FindILsb_int()
      : GlslBuiltinTest<glsl::intTy, glsl::intTy>(
            uvk::Shader::op_glsl_FindILsb_int) {}
};

TEST_F(op_glsl_FindILsb_int, Smoke) { RunWithArgs(2); }

class op_glsl_FindILsb_ivec2
    : public GlslBuiltinTest<glsl::ivec2Ty, glsl::ivec2Ty> {
 public:
  op_glsl_FindILsb_ivec2()
      : GlslBuiltinTest<glsl::ivec2Ty, glsl::ivec2Ty>(
            uvk::Shader::op_glsl_FindILsb_ivec2) {}
};

TEST_F(op_glsl_FindILsb_ivec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_FindILsb_ivec3
    : public GlslBuiltinTest<glsl::ivec3Ty, glsl::ivec3Ty> {
 public:
  op_glsl_FindILsb_ivec3()
      : GlslBuiltinTest<glsl::ivec3Ty, glsl::ivec3Ty>(
            uvk::Shader::op_glsl_FindILsb_ivec3) {}
};

TEST_F(op_glsl_FindILsb_ivec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_FindILsb_ivec4
    : public GlslBuiltinTest<glsl::ivec4Ty, glsl::ivec4Ty> {
 public:
  op_glsl_FindILsb_ivec4()
      : GlslBuiltinTest<glsl::ivec4Ty, glsl::ivec4Ty>(
            uvk::Shader::op_glsl_FindILsb_ivec4) {}
};

TEST_F(op_glsl_FindILsb_ivec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_FindILsb_uint, BasicCorrectnessTest) {
  // From specification:
  //   Integer least-significant bit.
  //
  //   Results in the bit number of the least-significant 1-bit in the binary
  //   representation of Value. If Value is 0, the result is -1.
  //
  //   Result Type and the type of Value must both be integer scalar or integer
  //   vector types. Result Type and operand types must have the same number of
  //   components with the same component width. Results are computed per
  //   component.
  // Expected results:
  //   FindILsb(72) = 3

  auto result = RunWithArgs(72);
  EXPECT_EQ(3, result);
}

TEST_F(op_glsl_FindILsb_uvec4, BasicCorrectnessTest) {
  // From specification:
  //   Integer least-significant bit.
  //
  //   Results in the bit number of the least-significant 1-bit in the binary
  //   representation of Value. If Value is 0, the result is -1.
  //
  //   Result Type and the type of Value must both be integer scalar or integer
  //   vector types. Result Type and operand types must have the same number of
  //   components with the same component width. Results are computed per
  //   component.
  // Expected results:
  //   FindILsb(<0, 7, 4294967295, 2147483648>) = <-1, 0, 0, 32>

  auto result = RunWithArgs({0, 7, U_MAX, ((uint32_t)I_MAX) + 1});
  EXPECT_EQ(glsl::ivec4Ty(-1, 0, 0, 31), result);
}

TEST_F(op_glsl_FindILsb_int, BasicCorrectnessTest) {
  // From specification:
  //   Integer least-significant bit.
  //
  //   Results in the bit number of the least-significant 1-bit in the binary
  //   representation of Value. If Value is 0, the result is -1.
  //
  //   Result Type and the type of Value must both be integer scalar or integer
  //   vector types. Result Type and operand types must have the same number of
  //   components with the same component width. Results are computed per
  //   component.
  // Expected results:
  //   FindILsb(72) = 3

  auto result = RunWithArgs(-72);
  EXPECT_EQ(3, result);
}

TEST_F(op_glsl_FindILsb_ivec4, BasicCorrectnessTest) {
  // From specification:
  //   Integer least-significant bit.
  //
  //   Results in the bit number of the least-significant 1-bit in the binary
  //   representation of Value. If Value is 0, the result is -1.
  //
  //   Result Type and the type of Value must both be integer scalar or integer
  //   vector types. Result Type and operand types must have the same number of
  //   components with the same component width. Results are computed per
  //   component.
  // Expected results:
  //   FindILsb(<0, 7, 4294967295, 2147483648>) = <0, 0, -1, 31>

  auto result = RunWithArgs({1, -1, 0, I_MIN});
  EXPECT_EQ(glsl::ivec4Ty(0, 0, -1, 31), result);
}

class op_glsl_FindSMsb_int : public GlslBuiltinTest<glsl::intTy, glsl::intTy> {
 public:
  op_glsl_FindSMsb_int()
      : GlslBuiltinTest<glsl::intTy, glsl::intTy>(
            uvk::Shader::op_glsl_FindSMsb_int) {}
};

TEST_F(op_glsl_FindSMsb_int, Smoke) { RunWithArgs(2); }

class op_glsl_FindSMsb_ivec2
    : public GlslBuiltinTest<glsl::ivec2Ty, glsl::ivec2Ty> {
 public:
  op_glsl_FindSMsb_ivec2()
      : GlslBuiltinTest<glsl::ivec2Ty, glsl::ivec2Ty>(
            uvk::Shader::op_glsl_FindSMsb_ivec2) {}
};

TEST_F(op_glsl_FindSMsb_ivec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_FindSMsb_ivec3
    : public GlslBuiltinTest<glsl::ivec3Ty, glsl::ivec3Ty> {
 public:
  op_glsl_FindSMsb_ivec3()
      : GlslBuiltinTest<glsl::ivec3Ty, glsl::ivec3Ty>(
            uvk::Shader::op_glsl_FindSMsb_ivec3) {}
};

TEST_F(op_glsl_FindSMsb_ivec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_FindSMsb_ivec4
    : public GlslBuiltinTest<glsl::ivec4Ty, glsl::ivec4Ty> {
 public:
  op_glsl_FindSMsb_ivec4()
      : GlslBuiltinTest<glsl::ivec4Ty, glsl::ivec4Ty>(
            uvk::Shader::op_glsl_FindSMsb_ivec4) {}
};

TEST_F(op_glsl_FindSMsb_ivec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_FindSMsb_int, BasicCorrectnessTest) {
  // From specification:
  //   Signed-integer most-significant bit, with Value interpreted as a signed
  //   integer.
  //
  //   For positive numbers, the result will be the bit number of the most
  //   significant 1-bit. For negative numbers, the result will be the bit
  //   number of the most significant 0-bit. For a Value of 0 or -1, the result
  //   is -1.
  //
  //   Result Type and the type of Value must both be integer scalar or integer
  //   vector types. Result Type and operand types must have the same number of
  //   components with the same component width. Results are computed per
  //   component.
  //
  //   This instruction is currently limited to 32-bit width components.
  // Expected results:
  //   FindSMsb(72) = 6

  auto result = RunWithArgs(72);
  EXPECT_EQ(6, result);
}

TEST_F(op_glsl_FindSMsb_ivec4, BasicCorrectnessTest) {
  // From specification:
  //   Signed-integer most-significant bit, with Value interpreted as a signed
  //   integer.
  //
  //   For positive numbers, the result will be the bit number of the most
  //   significant 1-bit. For negative numbers, the result will be the bit
  //   number of the most significant 0-bit. For a Value of 0 or -1, the result
  //   is -1.
  //
  //   Result Type and the type of Value must both be integer scalar or integer
  //   vector types. Result Type and operand types must have the same number of
  //   components with the same component width. Results are computed per
  //   component.
  //
  //   This instruction is currently limited to 32-bit width components.
  // Expected results:
  //   FindSMsb(<-2147483648 , 2147483647, 0, -1>) = <31, 31, -1, -1>

  auto result = RunWithArgs({I_MIN, I_MAX, 0, -1});
  EXPECT_EQ(glsl::ivec4Ty(30, 30, -1, -1), result);
}

class op_glsl_FindUMsb_uint
    : public GlslBuiltinTest<glsl::intTy, glsl::uintTy> {
 public:
  op_glsl_FindUMsb_uint()
      : GlslBuiltinTest<glsl::intTy, glsl::uintTy>(
            uvk::Shader::op_glsl_FindUMsb_uint) {}
};

TEST_F(op_glsl_FindUMsb_uint, Smoke) { RunWithArgs(2); }

class op_glsl_FindUMsb_uvec2
    : public GlslBuiltinTest<glsl::ivec2Ty, glsl::uvec2Ty> {
 public:
  op_glsl_FindUMsb_uvec2()
      : GlslBuiltinTest<glsl::ivec2Ty, glsl::uvec2Ty>(
            uvk::Shader::op_glsl_FindUMsb_uvec2) {}
};

TEST_F(op_glsl_FindUMsb_uvec2, Smoke) { RunWithArgs({2, 2}); }

class op_glsl_FindUMsb_uvec3
    : public GlslBuiltinTest<glsl::ivec3Ty, glsl::uvec3Ty> {
 public:
  op_glsl_FindUMsb_uvec3()
      : GlslBuiltinTest<glsl::ivec3Ty, glsl::uvec3Ty>(
            uvk::Shader::op_glsl_FindUMsb_uvec3) {}
};

TEST_F(op_glsl_FindUMsb_uvec3, Smoke) { RunWithArgs({2, 2, 2}); }

class op_glsl_FindUMsb_uvec4
    : public GlslBuiltinTest<glsl::ivec4Ty, glsl::uvec4Ty> {
 public:
  op_glsl_FindUMsb_uvec4()
      : GlslBuiltinTest<glsl::ivec4Ty, glsl::uvec4Ty>(
            uvk::Shader::op_glsl_FindUMsb_uvec4) {}
};

TEST_F(op_glsl_FindUMsb_uvec4, Smoke) { RunWithArgs({2, 2, 2, 2}); }

TEST_F(op_glsl_FindUMsb_uint, BasicCorrectnessTest) {
  // From specification:
  //   Unsigned-integer most-significant bit.
  //
  //   Results in the bit number of the most-significant 1-bit in the binary
  //   representation of Value. If Value is 0, the result is -1.
  //
  //   Result Type and the type of Value must both be integer scalar or integer
  //   vector types. Result Type and operand types must have the same number of
  //   components with the same component width. Results are computed per
  //   component.
  //
  //   This instruction is currently limited to 32-bit width components.
  // Expected results:
  //   FindUMsb(72) = 6

  auto result = RunWithArgs(72);
  EXPECT_EQ(6, result);
}

TEST_F(op_glsl_FindUMsb_uvec4, BasicCorrectnessTest) {
  // From specification:
  //   Unsigned-integer most-significant bit.
  //
  //   Results in the bit number of the most-significant 1-bit in the binary
  //   representation of Value. If Value is 0, the result is -1.
  //
  //   Result Type and the type of Value must both be integer scalar or integer
  //   vector types. Result Type and operand types must have the same number of
  //   components with the same component width. Results are computed per
  //   component.
  //
  //   This instruction is currently limited to 32-bit width components.
  // Expected results:
  //   FindUMsb(<68924, 2147483647, 0, 1>) = <16, 30, -1, 0>

  auto result = RunWithArgs({68924, (uint32_t)I_MAX, 0, 1});
  EXPECT_EQ(glsl::ivec4Ty(16, 30, -1, 0), result);
}
