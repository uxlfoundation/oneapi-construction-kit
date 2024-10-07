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

#include <limits.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include <array>

#include "Common.h"
#include "cargo/utility.h"
#include "kts/precision.h"

using namespace kts::ucl;

namespace {
/// @brief Remquo implementation for 7-bit quotient
///
/// `std::remquo` is only guaranteed to return the quotient to 3-bits of
/// precision, but OpenCL 1.2 specifies 7-bits of precision. This reference
/// implementation is based on the CTS reference function.
template <typename T>
T Remquo7BitRef(T x, T y, cl_int &quo_out) {
  if (std::isnan(x) || std::isnan(y) || std::isinf(x) || y == T(0.0)) {
    quo_out = 0;
    return NAN;
  }

  if (std::isinf(y) || x == T(0.0)) {
    quo_out = 0;
    return x;
  }

  if (std::fabs(x) == std::fabs(y)) {
    quo_out = (x == y) ? 1 : -1;
    return std::signbit(x) ? T(-0.0) : T(0.0);
  }

  const T xAbs = std::fabs(x);
  const T yAbs = std::fabs(y);

  const int ex = std::ilogb(x);
  const int ey = std::ilogb(y);

  T xr = xAbs;
  T yr = yAbs;
  cl_uint q = 0;

  if (ex - ey >= -1) {
    yr = std::ldexp(yAbs, -ey);
    xr = std::ldexp(xAbs, -ex);

    if (ex - ey >= 0) {
      int i;
      for (i = ex - ey; i > 0; i--) {
        q <<= 1;
        if (xr >= yr) {
          xr -= yr;
          q += 1;
        }
        xr += xr;
      }
      q <<= 1;
      if (xr > yr) {
        xr -= yr;
        q += 1;
      }
    } else {
      // ex-ey = -1
      xr = std::ldexp(xr, ex - ey);
    }
  }

  if ((yr < T(2.0) * xr) || ((yr == T(2.0) * xr) && (q & 0x00000001))) {
    xr -= yr;
    q += 1;
  }

  if (ex - ey >= -1) {
    xr = std::ldexp(xr, ey);
  }

  // 7-bits of quotient
  quo_out = q & 0x0000007f;
  if (std::signbit(x) != std::signbit(y)) {
    quo_out = -quo_out;
  }

  if (x < T(0.0)) {
    xr = -xr;
  }

  return xr;
}
}  // namespace

#if defined(__arm__) || defined(_WIN32) || defined(__APPLE__)
// TODO This test has double precision reference results and we only pass when
// we can pretend they are extended precision reference results.
TEST_P(Execution, DISABLED_Precision_01_Pow_Func) {
#else
TEST_P(Execution, Precision_01_Pow_Func) {
#endif
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  const std::array<std::tuple<cl_long, cl_long, cl_long>, 18> payload{
      {std::make_tuple(0x3fe68916486fc659LL, 0x409d807d465cdba5LL,
                       0x043cd984ba25c315LL),
       std::make_tuple(0x3fe6965dce5db957LL, 0xc09ede2fd8959cd0LL,
                       0x7dfc4a45eca67233LL),
       std::make_tuple(0x3ff5c9415bcf0e2aLL, 0xc09c597f8214f74fLL,
                       0xd6fb7865f5a5847LL),
       std::make_tuple(0x3ff563f1817987e4LL, 0x40a07aa63df47685LL,
                       0x7727e603ab04a097LL),
       std::make_tuple(0x3ff571193ca843ceLL, 0x4098d60c97d05248LL,
                       0x69e432acd4602312LL),
       std::make_tuple(0x3ff5b395c8074f92LL, 0xc0946f3211d5f53aLL,
                       0x1bfe7f0ca3ccdcffLL),
       std::make_tuple(0x3ff5e124309ae9d4LL, 0xc09ef9c49b949a1cLL,
                       0x7fe377c6a2d7bf0LL),
       std::make_tuple(0x3ff5f0dfe9487bb0LL, 0xc09b2a94170d7029LL,
                       0x0e6f10480d8ad105LL),
       std::make_tuple(0x3ff67c98ff145da5LL, 0xc09e2b9d5d27f252LL,
                       0x04ae61170d1c68d8LL),
       std::make_tuple(0x29b4c1257162c100LL, 0x3fecb3f5c779c8baLL,
                       0x2c002e19c3d8bdcaLL),
       std::make_tuple(0x41feddc8713e7b83LL, 0x4001b79b3d833f11LL,
                       0x447f4db8dc6a93f0LL),
       std::make_tuple(0x165e7a512b9c3420LL, 0x3faa351886d030f0LL,
                       0x3dcf121d46673250LL),
       std::make_tuple(0x7b7f9e543deddcf0LL, 0xbfe2e7bf30a589c8LL,
                       0x1cbfbbac958b85e6LL),
       std::make_tuple(0x30d583b5be30b8b8LL, 0xbfffec47423ec5f8LL,
                       0x5e0f9c9ae75a7d77LL),
       std::make_tuple(0x4599cfff90d958bfLL, 0x4018a851782cb994LL,
                       0x62e087a183eba8dfLL),
       std::make_tuple(0xffefffffffffffffLL, 0x3ff0000000000000LL,
                       0xffefffffffffffffLL),
       std::make_tuple(0x3ff5e92baa52528aLL, 0x40a0d69c3f9fd885LL,
                       0x7d08025a7da98980LL),
       std::make_tuple(0x3ff5f6610ee72f73LL, 0x40a13746edc29098LL,
                       0x7edfaf6f1cef3dccLL)}};

  const size_t size = payload.size();

  AddInputBuffer(size, kts::Reference1D<cl_double>([&payload](size_t id) {
                   const cl_double input =
                       matchingType(std::get<0>(payload[id]));
                   return input;
                 }));
  AddInputBuffer(size, kts::Reference1D<cl_double>([&payload](size_t id) {
                   const cl_double input =
                       matchingType(std::get<1>(payload[id]));
                   return input;
                 }));
  AddOutputBuffer(size, makeULPStreamer<cl_double, 16_ULP>(
                            [&payload](size_t id) -> long double {
                              const cl_double output =
                                  matchingType(std::get<2>(payload[id]));
                              return static_cast<long double>(output);
                            },
                            device));

  RunGeneric1D(size);
}

// This test only works for Execution and OfflineExecution because of the nature
// of it's divergent paths. As such SPIR-V variants here are disabled and
// skipped.
TEST_P(ExecutionOpenCLC, Precision_17_Double_Constant) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  // This test relies on clang promoting floats to doubles when doubles are
  // available. If doubles are available, the calculation is more precise, and
  // we check for this precision. If doubles are disabled (with, e.g.,
  // -cl-single-precision-constant), then floats will not be promoted even when
  // doubles are available, and the test will fail.
  float expected = FLT_MIN;
  if (!UCL::hasDoubleSupport(this->device)) {
    expected = 0.0f;
  }

  AddOutputBuffer(
      1, kts::Reference1D<cl_float>([&expected](size_t) { return expected; }));

  RunGeneric1D(1);
}

using DenormalsTest = ExecutionWithParam<bool>;
TEST_P(DenormalsTest, Precision_02_Denorms) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection causes validation failure.
  }
  const bool denorms_may_be_zero = getParam();
  if (denorms_may_be_zero) {
    // Performance hint that denormalized numbers may be flushed to zero
    AddBuildOption("-cl-denorms-are-zero");
  }

  // Kernel multiplies the first two values together, and expects the third as
  // the result
  const std::array<std::tuple<cl_int, cl_int, cl_int>, 5> payload{
      {std::make_tuple(0x00400000, 0x3f000000, 0x00200000),
       std::make_tuple(0x00400000, 0x3e000000, 0x00080000),
       std::make_tuple(0x00400000, 0x3e99999a, 0x00133333),
       std::make_tuple(0x00001803, 0x3f000000, 0x00000c02),
       std::make_tuple(0x00180000, 0x4d040401, 0x0cc60602)}};

  const size_t size = payload.size();

  // First value is a denormal float value
  AddInputBuffer(size, kts::Reference1D<cl_float>([&payload](size_t id) {
                   return matchingType(std::get<0>(payload[id]));
                 }));

  // Second value is a normal float value
  auto normal_input = [&payload](size_t id) {
    return matchingType(std::get<1>(payload[id]));
  };
  AddInputBuffer(size, kts::Reference1D<cl_float>(normal_input));

  // Third value is reference result
  auto ref_lambda = [&payload](size_t id) -> cl_double {
    const cl_float output = matchingType(std::get<2>(payload[id]));
    return static_cast<cl_double>(output);
  };

  // Device may not support denormals regardless of `-cl-denorm-are-zero` flag
  const bool device_denorm_support = UCL::hasDenormSupport(
      ucl::Environment::instance->GetDevice(), CL_DEVICE_SINGLE_FP_CONFIG);

  const cl_ulong ULP = 1;  // For rounding differences
  if (denorms_may_be_zero || !device_denorm_support) {
    // Flush To Zero results if input value is a denormal, according to spec
    // section 7.5.3 the sign of zero is not defined
    auto ftz_positive = [](size_t) -> cl_double { return 0.0; };
    auto ftz_negative = [](size_t) -> cl_double { return -0.0; };

    // If denormals are treated as zero input then returning the normal
    // operand is also a valid result
    const std::vector<kts::Reference1D<cl_double>> fallbacks{
        normal_input, ftz_positive, ftz_negative};
    auto FTZStreamer = makeULPStreamer<cl_float, ULP>(
        ref_lambda, std::move(fallbacks), device);
    AddOutputBuffer(size, FTZStreamer);
  } else {
    AddOutputBuffer(size, makeULPStreamer<cl_float, ULP>(ref_lambda, device));
  }
  RunGeneric1D(size);
}

