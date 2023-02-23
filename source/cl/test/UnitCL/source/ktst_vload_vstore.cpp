// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <gtest/gtest.h>

#include "Common.h"
#include "kts/precision.h"

using namespace kts::ucl;

using HalfVloadVstoreTests = ExecutionWithParam<size_t>;
TEST_P(HalfVloadVstoreTests, Vloadvstore_01_Half_Global) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  const size_t vec_width = getParam();

  std::string type("half");
  std::string load_func("vload");
  std::string store_func("vstore");
  std::string vec_str = std::to_string(vec_width);

  AddMacro("HALFN", type + vec_str);
  AddMacro("LOADN", load_func + vec_str);
  AddMacro("STOREN", store_func + vec_str);

  kts::Reference1D<cl_half> refIn = [](size_t id) {
    const auto &inputs = InputGenerator::half_edge_cases;
    return cargo::bit_cast<cl_half>(inputs[id % inputs.size()]);
  };

  kts::Reference1D<cl_half> refOut = [](size_t id) {
    const auto &inputs = InputGenerator::half_edge_cases;
    return cargo::bit_cast<cl_half>(inputs[id % inputs.size()]);
  };

  const size_t N = InputGenerator::half_edge_cases.size();
  const size_t elements = N * vec_width;

  AddInputBuffer(elements, refIn);
  AddOutputBuffer(elements, refOut);
  RunGeneric1D(N);
}

TEST_P(HalfVloadVstoreTests, Vloadvstore_02_Half_Local) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  const size_t vec_width = getParam();

  std::string type("half");
  std::string load_func("vload");
  std::string store_func("vstore");
  std::string vec_str = std::to_string(vec_width);

  AddMacro("HALFN", type + vec_str);
  AddMacro("LOADN", load_func + vec_str);
  AddMacro("STOREN", store_func + vec_str);

  kts::Reference1D<cl_half> refIn = [](size_t id) {
    const auto &inputs = InputGenerator::half_edge_cases;
    return cargo::bit_cast<cl_half>(inputs[id % inputs.size()]);
  };

  kts::Reference1D<cl_half> refOut = [](size_t id) {
    const auto &inputs = InputGenerator::half_edge_cases;
    return cargo::bit_cast<cl_half>(inputs[id % inputs.size()]);
  };

  const size_t N = InputGenerator::half_edge_cases.size();
  const size_t elements = N * vec_width;

  AddInputBuffer(elements, refIn);
  AddLocalBuffer(elements * sizeof(cl_half));
  AddOutputBuffer(elements, refOut);
  RunGeneric1D(N, N);
}

// TODO: CA-2731
#ifdef __arm__
TEST_P(HalfVloadVstoreTests, DISABLED_Vloadvstore_03_Half_Private) {
#else
TEST_P(HalfVloadVstoreTests, Vloadvstore_03_Half_Private) {
#endif
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }
  fail_if_not_vectorized_ = false;

  const size_t vec_width = getParam();

  std::string type("half");
  std::string load_func("vload");
  std::string store_func("vstore");
  std::string vec_str = std::to_string(vec_width);

  AddMacro("HALFN", type + vec_str);
  AddMacro("LOADN", load_func + vec_str);
  AddMacro("STOREN", store_func + vec_str);

  kts::Reference1D<cl_half> refIn = [](size_t id) {
    const auto &inputs = InputGenerator::half_edge_cases;
    return cargo::bit_cast<cl_half>(inputs[id % inputs.size()]);
  };

  kts::Reference1D<cl_half> refOut = [](size_t id) {
    const auto &inputs = InputGenerator::half_edge_cases;
    return cargo::bit_cast<cl_half>(inputs[id % inputs.size()]);
  };

  const cl_uint iterations = 32;
  AddMacro("ARRAY_LEN", vec_width * iterations);

  const size_t N = InputGenerator::half_edge_cases.size();
  const size_t elements = N * vec_width;

  AddInputBuffer(elements, refIn);
  AddPrimitive(iterations);
  AddOutputBuffer(elements, refOut);
  RunGeneric1D(N);
}

TEST_P(HalfVloadVstoreTests, Vloadvstore_04_Half_Constant) {
  if (!UCL::hasHalfSupport(device)) {
    GTEST_SKIP();
  }

  const size_t vec_width = getParam();

  std::string type("half");
  std::string load_func("vload");
  std::string store_func("vstore");
  std::string vec_str = std::to_string(vec_width);

  AddMacro("HALFN", type + vec_str);
  AddMacro("LOADN", load_func + vec_str);
  AddMacro("STOREN", store_func + vec_str);

  kts::Reference1D<cl_half> refIn = [](size_t id) {
    const auto &inputs = InputGenerator::half_edge_cases;
    return cargo::bit_cast<cl_half>(inputs[id % inputs.size()]);
  };

  kts::Reference1D<cl_half> refOut = [](size_t id) {
    const auto &inputs = InputGenerator::half_edge_cases;
    return cargo::bit_cast<cl_half>(inputs[id % inputs.size()]);
  };

  const size_t N = InputGenerator::half_edge_cases.size();
  const size_t elements = N * vec_width;

  AddInputBuffer(elements, refIn);
  AddOutputBuffer(elements, refOut);
  RunGeneric1D(N);
}

UCL_EXECUTION_TEST_SUITE_P(HalfVloadVstoreTests, testing::Values(OPENCL_C),
                           testing::Values(2u, 3u, 4u, 8u, 16u));
