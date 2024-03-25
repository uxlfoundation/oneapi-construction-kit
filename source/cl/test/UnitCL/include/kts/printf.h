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
#ifndef UNITCL_KTS_PRINTF_H_INCLUDED
#define UNITCL_KTS_PRINTF_H_INCLUDED

// System headers
#include <regex>
#include <string>

// In-house headers
#include "kts/execution.h"
#include "kts/stdout_capture.h"

namespace kts {
namespace ucl {

using ReferencePrintfString = Reference1D<std::string>;
using ReferencePrintfRegex = Reference1D<std::regex>;

/// @brief Abstract class for providing a reference to verify PrintfExecution
/// kernel output against
struct PrintfReference {
  PrintfReference(size_t size) : size(size) {}
  /// @brief Abstract class needs virtual destructor
  virtual ~PrintfReference() = default;

  /// @brief Number of elements in reference, equivalent to number of threads
  /// we want to test the output of
  size_t size;

  /// @brief Verifies that kernel output matches reference at index using
  /// GTEST checks, and if so removes the matched substring from buf.
  /// @param[in] index - Thread id of the reference to verify against
  /// @param[in] buf - Kernel output from PrintfExecution
  /// @return    None
  virtual void Verify(size_t index, std::string &buf) = 0;
};

/// @brief Functionality for verifying that kernel output starts with a
/// reference std::string
struct PrintfStringReference final : PrintfReference {
  PrintfStringReference(size_t size, ReferencePrintfString ref)
      : PrintfReference(size), string_ref(ref) {}

  void Verify(size_t index, std::string &buf) override {
    // String to use as reference for thread-id index
    const std::string ref = string_ref(index);

    // Reference size cannot exceed size of kernel output
    ASSERT_GE(buf.size(), ref.size());

    // Check kernel output starts with reference string
    const std::string cmp = buf.substr(0, ref.size());
    ASSERT_EQ(ref, cmp) << "Output buffer was: " << buf;

    // Remove reference string prefix from kernel output
    buf.erase(0, ref.size());
  }

  /// @brief A string reference to compare against.
  ReferencePrintfString string_ref;
};

/// @brief Functionality for verifying that kernel output contains a reference
/// std::regex
struct PrintfRegexReference final : PrintfReference {
  PrintfRegexReference(size_t size, ReferencePrintfRegex ref)
      : PrintfReference(size), regex_ref(ref) {}

  void Verify(size_t index, std::string &buf) override {
    // Regex to use as reference for thread-id index
    const std::regex ref = regex_ref(index);

    // Check regex is found in kernel output
    std::smatch match;
    const bool found = std::regex_match(buf, match, ref);
    ASSERT_TRUE(found) << "Output buffer was: " << buf;

    // End of reference match cannot exceed size of kernel output
    const size_t match_length = match.length();
    ASSERT_GE(buf.size(), match.position() + match_length);

    // Find start point in kernel output string
    buf.erase(match.position(), match_length);
  }

  /// @brief A regex reference to compare against.
  ReferencePrintfRegex regex_ref;
};

/// @brief Functionality to test kernels that use printf.
struct BasePrintfExecution : BaseExecution {
  /// @brief     Constructor for printf test executor.
  BasePrintfExecution();

  /// @brief     Provide a reference to compare printf output against.
  /// @param[in] size - The total number of threads to emulate.
  /// @param[in] ref - The reference function, containing a string to match
  ///            in the output of tested kernel.
  /// @return    None
  void SetPrintfReference(size_t size, ReferencePrintfString ref);

  /// @brief     Provide a reference to compare printf output against.
  /// @param[in] size - The total number of threads to emulate.
  /// @param[in] ref - The reference function, containing a regex to match
  ///            in the output of tested kernel.
  /// @return    None
  void SetPrintfReference(size_t size, ReferencePrintfRegex ref);

  /// @brief     Similar to RunGenericND, but check printf output as well.
  ///
  ///            The argument list must be populated, and a reference printf
  ///            output provided.
  /// @param[in] numDims - The size of the global dimension.
  /// @param[in] globalDims - The array of sizes of the global dimensions
  /// @param[in] localDims - The array of sizes of the local dimensions
  /// @return    None
  void RunPrintfND(cl_uint numDims, size_t *globalDims, size_t *localDims);

  /// @brief     Similar to RunPrintfND, but in a single dimension
  /// @param[in] globalX - Global size
  /// @param[in] localX - Local workgroup size
  /// @return    None
  void RunPrintf1D(size_t globalX, size_t localX = 0);

  /// @brief     Similar to RunPrintfND, but for concurrent threads.
  ///
  ///            This only checks the total size of text printed, which allows
  ///            the output to be interleaved.
  ///
  ///            The argument list must be populated.
  /// @param[in] numDims - The size of the global dimension.
  /// @param[in] globalDims - The array of sizes of the global dimensions
  /// @param[in] localDims - The array of sizes of the local dimensions
  /// @param[in] expectedTotalPrintSize - The total size of printed text
  /// expected
  /// @return    None
  void RunPrintfNDConcurrent(cl_uint numDims, size_t *globalDims,
                             size_t *localDims, size_t expectedTotalPrintSize);

  /// @brief     Similar to RunPrintf1D, but for concurrent threads.
  ///
  ///            This only checks the total size of text printed, which allows
  ///            the output to be interleaved.
  ///
  ///            The argument list must be populated.
  /// @param[in] globalX - The size of the global dimension.
  /// @param[in] localX - The size of the local dimension.
  /// @param[in] expectedTotalPrintSize - The total size of printed text
  /// expected
  /// @return    None
  void RunPrintf1DConcurrent(size_t globalX, size_t localX,
                             size_t expectedTotalPrintSize);

 private:
  std::unique_ptr<PrintfReference> reader;
  StdoutCapture stdout_capture;
};

struct PrintfExecution : BasePrintfExecution,
                         testing::WithParamInterface<SourceType> {
  PrintfExecution() {
    is_parameterized_ = true;
    source_type = GetParam();
  }

  static std::string getParamName(
      const testing::TestParamInfo<kts::ucl::SourceType> &info) {
    return to_string(info.param);
  }
};

using PrintfExecutionSPIRV = PrintfExecution;

template <class Param>
struct PrintfExecutionWithParam
    : BasePrintfExecution,
      testing::WithParamInterface<std::tuple<SourceType, Param>> {
  PrintfExecutionWithParam() {
    is_parameterized_ = true;
    source_type = std::get<0>(this->GetParam());
  }

  const Param &getParam() const { return std::get<1>(this->GetParam()); }

  static std::string getParamName(
      const testing::TestParamInfo<std::tuple<kts::ucl::SourceType, Param>>
          &info) {
    return to_string(std::get<0>(info.param)) + '_' +
           std::to_string(info.index);
  }
};
}  // namespace ucl
}  // namespace kts

#endif  // UNITCL_KTS_PRINTF_H_INCLUDED
