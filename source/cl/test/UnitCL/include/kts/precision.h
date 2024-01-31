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

#ifndef UNITCL_KTS_PRECISION_H_INCLUDED
#define UNITCL_KTS_PRECISION_H_INCLUDED

#include <CL/cl.h>
#include <CL/cl_ext.h>  // For CL_DEVICE_HALF_FP_CONFIG.
#include <CL/cl_platform.h>

#include <algorithm>
#include <array>
#include <limits>
#include <random>
#include <sstream>
#include <vector>

#include "Common.h"
#include "cargo/type_traits.h"
#include "cargo/utility.h"
#include "kts/arguments_shared.h"
#include "kts/generator.h"
#include "kts/type_info.h"
#include "ucl/enums.h"

namespace kts {
namespace ucl {

// NaN ULP error is represented by CL_ULONG_MAX. Defining our maximum allowable
// ULP error for builtin functions with unspecified precision as less than
// CL_ULONG_MAX means we can raise errors when finiteness of result doesn't
// match expectations.
const cl_ulong MAX_ULP_ERROR = (CL_ULONG_MAX - cl_ulong(1));

// Constants used in in the half tests various operators
namespace HalfInputSizes {
const unsigned quick = 128;   // Token number for smoke testing
const unsigned wimpy = 8192;  // Test 8k input values
const unsigned full = 65536;  // Test full range of possible half values

unsigned getInputSize(const ::ucl::MathMode);
}  // namespace HalfInputSizes

/// @brief Floating point rounding modes
enum class RoundingMode {
  NONE,  // No modifier, default rounding mode
  RTE,   // Round to nearest even
  RTZ,   // Round to zero
  RTP,   // Round to positive infinity
  RTN    // Round to negative infinity
};

inline std::ostream &operator<<(std::ostream &out, const RoundingMode &mode) {
  switch (mode) {
#define CASE(MODE)                 \
  case RoundingMode::MODE:         \
    out << "RoundingMode::" #MODE; \
    break;
    CASE(NONE)
    CASE(RTE)
    CASE(RTZ)
    CASE(RTP)
    CASE(RTN)
#undef CASE
  }
  return out;
}

/// @brief Strong Type wrapper class so we can use template specializations for
/// cl_* types typedefed to the same underlying type.
template <typename T, typename Parameter>
class NamedType {
 public:
  explicit NamedType(const T &value) : value_(value) {}
  explicit NamedType(T &&value) : value_(std::move(value)) {}
  T &get() { return value_; }
  const T &get() const { return value_; }

  using WrappedT = T;

 private:
  T value_;
};

using CLuchar = NamedType<cl_uchar, struct UCharParameter>;
using CLchar = NamedType<cl_char, struct CharParameter>;
using CLushort = NamedType<cl_ushort, struct UShortParameter>;
using CLshort = NamedType<cl_short, struct ShortParameter>;
using CLuint = NamedType<cl_uint, struct UIntParameter>;
using CLint = NamedType<cl_int, struct IntParameter>;
using CLulong = NamedType<cl_ulong, struct ULongParameter>;
using CLlong = NamedType<cl_long, struct LongParameter>;
using CLhalf = NamedType<cl_half, struct HalfParameter>;
using CLfloat = NamedType<cl_float, struct FloatParameter>;
using CLdouble = NamedType<cl_double, struct DoubleParameter>;
using CLbool = NamedType<cl_bool, struct BoolParameter>;

/// @brief Helper class for getting C string representation of cl_* types
template <class T>
struct Stringify;

#define STRONG_STRINGIFY(TYPE)                   \
  template <>                                    \
  struct Stringify<CL##TYPE> final {             \
    static constexpr const char *as_str = #TYPE; \
  };

#define WEAK_STRINGIFY(TYPE)                     \
  template <>                                    \
  struct Stringify<cl_##TYPE> final {            \
    static constexpr const char *as_str = #TYPE; \
  };

