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

#include "Common.h"
#include "kts/precision.h"

// This is needed for 'TEST_F'.
using namespace kts::ucl;

namespace {
// Given a callable function that returns a float, we generate and return a new
// wrapper function which flushes to zero values returned by the reference if
// they are denormal in half precision.
template <class T>
std::function<cl_float(size_t)> createFTZValidator(T &reference_func) {
  return [&reference_func](size_t id) -> cl_float {
    const cl_float r = reference_func(id);
    return IsDenormalAsHalf(r) ? 0.0f : r;
  };
}
}  // namespace

unsigned kts::ucl::HalfInputSizes::getInputSize(
    const ::ucl::MathMode math_mode) {
  switch (math_mode) {
    case ::ucl::MathMode::FULL:
      return kts::ucl::HalfInputSizes::full;
    case ::ucl::MathMode::QUICK:
      return kts::ucl::HalfInputSizes::quick;
    default:
      return kts::ucl::HalfInputSizes::wimpy;
  }
}

unsigned HalfParamExecution::SetMacro() {
  // Test the various vector widths using macros
  std::string float_type_name("half");
  std::string int_type_name("int");

  const unsigned vec_width = this->getParam();
  if (vec_width != 1) {
    const std::string vec_str = std::to_string(vec_width);

    float_type_name.append(vec_str);
    int_type_name.append(vec_str);
    this->AddMacro("LOAD_FUNC", "vload" + vec_str);
    this->AddMacro("STORE_FUNC", "vstore" + vec_str);
  }

  this->AddMacro("INT_TYPE", int_type_name);
  this->AddMacro("FLOAT_TYPE", float_type_name);
  this->AddMacro("TYPE", float_type_name);

  return vec_width;
}

void HalfParamExecution::InitScalarArgIndices(
    const std::vector<unsigned> &&args) {
  scalar_arg_indices = args;
}

bool HalfParamExecution::IsArgScalar(unsigned index) const {
  // Used to check if function signature takes a scalar parameter type at
  // parametrized index, while output and other inputs are vector types.
  return scalar_arg_indices.end() !=
         std::find(scalar_arg_indices.begin(), scalar_arg_indices.end(), index);
}

const std::vector<cl_ushort> &HalfParamExecution::GetEdgeCases() const {
  static const std::vector<cl_ushort> EdgeCases =
      std::vector<cl_ushort>(InputGenerator::half_edge_cases.begin(),
                             InputGenerator::half_edge_cases.end());
  return EdgeCases;
}

template <size_t N>
unsigned HalfParamExecution::FillInputBuffers(
    std::array<input_details_t, N> &inputs) {
  const auto &edge_cases = GetEdgeCases();

  // The more parameters a functions takes, the more data we need for verifying
  // it's behaviour across the increased range of input combinations. Even in
  // wimpy mode we want to test at least all the combinations of edge case
  // values which have been known to cause failures.
  const unsigned edge_case_len = edge_cases.size();
  size_t cartesian_len = edge_case_len;
  for (size_t i = 1; i < N; i++) {
    cartesian_len *= edge_case_len;
  }

  const auto math_mode = getEnvironment()->math_mode;

  // Wimpy testing buffer size, this is the default
  unsigned length = HalfInputSizes::getInputSize(math_mode);

  if (::ucl::MathMode::FULL == math_mode) {
    // Buffer size for thorough testing, has to be enabled by user
    const unsigned num_inputs_multiplier = N * N;
    length *= num_inputs_multiplier;
  } else {
    length += cartesian_len;
  }

  // Ensure work items divide number of buffer elements equally.
  // Each work item consists of "vec_width" elements. Multiplying this by 4
  // ensures we can have a work group size of at least 4 for vectorization
  // testing purposes.
  const unsigned vec_width = this->getParam() * 4;
  const unsigned remainder = length % vec_width;
  if (0 != remainder) {
    length += vec_width - remainder;
  }

  // Fill buffers with random data
  for (input_details_t &input : inputs) {
    input.is_scalar = IsArgScalar(input.arg_index);
    input.data.resize(length);
    getInputGenerator().GenerateFloatData(input.data);
  }

  // Insert edge cases at the beginning of input buffers in an ordering that
  // reflects the Cartesian product of edge cases across all input combinations
  for (input_details_t &input : inputs) {
    unsigned arg_index = input.arg_index;
    for (size_t i = 0; i < cartesian_len; i++) {
      size_t edge_idx = i % edge_case_len;
      if (0 != arg_index) {
        size_t div = edge_case_len;
        for (size_t n = 1; n < arg_index; n++) {
          div *= edge_case_len;
        }
        edge_idx = i / div;
      }

      input[i] = cargo::bit_cast<cl_half>(edge_cases[edge_idx]);
    }
  }

  // Populate OpenCL input buffers with the data we've just set
  for (const input_details_t &buffer : inputs) {
    this->AddInputBuffer(
        length,
        kts::Reference1D<cl_half>([&buffer](size_t id) { return buffer(id); }));
  }
  return length;
}

template <cl_ulong ULP>
void HalfParamExecution::TestAgainstRef(
    const std::function<cl_float(cl_float)> &ref) {
  const unsigned vec_width = SetMacro();

  // Populate input buffer
  std::array<input_details_t, 1> inputs{{0}};
  const unsigned N = FillInputBuffers(inputs);

  // Use single precision host float for reference
  const input_details_t &half_input = inputs[0];
  const auto ref_lambda = [&half_input, &ref](size_t id) -> cl_float {
    cl_half half_in = half_input(id);
    cl_float float_in = ConvertHalfToFloat(half_in);
    cl_float reference = ref(float_in);
    return reference;
  };
  auto ref_input_formatter = [&half_input](std::stringstream &ss, size_t id) {
    auto value = half_input(id);
    ss << "half " << ConvertHalfToFloat(value);
    ss << "[0x" << std::hex << matchingType(value) << "]" << std::dec;
  };

  // Flush To Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto ftz_input = [&half_input, &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(half_input(id))) {
      return 0.0f;
    }
    return ref_lambda(id);
  };

  const auto ftz_result = [&ref_lambda](size_t id) -> cl_float {
    const cl_float r = ref_lambda(id);
    return IsDenormalAsHalf(r) ? 0.0f : r;
  };

  // Denormals Are Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto daz_positive = [&half_input, &ref,
                             &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(half_input(id))) {
      return ref(0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_negative = [&half_input, &ref,
                             &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(half_input(id))) {
      return ref(-0.0f);
    }
    return ref_lambda(id);
  };

  // Check if the result of the reference function with +/0 is itself a
  // denormal
  const auto daz_neg_result = createFTZValidator(daz_negative);
  const auto daz_pos_result = createFTZValidator(daz_positive);

  // Check if device supports denormals
  const bool denormSupport =
      UCL::hasDenormSupport(device, CL_DEVICE_HALF_FP_CONFIG);
  if (denormSupport) {
    auto refOut = makeULPStreamer<cl_half, ULP>(ref_lambda, device);
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  } else {
    const std::vector<kts::Reference1D<cl_float>> fallbacks{
        ftz_input,    ftz_result,     daz_positive,
        daz_negative, daz_pos_result, daz_neg_result};
    auto refOut =
        makeULPStreamer<cl_half, ULP>(ref_lambda, std::move(fallbacks), device);
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  }

  const unsigned work_items = N / vec_width;
  this->RunGeneric1D(work_items);
}

