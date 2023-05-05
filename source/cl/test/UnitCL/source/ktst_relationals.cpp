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

#include <cargo/utility.h>

#include <cmath>

#include "Common.h"
#include "kts/precision.h"
#include "kts/relationals.h"

using namespace kts::ucl;

// Relational builtins which take a single argument
TEST_F(OneArgRelational, IsFinite_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_half x) -> bool { return IsFinite(x); };
  TestAgainstReference<cl_half>("isfinite", half_ref);
}

TEST_F(OneArgRelational, IsFinite_Float) {
  auto float_ref = [](cl_float x) -> bool { return std::isfinite(x); };
  TestAgainstReference<cl_float>("isfinite", float_ref);
}

TEST_F(OneArgRelational, IsFinite_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_double x) -> bool { return std::isfinite(x); };
  TestAgainstReference<cl_double>("isfinite", double_ref);
}

TEST_F(OneArgRelational, IsInf_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_half x) -> bool { return IsInf(x); };
  TestAgainstReference<cl_half>("isinf", half_ref);
}

TEST_F(OneArgRelational, IsInf_Float) {
  auto float_ref = [](cl_float x) -> bool { return std::isinf(x); };
  TestAgainstReference<cl_float>("isinf", float_ref);
}

TEST_F(OneArgRelational, IsInf_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_double x) -> bool { return std::isinf(x); };
  TestAgainstReference<cl_double>("isinf", double_ref);
}

TEST_F(OneArgRelational, IsNan_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_half x) -> bool { return IsNaN(x); };
  TestAgainstReference<cl_half>("isnan", half_ref);
}

TEST_F(OneArgRelational, IsNan_Float) {
  auto float_ref = [](cl_float x) -> bool { return std::isnan(x); };
  TestAgainstReference<cl_float>("isnan", float_ref);
}

TEST_F(OneArgRelational, IsNan_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_double x) -> bool { return std::isnan(x); };
  TestAgainstReference<cl_double>("isnan", double_ref);
}

TEST_F(OneArgRelational, IsNormal_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_half x) -> bool { return IsNormal(x); };
  TestAgainstReference<cl_half>("isnormal", half_ref);
}

TEST_F(OneArgRelational, IsNormal_Float) {
  auto float_ref = [](cl_float x) -> bool { return std::isnormal(x); };
  TestAgainstReference<cl_float>("isnormal", float_ref);
}

TEST_F(OneArgRelational, IsNormal_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_double x) -> bool { return std::isnormal(x); };
  TestAgainstReference<cl_double>("isnormal", double_ref);
}

TEST_F(OneArgRelational, SignBit_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_half x) -> bool {
    const cl_short as_short = cargo::bit_cast<cl_short>(x);
    return as_short < 0;
  };
  TestAgainstReference<cl_half>("signbit", half_ref);
}

TEST_F(OneArgRelational, SignBit_Float) {
  auto float_ref = [](cl_float x) -> bool { return std::signbit(x); };
  TestAgainstReference<cl_float>("signbit", float_ref);
}

TEST_F(OneArgRelational, SignBit_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_double x) -> bool { return std::signbit(x); };
  TestAgainstReference<cl_double>("signbit", double_ref);
}

// Relational builtins which take two arguments
TEST_F(TwoArgRelational, IsEqual_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_half x, cl_half y) -> bool {
    // Equality with NaN is always false
    if (IsNaN(x) || IsNaN(y)) {
      return false;
    }
    return x == y;
  };
  TestAgainstReference<cl_half>("isequal", half_ref);
}

TEST_F(TwoArgRelational, IsEqual_Float) {
  auto float_ref = [](cl_float x, cl_float y) -> bool { return x == y; };
  TestAgainstReference<cl_float>("isequal", float_ref);
}

TEST_F(TwoArgRelational, IsEqual_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_double x, cl_double y) -> bool { return x == y; };
  TestAgainstReference<cl_double>("isequal", double_ref);
}