STRONG_STRINGIFY(uchar);
WEAK_STRINGIFY(uchar);
STRONG_STRINGIFY(char);
WEAK_STRINGIFY(char);
STRONG_STRINGIFY(ushort);
WEAK_STRINGIFY(ushort);
STRONG_STRINGIFY(short);
WEAK_STRINGIFY(short);
STRONG_STRINGIFY(uint);
WEAK_STRINGIFY(uint);
STRONG_STRINGIFY(int);
WEAK_STRINGIFY(int);
STRONG_STRINGIFY(ulong);
WEAK_STRINGIFY(ulong);
STRONG_STRINGIFY(long);
WEAK_STRINGIFY(long);
STRONG_STRINGIFY(half);
STRONG_STRINGIFY(float);
WEAK_STRINGIFY(float);
STRONG_STRINGIFY(double);
WEAK_STRINGIFY(double);
STRONG_STRINGIFY(bool);
#undef STRONG_STRINGIFY
#undef WEAK_STRINGIFY

template <typename T>
struct Helper;

template <>
struct Helper<cl_half> final {
  typedef cl_short ConvertType;
};

template <>
struct Helper<cl_short> final {
  typedef cl_half ConvertType;
};

template <>
struct Helper<cl_float> final {
  typedef cl_int ConvertType;
};

template <>
struct Helper<cl_int> final {
  typedef cl_float ConvertType;
};

template <>
struct Helper<cl_double> final {
  typedef cl_long ConvertType;
};

template <>
struct Helper<cl_long> final {
  typedef cl_double ConvertType;
};

/// @brief Bit casts between integer and float types of the same size
template <typename T>
typename Helper<T>::ConvertType matchingType(T t) {
  return cargo::bit_cast<typename Helper<T>::ConvertType>(t);
}

/// @brief Discovers if input is one of the possible half precision NaN values
//
/// @param x 16-bit half value to check
///
/// @return True if input is NaN, false otherwise
bool IsNaN(cl_half x);

/// @brief Discovers if input is positive or negative infinity
//
/// @param x 16-bit half value to check
///
/// @return True if input is +/- Inf, false otherwise
bool IsInf(cl_half x);

/// @brief Discovers if input is not an infinity or NaN value
//
/// @param x 16-bit half value to check
///
/// @return True if input is finite, false otherwise
bool IsFinite(cl_half x);

/// @brief Discovers if input is a normal floating point value
//
/// @param x 16-bit half value to check
///
/// @return True if input is normal, false otherwise
bool IsNormal(cl_half x);

/// @brief Converts a half float to single precision
//
/// @param x 16-bit half value to cast
///
/// @return 32-bit float representation of input
cl_float ConvertHalfToFloat(const cl_half x);

/// @brief Converts single or double precision float to half precision
//
/// @param x single/double precision float value to downcast
/// @param rounding Rounding mode to use
///
/// @return 16-bit float representation of input
template <class T>
cl_half ConvertFloatToHalf(const T x,
                           const RoundingMode rounding = RoundingMode::NONE);

/// @brief Calculates the ULP between two floating points values, ignoring
///        the mantissa bits not available in half precision.
///
/// @param reference        Reference 32-bit float value
/// @param test             Half precision value we've calculated
///
/// @return ULP calculated as a float value.
cl_float calcHalfPrecisionULP(const cl_float reference, const cl_half test);

/// @brief Calculates the ULP between two floating points values, ignoring
///        the mantissa bits not available in half precision.
///
/// @param reference        Reference 64-bit float value
/// @param test             Half precision value we've calculated
///
/// @return ULP calculated as a double precision value.
cl_double calcHalfPrecisionULP(const cl_double reference, const cl_half test);

/// @brief Discovers if input is a finite denormal number when converted from
///        single precision to half precision
//
/// @param x 32-bit float value to check
///
/// @return True if input is a denormal, false otherwise
bool IsDenormalAsHalf(cl_float x);

