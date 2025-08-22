// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
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

// For native we need an extremely lax validator. Native type builtins have
// implementation defined precision, so we only care about ballpark accuracy.
template <typename T, cl_ulong t> struct AbsoluteErrValidator final {
  AbsoluteErrValidator() : threshold(1.0 / static_cast<T>(t)) {}

  // Validator checks for inf and nan, and then just checks the actual value is
  // within an absolute error of the expected.
  bool validate(const T &expected, const T &actual) {
    if (std::isnan(expected) && std::isnan(actual)) {
      return true;
    }

    if (std::isinf(expected) && std::isinf(actual)) {
      return true;
    }

    const T err = std::fabs(expected - actual);
    return err < threshold ? true : false;
  }

  void print(std::stringstream &s, T value) { s << value; }

private:
  // Error threshold is 1 over tparam t (template params can't be float).
  cl_float threshold;
};

template <typename T, cl_ulong threshold>
using AbsoluteErrStreamerTy =
    kts::GenericStreamer<T, AbsoluteErrValidator<T, threshold>>;

template <typename T, cl_ulong threshold, typename F>
std::shared_ptr<AbsoluteErrStreamerTy<T, threshold>>
makeAbsoluteErrStreamer(F &&f) {
  auto s = std::make_shared<AbsoluteErrStreamerTy<T, threshold>>(
      kts::Reference1D<T>(std::forward<F>(f)));

  return s;
}

TEST_F(BaseExecution, Native_01_Log2_Accuracy) {
  std::vector<cl_float> input(kts::N);

  // Using `nueric_limits::min()` for the minimum value excludes the denormals,
  // which are not computed accurately.
  ucl::Environment::instance->GetInputGenerator().GenerateFiniteFloatData(
      input, std::numeric_limits<cl_float>::min());

  // Negative values are not really meaningful for logarithms (result should be
  // NaN), but throw some in, for completeness.
  for (size_t i = 0; i < kts::N; i += 37) {
    input[i] = -input[i];
  }

  AddInputBuffer(kts::N, kts::Reference1D<cl_float>(
                             [&input](size_t id) { return input[id]; }));

  AddOutputBuffer(kts::N, makeAbsoluteErrStreamer<cl_float, 1>(
                              [&input](size_t id) -> cl_float {
                                return std::log2(input[id]);
                              }));
  RunGeneric1D(kts::N);
}

// TODO: extend native precision testing to all of the native builtins, see
// CA-3336 for a full list and details.

} // namespace
