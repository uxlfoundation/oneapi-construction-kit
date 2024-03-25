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

#ifndef UNITCL_KTS_RELATIONALS_H_INCLUDED
#define UNITCL_KTS_RELATIONALS_H_INCLUDED

#include <CL/cl.h>
#include <gtest/gtest.h>

#include <functional>
#include <string>
#include <unordered_map>

#include "cargo/small_vector.h"
#include "ucl/fixtures.h"

namespace kts {
namespace ucl {

/// @brief Class for testing relational OpenCL 1.2 builtins from table 6.14
class RelationalTest : public ::ucl::CommandQueueTest {
 public:
  /// @brief Sets up the text fixture.
  void SetUp() override;

  /// @brief Cleans up OpenCL objects created
  void TearDown() override;

  /// @brief Map of input types to output types for relational builtins
  ///
  /// Scalar types map to cl_int, vector types to the unsigned int equivalent
  /// of the same vector width.
  static const std::unordered_map<std::string, std::string> out_type_map;

  /// @brief Builds an OpenCL program then creates a kernel and sets its args
  ///
  /// @param program Program which has been created but not Built
  ///
  /// @returns Created kernel, or nullptr on error
  cl_kernel BuildKernel(cl_program program);

  /// @brief Populates input buffers with data
  ///
  /// @tparam T floating point scalar type to fill with
  ///
  /// @param num_elements Number of T elements to write to buffer
  template <class T>
  void FillInputBuffers(unsigned num_elements);

  /// @brief Maps all the bytes of the first N buffers to void* pointers
  ///
  /// @tparam N Number of buffers to map
  ///
  /// @param[out] mapped_ptrs Pointers we've mapped
  template <unsigned N>
  void ReadMapBuffers(cargo::small_vector<void *, N> &mapped_ptrs);

  /// @brief Unmaps pointers to the first N buffers
  ///
  /// @tparam N Number of buffers to unmap
  ///
  /// @param[in] mapped_ptrs Pointers we want to unmap
  template <unsigned N>
  void UnmapBuffers(cargo::small_vector<void *, N> &mapped_ptrs);

  /// @brief Calculates the maximum size to allocate for a single buffer
  ///
  /// @return Buffer size in bytes
  cl_ulong GetBufferLimit();

  /// @brief Enqueues a kernel to run in 1D with specified work-items
  ///
  /// @param kernel Kernel to run
  /// @param work_items Number of work-items to scheduled
  void EnqueueKernel(cl_kernel kernel, size_t work_items);

  /// @brief OpenCL programs we've built
  cargo::small_vector<cl_program, 6> programs_;
  /// @brief OpenCL kernels created
  cargo::small_vector<cl_kernel, 6> kernels_;
  /// @brief Buffers to pass to kernels
  cargo::small_vector<cl_mem, 4> buffers_;
  /// @brief Number of bytes to allocate for our OpenCL buffers
  unsigned buffer_size_;
};

/// @brief Tests builtins with a single input argument
class OneArgRelational : public RelationalTest {
 public:
  /// @brief Creates OpenCL buffers to use, and sets buffer_size_
  void SetUp() override;

  /// @brief Called from gtest test fixture to run test
  ///
  /// @tparam T Scalar floating point type to test
  ///
  /// @param builtin Name of builtin function to test
  /// @param ref Reference function to verify results against
  template <class T>
  void TestAgainstReference(const char *builtin,
                            const std::function<bool(T)> &ref);

  /// @brief Create an OpenCL-C program for testing the builtin with 1 input.
  ///
  /// Substitutes builtin name into OpenCL-C kernel string as well as input
  /// and output types then compiles the program and returns kernel for testing.
  ///
  /// @param builtin name of builtin to test
  /// @param type floating point type to test
  ///
  /// @return kernel created, or nullptr on failure
  cl_kernel ConstructProgram(const char *, const std::string &);

  /// @brief Build OpenCL-C source code for program
  static std::string source_fmt_string(std::string extension,
                                       std::string in_type,
                                       std::string out_type,
                                       std::string builtin);
};

/// @brief Tests builtins with a two input arguments
class TwoArgRelational : public RelationalTest {
 public:
  /// @brief Creates OpenCL buffers to use, and sets buffer_size_
  void SetUp() override;

