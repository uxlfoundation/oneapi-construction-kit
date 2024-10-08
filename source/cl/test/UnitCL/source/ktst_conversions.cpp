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

// Third-party headers
#include <gtest/gtest.h>

// Standard headers
#include <numeric>
#include <type_traits>

// In-house headers
#include <cargo/utility.h>

#include "Common.h"
#include "Device.h"
#include "kts/execution.h"
#include "kts/precision.h"

using namespace kts::ucl;

namespace {
// The different config parameters we want to test the Cartesian product of
const std::array<unsigned, 6> vector_widths{{1, 2, 3, 4, 8, 16}};
const std::array<bool, 2> sat{{true, false}};
const std::array<RoundingMode, 5> roundings{
    {RoundingMode::NONE, RoundingMode::RTE, RoundingMode::RTP,
     RoundingMode::RTZ, RoundingMode::RTN}};

cl_float roundFloat(const cl_float in, const RoundingMode rounding) {
  switch (rounding) {
    case RoundingMode::RTE:
      return std::rint(in);
    case RoundingMode::RTZ:
      return std::trunc(in);
    case RoundingMode::RTP:
      return std::ceil(in);
    case RoundingMode::RTN:
      return std::floor(in);
    default:
      return in;
  }
}

// Reference functions for explicit conversions
template <typename StrongFrom, typename StrongTo>
struct ConvertRefHelper;

// half -> float
template <>
struct ConvertRefHelper<CLhalf, CLfloat> {
  static cl_float reference(const cl_half x, const RoundingMode, const bool) {
    return ConvertHalfToFloat(x);
  }

  static bool undef(const cl_half, const bool) { return false; }
  static bool denormal(const cl_half x) { return IsDenormal(x); }
};

// half -> double
template <>
struct ConvertRefHelper<CLhalf, CLdouble> {
  static cl_double reference(const cl_half x, const RoundingMode, const bool) {
    return ConvertHalfToFloat(x);
  }

  static bool undef(const cl_half, const bool) { return false; }
  static bool denormal(const cl_half x) { return IsDenormal(x); }
};

// half -> half
template <>
struct ConvertRefHelper<CLhalf, CLhalf> {
  static cl_half reference(const cl_half x, const RoundingMode, const bool) {
    return x;
  }
  static bool undef(const cl_half, const bool) { return false; }
  static bool denormal(const cl_half x) { return IsDenormal(x); }
};

// half -> bool
template <>
struct ConvertRefHelper<CLhalf, CLbool> {
  static cl_bool reference(const cl_half x, const RoundingMode, const bool) {
    const cl_float as_float = ConvertHalfToFloat(x);
    const bool host_bool(as_float);
    return static_cast<cl_bool>(host_bool);
  }

  static bool undef(const cl_half, const bool) { return false; }
  static bool denormal(const cl_half x) { return IsDenormal(x); }
};

// half -> {u}char/{u}short/{u}int/{u}long
template <typename StrongTo>
struct ConvertRefHelper<CLhalf, StrongTo> {
  using WeakTo = typename StrongTo::WrappedT;
  static WeakTo reference(const cl_half x, const RoundingMode rounding,
                          const bool saturated) {
    const cl_float as_float = ConvertHalfToFloat(x);
    const cl_float rounded_float = roundFloat(as_float, rounding);

    if (saturated) {
      if (std::isnan(rounded_float)) {
        return 0;
      }

      const WeakTo max_int = std::numeric_limits<WeakTo>::max();
      if (rounded_float > static_cast<cl_float>(max_int)) {
        return max_int;
      }

      const WeakTo min_int = std::numeric_limits<WeakTo>::min();
      if (rounded_float < static_cast<cl_float>(min_int)) {
        return min_int;
      }
    }

    return WeakTo(rounded_float);
  }

  static bool undef(const cl_half x, const bool saturated) {
    // Saturation defines results for out-of-range values
    if (saturated) {
      return false;
    }

    // C99 specification, section 6.3.1.4:
    //
    // When a finite value of real floating type is converted to an integer type
    // other than _Bool, the fractional part is discarded (i.e., the value is
    // truncated toward zero). If the value of the integral part cannot be
    // represented by the integer type, the behaviour is undefined.
    const cl_float as_float = ConvertHalfToFloat(x);

    // Converting signed float to unsigned int
    const bool signed_to_unsigned =
        std::is_unsigned_v<WeakTo> && std::signbit(as_float);

    // Integral component is too large to be represented in the int type
    const WeakTo max_int = std::numeric_limits<WeakTo>::max();
    const bool too_large = std::fabs(as_float) > static_cast<cl_float>(max_int);

    // NAN can't be represented by an integer type
    const bool is_nan = std::isnan(as_float);

    return signed_to_unsigned || too_large || is_nan;
  }
  static bool denormal(const cl_half x) { return IsDenormal(x); }
};

// {u}char/{u}short/{u}int/{u}long -> half
template <typename StrongFrom>
struct ConvertRefHelper<StrongFrom, CLhalf> {
  using WeakFrom = typename StrongFrom::WrappedT;
  static cl_half reference(const WeakFrom x, const RoundingMode rounding,
                           const bool) {
    const cl_float as_float(x);
    return ConvertFloatToHalf(as_float, rounding);
  }

  // Signed integer types
  template <class T = WeakFrom>
  static bool undef(const std::enable_if_t<std::is_signed_v<T>, T> x,
                    const bool) {
    // Signed 32 & 64 bit integer types which are too large to represent in
    // half precision have an undefined result, and saturation isn't valid
    return (x > TypeInfo<cl_half>::max_int_bits) ||
           (x < -TypeInfo<cl_half>::max_int_bits);
  }