void HalfParamExecution::TestAgainstIntReturn(
    const std::function<cl_int(cl_float)> &ref) {
  const unsigned vec_width = SetMacro();

  // Populate input buffer
  std::array<input_details_t, 1> inputs{{0}};
  const unsigned N = FillInputBuffers(inputs);

  // Use single precision host float for reference
  const input_details_t &half_input = inputs[0];
  auto ref_lambda = [&half_input, &ref](size_t id) -> cl_int {
    cl_half half_in = half_input(id);
    cl_float float_in = ConvertHalfToFloat(half_in);
    return ref(float_in);
  };
  auto ref_input_formatter = [&half_input](std::stringstream &ss, size_t id) {
    auto value = half_input(id);
    ss << "half " << ConvertHalfToFloat(value);
    ss << "[0x" << std::hex << matchingType(value) << "]" << std::dec;
  };

  // Flush To Zero result if input value is a denormal
  const auto ftz_input = [&half_input, &ref_lambda](size_t id) -> cl_int {
    if (IsDenormal(half_input(id))) {
      return 0;
    }
    return ref_lambda(id);
  };

  // Denormals Are Zero result if input value is a denormal
  const auto daz_positive = [&half_input, &ref,
                             &ref_lambda](size_t id) -> cl_int {
    if (IsDenormal(half_input(id))) {
      return ref(0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_negative = [&half_input, &ref,
                             &ref_lambda](size_t id) -> cl_int {
    if (IsDenormal(half_input(id))) {
      return ref(-0.0f);
    }
    return ref_lambda(id);
  };

  // Check if device supports denormals
  const bool denormSupport =
      UCL::hasDenormSupport(device, CL_DEVICE_HALF_FP_CONFIG);
  if (denormSupport) {
    auto refOut = std::make_shared<GenericStreamer<cl_int>>(ref_lambda);
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  } else {
    const std::vector<kts::Reference1D<cl_int>> fallbacks{
        ftz_input, daz_positive, daz_negative};
    auto refOut = std::make_shared<GenericStreamer<cl_int>>(
        ref_lambda, std::move(fallbacks));
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  }

  const unsigned work_items = N / vec_width;
  this->RunGeneric1D(work_items);
}

template <cl_ulong ULP>
void HalfParamExecution::TestAgainstRef(
    const std::function<cl_float(cl_float, cl_float)> &ref,
    const std::function<bool(cl_float, cl_float)> *undef_ref) {
  const unsigned vec_width = SetMacro();

  // Populate input buffers
  std::array<input_details_t, 2> inputs{{0, 1}};
  const unsigned N = FillInputBuffers(inputs);

  // Helper lambdas for finding input used when calculating output reference
  const input_details_t &buffer_A = inputs[0];
  const auto ref_A = [&buffer_A, vec_width](size_t id) -> cl_half {
    const size_t index = buffer_A.is_scalar ? id / vec_width : id;
    return buffer_A(index);
  };

  const input_details_t &buffer_B = inputs[1];
  const auto ref_B = [&buffer_B, vec_width](size_t id) -> cl_half {
    const size_t index = buffer_B.is_scalar ? id / vec_width : id;
    return buffer_B(index);
  };

  // Use single precision host float for reference
  const auto ref_lambda = [&ref_A, &ref_B, &ref](size_t id) -> cl_float {
    cl_half half_A = ref_A(id);
    cl_half half_B = ref_B(id);

    cl_float float_A = ConvertHalfToFloat(half_A);
    cl_float float_B = ConvertHalfToFloat(half_B);
    cl_float reference = ref(float_A, float_B);
    return reference;
  };
  auto ref_input_formatter = [&ref_A, &ref_B](std::stringstream &ss,
                                              size_t id) {
    cl_half half_A = ref_A(id);
    cl_half half_B = ref_B(id);

    ss << "(half " << ConvertHalfToFloat(half_A);
    ss << "[0x" << std::hex << matchingType(half_A) << "]" << std::dec;
    ss << ", half " << ConvertHalfToFloat(half_B);
    ss << "[0x" << std::hex << matchingType(half_B) << "])" << std::dec;
  };

  // Flush To Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto ftz_input = [&ref_A, &ref_B, &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) || IsDenormal(ref_B(id))) {
      return 0.0f;
    }
    return ref_lambda(id);
  };

  const auto ftz_result = [&ref_lambda](size_t id) -> cl_float {
    const cl_float r = ref_lambda(id);
    if (IsDenormalAsHalf(r)) {
      return 0.0f;
    }
    return r;
  };

  // Denormals Are Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto daz_A_positive = [&ref_A, &ref_B, &ref,
                               &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id))) {
      const cl_float float_B = ConvertHalfToFloat(ref_B(id));
      return ref(0.0f, float_B);
    }
    return ref_lambda(id);
  };

  const auto daz_B_positive = [&ref_A, &ref_B, &ref,
                               &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_B(id))) {
      const cl_float float_A = ConvertHalfToFloat(ref_A(id));
      return ref(float_A, 0.0f);
    }
    return ref_lambda(id);
  };

  // Check if the result of the reference function with +/0 is itself a
  // denormal
  const auto daz_A_positive_result = createFTZValidator(daz_A_positive);
  const auto daz_B_positive_result = createFTZValidator(daz_B_positive);

  const auto daz_A_negative = [&ref_A, &ref_B, &ref,
                               &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id))) {
      const cl_float float_B = ConvertHalfToFloat(ref_B(id));
      return ref(-0.0f, float_B);
    }
    return ref_lambda(id);
  };

  const auto daz_B_negative = [&ref_A, &ref_B, &ref,
                               &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_B(id))) {
      const cl_float float_A = ConvertHalfToFloat(ref_A(id));
      return ref(float_A, -0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_A_negative_result = createFTZValidator(daz_A_negative);
  const auto daz_B_negative_result = createFTZValidator(daz_B_negative);

  const auto daz_pos_pos = [&ref_A, &ref_B, &ref,
                            &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_B(id))) {
      return ref(0.0f, 0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_neg_pos = [&ref_A, &ref_B, &ref,
                            &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_B(id))) {
      return ref(-0.0f, 0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_pos_neg = [&ref_A, &ref_B, &ref,
                            &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_B(id))) {
      return ref(0.0f, -0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_neg_neg = [&ref_A, &ref_B, &ref,
                            &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_B(id))) {
      return ref(-0.0f, -0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_pos_pos_result = createFTZValidator(daz_pos_pos);
  const auto daz_pos_neg_result = createFTZValidator(daz_pos_neg);
  const auto daz_neg_pos_result = createFTZValidator(daz_neg_pos);
  const auto daz_neg_neg_result = createFTZValidator(daz_neg_neg);

  // Some maths functions can have undefined behaviour dependent on the inputs.
  // If the test has passed a reference that returns true when result is
  // undefined create a callback to pass to the streamer
  const auto undef_callback = [&ref_A, &ref_B, &undef_ref](size_t id) {
    if (!undef_ref) {
      return false;
    }

    cl_float float_A = ConvertHalfToFloat(ref_A(id));
    cl_float float_B = ConvertHalfToFloat(ref_B(id));
    return (*undef_ref)(float_A, float_B);
  };

  // Check if device supports denormals
  const bool denormSupport =
      UCL::hasDenormSupport(device, CL_DEVICE_HALF_FP_CONFIG);
  if (denormSupport) {
    auto refOut = makeULPStreamer<cl_half, ULP>(ref_lambda, device);
    if (undef_ref) {
      refOut->SetUndefCallback(std::move(undef_callback));
    }
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  } else {
    const std::vector<kts::Reference1D<cl_float>> fallbacks{
        ftz_input,
        ftz_result,
        daz_A_positive,
        daz_B_positive,
        daz_A_positive_result,
        daz_B_positive_result,
        daz_A_negative,
        daz_B_negative,
        daz_A_negative_result,
        daz_B_negative_result,
        daz_pos_pos,
        daz_pos_neg,
        daz_neg_pos,
        daz_neg_neg,
        daz_pos_pos_result,
        daz_pos_neg_result,
        daz_neg_pos_result,
        daz_neg_neg_result};

    auto refOut =
        makeULPStreamer<cl_half, ULP>(ref_lambda, std::move(fallbacks), device);
    if (undef_ref) {
      refOut->SetUndefCallback(std::move(undef_callback));
    }
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  }

  const unsigned work_items = N / vec_width;
  this->RunGeneric1D(work_items);
}

template <cl_ulong ULP>
void HalfParamExecution::TestAgainstRef(
    const std::function<cl_float(cl_float, cl_float, cl_float)> &ref,
    const std::function<bool(cl_float, cl_float, cl_float)> *undef_ref) {
  const unsigned vec_width = SetMacro();

  // Populate input buffers
  std::array<input_details_t, 3> inputs{{0, 1, 2}};
  const unsigned N = FillInputBuffers(inputs);

  // Helper lambdas for finding input used when calculating output reference
  const input_details_t &buffer_A = inputs[0];
  const auto ref_A = [&buffer_A, vec_width](size_t id) -> cl_half {
    const size_t index = buffer_A.is_scalar ? id / vec_width : id;
    return buffer_A(index);
  };

  const input_details_t &buffer_B = inputs[1];
  const auto ref_B = [&buffer_B, vec_width](size_t id) -> cl_half {
    const size_t index = buffer_B.is_scalar ? id / vec_width : id;
    return buffer_B(index);
  };

  const input_details_t &buffer_C = inputs[2];
  const auto ref_C = [&buffer_C, vec_width](size_t id) -> cl_half {
    const size_t index = buffer_C.is_scalar ? id / vec_width : id;
    return buffer_C(index);
  };

  // Use single precision host float for reference
  const auto ref_lambda = [&ref_A, &ref_B, &ref_C,
                           &ref](size_t id) -> cl_float {
    cl_half half_A = ref_A(id);
    cl_half half_B = ref_B(id);
    cl_half half_C = ref_C(id);

    cl_float float_A = ConvertHalfToFloat(half_A);
    cl_float float_B = ConvertHalfToFloat(half_B);
    cl_float float_C = ConvertHalfToFloat(half_C);
    cl_float reference = ref(float_A, float_B, float_C);
    return reference;
  };
  auto ref_input_formatter = [&ref_A, &ref_B, &ref_C](std::stringstream &ss,
                                                      size_t id) {
    cl_half half_A = ref_A(id);
    cl_half half_B = ref_B(id);
    cl_half half_C = ref_C(id);

    ss << "(half " << ConvertHalfToFloat(half_A);
    ss << "[0x" << std::hex << matchingType(half_A) << "]" << std::dec;
    ss << ", half " << ConvertHalfToFloat(half_B);
    ss << "[0x" << std::hex << matchingType(half_B) << "]" << std::dec;
    ss << ", half " << ConvertHalfToFloat(half_C);
    ss << "[0x" << std::hex << matchingType(half_C) << "])" << std::dec;
  };

  // Flush To Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto ftz_input = [&ref_A, &ref_B, &ref_C,
                          &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) || IsDenormal(ref_B(id)) ||
        IsDenormal(ref_C(id))) {
      return 0.0f;
    }
    return ref_lambda(id);
  };

  const auto ftz_result = [&ref_lambda](size_t id) -> cl_float {
    const cl_float r = ref_lambda(id);
    if (IsDenormalAsHalf(r)) {
      return 0.0f;
    }
    return r;
  };

  // Denormals Are Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  //
  // Try flushing a single input to zero if it's denormal
  const auto daz_A_positive = [&ref_A, &ref_B, &ref_C, &ref,
                               &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id))) {
      const cl_float float_B = ConvertHalfToFloat(ref_B(id));
      const cl_float float_C = ConvertHalfToFloat(ref_C(id));
      return ref(0.0f, float_B, float_C);
    }
    return ref_lambda(id);
  };

  const auto daz_B_positive = [&ref_A, &ref_B, &ref_C, &ref,
                               &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_B(id))) {
      const cl_float float_A = ConvertHalfToFloat(ref_A(id));
      const cl_float float_C = ConvertHalfToFloat(ref_C(id));
      return ref(float_A, 0.0f, float_C);
    }
    return ref_lambda(id);
  };

  const auto daz_C_positive = [&ref_A, &ref_B, &ref_C, &ref,
                               &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_C(id))) {
      const cl_float float_A = ConvertHalfToFloat(ref_A(id));
      const cl_float float_B = ConvertHalfToFloat(ref_B(id));
      return ref(float_A, float_B, 0.0f);
    }
    return ref_lambda(id);
  };

  // Check if the result of the reference function with +/0 is itself a
  // denormal
  const auto daz_A_positive_result = createFTZValidator(daz_A_positive);
  const auto daz_B_positive_result = createFTZValidator(daz_B_positive);
  const auto daz_C_positive_result = createFTZValidator(daz_C_positive);

  const auto daz_A_negative = [&ref_A, &ref_B, &ref_C, &ref,
                               &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id))) {
      const cl_float float_B = ConvertHalfToFloat(ref_B(id));
      const cl_float float_C = ConvertHalfToFloat(ref_C(id));
      return ref(-0.0f, float_B, float_C);
    }
    return ref_lambda(id);
  };

  const auto daz_B_negative = [&ref_A, &ref_B, &ref_C, &ref,
                               &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_B(id))) {
      const cl_float float_A = ConvertHalfToFloat(ref_A(id));
      const cl_float float_C = ConvertHalfToFloat(ref_C(id));
      return ref(float_A, -0.0f, float_C);
    }
    return ref_lambda(id);
  };

  const auto daz_C_negative = [&ref_A, &ref_B, &ref_C, &ref,
                               &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_C(id))) {
      const cl_float float_A = ConvertHalfToFloat(ref_A(id));
      const cl_float float_B = ConvertHalfToFloat(ref_B(id));
      return ref(float_A, float_B, -0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_A_negative_result = createFTZValidator(daz_A_negative);
  const auto daz_B_negative_result = createFTZValidator(daz_B_negative);
  const auto daz_C_negative_result = createFTZValidator(daz_C_negative);

  // Try flushing two inputs to zero if they're both denormal
  const auto daz_pos_pos_c = [&ref_A, &ref_B, &ref_C, &ref,
                              &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_B(id))) {
      const cl_float float_C = ConvertHalfToFloat(ref_C(id));
      return ref(0.0f, 0.0f, float_C);
    }
    return ref_lambda(id);
  };

  const auto daz_neg_pos_c = [&ref_A, &ref_B, &ref_C, &ref,
                              &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_B(id))) {
      const cl_float float_C = ConvertHalfToFloat(ref_C(id));
      return ref(-0.0f, 0.0f, float_C);
    }
    return ref_lambda(id);
  };

  const auto daz_pos_neg_c = [&ref_A, &ref_B, &ref_C, &ref,
                              &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_B(id))) {
      const cl_float float_C = ConvertHalfToFloat(ref_C(id));
      return ref(0.0f, -0.0f, float_C);
    }
    return ref_lambda(id);
  };

  const auto daz_neg_neg_c = [&ref_A, &ref_B, &ref_C, &ref,
                              &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_B(id))) {
      const cl_float float_C = ConvertHalfToFloat(ref_C(id));
      return ref(-0.0f, -0.0f, float_C);
    }
    return ref_lambda(id);
  };

  const auto daz_pos_pos_c_result = createFTZValidator(daz_pos_pos_c);
  const auto daz_pos_neg_c_result = createFTZValidator(daz_pos_neg_c);
  const auto daz_neg_pos_c_result = createFTZValidator(daz_neg_pos_c);
  const auto daz_neg_neg_c_result = createFTZValidator(daz_neg_neg_c);

  const auto daz_a_pos_pos = [&ref_A, &ref_B, &ref_C, &ref,
                              &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_B(id)) && IsDenormal(ref_C(id))) {
      const cl_float float_A = ConvertHalfToFloat(ref_A(id));
      return ref(float_A, 0.0f, 0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_a_neg_pos = [&ref_A, &ref_B, &ref_C, &ref,
                              &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_B(id)) && IsDenormal(ref_C(id))) {
      const cl_float float_A = ConvertHalfToFloat(ref_A(id));
      return ref(float_A, -0.0f, 0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_a_pos_neg = [&ref_A, &ref_B, &ref_C, &ref,
                              &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_B(id)) && IsDenormal(ref_C(id))) {
      const cl_float float_A = ConvertHalfToFloat(ref_A(id));
      return ref(float_A, 0.0f, -0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_a_neg_neg = [&ref_A, &ref_B, &ref_C, &ref,
                              &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_B(id)) && IsDenormal(ref_C(id))) {
      const cl_float float_A = ConvertHalfToFloat(ref_A(id));
      return ref(float_A, -0.0f, -0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_a_pos_pos_result = createFTZValidator(daz_a_pos_pos);
  const auto daz_a_pos_neg_result = createFTZValidator(daz_a_pos_neg);
  const auto daz_a_neg_pos_result = createFTZValidator(daz_a_neg_pos);
  const auto daz_a_neg_neg_result = createFTZValidator(daz_a_neg_neg);

  const auto daz_pos_b_pos = [&ref_A, &ref_B, &ref_C, &ref,
                              &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_C(id))) {
      const cl_float float_B = ConvertHalfToFloat(ref_B(id));
      return ref(0.0f, float_B, 0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_neg_b_pos = [&ref_A, &ref_B, &ref_C, &ref,
                              &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_C(id))) {
      const cl_float float_B = ConvertHalfToFloat(ref_B(id));
      return ref(-0.0f, float_B, 0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_pos_b_neg = [&ref_A, &ref_B, &ref_C, &ref,
                              &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_C(id))) {
      const cl_float float_B = ConvertHalfToFloat(ref_B(id));
      return ref(0.0f, float_B, -0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_neg_b_neg = [&ref_A, &ref_B, &ref_C, &ref,
                              &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_C(id))) {
      const cl_float float_B = ConvertHalfToFloat(ref_B(id));
      return ref(-0.0f, float_B, -0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_pos_b_pos_result = createFTZValidator(daz_pos_b_pos);
  const auto daz_pos_b_neg_result = createFTZValidator(daz_pos_b_neg);
  const auto daz_neg_b_pos_result = createFTZValidator(daz_neg_b_pos);
  const auto daz_neg_b_neg_result = createFTZValidator(daz_neg_b_neg);

  // Try flushing all three inputs to zero if they're denormal
  const auto daz_pos_pos_pos = [&ref_A, &ref_B, &ref_C, &ref,
                                &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_B(id)) &&
        IsDenormal(ref_C(id))) {
      return ref(0.0f, 0.0f, 0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_pos_pos_neg = [&ref_A, &ref_B, &ref_C, &ref,
                                &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_B(id)) &&
        IsDenormal(ref_C(id))) {
      return ref(0.0f, 0.0f, -0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_pos_neg_pos = [&ref_A, &ref_B, &ref_C, &ref,
                                &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_B(id)) &&
        IsDenormal(ref_C(id))) {
      return ref(0.0f, -0.0f, 0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_neg_pos_pos = [&ref_A, &ref_B, &ref_C, &ref,
                                &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_B(id)) &&
        IsDenormal(ref_C(id))) {
      return ref(-0.0f, 0.0f, 0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_pos_neg_neg = [&ref_A, &ref_B, &ref_C, &ref,
                                &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_B(id)) &&
        IsDenormal(ref_C(id))) {
      return ref(0.0f, -0.0f, -0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_neg_pos_neg = [&ref_A, &ref_B, &ref_C, &ref,
                                &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_B(id)) &&
        IsDenormal(ref_C(id))) {
      return ref(-0.0f, 0.0f, -0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_neg_neg_pos = [&ref_A, &ref_B, &ref_C, &ref,
                                &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_B(id)) &&
        IsDenormal(ref_C(id))) {
      return ref(-0.0f, -0.0f, 0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_neg_neg_neg = [&ref_A, &ref_B, &ref_C, &ref,
                                &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_A(id)) && IsDenormal(ref_B(id)) &&
        IsDenormal(ref_C(id))) {
      return ref(-0.0f, -0.0f, -0.0f);
    }
    return ref_lambda(id);
  };

  const auto daz_pos_pos_pos_result = createFTZValidator(daz_pos_pos_pos);
  const auto daz_pos_pos_neg_result = createFTZValidator(daz_pos_pos_neg);
  const auto daz_pos_neg_pos_result = createFTZValidator(daz_pos_neg_pos);
  const auto daz_neg_pos_pos_result = createFTZValidator(daz_neg_pos_pos);
  const auto daz_pos_neg_neg_result = createFTZValidator(daz_pos_neg_neg);
  const auto daz_neg_pos_neg_result = createFTZValidator(daz_neg_pos_neg);
  const auto daz_neg_neg_pos_result = createFTZValidator(daz_neg_neg_pos);
  const auto daz_neg_neg_neg_result = createFTZValidator(daz_neg_neg_neg);

  // Some maths functions can have undefined behaviour dependent on the inputs.
  // If the test has passed a reference that returns true when result is
  // undefined create a callback to pass to the streamer
  const auto undef_callback = [&ref_A, &ref_B, &ref_C, &undef_ref](size_t id) {
    if (!undef_ref) {
      return false;
    }

    cl_float float_A = ConvertHalfToFloat(ref_A(id));
    cl_float float_B = ConvertHalfToFloat(ref_B(id));
    cl_float float_C = ConvertHalfToFloat(ref_C(id));
    return (*undef_ref)(float_A, float_B, float_C);
  };

  // Check if device supports denormals
  const bool denormSupport =
      UCL::hasDenormSupport(device, CL_DEVICE_HALF_FP_CONFIG);
  if (denormSupport) {
    auto refOut = makeULPStreamer<cl_half, ULP>(ref_lambda, device);
    if (undef_ref) {
      refOut->SetUndefCallback(std::move(undef_callback));
    }
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  } else {
    const std::vector<kts::Reference1D<cl_float>> fallbacks{
        ftz_input,
        ftz_result,
        daz_A_positive,
        daz_B_positive,
        daz_C_positive,
        daz_A_positive_result,
        daz_B_positive_result,
        daz_C_positive_result,
        daz_A_negative,
        daz_B_negative,
        daz_C_negative,
        daz_A_negative_result,
        daz_B_negative_result,
        daz_C_negative_result,
        daz_a_pos_pos,
        daz_a_pos_neg,
        daz_a_neg_pos,
        daz_a_neg_neg,
        daz_a_pos_pos_result,
        daz_a_pos_neg_result,
        daz_a_neg_pos_result,
        daz_a_neg_neg_result,
        daz_pos_b_pos,
        daz_pos_b_neg,
        daz_neg_b_pos,
        daz_neg_b_neg,
        daz_pos_b_pos_result,
        daz_pos_b_neg_result,
        daz_neg_b_pos_result,
        daz_neg_b_neg_result,
        daz_pos_pos_c,
        daz_pos_neg_c,
        daz_neg_pos_c,
        daz_neg_neg_c,
        daz_pos_pos_c_result,
        daz_pos_neg_c_result,
        daz_neg_pos_c_result,
        daz_neg_neg_c_result,
        daz_pos_pos_pos,
        daz_pos_pos_neg,
        daz_pos_neg_pos,
        daz_neg_pos_pos,
        daz_pos_neg_neg,
        daz_neg_pos_neg,
        daz_neg_neg_pos,
        daz_neg_neg_neg,
        daz_pos_pos_pos_result,
        daz_pos_pos_neg_result,
        daz_pos_neg_pos_result,
        daz_neg_pos_pos_result,
        daz_pos_neg_neg_result,
        daz_neg_pos_neg_result,
        daz_neg_neg_pos_result,
        daz_neg_neg_neg_result};
    auto refOut =
        makeULPStreamer<cl_half, ULP>(ref_lambda, std::move(fallbacks), device);
    if (undef_ref) {
      refOut->SetUndefCallback(std::move(undef_callback));
    }
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  }

  const unsigned work_items = N / vec_width;
  this->RunGeneric1D(work_items);
}

template <cl_ulong ULP>
void HalfParamExecution::TestAgainstIntArgRef(
    const std::function<cl_float(cl_float, cl_int)> &ref) {
  const unsigned vec_width = SetMacro();

  // Populate float input buffer
  std::array<input_details_t, 1> input_halfs{{0}};
  const unsigned N = FillInputBuffers(input_halfs);

  // Use short input to constrain the range, although the ldexp function API
  // takes an int. Otherwise the majority of the tests exceed the precision
  // limits of half and return zero/inf on under/overflow
  std::vector<cl_short> input_ints(N);
  getInputGenerator().GenerateIntData(input_ints);

  kts::Reference1D<cl_int> ref_int = [&input_ints, N](size_t id) {
    // Use occasional large 32-bit int input
    if ((N >= 128u) && (id % 128u) == 126u) {
      return (static_cast<cl_int>(input_ints[id]) << 16) |
             static_cast<cl_int>(input_ints[id + 1]);
    } else {
      return static_cast<cl_int>(input_ints[id]);
    }
  };
  this->AddInputBuffer(N, ref_int);

  // Use single precision host float for reference
  const input_details_t ref_half = input_halfs[0];
  const auto ref_lambda = [&ref_int, &ref_half, &ref](size_t id) -> cl_float {
    cl_half half_input = ref_half(id);
    cl_int int_input = ref_int(id);

    cl_float float_input = ConvertHalfToFloat(half_input);

    cl_float reference = ref(float_input, int_input);
    return reference;
  };
  auto ref_input_formatter = [&ref_int, &ref_half](std::stringstream &ss,
                                                   size_t id) {
    cl_half half_input = ref_half(id);
    cl_int int_input = ref_int(id);

    ss << "(half " << ConvertHalfToFloat(half_input);
    ss << "[0x" << std::hex << matchingType(half_input) << "]" << std::dec;
    ss << ", " << int_input << ")";
  };

  // Flush To Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto ftz_input = [&ref_half, &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_half(id))) {
      return 0.0f;
    }
    return ref_lambda(id);
  };

  const auto ftz_result = [&ref_lambda](size_t id) -> cl_float {
    const cl_float r = ref_lambda(id);
    return IsDenormalAsHalf(r) ? 0.0f : r;
  };

  // Denormals Are Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto daz_positive = [&ref_half, &ref_int, &ref,
                             &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_half(id))) {
      return ref(0.0f, ref_int(id));
    }
    return ref_lambda(id);
  };

  const auto daz_negative = [&ref_half, &ref_int, &ref,
                             &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(ref_half(id))) {
      return ref(-0.0f, ref_int(id));
    }
    return ref_lambda(id);
  };

  // Check if the result of the reference function with +/0 is itself a
  // denormal
  const auto daz_pos_result = createFTZValidator(daz_positive);
  const auto daz_neg_result = createFTZValidator(daz_negative);

  // Check if device supports denormals
  const bool denormSupport =
      UCL::hasDenormSupport(device, CL_DEVICE_HALF_FP_CONFIG);
  if (denormSupport) {
    auto refOut = makeULPStreamer<cl_half, ULP>(ref_lambda, device);
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  } else {
    const std::vector<kts::Reference1D<cl_float>> fallbacks{
        ftz_input,    ftz_result,     daz_positive,
        daz_negative, daz_pos_result, daz_neg_result};
    auto refOut =
        makeULPStreamer<cl_half, ULP>(ref_lambda, std::move(fallbacks), device);
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  }

  const unsigned work_items = N / vec_width;
  this->RunGeneric1D(work_items);
}

template <cl_ulong ULP>
void HalfParamExecution::TestAgainstIntReferenceArgRef(
    const std::function<cl_float(cl_float, cl_int &)> &ref) {
  const unsigned vec_width = SetMacro();

  // Populate input buffer
  std::array<input_details_t, 1> inputs{{0}};
  const unsigned N = FillInputBuffers(inputs);

  // Use single precision host float for reference
  const input_details_t &input_half = inputs[0];
  const auto ref_lambda = [&input_half, &ref](size_t id) -> cl_float {
    cl_half half_input = input_half(id);

    cl_float float_input = ConvertHalfToFloat(half_input);
    cl_int out_int{0};
    cl_float reference = ref(float_input, out_int);
    return reference;
  };
  auto ref_input_formatter = [input_half](std::stringstream &ss, size_t id) {
    cl_half half_input = input_half(id);

    ss << "half " << ConvertHalfToFloat(half_input);
    ss << "[0x" << std::hex << matchingType(half_input) << "]" << std::dec;
  };

  // Flush To Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto ftz_input = [&input_half, &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(input_half(id))) {
      return 0.0f;
    }
    return ref_lambda(id);
  };

  const auto ftz_result = [&ref_lambda](size_t id) -> cl_float {
    const cl_float r = ref_lambda(id);
    return IsDenormalAsHalf(r) ? 0.0f : r;
  };

  // Denormals Are Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto daz_positive = [&input_half, &ref,
                             &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(input_half(id))) {
      cl_int out_int{0};
      return ref(0.0f, out_int);
    }
    return ref_lambda(id);
  };

  const auto daz_negative = [&input_half, &ref,
                             &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(input_half(id))) {
      cl_int out_int{0};
      return ref(-0.0f, out_int);
    }
    return ref_lambda(id);
  };

  // Check if the result of the reference function with +/0 is itself a
  // denormal
  const auto daz_neg_result = createFTZValidator(daz_negative);
  const auto daz_pos_result = createFTZValidator(daz_positive);

  // Check if device supports denormals
  const bool denormSupport =
      UCL::hasDenormSupport(device, CL_DEVICE_HALF_FP_CONFIG);
  if (denormSupport) {
    auto refOut = makeULPStreamer<cl_half, ULP>(ref_lambda, device);
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  } else {
    const std::vector<kts::Reference1D<cl_float>> fallbacks{
        ftz_input,    ftz_result,     daz_positive,
        daz_negative, daz_pos_result, daz_neg_result};
    auto refOut =
        makeULPStreamer<cl_half, ULP>(ref_lambda, std::move(fallbacks), device);
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  }

  // Reference lambdas for integer output argument
  auto ref_int_out = [&input_half, &ref](size_t id) -> cl_int {
    cl_half half_input = input_half(id);
    cl_float float_input = ConvertHalfToFloat(half_input);
    cl_int out_int{0};
    ref(float_input, out_int);
    return out_int;
  };

  // Flush To Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto ftz_int_input = [&input_half, &ref_int_out](size_t id) -> cl_int {
    if (IsDenormal(input_half(id))) {
      return 0;
    }
    return ref_int_out(id);
  };

  // Denormals Are Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto daz_int_positive = [&input_half, &ref,
                                 &ref_int_out](size_t id) -> cl_int {
    if (IsDenormal(input_half(id))) {
      cl_int daz_int{0};
      ref(0.0f, daz_int);
      return daz_int;
    }
    return ref_int_out(id);
  };

  const auto daz_int_negative = [&input_half, &ref,
                                 &ref_int_out](size_t id) -> cl_int {
    if (IsDenormal(input_half(id))) {
      cl_int daz_int{0};
      ref(-0.0f, daz_int);
      return daz_int;
    }
    return ref_int_out(id);
  };

  if (denormSupport) {
    auto refOut = std::make_shared<GenericStreamer<cl_int>>(ref_int_out);
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  } else {
    const std::vector<kts::Reference1D<cl_int>> fallbacks{
        ftz_int_input, daz_int_positive, daz_int_negative};

    auto refOut = std::make_shared<GenericStreamer<cl_int>>(
        ref_int_out, std::move(fallbacks));
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  }

  const unsigned work_items = N / vec_width;
  this->RunGeneric1D(work_items);
}