TEST_F(TwoArgRelational, IsNotEqual_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_half x, cl_half y) -> bool {
    // Equality with NaN is always false
    if (IsNaN(x) || IsNaN(y)) {
      return true;
    }
    return x != y;
  };
  TestAgainstReference<cl_half>("isnotequal", half_ref);
}

TEST_F(TwoArgRelational, IsNotEqual_Float) {
  auto float_ref = [](cl_float x, cl_float y) -> bool { return x != y; };
  TestAgainstReference<cl_float>("isnotequal", float_ref);
}

TEST_F(TwoArgRelational, IsNotEqual_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_double x, cl_double y) -> bool { return x != y; };
  TestAgainstReference<cl_double>("isnotequal", double_ref);
}

TEST_F(TwoArgRelational, IsGreater_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_half x, cl_half y) -> bool {
    // Comparison with NaN is always false
    if (IsNaN(x) || IsNaN(y)) {
      return false;
    }

    // Check if arguments have a different sign, +ve always greater than -ve
    const bool x_neg = (x & TypeInfo<cl_half>::sign_bit) != 0;
    const bool y_neg = (y & TypeInfo<cl_half>::sign_bit) != 0;
    if (x_neg != y_neg) {
      return !x_neg;
    }

    // If exponents are different, we don't need to check the mantissa
    const cl_ushort exp_mask = TypeInfo<cl_half>::exponent_mask;
    const cl_ushort x_exp = x & exp_mask;
    const cl_ushort y_exp = y & exp_mask;
    if (x_exp != y_exp) {
      return x_neg ? x_exp < y_exp : x_exp > y_exp;
    }

    const cl_ushort mantissa_mask = TypeInfo<cl_half>::mantissa_mask;
    const cl_ushort x_mant = x & mantissa_mask;
    const cl_ushort y_mant = y & mantissa_mask;
    return x_neg ? x_mant < y_mant : x_mant > y_mant;
  };
  TestAgainstReference<cl_half>("isgreater", half_ref);
}

TEST_F(TwoArgRelational, IsGreater_Float) {
  auto float_ref = [](cl_float x, cl_float y) -> bool { return x > y; };
  TestAgainstReference<cl_float>("isgreater", float_ref);
}

TEST_F(TwoArgRelational, IsGreater_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_double x, cl_double y) -> bool { return x > y; };
  TestAgainstReference<cl_double>("isgreater", double_ref);
}

TEST_F(TwoArgRelational, IsGreaterEqual_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_half x, cl_half y) -> bool {
    // Comparison with NaN is always false
    if (IsNaN(x) || IsNaN(y)) {
      return false;
    }

    // Check if arguments have a different sign, +ve always greater than -ve
    const bool x_neg = (x & TypeInfo<cl_half>::sign_bit) != 0;
    const bool y_neg = (y & TypeInfo<cl_half>::sign_bit) != 0;
    if (x_neg != y_neg) {
      return !x_neg;
    }

    // If exponents are different, we don't need to check the mantissa
    const cl_ushort exp_mask = TypeInfo<cl_half>::exponent_mask;
    const cl_ushort x_exp = x & exp_mask;
    const cl_ushort y_exp = y & exp_mask;
    if (x_exp != y_exp) {
      return x_neg ? x_exp < y_exp : x_exp > y_exp;
    }

    const cl_ushort mantissa_mask = TypeInfo<cl_half>::mantissa_mask;
    const cl_ushort x_mant = x & mantissa_mask;
    const cl_ushort y_mant = y & mantissa_mask;
    return x_neg ? x_mant <= y_mant : x_mant >= y_mant;
  };
  TestAgainstReference<cl_half>("isgreaterequal", half_ref);
}

TEST_F(TwoArgRelational, IsGreaterEqual_Float) {
  auto float_ref = [](cl_float x, cl_float y) -> bool { return x >= y; };
  TestAgainstReference<cl_float>("isgreaterequal", float_ref);
}