UCL_EXECUTION_TEST_SUITE_P(DenormalsTest, testing::Values(OPENCL_C),
                           testing::Values(true, false))

TEST_P(ExecutionSPIRV, Precision_02_Denorms) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  // Kernel multiplies the first two values together, and expects the third as
  // the result.
  const std::array<std::tuple<cl_int, cl_int, cl_int>, 5> payload{
      {std::make_tuple(0x00400000, 0x3f000000, 0x00200000),
       std::make_tuple(0x00400000, 0x3e000000, 0x00080000),
       std::make_tuple(0x00400000, 0x3e99999a, 0x00133333),
       std::make_tuple(0x00001803, 0x3f000000, 0x00000c02),
       std::make_tuple(0x00180000, 0x4d040401, 0x0cc60602)}};

  const size_t size = payload.size();

  // First value is a denormal float value
  AddInputBuffer(size, kts::Reference1D<cl_float>([&payload](size_t id) {
                   return matchingType(std::get<0>(payload[id]));
                 }));

  // Second value is a normal float value
  const auto normal_input = [&payload](size_t id) {
    return matchingType(std::get<1>(payload[id]));
  };
  AddInputBuffer(size, kts::Reference1D<cl_float>(normal_input));

  // Third value is reference result
  const auto ref_lambda = [&payload](size_t id) -> cl_double {
    const cl_float output = matchingType(std::get<2>(payload[id]));
    return static_cast<cl_double>(output);
  };

  // Flush To Zero results if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto ftz_positive = [](size_t) -> cl_double { return 0.0; };
  const auto ftz_negative = [](size_t) -> cl_double { return -0.0; };

  // If denormals are treated as zero input then returning the normal
  // operand is also a valid result
  const std::vector<kts::Reference1D<cl_double>> fallbacks{
      normal_input, ftz_positive, ftz_negative};
  auto FTZStreamer = makeULPStreamer<cl_float, 1_ULP>(
      ref_lambda, std::move(fallbacks), device);
  AddOutputBuffer(size, FTZStreamer);

  RunGeneric1D(size);
}

using HalfOperatorTest = HalfParamExecution;
TEST_P(HalfOperatorTest, Precision_03_Half_Add) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  // TODO: CA-2882: Vector width 3 doesn't work.
#ifdef __arm__
  if (getParam() == 3) {
    GTEST_SKIP();
  }
#endif

  auto add_ref = [](cl_float a, cl_float b) -> cl_float { return a + b; };

  TestAgainstRef<0_ULP>(add_ref);
}

TEST_P(HalfOperatorTest, Precision_04_Half_Sub) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  // TODO: CA-2882: Vector width 3 doesn't work.
#ifdef __arm__
  if (getParam() == 3) {
    GTEST_SKIP();
  }
#endif

  auto sub_ref = [](cl_float a, cl_float b) -> cl_float { return a - b; };

  TestAgainstRef<0_ULP>(sub_ref);
}

TEST_P(HalfOperatorTest, Precision_05_Half_Mul) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  // TODO: CA-2882: Vector width 3 doesn't work.
#ifdef __arm__
  if (getParam() == 3) {
    GTEST_SKIP();
  }
#endif

  auto mul_ref = [](cl_float a, cl_float b) -> cl_float { return a * b; };

  TestAgainstRef<0_ULP>(mul_ref);
}

TEST_P(HalfOperatorTest, Precision_06_Half_Div) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  // TODO: CA-2882: Vector width 3 doesn't work.
#ifdef __arm__
  if (getParam() == 3) {
    GTEST_SKIP();
  }
#endif

  auto div_ref = [](cl_float a, cl_float b) -> cl_float { return a / b; };

  TestAgainstRef<0_ULP>(div_ref);
}

TEST_P(HalfOperatorTest, Precision_07_Half_Recip) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  // TODO: CA-2882: Vector width 3 doesn't work.
#ifdef __arm__
  if (getParam() == 3) {
    GTEST_SKIP();
  }
#endif

  auto recip_ref = [](cl_float x) -> cl_float { return 1.0f / x; };

  TestAgainstRef<0_ULP>(recip_ref);
}

UCL_EXECUTION_TEST_SUITE_P(HalfOperatorTest, testing::Values(OPENCL_C),
                           testing::Values(1, 2, 3, 4, 8, 16))

using HalfMathBuiltins = HalfParamExecution;

struct HalfMathBuiltinsPow : HalfMathBuiltins {
  const std::vector<cl_ushort> &GetEdgeCases() const override {
    static const std::vector<cl_ushort> EdgeCases = [&] {
      std::vector<cl_ushort> EdgeCases = HalfMathBuiltins::GetEdgeCases();
      // 0x39f6 is singled out as a special case in log2_extended_precision.
      EdgeCases.push_back(0x39f6);
      // pow(0x39f0, 0xd00e) is just one example where evaluating
      // horner_polynomial without FMA gives results with insufficient
      // precision.
      EdgeCases.push_back(0x39f0);
      EdgeCases.push_back(0xd00e);
      return EdgeCases;
    }();
    return EdgeCases;
  }
};

TEST_P(HalfMathBuiltins, Precision_08_Half_Ldexp) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto ldexp_ref = [](cl_float x, cl_int n) -> cl_float {
    return std::ldexp(x, n);
  };

  TestAgainstIntArgRef<0_ULP>(ldexp_ref);
}

TEST_P(HalfMathBuiltins, Precision_09_Half_Exp10) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto exp10_ref = [](cl_float x) -> cl_float { return std::pow(10.0f, x); };

  TestAgainstRef<2_ULP>(exp10_ref);
}

TEST_P(HalfMathBuiltins, Precision_10_Half_Exp) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto exp_ref = [](cl_float x) -> cl_float { return std::exp(x); };

  TestAgainstRef<2_ULP>(exp_ref);
}

TEST_P(HalfMathBuiltins, Precision_11_Half_Exp2) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto exp2_ref = [](cl_float x) -> cl_float { return std::exp2(x); };

  TestAgainstRef<2_ULP>(exp2_ref);
}

TEST_P(HalfMathBuiltins, Precision_12_Half_Expm1) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto expm1_ref = [](cl_float x) -> cl_float { return std::expm1(x); };

  TestAgainstRef<2_ULP>(expm1_ref);
}

TEST_P(HalfMathBuiltins, Precision_13_Half_Fabs) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto fabs_ref = [](cl_float x) -> cl_float { return std::fabs(x); };

  TestAgainstRef<0_ULP>(fabs_ref);
}

TEST_P(HalfMathBuiltins, Precision_14_Half_Copysign) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto copysign_ref = [](cl_float x, cl_float y) -> cl_float {
    return std::copysign(x, y);
  };

  TestAgainstRef<0_ULP>(copysign_ref);
}

TEST_P(HalfMathBuiltins, Precision_15_Half_Floor) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto floor_ref = [](cl_float x) -> cl_float { return std::floor(x); };

  TestAgainstRef<0_ULP>(floor_ref);
}

TEST_P(HalfMathBuiltins, Precision_16_Half_Ceil) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto ceil_ref = [](cl_float x) -> cl_float { return std::ceil(x); };

  TestAgainstRef<0_ULP>(ceil_ref);
}

TEST_P(HalfMathBuiltins, Precision_17_Half_sqrt) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto sqrt_ref = [](cl_float x) -> cl_float { return std::sqrt(x); };

  TestAgainstRef<0_ULP>(sqrt_ref);
}

TEST_P(HalfMathBuiltins, Precision_18_Half_frexp) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto frexp_ref = [](cl_float x, cl_int &exp_out) -> cl_float {
    return std::frexp(x, &exp_out);
  };

  TestAgainstIntReferenceArgRef<2_ULP>(frexp_ref);
}

TEST_P(HalfMathBuiltins, Precision_18_Half_frexp_local) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto frexp_ref = [](cl_float x, cl_int &exp_out) -> cl_float {
    return std::frexp(x, &exp_out);
  };

  TestAgainstIntReferenceArgRef<2_ULP>(frexp_ref);
}

TEST_P(HalfMathBuiltins, Precision_18_Half_frexp_private) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto frexp_ref = [](cl_float x, cl_int &exp_out) -> cl_float {
    return std::frexp(x, &exp_out);
  };

  TestAgainstIntReferenceArgRef<2_ULP>(frexp_ref);
}

TEST_P(HalfMathBuiltins, Precision_19_Half_rsqrt) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto rsqrt_ref = [](cl_float x) -> cl_float { return 1.0f / std::sqrt(x); };

  TestAgainstRef<1_ULP>(rsqrt_ref);
}

TEST_P(HalfMathBuiltins, Precision_20_Half_Sinpi) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto sinpi_ref = [](cl_float x) -> cl_float { return std::sin(M_PI * x); };

  TestAgainstRef<2_ULP>(sinpi_ref);
}

TEST_P(HalfMathBuiltins, Precision_21_Half_Cospi) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto cospi_ref = [](cl_float x) -> cl_float { return std::cos(M_PI * x); };

  TestAgainstRef<2_ULP>(cospi_ref);
}