  // Unsigned integer types
  template <class T = WeakFrom>
  static bool undef(const std::enable_if_t<std::is_unsigned_v<T>, T> x,
                    const bool) {
    // Unsigned 32 & 64 bit integer types which are too large to represent in
    // half precision have an undefined result, and saturation isn't valid
    return x > TypeInfo<cl_half>::max_int_bits;
  }
  static bool denormal(const WeakFrom) { return false; }
};

// float -> half
template <>
struct ConvertRefHelper<CLfloat, CLhalf> {
  static cl_half reference(const cl_float x, const RoundingMode rounding,
                           const bool) {
    return ConvertFloatToHalf(x, rounding);
  }

  static bool undef(const cl_float, const bool) { return false; }
  static bool denormal(const cl_float) { return false; }
};

// double -> half
template <>
struct ConvertRefHelper<CLdouble, CLhalf> {
  static cl_half reference(const cl_double x, const RoundingMode rounding,
                           const bool) {
    return ConvertFloatToHalf(x, rounding);
  }

  static bool undef(const cl_double, const bool) { return false; }
  static bool denormal(const cl_double) { return false; }
};

template <typename StrongT>
struct ConvertValidator : public kts::Validator<typename StrongT::WrappedT> {
  ConvertValidator(cl_device_id) {}
};

template <>
struct ConvertValidator<CLhalf> {
  ConvertValidator(cl_device_id device) : device(device) {}

  bool validate(const cl_half &expected, const cl_half &actual) {
    const bool denormSupport =
        UCL::hasDenormSupport(device, CL_DEVICE_HALF_FP_CONFIG);

    const bool expectedDenormal =
        !IsNormal(expected) && !IsInf(expected) && !IsNaN(expected);
    if (!denormSupport && expectedDenormal) {
      // Accept +/- 0.0 if denormals aren't supported and result was a denormal
      const cl_ushort asInt = matchingType(actual);
      if (0 == asInt || 0x8000 == asInt) {
        return true;
      }
    }

    if (IsNaN(expected) && IsNaN(actual)) {
      return true;
    }
    return expected == actual;
  }

  void print(std::stringstream &s, const cl_half &value) {
    const cl_float as_float = ConvertHalfToFloat(value);
    s << "0x" << std::hex << value << std::dec << "[" << as_float << "]";
  }