template <cl_ulong ULP>
void HalfParamExecution::TestAgainstIntReferenceArgRef(
    const std::function<cl_float(cl_float, cl_float, cl_int &)> &ref) {
  const unsigned vec_width = SetMacro();

  // Populate input buffers
  std::array<input_details_t, 2> inputs{{0, 1}};
  const unsigned N = FillInputBuffers(inputs);

  // Use single precision host float for reference
  const input_details_t &input_halfs_A = inputs[0];
  const input_details_t &input_halfs_B = inputs[1];
  const auto ref_lambda = [&input_halfs_A, &input_halfs_B,
                           &ref](size_t id) -> cl_float {
    cl_half half_input_a = input_halfs_A(id);
    cl_half half_input_b = input_halfs_B(id);

    cl_float float_input_a = ConvertHalfToFloat(half_input_a);
    cl_float float_input_b = ConvertHalfToFloat(half_input_b);
    cl_int out_int{0};
    cl_float reference = ref(float_input_a, float_input_b, out_int);
    return reference;
  };
  auto ref_input_formatter = [&input_halfs_A, &input_halfs_B](
                                 std::stringstream &ss, size_t id) {
    cl_half half_input_a = input_halfs_A(id);
    cl_half half_input_b = input_halfs_B(id);

    ss << "(half " << ConvertHalfToFloat(half_input_a);
    ss << "[0x" << std::hex << matchingType(half_input_a) << "]" << std::dec;
    ss << ", half " << ConvertHalfToFloat(half_input_b);
    ss << "[0x" << std::hex << matchingType(half_input_b) << "])" << std::dec;
  };

  // Flush To Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto ftz_input = [&input_halfs_A, &input_halfs_B,
                          &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(input_halfs_A(id)) || IsDenormal(input_halfs_B(id))) {
      return 0.0f;
    }
    return ref_lambda(id);
  };

  const auto ftz_result = [&ref_lambda](size_t id) -> cl_float {
    const cl_float r = ref_lambda(id);
    if (IsDenormalAsHalf(r)) {
      return 0.0f;
    }
    return r;
  };

  // Denormals Are Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto daz_A_positive = [&input_halfs_A, &input_halfs_B, &ref,
                               &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(input_halfs_A(id))) {
      const cl_float float_B = ConvertHalfToFloat(input_halfs_B(id));
      cl_int out_int{0};
      return ref(0.0f, float_B, out_int);
    }
    return ref_lambda(id);
  };

  const auto daz_B_positive = [&input_halfs_A, &input_halfs_B, &ref,
                               &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(input_halfs_B(id))) {
      const cl_float float_A = ConvertHalfToFloat(input_halfs_A(id));
      cl_int out_int{0};
      return ref(float_A, 0.0f, out_int);
    }
    return ref_lambda(id);
  };

  // Check if the result of the reference function with +/0 is itself a
  // denormal
  const auto daz_A_positive_result = createFTZValidator(daz_A_positive);
  const auto daz_B_positive_result = createFTZValidator(daz_B_positive);

  const auto daz_A_negative = [&input_halfs_A, &input_halfs_B, &ref,
                               &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(input_halfs_A(id))) {
      const cl_float float_B = ConvertHalfToFloat(input_halfs_B(id));
      cl_int out_int{0};
      return ref(-0.0f, float_B, out_int);
    }
    return ref_lambda(id);
  };

  const auto daz_B_negative = [&input_halfs_A, &input_halfs_B, &ref,
                               &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(input_halfs_B(id))) {
      const cl_float float_A = ConvertHalfToFloat(input_halfs_A(id));
      cl_int out_int{0};
      return ref(float_A, -0.0f, out_int);
    }
    return ref_lambda(id);
  };

  const auto daz_A_negative_result = createFTZValidator(daz_A_negative);
  const auto daz_B_negative_result = createFTZValidator(daz_B_negative);

  const auto daz_pos_pos = [&input_halfs_A, &input_halfs_B, &ref,
                            &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(input_halfs_A(id)) && IsDenormal(input_halfs_B(id))) {
      cl_int out_int{0};
      return ref(0.0f, 0.0f, out_int);
    }
    return ref_lambda(id);
  };

  const auto daz_neg_pos = [&input_halfs_A, &input_halfs_B, &ref,
                            &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(input_halfs_A(id)) && IsDenormal(input_halfs_B(id))) {
      cl_int out_int{0};
      return ref(-0.0f, 0.0f, out_int);
    }
    return ref_lambda(id);
  };

  const auto daz_pos_neg = [&input_halfs_A, &input_halfs_B, &ref,
                            &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(input_halfs_A(id)) && IsDenormal(input_halfs_B(id))) {
      cl_int out_int{0};
      return ref(0.0f, -0.0f, out_int);
    }
    return ref_lambda(id);
  };

  const auto daz_neg_neg = [&input_halfs_A, &input_halfs_B, &ref,
                            &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(input_halfs_A(id)) && IsDenormal(input_halfs_B(id))) {
      cl_int out_int{0};
      return ref(-0.0f, -0.0f, out_int);
    }
    return ref_lambda(id);
  };

  const auto daz_pos_pos_result = createFTZValidator(daz_pos_pos);
  const auto daz_pos_neg_result = createFTZValidator(daz_pos_neg);
  const auto daz_neg_pos_result = createFTZValidator(daz_neg_pos);
  const auto daz_neg_neg_result = createFTZValidator(daz_neg_neg);

  // Check if device supports denormals
  const bool denormSupport =
      UCL::hasDenormSupport(device, CL_DEVICE_HALF_FP_CONFIG);
  if (denormSupport) {
    auto refOut = makeULPStreamer<cl_half, ULP>(ref_lambda, device);
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  } else {
    const std::vector<kts::Reference1D<cl_float>> fallbacks{
        ftz_input,
        ftz_result,
        daz_A_positive,
        daz_B_positive,
        daz_A_positive_result,
        daz_B_positive_result,
        daz_A_negative,
        daz_B_negative,
        daz_A_negative_result,
        daz_B_negative_result,
        daz_pos_pos,
        daz_pos_neg,
        daz_neg_pos,
        daz_neg_neg,
        daz_pos_pos_result,
        daz_pos_neg_result,
        daz_neg_pos_result,
        daz_neg_neg_result};
    auto refOut =
        makeULPStreamer<cl_half, ULP>(ref_lambda, std::move(fallbacks), device);
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  }

  // Reference lambdas for integer output argument
  auto ref_int_out = [&input_halfs_A, &input_halfs_B,
                      &ref](size_t id) -> cl_int {
    cl_half half_input_a = input_halfs_A(id);
    cl_half half_input_b = input_halfs_B(id);

    cl_float float_input_a = ConvertHalfToFloat(half_input_a);
    cl_float float_input_b = ConvertHalfToFloat(half_input_b);

    cl_int out_int{0};
    ref(float_input_a, float_input_b, out_int);
    return out_int;
  };

  // Flush To Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto ftz_int_input = [&input_halfs_A, &input_halfs_B,
                              &ref_int_out](size_t id) -> cl_int {
    if (IsDenormal(input_halfs_A(id)) || IsDenormal(input_halfs_B(id))) {
      return 0;
    }
    return ref_int_out(id);
  };

  // Denormals Are Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto daz_int_A_positive = [&input_halfs_A, &input_halfs_B, &ref,
                                   &ref_int_out](size_t id) -> cl_int {
    if (IsDenormal(input_halfs_A(id))) {
      cl_int daz_int{0};
      const cl_float float_input_b = ConvertHalfToFloat(input_halfs_B(id));
      ref(0.0f, float_input_b, daz_int);
      return daz_int;
    }
    return ref_int_out(id);
  };

  const auto daz_int_A_negative = [&input_halfs_A, &input_halfs_B, &ref,
                                   &ref_int_out](size_t id) -> cl_int {
    if (IsDenormal(input_halfs_A(id))) {
      cl_int daz_int{0};
      const cl_float float_input_b = ConvertHalfToFloat(input_halfs_B(id));
      ref(-0.0f, float_input_b, daz_int);
      return daz_int;
    }
    return ref_int_out(id);
  };

  const auto daz_int_B_positive = [&input_halfs_A, &input_halfs_B, &ref,
                                   &ref_int_out](size_t id) -> cl_int {
    if (IsDenormal(input_halfs_B(id))) {
      cl_int daz_int{0};
      const cl_float float_input_a = ConvertHalfToFloat(input_halfs_A(id));
      ref(float_input_a, 0.0f, daz_int);
      return daz_int;
    }
    return ref_int_out(id);
  };

  const auto daz_int_B_negative = [&input_halfs_A, &input_halfs_B, &ref,
                                   &ref_int_out](size_t id) -> cl_int {
    if (IsDenormal(input_halfs_B(id))) {
      cl_int daz_int{0};
      const cl_float float_input_a = ConvertHalfToFloat(input_halfs_A(id));
      ref(float_input_a, -0.0f, daz_int);
      return daz_int;
    }
    return ref_int_out(id);
  };

  if (denormSupport) {
    auto refOut = std::make_shared<GenericStreamer<cl_int>>(ref_int_out);
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  } else {
    const std::vector<kts::Reference1D<cl_int>> fallbacks{
        ftz_int_input, daz_int_A_positive, daz_int_A_negative,
        daz_int_B_positive, daz_int_B_negative};

    auto refOut = std::make_shared<GenericStreamer<cl_int>>(
        ref_int_out, std::move(fallbacks));
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  }
  const unsigned work_items = N / vec_width;
  this->RunGeneric1D(work_items);
}