TEST_P(HalfMathBuiltins, Precision_22_Half_ilogb) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  // If x is NAN we must return INT_MAX as that is what abacus returns for nan.
  // The C++ spec allows for *either* INT_MIN *or* INT_MAX via the signaling
  // value FP_ILOGBNAN. This makes testing on different platforms problematic so
  // we just pick one to test against as either are legal results. For 0.0f and
  // -0.0f we have the same issue but for INT_MIN, so we return that.
  auto ilogb_ref = [](cl_float x) -> cl_int {
    if (std::isnan(x)) {
      return INT_MAX;
    }
    if (x == 0.0f) {
      return INT_MIN;
    }
    return std::ilogb(x);
  };
  TestAgainstIntReturn(ilogb_ref);
}

TEST_P(HalfMathBuiltins, Precision_23_Half_log2) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto log2_ref = [](cl_float x) -> cl_float { return std::log2(x); };

  TestAgainstRef<2_ULP>(log2_ref);
}

TEST_P(HalfMathBuiltins, Precision_24_Half_log10) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto log10_ref = [](cl_float x) -> cl_float { return std::log10(x); };

  TestAgainstRef<2_ULP>(log10_ref);
}

TEST_P(HalfMathBuiltins, Precision_25_Half_log) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto log_ref = [](cl_float x) -> cl_float { return std::log(x); };

  TestAgainstRef<2_ULP>(log_ref);
}

TEST_P(HalfMathBuiltins, Precision_26_Half_fmax) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto fmax_ref = [](cl_float x, cl_float y) -> cl_float {
    // Work around issue on 64-bit arm std::fmaxf by casting to double
    return std::fmax((double)x, (double)y);
  };

  TestAgainstRef<0_ULP>(fmax_ref);
}

TEST_P(HalfMathBuiltins, Precision_27_Half_fmin) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto fmin_ref = [](cl_float x, cl_float y) -> cl_float {
    // Work around issue on 64-bit arm std::fminf by casting to double
    return std::fmin((double)x, (double)y);
  };

  TestAgainstRef<0_ULP>(fmin_ref);
}

TEST_P(HalfMathBuiltins, Precision_28_Half_Maxmag) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto maxmag_ref = [](cl_float a, cl_float b) -> cl_float {
    const cl_float mag_a = std::fabs(a);
    const cl_float mag_b = std::fabs(b);
    if (mag_a > mag_b) {
      return a;
    } else if (mag_a < mag_b) {
      return b;
    }
    // Work around issue on 64-bit arm std::fmaxf by casting to double
    return std::fmax((double)a, (double)b);
  };

  TestAgainstRef<0_ULP>(maxmag_ref);
}

TEST_P(HalfMathBuiltins, Precision_29_Half_Minmag) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto minmag_ref = [](cl_float a, cl_float b) -> cl_float {
    const cl_float mag_a = std::fabs(a);
    const cl_float mag_b = std::fabs(b);
    if (mag_a < mag_b) {
      return a;
    } else if (mag_a > mag_b) {
      return b;
    }
    // Work around issue on 64-bit arm std::fminf by casting to double
    return std::fmin((double)a, (double)b);
  };

  TestAgainstRef<0_ULP>(minmag_ref);
}

TEST_P(HalfMathBuiltins, Precision_30_Half_Trunc) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto trunc_ref = [](cl_float x) -> cl_float { return std::trunc(x); };

  TestAgainstRef<0_ULP>(trunc_ref);
}

TEST_P(HalfMathBuiltins, Precision_31_Half_Nan) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }
  const unsigned vec_width = getParam();

  auto env = ucl::Environment::instance;
  const ucl::MathMode math_mode = env->math_mode;
  const size_t N = HalfInputSizes::getInputSize(math_mode);

  std::string float_type_name("half");
  std::string int_type_name("ushort");
  if (vec_width != 1) {
    float_type_name.append(std::to_string(vec_width));
    int_type_name.append(std::to_string(vec_width));
  }
  AddMacro("FLOAT_TYPE", float_type_name);
  AddMacro("INT_TYPE", int_type_name);

  std::vector<cl_ushort> inputShorts(N);
  env->GetInputGenerator().GenerateIntData(inputShorts);

  kts::Reference1D<cl_half> refShorts = [&inputShorts](size_t id) {
    return inputShorts[id];
  };
  this->AddInputBuffer(N, refShorts);

  // We only need to verify that the result is NaN. Asserting that the nancode
  // from the input is present is optional. From the spec:
  // "Returns a quiet NaN. The nancode **may** be placed in the significand of
  // the resulting NaN."
  kts::Reference1D<cl_float> nopRef = [](size_t) { return 0.0f; };

  this->AddOutputBuffer(
      N,
      std::make_shared<kts::GenericStreamer<cl_half, NaNValidator, cl_float>>(
          nopRef));
  this->RunGeneric1D(N / vec_width);
}

TEST_P(HalfMathBuiltins, Precision_32_Half_Mad) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  // According to the OpenCL 1.2 spec `mad()` has infinite ULP, however later
  // specs(2.2) define the ULP for Full profile as a correctly rounded `fma()`,
  // or multiply followed by an add, both of which are correctly rounded. We
  // verify against the second option here.
  auto mad_ref = [](cl_float a, cl_float b, cl_float c) -> cl_float {
    const cl_float mul = a * b;
    const cl_half asHalf = ConvertFloatToHalf(mul);
    const cl_float asFloat = ConvertHalfToFloat(asHalf);
    const cl_float add = asFloat + c;
    return add;
  };

  // How subnormal intermediate products are handled is not defined for mad()
  const std::function<bool(cl_float, cl_float, cl_float)> undef_check(
      [](cl_float a, cl_float b, cl_float) {
        const cl_float mul = a * b;
        return IsDenormalAsHalf(mul);
      });

  TestAgainstRef<0_ULP>(mad_ref, &undef_check);
}

TEST_P(HalfMathBuiltins, Precision_33_Half_fmod) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto fmod_ref = [](cl_float x, cl_float y) -> cl_float {
    return std::fmod(x, y);
  };

  TestAgainstRef<0_ULP>(fmod_ref);
}

TEST_P(HalfMathBuiltins, Precision_34_Half_round) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto round_ref = [](cl_float x) -> cl_float { return std::round(x); };

  TestAgainstRef<0_ULP>(round_ref);
}

TEST_P(HalfMathBuiltins, Precision_35_Half_rint) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto rint_ref = [](cl_float x) -> cl_float { return std::rint(x); };

  TestAgainstRef<0_ULP>(rint_ref);
}

TEST_P(HalfMathBuiltins, Precision_36_Half_remainder) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto remainder_ref = [](cl_float x, cl_float y) -> cl_float {
    return std::remainder(x, y);
  };

  TestAgainstRef<0_ULP>(remainder_ref);
}

TEST_P(HalfMathBuiltins, Precision_37_Half_remquo) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  TestAgainstIntReferenceArgRef<0_ULP>(Remquo7BitRef<cl_float>);
}

TEST_P(HalfMathBuiltins, Precision_37_Half_remquo_local) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  TestAgainstIntReferenceArgRef<0_ULP>(Remquo7BitRef<cl_float>);
}

TEST_P(HalfMathBuiltins, Precision_37_Half_remquo_private) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  TestAgainstIntReferenceArgRef<0_ULP>(Remquo7BitRef<cl_float>);
}

TEST_P(HalfMathBuiltins, Precision_38_Half_fdim) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto fdim_ref = [](cl_float x, cl_float y) -> cl_float {
    return std::fdim(x, y);
  };

  TestAgainstRef<0_ULP>(fdim_ref);
}

TEST_P(HalfMathBuiltins, Precision_39_Half_fract) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto fract_ref = [](cl_float x, cl_float &out) -> cl_float {
    // NaN and Infinity behaviour taken from `std::modf` which the CTS uses as
    // part of it's reference function, since `fract` isn't in the standard lib
    if (std::isnan(x)) {
      out = x;
      return x;
    } else if (std::isinf(x)) {
      out = x;
      return std::copysign(0.0f, x);
    }

    const cl_float floor = std::floor(x);
    out = floor;

    // 0x1.ffcp-1f literal from spec doesn't compile on MSVC, so use 0.999512f
    return std::fmin((x - floor), 0.999512f);
  };

  TestAgainstFloatReferenceArgRef<0_ULP>(fract_ref);
}

TEST_P(HalfMathBuiltins, Precision_39_Half_fract_local) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto fract_ref = [](cl_float x, cl_float &out) -> cl_float {
    // NaN and Infinity behaviour taken from `std::modf` which the CTS uses as
    // part of it's reference function, since `fract` isn't in the standard lib
    if (std::isnan(x)) {
      out = x;
      return x;
    } else if (std::isinf(x)) {
      out = x;
      return std::copysign(0.0f, x);
    }

    const cl_float floor = std::floor(x);
    out = floor;

    // 0x1.ffcp-1f literal from spec doesn't compile on MSVC, so use 0.999512f
    return std::fmin((x - floor), 0.999512f);
  };

  TestAgainstFloatReferenceArgRef<0_ULP>(fract_ref);
}

TEST_P(HalfMathBuiltins, Precision_39_Half_fract_private) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto fract_ref = [](cl_float x, cl_float &out) -> cl_float {
    // NaN and Infinity behaviour taken from `std::modf` which the CTS uses as
    // part of it's reference function, since `fract` isn't in the standard lib
    if (std::isnan(x)) {
      out = x;
      return x;
    } else if (std::isinf(x)) {
      out = x;
      return std::copysign(0.0f, x);
    }

    const cl_float floor = std::floor(x);
    out = floor;

    // 0x1.ffcp-1f literal from spec doesn't compile on MSVC, so use 0.999512f
    return std::fmin((x - floor), 0.999512f);
  };

  TestAgainstFloatReferenceArgRef<0_ULP>(fract_ref);
}

