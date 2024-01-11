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
#ifndef UNITCL_KTS_EXECUTION_H_INCLUDED
#define UNITCL_KTS_EXECUTION_H_INCLUDED

#include <CL/cl.h>
#include <CL/cl_ext.h>

#include <cmath>
#include <memory>
#include <string>

#include "cargo/allocator.h"
#include "kts/arguments.h"
#include "kts/execution_shared.h"
#include "ucl/environment.h"
#include "ucl/fixtures.h"

namespace kts {
namespace ucl {

class ArgumentList;

enum SourceType {
  OPENCL_C,
  SPIRV,
  OFFLINE,
  OFFLINESPIRV,
};

inline std::string to_string(const SourceType &source_type) {
  switch (source_type) {
    case kts::ucl::SourceType::OPENCL_C:
      return "OpenCLC";
    case kts::ucl::SourceType::SPIRV:
      return "SPIRV";
    case kts::ucl::SourceType::OFFLINE:
      return "OfflineOpenCLC";
    case kts::ucl::SourceType::OFFLINESPIRV:
      return "OfflineSPIRV";
    default:
      UCL_ABORT("invalid SourceType: %d\n", source_type);
  }
}

const std::array<SourceType, 4> &getSourceTypes();
const std::array<SourceType, 2> &getOnlineSourceTypes();
const std::array<SourceType, 2> &getOfflineSourceTypes();

// Represents the execution of a test.
struct BaseExecution : ::ucl::CommandQueueTest, SharedExecution {
  BaseExecution();

  /// @brief Sets up the test fixture.
  virtual void SetUp() override;

  /// @brief Tears down the test fixture.
  virtual void TearDown() override;

  /// @brief Return pointer to the argument list constructed for this test.
  ArgumentList &GetArgumentList();

  void AddInputBuffer(BufferDesc &&desc);
  inline void AddInputBuffer(size_t size,
                             std::shared_ptr<BufferStreamer> streamer);
  template <typename T>
  void AddInputBuffer(size_t size, Reference1D<T> ref);
  template <typename T>
  void AddInputBuffer(size_t size, Reference1DPtr<T> ref);

  void AddOutputBuffer(BufferDesc &&desc);
  inline void AddOutputBuffer(size_t size,
                              std::shared_ptr<BufferStreamer> streamer);
  template <typename T>
  void AddOutputBuffer(size_t size, Reference1D<T> ref);
  template <typename T>
  void AddOutputBuffer(size_t size, Reference1DPtr<T> ref);

  void AddInOutBuffer(BufferDesc &&desc);
  inline void AddInOutBuffer(size_t size,
                             std::shared_ptr<BufferStreamer> streamer,
                             std::shared_ptr<BufferStreamer> streamer2);
  template <typename T>
  void AddInOutBuffer(size_t size, Reference1D<T> refIn, Reference1D<T> refOut);
  template <typename T>
  void AddInOutBuffer(size_t size, Reference1DPtr<T> refIn,
                      Reference1DPtr<T> refOut);
  template <typename T>
  void AddInOutBuffer(size_t size, Reference1D<T> refIn,
                      Reference1DPtr<T> refOut);
  template <typename T>
  void AddInOutBuffer(size_t size, Reference1DPtr<T> refIn,
                      Reference1D<T> refOut);

  void AddLocalBuffer(size_t nelm, size_t elmsize);

  template <typename T>
  void AddLocalBuffer(size_t size) {
    AddLocalBuffer(size, sizeof(T));
  }

  template <typename T>
  void AddInputImage(const cl_image_format &format, const cl_image_desc &desc,
                     size_t size, Reference1D<T> ref);

  void AddSampler(cl_bool normalized_coords, cl_addressing_mode addressing_mode,
                  cl_filter_mode filter_mode);

  template <typename T>
  void AddPrimitive(T value);

  /// @brief Build the kernel program. The file prefix and kernel names are
  /// determined from the current test name.
  ///
  /// @return Boolean value indicating success of build.
  /// @retval True if program successfully built
  /// @retval False if program did not successfully build or if test should be
  /// skipped
  [[nodiscard]] bool BuildProgram();