  cl_device_id device;
};

template <typename StrongT>
struct ConvertStreamer final
    : public kts::GenericStreamer<typename StrongT::WrappedT,
                                  ConvertValidator<StrongT>> {
  using WeakT = typename StrongT::WrappedT;
  using V = ConvertValidator<StrongT>;
  ConvertStreamer(kts::Reference1D<WeakT> ref, cl_device_id device)
      : kts::GenericStreamer<WeakT, V>(ref, {device}) {}
  ConvertStreamer(kts::Reference1D<WeakT> ref,
                  const std::vector<kts::Reference1D<WeakT>> &&f,
                  cl_device_id device)
      : kts::GenericStreamer<WeakT, V>(ref, std::forward<decltype(f)>(f),
                                       {device}) {}
};

// Fixture for testing cl_half being converted to other CL types
struct HalfToGentypeConversions : public ExecutionWithParam<unsigned> {
  template <typename StrongT>
  void Run() {
    using WeakT = typename StrongT::WrappedT;
    if (!UCL::hasHalfSupport(device)) {
      GTEST_SKIP();
    }
    const ucl::MathMode math_mode = ucl::Environment::instance->math_mode;
    const size_t in_elements = HalfInputSizes::getInputSize(math_mode);

    const unsigned vec_width = getParam();
    const size_t out_elements = in_elements * vec_width;

    kts::Reference1D<cl_half> refIn = [](size_t index) {
      const cl_ushort as_ushort = index;
      return cargo::bit_cast<cl_half>(as_ushort);
    };
    AddInputBuffer(in_elements, refIn);

    kts::Reference1D<WeakT> refOut = [&refIn, vec_width](size_t index) {
      // Scalar input is broadcast across all vectors elements
      index /= vec_width;
      const cl_half in = refIn(index);

      return ConvertRefHelper<CLhalf, StrongT>::reference(
          in, RoundingMode::NONE, false);
    };

    // Custom streamer for validation that we can specialize for half
    std::shared_ptr<ConvertStreamer<StrongT>> out_streamer;

    // Checks for flush to zero behaviour of denormal inputs
    auto ftz_fallback = [&refIn, vec_width](size_t index) {
      // Scalar input is broadcast across all vectors elements
      index /= vec_width;
      const cl_half in = refIn(index);
      // Flush denormals to zero
      if (ConvertRefHelper<CLhalf, StrongT>::denormal(in)) {
        return WeakT(0);
      }

      return ConvertRefHelper<CLhalf, StrongT>::reference(
          in, RoundingMode::NONE, false);
    };

    const bool denormSupport =
        UCL::hasDenormSupport(device, CL_DEVICE_HALF_FP_CONFIG);
    if (denormSupport) {
      out_streamer = std::make_shared<ConvertStreamer<StrongT>>(refOut, device);
    } else {
      const std::vector<kts::Reference1D<WeakT>> fallbacks{ftz_fallback};
      out_streamer = std::make_shared<ConvertStreamer<StrongT>>(
          refOut, std::move(fallbacks), device);
    }

    // Some conversions allow undefined behaviour, which we see manifest in
    // differing conversion results depending on vectorization width due to
    // optimizations.
    auto undef_callback = [&refIn, vec_width](size_t index) -> bool {
      // Scalar input is broadcast across all vectors elements
      index /= vec_width;
      const cl_half in = refIn(index);
      return ConvertRefHelper<CLhalf, StrongT>::undef(in, false);
    };

    out_streamer->SetUndefCallback(std::move(undef_callback));
    AddOutputBuffer(out_elements, out_streamer);

    AddMacro("IN_TYPE", "half");
    if (std::is_same_v<StrongT, CLbool>) {
      // The size of bool device side is implementation defined, so use an
      // unsigned int as the parameter type. See OpenCL spec section 6.9 for
      // kernel argument restrictions.
      AddMacro("OUT_TYPE_SCALAR", Stringify<WeakT>::as_str);
    } else {
      AddMacro("OUT_TYPE_SCALAR", Stringify<StrongT>::as_str);
    }
    if (1 == vec_width) {
      AddMacro("OUT_TYPE_VECTOR", Stringify<StrongT>::as_str);
    } else {
      AddMacro("OUT_TYPE_VECTOR",
               Stringify<StrongT>::as_str + std::to_string(vec_width));
      AddMacro("STORE_FUNC", "vstore" + std::to_string(vec_width));
    }

    RunGeneric1D(in_elements);
  }
};

// Fills input buffer with integer data to test
template <typename T>
void PopulateData(std::vector<T> &buffer) {
  auto env = ucl::Environment::instance;
  env->GetInputGenerator().GenerateIntData(buffer);
}

// std::uniform_distribution isn't defined for char types, just test them all
void PopulateData(std::vector<cl_char> &buffer) {
  std::iota(buffer.begin(), buffer.end(), std::numeric_limits<cl_char>::min());
}

void PopulateData(std::vector<cl_uchar> &buffer) {
  std::iota(buffer.begin(), buffer.end(), std::numeric_limits<cl_uchar>::min());
}

void PopulateData(std::vector<cl_float> &buffer) {
  auto env = ucl::Environment::instance;
  env->GetInputGenerator().GenerateFloatData(buffer);
}

void PopulateData(std::vector<cl_double> &buffer) {
  auto env = ucl::Environment::instance;
  env->GetInputGenerator().GenerateFloatData(buffer);
}

// Fixture for testing CL types being converted to cl_half
struct GentypeToHalfConversions : public ExecutionWithParam<unsigned> {
  template <typename StrongT>
  void Run() {
    using WeakT = typename StrongT::WrappedT;
    if (!UCL::hasHalfSupport(device)) {
      GTEST_SKIP();
    }

    // We can't test every input value for all types, for 32-bit types and
    // larger there are too many variants. Instead randomly generate inputs
    // across the full range of possible values.
    const ucl::MathMode math_mode = ucl::Environment::instance->math_mode;
    const size_t in_elements = HalfInputSizes::getInputSize(math_mode);

    const unsigned vec_width = getParam();
    const size_t out_elements = in_elements * vec_width;

    std::vector<WeakT> input_data(in_elements);
    PopulateData(input_data);

    kts::Reference1D<WeakT> refIn = [&input_data](size_t x) {
      return input_data[x];
    };
    AddInputBuffer(in_elements, refIn);

    // Use cl_float except where weakT is cl_double
    using FloatT = decltype(WeakT(1) + 1.0f);
    kts::Reference1D<FloatT> reference = [&refIn, vec_width](size_t index) {
      // Scalar input is broadcast across all vector elements
      index /= vec_width;

      const WeakT in = refIn(index);
      const FloatT as_float(in);

      if (std::is_same_v<StrongT, CLbool> && (1 != vec_width) &&
          (FloatT(1.0) == as_float)) {
        // Casting a bool to a vector type results in -1, rather than 1
        return FloatT(-1.0);
      }

      return as_float;
    };

    auto refOut = makeULPStreamer<cl_half, 0_ULP>(reference, device);
    AddOutputBuffer(out_elements, refOut);

    if (std::is_same_v<StrongT, CLbool>) {
      // The size of bool device side is implementation defined, so use an
      // unsigned int as the parameter type. See OpenCL spec section 6.9 for
      // kernel argument restrictions.
      AddMacro("IN_TYPE", Stringify<WeakT>::as_str);
    } else {
      AddMacro("IN_TYPE", Stringify<StrongT>::as_str);
    }
    AddMacro("OUT_TYPE_SCALAR", "half");

    if (1 == vec_width) {
      AddMacro("OUT_TYPE_VECTOR", "half");
    } else {
      AddMacro("OUT_TYPE_VECTOR", "half" + std::to_string(vec_width));
      AddMacro("STORE_FUNC", "vstore" + std::to_string(vec_width));
    }

    RunGeneric1D(in_elements);
  }
};

class ReinterpretAllVecWidthsTest : public ExecutionWithParam<unsigned> {
  void SetMacro(const std::string &in_type, const std::string &out_type) {
    AddMacro("IN_TYPE_SCALAR", in_type);
    AddMacro("OUT_TYPE_SCALAR", out_type);

    const unsigned vec_width = getParam();
    if (1 == vec_width) {
      AddMacro("IN_TYPE_VECTOR", in_type);
      AddMacro("OUT_TYPE_VECTOR", out_type);
      AddMacro("AS_FUNC", "as_" + out_type);
    } else {
      const std::string vec_str = std::to_string(vec_width);
      const std::string out_type_vec(out_type + vec_str);
      AddMacro("IN_TYPE_VECTOR", in_type + vec_str);
      AddMacro("OUT_TYPE_VECTOR", out_type_vec);
      AddMacro("AS_FUNC", "as_" + out_type_vec);
      AddMacro("STORE_FUNC", "vstore" + vec_str);
      AddMacro("LOAD_FUNC", "vload" + vec_str);
    }
  }