TEST_P(HalfMathBuiltins, Precision_40_Half_logb) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto logb_ref = [](cl_float x) -> cl_float { return std::logb(x); };

  TestAgainstRef<0_ULP>(logb_ref);
}

TEST_P(HalfMathBuiltins, Precision_41_Half_modf) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto modf_ref = [](cl_float x, cl_float &out) -> cl_float {
    return std::modf(x, &out);
  };

  TestAgainstFloatReferenceArgRef<0_ULP>(modf_ref);
}

TEST_P(HalfMathBuiltins, Precision_41_Half_modf_local) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto modf_ref = [](cl_float x, cl_float &out) -> cl_float {
    return std::modf(x, &out);
  };

  TestAgainstFloatReferenceArgRef<0_ULP>(modf_ref);
}

TEST_P(HalfMathBuiltins, Precision_41_Half_modf_private) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto modf_ref = [](cl_float x, cl_float &out) -> cl_float {
    return std::modf(x, &out);
  };

  TestAgainstFloatReferenceArgRef<0_ULP>(modf_ref);
}

TEST_P(HalfMathBuiltins, Precision_42_Half_nextafter) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  // Based on 1.2 CTS nextafter reference
  auto nextafter_ref = [](cl_float x, cl_float y) -> cl_float {
    if (std::isnan(x)) {
      return x;
    } else if (std::isnan(y)) {
      return y;
    } else if (x == y) {
      return x;
    }

    const cl_half xHalf = ConvertFloatToHalf(x);
    const cl_half yHalf = ConvertFloatToHalf(y);

    cl_short xShort = cargo::bit_cast<cl_short>(xHalf);
    cl_short yShort = cargo::bit_cast<cl_short>(yHalf);

    const cl_ushort signBit = 0x8000;
    if (xShort & signBit) {
      xShort = signBit - xShort;
    }

    if (yShort & signBit) {
      yShort = signBit - yShort;
    }

    xShort += xShort < yShort ? 1 : -1;
    xShort = (xShort < 0) ? signBit - xShort : xShort;

    const cl_half result = cargo::bit_cast<cl_half>(xShort);
    return ConvertHalfToFloat(result);
  };

  TestAgainstRef<0_ULP>(nextafter_ref);
}

TEST_P(HalfMathBuiltins, Precision_43_Half_fma) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  // fma() returns the correctly rounded floating point representation of the
  // sum of c with the infinitely precise product of a and b. Rounding of
  // intermediate products shall not occur.
  //
  // Because the intermediate value in our half implementation should be
  // infinitely precise, 32-bit float does not have enough mantissa bits for a
  // reference, so we use a 64-bit double reference instead
  auto fma_ref = [](cl_float a, cl_float b, cl_float c) -> cl_float {
    // Double precision reference
    const double ref_double = std::fma(double(a), double(b), double(c));
    const cl_half as_half = ConvertFloatToHalf(ref_double);

    // Our validation API expects a 32-bit float, so upcast half to float
    const cl_float as_float = ConvertHalfToFloat(as_half);
    return as_float;
  };

  TestAgainstRef<0_ULP>(fma_ref);
}

TEST_P(HalfMathBuiltins, Precision_44_Half_log1p) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto log1p_ref = [](cl_float x) -> cl_float { return std::log1p(x); };

  TestAgainstRef<2_ULP>(log1p_ref);
}

TEST_P(HalfMathBuiltins, Precision_45_Half_cbrt) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto cbrt_ref = [](cl_float x) -> cl_float { return std::cbrt(x); };

  TestAgainstRef<2_ULP>(cbrt_ref);
}

TEST_P(HalfMathBuiltins, Precision_46_Half_hypot) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto hypot_ref = [](cl_float x, cl_float y) -> cl_float {
    if (std::isinf(x) || std::isinf(y)) {
      // Infinity check from the CTS `reference_hypot`, resulting in the value
      // of infinities overwriting NaNs for `hypot(INF, NAN)`
      return INFINITY;
    }

    return std::hypot(x, y);
  };

  TestAgainstRef<2_ULP>(hypot_ref);
}

TEST_P(HalfMathBuiltins, Precision_47_Half_max) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto max_ref = [](cl_float x, cl_float y) -> cl_float {
    return std::max(x, y);
  };

  const std::function<bool(cl_float, cl_float)> undef_check(
      [](cl_float x, cl_float y) {
        // Unlike `fmax()`, `max()` has undefined behaviour if either input is
        // NaN, our abacus implementation always returns `y` in this case.
        return std::isnan(x) || std::isnan(y);
      });

  TestAgainstRef<0_ULP>(max_ref, &undef_check);
}

TEST_P(HalfMathBuiltins, Precision_47_Half_max_scalar) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto max_ref = [](cl_float x, cl_float y) -> cl_float {
    return std::max(x, y);
  };

  const std::function<bool(cl_float, cl_float)> undef_check(
      [](cl_float x, cl_float y) {
        // Unlike `fmax()`, `max()` has undefined behaviour if either input is
        // NaN, our abacus implementation always returns `y` in this case.
        return std::isnan(x) || std::isnan(y);
      });

  // Test `max()` signature where second input is scalar half
  InitScalarArgIndices({1});

  TestAgainstRef<0_ULP>(max_ref, &undef_check);
}

TEST_P(HalfMathBuiltins, Precision_48_Half_min) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto min_ref = [](cl_float x, cl_float y) -> cl_float {
    return std::min(x, y);
  };

  const std::function<bool(cl_float, cl_float)> undef_check(
      [](cl_float x, cl_float y) {
        // Unlike `fmin()`, `min()` has undefined behaviour if either input is
        // NaN Our abacus implementation always returns `y` in this case.
        return std::isnan(x) || std::isnan(y);
      });

  TestAgainstRef<0_ULP>(min_ref, &undef_check);
}

TEST_P(HalfMathBuiltins, Precision_48_Half_min_scalar) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto min_ref = [](cl_float x, cl_float y) -> cl_float {
    return std::min(x, y);
  };

  const std::function<bool(cl_float, cl_float)> undef_check(
      [](cl_float x, cl_float y) {
        // Unlike `fmin()`, `min()` has undefined behaviour if either input is
        // NaN Our abacus implementation always returns `y` in this case.
        return std::isnan(x) || std::isnan(y);
      });

  // Test `min()` signature where second input is scalar half
  InitScalarArgIndices({1});

  TestAgainstRef<0_ULP>(min_ref, &undef_check);
}

TEST_P(HalfMathBuiltins, Precision_49_Half_sign) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto sign_ref = [](cl_float x) -> cl_float {
    if (std::isnan(x)) {
      return 0.0f;
    }

    const cl_float result = x == 0.0f ? 0.0f : 1.0f;
    return std::copysign(result, x);
  };

  TestAgainstRef<0_ULP>(sign_ref);
}

TEST_P(HalfMathBuiltins, Precision_50_Half_degrees) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto degrees_ref = [](cl_float radians) -> cl_float {
    return (180.0f / M_PI) * radians;
  };

  // Defined as 2 ULP in https://github.com/KhronosGroup/OpenCL-Docs/pull/44
  TestAgainstRef<2_ULP>(degrees_ref);
}

TEST_P(HalfMathBuiltins, Precision_51_Half_radians) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto radians_ref = [](cl_float degrees) -> cl_float {
    return (M_PI / 180.0f) * degrees;
  };

  // Defined as 2 ULP in https://github.com/KhronosGroup/OpenCL-Docs/pull/44
  TestAgainstRef<2_ULP>(radians_ref);
}

TEST_P(HalfMathBuiltins, Precision_52_Half_clamp) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto clamp_ref = [](cl_float x, cl_float min, cl_float max) -> cl_float {
    return std::min(std::max(x, min), max);
  };

  const std::function<bool(cl_float, cl_float, cl_float)> undef_check(
      [](cl_float x, cl_float min, cl_float max) {
        return std::isnan(x) || std::isnan(min) || std::isnan(max) || min > max;
      });

  TestAgainstRef<0_ULP>(clamp_ref, &undef_check);
}

TEST_P(HalfMathBuiltins, Precision_52_Half_clamp_scalar) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto clamp_ref = [](cl_float x, cl_float min, cl_float max) -> cl_float {
    return std::min(std::max(x, min), max);
  };

  const std::function<bool(cl_float, cl_float, cl_float)> undef_check(
      [](cl_float x, cl_float min, cl_float max) {
        return std::isnan(x) || std::isnan(min) || std::isnan(max) || min > max;
      });

  // Test `clamp()` signature where min and max inputs are scalar half
  InitScalarArgIndices({1, 2});

  TestAgainstRef<0_ULP>(clamp_ref, &undef_check);
}

TEST_P(HalfMathBuiltins, Precision_53_Half_mix) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto mix_ref = [](cl_float x, cl_float y, cl_float a) -> cl_float {
    cl_float sub = y - x;

    // Check for overflow and underflow of intermediate
    const cl_half sub_as_half = ConvertFloatToHalf(sub);
    if (IsInf(sub_as_half)) {
      sub = std::copysign(INFINITY, sub);
    } else if (0 == (sub_as_half & ~TypeInfo<cl_half>::sign_bit)) {
      sub = std::copysign(0.0f, sub);
    }

    return x + (sub * a);
  };

  const std::function<bool(cl_float, cl_float, cl_float)> undef_check(
      [](cl_float, cl_float, cl_float a) { return a < 0.0f || a > 1.0f; });

  // mix() has Implementation-defined ULP for half
  TestAgainstRef<MAX_ULP_ERROR>(mix_ref, &undef_check);
}