  /// @brief Build the kernel program.
  ///
  /// @param file_prefix Where to find kernel source.
  /// @param kernel_name The name of the kernel to build.
  ///
  /// @return Boolean value indicating success of build.
  /// @retval True if program successfully built
  /// @retval False if program did not successfully build or if test should be
  /// skipped
  [[nodiscard]] virtual bool BuildProgram(std::string file_prefix,
                                          std::string kernel_name);

  /// @brief Check whether the kernel was vectorized or not.
  bool CheckVectorized();

  void AddBuildOption(std::string option);

  void AddMacro(std::string name, unsigned value);
  void AddMacro(const std::string &name, const std::string &value);

  /// @brief Build and run a 1-D kernel for the given test.
  ///
  ///        The argument list must be populated.
  void RunGeneric1D(size_t globalX, size_t localX = 0);

  /// @brief Build and run a N-D kernel for the given test.
  void RunGenericND(cl_uint numDims, const size_t *globalDims,
                    const size_t *localDims);

  bool isSourceTypeIn(std::initializer_list<SourceType> source_types);

 protected:
  // Load the kernel source from the given path.
  bool LoadSource(const std::string &path);

  std::string GetSourcePath(const std::string &file_prefix,
                            const std::string &kernel_name);

  using MacroDef = std::pair<std::string, std::string>;

  struct EnqueueDimensions {
    cl_uint count;
    std::vector<size_t> global;
    std::vector<size_t> local;
  };

  cl_program program_;
  cl_kernel kernel_;
  std::string source_;
  std::unique_ptr<ArgumentList> args_;
  std::vector<MacroDef> macros_;
  SourceType source_type;
  bool fail_if_not_vectorized_;
  std::string build_options_;  // Appended to those passed on command line
  std::vector<EnqueueDimensions> dims_to_test_;

  clGetKernelWFVInfoCODEPLAY_fn clGetKernelWFVInfoCODEPLAY = nullptr;
  clCreateProgramWithILKHR_fn clCreateProgramWithILKHR = nullptr;
  /// @brief Controls whether to fail the test if BuildProgram fails.
  ///
  /// When set to true, BuildProgram will fail the test if any of the OpenCL
  /// build APIs (clCreateProgramWithXXX, clBuildProgram, clCreateKernel) fail.
  /// When set to false, BuildProgram will silently return false, leaving the
  /// caller to handle the error.
  bool fail_if_build_program_failed = true;
};

struct Execution : BaseExecution, testing::WithParamInterface<SourceType> {
  Execution() {
    is_parameterized_ = true;
    source_type = GetParam();
  }

  static std::string getParamName(
      const testing::TestParamInfo<kts::ucl::SourceType> &info) {
    return to_string(info.param);
  }
};

using ExecutionOpenCLC = Execution;
using ExecutionSPIRV = Execution;
using ExecutionOnline = Execution;
using ExecutionOffline = Execution;

template <typename T>
struct ExecutionWithParam
    : BaseExecution,
      testing::WithParamInterface<std::tuple<SourceType, T>> {
  ExecutionWithParam() {
    is_parameterized_ = true;
    source_type = std::get<0>(this->GetParam());
  }

  const T &getParam() const { return std::get<1>(this->GetParam()); }

  static std::string getParamName(
      const testing::TestParamInfo<std::tuple<kts::ucl::SourceType, T>> &info) {
    return to_string(std::get<0>(info.param)) + '_' +
           std::to_string(info.index);
  }
};

#define UCL_EXECUTION_TEST_SUITE(FIXTURE, SOURCE_TYPES)      \
  INSTANTIATE_TEST_SUITE_P(Execution, FIXTURE, SOURCE_TYPES, \
                           FIXTURE::getParamName);

#define UCL_EXECUTION_TEST_SUITE_P(FIXTURE, SOURCE_TYPES, VALUES)  \
  INSTANTIATE_TEST_SUITE_P(Execution, FIXTURE,                     \
                           testing::Combine(SOURCE_TYPES, VALUES), \
                           FIXTURE::getParamName);

// User defined ULP literal
constexpr cl_ulong operator"" _ULP(unsigned long long int ulp) {
  return static_cast<cl_ulong>(ulp);
}

/// @brief Encapsulates test setup, run, and verification code for fp16 testing
class HalfParamExecution : public ExecutionWithParam<unsigned> {
 public:
  /// @brief Test half precision functions with a single float input
  /// @tparam ULP Precision requirement of result
  /// @param[in] Reference function to verify results against
  template <cl_ulong ULP>
  void TestAgainstRef(const std::function<cl_float(cl_float)> &);