 public:
  template <typename StrongFrom, typename StrongTo>
  void Run() {
    using WeakFrom = typename StrongFrom::WrappedT;
    using WeakTo = typename StrongTo::WrappedT;

    const bool uses_half =
        std::is_same_v<StrongFrom, CLhalf> || std::is_same_v<StrongTo, CLhalf>;
    if (uses_half && !UCL::hasHalfSupport(device)) {
      GTEST_SKIP();
    }

    SetMacro(Stringify<StrongFrom>::as_str, Stringify<StrongTo>::as_str);

    const ucl::MathMode math_mode = ucl::Environment::instance->math_mode;
    const size_t elements = HalfInputSizes::getInputSize(math_mode);

    std::vector<WeakFrom> input_data(elements);
    PopulateData(input_data);

    kts::Reference1D<WeakFrom> refIn = [&input_data](size_t x) {
      return input_data[x];
    };

    kts::Reference1D<WeakTo> refOut = [&refIn](size_t x) {
      const WeakFrom in = refIn(x);
      return cargo::bit_cast<WeakTo>(in);
    };

    AddInputBuffer(elements, refIn);
    AddOutputBuffer(elements, refOut);

    // Round up 'elements / vector width' division
    const unsigned vec_width = getParam();
    const unsigned work_items = (elements + vec_width - 1) / vec_width;
    RunGeneric1D(work_items);
  }
};

class ReinterpretSingleTest : public Execution {
  void SetMacro(const std::string &in_type, const std::string &out_type,
                const std::string &in_vec_width,
                const std::string &out_vec_width) {
    AddMacro("IN_TYPE_SCALAR", in_type);
    AddMacro("OUT_TYPE_SCALAR", out_type);

    if (!out_vec_width.empty()) {
      AddMacro("STORE_FUNC", "vstore" + out_vec_width);
    }

    if (!in_vec_width.empty()) {
      AddMacro("LOAD_FUNC", "vload" + in_vec_width);
    }

    AddMacro("IN_TYPE_VECTOR", in_type + in_vec_width);
    const std::string out_type_vec(out_type + out_vec_width);
    AddMacro("OUT_TYPE_VECTOR", out_type_vec);
    AddMacro("AS_FUNC", "as_" + out_type_vec);
  }

 public:
  template <typename StrongFrom, typename StrongTo, unsigned vIn, unsigned vOut>
  void Run() {
    using WeakFrom = typename StrongFrom::WrappedT;
    using WeakTo = typename StrongTo::WrappedT;

    const bool uses_half =
        std::is_same_v<StrongFrom, CLhalf> || std::is_same_v<StrongTo, CLhalf>;
    if (uses_half && !UCL::hasHalfSupport(device)) {
      GTEST_SKIP();
    }

    const bool uses_double = std::is_same_v<StrongFrom, CLdouble> ||
                             std::is_same_v<StrongTo, CLdouble>;
    if (uses_double && !UCL::hasDoubleSupport(device)) {
      GTEST_SKIP();
    }

    const ucl::MathMode math_mode = ucl::Environment::instance->math_mode;
    const size_t in_elements = HalfInputSizes::getInputSize(math_mode);

    const unsigned work_items = in_elements / vIn;
    const unsigned out_elements = work_items * vOut;

    const std::string vIn_str = 1 == vIn ? "" : std::to_string(vIn);
    const std::string vOut_str = 1 == vOut ? "" : std::to_string(vOut);

    SetMacro(Stringify<StrongFrom>::as_str, Stringify<StrongTo>::as_str,
             vIn_str, vOut_str);

    std::vector<WeakFrom> input_data(in_elements);
    PopulateData(input_data);

    kts::Reference1D<WeakFrom> refIn = [&input_data](size_t x) {
      return input_data[x];
    };

    const WeakTo *ptr = reinterpret_cast<WeakTo *>(input_data.data());
    size_t index = 0;
    kts::Reference1D<WeakTo> refOut = [ptr, &index](size_t) {
      // When reinterpreting a vec4 as a vec3 the 4th element is ignored
      if (4 == vIn && 3 == vOut && (3 == (index % 4))) {
        index++;
      }

      const WeakTo in = ptr[index++];
      return cargo::bit_cast<WeakTo>(in);
    };

    AddInputBuffer(in_elements, refIn);
    AddOutputBuffer(out_elements, refOut);
    RunGeneric1D(work_items);
  }
};

using ConvertConfigTriple = std::tuple<unsigned, bool, RoundingMode>;
class ExplicitConvertTest : public ExecutionWithParam<ConvertConfigTriple> {
  void SetMacro(const std::string &in_type, const std::string &out_type,
                const RoundingMode rounding, const bool saturated,
                const unsigned vec_width) {
    AddMacro("IN_TYPE_SCALAR", in_type);
    AddMacro("OUT_TYPE_SCALAR", out_type);

    std::string convert_str("convert_");
    if (1 == vec_width) {
      AddMacro("IN_TYPE_VECTOR", in_type);
      AddMacro("OUT_TYPE_VECTOR", out_type);
      convert_str.append(out_type);
    } else {
      const std::string vec_str = std::to_string(vec_width);
      const std::string out_type_vec(out_type + vec_str);
      convert_str.append(out_type_vec);
      AddMacro("IN_TYPE_VECTOR", in_type + vec_str);
      AddMacro("OUT_TYPE_VECTOR", out_type_vec);
      AddMacro("STORE_FUNC", "vstore" + vec_str);
      AddMacro("LOAD_FUNC", "vload" + vec_str);
    }

    if (saturated) {
      convert_str.append("_sat");
    }

    switch (rounding) {
      case RoundingMode::RTE:
        convert_str.append("_rte");
        break;
      case RoundingMode::RTZ:
        convert_str.append("_rtz");
        break;
      case RoundingMode::RTP:
        convert_str.append("_rtp");
        break;
      case RoundingMode::RTN:
        convert_str.append("_rtn");
        break;
      default:
        break;
    }
    AddMacro("CONVERT_FUNC", convert_str);
  }