TEST_P(HalfMathBuiltins, Precision_53_Half_mix_scalar) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto mix_ref = [](cl_float x, cl_float y, cl_float a) -> cl_float {
    cl_float sub = y - x;

    // Check for overflow and underflow of intermediate
    const cl_half sub_as_half = ConvertFloatToHalf(sub);
    if (IsInf(sub_as_half)) {
      sub = std::copysign(INFINITY, sub);
    } else if (0 == (sub_as_half & ~TypeInfo<cl_half>::sign_bit)) {
      sub = std::copysign(0.0f, sub);
    }

    return x + (sub * a);
  };

  const std::function<bool(cl_float, cl_float, cl_float)> undef_check(
      [](cl_float, cl_float, cl_float a) { return a < 0.0f || a > 1.0f; });

  // Test `mix()` signature where last input is a scalar half
  InitScalarArgIndices({2});

  // mix() has Implementation-defined ULP for half
  TestAgainstRef<MAX_ULP_ERROR>(mix_ref, &undef_check);
}

TEST_P(HalfMathBuiltins, Precision_54_Half_step) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto step_ref = [](cl_float edge, cl_float x) -> cl_float {
    return x < edge ? 0.0f : 1.0f;
  };

  const std::function<bool(cl_float, cl_float)> undef_check(
      [](cl_float edge, cl_float x) {
        return std::isnan(edge) || std::isnan(x);
      });

  TestAgainstRef<0_ULP>(step_ref, &undef_check);
}

TEST_P(HalfMathBuiltins, Precision_54_Half_step_scalar) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto step_ref = [](cl_float edge, cl_float x) -> cl_float {
    return x < edge ? 0.0f : 1.0f;
  };

  const std::function<bool(cl_float, cl_float)> undef_check(
      [](cl_float edge, cl_float x) {
        return std::isnan(edge) || std::isnan(x);
      });

  // Test `step()` signature where first input is scalar half
  InitScalarArgIndices({0});

  TestAgainstRef<0_ULP>(step_ref, &undef_check);
}

TEST_P(HalfMathBuiltins, Precision_55_Half_smoothstep) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto smoothstep_ref = [](cl_float edge0, cl_float edge1,
                           cl_float x) -> cl_float {
    const cl_float t = (x - edge0) / (edge1 - edge0);
    if (t > 1.0f) {
      return 1.0f;
    }

    if (t < 0.0f) {
      return 0.0f;
    }

    if (std::isnan(t)) {
      // `clamp(NAN, 0.0, 1.0)` is defined as 0.0
      // see https://github.com/KhronosGroup/OpenCL-Docs/issues/49
      return 0.0f;
    }

    return t * t * (3.0f - (2.0f * t));
  };

  const std::function<bool(cl_float, cl_float, cl_float)> undef_check(
      [](cl_float edge0, cl_float edge1, cl_float x) {
        return std::isnan(edge0) || std::isnan(edge1) || std::isnan(x) ||
               edge0 >= edge1;
      });

  // smoothstep() has Implementation-defined ULP for half
  TestAgainstRef<MAX_ULP_ERROR>(smoothstep_ref, &undef_check);
}

TEST_P(HalfMathBuiltins, Precision_55_Half_smoothstep_scalar) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto smoothstep_ref = [](cl_float edge0, cl_float edge1,
                           cl_float x) -> cl_float {
    const cl_float t = (x - edge0) / (edge1 - edge0);

    if (t > 1.0f) {
      return 1.0f;
    }

    if (t < 0.0f) {
      return 0.0f;
    }

    if (std::isnan(t)) {
      // `clamp(NAN, 0.0, 1.0)` is defined as 0.0
      // see https://github.com/KhronosGroup/OpenCL-Docs/issues/49
      return 0.0f;
    }

    return t * t * (3.0f - (2.0f * t));
  };

  const std::function<bool(cl_float, cl_float, cl_float)> undef_check(
      [](cl_float edge0, cl_float edge1, cl_float x) {
        return std::isnan(edge0) || std::isnan(edge1) || std::isnan(x) ||
               edge0 >= edge1;
      });

  // Test `smoothstep()` signature where first two edge inputs are scalar half
  InitScalarArgIndices({0, 1});

  // smoothstep() has Implementation-defined ULP for half
  TestAgainstRef<MAX_ULP_ERROR>(smoothstep_ref, &undef_check);
}

TEST_P(HalfMathBuiltins, Precision_56_Half_asin) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto asin_ref = [](cl_float x) -> cl_float { return std::asin(x); };

  TestAgainstRef<2_ULP>(asin_ref);
}

TEST_P(HalfMathBuiltins, Precision_57_Half_acos) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto acos_ref = [](cl_float x) -> cl_float { return std::acos(x); };

  TestAgainstRef<2_ULP>(acos_ref);
}

TEST_P(HalfMathBuiltins, Precision_58_Half_atan) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto atan_ref = [](cl_float x) -> cl_float { return std::atan(x); };

  TestAgainstRef<2_ULP>(atan_ref);
}

TEST_P(HalfMathBuiltins, Precision_59_Half_sin) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto sin_ref = [](cl_float x) -> cl_float { return std::sin(x); };

  TestAgainstRef<2_ULP>(sin_ref);
}

TEST_P(HalfMathBuiltins, Precision_60_Half_cos) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto cos_ref = [](cl_float x) -> cl_float { return std::cos(x); };

  TestAgainstRef<2_ULP>(cos_ref);
}

TEST_P(HalfMathBuiltins, Precision_61_Half_tan) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto tan_ref = [](cl_float x) -> cl_float { return std::tan(x); };

  TestAgainstRef<2_ULP>(tan_ref);
}

TEST_P(HalfMathBuiltins, Precision_62_Half_asinh) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto asinh_ref = [](cl_float x) -> cl_float { return std::asinh(x); };

  TestAgainstRef<2_ULP>(asinh_ref);
}

TEST_P(HalfMathBuiltins, Precision_63_Half_acosh) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto acosh_ref = [](cl_float x) -> cl_float { return std::acosh(x); };

  TestAgainstRef<2_ULP>(acosh_ref);
}

TEST_P(HalfMathBuiltins, Precision_64_Half_atanh) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto atanh_ref = [](cl_float x) -> cl_float { return std::atanh(x); };

  TestAgainstRef<2_ULP>(atanh_ref);
}

TEST_P(HalfMathBuiltins, Precision_65_Half_asinpi) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto asinpi_ref = [](cl_float x) -> cl_float {
    return std::asin(x) * M_1_PI;
  };

  TestAgainstRef<2_ULP>(asinpi_ref);
}

TEST_P(HalfMathBuiltins, Precision_66_Half_acospi) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto acospi_ref = [](cl_float x) -> cl_float {
    return std::acos(x) * M_1_PI;
  };

  TestAgainstRef<2_ULP>(acospi_ref);
}

TEST_P(HalfMathBuiltins, Precision_67_Half_atanpi) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto atanpi_ref = [](cl_float x) -> cl_float {
    return std::atan(x) * M_1_PI;
  };

  TestAgainstRef<2_ULP>(atanpi_ref);
}

TEST_P(HalfMathBuiltins, Precision_68_Half_atan2) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto atan2_ref = [](cl_float x, cl_float y) -> cl_float {
    return std::atan2(x, y);
  };

  const bool denorm_support =
      UCL::hasDenormSupport(device, CL_DEVICE_HALF_FP_CONFIG);
  const std::function<bool(cl_float, cl_float)> undef_check(
      [denorm_support](cl_float x, cl_float y) {
        // atan2(0.0, 0.0) is undefined
        if (!denorm_support) {
          return IsDenormalAsHalf(x) && IsDenormalAsHalf(y);
        }
        return (x == 0.0f) && (y == 0.0f);
      });

  TestAgainstRef<2_ULP>(atan2_ref, &undef_check);
}

TEST_P(HalfMathBuiltins, Precision_69_Half_atan2pi) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto atan2pi_ref = [](cl_float x, cl_float y) -> cl_float {
    return std::atan2(x, y) * M_1_PI;
  };

  const bool denorm_support =
      UCL::hasDenormSupport(device, CL_DEVICE_HALF_FP_CONFIG);
  const std::function<bool(cl_float, cl_float)> undef_check(
      [denorm_support](cl_float x, cl_float y) {
        // atan2pi(0.0, 0.0) is undefined
        if (!denorm_support) {
          return IsDenormalAsHalf(x) && IsDenormalAsHalf(y);
        }
        return (x == 0.0f) && (y == 0.0f);
      });

  TestAgainstRef<2_ULP>(atan2pi_ref, &undef_check);
}

TEST_P(HalfMathBuiltins, Precision_70_Half_sincos) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto sincos_ref = [](cl_float x, cl_float &out_cos) -> cl_float {
    out_cos = std::cos(x);
    return std::sin(x);
  };

  TestAgainstFloatReferenceArgRef<2_ULP>(sincos_ref);
}

TEST_P(HalfMathBuiltins, Precision_70_Half_sincos_local) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto sincos_ref = [](cl_float x, cl_float &out_cos) -> cl_float {
    out_cos = std::cos(x);
    return std::sin(x);
  };

  TestAgainstFloatReferenceArgRef<2_ULP>(sincos_ref);
}

TEST_P(HalfMathBuiltins, Precision_70_Half_sincos_private) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto sincos_ref = [](cl_float x, cl_float &out_cos) -> cl_float {
    out_cos = std::cos(x);
    return std::sin(x);
  };

  TestAgainstFloatReferenceArgRef<2_ULP>(sincos_ref);
}