TEST_F(TwoArgRelational, IsGreaterEqual_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_double x, cl_double y) -> bool { return x >= y; };
  TestAgainstReference<cl_double>("isgreaterequal", double_ref);
}

TEST_F(TwoArgRelational, IsLess_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_half x, cl_half y) -> bool {
    // Comparison with NaN is always false
    if (IsNaN(x) || IsNaN(y)) {
      return false;
    }

    // Check if arguments have a different sign, -ve always less than +ve
    const bool x_neg = (x & TypeInfo<cl_half>::sign_bit) != 0;
    const bool y_neg = (y & TypeInfo<cl_half>::sign_bit) != 0;
    if (x_neg != y_neg) {
      return x_neg;
    }

    // If exponents are different, we don't need to check the mantissa
    const cl_ushort exp_mask = TypeInfo<cl_half>::exponent_mask;
    const cl_ushort x_exp = x & exp_mask;
    const cl_ushort y_exp = y & exp_mask;
    if (x_exp != y_exp) {
      return x_neg ? x_exp > y_exp : x_exp < y_exp;
    }

    const cl_ushort mantissa_mask = TypeInfo<cl_half>::mantissa_mask;
    const cl_ushort x_mant = x & mantissa_mask;
    const cl_ushort y_mant = y & mantissa_mask;
    return x_neg ? x_mant > y_mant : x_mant < y_mant;
  };
  TestAgainstReference<cl_half>("isless", half_ref);
}

TEST_F(TwoArgRelational, IsLess_Float) {
  auto float_ref = [](cl_float x, cl_float y) -> bool { return x < y; };
  TestAgainstReference<cl_float>("isless", float_ref);
}

TEST_F(TwoArgRelational, IsLess_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_double x, cl_double y) -> bool { return x < y; };
  TestAgainstReference<cl_double>("isless", double_ref);
}

TEST_F(TwoArgRelational, IsLessEqual_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_half x, cl_half y) -> bool {
    // Comparison with NaN is always false
    if (IsNaN(x) || IsNaN(y)) {
      return false;
    }

    // Check if arguments have a different sign, -ve always less than +ve
    const bool x_neg = (x & TypeInfo<cl_half>::sign_bit) != 0;
    const bool y_neg = (y & TypeInfo<cl_half>::sign_bit) != 0;
    if (x_neg != y_neg) {
      return x_neg;
    }

    // If exponents are different, we don't need to check the mantissa
    const cl_ushort exp_mask = TypeInfo<cl_half>::exponent_mask;
    const cl_ushort x_exp = x & exp_mask;
    const cl_ushort y_exp = y & exp_mask;
    if (x_exp != y_exp) {
      return x_neg ? x_exp > y_exp : x_exp < y_exp;
    }

    const cl_ushort mantissa_mask = TypeInfo<cl_half>::mantissa_mask;
    const cl_ushort x_mant = x & mantissa_mask;
    const cl_ushort y_mant = y & mantissa_mask;
    return x_neg ? x_mant >= y_mant : x_mant <= y_mant;
  };
  TestAgainstReference<cl_half>("islessequal", half_ref);
}

TEST_F(TwoArgRelational, IsLessEqual_Float) {
  auto float_ref = [](cl_float x, cl_float y) -> bool { return x <= y; };
  TestAgainstReference<cl_float>("islessequal", float_ref);
}

TEST_F(TwoArgRelational, IsLessEqual_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_double x, cl_double y) -> bool { return x <= y; };
  TestAgainstReference<cl_double>("islessequal", double_ref);
}