  /// @brief Test half precision functions with two float inputs
  /// @tparam ULP Precision requirement of result
  /// @param[in] Reference function to verify results against
  /// @param[in] Optional reference function for undefined behaviour
  template <cl_ulong ULP>
  void TestAgainstRef(
      const std::function<cl_float(cl_float, cl_float)> &,
      const std::function<bool(cl_float, cl_float)> * = nullptr);

  /// @brief Test half precision functions with three float inputs
  /// @tparam ULP Precision requirement of result
  /// @param[in] Reference function to verify results against
  /// @param[in] Optional reference function for undefined behaviour
  template <cl_ulong ULP>
  void TestAgainstRef(
      const std::function<cl_float(cl_float, cl_float, cl_float)> &,
      const std::function<bool(cl_float, cl_float, cl_float)> * = nullptr);

  /// @brief Test half precision functions with one float and one int input
  /// @tparam ULP Precision requirement of result
  /// @param[in] Reference function to verify results against
  template <cl_ulong ULP>
  void TestAgainstIntArgRef(const std::function<cl_float(cl_float, cl_int)> &);

  /// @brief Test half precision functions with a float input, and one int
  ///        output
  /// @tparam ULP Precision requirement of result
  /// @param[in] Reference function to verify results against
  template <cl_ulong ULP>
  void TestAgainstIntReferenceArgRef(
      const std::function<cl_float(cl_float, cl_int &)> &);

  /// @brief Test half precision functions with two float inputs, and an int
  /// output
  /// @tparam ULP Precision requirement of result
  /// @param[in] Reference function to verify results against
  template <cl_ulong ULP>
  void TestAgainstIntReferenceArgRef(
      const std::function<cl_float(cl_float, cl_float, cl_int &)> &);

  /// @brief Test half precision functions with one float input, and a float
  ///        output
  /// @tparam ULP Precision requirement of result
  /// @param[in] Reference function to verify results against
  template <cl_ulong ULP>
  void TestAgainstFloatReferenceArgRef(
      const std::function<cl_float(cl_float, cl_float &)> &);

  /// @brief Test half precision functions with a float input, and int output
  /// @param[in] Reference function to verify results against
  void TestAgainstIntReturn(const std::function<cl_int(cl_float)> &);

  /// @brief Initializes the scalar_arg_indices member variable
  /// @param[in] Vector containing indices of scalar args to set
  void InitScalarArgIndices(const std::vector<unsigned> &&);

 protected:
  /// @brief Encapsulates parameter type information with input buffer
  struct input_details_t final {
    input_details_t(unsigned idx) : arg_index(idx), is_scalar(false) {}

    size_t length() const { return data.size(); }

    cl_half &operator[](size_t idx) { return data[idx]; }
    const cl_half &operator()(size_t idx) const { return data[idx]; }

    /// @brief Index from 0 of argument in kernel parameters
    unsigned arg_index;
    /// @brief Input buffer tied to parameter
    std::vector<cl_half> data;
    /// @brief Scalar argument type, differentiates between overloads
    bool is_scalar;
  };

  /// @brief Sets program macro for vector width to test
  unsigned SetMacro();

  /// @brief Populate half precision input buffers with data
  /// @param[in] List of buffers to fill
  /// @return The number of elements allocated in buffers
  template <size_t N>
  unsigned FillInputBuffers(std::array<input_details_t, N> &);

