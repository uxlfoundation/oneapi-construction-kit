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

#include <gtest/gtest.h>

#include "Common.h"
#include "kts/precision.h"

// This is needed for 'TEST_F'.
using namespace kts::ucl;

namespace {
struct AbsoluteErrValidator final {
  AbsoluteErrValidator(cl_device_id device)
      : device(device), previous(0.0f), previous_set(false) {}

  bool validate(const cl_float &expected, const cl_half &actual,
                cl_float err_threshold) {
    const bool denormSupport =
        UCL::hasDenormSupport(device, CL_DEVICE_HALF_FP_CONFIG);
    if (!denormSupport && IsDenormalAsHalf(expected)) {
      // Accept +/- 0.0 if denormals aren't supported and result was a denormal
      const cl_ushort asInt = matchingType(actual);
      if (0 == asInt || 0x8000 == asInt) {
        return true;
      }
    }

    const cl_half ref_as_half = ConvertFloatToHalf(expected);
    if (ref_as_half == actual) {
      return true;
    }

    const cl_float result_as_float = ConvertHalfToFloat(actual);
    if (std::isnan(expected) && std::isnan(result_as_float)) {
      return true;
    }

    if (err_threshold >= TypeInfo<cl_half>::max) {
      // An intermediate value will overflow
      return true;
    } else if (err_threshold < TypeInfo<cl_half>::lowest) {
      // Error tolerance can't be below smallest representable half
      err_threshold = TypeInfo<cl_half>::lowest;
    }

    // 32-bit reference saturated to half precision
    const cl_float ref_saturated = ConvertHalfToFloat(ref_as_half);
    // Absolute error
    const cl_float error = std::fabs(ref_saturated - result_as_float);
    return !(error > err_threshold);
  }

  void print(std::stringstream &s, const cl_half value) {
    s << "[half] 0x" << std::hex << matchingType(value);
    const cl_float as_float = ConvertHalfToFloat(value);
    s << ", as float 0x" << matchingType(as_float) << std::dec;

    if (previous_set) {
      s << ", absolute error: " << std::fabs(previous - as_float);
    }
  }

  void print(std::stringstream &s, const cl_float value) {
    s << "[float] 0x" << std::hex << matchingType(value) << std::dec;
    const cl_half as_half = ConvertFloatToHalf(value);
    s << ", as half 0x" << std::hex << as_half << std::dec;
    if (!previous_set) {
      previous_set = true;
      previous = value;
    }
  }

 private:
  cl_device_id device;
  cl_float previous;
  bool previous_set;
};

struct ULPErrValidator final {
  ULPErrValidator(cl_device_id) : previous(0.0), previous_set(false) {}

  bool validate(const cl_float &expected, const cl_half &actual,
                const cl_float ulp_threshold) {
    const cl_float ulp = calcHalfPrecisionULP(expected, actual);
    return std::fabs(ulp) <= std::fabs(ulp_threshold);
  }

  void print(std::stringstream &s, const cl_half value) {
    const cl_float as_float = ConvertHalfToFloat(value);
    s << "half " << as_float << "[0x" << std::hex << value << "]" << std::dec;

    if (previous_set) {
      s << ", ulp: " << calcHalfPrecisionULP(previous, value);
    }
  }

  void print(std::stringstream &s, const cl_float value) {
    const cl_half as_half = ConvertFloatToHalf(value);
    s << value << " (half " << ConvertHalfToFloat(as_half) << "[0x" << std::hex
      << as_half << std::dec << "])";
    if (!previous_set) {
      previous_set = true;
      previous = value;
    }
  }

 private:
  cl_float previous;
  bool previous_set;
};

template <class V>
struct GeometricStreamer final : public kts::GenericStreamer<cl_float> {
  GeometricStreamer(kts::Reference1D<cl_float> ref,
                    std::function<cl_float(size_t)> e, V validator)
      : kts::GenericStreamer<cl_float>(ref),
        error_callback(e),
        validator(validator) {}
  GeometricStreamer(kts::Reference1D<cl_float> ref,
                    std::function<cl_float(size_t)> e,
                    const std::vector<kts::Reference1D<float>> &&f, V validator)
      : kts::GenericStreamer<cl_float>(ref, std::forward<decltype(f)>(f)),
        error_callback(e),
        validator(validator) {}