  /// @brief Called from gtest test fixture to run test
  ///
  /// @tparam T Scalar floating point type to test
  ///
  /// @param builtin Name of builtin function to test
  /// @param ref Reference function to verify results against
  template <class T>
  void TestAgainstReference(const char *builtin,
                            const std::function<bool(T, T)> &ref);

  /// @brief Checks kernel result against Flush To Zero behaviour
  ///
  /// @tparam T Scalar floating point type to test
  //
  /// @param ref Reference function to verify device results against
  /// @param a First input parameter
  /// @param b Second input parameter
  /// @param result Outcome returned by device
  ///
  /// @return True if device result is valid FTZ behaviour, false otherwise
  template <class T>
  static bool FTZVerify(const std::function<bool(T, T)> &ref, T a, T b,
                        bool result);

  /// @brief Create an OpenCL-C program for testing the builtin with 2 inputs.
  ///
  /// Substitutes builtin name into OpenCL-C kernel string as well as input
  /// and output types then compiles the program and returns kernel for testing.
  ///
  /// @param builtin name of builtin to test
  /// @param type floating point type to test
  ///
  /// @return kernel created, or nullptr on failure
  cl_kernel ConstructProgram(const char *builtin, const std::string &type);

  /// @brief Build OpenCL-C source code for program
  static std::string source_fmt_string(std::string extension,
                                       std::array<std::string, 2> in_types,
                                       std::string out_type,
                                       std::string builtin);
};

/// @brief Tests builtins with a three input arguments
class ThreeArgRelational : public RelationalTest {
 public:
  /// @brief Creates OpenCL buffers to use, and sets buffer_size_
  void SetUp() override;

  /// @brief Build OpenCL-C source code for program
  static std::string source_fmt_string(std::string extension,
                                       std::array<std::string, 3> in_types,
                                       std::string out_type,
                                       std::string builtin);
};

/// @brief Tests the `bitselect()` builtin
class BitSelectTest : public ThreeArgRelational {
 public:
  /// @brief Called from gtest test fixture to run test
  ///
  /// Needs two template parameters because floats which are signalling NaNs can
  /// have their bit representation changed to be quiet NaNs. So we can't
  /// return a float type from our reference function to check the bitness.
  ///
  /// @tparam T Scalar floating point type to test
  /// @tparam U Unsigned integer type of same size as T
  ///
  /// @param ref Reference function to verify results against
  template <class T, class U>
  void TestAgainstReference(const std::function<U(U, U, U)> &ref);

 private:
  /// @brief Create an OpenCL-C program for testing the bitselect builtin.
  ///
  /// Substitutes type in OpenCL-C kernel string then compiles program
  /// and returns kernel for testing.
  ///
  /// @param test_type floating point type to test
  ///
  /// @return kernel created, or nullptr on failure
  cl_kernel ConstructProgram(const char *test_type);
};

/// @brief Tests the `select()` builtin
class SelectTest : public ThreeArgRelational {
 public:
  /// @brief Called from gtest test fixture to run test
  ///
  /// Needs two template parameters because floats which are signalling NaNs can
  /// have their bit representation changed to be quiet NaNs.
  //
  /// @tparam T Scalar floating point type to test
  /// @tparam U Integer type of same size as T
  ///
  /// @param ref Reference function to verify results against
  /// @param scalar True to just test scalars, False to test vectors. Needed
  ///        because select() has different semantics accordingly.
  template <class T, class U>
  void TestAgainstReference(const std::function<T(T, T, U)> &ref, bool scalar);

 private:
  /// @brief Create an OpenCL-C program for testing the select builtin.
  ///
  /// Substitutes types in OpenCL-C kernel string then compiles program
  /// and returns kernel for testing.
  ///
  /// @param float_type floating point type to test
  /// @param int_type integer type for third argument
  ///
  /// @return kernel created, or nullptr on failure
  cl_kernel ConstructProgram(const char *float_type, const char *int_type);
};

}  // namespace ucl
}  // namespace kts

#endif  // UNITCL_KTS_RELATIONALS_H_INCLUDED