template <cl_ulong ULP>
void HalfParamExecution::TestAgainstFloatReferenceArgRef(
    const std::function<cl_float(cl_float, cl_float &)> &ref) {
  const unsigned vec_width = SetMacro();

  // Populate input buffer
  std::array<input_details_t, 1> inputs{{0}};
  const unsigned N = FillInputBuffers(inputs);

  // Use single precision host float for reference
  const input_details_t &input_halfs = inputs[0];
  const auto ref_lambda = [&input_halfs, &ref](size_t id) -> cl_float {
    cl_half half_input = input_halfs(id);

    cl_float float_input = ConvertHalfToFloat(half_input);
    cl_float out_float{0};
    cl_float reference = ref(float_input, out_float);
    return reference;
  };
  auto ref_input_formatter = [&input_halfs](std::stringstream &ss, size_t id) {
    cl_half half_input = input_halfs(id);

    ss << ConvertHalfToFloat(half_input);
    ss << "[0x" << std::hex << matchingType(half_input) << "]" << std::dec;
  };

  // Flush To Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto ftz_input = [&input_halfs, &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(input_halfs(id))) {
      return 0.0f;
    }
    return ref_lambda(id);
  };

  const auto ftz_result = [&ref_lambda](size_t id) -> cl_float {
    const cl_float r = ref_lambda(id);
    return IsDenormalAsHalf(r) ? 0.0f : r;
  };

  // Denormals Are Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto daz_positive = [&input_halfs, &ref,
                             &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(input_halfs(id))) {
      cl_float out_float{0};
      return ref(0.0f, out_float);
    }
    return ref_lambda(id);
  };

  const auto daz_negative = [&input_halfs, &ref,
                             &ref_lambda](size_t id) -> cl_float {
    if (IsDenormal(input_halfs(id))) {
      cl_float out_float{0};
      return ref(-0.0f, out_float);
    }
    return ref_lambda(id);
  };

  // Check if the result of the reference function with +/0 is itself a
  // denormal
  const auto daz_neg_result = createFTZValidator(daz_negative);
  const auto daz_pos_result = createFTZValidator(daz_positive);

  // Check if device supports denormals
  const bool denormSupport =
      UCL::hasDenormSupport(device, CL_DEVICE_HALF_FP_CONFIG);
  if (denormSupport) {
    this->AddOutputBuffer(N, makeULPStreamer<cl_half, ULP>(ref_lambda, device));
  } else {
    const std::vector<kts::Reference1D<cl_float>> fallbacks{
        ftz_input,    ftz_result,     daz_positive,
        daz_negative, daz_pos_result, daz_neg_result};
    this->AddOutputBuffer(N, makeULPStreamer<cl_half, ULP>(
                                 ref_lambda, std::move(fallbacks), device));
  }

  // Reference lambdas for float output argument
  const auto ref_float_out = [&input_halfs, &ref](size_t id) -> cl_float {
    cl_half half_input = input_halfs(id);
    cl_float float_input = ConvertHalfToFloat(half_input);

    cl_float out_float{0};
    ref(float_input, out_float);
    return out_float;
  };

  // Flush To Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto ftz_float_output = [&input_halfs,
                                 &ref_float_out](size_t id) -> cl_float {
    if (IsDenormal(input_halfs(id))) {
      return 0.0f;
    }
    return ref_float_out(id);
  };

  const auto ftz_out_result = [&ref_float_out](size_t id) -> cl_float {
    const cl_float r = ref_float_out(id);
    return IsDenormalAsHalf(r) ? 0.0f : r;
  };

  // Denormals Are Zero result if input value is a denormal, according to spec
  // section 7.5.3 the sign of zero is not defined
  const auto daz_out_positive = [&input_halfs, &ref,
                                 &ref_float_out](size_t id) -> cl_float {
    if (IsDenormal(input_halfs(id))) {
      cl_float out_float{0};
      ref(0.0f, out_float);
      return out_float;
    }
    return ref_float_out(id);
  };

  const auto daz_out_negative = [&input_halfs, &ref,
                                 &ref_float_out](size_t id) -> cl_float {
    if (IsDenormal(input_halfs(id))) {
      cl_float out_float{0};
      ref(-0.0f, out_float);
      return out_float;
    }
    return ref_float_out(id);
  };

  // Check if the result of the reference function with +/0 is itself a
  // denormal
  const auto daz_out_neg_result = createFTZValidator(daz_out_negative);
  const auto daz_out_pos_result = createFTZValidator(daz_out_positive);

  if (denormSupport) {
    auto refOut = makeULPStreamer<cl_half, ULP>(ref_float_out, device);
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  } else {
    const std::vector<kts::Reference1D<cl_float>> fallbacks{
        ftz_float_output, ftz_out_result,     daz_out_positive,
        daz_out_negative, daz_out_pos_result, daz_out_neg_result};
    auto refOut =
        makeULPStreamer<cl_half, ULP>(ref_lambda, std::move(fallbacks), device);
    refOut->SetInputFormatter(ref_input_formatter);
    this->AddOutputBuffer(N, refOut);
  }

  const unsigned work_items = N / vec_width;
  this->RunGeneric1D(work_items);
}

