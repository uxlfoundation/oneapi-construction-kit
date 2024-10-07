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

#ifndef UNITCL_KTS_TYPE_INFO_H_INCLUDES
#define UNITCL_KTS_TYPE_INFO_H_INCLUDES

#include <CL/cl.h>

#include <limits>

namespace kts {
namespace ucl {
/// @brief Provides type traits information on OpenCL floating point types
///
/// @tparam T Floating point type to get info for
template <class T>
struct TypeInfo;

/// @brief Type trait info for 16-bit half precision float
template <>
struct TypeInfo<cl_half> {
  static constexpr const char *as_str = "half";
  static constexpr const char *as_signed_str = "short";
  using AsUnsigned = cl_ushort;
  using AsSigned = cl_short;
  using LargerType = cl_float;

  static const unsigned mantissa_bits = 10;
  static const int bias = 15;

  static const AsUnsigned mantissa_mask = 0x03FF;
  static const AsUnsigned exponent_mask = 0x7C00;
  static const AsUnsigned low_exp_mask = 0x0400;
  static const AsUnsigned sign_bit = 0x8000;

  static const AsUnsigned max_float_bits = 0x7BFF;
  static const AsUnsigned max_int_bits = 65504;

  static constexpr cl_float lowest = 5.960464477539063e-08f;
  static constexpr cl_float max = 65504.0f;

  static const AsSigned min_exp = CL_HALF_MIN_EXP;
};

/// @brief Type trait info for 32-bit single precision float
template <>
struct TypeInfo<cl_float> {
  static constexpr const char *as_str = "float";
  static constexpr const char *as_signed_str = "int";
  using AsUnsigned = cl_uint;
  using AsSigned = cl_int;
  using LargerType = cl_double;

  static const unsigned mantissa_bits = 23;
  static const int bias = 127;

  static const AsUnsigned mantissa_mask = 0x007FFFFF;
  static const AsUnsigned exponent_mask = 0x7F800000;
  static const AsUnsigned low_exp_mask = 0x00800000;
  static const AsUnsigned sign_bit = 0x80000000;

  static constexpr cl_float lowest = std::numeric_limits<cl_float>::lowest();
  static constexpr cl_float max = std::numeric_limits<cl_float>::max();

  static const AsSigned min_exp = CL_FLT_MIN_EXP;
};

/// @brief Type trait info for 64-bit double precision float
template <>
struct TypeInfo<cl_double> {
  static constexpr const char *as_str = "double";
  static constexpr const char *as_signed_str = "long";
  using AsUnsigned = cl_ulong;
  using AsSigned = cl_long;
  using LargerType = long double;

  static const unsigned mantissa_bits = 52;

  static const AsUnsigned mantissa_mask = 0x000FFFFFFFFFFFFF;
  static const AsUnsigned exponent_mask = 0x7FF0000000000000;
  static const AsUnsigned low_exp_mask = 0x0010000000000000;
  static const AsUnsigned sign_bit = 0x8000000000000000;

  static constexpr cl_double lowest = std::numeric_limits<cl_double>::lowest();
  static constexpr cl_double max = std::numeric_limits<cl_double>::max();

  static const AsSigned min_exp = CL_DBL_MIN_EXP;
};
}  // namespace ucl
}  // namespace kts

#endif  // UNITCL_KTS_TYPE_INFO_H_INCLUDES