TEST_P(HalfMathBuiltins, Precision_71_Half_tanpi) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto tanpi_ref = [](cl_float x) -> cl_float {
    // We need to manually track the sign to get
    // correct signedess for infinity
    cl_float sign = std::copysign(1.0, x);

    // reduce to the range [ -0.5, 0.5 ]
    cl_float absX = std::fabs(x);
    absX -= std::rint(absX);

    // remove the sign again
    sign *= std::copysign(1.0, absX);
    absX = std::fabs(absX);

    // use system tan to get the right result
    return sign * std::tan(absX * M_PI);
  };

  TestAgainstRef<2_ULP>(tanpi_ref);
}

TEST_P(HalfMathBuiltins, Precision_72_Half_erfc) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto erfc_ref = [](cl_float x) -> cl_float { return std::erfc(x); };

  TestAgainstRef<4_ULP>(erfc_ref);
}

TEST_P(HalfMathBuiltins, Precision_73_Half_erf) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto erf_ref = [](cl_float x) -> cl_float { return std::erf(x); };

  TestAgainstRef<4_ULP>(erf_ref);
}

TEST_P(HalfMathBuiltins, Precision_74_Half_lgamma) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto lgamma_ref = [](cl_float x) -> cl_float { return std::lgamma(x); };

  // lgamma has undefined ULP
  TestAgainstRef<MAX_ULP_ERROR>(lgamma_ref);
}

TEST_P(HalfMathBuiltins, Precision_75_Half_lgammar) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto lgammar_ref = [](cl_float x, cl_int &sign_out) -> cl_float {
    sign_out = std::signbit(std::tgamma(x)) ? -1 : 1;
    const cl_float result = std::lgamma(x);
    if (std::isnan(result) || ((0.0f >= x) && (std::floor(x) == x))) {
      sign_out = 0;
    }
    return result;
  };

  // lgamma_r has undefined ULP
  TestAgainstIntReferenceArgRef<MAX_ULP_ERROR>(lgammar_ref);
}

TEST_P(HalfMathBuiltins, Precision_75_Half_lgammar_local) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto lgammar_ref = [](cl_float x, cl_int &sign_out) -> cl_float {
    sign_out = std::signbit(std::tgamma(x)) ? -1 : 1;
    const cl_float result = std::lgamma(x);
    if (std::isnan(result) || ((0.0f >= x) && (std::floor(x) == x))) {
      sign_out = 0;
    }
    return result;
  };

  // lgamma_r has undefined ULP
  TestAgainstIntReferenceArgRef<MAX_ULP_ERROR>(lgammar_ref);
}

TEST_P(HalfMathBuiltins, Precision_75_Half_lgammar_private) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto lgammar_ref = [](cl_float x, cl_int &sign_out) -> cl_float {
    sign_out = std::signbit(std::tgamma(x)) ? -1 : 1;
    const cl_float result = std::lgamma(x);
    if (std::isnan(result) || ((0.0f >= x) && (std::floor(x) == x))) {
      sign_out = 0;
    }
    return result;
  };

  // lgamma_r has undefined ULP
  TestAgainstIntReferenceArgRef<MAX_ULP_ERROR>(lgammar_ref);
}

TEST_P(HalfMathBuiltins, Precision_76_Half_tgamma) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto tgamma_ref = [](cl_float x) -> cl_float { return std::tgamma(x); };

  TestAgainstRef<4_ULP>(tgamma_ref);
}

TEST_P(HalfMathBuiltins, Precision_77_Half_sinh) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto sinh_ref = [](cl_float x) -> cl_float { return std::sinh(x); };

  TestAgainstRef<2_ULP>(sinh_ref);
}

TEST_P(HalfMathBuiltins, Precision_78_Half_cosh) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto cosh_ref = [](cl_float x) -> cl_float { return std::cosh(x); };

  TestAgainstRef<2_ULP>(cosh_ref);
}

TEST_P(HalfMathBuiltins, Precision_79_Half_tanh) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto tanh_ref = [](cl_float x) -> cl_float { return std::tanh(x); };

  TestAgainstRef<2_ULP>(tanh_ref);
}

TEST_P(HalfMathBuiltinsPow, Precision_80_Half_pow) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto pow_ref = [](cl_float x, cl_float y) -> cl_float {
    // Special cases from CTS pow() reference:
    // if x = 1, return x for any y, even NaN
    if (x == 1.0f) {
      return x;
    }

    // if y == 0, return 1 for any x, even NaN
    if (y == 0.0f) {
      return 1.0f;
    }
    return std::pow(x, y);
  };

  TestAgainstRef<4_ULP>(pow_ref);
}

TEST_P(HalfMathBuiltinsPow, Precision_81_Half_powr) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto powr_ref = [](cl_float x, cl_float y) -> cl_float {
    if (x < 0.0f) {
      return NAN;
    }

    if (std::isnan(x) || std::isnan(y)) {
      return x + y;
    }

    if (1.0f == x) {
      return INFINITY == std::fabs(y) ? NAN : 1.0f;
    }

    if (0.0f == y) {
      return (0.0f == x || std::isinf(x)) ? NAN : 1.0f;
    }

    if (0.0f == x) {
      return y < 0.0f ? INFINITY : 0.0f;
    }

    if (std::isinf(x)) {
      return y < 0.0f ? 0.0f : INFINITY;
    }

    return std::pow(x, y);
  };

  TestAgainstRef<4_ULP>(powr_ref);
}

TEST_P(HalfMathBuiltinsPow, Precision_82_Half_pown) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto pown_ref = [](cl_float x, cl_int n) -> cl_float {
    return std::pow(x, n);
  };

  TestAgainstIntArgRef<4_ULP>(pown_ref);
}

TEST_P(HalfMathBuiltins, Precision_83_Half_rootn) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  auto rootn_ref = [](cl_float x, cl_int y) -> cl_float {
    if (0 == y) {
      return NAN;
    }

    // returns a NaN for x < 0 and y is even.
    if ((x < 0.0f) && (0 == (y & 1))) {
      return NAN;
    }

    if (0.0f == x) {
      // Mask sign and even/odd bits
      const cl_uint masked = y & cl_uint(0x80000001);
      switch (masked) {
        // x is +/-  zero and y is even > 0
        case 0:
          return 0.0f;

        // x is +/- zero and y is odd > 0
        case 1:
          return x;

        // x is +/- zero and y is even < 0.
        case 0x80000000:
          return INFINITY;

        // x is +/0 zero and y is odd < 0.
        case 0x80000001:
          return std::copysign(INFINITY, x);

        // not possible
        default:
          return NAN;
      }
    }

    const cl_float sign = x;
    x = std::fabs(x);
    x = std::exp2(std::log2(x) / cl_float(y));
    return std::copysign(x, sign);
  };

  TestAgainstIntArgRef<4_ULP>(rootn_ref);
}

// Miss out half3 to avoid complications with having sizeof(half4)
UCL_EXECUTION_TEST_SUITE_P(HalfMathBuiltins, testing::Values(OPENCL_C),
                           testing::Values(1, 2, 4, 8, 16))
UCL_EXECUTION_TEST_SUITE_P(HalfMathBuiltinsPow, testing::Values(OPENCL_C),
                           testing::Values(1, 2, 4, 8, 16))

TEST_P(Execution, Precision_84_Double_Remquo) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  const uint64_t N = 1;

  const cl_double x = -4.1757451841279743e+225;
  const cl_double y = std::numeric_limits<cl_double>::infinity();

  // This test checks we correctly set the quotient to 0 if `y` is infinite.
  AddOutputBuffer(N, kts::Reference1D<cl_int>([x, y](size_t) {
                    cl_int ret = std::numeric_limits<cl_int>::min();
                    Remquo7BitRef<cl_double>(x, y, ret);
                    return ret;
                  }));

  AddPrimitive(x);
  AddPrimitive(y);

  RunGeneric1D(N);
}

// CA-2476: Enable when fixed
#if defined(__MINGW32__) || defined(__MINGW64__)
TEST_P(Execution, DISABLED_Precision_85_Single_tgamma) {
#else
TEST_P(Execution, Precision_85_Single_tgamma) {
#endif
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  // tgamma() builtin isn't tested by the CTS, unit test checks
  // we meet ULP requirements for single precision
  std::vector<cl_float> input(65536);
  ucl::Environment::instance->GetInputGenerator().GenerateFloatData(input);

  // Boundary cases taken from our tgamma() abacus implementation
  const std::array<cl_float, 5> special_cases{1.8e-6f, 36.0f, -87.0f, 2.0f,
                                              -100.0f};
  for (auto f : special_cases) {
    input.push_back(f);
    input.push_back(std::nextafter(f, INFINITY));
    input.push_back(std::nextafter(f, -INFINITY));
  }

  const size_t N = input.size();
  AddInputBuffer(
      N, kts::Reference1D<cl_float>([&input](size_t id) { return input[id]; }));

  AddOutputBuffer(N, makeULPStreamer<cl_float, 16_ULP>(
                         [&input](size_t id) -> cl_double {
                           const cl_double promote =
                               static_cast<cl_double>(input[id]);
                           return std::tgamma(promote);
                         },
                         this->device));

  RunGeneric1D(N);
}

// CA-2637 - Offline 32-bit precision test mismatch
TEST_P(Execution, Precision_86_Single_lgamma) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  const std::vector<cl_float> input{0.0f, 1.0f, 3.14f, 5.15f, 6.01f, 7.89f};

  const size_t N = input.size();
  AddInputBuffer(
      N, kts::Reference1D<cl_float>([&input](size_t id) { return input[id]; }));

  AddOutputBuffer(N, makeULPStreamer<cl_float, 16_ULP>(
                         [&input](size_t id) -> cl_double {
                           const cl_double promote =
                               static_cast<cl_double>(input[id]);
                           return std::lgamma(promote);
                         },
                         this->device));

  RunGeneric1D(N / 2);
}