  bool ValidateBuffer(kts::ArgumentBase &arg, const kts::BufferDesc &desc,
                      std::vector<std::string> *errors) override {
    if ((arg.GetKind() != kts::eOutputBuffer) &&
        (arg.GetKind() != kts::eInOutBuffer)) {
      return true;
    }

    kts::MemoryAccessor<cl_half> accessor;
    for (size_t j = 0; j < desc.size_; j++) {
      const cl_half actual =
          accessor.LoadFromBuffer(arg.GetBufferStoragePtr(), j);
      const cl_float expected = this->ref_(j);
      const cl_float error_tolerance = error_callback(j);
      if (!validator.validate(expected, actual, error_tolerance)) {
        // Try verifying against fallback references
        if (std::any_of(this->fallbacks_.begin(), this->fallbacks_.end(),
                        [this, actual, j,
                         error_tolerance](const kts::Reference1D<cl_float> &r) {
                          const cl_float fallback = r(j);
                          return validator.validate(fallback, actual,
                                                    error_tolerance);
                        })) {
          continue;
        }

        if (this->CheckIfUndef(j)) {
          // Result is undefined at this index, skip
          continue;
        }

        if (errors) {
          std::stringstream ss;
          ss << "Result mismatch at index 0x" << std::hex << j << std::dec;
          ss << " (expected: ";
          validator.print(ss, expected);
          ss << ", actual: ";
          validator.print(ss, actual);
          ss << ")";
          errors->push_back(ss.str());
        }
        return false;
      }
    }
    return true;
  }

  std::function<cl_float(size_t)> error_callback;
  V validator;
};

using AbsoluteErrStreamer = GeometricStreamer<AbsoluteErrValidator>;
using ULPErrStreamer = GeometricStreamer<ULPErrValidator>;

// We need specific behaviour when generating input data for the geometric class
// of tests to match the CTS which avoids denormal values
struct GeometricParamExecution : public HalfParamExecution {
  template <size_t N>
  unsigned GeometricFillBuffers(std::array<input_details_t, N> &inputs);
};

template <size_t N>
unsigned GeometricParamExecution::GeometricFillBuffers(
    std::array<input_details_t, N> &inputs) {
  auto env = ucl::Environment::instance;
  const ucl::MathMode math_mode = env->math_mode;

  unsigned length = HalfInputSizes::getInputSize(math_mode);
  if (ucl::MathMode::FULL == math_mode) {
    // Buffer size for thorough testing, has to be enabled by user
    const unsigned num_inputs_multiplier = N * N;
    length *= num_inputs_multiplier;
  }

  // Ensure work items divide number of buffer elements equally
  const unsigned vec_width = this->getParam();
  const unsigned remainder = length % vec_width;
  if (0 != remainder) {
    length += vec_width - remainder;
  }

  // The CTS Geometric test 32-bit float input generator `get_random_float`
  // clamps the input values for 32-bit floats to [-512.0f, 512.0f], and avoids
  // generating denormal inputs.
  //
  // From test_common/harness/conversions.c
  // ```c
  //   float get_random_float(float low, float high, MTdata d)
  //   {
  //     float t = (float)((double)genrand_int32(d) / (double)0xFFFFFFFF);
  //     return (1.0f - t) * low + t * high;
  //   }
  // ```
  //
  // We replicate this test input range for halfs
  const auto geom_generator = [](cl_half x) -> cl_half {
    // bitcast half value to 16-bit ushort
    const cl_ushort bitcast_int = cargo::bit_cast<cl_ushort>(x);

    // 65535(0xFFF) Max value of a cl_ushort
    const cl_float ushort_max =
        static_cast<cl_float>(std::numeric_limits<cl_ushort>::max());

    // Bring into range [0.0, 1.0]
    const cl_float normalized = bitcast_int / ushort_max;

    // Low and high limits from CTS geometric calls to `get_random_float()`
    // If the number is too high or low the value can overflow as these
    // operations generally involve a multiply. Once we overflow in an
    // intermediate calculation we start passing either NAN or INF through the
    // rest of the operations. 32 is the lowest power of 2 found to work on
    // FTZ hardware.
    const cl_float low = -32.0f;
    const cl_float high = 32.0f;

    const cl_float clamped = ((1.0f - normalized) * low) + (normalized * high);
    return ConvertFloatToHalf(clamped);
  };

  for (input_details_t &input : inputs) {
    // Populate buffer with cl_halfs we will bitcast to cl_ushort
    std::vector<cl_half> &buffer = input.data;
    buffer.resize(length);
    env->GetInputGenerator().GenerateIntData(buffer);

    // Apply transform to bring into cl_half into our desired range
    std::transform(buffer.begin(), buffer.end(), buffer.begin(),
                   geom_generator);

    // Populate OpenCL input buffers with the data we've just set
    this->AddInputBuffer(
        length,
        kts::Reference1D<cl_half>([&buffer](size_t id) { return buffer[id]; }));
  }
  return length;
}
}  // namespace