 public:
  template <typename StrongFrom, typename StrongTo>
  void Run() {
    using WeakFrom = typename StrongFrom::WrappedT;
    using WeakTo = typename StrongTo::WrappedT;

    const bool uses_half =
        std::is_same_v<StrongFrom, CLhalf> || std::is_same_v<StrongTo, CLhalf>;
    if (uses_half && !UCL::hasHalfSupport(device)) {
      GTEST_SKIP();
    }

    const bool uses_double = std::is_same_v<StrongFrom, CLdouble> ||
                             std::is_same_v<StrongTo, CLdouble>;
    if (uses_double && !UCL::hasDoubleSupport(device)) {
      GTEST_SKIP();
    }

    const unsigned vec_width = std::get<0>(getParam());
    const bool saturated = std::get<1>(getParam());
    const RoundingMode rounding = std::get<2>(getParam());

    SetMacro(Stringify<StrongFrom>::as_str, Stringify<StrongTo>::as_str,
             rounding, saturated, vec_width);

    const ucl::MathMode math_mode = ucl::Environment::instance->math_mode;
    size_t elements = HalfInputSizes::getInputSize(math_mode);

    const unsigned remainder = elements % vec_width;
    if (0 != remainder) {
      // Ensure vec3 types divide the number of buffer elements equally
      elements += vec_width - remainder;
    }
    const unsigned work_items = elements / vec_width;

    std::vector<WeakFrom> input_data(elements);
    PopulateData(input_data);

    kts::Reference1D<WeakFrom> refIn = [&input_data](size_t x) {
      return input_data[x];
    };
    AddInputBuffer(elements, refIn);

    // Use a templated helper function to make validating the various
    // type combinations is easier using specialization.
    const auto refOut = [&refIn, saturated, rounding](size_t x) -> WeakTo {
      const WeakFrom in = refIn(x);
      return ConvertRefHelper<StrongFrom, StrongTo>::reference(in, rounding,
                                                               saturated);
    };

    // Custom streamer for validation that we can specialize for half
    std::shared_ptr<ConvertStreamer<StrongTo>> out_streamer;

    // Checks for flush to zero behaviour of denormal inputs
    auto ftz_fallback = [&refIn, saturated, rounding](size_t x) -> WeakTo {
      const WeakFrom in = refIn(x);
      if (ConvertRefHelper<StrongFrom, StrongTo>::denormal(in)) {
        return WeakTo(0);
      }

      return ConvertRefHelper<StrongFrom, StrongTo>::reference(in, rounding,
                                                               saturated);
    };

    const bool denormSupport =
        UCL::hasDenormSupport(device, CL_DEVICE_HALF_FP_CONFIG);
    if (denormSupport) {
      out_streamer =
          std::make_shared<ConvertStreamer<StrongTo>>(refOut, device);
    } else {
      const std::vector<kts::Reference1D<WeakTo>> fallbacks{ftz_fallback};
      out_streamer = std::make_shared<ConvertStreamer<StrongTo>>(
          refOut, std::move(fallbacks), device);
    }

    // Some conversions can have undefined behaviour according to spec
    const auto undef_callback = [&refIn, saturated](size_t x) -> bool {
      const WeakFrom in = refIn(x);
      return ConvertRefHelper<StrongFrom, StrongTo>::undef(in, saturated);
    };

    out_streamer->SetUndefCallback(std::move(undef_callback));
    AddOutputBuffer(elements, out_streamer);

    RunGeneric1D(work_items);
  }
};
}  // namespace

using HalfToBoolConversions = HalfToGentypeConversions;
TEST_P(HalfToBoolConversions, Conversion_01_Implicit_Cast) { Run<CLbool>(); }
TEST_P(HalfToBoolConversions, Conversion_02_Explicit_Cast) { Run<CLbool>(); }
// Bool is a scalar type, so don't need to test across vector widths
UCL_EXECUTION_TEST_SUITE_P(HalfToBoolConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(std::array<unsigned, 1>({1})))