  /// @brief If function signature has scalar input arguments, even when
  ///        testing vector types for output and other input arguments then
  ///        the indices of the scalar parameters are set here.
  std::vector<unsigned> scalar_arg_indices;

  /// @brief Returns true if parameter at index has a scalar type
  /// @param[in] Index of parameter to check
  bool IsArgScalar(unsigned index) const;

  /// @brief Returns a vector of edge cases that need extra testing.
  virtual const std::vector<cl_ushort> &GetEdgeCases() const;
};

template <typename T>
void BaseExecution::AddPrimitive(T value) {
  // UnitCL AddPrimitive requires this to be allocated with cargo::alloc.  We
  // potentially over-align the BoxedPrimitive because if T is an OpenCL vector
  // type then it may have higher alignment requirements than a raw `malloc`
  // (or pre-C++17 `new`) will provide.
  void *raw = cargo::alloc(sizeof(BoxedPrimitive<T>), sizeof(cl_long16));
  BoxedPrimitive<T> *boxed_primitive = new (raw) BoxedPrimitive<T>(value);
  args_->AddPrimitive(boxed_primitive);
}

void BaseExecution::AddInputBuffer(size_t size,
                                   std::shared_ptr<BufferStreamer> streamer) {
  args_->AddInputBuffer(BufferDesc(size, streamer));
}

template <typename T>
void BaseExecution::AddInputBuffer(size_t size, Reference1D<T> ref) {
  args_->AddInputBuffer(BufferDesc(size, ref));
}

template <typename T>
void BaseExecution::AddInputBuffer(size_t size, Reference1DPtr<T> ref) {
  args_->AddInputBuffer(BufferDesc(size, Reference1D<T>(ref)));
}

void BaseExecution::AddOutputBuffer(size_t size,
                                    std::shared_ptr<BufferStreamer> streamer) {
  args_->AddOutputBuffer(BufferDesc(size, streamer));
}

template <typename T>
void BaseExecution::AddOutputBuffer(size_t size, Reference1D<T> ref) {
  args_->AddOutputBuffer(BufferDesc(size, ref));
}

template <typename T>
void BaseExecution::AddOutputBuffer(size_t size, Reference1DPtr<T> ref) {
  args_->AddOutputBuffer(BufferDesc(size, Reference1D<T>(ref)));
}

void BaseExecution::AddInOutBuffer(size_t size,
                                   std::shared_ptr<BufferStreamer> streamer,
                                   std::shared_ptr<BufferStreamer> streamer2) {
  args_->AddInOutBuffer(BufferDesc(size, streamer, streamer2));
}

template <typename T>
void BaseExecution::AddInOutBuffer(size_t size, Reference1D<T> refIn,
                                   Reference1D<T> refOut) {
  args_->AddInOutBuffer(BufferDesc(size, refIn, refOut));
}

template <typename T>
void BaseExecution::AddInOutBuffer(size_t size, Reference1DPtr<T> refIn,
                                   Reference1DPtr<T> refOut) {
  args_->AddInOutBuffer(
      BufferDesc(size, Reference1D<T>(refIn), Reference1D<T>(refOut)));
}

template <typename T>
void BaseExecution::AddInOutBuffer(size_t size, Reference1D<T> refIn,
                                   Reference1DPtr<T> refOut) {
  args_->AddInOutBuffer(BufferDesc(size, refIn, Reference1D<T>(refOut)));
}

template <typename T>
void BaseExecution::AddInOutBuffer(size_t size, Reference1DPtr<T> refIn,
                                   Reference1D<T> refOut) {
  args_->AddInOutBuffer(BufferDesc(size, Reference1D<T>(refIn), refOut));
}

template <typename T>
void BaseExecution::AddInputImage(const cl_image_format &format,
                                  const cl_image_desc &desc, size_t size,
                                  Reference1D<T> ref) {
  args_->AddInputImage(format, desc, BufferDesc(size, Reference1D<T>(ref)));
}
}  // namespace ucl
}  // namespace kts

#endif  // UNITCL_KTS_EXECUTION_H_INCLUDED