using HalfGeometricBuiltins = GeometricParamExecution;
TEST_P(HalfGeometricBuiltins, Geometric_01_Half_Dot) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }
  fail_if_not_vectorized_ = false;

  const unsigned vec_width = this->SetMacro();

  // Populate input buffers
  std::array<input_details_t, 2> inputs{{0, 1}};
  const unsigned N = GeometricFillBuffers(inputs);
  const unsigned work_items = N / vec_width;

  // Use single precision host float for reference
  const input_details_t &ref_A = inputs[0];
  const input_details_t &ref_B = inputs[1];

  const auto ref_lambda = [&ref_A, &ref_B, vec_width](size_t id) -> cl_float {
    cl_float sum = 0.0f;

    for (unsigned i = 0; i < vec_width; i++) {
      const size_t offset = (id * vec_width) + i;
      const cl_half half_A = ref_A(offset);
      const cl_half half_B = ref_B(offset);

      const cl_float float_A = ConvertHalfToFloat(half_A);
      const cl_float float_B = ConvertHalfToFloat(half_B);
      sum += float_A * float_B;
    }
    return sum;
  };

  // Function used to determine absolute error threshold, which is dependent on
  // operation inputs.
  const auto err_callback = [&ref_A, ref_B, vec_width](size_t id) -> cl_float {
    // Find input value with largest magnitude
    cl_float max_value = 0.0f;
    for (size_t i = 0; i < vec_width; i++) {
      const size_t offset = (id * vec_width) + i;
      const cl_float a = ConvertHalfToFloat(ref_A(offset));
      const cl_float b = ConvertHalfToFloat(ref_B(offset));
      max_value = std::fmax(std::fabs(a), std::fmax(std::fabs(b), max_value));
    }

    // CTS uses an absolute error tolerance for 32-bit float dot validation, so
    // use the same here method
    const cl_float err_tolerance =
        max_value * max_value * cl_float((2 * vec_width) - 1);

    // Check for overflow in tolerance calculation which should be done in
    // same precision as we're testing, but fp16 isn't available on host.
    if (err_tolerance > TypeInfo<cl_half>::max) {
      return TypeInfo<cl_half>::max;
    }

    // HLF_EPSILON macro from fp16 extension spec
    const cl_float half_epsilon = 0.0009765625f;  // 2^-10
    return err_tolerance * half_epsilon;
  };

  const auto ref_streamer =
      std::make_shared<AbsoluteErrStreamer>(ref_lambda, err_callback, device);
  this->AddOutputBuffer(work_items, ref_streamer);
  this->RunGeneric1D(work_items);
}