using HalfToCharConversions = HalfToGentypeConversions;
TEST_P(HalfToCharConversions, Conversion_01_Implicit_Cast) { Run<CLchar>(); }
TEST_P(HalfToCharConversions, Conversion_02_Explicit_Cast) { Run<CLchar>(); }
UCL_EXECUTION_TEST_SUITE_P(HalfToCharConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using HalfToUcharConversions = HalfToGentypeConversions;
TEST_P(HalfToUcharConversions, Conversion_01_Implicit_Cast) { Run<CLuchar>(); }
TEST_P(HalfToUcharConversions, Conversion_02_Explicit_Cast) { Run<CLuchar>(); }
UCL_EXECUTION_TEST_SUITE_P(HalfToUcharConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using HalfToShortConversions = HalfToGentypeConversions;
TEST_P(HalfToShortConversions, Conversion_01_Implicit_Cast) { Run<CLshort>(); }
TEST_P(HalfToShortConversions, Conversion_02_Explicit_Cast) { Run<CLshort>(); }
UCL_EXECUTION_TEST_SUITE_P(HalfToShortConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using HalfToUshortConversions = HalfToGentypeConversions;
TEST_P(HalfToUshortConversions, Conversion_01_Implicit_Cast) {
  Run<CLushort>();
}
TEST_P(HalfToUshortConversions, Conversion_02_Explicit_Cast) {
  Run<CLushort>();
}
UCL_EXECUTION_TEST_SUITE_P(HalfToUshortConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using HalfToIntConversions = HalfToGentypeConversions;
TEST_P(HalfToIntConversions, Conversion_01_Implicit_Cast) { Run<CLint>(); }
TEST_P(HalfToIntConversions, Conversion_02_Explicit_Cast) { Run<CLint>(); }
UCL_EXECUTION_TEST_SUITE_P(HalfToIntConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using HalfToUintConversions = HalfToGentypeConversions;
TEST_P(HalfToUintConversions, Conversion_01_Implicit_Cast) { Run<CLuint>(); }
TEST_P(HalfToUintConversions, Conversion_02_Explicit_Cast) { Run<CLuint>(); }
UCL_EXECUTION_TEST_SUITE_P(HalfToUintConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using HalfToLongConversions = HalfToGentypeConversions;
TEST_P(HalfToLongConversions, Conversion_01_Implicit_Cast) { Run<CLlong>(); }
TEST_P(HalfToLongConversions, Conversion_02_Explicit_Cast) { Run<CLlong>(); }
UCL_EXECUTION_TEST_SUITE_P(HalfToLongConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using HalfToUlongConversions = HalfToGentypeConversions;
TEST_P(HalfToUlongConversions, Conversion_01_Implicit_Cast) { Run<CLulong>(); }
TEST_P(HalfToUlongConversions, Conversion_02_Explicit_Cast) { Run<CLulong>(); }
UCL_EXECUTION_TEST_SUITE_P(HalfToUlongConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using HalfToFloatConversions = HalfToGentypeConversions;
TEST_P(HalfToFloatConversions, Conversion_01_Implicit_Cast) { Run<CLfloat>(); }
TEST_P(HalfToFloatConversions, Conversion_02_Explicit_Cast) { Run<CLfloat>(); }
UCL_EXECUTION_TEST_SUITE_P(HalfToFloatConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using HalfToDoubleConversions = HalfToGentypeConversions;
TEST_P(HalfToDoubleConversions, Conversion_01_Implicit_Cast) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }

  Run<CLdouble>();
}
TEST_P(HalfToDoubleConversions, Conversion_02_Explicit_Cast) {
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }
  Run<CLdouble>();
}
UCL_EXECUTION_TEST_SUITE_P(HalfToDoubleConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using BoolToHalfConversions = GentypeToHalfConversions;
TEST_P(BoolToHalfConversions, Conversion_01_Implicit_Cast) { Run<CLbool>(); }
TEST_P(BoolToHalfConversions, Conversion_02_Explicit_Cast) { Run<CLbool>(); }
UCL_EXECUTION_TEST_SUITE_P(BoolToHalfConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using CharToHalfConversions = GentypeToHalfConversions;
TEST_P(CharToHalfConversions, Conversion_01_Implicit_Cast) { Run<CLchar>(); }
TEST_P(CharToHalfConversions, Conversion_02_Explicit_Cast) { Run<CLchar>(); }
UCL_EXECUTION_TEST_SUITE_P(CharToHalfConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using UcharToHalfConversions = GentypeToHalfConversions;
TEST_P(UcharToHalfConversions, Conversion_01_Implicit_Cast) { Run<CLuchar>(); }
TEST_P(UcharToHalfConversions, Conversion_02_Explicit_Cast) { Run<CLuchar>(); }
UCL_EXECUTION_TEST_SUITE_P(UcharToHalfConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using ShortToHalfConversions = GentypeToHalfConversions;
TEST_P(ShortToHalfConversions, Conversion_01_Implicit_Cast) { Run<CLshort>(); }
TEST_P(ShortToHalfConversions, Conversion_02_Explicit_Cast) { Run<CLshort>(); }
UCL_EXECUTION_TEST_SUITE_P(ShortToHalfConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using UshortToHalfConversions = GentypeToHalfConversions;
TEST_P(UshortToHalfConversions, Conversion_01_Implicit_Cast) {
  Run<CLushort>();
}
TEST_P(UshortToHalfConversions, Conversion_02_Explicit_Cast) {
  Run<CLushort>();
}
UCL_EXECUTION_TEST_SUITE_P(UshortToHalfConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using IntToHalfConversions = GentypeToHalfConversions;
TEST_P(IntToHalfConversions, Conversion_01_Implicit_Cast) { Run<CLint>(); }
TEST_P(IntToHalfConversions, Conversion_02_Explicit_Cast) { Run<CLint>(); }
UCL_EXECUTION_TEST_SUITE_P(IntToHalfConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using UintToHalfConversions = GentypeToHalfConversions;
TEST_P(UintToHalfConversions, Conversion_01_Implicit_Cast) { Run<CLuint>(); }
TEST_P(UintToHalfConversions, Conversion_02_Explicit_Cast) { Run<CLuint>(); }
UCL_EXECUTION_TEST_SUITE_P(UintToHalfConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using LongToHalfConversions = GentypeToHalfConversions;
TEST_P(LongToHalfConversions, Conversion_01_Implicit_Cast) { Run<CLlong>(); }
TEST_P(LongToHalfConversions, Conversion_02_Explicit_Cast) { Run<CLlong>(); }
UCL_EXECUTION_TEST_SUITE_P(LongToHalfConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using UlongToHalfConversions = GentypeToHalfConversions;
TEST_P(UlongToHalfConversions, Conversion_01_Implicit_Cast) { Run<CLulong>(); }
TEST_P(UlongToHalfConversions, Conversion_02_Explicit_Cast) { Run<CLulong>(); }
UCL_EXECUTION_TEST_SUITE_P(UlongToHalfConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using FloatToHalfConversions = GentypeToHalfConversions;
TEST_P(FloatToHalfConversions, Conversion_01_Implicit_Cast) { Run<CLfloat>(); }
TEST_P(FloatToHalfConversions, Conversion_02_Explicit_Cast) { Run<CLfloat>(); }
UCL_EXECUTION_TEST_SUITE_P(FloatToHalfConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using DoubleToHalfConversions = GentypeToHalfConversions;
TEST_P(DoubleToHalfConversions, Conversion_01_Implicit_Cast) {
#ifdef __arm__
  // TODO CA-2654: This test causes 32-bit Arm Qemu to infinite loop.
  GTEST_SKIP();
#endif
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }
  Run<CLdouble>();
}

TEST_P(DoubleToHalfConversions, Conversion_02_Explicit_Cast) {
#ifdef __arm__
  // TODO CA-2654: This test causes 32-bit Arm Qemu to infinite loop.
  GTEST_SKIP();
#endif
  if (!UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }
  Run<CLdouble>();
}
UCL_EXECUTION_TEST_SUITE_P(DoubleToHalfConversions, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using HalfToShortReinterpret = ReinterpretAllVecWidthsTest;
TEST_P(HalfToShortReinterpret, Conversion_03_Reinterpret) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLhalf, CLshort>();
}
UCL_EXECUTION_TEST_SUITE_P(HalfToShortReinterpret, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using HalfToUshortReinterpret = ReinterpretAllVecWidthsTest;
TEST_P(HalfToUshortReinterpret, Conversion_03_Reinterpret) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLhalf, CLushort>();
}
UCL_EXECUTION_TEST_SUITE_P(HalfToUshortReinterpret, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using ShortToHalfReinterpret = ReinterpretAllVecWidthsTest;
TEST_P(ShortToHalfReinterpret, Conversion_03_Reinterpret) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLshort, CLhalf>();
}
UCL_EXECUTION_TEST_SUITE_P(ShortToHalfReinterpret, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using UshortToHalfReinterpret = ReinterpretAllVecWidthsTest;
TEST_P(UshortToHalfReinterpret, Conversion_03_Reinterpret) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLushort, CLhalf>();
}
UCL_EXECUTION_TEST_SUITE_P(UshortToHalfReinterpret, testing::Values(OPENCL_C),
                           testing::ValuesIn(vector_widths))

using Half4toHalf3Reinterpret = ReinterpretSingleTest;
TEST_P(Half4toHalf3Reinterpret, Conversion_03_Reinterpret) {
  Run<CLhalf, CLhalf, 4, 3>();
}
UCL_EXECUTION_TEST_SUITE(Half4toHalf3Reinterpret, testing::Values(OPENCL_C))

using Half4toShort3Reinterpret = ReinterpretSingleTest;
TEST_P(Half4toShort3Reinterpret, Conversion_03_Reinterpret) {
  Run<CLhalf, CLshort, 4, 3>();
}
UCL_EXECUTION_TEST_SUITE(Half4toShort3Reinterpret, testing::Values(OPENCL_C))

using Half4toUshort3Reinterpret = ReinterpretSingleTest;
TEST_P(Half4toUshort3Reinterpret, Conversion_03_Reinterpret) {
  Run<CLhalf, CLushort, 4, 3>();
}
UCL_EXECUTION_TEST_SUITE(Half4toUshort3Reinterpret, testing::Values(OPENCL_C))

using Short4toHalf3Reinterpret = ReinterpretSingleTest;
TEST_P(Short4toHalf3Reinterpret, Conversion_03_Reinterpret) {
  Run<CLshort, CLhalf, 4, 3>();
}
UCL_EXECUTION_TEST_SUITE(Short4toHalf3Reinterpret, testing::Values(OPENCL_C))

using Ushort4toHalf3Reinterpret = ReinterpretSingleTest;
TEST_P(Ushort4toHalf3Reinterpret, Conversion_03_Reinterpret) {
  Run<CLushort, CLhalf, 4, 3>();
}
UCL_EXECUTION_TEST_SUITE(Ushort4toHalf3Reinterpret, testing::Values(OPENCL_C))

using IntToHalf2Reinterpret = ReinterpretSingleTest;
TEST_P(IntToHalf2Reinterpret, Conversion_03_Reinterpret) {
  Run<CLint, CLhalf, 1, 2>();
}
UCL_EXECUTION_TEST_SUITE(IntToHalf2Reinterpret, testing::Values(OPENCL_C))

using Half2ToIntReinterpret = ReinterpretSingleTest;
TEST_P(Half2ToIntReinterpret, Conversion_03_Reinterpret) {
  Run<CLhalf, CLint, 2, 1>();
}
UCL_EXECUTION_TEST_SUITE(Half2ToIntReinterpret, testing::Values(OPENCL_C))

using UintToHalf2Reinterpret = ReinterpretSingleTest;
TEST_P(UintToHalf2Reinterpret, Conversion_03_Reinterpret) {
  Run<CLuint, CLhalf, 1, 2>();
}
UCL_EXECUTION_TEST_SUITE(UintToHalf2Reinterpret, testing::Values(OPENCL_C))

using Half2ToUintReinterpret = ReinterpretSingleTest;
TEST_P(Half2ToUintReinterpret, Conversion_03_Reinterpret) {
  Run<CLhalf, CLuint, 2, 1>();
}
UCL_EXECUTION_TEST_SUITE(Half2ToUintReinterpret, testing::Values(OPENCL_C))

using FloatToHalf2Reinterpret = ReinterpretSingleTest;
TEST_P(FloatToHalf2Reinterpret, Conversion_03_Reinterpret) {
  Run<CLfloat, CLhalf, 1, 2>();
}
UCL_EXECUTION_TEST_SUITE(FloatToHalf2Reinterpret, testing::Values(OPENCL_C))

using Half2ToFloatReinterpret = ReinterpretSingleTest;
TEST_P(Half2ToFloatReinterpret, Conversion_03_Reinterpret) {
  Run<CLhalf, CLfloat, 2, 1>();
}
UCL_EXECUTION_TEST_SUITE(Half2ToFloatReinterpret, testing::Values(OPENCL_C))

using LongToHalf4Reinterpret = ReinterpretSingleTest;
TEST_P(LongToHalf4Reinterpret, Conversion_03_Reinterpret) {
  Run<CLlong, CLhalf, 1, 4>();
}
UCL_EXECUTION_TEST_SUITE(LongToHalf4Reinterpret, testing::Values(OPENCL_C))

using Half4ToLongReinterpret = ReinterpretSingleTest;
TEST_P(Half4ToLongReinterpret, Conversion_03_Reinterpret) {
  Run<CLhalf, CLlong, 4, 1>();
}
UCL_EXECUTION_TEST_SUITE(Half4ToLongReinterpret, testing::Values(OPENCL_C))

using UlongToHalf4Reinterpret = ReinterpretSingleTest;
TEST_P(UlongToHalf4Reinterpret, Conversion_03_Reinterpret) {
  Run<CLulong, CLhalf, 1, 4>();
}
UCL_EXECUTION_TEST_SUITE(UlongToHalf4Reinterpret, testing::Values(OPENCL_C))

using Half4ToUlongReinterpret = ReinterpretSingleTest;
TEST_P(Half4ToUlongReinterpret, Conversion_03_Reinterpret) {
  Run<CLhalf, CLulong, 4, 1>();
}
UCL_EXECUTION_TEST_SUITE(Half4ToUlongReinterpret, testing::Values(OPENCL_C))

using DoubleToHalf4Reinterpret = ReinterpretSingleTest;
TEST_P(DoubleToHalf4Reinterpret, Conversion_03_Reinterpret) {
  Run<CLdouble, CLhalf, 1, 4>();
}
UCL_EXECUTION_TEST_SUITE(DoubleToHalf4Reinterpret, testing::Values(OPENCL_C))

using Half4ToDoubleReinterpret = ReinterpretSingleTest;
TEST_P(Half4ToDoubleReinterpret, Conversion_03_Reinterpret) {
  Run<CLhalf, CLdouble, 4, 1>();
}
UCL_EXECUTION_TEST_SUITE(Half4ToDoubleReinterpret, testing::Values(OPENCL_C))

using HalfToCharExplicitConvert = ExplicitConvertTest;
TEST_P(HalfToCharExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLhalf, CLchar>();
}
UCL_EXECUTION_TEST_SUITE_P(HalfToCharExplicitConvert, testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using CharToHalfExplicitConvert = ExplicitConvertTest;
TEST_P(CharToHalfExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLchar, CLhalf>();
}
UCL_EXECUTION_TEST_SUITE_P(CharToHalfExplicitConvert, testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using HalfToUcharExplicitConvert = ExplicitConvertTest;
TEST_P(HalfToUcharExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLhalf, CLuchar>();
}
UCL_EXECUTION_TEST_SUITE_P(HalfToUcharExplicitConvert,
                           testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using UcharToHalfExplicitConvert = ExplicitConvertTest;
TEST_P(UcharToHalfExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLuchar, CLhalf>();
}
UCL_EXECUTION_TEST_SUITE_P(UcharToHalfExplicitConvert,
                           testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using HalfToShortExplicitConvert = ExplicitConvertTest;
TEST_P(HalfToShortExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLhalf, CLshort>();
}
UCL_EXECUTION_TEST_SUITE_P(HalfToShortExplicitConvert,
                           testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using ShortToHalfExplicitConvert = ExplicitConvertTest;
TEST_P(ShortToHalfExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLshort, CLhalf>();
}
UCL_EXECUTION_TEST_SUITE_P(ShortToHalfExplicitConvert,
                           testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using HalfToUshortExplicitConvert = ExplicitConvertTest;
TEST_P(HalfToUshortExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLhalf, CLushort>();
}
UCL_EXECUTION_TEST_SUITE_P(HalfToUshortExplicitConvert,
                           testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using UshortToHalfExplicitConvert = ExplicitConvertTest;
TEST_P(UshortToHalfExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLushort, CLhalf>();
}
UCL_EXECUTION_TEST_SUITE_P(UshortToHalfExplicitConvert,
                           testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using HalfToIntExplicitConvert = ExplicitConvertTest;
TEST_P(HalfToIntExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLhalf, CLint>();
}
UCL_EXECUTION_TEST_SUITE_P(HalfToIntExplicitConvert, testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using IntToHalfExplicitConvert = ExplicitConvertTest;
TEST_P(IntToHalfExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLint, CLhalf>();
}
UCL_EXECUTION_TEST_SUITE_P(IntToHalfExplicitConvert, testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using HalfToUintExplicitConvert = ExplicitConvertTest;
TEST_P(HalfToUintExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLhalf, CLuint>();
}
UCL_EXECUTION_TEST_SUITE_P(HalfToUintExplicitConvert, testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using UintToHalfExplicitConvert = ExplicitConvertTest;
TEST_P(UintToHalfExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLuint, CLhalf>();
}
UCL_EXECUTION_TEST_SUITE_P(UintToHalfExplicitConvert, testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using HalfToLongExplicitConvert = ExplicitConvertTest;
TEST_P(HalfToLongExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLhalf, CLlong>();
}
UCL_EXECUTION_TEST_SUITE_P(HalfToLongExplicitConvert, testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using LongToHalfExplicitConvert = ExplicitConvertTest;
TEST_P(LongToHalfExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLlong, CLhalf>();
}
UCL_EXECUTION_TEST_SUITE_P(LongToHalfExplicitConvert, testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using HalfToUlongExplicitConvert = ExplicitConvertTest;
TEST_P(HalfToUlongExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLhalf, CLulong>();
}
UCL_EXECUTION_TEST_SUITE_P(HalfToUlongExplicitConvert,
                           testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using UlongToHalfExplicitConvert = ExplicitConvertTest;
TEST_P(UlongToHalfExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLulong, CLhalf>();
}
UCL_EXECUTION_TEST_SUITE_P(UlongToHalfExplicitConvert,
                           testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using HalfToHalfExplicitConvert = ExplicitConvertTest;
TEST_P(HalfToHalfExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLhalf, CLhalf>();
}
UCL_EXECUTION_TEST_SUITE_P(HalfToHalfExplicitConvert, testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using HalfToFloatExplicitConvert = ExplicitConvertTest;
TEST_P(HalfToFloatExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLhalf, CLfloat>();
}
UCL_EXECUTION_TEST_SUITE_P(HalfToFloatExplicitConvert,
                           testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using FloatToHalfExplicitConvert = ExplicitConvertTest;
TEST_P(FloatToHalfExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLfloat, CLhalf>();
}
UCL_EXECUTION_TEST_SUITE_P(FloatToHalfExplicitConvert,
                           testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using HalfToDoubleExplicitConvert = ExplicitConvertTest;
TEST_P(HalfToDoubleExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

  Run<CLhalf, CLdouble>();
}
UCL_EXECUTION_TEST_SUITE_P(HalfToDoubleExplicitConvert,
                           testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))

using DoubleToHalfExplicitConvert = ExplicitConvertTest;
TEST_P(DoubleToHalfExplicitConvert, Conversion_04_Explicit_Convert) {
  // TODO: Investigate why this test doesn't vectorize (CA-4552).
  fail_if_not_vectorized_ = false;

#ifdef __arm__
  // TODO(CA-2654): This test causes 32-bit Arm Qemu to infinite loop.
  GTEST_SKIP();
#endif
  Run<CLdouble, CLhalf>();
}
UCL_EXECUTION_TEST_SUITE_P(DoubleToHalfExplicitConvert,
                           testing::Values(OPENCL_C),
                           testing::Combine(testing::ValuesIn(vector_widths),
                                            testing::ValuesIn(sat),
                                            testing::ValuesIn(roundings)))