TEST_F(TwoArgRelational, IsLessGreater_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_half x, cl_half y) -> bool {
    // Comparison with NaN is always false
    if (IsNaN(x) || IsNaN(y)) {
      return false;
    }

    // Check if arguments have a different sign they can't be equal
    const cl_ushort x_neg = x & TypeInfo<cl_half>::sign_bit;
    const cl_ushort y_neg = y & TypeInfo<cl_half>::sign_bit;
    if (x_neg != y_neg) {
      return true;
    }

    // If exponents are different, we don't need to check the mantissa
    const cl_ushort exp_mask = TypeInfo<cl_half>::exponent_mask;
    const cl_ushort x_exp = x & exp_mask;
    const cl_ushort y_exp = y & exp_mask;
    if (x_exp != y_exp) {
      return true;
    }

    const cl_ushort mantissa_mask = TypeInfo<cl_half>::mantissa_mask;
    const cl_ushort x_mant = x & mantissa_mask;
    const cl_ushort y_mant = y & mantissa_mask;
    return x_mant != y_mant;
  };
  TestAgainstReference<cl_half>("islessgreater", half_ref);
}

TEST_F(TwoArgRelational, IsLessGreater_Float) {
  auto float_ref = [](cl_float x, cl_float y) -> bool {
    return (x < y) || (x > y);
  };
  TestAgainstReference<cl_float>("islessgreater", float_ref);
}

TEST_F(TwoArgRelational, IsLessGreater_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_double x, cl_double y) -> bool {
    return (x < y) || (x > y);
  };
  TestAgainstReference<cl_double>("islessgreater", double_ref);
}

TEST_F(TwoArgRelational, IsOrdered_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_half x, cl_half y) -> bool {
    return !IsNaN(x) && !IsNaN(y);
  };
  TestAgainstReference<cl_half>("isordered", half_ref);
}

TEST_F(TwoArgRelational, IsOrdered_Float) {
  auto float_ref = [](cl_float x, cl_float y) -> bool {
    return (x == x) && (y == y);
  };
  TestAgainstReference<cl_float>("isordered", float_ref);
}

TEST_F(TwoArgRelational, IsOrdered_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_double x, cl_double y) -> bool {
    return (x == x) && (y == y);
  };
  TestAgainstReference<cl_double>("isordered", double_ref);
}

TEST_F(TwoArgRelational, IsUnordered_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_half x, cl_half y) -> bool {
    return IsNaN(x) || IsNaN(y);
  };
  TestAgainstReference<cl_half>("isunordered", half_ref);
}

TEST_F(TwoArgRelational, IsUnordered_Float) {
  auto float_ref = [](cl_float x, cl_float y) -> bool {
    return std::isnan(x) || std::isnan(y);
  };
  TestAgainstReference<cl_float>("isunordered", float_ref);
}

TEST_F(TwoArgRelational, IsUnordered_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_double x, cl_double y) -> bool {
    return std::isnan(x) || std::isnan(y);
  };
  TestAgainstReference<cl_double>("isunordered", double_ref);
}

TEST_F(BitSelectTest, BitSelect_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_ushort a, cl_ushort b, cl_ushort c) -> cl_ushort {
    cl_ushort result = 0;
    for (int i = 0; i < 16; i++) {
      const cl_ushort mask = 0x1 << i;
      const cl_ushort a_masked = a & mask;
      const cl_ushort b_masked = b & mask;
      const cl_ushort c_masked = c & mask;
      result = c_masked == 0 ? result | a_masked : result | b_masked;
    }

    return result;
  };
  TestAgainstReference<cl_half, cl_ushort>(half_ref);
}

TEST_F(BitSelectTest, BitSelect_Float) {
  auto float_ref = [](cl_uint a, cl_uint b, cl_uint c) -> cl_uint {
    cl_uint result = 0;
    for (int i = 0; i < 32; i++) {
      const cl_uint mask = 0x1 << i;
      const cl_uint a_masked = a & mask;
      const cl_uint b_masked = b & mask;
      const cl_uint c_masked = c & mask;
      result = c_masked == 0 ? result | a_masked : result | b_masked;
    }

    return result;
  };
  TestAgainstReference<cl_float, cl_uint>(float_ref);
}