TEST_P(HalfGeometricBuiltins, Geometric_02_Half_Length) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }
  fail_if_not_vectorized_ = false;

  const unsigned vec_width = this->SetMacro();

  // Populate input buffers
  std::array<input_details_t, 1> input{{0}};
  const unsigned N = GeometricFillBuffers(input);
  const unsigned work_items = N / vec_width;

  // Use single precision host float for reference
  const input_details_t &ref_buffer = input[0];
  const auto ref_lambda = [&ref_buffer, vec_width](size_t id) -> cl_float {
    cl_float sum = 0.0f;
    for (unsigned i = 0; i < vec_width; i++) {
      const size_t offset = (id * vec_width) + i;
      const cl_half half_val = ref_buffer(offset);
      const cl_float as_float = ConvertHalfToFloat(half_val);

      sum += as_float * as_float;
    }
    return std::sqrt(sum);
  };

  // Function used to determine ULP error threshold, which is dependent on
  // vectorization width
  const auto err_callback = [vec_width](size_t) -> cl_float {
    cl_float max_ulp = 0.5f;  // error in sqrt(correctly rounded)

    max_ulp +=
        0.5f *                         // effect on e of taking sqrt( x + e )
        (0.5f * cl_float(vec_width) +  // cumulative error for multiplications
         0.5f * cl_float(vec_width - 1));  // cumulative error for additions

    return max_ulp;
  };

  const auto ref_streamer =
      std::make_shared<ULPErrStreamer>(ref_lambda, err_callback, device);
  this->AddOutputBuffer(work_items, ref_streamer);
  this->RunGeneric1D(work_items);
}

TEST_P(HalfGeometricBuiltins, Geometric_03_Half_Distance) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }
  fail_if_not_vectorized_ = false;

  const unsigned vec_width = this->SetMacro();

  // Populate input buffers
  std::array<input_details_t, 2> inputs{{0, 1}};
  const unsigned N = GeometricFillBuffers(inputs);
  const unsigned work_items = N / vec_width;

  // Use single precision host float for reference
  const input_details_t &ref_A = inputs[0];
  const input_details_t &ref_B = inputs[1];

  const auto ref_lambda = [&ref_A, &ref_B, vec_width](size_t id) -> cl_float {
    cl_float sum = 0.0f;
    for (unsigned i = 0; i < vec_width; i++) {
      const size_t offset = (id * vec_width) + i;
      const cl_half half_A = ref_A(offset);
      const cl_half half_B = ref_B(offset);

      const cl_float float_A = ConvertHalfToFloat(half_A);
      const cl_float float_B = ConvertHalfToFloat(half_B);

      const cl_float subtract = float_A - float_B;
      sum += subtract * subtract;
    }
    return std::sqrt(sum);
  };

  // Function used to determine ULP error threshold, which is dependent on
  // vectorization width
  const auto err_callback = [vec_width](size_t) -> cl_float {
    cl_float max_ulp = 0.5f;  // error in sqrt

    // Cumulative error for multiplications and additions
    max_ulp += (1.5f * cl_float(vec_width) + 0.5f * cl_float(vec_width - 1));
    return max_ulp;
  };

  const auto ref_streamer =
      std::make_shared<ULPErrStreamer>(ref_lambda, err_callback, device);
  this->AddOutputBuffer(work_items, ref_streamer);
  this->RunGeneric1D(work_items);
}