// Broken on 32-bit Windows [CA-2112] and MinGW [CA-2478]
#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__MINGW32__) || \
    defined(__MINGW64__)
TEST_P(Execution, DISABLED_Precision_87_Single_sincos)
#else
// CA-2637 - Offline 32-bit precision test mismatch
TEST_P(Execution, Precision_87_Single_sincos)
#endif
{
  if (!isSourceTypeIn({OPENCL_C, SPIRV, OFFLINESPIRV})) {
    GTEST_SKIP();
  }
  std::vector<cl_float> input(128);
  ucl::Environment::instance->GetInputGenerator().GenerateFloatData(input);

  const size_t N = input.size();
  AddInputBuffer(
      N, kts::Reference1D<cl_float>([&input](size_t id) { return input[id]; }));

  AddOutputBuffer(N, makeULPStreamer<cl_float, 4_ULP>(
                         [&input](size_t id) -> cl_double {
                           const cl_double promote =
                               static_cast<cl_double>(input[id]);
                           return std::sin(promote);
                         },
                         this->device));

  AddOutputBuffer(N, makeULPStreamer<cl_float, 4_ULP>(
                         [&input](size_t id) -> cl_double {
                           const cl_double promote =
                               static_cast<cl_double>(input[id]);
                           return std::cos(promote);
                         },
                         this->device));

  RunGeneric1D(N);
}

// Broken on 32-bit Windows [CA-2112] and MinGW [CA-2478]
#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__MINGW32__) || \
    defined(__MINGW64__)
TEST_P(Execution, DISABLED_Precision_87_Single_sincos_local)
#else
// CA-2637 - Offline 32-bit precision test mismatch
TEST_P(Execution, Precision_87_Single_sincos_local)
#endif
{
  if (!isSourceTypeIn({OPENCL_C, SPIRV, OFFLINESPIRV})) {
    GTEST_SKIP();
  }
  std::vector<cl_float> input(128);
  ucl::Environment::instance->GetInputGenerator().GenerateFloatData(input);

  const size_t N = input.size();
  AddInputBuffer(
      N, kts::Reference1D<cl_float>([&input](size_t id) { return input[id]; }));

  AddOutputBuffer(N, makeULPStreamer<cl_float, 4_ULP>(
                         [&input](size_t id) -> cl_double {
                           const cl_double promote =
                               static_cast<cl_double>(input[id]);
                           return std::sin(promote);
                         },
                         this->device));

  AddOutputBuffer(N, makeULPStreamer<cl_float, 4_ULP>(
                         [&input](size_t id) -> cl_double {
                           const cl_double promote =
                               static_cast<cl_double>(input[id]);
                           return std::cos(promote);
                         },
                         this->device));

  RunGeneric1D(N);
}

TEST_P(Execution, Precision_87_Double_sincos) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  std::vector<cl_double> input(128);
  ucl::Environment::instance->GetInputGenerator().GenerateFloatData(input);

  const size_t N = input.size();
  AddInputBuffer(N, kts::Reference1D<cl_double>(
                        [&input](size_t id) { return input[id]; }));

  AddOutputBuffer(N, makeULPStreamer<cl_double, 4_ULP>(
                         [&input](size_t id) -> long double {
                           const long double promote =
                               static_cast<long double>(input[id]);
                           return std::sin(promote);
                         },
                         this->device));

  AddOutputBuffer(N, makeULPStreamer<cl_double, 4_ULP>(
                         [&input](size_t id) -> long double {
                           const long double promote =
                               static_cast<long double>(input[id]);
                           return std::cos(promote);
                         },
                         this->device));

  RunGeneric1D(N);
}

TEST_P(Execution, Precision_87_Double_sincos_local) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  std::vector<cl_double> input(128);
  ucl::Environment::instance->GetInputGenerator().GenerateFloatData(input);

  const size_t N = input.size();
  AddInputBuffer(N, kts::Reference1D<cl_double>(
                        [&input](size_t id) { return input[id]; }));

  AddOutputBuffer(N, makeULPStreamer<cl_double, 4_ULP>(
                         [&input](size_t id) -> long double {
                           const long double promote =
                               static_cast<long double>(input[id]);
                           return std::sin(promote);
                         },
                         this->device));

  AddOutputBuffer(N, makeULPStreamer<cl_double, 4_ULP>(
                         [&input](size_t id) -> long double {
                           const long double promote =
                               static_cast<long double>(input[id]);
                           return std::cos(promote);
                         },
                         this->device));

  RunGeneric1D(N);
}

TEST_P(ExecutionOpenCLC, Precision_88_Half_Pown_Edgecases) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  const size_t N = 11;
  const std::pair<cl_half, cl_int> inputs[N] = {{0x7C00 /* INF */, 0},
                                                {0xFC00 /* -INF */, 0},
                                                {0x3B00 /* NAN */, 0},
                                                {0, 0},
                                                {0x3C00 /* 1.0 */, 0},
                                                {0x4000 /* 2.0 */, 0},
                                                {0x3B00 /* NAN */, 1},
                                                {0, -1},
                                                {0, -2},
                                                {0, 1},
                                                {0, 2}};

  const cl_ushort outputs[N] = {0x3C00 /* 1.0 */,
                                0x3C00 /* 1.0 */,
                                0x3C00 /* 1.0 */,
                                0x3C00 /* 1.0 */,
                                0x3C00 /* 1.0 */,
                                0x3C00 /* 1.0 */,
                                0x3B00 /* NAN */,
                                0x7C00 /* INF */,
                                0x7C00 /* INF */,
                                0x0,
                                0x0};

  AddInputBuffer(N, kts::Reference1D<cl_half>([&inputs](size_t i) {
                   return std::get<0>(inputs[i]);
                 }));
  AddInputBuffer(N, kts::Reference1D<cl_int>([&inputs](size_t i) {
                   return std::get<1>(inputs[i]);
                 }));

  AddOutputBuffer(N, kts::Reference1D<cl_ushort>(
                         [&outputs](size_t i) { return outputs[i]; }));

  RunGeneric1D(N);
}

TEST_P(ExecutionOpenCLC, Precision_89_Half_atan2_zeros) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  const size_t N = 4;
  const std::pair<cl_half, cl_half> inputs[N] = {
      {0x8000 /* -0 */, 0},
      {0x8000 /* -0 */, 0x8000 /* -0 */},
      {0, 0x8000 /* -0 */},
      {0, 0}};

  const cl_ushort atan2_outputs[N] = {0x8000 /* -0 */, 0xC248 /* -PI */,
                                      0x4248 /* PI */, 0x0};
  const cl_ushort atan2pi_outputs[N] = {0x8000 /* -0 */, 0xBC00 /* -1.0 */,
                                        0x3C00 /* 1.0 */, 0x0};

  AddInputBuffer(N, kts::Reference1D<cl_half>([&inputs](size_t i) {
                   return std::get<0>(inputs[i]);
                 }));
  AddInputBuffer(N, kts::Reference1D<cl_half>([&inputs](size_t i) {
                   return std::get<1>(inputs[i]);
                 }));

  // Verify atan2()
  AddOutputBuffer(N, kts::Reference1D<cl_ushort>([&atan2_outputs](size_t i) {
                    return atan2_outputs[i];
                  }));

  // Verify atan2pi()
  AddOutputBuffer(N, kts::Reference1D<cl_ushort>([&atan2pi_outputs](size_t i) {
                    return atan2pi_outputs[i];
                  }));

  RunGeneric1D(N);
}