// Manual instantiation of template functions
namespace kts {
namespace ucl {

// fract(), modf()
template void HalfParamExecution::TestAgainstFloatReferenceArgRef<0_ULP>(
    const std::function<cl_float(cl_float, cl_float &)> &);

// sincos()
template void HalfParamExecution::TestAgainstFloatReferenceArgRef<2_ULP>(
    const std::function<cl_float(cl_float, cl_float &)> &);

// frexp()
template void HalfParamExecution::TestAgainstIntReferenceArgRef<2_ULP>(
    const std::function<cl_float(cl_float, cl_int &)> &);

// lgamma_r()
template void HalfParamExecution::TestAgainstIntReferenceArgRef<MAX_ULP_ERROR>(
    const std::function<cl_float(cl_float, cl_int &)> &);

// remquo()
template void HalfParamExecution::TestAgainstIntReferenceArgRef<0_ULP>(
    const std::function<cl_float(cl_float, cl_float, cl_int &)> &);

// ldexp()
template void HalfParamExecution::TestAgainstIntArgRef<0_ULP>(
    const std::function<cl_float(cl_float, cl_int)> &);

// pown(), rootn()
template void HalfParamExecution::TestAgainstIntArgRef<4_ULP>(
    const std::function<cl_float(cl_float, cl_int)> &);

// fabs(), floor(), ceil(), trunc(), round(), rint(), logb(), ...
template void HalfParamExecution::TestAgainstRef<0_ULP>(
    const std::function<cl_float(cl_float)> &);

// rsqrt
template void HalfParamExecution::TestAgainstRef<1_ULP>(
    const std::function<cl_float(cl_float)> &);

// exp(), exp2(), exp10(), expm1(), sinpi(), cospi(), log(), log2(), log10(),...
template void HalfParamExecution::TestAgainstRef<2_ULP>(
    const std::function<cl_float(cl_float)> &);

// erf(), erfc(), tgamma()
template void HalfParamExecution::TestAgainstRef<4_ULP>(
    const std::function<cl_float(cl_float)> &);

// lgamma()
template void HalfParamExecution::TestAgainstRef<MAX_ULP_ERROR>(
    const std::function<cl_float(cl_float)> &);

// mad(), fma(), clamp()
template void HalfParamExecution::TestAgainstRef<0_ULP>(
    const std::function<cl_float(cl_float, cl_float, cl_float)> &,
    const std::function<bool(cl_float, cl_float, cl_float)> *);

// mix(), smoothstep()
template void HalfParamExecution::TestAgainstRef<MAX_ULP_ERROR>(
    const std::function<cl_float(cl_float, cl_float, cl_float)> &,
    const std::function<bool(cl_float, cl_float, cl_float)> *);

// add, sub, multiply, divide, copysign(), fmin(), fmax(), maxmag(), fmod(), ...
template void HalfParamExecution::TestAgainstRef<0_ULP>(
    const std::function<cl_float(cl_float, cl_float)> &,
    const std::function<bool(cl_float, cl_float)> *);

// pow(), powr()
template void HalfParamExecution::TestAgainstRef<4_ULP>(
    const std::function<cl_float(cl_float, cl_float)> &,
    const std::function<bool(cl_float, cl_float)> *);

// hypot
template void HalfParamExecution::TestAgainstRef<2_ULP>(
    const std::function<cl_float(cl_float, cl_float)> &,
    const std::function<bool(cl_float, cl_float)> *);
}  // namespace ucl
}  // namespace kts