TEST_P(HalfGeometricBuiltins, Geometric_04_Half_Normalize) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  fail_if_not_vectorized_ = false;

  const unsigned vec_width = this->SetMacro();

  // Populate input buffer, normalize doesn't clamp input range
  std::array<input_details_t, 1> input{{0}};
  const unsigned N = FillInputBuffers(input);

  // Use single precision host float for reference
  const input_details_t &ref_buffer = input[0];
  const auto ref_lambda = [&ref_buffer, vec_width](size_t id) -> cl_float {
    const size_t vector_group = id / vec_width;
    cl_float input = ConvertHalfToFloat(ref_buffer(id));

    // We need to check if ALL of the fields are zero as in that case we need to
    // return the input.
    // We need to check if ANY of the fields are INF as that changes the
    // behaviour of the test. The spec says that if any vector lanes are INF all
    // INF fields should be flushed to 1 and all others to 0.
    bool all_zero = true;
    bool has_inf = false;
    for (unsigned i = 0; i < vec_width; i++) {
      const size_t offset = (vector_group * vec_width) + i;
      const cl_float as_float = ConvertHalfToFloat(ref_buffer(offset));
      if (as_float != 0) {
        all_zero = false;
      }
      if (std::isinf(as_float)) {
        has_inf = true;
      }
    }

    if (all_zero) {
      return input;
    }

    cl_double dot = 0.0f;
    for (unsigned i = 0; i < vec_width; i++) {
      const size_t offset = (vector_group * vec_width) + i;
      const cl_float as_float = ConvertHalfToFloat(ref_buffer(offset));

      if (has_inf) {
        // Flush INF lane to 1.0 if any lane is INF. From spec:
        //   `v[i] = isinf(v[i])  ?  copysign(1.0, v[i]) : 0.0 * v[i]`
        if (std::isinf(as_float)) {
          dot += 1.0f;
        } else if (std::isnan(as_float)) {
          dot += NAN;
        }
      } else {
        dot += as_float * as_float;
      }
    }

    const cl_float rsqrt_dot = 1.0f / std::sqrt(dot);
    if (std::isinf(input)) {
      return std::copysign(rsqrt_dot, input);
    } else if (has_inf) {
      input = 0.0f * input;  // evaluates to 0.0f or -0.0f or NAN
    }
    return input * rsqrt_dot;
  };

  // Function used to determine ULP error threshold, which is dependent on
  // vectorization width
  const auto err_callback = [vec_width](size_t) -> cl_float {
    cl_float max_ulp = 1.5f;  // error in rsqrt + error in multiply

    // Cumulative error for multiplications and additions
    max_ulp += (0.5f * cl_float(vec_width) + 0.5f * cl_float(vec_width - 1));
    return max_ulp;
  };

  // Turns denormals into zero and propagates them throughout the reference
  const auto ftz_lambda = [&ref_buffer, vec_width](size_t id) -> cl_float {
    const size_t vector_group = id / vec_width;

    cl_float dot = 0.0f;
    for (unsigned i = 0; i < vec_width; i++) {
      const size_t offset = (vector_group * vec_width) + i;

      const cl_half half_val = ref_buffer(offset);

      const cl_float as_float = ConvertHalfToFloat(half_val);
      if (IsDenormalAsHalf(as_float)) {
        continue;
      }

      dot += as_float * as_float;
    }

    // CTS tests handle this case, but it's not in the spec anywhere
    if (std::isinf(dot)) {
      return NAN;
    }

    const cl_float sqrt_dot = std::sqrt(dot);
    const cl_float rsqrt_dot = 1.0f / sqrt_dot;

    const cl_half half_input = ref_buffer(id);
    const cl_float input = ConvertHalfToFloat(half_input);
    const cl_float result = input * rsqrt_dot;

    if (IsDenormalAsHalf(input) || IsDenormalAsHalf(sqrt_dot) ||
        IsDenormalAsHalf(rsqrt_dot) || IsDenormalAsHalf(result)) {
      return 0.0f;
    }

    return result;
  };

  // FTZ fallback reference if device doesn't support denormals
  // Normalize is the only CTS geometric single-precision test to check FTZ
  const bool denormSupport =
      UCL::hasDenormSupport(device, CL_DEVICE_HALF_FP_CONFIG);
  if (denormSupport) {
    auto ref_streamer =
        std::make_shared<ULPErrStreamer>(ref_lambda, err_callback, device);
    this->AddOutputBuffer(N, ref_streamer);
  } else {
    const std::vector<kts::Reference1D<cl_float>> fallbacks{ftz_lambda};
    auto ref_streamer = std::make_shared<ULPErrStreamer>(
        ref_lambda, err_callback, std::move(fallbacks), device);
    this->AddOutputBuffer(N, ref_streamer);
  }

  const unsigned work_items = N / vec_width;
  this->RunGeneric1D(work_items);
}

// No vector widths 8 or 16 defined for geometrics
UCL_EXECUTION_TEST_SUITE_P(HalfGeometricBuiltins, testing::Values(OPENCL_C),
                           testing::Values(1, 2, 3, 4))