TEST_P(ExecutionOpenCLC, Precision_90_Half_Ldexp_Edgecases) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  if (!UCL::hasDenormSupport(device, CL_DEVICE_HALF_FP_CONFIG)) {
    // All save two of the edge cases tested expect a denormal result,
    // as the focus is on avoiding underflow to zero.
    GTEST_SKIP();
  }

  const size_t N = 19;
  const std::pair<cl_half, cl_int> inputs[N] = {
      {0x21f8 /* 0.01165772 */, -17},
      {0x11f8 /* 0.0007286075 */, -13},
      {0x15f8 /* 0.001457215 */, -14},
      {0x1df7 /* 0.00582504 */, -16},
      {0x1df8 /* 0.00582886 */, -16},
      {0x1df9 /* 0.00583267 */, -16},
      {0x19f7 /* 0.00291252 */, -15},
      {0x19f8 /* 0.00291443 */, -15},
      {0x19f9 /* 0.00291634 */, -15},
      {0xb340 /* -0.226562 */, -22},
      {0x0001 /* 1p-24 */, CL_INT_MIN /*-2147483648*/},
      {0x4000 /* 4.0 */, CL_INT_MAX /*2147483647*/},
      {0xe73c /* -1852 */, -35},
      {0xfaec /* -56704 */, -40},
      {0x78ae /* 38336 */, -40},
      {0xfb93 /* -62048 */, -40},
      {0x7bed /* 64928 */, -40},
      {0xf934 /* -42624 */, -41},
      {0x7287 /* 13368 */, 2},
  };

  const cl_half outputs[N] = {
      // ldexp(0.01165772, -17) ==> 8.894134521484375e-08
      // Rounds to 0x1p-24 (half 0x0001) rather than 0x1p-23 (half 0x0002).
      0x0001,
      // ldexp(0.0007286075, -13) ==> 8.894134521484375e-08
      0x0001,
      // ldexp(0.001457215, -14) ==> 8.894134521484375e-08
      0x0001,
      // ldexp(0.00582504, -16) ==> 8.894134521484375e-08
      0x0001,
      // ldexp(0.00582886, -16) ==> 8.894134521484375e-08
      0x0001,
      // ldexp(0.00583267, -16) ==> 8.894134521484375e-08
      0x0001,
      // ldexp(0.00291443, -15) ==> 8.894134521484375e-08
      0x0001,
      // ldexp(0.00291252, -15) ==> 8.88831e-08
      0x0001,
      // ldexp(0.00291634, -15) ==> 8.89995e-08
      0x0001,
      // ldexp(-0.226562, -22) ==> -5.4016590118408206e-08
      // Although this result is too low to be representable in half we expect
      // the lowest representable half rather than zero due to rounding.
      0x8001, /* -5.960464477539063e-08 */
      // ldexp(5.960464477539063e-08, -2147483648) is too small to represent
      0x0000,
      // ldexp(4, 2147483647) is too large to represent
      0x7C00, /* INF */
      // ldexp(-1852, -35) ==> -5.390029400587082e-08
      // Although this result is too low to be representable in half we expect
      // the lowest representable half rather than zero due to rounding.
      0x8001, /* -5.960464477539063e-08 */
      // ldexp(-56704, -40) ==> -5.1572e-08
      // Although this result is too low to be representable in half we expect
      // the lowest representable half rather than zero due to rounding.
      0x8001, /* -5.960464477539063e-08 */
      // ldexp(-38336, -40) ==> 3.48664e-08
      // Although this result is too low to be representable in half we expect
      // the lowest representable half rather than zero due to rounding.
      0x1, /* 5.960464477539063e-08 */
      // ldexp(-62048, -40) ==> -5.64323e-08
      // Although this result is too low to be representable in half we expect
      // the lowest representable half rather than zero due to rounding.
      0x8001, /* -5.960464477539063e-08 */
      // ldexp(64928, -40) ==> 5.90517e-08
      // Although this result is too low to be representable in half we expect
      // the lowest representable half rather than zero due to rounding.
      0x1, /* 5.960464477539063e-08 */
      // ldexp(-42624, -41) is too small to represent.
      0x8000, /* -0.0 */
      // ldexp(13368, 2) must not involve an infinite intermediate result.
      0x7a87, /* 53472 */
  };

  AddInputBuffer(N, kts::Reference1D<cl_half>([&inputs](size_t i) {
                   return std::get<0>(inputs[i]);
                 }));

  AddInputBuffer(N, kts::Reference1D<cl_int>([&inputs](size_t i) {
                   return std::get<1>(inputs[i]);
                 }));

  AddOutputBuffer(N, kts::Reference1D<cl_half>(
                         [&outputs](size_t i) { return outputs[i]; }));

  RunGeneric1D(N);
}

TEST_P(Execution, Precision_91_double_convert_char_rtn) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }
  AddInputBuffer(
      1, kts::Reference1D<cl_double>([](size_t) { return cl_double(-3.5); }));
  AddOutputBuffer(
      1, kts::Reference1D<cl_char>([](size_t) { return cl_char(-4); }));
  RunGeneric1D(1);
}

TEST_P(Execution, Precision_91_double_convert_char_rtp) {
  // Whether or not the kernel will be vectorized at a global size of 1 is
  // dependent on the target.
  fail_if_not_vectorized_ = false;

  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }
  AddInputBuffer(
      1, kts::Reference1D<cl_double>([](size_t) { return cl_double(-3.5); }));
  AddOutputBuffer(
      1, kts::Reference1D<cl_char>([](size_t) { return cl_char(-3); }));
  RunGeneric1D(1);
}

TEST_P(ExecutionOpenCLC, Precision_92_Half_Hypot_Edgecases) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  const size_t N = 14;
  const std::pair<cl_half, cl_half> inputs[N] = {
      {0xa051 /* -0.00843048 */, 0xa7b4 /* -0.0300903 */},
      {0x2e2c /* 0.0964355 */, 0xad17 /* -0.0795288 */},
      {0x1217 /* 0.000743389 */, 0x9130 /*-0.00063324*/},
      {0xe517 /* -1303 */, 0x662c /* 1580 */},
      {0xf9a8 /* -46336 */, 0xf9a7 /* -46304 */},
      {0x7be4 /* 64640.0 */, 0xf126 /* -10544 */},
      {0xfbe2 /* -64576.0 */, 0xf159 /* -10952 */},
      {0x7981 /* 45088.0 */, 0x79cd /* 47520 */},
      {0xf182 /* -11280.0 */, 0xfbe0 /* -64512.0 */},
      {0xf66c /* -26304.0 */, 0xfb6c /* -60800.0 */},
      {0xfbf4 /* -65152.0 */, 0x79cd /* 47520.0 */},
      {0x7b80 /* 61440.0 */, 0x7105 /* 10280.0 */},
      {0x68b6 /* 2412.0 */, 0x7bfe /* 65472.0 */},
      {0xf9f7 /* -48864.0 */, 0x7954 /* 43648.0 */},
  };

  const cl_float outputs[N] = {
      /*
       * Fails with fast divide implementation of hypot() where error
       * is greater than 2 ULP. Can be triggered on host with the option
       * CA_EXTRA_COMPILE_OPTS=-cl-fast-relaxed-math
       */
      // hypot(-0.00843048, -0.0300903)
      0.031249f,
      // hypot(0.0964355, -0.0795288)
      0.124999f,
      // hypot(0.000743389, -0.00063324)
      0.000976535f,
      // hypot(-1303, 1580)
      2047.98f,
      /*
       * Fails with fast divide implementation of hypot() without error
       * leniency allowing overflow to within ULP error of reference.
       */
      // hypot(-46336.0, -46304.0), RTE rounds to 65504 as half
      65506.4f,
      // hypot(64640.0, -10544.0), RTE rounds to 65504 as half
      65494.3f,
      // hypot(-64576, 10952.0), RTE rounds to 65504 as half
      65498.1f,
      // hypot(45088.0, 47520.0), RTE rounds to 65504 as half
      65506.3f,
      // hypot(-11280.0, -64512.0), RTE rounds to 65504 as half
      65490.7f,
      // hypot(-26304.0, -60800.0), RTE rounds to INF as half
      66246.1f,
      // hypot(-65152.0, 47520.0), RTE rounds to INF as half
      80640.8f,
      // hypot(61440.0, 10280.0), RTE rounds to 62304 as half
      62294.1f,
      // hypot(2412.0, 65472.0), RTE rounds to 65504 as half
      65516.4f,
      /*
       * Fails with safe sqrt() implementation of hypot() without error
       * leniency allowing overflow to within ULP error of reference.
       */
      // hypot(-48864.0, 43648.0), RTE rounds to 65504 as half
      65519.8f,
  };

  AddInputBuffer(N, kts::Reference1D<cl_half>([&inputs](size_t i) {
                   return std::get<0>(inputs[i]);
                 }));

  AddInputBuffer(N, kts::Reference1D<cl_half>([&inputs](size_t i) {
                   return std::get<1>(inputs[i]);
                 }));

  kts::Reference1D<cl_float> ref_lambda(
      [&outputs](size_t i) -> cl_float { return outputs[i]; });

  AddOutputBuffer(N, makeULPStreamer<cl_half, 2_ULP>(ref_lambda, device));

  RunGeneric1D(N);
}

TEST_P(Execution, Precision_93_Divide_Relaxed) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  if (!isSourceTypeIn({OPENCL_C, SPIRV, OFFLINE})) {
    GTEST_SKIP();
  }
  // Fast math ULP requirements only apply after CL 2.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }
  AddBuildOption("-cl-fast-relaxed-math");

  const size_t N = 3;
  // The kernel treats these values as float2s, it divides pairs of values from
  // lhs by pairs from rhs and expects the values in expected_outputs to result.
  const cl_uint lhs_inputs[N * 2] = {0x7fc00000, 0xa1fcbd7c, 0x2fb75fe2,
                                     0x380b4a11, 0xcec6f557, 0x7fc00000};

  const cl_uint rhs_inputs[N * 2] = {0x7fc00000, 0x2eff9ba9, 0xc4c8ffcd,
                                     0x7fc00000, 0x44cc6fa1, 0xb57b0ced};

  const cl_uint expected_outputs[N * 2] = {0x7fc00000, 0xb27d20b3, 0xaa698d75,
                                           0x7fc00000, 0xc9792405, 0x7fc00000};

  AddInputBuffer(N * 2, kts::Reference1D<cl_float>([&lhs_inputs](size_t id) {
                   return cargo::bit_cast<cl_float>(lhs_inputs[id]);
                 }));

  AddInputBuffer(N * 2, kts::Reference1D<cl_float>([&rhs_inputs](size_t id) {
                   return cargo::bit_cast<cl_float>(rhs_inputs[id]);
                 }));

  auto ref_output = [&expected_outputs](size_t id) -> cl_double {
    const cl_float expected = cargo::bit_cast<cl_float>(expected_outputs[id]);
    return static_cast<cl_double>(expected);
  };
  AddOutputBuffer(N * 2,
                  makeULPStreamer<cl_float, 2_ULP>(ref_output, this->device));

  RunGeneric1D(N);
}