/// @brief Calculate ULP error of test result against reference value
///
/// Based on ULP calculation in CTS functions `Ulp_Error_Double()` and
/// `Ulp_Error()` from test_common/harness/errorHelpers.cpp
///
/// @tparam T Floating point type of test result. Must be `cl_float` or
/// `cl_double`, use `calcHalfPrecisionULP()` instead for `cl_half`.
///
///
/// @param reference Correct result, of higher or equal precision to T
/// @param actual Value from test we want to calculate error of.
///
/// @return ULP error from reference
template <class T>
cl_float calculateULP(const typename TypeInfo<T>::LargerType reference,
                      const T actual) {
  static_assert(std::is_same_v<T, cl_float> || std::is_same_v<T, cl_double>,
                "T must be cl_float or cl_double");

  if (static_cast<T>(reference) == actual) {
    // catches reference overflow and underflow
    return 0.0f;
  }

  if (std::isnan(reference) && std::isnan(actual)) {
    // NaNs don't need to be bit exact
    return 0.0f;
  }

  // Promote our test result to the same precision as the reference
  using LargerType = typename TypeInfo<T>::LargerType;
  LargerType promoted(actual);

  if (std::isinf(reference)) {
    if (promoted == reference) {
      return 0.0f;
    }

    return static_cast<cl_float>(promoted - reference);
  }

  if (std::isinf(promoted) && std::is_same_v<T, cl_float>) {
    // 2**128 is next representable value on the single-precision number line
    promoted = std::copysign(3.4028237e+38, promoted);
  }

  int reference_exp = std::ilogb(reference);
  int unused_exp;
  if (LargerType(0.5) == std::frexp(reference, &unused_exp)) {
    // Reference is power of two
    reference_exp--;
  }

  constexpr int exp_lower_bound = TypeInfo<T>::min_exp - 1;
  const int ulp_exp =
      TypeInfo<T>::mantissa_bits - std::max(reference_exp, exp_lower_bound);

  // Scale the absolute error by the exponent
  cl_float result = std::scalbn(promoted - reference, ulp_exp);

  // account for rounding error in reference result on systems that do not have
  // a higher precision floating point type, i.e long double
  if (sizeof(typename TypeInfo<T>::LargerType) == sizeof(T)) {
    result += std::copysign(0.5f, result);
  }
  return result;
}

template <class T>
bool IsDenormal(T x) {
  using IntTy = typename TypeInfo<T>::AsSigned;
  const IntTy exp_mask = TypeInfo<T>::exponent_mask;
  const IntTy mantissa_mask = TypeInfo<T>::mantissa_mask;

  IntTy asInt = cargo::bit_cast<IntTy>(x);
  return (0 == (asInt & exp_mask)) && (asInt & mantissa_mask);
}

/// @brief Determines whether result is any NaN value, used to verify `isnan()`
struct NaNValidator final {
  bool validate(const cl_float, const cl_half &actual) {
    return kts::ucl::IsNaN(actual);
  }

  void print(std::stringstream &s, cl_half value) {
    s << value << "[0x" << std::hex << matchingType(value) << std::dec << "]";
  }
};

/// @brief Verifies that two float values are within a template defined ULP
template <typename T, cl_ulong ULP, bool test_denormals = true>
struct ULPValidator final {
  ULPValidator(cl_device_id device) : device(device), ulp_err(0.0f) {}

  using LargerType = typename TypeInfo<T>::LargerType;
  bool validate(const LargerType &expected, const T &actual) {
    bool denormSupport =
        test_denormals &&
        UCL::hasDenormSupport(device, std::is_same_v<T, cl_float>
                                          ? CL_DEVICE_SINGLE_FP_CONFIG
                                          : CL_DEVICE_DOUBLE_FP_CONFIG);
    // Note that we cannot use `std::isnormal` or `std::fpclassify` to detect
    // denormals/subnormals on platforms that don't support them, because they
    // will always report "normal" regardless. We have to look at the actual
    // bits!
    if (!denormSupport && IsDenormal<T>(expected) && ((T)0.0 == actual)) {
      // Accept +/- 0.0 if denormals aren't supported and result was a denormal
      return true;
    }

    ulp_err = kts::ucl::calculateULP(expected, actual);
    return (std::fabs(ulp_err) <= static_cast<cl_float>(ULP)) ||
           (std::isinf(ulp_err) && (ULP == MAX_ULP_ERROR));
  }

  template <typename Y>
  void print(std::stringstream &s, Y value) {
    s << value << "[0x" << std::hex << matchingType(value) << std::dec << "]";

    if (std::is_same_v<Y, T>) {
      printULPError(s);
    }
  }

  void print(std::stringstream &s, long double value) { s << value; }

  void printULPError(std::stringstream &s) {
    s << ". ULP error " << ulp_err;
    if (MAX_ULP_ERROR == ULP) {
      // We treat MAX_ULP_ERROR as infinite ULP tolerance if the spec says
      // that a function is allowed implementation defined or infinite ULP
      // error. We can still violate this tolerance however, printing an error
      // message here, when the result has NAN ULP error. E.g we return a NAN
      // but the reference is a finite value.
      s << " is a fail even for results allowed infinite ULP error";
    } else {
      s << " exceeded " << ULP << " ULP error tolerance";
    }
  }