TEST_F(BitSelectTest, BitSelect_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_ulong a, cl_ulong b, cl_ulong c) -> cl_ulong {
    cl_ulong result = 0;
    for (int i = 0; i < 64; i++) {
      const cl_ulong mask = static_cast<cl_ulong>(0x1) << i;
      const cl_ulong a_masked = a & mask;
      const cl_ulong b_masked = b & mask;
      const cl_ulong c_masked = c & mask;
      result = c_masked == 0 ? result | a_masked : result | b_masked;
    }
    return result;
  };
  TestAgainstReference<cl_double, cl_ulong>(double_ref);
}

TEST_F(SelectTest, UnsignedVector_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_half a, cl_half b, cl_ushort c) -> cl_half {
    return (c & TypeInfo<cl_half>::sign_bit) ? b : a;
  };
  TestAgainstReference<cl_half, cl_ushort>(half_ref, false);
}

TEST_F(SelectTest, UnsignedVector_Float) {
  auto float_ref = [](cl_float a, cl_float b, cl_uint c) -> cl_float {
    return (c & TypeInfo<cl_float>::sign_bit) ? b : a;
  };
  TestAgainstReference<cl_float, cl_uint>(float_ref, false);
}

TEST_F(SelectTest, UnsignedVector_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_double a, cl_double b, cl_ulong c) -> cl_double {
    return (c & TypeInfo<cl_double>::sign_bit) ? b : a;
  };
  TestAgainstReference<cl_double, cl_ulong>(double_ref, false);
}

TEST_F(SelectTest, SignedVector_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_half a, cl_half b, cl_short c) -> cl_half {
    return (c & TypeInfo<cl_half>::sign_bit) ? b : a;
  };
  TestAgainstReference<cl_half, cl_short>(half_ref, false);
}

TEST_F(SelectTest, SignedVector_Float) {
  auto float_ref = [](cl_float a, cl_float b, cl_int c) -> cl_float {
    return (c & TypeInfo<cl_float>::sign_bit) ? b : a;
  };
  TestAgainstReference<cl_float, cl_int>(float_ref, false);
}

TEST_F(SelectTest, SignedVector_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_double a, cl_double b, cl_long c) -> cl_double {
    return (c & TypeInfo<cl_double>::sign_bit) ? b : a;
  };
  TestAgainstReference<cl_double, cl_long>(double_ref, false);
}

TEST_F(SelectTest, UnsignedScalar_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_half a, cl_half b, cl_ushort c) -> cl_half {
    return c ? b : a;
  };
  TestAgainstReference<cl_half, cl_ushort>(half_ref, true);
}

TEST_F(SelectTest, UnsignedScalar_Float) {
  auto float_ref = [](cl_float a, cl_float b, cl_uint c) -> cl_float {
    return c ? b : a;
  };
  TestAgainstReference<cl_float, cl_uint>(float_ref, true);
}

TEST_F(SelectTest, UnsignedScalar_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_double a, cl_double b, cl_ulong c) -> cl_double {
    return c ? b : a;
  };
  TestAgainstReference<cl_double, cl_ulong>(double_ref, true);
}

TEST_F(SelectTest, SignedScalar_Half) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto half_ref = [](cl_half a, cl_half b, cl_short c) -> cl_half {
    return c ? b : a;
  };
  TestAgainstReference<cl_half, cl_short>(half_ref, true);
}

TEST_F(SelectTest, SignedScalar_Float) {
  auto float_ref = [](cl_float a, cl_float b, cl_int c) -> cl_float {
    return c ? b : a;
  };
  TestAgainstReference<cl_float, cl_int>(float_ref, true);
}

TEST_F(SelectTest, SignedScalar_Double) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  auto double_ref = [](cl_double a, cl_double b, cl_long c) -> cl_double {
    return c ? b : a;
  };
  TestAgainstReference<cl_double, cl_long>(double_ref, true);
}