using HalfGeometricCross = GeometricParamExecution;
TEST_P(HalfGeometricCross, Geometric_05_Half_Cross) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }
  fail_if_not_vectorized_ = false;

  const unsigned vec_width = this->SetMacro();

  // Populate input buffer
  std::array<input_details_t, 2> inputs{{0, 1}};
  const unsigned N = GeometricFillBuffers(inputs);

  // Use single precision host float for reference
  const input_details_t &ref_A = inputs[0];
  const input_details_t &ref_B = inputs[1];

  const auto ref_lambda = [&ref_A, &ref_B, &vec_width](size_t id) -> cl_float {
    const size_t offset = id % vec_width;
    if (3 == offset) {
      return 0.0f;  // 4th element of vec4 is defined as zero
    }

    cl_int a_offset = offset < 2 ? 1 : -2;
    cl_int b_offset = offset == 0 ? 2 : -1;

    cl_half half_A = ref_A(id + a_offset);
    cl_float float_A = ConvertHalfToFloat(half_A);

    cl_half half_B = ref_B(id + b_offset);
    cl_float float_B = ConvertHalfToFloat(half_B);

    cl_float result = float_A * float_B;

    a_offset = offset == 0 ? 2 : -1;
    b_offset = offset < 2 ? 1 : -2;

    half_A = ref_A(id + a_offset);
    float_A = ConvertHalfToFloat(half_A);

    half_B = ref_B(id + b_offset);
    float_B = ConvertHalfToFloat(half_B);

    result -= float_A * float_B;
    return result;
  };

  // Function used to determine absolute error threshold, which is dependent on
  // operation inputs.
  const auto err_callback = [&ref_A, ref_B, &vec_width](size_t id) -> cl_float {
    const size_t offset = id % vec_width;
    if (3 == offset) {
      return 0.0f;  // 4th element of vec4 is defined as zero
    }

    size_t a_offset = offset < 2 ? 1 : -2;
    size_t b_offset = offset == 0 ? 2 : -1;

    cl_half half_A = ref_A(id + a_offset);
    cl_float float_A = ConvertHalfToFloat(half_A);
    cl_float max_value = std::fabs(float_A);

    cl_half half_B = ref_B(id + b_offset);
    cl_float float_B = ConvertHalfToFloat(half_B);
    max_value = std::fmax(std::fabs(float_B), max_value);

    a_offset = offset == 0 ? 2 : -1;
    b_offset = offset < 2 ? 1 : -2;

    half_A = ref_A(id + a_offset);
    float_A = ConvertHalfToFloat(half_A);
    max_value = std::fmax(std::fabs(float_A), max_value);

    half_B = ref_B(id + b_offset);
    float_B = ConvertHalfToFloat(half_B);
    max_value = std::fmax(std::fabs(float_B), max_value);

    // On an embedded device w/ round-to-zero, 3 ulps is the worst-case
    // tolerance for cross product
    const cl_float ulp_tolerance = 3.0f;

    // This gives us max squared times ulp tolerance, i.e. the worst-case
    // expected variance we could expect from this result
    const cl_float err_tolerance = max_value * max_value * ulp_tolerance;

    // Check for overflow in tolerance calculation which should be done in
    // same precision as we're testing, but fp16 isn't available on host.
    if (std::fabs(err_tolerance) > TypeInfo<cl_half>::max) {
      return TypeInfo<cl_half>::max;
    }

    // HLF_EPSILON macro from fp16 extension spec
    const cl_float half_epsilon = 0.0009765625f;  // 2^-10
    return err_tolerance * half_epsilon;
  };

  const auto ref_streamer =
      std::make_shared<AbsoluteErrStreamer>(ref_lambda, err_callback, device);
  this->AddOutputBuffer(N, ref_streamer);

  const unsigned work_items = N / vec_width;
  this->RunGeneric1D(work_items);
}

// Cross is only defined for vector widths 3 and 4
UCL_EXECUTION_TEST_SUITE_P(HalfGeometricCross, testing::Values(OPENCL_C),
                           testing::Values(3, 4))