 private:
  cl_device_id device;
  cl_float ulp_err;
};

/// @brief Verifies that two half values are within a template defined ULP
template <cl_ulong ULP, bool test_denormals>
struct ULPValidator<cl_half, ULP, test_denormals> final {
  ULPValidator(cl_device_id) : ulp_err(0.0f) {}

  bool validate(const cl_float &expected, const cl_half &actual) {
    ulp_err = calcHalfPrecisionULP(expected, actual);
    return (std::fabs(ulp_err) <= static_cast<cl_float>(ULP)) ||
           (std::isinf(ulp_err) && (ULP == MAX_ULP_ERROR));
  }

  bool validate(const cl_double &expected, const cl_half &actual) {
    ulp_err = calcHalfPrecisionULP(expected, actual);
    return (std::fabs(ulp_err) <= static_cast<cl_double>(ULP)) ||
           (std::isinf(ulp_err) && (ULP == MAX_ULP_ERROR));
  }

  void print(std::stringstream &s, cl_half value) {
    const cl_float as_float = ConvertHalfToFloat(value);

    s << "half " << as_float;
    s << "[0x" << std::hex << matchingType(value) << "]" << std::dec;

    s << ". ULP error " << ulp_err;
    if (MAX_ULP_ERROR == ULP) {
      // We treat MAX_ULP_ERROR as infinite ULP tolerance if the spec says
      // that a function is allowed implementation defined or infinite ULP
      // error. We can still violate this tolerance however, printing an error
      // message here, when the result has NAN ULP error. E.g we return a NAN
      // but the reference is a finite value.
      s << " is a fail even for results allowed infinite ULP error";
    } else {
      s << " exceeded " << ULP << " ULP error tolerance";
    }
  }

  void print(std::stringstream &s, cl_float value) {
    s << "float " << value;
    s << "[0x" << std::hex << matchingType(value) << "]" << std::dec;

    const cl_half as_half = ConvertFloatToHalf(value);
    s << " -> half " << ConvertHalfToFloat(as_half);
    s << "[0x" << std::hex << as_half << "]" << std::dec;
  }

  void print(std::stringstream &s, cl_double value) {
    s << "double " << value;
    s << "[0x" << std::hex << matchingType(value) << "]" << std::dec;

    const cl_half as_half = ConvertFloatToHalf(value);
    s << " -> half " << ConvertHalfToFloat(as_half);
    s << "[0x" << std::hex << as_half << "]" << std::dec;
  }

 private:
  cl_float ulp_err;
};

template <typename T, cl_ulong ULP, bool test_denormals = true, typename F>
auto makeULPStreamer(F &&f, cl_device_id device) {
  // Reference should be a more precise floating point type
  using RefType = typename std::result_of<F(size_t)>::type;
  static_assert(
      sizeof(RefType) >= sizeof(T),
      "Reference type should be at least as precise as the actual type");

  using ValidatorType = ULPValidator<T, ULP, test_denormals>;
  auto s = std::make_shared<GenericStreamer<T, ValidatorType, RefType>>(
      std::forward<F>(f), ValidatorType{device});

  return s;
}

template <typename T, cl_ulong ULP, bool test_denormals = true, typename F>
auto makeULPStreamer(
    F &&ref,
    const std::vector<kts::Reference1D<typename TypeInfo<T>::LargerType>>
        &&fallbacks,
    cl_device_id device) {
  // Reference should be a more precise floating point type
  using RefType = typename std::result_of<F(size_t)>::type;
  static_assert(
      sizeof(RefType) >= sizeof(T),
      "Reference type should be at least as precise as the actual type");

  using ValidatorType = ULPValidator<T, ULP, test_denormals>;
  auto s = std::make_shared<GenericStreamer<T, ValidatorType, RefType>>(
      std::forward<F>(ref), std::forward<decltype(fallbacks)>(fallbacks),
      ValidatorType{device});
  return s;
}
}  // namespace ucl
}  // namespace kts

#endif  // UNITCL_KTS_PRECISION_H_INCLUDED
