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

#include <kts/precision.h>

#include <numeric>
#include <vector>

#include "Common.h"
#include "kts/reference_functions.h"

using namespace kts::ucl;

static const kts::ucl::SourceType source_types[] = {
    kts::ucl::OPENCL_C, kts::ucl::OFFLINE, kts::ucl::SPIRV,
    kts::ucl::OFFLINESPIRV};

class C11AtomicTestBase : public kts::ucl::Execution {
 public:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(BaseExecution::SetUp());
    // The C11 atomic were introduced in 2.0 however here we only test
    // the minimum required subset for 3.0.
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }

    this->AddBuildOption("-cl-std=CL3.0");
  }
};

class InitTest : public C11AtomicTestBase {
 public:
  template <typename T>
  void doTest(bool local = false) {
    // Generate the random input.
    std::vector<T> input_data(kts::N, T{});
    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

    // Set up references.
    kts::Reference1D<T> random_reference = [&input_data](size_t index) {
      return input_data[index];
    };

    // Set up the buffers and run the test.
    AddInputBuffer(kts::N, random_reference);
    AddOutputBuffer(kts::N, random_reference);
    if (local) {
      AddLocalBuffer<T>(kts::localN);
      RunGeneric1D(kts::N, kts::localN);
    } else {
      RunGeneric1D(kts::N);
    }
  }
};

TEST_P(InitTest, C11Atomics_01_Init_Global_Int) { doTest<cl_int>(); }
TEST_P(InitTest, C11Atomics_01_Init_Global_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>();
}
TEST_P(InitTest, C11Atomics_01_Init_Global_Uint) { doTest<cl_uint>(); }
TEST_P(InitTest, C11Atomics_01_Init_Global_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>();
}
TEST_P(InitTest, C11Atomics_01_Init_Global_Float) { doTest<cl_float>(); }
TEST_P(InitTest, C11Atomics_01_Init_Global_Double) {
  if (!UCL::hasAtomic64Support(device) || !UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }
  doTest<cl_double>();
}

TEST_P(InitTest, C11Atomics_02_Init_Local_Int) {
  doTest<cl_int>(/*local*/ true);
}
TEST_P(InitTest, C11Atomics_02_Init_Local_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>(/*local*/ true);
}
TEST_P(InitTest, C11Atomics_02_Init_Local_Uint) {
  doTest<cl_uint>(/*local*/ true);
}
TEST_P(InitTest, C11Atomics_02_Init_Local_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>(/*local*/ true);
}
TEST_P(InitTest, C11Atomics_02_Init_Local_Float) {
  doTest<cl_float>(/*local*/ true);
}
TEST_P(InitTest, C11Atomics_02_Init_Local_Double) {
  if (!UCL::hasAtomic64Support(device) || !UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }
  doTest<cl_double>(/*local*/ true);
}

UCL_EXECUTION_TEST_SUITE(InitTest, testing::ValuesIn(source_types));

class FenceTest : public C11AtomicTestBase {};

TEST_P(FenceTest, C11Atomics_03_Fence_Acquire_Release) {
  // Set up the buffers.
  this->AddInputBuffer(kts::N, kts::Ref_Identity);
  kts::Reference1D<cl_int> zero_reference = [](size_t) { return cl_int{0}; };
  this->AddInputBuffer(kts::N, zero_reference);
  this->AddOutputBuffer(kts::N, kts::Ref_Identity);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TEST_P(FenceTest, C11Atomics_04_Fence_Acquire) {
  // Set up the buffers.
  this->AddInputBuffer(kts::N, kts::Ref_Identity);
  kts::Reference1D<cl_int> zero_reference = [](size_t) { return cl_int{0}; };
  this->AddInputBuffer(kts::N, zero_reference);
  this->AddOutputBuffer(kts::N, kts::Ref_Identity);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TEST_P(FenceTest, C11Atomics_05_Fence_Release) {
  // Set up the buffers.
  this->AddInputBuffer(kts::N, kts::Ref_Identity);
  kts::Reference1D<cl_int> zero_reference = [](size_t) { return cl_int{0}; };
  this->AddInputBuffer(kts::N, zero_reference);
  this->AddOutputBuffer(kts::N, kts::Ref_Identity);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TEST_P(FenceTest, C11Atomics_06_Fence_Relaxed) {
  // Set up the buffers.
  this->AddInputBuffer(kts::N, kts::Ref_Identity);
  kts::Reference1D<cl_int> zero_reference = [](size_t) { return cl_int{0}; };
  this->AddInputBuffer(kts::N, zero_reference);
  this->AddOutputBuffer(kts::N, kts::Ref_Identity);

  this->AddBuildOption("-cl-std=CL3.0");

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TEST_P(FenceTest, C11Atomics_07_Fence_Global) {
  // Set up the buffers.
  this->AddInputBuffer(kts::N, kts::Ref_Identity);
  kts::Reference1D<cl_int> zero_reference = [](size_t) { return cl_int{0}; };
  this->AddInputBuffer(kts::N, zero_reference);
  this->AddOutputBuffer(kts::N, kts::Ref_Identity);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TEST_P(FenceTest, C11Atomics_08_Fence_Local) {
  // Set up the buffers.
  this->AddInputBuffer(kts::N, kts::Ref_Identity);
  this->AddOutputBuffer(kts::N, kts::Ref_Identity);
  this->AddLocalBuffer<cl_int>(kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

UCL_EXECUTION_TEST_SUITE(FenceTest, testing::ValuesIn(source_types));

class LoadStoreTest : public C11AtomicTestBase {
 public:
  template <typename T>
  void doTest(bool local = false) {
    // Generate the random input.
    std::vector<T> input_data(kts::N, T{});
    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

    kts::Reference1D<T> random_reference = [&input_data](size_t index) {
      return input_data[index];
    };

    // Set up the buffers and run the test.
    AddInputBuffer(kts::N, random_reference);
    AddOutputBuffer(kts::N, random_reference);
    if (local) {
      AddLocalBuffer<T>(kts::localN);
      RunGeneric1D(kts::N, kts::localN);
    } else {
      RunGeneric1D(kts::N);
    }
  }
};

TEST_P(LoadStoreTest, C11Atomics_09_Store_Local_Int) {
  doTest<cl_int>(/*local*/ true);
}
TEST_P(LoadStoreTest, C11Atomics_09_Store_Local_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>(/*local*/ true);
}
TEST_P(LoadStoreTest, C11Atomics_09_Store_Local_Uint) {
  doTest<cl_uint>(/*local*/ true);
}
TEST_P(LoadStoreTest, C11Atomics_09_Store_Local_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>(/*local*/ true);
}
TEST_P(LoadStoreTest, C11Atomics_09_Store_Local_Float) {
  doTest<cl_float>(/*local*/ true);
}
TEST_P(LoadStoreTest, C11Atomics_09_Store_Local_Double) {
  if (!UCL::hasAtomic64Support(device) || !UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }
  doTest<cl_double>(/*local*/ true);
}

TEST_P(LoadStoreTest, C11Atomics_10_Store_Global_Int) { doTest<cl_int>(); }
TEST_P(LoadStoreTest, C11Atomics_10_Store_Global_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>();
}
TEST_P(LoadStoreTest, C11Atomics_10_Store_Global_Uint) { doTest<cl_uint>(); }
TEST_P(LoadStoreTest, C11Atomics_10_Store_Global_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>();
}
TEST_P(LoadStoreTest, C11Atomics_10_Store_Global_Float) { doTest<cl_float>(); }
TEST_P(LoadStoreTest, C11Atomics_10_Store_Global_Double) {
  if (!UCL::hasAtomic64Support(device) || !UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }
  doTest<cl_double>();
}

TEST_P(LoadStoreTest, C11Atomics_11_Load_Local_Int) {
  doTest<cl_int>(/*local*/ true);
}
TEST_P(LoadStoreTest, C11Atomics_11_Load_Local_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>(/*local*/ true);
}
TEST_P(LoadStoreTest, C11Atomics_11_Load_Local_Uint) {
  doTest<cl_uint>(/*local*/ true);
}
TEST_P(LoadStoreTest, C11Atomics_11_Load_Local_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>(/*local*/ true);
}
TEST_P(LoadStoreTest, C11Atomics_11_Load_Local_Float) {
  doTest<cl_float>(/*local*/ true);
}
TEST_P(LoadStoreTest, C11Atomics_11_Load_Local_Double) {
  if (!UCL::hasAtomic64Support(device) || !UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }
  doTest<cl_double>(/*local*/ true);
}

TEST_P(LoadStoreTest, C11Atomics_12_Load_Global_Int) { doTest<cl_int>(); }
TEST_P(LoadStoreTest, C11Atomics_12_Load_Global_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>();
}
TEST_P(LoadStoreTest, C11Atomics_12_Load_Global_Uint) { doTest<cl_uint>(); }
TEST_P(LoadStoreTest, C11Atomics_12_Load_Global_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>();
}
TEST_P(LoadStoreTest, C11Atomics_12_Load_Global_Float) { doTest<cl_float>(); }
TEST_P(LoadStoreTest, C11Atomics_12_Load_Global_Double) {
  if (!UCL::hasAtomic64Support(device) || !UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }
  doTest<cl_double>();
}

UCL_EXECUTION_TEST_SUITE(LoadStoreTest, testing::ValuesIn(source_types));

class ExchangeTest : public C11AtomicTestBase {
 public:
  template <typename T>
  void doTest(bool local = false) {
    // Generate the random input.
    std::vector<T> initializer_data(kts::N, T{});
    ucl::Environment::instance->GetInputGenerator().GenerateData(
        initializer_data);

    std::vector<T> desired_data(kts::N, T{});
    ucl::Environment::instance->GetInputGenerator().GenerateData(desired_data);

    kts::Reference1D<T> initializer_reference =
        [&initializer_data](size_t index) { return initializer_data[index]; };

    kts::Reference1D<T> desired_reference = [&desired_data](size_t index) {
      return desired_data[index];
    };

    // Set up the buffers and run the test.
    // The initial values of the atomics are the input.
    // The desired values exchanged into the atomics are the expected output.
    AddInOutBuffer(kts::N, initializer_reference, desired_reference);
    // The desired values exchanged into the atomics are the input.
    AddInputBuffer(kts::N, desired_reference);
    // The initial atomic values exchanged out of the atomics are the expected
    // output.
    AddOutputBuffer(kts::N, initializer_reference);
    if (local) {
      AddLocalBuffer<T>(kts::localN);
      RunGeneric1D(kts::N, kts::localN);
    } else {
      RunGeneric1D(kts::N);
    }
  }
};

TEST_P(ExchangeTest, C11Atomics_13_Exchange_Local_Int) {
  doTest<cl_int>(/*local*/ true);
}
TEST_P(ExchangeTest, C11Atomics_13_Exchange_Local_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>(/*local*/ true);
}
TEST_P(ExchangeTest, C11Atomics_13_Exchange_Local_Uint) {
  doTest<cl_uint>(/*local*/ true);
}
TEST_P(ExchangeTest, C11Atomics_13_Exchange_Local_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>(/*local*/ true);
}
TEST_P(ExchangeTest, C11Atomics_13_Exchange_Local_Float) {
  doTest<cl_float>(/*local*/ true);
}
TEST_P(ExchangeTest, C11Atomics_13_Exchange_Local_Double) {
  if (!UCL::hasAtomic64Support(device) || !UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }
  doTest<cl_double>(/*local*/ true);
}

TEST_P(ExchangeTest, C11Atomics_14_Exchange_Global_Int) { doTest<cl_int>(); }
TEST_P(ExchangeTest, C11Atomics_14_Exchange_Global_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>();
}
TEST_P(ExchangeTest, C11Atomics_14_Exchange_Global_Uint) { doTest<cl_uint>(); }
TEST_P(ExchangeTest, C11Atomics_14_Exchange_Global_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>();
}
TEST_P(ExchangeTest, C11Atomics_14_Exchange_Global_Float) {
  doTest<cl_float>();
}
TEST_P(ExchangeTest, C11Atomics_14_Exchange_Global_Double) {
  if (!UCL::hasAtomic64Support(device) || !UCL::hasDoubleSupport(device)) {
    GTEST_SKIP();
  }
  doTest<cl_double>();
}

UCL_EXECUTION_TEST_SUITE(ExchangeTest, testing::ValuesIn(source_types));

class FlagTest : public kts::ucl::Execution {
 protected:
  static const kts::Reference1D<cl_int> false_reference;
  static const kts::Reference1D<cl_int> true_reference;
};

const kts::Reference1D<cl_int> FlagTest::false_reference = [](size_t) {
  return CL_FALSE;
};
const kts::Reference1D<cl_int> FlagTest::true_reference = [](size_t) {
  return CL_TRUE;
};

TEST_P(FlagTest, C11Atomics_17_Flag_Local_Clear_Set) {
  // The C11 atomic were introduced in 2.0 however here we only test
  // the minimum required subset for 3.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  this->AddBuildOption("-cl-std=CL3.0");

  // Set up the buffers.
  // The expected output is that the local atomic flags are all unset
  // by the kernel.
  this->AddOutputBuffer(kts::N, false_reference);
  this->AddLocalBuffer<cl_bool>(kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TEST_P(FlagTest, C11Atomics_18_Flag_Local_Set_Twice) {
  // The C11 atomic were introduced in 2.0 however here we only test
  // the minimum required subset for 3.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  this->AddBuildOption("-cl-std=CL3.0");

  // Set up the buffers.
  // The expected output is that the local atomic flags are all set
  // by the kernel.
  this->AddOutputBuffer(kts::N, true_reference);
  this->AddLocalBuffer<cl_bool>(kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TEST_P(FlagTest, C11Atomics_19_Flag_Global_Clear) {
  // The C11 atomic were introduced in 2.0 however here we only test
  // the minimum required subset for 3.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  this->AddBuildOption("-cl-std=CL3.0");

  // Set up the buffers.
  // The input is that all flags are set.
  // The expected output is that all flags are unset by the kernel.
  this->AddInOutBuffer(kts::N, true_reference, false_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TEST_P(FlagTest, C11Atomics_20_Flag_Global_Set_Once) {
  // The C11 atomic were introduced in 2.0 however here we only test
  // the minimum required subset for 3.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  this->AddBuildOption("-cl-std=CL3.0");

  // Set up the buffers.
  // The input is that all flags are unset.
  // The expected output is that all flags are set by the kernel.
  this->AddInOutBuffer(kts::N, false_reference, true_reference);
  this->AddOutputBuffer(kts::N, false_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

UCL_EXECUTION_TEST_SUITE(FlagTest, testing::ValuesIn(source_types));

class FetchTest : public C11AtomicTestBase {
 protected:
  template <typename T>
  void doCheckReturnTest(bool local = false) {
    // Generate the random input.
    std::vector<T> input_data(kts::N, T{});
    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

    kts::Reference1D<T> random_reference = [&input_data](size_t index) {
      return input_data[index];
    };

    // Set up the buffers and run the test.
    // The initial values of the atomics are the random input.
    AddInputBuffer(kts::N, random_reference);
    // The expected output values are the initial values loaded atomically.
    AddOutputBuffer(kts::N, random_reference);
    if (local) {
      AddLocalBuffer<T>(kts::localN);
      RunGeneric1D(kts::N, kts::localN);
    } else {
      RunGeneric1D(kts::N);
    }
  }

  template <typename T>
  void doTest(
      const std::function<T(size_t, const std::vector<T> &)> &init_ref_fn,
      const std::function<T(size_t, const std::vector<T> &)> &op_ref_fn,
      bool clamp = false) {
    // Generate the random input.
    std::vector<T> input_data(kts::N, T{});
    if (!clamp) {
      ucl::Environment::instance->GetInputGenerator().GenerateIntData(
          input_data);
    } else {
      // We need to be careful we don't overflow, so limit the min and max
      // values.
      const T min = std::numeric_limits<T>::min() / static_cast<T>(kts::N);
      const T max = std::numeric_limits<T>::max() / static_cast<T>(kts::N);
      ucl::Environment::instance->GetInputGenerator().GenerateIntData(
          input_data, min, max);
    }
    kts::Reference1D<T> random_reference = [&input_data](size_t index) {
      return input_data[index];
    };

    kts::Reference1D<T> init_reference = [&input_data,
                                          &init_ref_fn](size_t index) {
      return init_ref_fn(index, input_data);
    };
    kts::Reference1D<T> op_reference = [&input_data, &op_ref_fn](size_t index) {
      return op_ref_fn(index, input_data);
    };

    // Set up the buffers.
    // The initial values, and the expected output values
    AddInOutBuffer(1, init_reference, op_reference);
    // The input values to be summed.
    AddInputBuffer(kts::N, random_reference);

    // Run the test.
    RunGeneric1D(kts::N);
  }

  template <typename T>
  void doLocalTest(
      const std::function<T(size_t, const std::vector<T> &)> &init_ref_fn,
      const std::function<T(size_t, const std::vector<T> &)> &op_ref_fn,
      bool clamp = false) {
    // Generate the random input.
    std::vector<T> input_data(kts::N, T{});
    if (!clamp) {
      ucl::Environment::instance->GetInputGenerator().GenerateIntData(
          input_data);
    } else {
      // We need to be careful we don't overflow, so limit the min and max
      // values.
      const T min = std::numeric_limits<T>::min() / static_cast<T>(kts::N);
      const T max = std::numeric_limits<T>::max() / static_cast<T>(kts::N);
      ucl::Environment::instance->GetInputGenerator().GenerateIntData(
          input_data, min, max);
    }

    kts::Reference1D<T> random_reference = [&input_data](size_t index) {
      return input_data[index];
    };
    kts::Reference1D<T> op_reference = [&input_data, &op_ref_fn](size_t index) {
      return op_ref_fn(index, input_data);
    };

    // Set up the buffers.
    // The input is a random set of on and off bits.
    AddInputBuffer(kts::N, random_reference);
    // The expected output for each work-group is all the input bits in the
    // work-group and'd together.
    AddOutputBuffer(kts::N / kts::localN, op_reference);

    // Optional third buffer for initial data
    if (init_ref_fn) {
      kts::Reference1D<T> init_reference = [&input_data,
                                            &init_ref_fn](size_t index) {
        return init_ref_fn(index, input_data);
      };
      AddInputBuffer(kts::N / kts::localN, init_reference);
    }

    AddLocalBuffer<T>(kts::localN);

    // Run the test.
    RunGeneric1D(kts::N, kts::localN);
  }
};

template <typename T>
T zeroReference(size_t, const std::vector<T> &) {
  return T{0};
}

template <typename T>
T firstEltReference(size_t, const std::vector<T> &input) {
  return input[0];
}

TEST_P(FetchTest, C11Atomics_21_Fetch_Global_Add_Check_Return_Int) {
  doCheckReturnTest<cl_int>();
}
TEST_P(FetchTest, C11Atomics_21_Fetch_Global_Add_Check_Return_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_long>();
}
TEST_P(FetchTest, C11Atomics_21_Fetch_Global_Add_Check_Return_Uint) {
  doCheckReturnTest<cl_uint>();
}
TEST_P(FetchTest, C11Atomics_21_Fetch_Global_Add_Check_Return_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_ulong>();
}

TEST_P(FetchTest, C11Atomics_22_Fetch_Global_Sub_Check_Return_Int) {
  doCheckReturnTest<cl_int>();
}
TEST_P(FetchTest, C11Atomics_22_Fetch_Global_Sub_Check_Return_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_long>();
}
TEST_P(FetchTest, C11Atomics_22_Fetch_Global_Sub_Check_Return_Uint) {
  doCheckReturnTest<cl_uint>();
}
TEST_P(FetchTest, C11Atomics_22_Fetch_Global_Sub_Check_Return_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_ulong>();
}

TEST_P(FetchTest, C11Atomics_23_Fetch_Global_Or_Check_Return_Int) {
  doCheckReturnTest<cl_int>();
}
TEST_P(FetchTest, C11Atomics_23_Fetch_Global_Or_Check_Return_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_long>();
}
TEST_P(FetchTest, C11Atomics_23_Fetch_Global_Or_Check_Return_Uint) {
  doCheckReturnTest<cl_uint>();
}
TEST_P(FetchTest, C11Atomics_23_Fetch_Global_Or_Check_Return_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_ulong>();
}

TEST_P(FetchTest, C11Atomics_24_Fetch_Global_Xor_Check_Return_Int) {
  doCheckReturnTest<cl_int>();
}
TEST_P(FetchTest, C11Atomics_24_Fetch_Global_Xor_Check_Return_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_long>();
}
TEST_P(FetchTest, C11Atomics_24_Fetch_Global_Xor_Check_Return_Uint) {
  doCheckReturnTest<cl_uint>();
}
TEST_P(FetchTest, C11Atomics_24_Fetch_Global_Xor_Check_Return_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_ulong>();
}

TEST_P(FetchTest, C11Atomics_25_Fetch_Global_And_Check_Return_Int) {
  doCheckReturnTest<cl_int>();
}
TEST_P(FetchTest, C11Atomics_25_Fetch_Global_And_Check_Return_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_long>();
}
TEST_P(FetchTest, C11Atomics_25_Fetch_Global_And_Check_Return_Uint) {
  doCheckReturnTest<cl_uint>();
}
TEST_P(FetchTest, C11Atomics_25_Fetch_Global_And_Check_Return_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_ulong>();
}

TEST_P(FetchTest, C11Atomics_26_Fetch_Global_Min_Check_Return_Int) {
  doCheckReturnTest<cl_int>();
}
TEST_P(FetchTest, C11Atomics_26_Fetch_Global_Min_Check_Return_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_long>();
}
TEST_P(FetchTest, C11Atomics_26_Fetch_Global_Min_Check_Return_Uint) {
  doCheckReturnTest<cl_uint>();
}
TEST_P(FetchTest, C11Atomics_26_Fetch_Global_Min_Check_Return_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_ulong>();
}

TEST_P(FetchTest, C11Atomics_27_Fetch_Global_Max_Check_Return_Int) {
  doCheckReturnTest<cl_int>();
}
TEST_P(FetchTest, C11Atomics_27_Fetch_Global_Max_Check_Return_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_long>();
}
TEST_P(FetchTest, C11Atomics_27_Fetch_Global_Max_Check_Return_Uint) {
  doCheckReturnTest<cl_uint>();
}
TEST_P(FetchTest, C11Atomics_27_Fetch_Global_Max_Check_Return_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_ulong>();
}

TEST_P(FetchTest, C11Atomics_28_Fetch_Local_Add_Check_Return_Int) {
  doCheckReturnTest<cl_int>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_28_Fetch_Local_Add_Check_Return_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_long>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_28_Fetch_Local_Add_Check_Return_Uint) {
  doCheckReturnTest<cl_uint>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_28_Fetch_Local_Add_Check_Return_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_ulong>(/*local*/ true);
}

TEST_P(FetchTest, C11Atomics_29_Fetch_Local_Sub_Check_Return_Int) {
  doCheckReturnTest<cl_int>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_29_Fetch_Local_Sub_Check_Return_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_long>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_29_Fetch_Local_Sub_Check_Return_Uint) {
  doCheckReturnTest<cl_uint>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_29_Fetch_Local_Sub_Check_Return_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_ulong>(/*local*/ true);
}

TEST_P(FetchTest, C11Atomics_30_Fetch_Local_Or_Check_Return_Int) {
  doCheckReturnTest<cl_int>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_30_Fetch_Local_Or_Check_Return_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_long>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_30_Fetch_Local_Or_Check_Return_Uint) {
  doCheckReturnTest<cl_uint>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_30_Fetch_Local_Or_Check_Return_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_ulong>(/*local*/ true);
}

TEST_P(FetchTest, C11Atomics_31_Fetch_Local_Xor_Check_Return_Int) {
  doCheckReturnTest<cl_int>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_31_Fetch_Local_Xor_Check_Return_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_long>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_31_Fetch_Local_Xor_Check_Return_Uint) {
  doCheckReturnTest<cl_uint>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_31_Fetch_Local_Xor_Check_Return_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_ulong>(/*local*/ true);
}

TEST_P(FetchTest, C11Atomics_32_Fetch_Local_And_Check_Return_Int) {
  doCheckReturnTest<cl_int>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_32_Fetch_Local_And_Check_Return_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_long>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_32_Fetch_Local_And_Check_Return_Uint) {
  doCheckReturnTest<cl_uint>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_32_Fetch_Local_And_Check_Return_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_ulong>(/*local*/ true);
}

TEST_P(FetchTest, C11Atomics_33_Fetch_Local_Min_Check_Return_Int) {
  doCheckReturnTest<cl_int>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_33_Fetch_Local_Min_Check_Return_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_long>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_33_Fetch_Local_Min_Check_Return_Uint) {
  doCheckReturnTest<cl_uint>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_33_Fetch_Local_Min_Check_Return_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_ulong>(/*local*/ true);
}

TEST_P(FetchTest, C11Atomics_34_Fetch_Local_Max_Check_Return_Int) {
  doCheckReturnTest<cl_int>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_34_Fetch_Local_Max_Check_Return_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_long>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_34_Fetch_Local_Max_Check_Return_Uint) {
  doCheckReturnTest<cl_uint>(/*local*/ true);
}
TEST_P(FetchTest, C11Atomics_34_Fetch_Local_Max_Check_Return_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doCheckReturnTest<cl_ulong>(/*local*/ true);
}

TEST_P(FetchTest, C11Atomics_35_Fetch_Global_Add_Int) {
  const auto accumulate_ref = [](size_t, const std::vector<cl_int> &input) {
    return std::accumulate(std::begin(input), std::end(input), cl_int{0});
  };
  doTest<cl_int>(zeroReference<cl_int>, accumulate_ref, /*clamp*/ true);
}
TEST_P(FetchTest, C11Atomics_35_Fetch_Global_Add_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto accumulate_ref = [](size_t, const std::vector<cl_long> &input) {
    return std::accumulate(std::begin(input), std::end(input), cl_long{0});
  };
  doTest<cl_long>(zeroReference<cl_long>, accumulate_ref, /*clamp*/ true);
}
TEST_P(FetchTest, C11Atomics_35_Fetch_Global_Add_Uint) {
  const auto accumulate_ref = [](size_t, const std::vector<cl_uint> &input) {
    return std::accumulate(std::begin(input), std::end(input), cl_uint{0});
  };
  doTest<cl_uint>(zeroReference<cl_uint>, accumulate_ref, /*clamp*/ true);
}
TEST_P(FetchTest, C11Atomics_35_Fetch_Global_Add_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto accumulate_ref = [](size_t, const std::vector<cl_ulong> &input) {
    return std::accumulate(std::begin(input), std::end(input), cl_ulong{0});
  };
  doTest<cl_ulong>(zeroReference<cl_ulong>, accumulate_ref, /*clamp*/ true);
}

TEST_P(FetchTest, C11Atomics_36_Fetch_Local_Add_Int) {
  const auto accumulate_ref = [](size_t index,
                                 const std::vector<cl_int> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = start + kts::localN;
    return std::accumulate(start, end, cl_int{0});
  };
  doLocalTest<cl_int>(nullptr, accumulate_ref, /*clamp*/ true);
}
TEST_P(FetchTest, C11Atomics_36_Fetch_Local_Add_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto accumulate_ref = [](size_t index,
                                 const std::vector<cl_long> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = start + kts::localN;
    return std::accumulate(start, end, cl_long{0});
  };
  doLocalTest<cl_long>(nullptr, accumulate_ref, /*clamp*/ true);
}
TEST_P(FetchTest, C11Atomics_36_Fetch_Local_Add_Uint) {
  const auto accumulate_ref = [](size_t index,
                                 const std::vector<cl_uint> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = start + kts::localN;
    return std::accumulate(start, end, cl_uint{0});
  };
  doLocalTest<cl_uint>(nullptr, accumulate_ref, /*clamp*/ true);
}
TEST_P(FetchTest, C11Atomics_36_Fetch_Local_Add_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto accumulate_ref = [](size_t index,
                                 const std::vector<cl_ulong> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = start + kts::localN;
    return std::accumulate(start, end, cl_ulong{0});
  };
  doLocalTest<cl_ulong>(nullptr, accumulate_ref, /*clamp*/ true);
}

TEST_P(FetchTest, C11Atomics_37_Fetch_Global_Sub_Int) {
  const auto accumulate_ref = [](size_t, const std::vector<cl_int> &input) {
    return std::accumulate(std::begin(input), std::end(input), cl_int{0});
  };
  doTest<cl_int>(accumulate_ref, zeroReference<cl_int>, /*clamp*/ true);
}
TEST_P(FetchTest, C11Atomics_37_Fetch_Global_Sub_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto accumulate_ref = [](size_t, const std::vector<cl_long> &input) {
    return std::accumulate(std::begin(input), std::end(input), cl_long{0});
  };
  doTest<cl_long>(accumulate_ref, zeroReference<cl_long>, /*clamp*/ true);
}
TEST_P(FetchTest, C11Atomics_37_Fetch_Global_Sub_Uint) {
  const auto accumulate_ref = [](size_t, const std::vector<cl_uint> &input) {
    return std::accumulate(std::begin(input), std::end(input), cl_uint{0});
  };
  doTest<cl_uint>(accumulate_ref, zeroReference<cl_uint>, /*clamp*/ true);
}
TEST_P(FetchTest, C11Atomics_37_Fetch_Global_Sub_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto accumulate_ref = [](size_t, const std::vector<cl_ulong> &input) {
    return std::accumulate(std::begin(input), std::end(input), cl_ulong{0});
  };
  doTest<cl_ulong>(accumulate_ref, zeroReference<cl_ulong>, /*clamp*/ true);
}

TEST_P(FetchTest, C11Atomics_38_Fetch_Local_Sub_Int) {
  const auto accumulate_ref = [](size_t index,
                                 const std::vector<cl_int> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = start + kts::localN;
    return std::accumulate(start, end, cl_int{0});
  };
  doLocalTest<cl_int>(accumulate_ref, zeroReference<cl_int>, /*clamp*/ true);
}
TEST_P(FetchTest, C11Atomics_38_Fetch_Local_Sub_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto accumulate_ref = [](size_t index,
                                 const std::vector<cl_long> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = start + kts::localN;
    return std::accumulate(start, end, cl_long{0});
  };
  doLocalTest<cl_long>(accumulate_ref, zeroReference<cl_long>, /*clamp*/ true);
}
TEST_P(FetchTest, C11Atomics_38_Fetch_Local_Sub_Uint) {
  const auto accumulate_ref = [](size_t index,
                                 const std::vector<cl_uint> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = start + kts::localN;
    return std::accumulate(start, end, cl_uint{0});
  };
  doLocalTest<cl_uint>(accumulate_ref, zeroReference<cl_uint>, /*clamp*/ true);
}
TEST_P(FetchTest, C11Atomics_38_Fetch_Local_Sub_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto accumulate_ref = [](size_t index,
                                 const std::vector<cl_ulong> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = start + kts::localN;
    return std::accumulate(start, end, cl_ulong{0});
  };
  doLocalTest<cl_ulong>(accumulate_ref, zeroReference<cl_ulong>,
                        /*clamp*/ true);
}

TEST_P(FetchTest, C11Atomics_39_Fetch_Global_Or_Int) {
  const auto or_ref = [](size_t, const std::vector<cl_int> &input) {
    return std::accumulate(std::next(std::begin(input)), std::end(input),
                           input[0],
                           [](cl_int lhs, cl_int rhs) { return lhs | rhs; });
  };
  doTest<cl_int>(firstEltReference<cl_int>, or_ref);
}
TEST_P(FetchTest, C11Atomics_39_Fetch_Global_Or_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto or_ref = [](size_t, const std::vector<cl_long> &input) {
    return std::accumulate(std::next(std::begin(input)), std::end(input),
                           input[0],
                           [](cl_long lhs, cl_long rhs) { return lhs | rhs; });
  };
  doTest<cl_long>(firstEltReference<cl_long>, or_ref);
}
TEST_P(FetchTest, C11Atomics_39_Fetch_Global_Or_Uint) {
  const auto or_ref = [](size_t, const std::vector<cl_uint> &input) {
    return std::accumulate(std::next(std::begin(input)), std::end(input),
                           input[0],
                           [](cl_uint lhs, cl_uint rhs) { return lhs | rhs; });
  };
  doTest<cl_uint>(firstEltReference<cl_uint>, or_ref);
}
TEST_P(FetchTest, C11Atomics_39_Fetch_Global_Or_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto or_ref = [](size_t, const std::vector<cl_ulong> &input) {
    return std::accumulate(
        std::next(std::begin(input)), std::end(input), input[0],
        [](cl_ulong lhs, cl_ulong rhs) { return lhs | rhs; });
  };
  doTest<cl_ulong>(firstEltReference<cl_ulong>, or_ref);
}

TEST_P(FetchTest, C11Atomics_40_Fetch_Local_Or_Int) {
  const auto or_ref = [](size_t index, const std::vector<cl_int> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return std::accumulate(std::next(start), end, *start,
                           [](cl_int lhs, cl_int rhs) { return lhs | rhs; });
  };
  doLocalTest<cl_int>(nullptr, or_ref);
}
TEST_P(FetchTest, C11Atomics_40_Fetch_Local_Or_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto or_ref = [](size_t index, const std::vector<cl_long> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return std::accumulate(std::next(start), end, *start,
                           [](cl_long lhs, cl_long rhs) { return lhs | rhs; });
  };
  doLocalTest<cl_long>(nullptr, or_ref);
}
TEST_P(FetchTest, C11Atomics_40_Fetch_Local_Or_Uint) {
  const auto or_ref = [](size_t index, const std::vector<cl_uint> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return std::accumulate(std::next(start), end, *start,
                           [](cl_uint lhs, cl_uint rhs) { return lhs | rhs; });
  };
  doLocalTest<cl_uint>(nullptr, or_ref);
}
TEST_P(FetchTest, C11Atomics_40_Fetch_Local_Or_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto or_ref = [](size_t index, const std::vector<cl_ulong> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return std::accumulate(
        std::next(start), end, *start,
        [](cl_ulong lhs, cl_ulong rhs) { return lhs | rhs; });
  };
  doLocalTest<cl_ulong>(nullptr, or_ref);
}

TEST_P(FetchTest, C11Atomics_41_Fetch_Global_Xor_Int) {
  const auto xor_ref = [](size_t, const std::vector<cl_int> &input) {
    return std::accumulate(std::next(std::begin(input)), std::end(input),
                           input[0],
                           [](cl_int lhs, cl_int rhs) { return lhs ^ rhs; });
  };

  doTest<cl_int>(firstEltReference<cl_int>, xor_ref);
}
TEST_P(FetchTest, C11Atomics_41_Fetch_Global_Xor_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto xor_ref = [](size_t, const std::vector<cl_long> &input) {
    return std::accumulate(std::next(std::begin(input)), std::end(input),
                           input[0],
                           [](cl_long lhs, cl_long rhs) { return lhs ^ rhs; });
  };

  doTest<cl_long>(firstEltReference<cl_long>, xor_ref);
}
TEST_P(FetchTest, C11Atomics_41_Fetch_Global_Xor_Uint) {
  const auto xor_ref = [](size_t, const std::vector<cl_uint> &input) {
    return std::accumulate(std::next(std::begin(input)), std::end(input),
                           input[0],
                           [](cl_uint lhs, cl_uint rhs) { return lhs ^ rhs; });
  };
  doTest<cl_uint>(firstEltReference<cl_uint>, xor_ref);
}
TEST_P(FetchTest, C11Atomics_41_Fetch_Global_Xor_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto xor_ref = [](size_t, const std::vector<cl_ulong> &input) {
    return std::accumulate(
        std::next(std::begin(input)), std::end(input), input[0],
        [](cl_ulong lhs, cl_ulong rhs) { return lhs ^ rhs; });
  };
  doTest<cl_ulong>(firstEltReference<cl_ulong>, xor_ref);
}

TEST_P(FetchTest, C11Atomics_42_Fetch_Local_Xor_Int) {
  const auto xor_ref = [](size_t index, const std::vector<cl_int> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return std::accumulate(std::next(start), end, *start,
                           [](cl_int lhs, cl_int rhs) { return lhs ^ rhs; });
  };
  doLocalTest<cl_int>(nullptr, xor_ref);
}
TEST_P(FetchTest, C11Atomics_42_Fetch_Local_Xor_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto xor_ref = [](size_t index, const std::vector<cl_long> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return std::accumulate(std::next(start), end, *start,
                           [](cl_long lhs, cl_long rhs) { return lhs ^ rhs; });
  };
  doLocalTest<cl_long>(nullptr, xor_ref);
}
TEST_P(FetchTest, C11Atomics_42_Fetch_Local_Xor_Uint) {
  const auto xor_ref = [](size_t index, const std::vector<cl_uint> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return std::accumulate(std::next(start), end, *start,
                           [](cl_uint lhs, cl_uint rhs) { return lhs ^ rhs; });
  };
  doLocalTest<cl_uint>(nullptr, xor_ref);
}
TEST_P(FetchTest, C11Atomics_42_Fetch_Local_Xor_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto xor_ref = [](size_t index, const std::vector<cl_ulong> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return std::accumulate(
        std::next(start), end, *start,
        [](cl_ulong lhs, cl_ulong rhs) { return lhs ^ rhs; });
  };
  doLocalTest<cl_ulong>(nullptr, xor_ref);
}

TEST_P(FetchTest, C11Atomics_43_Fetch_Global_And_Int) {
  const auto and_ref = [](size_t, const std::vector<cl_int> &input) {
    return std::accumulate(std::next(std::begin(input)), std::end(input),
                           input[0],
                           [](cl_int lhs, cl_int rhs) { return lhs & rhs; });
  };
  doTest<cl_int>(firstEltReference<cl_int>, and_ref);
}
TEST_P(FetchTest, C11Atomics_43_Fetch_Global_And_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto and_ref = [](size_t, const std::vector<cl_long> &input) {
    return std::accumulate(std::next(std::begin(input)), std::end(input),
                           input[0],
                           [](cl_long lhs, cl_long rhs) { return lhs & rhs; });
  };
  doTest<cl_long>(firstEltReference<cl_long>, and_ref);
}
TEST_P(FetchTest, C11Atomics_43_Fetch_Global_And_Uint) {
  const auto and_ref = [](size_t, const std::vector<cl_uint> &input) {
    return std::accumulate(std::next(std::begin(input)), std::end(input),
                           input[0],
                           [](cl_uint lhs, cl_uint rhs) { return lhs & rhs; });
  };
  doTest<cl_uint>(firstEltReference<cl_uint>, and_ref);
}
TEST_P(FetchTest, C11Atomics_43_Fetch_Global_And_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto and_ref = [](size_t, const std::vector<cl_ulong> &input) {
    return std::accumulate(
        std::next(std::begin(input)), std::end(input), input[0],
        [](cl_ulong lhs, cl_ulong rhs) { return lhs & rhs; });
  };
  doTest<cl_ulong>(firstEltReference<cl_ulong>, and_ref);
}

TEST_P(FetchTest, C11Atomics_44_Fetch_Local_And_Int) {
  const auto and_ref = [](size_t index, const std::vector<cl_int> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return std::accumulate(std::next(start), end, *start,
                           [](cl_int lhs, cl_int rhs) { return lhs & rhs; });
  };
  doLocalTest<cl_int>(nullptr, and_ref);
}
TEST_P(FetchTest, C11Atomics_44_Fetch_Local_And_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto and_ref = [](size_t index, const std::vector<cl_long> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return std::accumulate(std::next(start), end, *start,
                           [](cl_long lhs, cl_long rhs) { return lhs & rhs; });
  };
  doLocalTest<cl_long>(nullptr, and_ref);
}
TEST_P(FetchTest, C11Atomics_44_Fetch_Local_And_Uint) {
  const auto and_ref = [](size_t index, const std::vector<cl_uint> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return std::accumulate(std::next(start), end, *start,
                           [](cl_uint lhs, cl_uint rhs) { return lhs & rhs; });
  };
  doLocalTest<cl_uint>(nullptr, and_ref);
}
TEST_P(FetchTest, C11Atomics_44_Fetch_Local_And_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto and_ref = [](size_t index, const std::vector<cl_ulong> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return std::accumulate(
        std::next(start), end, *start,
        [](cl_ulong lhs, cl_ulong rhs) { return lhs & rhs; });
  };
  doLocalTest<cl_ulong>(nullptr, and_ref);
}

TEST_P(FetchTest, C11Atomics_45_Fetch_Global_Min_Int) {
  const auto min_ref = [](size_t, const std::vector<cl_int> &input) {
    return *std::min_element(std::begin(input), std::end(input));
  };
  doTest<cl_int>(firstEltReference<cl_int>, min_ref);
}
TEST_P(FetchTest, C11Atomics_45_Fetch_Global_Min_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto min_ref = [](size_t, const std::vector<cl_long> &input) {
    return *std::min_element(std::begin(input), std::end(input));
  };
  doTest<cl_long>(firstEltReference<cl_long>, min_ref);
}
TEST_P(FetchTest, C11Atomics_45_Fetch_Global_Min_Uint) {
  const auto min_ref = [](size_t, const std::vector<cl_uint> &input) {
    return *std::min_element(std::begin(input), std::end(input));
  };
  doTest<cl_uint>(firstEltReference<cl_uint>, min_ref);
}
TEST_P(FetchTest, C11Atomics_45_Fetch_Global_Min_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto min_ref = [](size_t, const std::vector<cl_ulong> &input) {
    return *std::min_element(std::begin(input), std::end(input));
  };
  doTest<cl_ulong>(firstEltReference<cl_ulong>, min_ref);
}

TEST_P(FetchTest, C11Atomics_46_Fetch_Local_Min_Int) {
  const auto min_ref = [](size_t index, const std::vector<cl_int> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return *std::min_element(start, end);
  };
  doLocalTest<cl_int>(nullptr, min_ref);
}
TEST_P(FetchTest, C11Atomics_46_Fetch_Local_Min_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto min_ref = [](size_t index, const std::vector<cl_long> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return *std::min_element(start, end);
  };
  doLocalTest<cl_long>(nullptr, min_ref);
}
TEST_P(FetchTest, C11Atomics_46_Fetch_Local_Min_Uint) {
  const auto min_ref = [](size_t index, const std::vector<cl_uint> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return *std::min_element(start, end);
  };
  doLocalTest<cl_uint>(nullptr, min_ref);
}
TEST_P(FetchTest, C11Atomics_46_Fetch_Local_Min_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto min_ref = [](size_t index, const std::vector<cl_ulong> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return *std::min_element(start, end);
  };
  doLocalTest<cl_ulong>(nullptr, min_ref);
}

TEST_P(FetchTest, C11Atomics_47_Fetch_Global_Max_Int) {
  const auto max_ref = [](size_t, const std::vector<cl_int> &input) {
    return *std::max_element(std::begin(input), std::end(input));
  };
  doTest<cl_int>(firstEltReference<cl_int>, max_ref);
}
TEST_P(FetchTest, C11Atomics_47_Fetch_Global_Max_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto max_ref = [](size_t, const std::vector<cl_long> &input) {
    return *std::max_element(std::begin(input), std::end(input));
  };
  doTest<cl_long>(firstEltReference<cl_long>, max_ref);
}
TEST_P(FetchTest, C11Atomics_47_Fetch_Global_Max_Uint) {
  const auto max_ref = [](size_t, const std::vector<cl_uint> &input) {
    return *std::max_element(std::begin(input), std::end(input));
  };
  doTest<cl_uint>(firstEltReference<cl_uint>, max_ref);
}
TEST_P(FetchTest, C11Atomics_47_Fetch_Global_Max_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto max_ref = [](size_t, const std::vector<cl_ulong> &input) {
    return *std::max_element(std::begin(input), std::end(input));
  };
  doTest<cl_ulong>(firstEltReference<cl_ulong>, max_ref);
}

TEST_P(FetchTest, C11Atomics_48_Fetch_Local_Max_Int) {
  const auto max_ref = [](size_t index, const std::vector<cl_int> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return *std::max_element(start, end);
  };
  doLocalTest<cl_int>(nullptr, max_ref);
}
TEST_P(FetchTest, C11Atomics_48_Fetch_Local_Max_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto max_ref = [](size_t index, const std::vector<cl_long> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return *std::max_element(start, end);
  };
  doLocalTest<cl_long>(nullptr, max_ref);
}
TEST_P(FetchTest, C11Atomics_48_Fetch_Local_Max_Uint) {
  const auto max_ref = [](size_t index, const std::vector<cl_uint> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return *std::max_element(start, end);
  };
  doLocalTest<cl_uint>(nullptr, max_ref);
}
TEST_P(FetchTest, C11Atomics_48_Fetch_Local_Max_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto max_ref = [](size_t index, const std::vector<cl_ulong> &input) {
    auto start = std::next(std::begin(input), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return *std::max_element(start, end);
  };
  doLocalTest<cl_ulong>(nullptr, max_ref);
}

UCL_EXECUTION_TEST_SUITE(FetchTest, testing::ValuesIn(source_types));

using TruthTableInputs = std::pair<unsigned, unsigned>;

// The following tests check the entire domain {{0, 1} x {0, 1}} for the logical
// operations. That is, it checks that for the atomic fetch logic operations the
// following truth tables hold:
//
// | 0 1   ^ 0 1   & 0 1
// 0 0 1   0 0 1   0 0 0
// 1 1 1   1 1 0   1 0 1
class FetchTruthTableTest
    : public kts::ucl::ExecutionWithParam<TruthTableInputs> {
 public:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(BaseExecution::SetUp());
    // The C11 atomic were introduced in 2.0 however here we only test
    // the minimum required subset for 3.0.
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }

    this->AddBuildOption("-cl-std=CL3.0");

    // This test only uses uniform inputs so the vectorizer doesn't vectorize.
    this->fail_if_not_vectorized_ = false;
  }

  template <typename T>
  void doTest(const std::function<T(T, T)> &ref_fn) {
    const auto &inputs = std::get<1>(GetParam());
    // Set up references.
    kts::Reference1D<T> initializer_reference = [&inputs](size_t) {
      return inputs.first;
    };
    kts::Reference1D<T> input_reference = [&inputs](size_t) {
      return inputs.second;
    };
    kts::Reference1D<T> output_reference = [&inputs, &ref_fn](size_t) {
      return ref_fn(inputs.first, inputs.second);
    };

    // Set up the buffers.
    // Input is the first element.
    // Expected output is the result of the binary operation with the second
    // element.
    this->AddInOutBuffer(1, initializer_reference, output_reference);
    // Input is the second element.
    this->AddInputBuffer(1, input_reference);

    // Run the test.
    this->RunGeneric1D(1);
  }

  template <typename T>
  void doLocalTest(const std::function<T(T, T)> &ref_fn) {
    const auto &inputs = std::get<1>(GetParam());
    // Set up references.
    kts::Reference1D<T> input_reference = [&inputs](size_t index) {
      return index == 0 ? inputs.first : inputs.second;
    };
    kts::Reference1D<T> output_reference = [&inputs, &ref_fn](size_t) {
      return ref_fn(inputs.first, inputs.second);
    };

    // Set up the buffers.
    // Input is the two elements for the binary operation.
    this->AddInputBuffer(2, input_reference);
    // Expected output is the result of the binary operation.
    this->AddOutputBuffer(1, output_reference);
    this->AddLocalBuffer<T>(2);

    // Run the test.
    this->RunGeneric1D(2, 2);
  }
};

TEST_P(FetchTruthTableTest, C11Atomics_49_Fetch_Global_Or_Truth_Table_Int) {
  const auto or_ref = [](cl_int A, cl_int B) { return A | B; };
  doTest<cl_int>(or_ref);
}
TEST_P(FetchTruthTableTest, C11Atomics_49_Fetch_Global_Or_Truth_Table_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto or_ref = [](cl_long A, cl_long B) { return A | B; };
  doTest<cl_long>(or_ref);
}
TEST_P(FetchTruthTableTest, C11Atomics_49_Fetch_Global_Or_Truth_Table_Uint) {
  const auto or_ref = [](cl_uint A, cl_uint B) { return A | B; };
  doTest<cl_uint>(or_ref);
}
TEST_P(FetchTruthTableTest, C11Atomics_49_Fetch_Global_Or_Truth_Table_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto or_ref = [](cl_ulong A, cl_ulong B) { return A | B; };
  doTest<cl_ulong>(or_ref);
}

TEST_P(FetchTruthTableTest, C11Atomics_50_Fetch_Global_Xor_Truth_Table_Int) {
  const auto xor_ref = [](cl_int A, cl_int B) { return A ^ B; };
  doTest<cl_int>(xor_ref);
}
TEST_P(FetchTruthTableTest, C11Atomics_50_Fetch_Global_Xor_Truth_Table_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto xor_ref = [](cl_long A, cl_long B) { return A ^ B; };
  doTest<cl_long>(xor_ref);
}
TEST_P(FetchTruthTableTest, C11Atomics_50_Fetch_Global_Xor_Truth_Table_Uint) {
  const auto xor_ref = [](cl_uint A, cl_uint B) { return A ^ B; };
  doTest<cl_uint>(xor_ref);
}
TEST_P(FetchTruthTableTest, C11Atomics_50_Fetch_Global_Xor_Truth_Table_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto xor_ref = [](cl_ulong A, cl_ulong B) { return A ^ B; };
  doTest<cl_ulong>(xor_ref);
}

TEST_P(FetchTruthTableTest, C11Atomics_51_Fetch_Global_And_Truth_Table_Int) {
  const auto and_ref = [](cl_int A, cl_int B) { return A & B; };
  doTest<cl_int>(and_ref);
}
TEST_P(FetchTruthTableTest, C11Atomics_51_Fetch_Global_And_Truth_Table_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto and_ref = [](cl_long A, cl_long B) { return A & B; };
  doTest<cl_long>(and_ref);
}
TEST_P(FetchTruthTableTest, C11Atomics_51_Fetch_Global_And_Truth_Table_Uint) {
  const auto and_ref = [](cl_uint A, cl_uint B) { return A & B; };
  doTest<cl_uint>(and_ref);
}
TEST_P(FetchTruthTableTest, C11Atomics_51_Fetch_Global_And_Truth_Table_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto and_ref = [](cl_ulong A, cl_ulong B) { return A & B; };
  doTest<cl_ulong>(and_ref);
}

TEST_P(FetchTruthTableTest, C11Atomics_52_Fetch_Local_Or_Truth_Table_Int) {
  const auto or_ref = [](cl_int A, cl_int B) { return A | B; };
  doLocalTest<cl_int>(or_ref);
}
TEST_P(FetchTruthTableTest, C11Atomics_52_Fetch_Local_Or_Truth_Table_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto or_ref = [](cl_long A, cl_int B) { return A | B; };
  doLocalTest<cl_long>(or_ref);
}
TEST_P(FetchTruthTableTest, C11Atomics_52_Fetch_Local_Or_Truth_Table_Uint) {
  const auto or_ref = [](cl_uint A, cl_uint B) { return A | B; };
  doLocalTest<cl_uint>(or_ref);
}
TEST_P(FetchTruthTableTest, C11Atomics_52_Fetch_Local_Or_Truth_Table_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto or_ref = [](cl_ulong A, cl_ulong B) { return A | B; };
  doLocalTest<cl_ulong>(or_ref);
}

TEST_P(FetchTruthTableTest, C11Atomics_53_Fetch_Local_Xor_Truth_Table_Int) {
  const auto xor_ref = [](cl_int A, cl_int B) { return A ^ B; };
  doLocalTest<cl_int>(xor_ref);
}
TEST_P(FetchTruthTableTest, C11Atomics_53_Fetch_Local_Xor_Truth_Table_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto xor_ref = [](cl_long A, cl_long B) { return A ^ B; };
  doLocalTest<cl_long>(xor_ref);
}
TEST_P(FetchTruthTableTest, C11Atomics_53_Fetch_Local_Xor_Truth_Table_Uint) {
  const auto xor_ref = [](cl_uint A, cl_uint B) { return A ^ B; };
  doLocalTest<cl_uint>(xor_ref);
}
TEST_P(FetchTruthTableTest, C11Atomics_53_Fetch_Local_Xor_Truth_Table_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto xor_ref = [](cl_ulong A, cl_ulong B) { return A ^ B; };
  doLocalTest<cl_ulong>(xor_ref);
}

TEST_P(FetchTruthTableTest, C11Atomics_54_Fetch_Local_And_Truth_Table_Int) {
  const auto and_ref = [](cl_int A, cl_int B) { return A & B; };
  doLocalTest<cl_int>(and_ref);
}
TEST_P(FetchTruthTableTest, C11Atomics_54_Fetch_Local_And_Truth_Table_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto and_ref = [](cl_long A, cl_long B) { return A & B; };
  doLocalTest<cl_long>(and_ref);
}
TEST_P(FetchTruthTableTest, C11Atomics_54_Fetch_Local_And_Truth_Table_Uint) {
  const auto and_ref = [](cl_uint A, cl_uint B) { return A & B; };
  doLocalTest<cl_uint>(and_ref);
}
TEST_P(FetchTruthTableTest, C11Atomics_54_Fetch_Local_And_Truth_Table_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  const auto and_ref = [](cl_ulong A, cl_ulong B) { return A & B; };
  doLocalTest<cl_ulong>(and_ref);
}

static const TruthTableInputs truth_table_domain[] = {
    {0, 0}, {0, 1}, {1, 0}, {1, 1}};

UCL_EXECUTION_TEST_SUITE(
    FetchTruthTableTest,
    testing::Combine(testing::ValuesIn(source_types),
                     testing::ValuesIn(truth_table_domain)));

class Strong : public C11AtomicTestBase {
 public:
  template <typename T>
  void doTest(bool local = false, bool local_local = false) {
    // Generate the random input.
    std::vector<T> input_data(kts::N, T{});
    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
    std::vector<T> desired_data(kts::N, T{});
    ucl::Environment::instance->GetInputGenerator().GenerateData(desired_data);

    // Set up references.
    kts::Reference1D<T> random_reference = [input_data](size_t index) {
      return input_data[index];
    };
    kts::Reference1D<T> expected_in_reference = [input_data](size_t index) {
      auto expected_value = input_data[index];
      // This ensures every other expected value matches the value in
      // the input.
      return (index % 2) ? expected_value : !expected_value;
    };
    kts::Reference1D<T> output_reference = [input_data,
                                            desired_data](size_t index) {
      return (index % 2) ? desired_data[index] : input_data[index];
    };
    kts::Reference1D<T> desired_reference = [desired_data](size_t index) {
      return desired_data[index];
    };
    kts::Reference1D<cl_int> bool_reference = [](size_t index) {
      return index % 2;
    };

    // Set up the buffers.
    AddInOutBuffer(kts::N, random_reference, output_reference);
    AddInOutBuffer(kts::N, expected_in_reference, random_reference);
    AddInputBuffer(kts::N, desired_reference);
    AddOutputBuffer(kts::N, bool_reference);

    if (!local) {
      RunGeneric1D(kts::N);
    } else {
      AddLocalBuffer<T>(kts::localN);
      if (local_local) {
        AddLocalBuffer<T>(kts::localN);
      }
      RunGeneric1D(kts::N, kts::localN);
    }
  }
};

TEST_P(Strong, C11Atomics_55_Compare_Exchange_Strong_Global_Global_Int) {
  doTest<cl_int>();
}
TEST_P(Strong, C11Atomics_55_Compare_Exchange_Strong_Global_Global_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>();
}
TEST_P(Strong, C11Atomics_55_Compare_Exchange_Strong_Global_Global_Uint) {
  doTest<cl_uint>();
}
TEST_P(Strong, C11Atomics_55_Compare_Exchange_Strong_Global_Global_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>();
}

TEST_P(Strong, C11Atomics_56_Compare_Exchange_Strong_Global_Local_Int) {
  doTest<cl_int>(/*local*/ true);
}
TEST_P(Strong, C11Atomics_56_Compare_Exchange_Strong_Global_Local_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>(/*local*/ true);
}
TEST_P(Strong, C11Atomics_56_Compare_Exchange_Strong_Global_Local_Uint) {
  doTest<cl_uint>(/*local*/ true);
}
TEST_P(Strong, C11Atomics_56_Compare_Exchange_Strong_Global_Local_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>(/*local*/ true);
}

TEST_P(Strong, C11Atomics_57_Compare_Exchange_Strong_Global_Private_Int) {
  doTest<cl_int>();
}
TEST_P(Strong, C11Atomics_57_Compare_Exchange_Strong_Global_Private_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>();
}
TEST_P(Strong, C11Atomics_57_Compare_Exchange_Strong_Global_Private_Uint) {
  doTest<cl_uint>();
}
TEST_P(Strong, C11Atomics_57_Compare_Exchange_Strong_Global_Private_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>();
}

TEST_P(Strong, C11Atomics_58_Compare_Exchange_Strong_Local_Global_Int) {
  doTest<cl_int>(/*local*/ true);
}
TEST_P(Strong, C11Atomics_58_Compare_Exchange_Strong_Local_Global_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>(/*local*/ true);
}
TEST_P(Strong, C11Atomics_58_Compare_Exchange_Strong_Local_Global_Uint) {
  doTest<cl_uint>(/*local*/ true);
}
TEST_P(Strong, C11Atomics_58_Compare_Exchange_Strong_Local_Global_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>(/*local*/ true);
}

TEST_P(Strong, C11Atomics_59_Compare_Exchange_Strong_Local_Local_Int) {
  doTest<cl_int>(/*local*/ true, /*local_local*/ true);
}
TEST_P(Strong, C11Atomics_59_Compare_Exchange_Strong_Local_Local_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>(/*local*/ true, /*local_local*/ true);
}
TEST_P(Strong, C11Atomics_59_Compare_Exchange_Strong_Local_Local_Uint) {
  doTest<cl_uint>(/*local*/ true, /*local_ulocal*/ true);
}
TEST_P(Strong, C11Atomics_59_Compare_Exchange_Strong_Local_Local_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>(/*local*/ true, /*local_ulocal*/ true);
}

TEST_P(Strong, C11Atomics_60_Compare_Exchange_Strong_Local_Private_Int) {
  doTest<cl_int>(/*local*/ true);
}
TEST_P(Strong, C11Atomics_60_Compare_Exchange_Strong_Local_Private_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>(/*local*/ true);
}
TEST_P(Strong, C11Atomics_60_Compare_Exchange_Strong_Local_Private_Uint) {
  doTest<cl_uint>(/*local*/ true);
}
TEST_P(Strong, C11Atomics_60_Compare_Exchange_Strong_Local_Private_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>(/*local*/ true);
}

UCL_EXECUTION_TEST_SUITE(Strong, testing::ValuesIn(source_types));

class StrongGlobalSingle : public C11AtomicTestBase {
 public:
  template <typename T>
  void doTest(bool local = false) {
    // Set up references.
    size_t success_index =
        ucl::Environment::instance->GetInputGenerator().GenerateInt<T>(
            0, kts::N - 1);

    // We need the expected values to be unique, otherwise we won't be able to
    // determine which thread updates the atomic.
    // We also require the intersection of the expected and desired values to
    // be empty otherwise subsequent threads could update the atomic. The
    // fastest way to do this is to generate a buffer of unqique values of size
    // 2 * kts::N, then just split it evenly between the two.
    std::vector<T> expected_values(kts::N, T{});
    std::vector<T> desired_values(kts::N, T{});
    std::vector<T> all_values(expected_values.size() + desired_values.size(),
                              T{});

    ucl::Environment::instance->GetInputGenerator().GenerateUniqueIntData<T>(
        all_values);
    expected_values.assign(std::begin(all_values),
                           std::next(std::begin(all_values), kts::N));
    desired_values.assign(std::next(std::begin(all_values), kts::N),
                          std::end(all_values));

    kts::Reference1D<T> initializer_reference = [success_index,
                                                 expected_values](size_t) {
      return expected_values[success_index];
    };
    kts::Reference1D<T> output_reference = [success_index,
                                            desired_values](size_t) {
      return desired_values[success_index];
    };
    kts::Reference1D<T> expected_in_reference =
        [expected_values](size_t index) { return expected_values[index]; };
    kts::Reference1D<T> expected_output_reference =
        [success_index, expected_values, desired_values](size_t index,
                                                         T value) {
          if (index == success_index) {
            return value == expected_values[index];
          }

          return value == expected_values[success_index] ||
                 value == desired_values[success_index];
        };
    kts::Reference1D<T> desired_reference = [desired_values](size_t index) {
      return desired_values[index];
    };
    kts::Reference1D<cl_int> bool_output_reference =
        [success_index](size_t index) { return index == success_index; };

    // Set up the buffers.
    AddInOutBuffer(1, initializer_reference, output_reference);
    AddInOutBuffer(kts::N, expected_in_reference, expected_output_reference);
    AddInputBuffer(kts::N, desired_reference);
    AddOutputBuffer(kts::N, bool_output_reference);
    if (!local) {
      RunGeneric1D(kts::N);
    } else {
      AddLocalBuffer<T>(kts::localN);

      // Run the test.
      RunGeneric1D(kts::N, kts::localN);
    }
  }
};

TEST_P(StrongGlobalSingle,
       C11Atomics_61_Compare_Exchange_Strong_Global_Global_Single_Int) {
  doTest<cl_int>();
}
TEST_P(StrongGlobalSingle,
       C11Atomics_61_Compare_Exchange_Strong_Global_Global_Single_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>();
}
TEST_P(StrongGlobalSingle,
       C11Atomics_61_Compare_Exchange_Strong_Global_Global_Single_Uint) {
  doTest<cl_uint>();
}
TEST_P(StrongGlobalSingle,
       C11Atomics_61_Compare_Exchange_Strong_Global_Global_Single_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>();
}

TEST_P(StrongGlobalSingle,
       C11Atomics_62_Compare_Exchange_Strong_Global_Local_Single_Int) {
  doTest<cl_int>(/*local*/ true);
}
TEST_P(StrongGlobalSingle,
       C11Atomics_62_Compare_Exchange_Strong_Global_Local_Single_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>(/*local*/ true);
}
TEST_P(StrongGlobalSingle,
       C11Atomics_62_Compare_Exchange_Strong_Global_Local_Single_Uint) {
  doTest<cl_uint>(/*local*/ true);
}
TEST_P(StrongGlobalSingle,
       C11Atomics_62_Compare_Exchange_Strong_Global_Local_Single_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>(/*local*/ true);
}

TEST_P(StrongGlobalSingle,
       C11Atomics_63_Compare_Exchange_Strong_Global_Private_Single_Int) {
  doTest<cl_int>();
}
TEST_P(StrongGlobalSingle,
       C11Atomics_63_Compare_Exchange_Strong_Global_Private_Single_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>();
}
TEST_P(StrongGlobalSingle,
       C11Atomics_63_Compare_Exchange_Strong_Global_Private_Single_Uint) {
  doTest<cl_uint>();
}
TEST_P(StrongGlobalSingle,
       C11Atomics_63_Compare_Exchange_Strong_Global_Private_Single_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>();
}

UCL_EXECUTION_TEST_SUITE(StrongGlobalSingle, testing::ValuesIn(source_types));

class StrongLocalSingle : public C11AtomicTestBase {
 public:
  template <typename T>
  void doTest(bool local_local = false) {
    // Set up references.
    const size_t work_group_count = kts::N / kts::localN;

    // Pick a random index in each work-group to hold the correct
    // expected value.
    std::vector<size_t> success_indices(work_group_count, 0);
    ucl::Environment::instance->GetInputGenerator().GenerateIntData<size_t>(
        success_indices, 0, kts::localN - 1);
    // Calculate the global id of these indices
    for (unsigned i = 0; i < success_indices.size(); ++i) {
      success_indices[i] += i * kts::localN;
    }

    // We still need N expected values, there will be exactly one correct value
    // in each work-group.
    // We also need N desired values where each work-group has an empty
    // intersection with its expected values. The easiest way to do this is to
    // just generate 2 * kts::N uniqueue values and divide them between the two
    // buffers.
    std::vector<T> expected_values(kts::N, T{});
    std::vector<T> desired_values(kts::N, T{});
    std::vector<T> all_values(expected_values.size() + desired_values.size(),
                              T{});

    ucl::Environment::instance->GetInputGenerator().GenerateUniqueIntData(
        all_values);
    expected_values.assign(std::begin(all_values),
                           std::next(std::begin(all_values), kts::N));
    desired_values.assign(std::next(std::begin(all_values), kts::N),
                          std::end(all_values));

    kts::Reference1D<T> initializer_reference =
        [expected_values, success_indices](size_t index) {
          return expected_values[success_indices[index]];
        };

    kts::Reference1D<T> output_reference = [desired_values,
                                            success_indices](size_t index) {
      return desired_values[success_indices[index]];
    };

    kts::Reference1D<T> expected_in_reference =
        [expected_values](size_t index) { return expected_values[index]; };

    kts::Reference1D<T> expected_output_reference =
        [success_indices, expected_values, desired_values](size_t index,
                                                           T value) {
          // Expected output will contain its original value if at a success
          // index otherwise it will contain the value stored in the atomic
          // which will be either the initial value if the sucessful thread
          // hasn't executed the exchange yet, or the desired value of the
          // success index if it has.
          const size_t work_group = index / kts::localN;
          const size_t success_index_of_workgroup = success_indices[work_group];
          if (index == success_index_of_workgroup) {
            return value == expected_values[index];
          }

          return value == expected_values[success_index_of_workgroup] ||
                 value == desired_values[success_index_of_workgroup];
        };
    kts::Reference1D<T> desired_reference = [desired_values](size_t index) {
      return desired_values[index];
    };
    kts::Reference1D<cl_int> bool_output_reference =
        [success_indices](size_t index) {
          const size_t work_group = index / kts::localN;
          const size_t success_index_of_workgroup = success_indices[work_group];
          return index == success_index_of_workgroup;
        };
    // Set up the buffers.
    AddInOutBuffer(kts::N / kts::localN, initializer_reference,
                   output_reference);
    AddInOutBuffer(kts::N, expected_in_reference, expected_output_reference);
    AddInputBuffer(kts::N, desired_reference);
    AddOutputBuffer(kts::N, bool_output_reference);
    AddLocalBuffer<T>(1);
    if (local_local) {
      AddLocalBuffer<T>(kts::localN);
    }

    // Run the test.
    RunGeneric1D(kts::N, kts::localN);
  }
};

TEST_P(StrongLocalSingle,
       C11Atomics_64_Compare_Exchange_Strong_Local_Global_Single_Int) {
  doTest<cl_int>();
}
TEST_P(StrongLocalSingle,
       C11Atomics_64_Compare_Exchange_Strong_Local_Global_Single_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>();
}
TEST_P(StrongLocalSingle,
       C11Atomics_64_Compare_Exchange_Strong_Local_Global_Single_Uint) {
  doTest<cl_uint>();
}
TEST_P(StrongLocalSingle,
       C11Atomics_64_Compare_Exchange_Strong_Local_Global_Single_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>();
}

TEST_P(StrongLocalSingle,
       C11Atomics_65_Compare_Exchange_Strong_Local_Local_Single_Int) {
  doTest<cl_int>(/*local_local*/ true);
}
TEST_P(StrongLocalSingle,
       C11Atomics_65_Compare_Exchange_Strong_Local_Local_Single_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>(/*local_local*/ true);
}
TEST_P(StrongLocalSingle,
       C11Atomics_65_Compare_Exchange_Strong_Local_Local_Single_Uint) {
  doTest<cl_uint>(/*local_local*/ true);
}
TEST_P(StrongLocalSingle,
       C11Atomics_65_Compare_Exchange_Strong_Local_Local_Single_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>(/*local_local*/ true);
}

TEST_P(StrongLocalSingle,
       C11Atomics_66_Compare_Exchange_Strong_Local_Private_Single_Int) {
  doTest<cl_int>();
}
TEST_P(StrongLocalSingle,
       C11Atomics_66_Compare_Exchange_Strong_Local_Private_Single_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>();
}
TEST_P(StrongLocalSingle,
       C11Atomics_66_Compare_Exchange_Strong_Local_Private_Single_Uint) {
  doTest<cl_uint>();
}
TEST_P(StrongLocalSingle,
       C11Atomics_66_Compare_Exchange_Strong_Local_Private_Single_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>();
}

UCL_EXECUTION_TEST_SUITE(StrongLocalSingle, testing::ValuesIn(source_types));

class Weak : public C11AtomicTestBase {
 public:
  template <typename T>
  void doTest(bool local = false, bool local_local = false) {
    // Generate the random input.
    std::vector<T> input_data(kts::N, T{});
    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
    std::vector<T> desired_data(kts::N, T{});
    ucl::Environment::instance->GetInputGenerator().GenerateData(desired_data);

    // Set up references.
    kts::Reference1D<T> random_reference = [input_data](size_t index) {
      return input_data[index];
    };
    kts::Reference1D<T> expected_in_reference = [input_data](size_t index) {
      auto expected_value = input_data[index];
      // This ensures every other expected value matches the value in the
      // input.
      return (index % 2) ? expected_value : !expected_value;
    };
    std::vector<size_t> failed_comparison_indices;
    kts::Reference1D<T> output_reference = [input_data, desired_data,
                                            &failed_comparison_indices](
                                               size_t index, T value) {
      // Weak compare-exchange operations may fail spuriously, returning 0
      // when the contents of memory in expected and the atomic are equal,
      // it may return zero and store back to expected the same memory
      // contents that were originally there.

      // Check if we are at success index.
      if (index % 2) {
        // Check if the operation succeeded.
        if (value == desired_data[index]) {
          return true;
        }
        // If not the memory shouldn't have been updated and we need to
        // record which exchanges didn't succeed.
        failed_comparison_indices.push_back(index);
      }
      // Failure index and we need to check the memory wasn't updated.
      return value == input_data[index];
    };
    kts::Reference1D<T> desired_reference = [desired_data](size_t index) {
      return desired_data[index];
    };
    kts::Reference1D<cl_int> bool_reference = [&failed_comparison_indices](
                                                  size_t index, cl_int value) {
      // Check if we are at success index that didn't fail.
      if ((index % 2) &&
          std::find(std::begin(failed_comparison_indices),
                    std::end(failed_comparison_indices),
                    index) == std::end(failed_comparison_indices)) {
        return value == true;
      }

      // Otherwise we are either at failure index, or a success index that
      // failed.
      return value == false;
    };

    // Set up the buffers.
    AddInOutBuffer(kts::N, random_reference, output_reference);
    AddInOutBuffer(kts::N, expected_in_reference, random_reference);
    AddInputBuffer(kts::N, desired_reference);
    AddOutputBuffer(kts::N, bool_reference);
    if (!local) {
      RunGeneric1D(kts::N);
    } else {
      AddLocalBuffer<T>(kts::localN);
      if (local_local) {
        AddLocalBuffer<T>(kts::localN);
      }
      RunGeneric1D(kts::N, kts::localN);
    }
  }
};

TEST_P(Weak, C11Atomics_67_Compare_Exchange_Weak_Global_Global_Int) {
  doTest<cl_int>();
}
TEST_P(Weak, C11Atomics_67_Compare_Exchange_Weak_Global_Global_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>();
}
TEST_P(Weak, C11Atomics_67_Compare_Exchange_Weak_Global_Global_Uint) {
  doTest<cl_uint>();
}
TEST_P(Weak, C11Atomics_67_Compare_Exchange_Weak_Global_Global_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>();
}

TEST_P(Weak, C11Atomics_68_Compare_Exchange_Weak_Global_Local_Int) {
  doTest<cl_int>(/*local*/ true);
}
TEST_P(Weak, C11Atomics_68_Compare_Exchange_Weak_Global_Local_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>(/*local*/ true);
}
TEST_P(Weak, C11Atomics_68_Compare_Exchange_Weak_Global_Local_Uint) {
  doTest<cl_uint>(/*local*/ true);
}
TEST_P(Weak, C11Atomics_68_Compare_Exchange_Weak_Global_Local_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>(/*local*/ true);
}

TEST_P(Weak, C11Atomics_69_Compare_Exchange_Weak_Global_Private_Int) {
  doTest<cl_int>();
}
TEST_P(Weak, C11Atomics_69_Compare_Exchange_Weak_Global_Private_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>();
}
TEST_P(Weak, C11Atomics_69_Compare_Exchange_Weak_Global_Private_Uint) {
  doTest<cl_uint>();
}
TEST_P(Weak, C11Atomics_69_Compare_Exchange_Weak_Global_Private_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>();
}

TEST_P(Weak, C11Atomics_70_Compare_Exchange_Weak_Local_Global_Int) {
  doTest<cl_int>(/*local*/ true);
}
TEST_P(Weak, C11Atomics_70_Compare_Exchange_Weak_Local_Global_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>(/*local*/ true);
}
TEST_P(Weak, C11Atomics_70_Compare_Exchange_Weak_Local_Global_Uint) {
  doTest<cl_uint>(/*local*/ true);
}
TEST_P(Weak, C11Atomics_70_Compare_Exchange_Weak_Local_Global_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>(/*local*/ true);
}

TEST_P(Weak, C11Atomics_71_Compare_Exchange_Weak_Local_Local_Int) {
  doTest<cl_int>(/*local*/ true, /*local_local*/ true);
}
TEST_P(Weak, C11Atomics_71_Compare_Exchange_Weak_Local_Local_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>(/*local*/ true, /*local_local*/ true);
}
TEST_P(Weak, C11Atomics_71_Compare_Exchange_Weak_Local_Local_Uint) {
  doTest<cl_uint>(/*local*/ true, /*local_ulocal*/ true);
}
TEST_P(Weak, C11Atomics_71_Compare_Exchange_Weak_Local_Local_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>(/*local*/ true, /*local_ulocal*/ true);
}

TEST_P(Weak, C11Atomics_72_Compare_Exchange_Weak_Local_Private_Int) {
  doTest<cl_int>(/*local*/ true);
}
TEST_P(Weak, C11Atomics_72_Compare_Exchange_Weak_Local_Private_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>(/*local*/ true);
}
TEST_P(Weak, C11Atomics_72_Compare_Exchange_Weak_Local_Private_Uint) {
  doTest<cl_uint>(/*local*/ true);
}
TEST_P(Weak, C11Atomics_72_Compare_Exchange_Weak_Local_Private_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>(/*local*/ true);
}

UCL_EXECUTION_TEST_SUITE(Weak, testing::ValuesIn(source_types));

class WeakGlobalSingle : public C11AtomicTestBase {
 public:
  template <typename T>
  void doTest(bool local = false) {
    bool weak_exchange_failed = false;
    // Set up references.
    size_t success_index =
        ucl::Environment::instance->GetInputGenerator().GenerateInt<T>(
            0, kts::N - 1);

    // We need the expected values to be unique, otherwise we won't be
    // able to determine which thread updates the atomic. We also require
    // the intersection of the expected and desired values to be empty
    // otherwise subsequent threads could update the atomic. The fastest
    // way to do this is to generate a buffer of unqique values of size 2
    // * kts::N, then just split it evenly between the two.
    std::vector<T> expected_values(kts::N, T{});
    std::vector<T> desired_values(kts::N, T{});
    std::vector<T> all_values(expected_values.size() + desired_values.size(),
                              T{});

    ucl::Environment::instance->GetInputGenerator().GenerateUniqueIntData<T>(
        all_values);
    expected_values.assign(std::begin(all_values),
                           std::next(std::begin(all_values), kts::N));
    desired_values.assign(std::next(std::begin(all_values), kts::N),
                          std::end(all_values));

    kts::Reference1D<T> initializer_reference = [success_index,
                                                 expected_values](size_t) {
      return expected_values[success_index];
    };
    kts::Reference1D<T> output_reference =
        [&expected_values, &desired_values, success_index,
         &weak_exchange_failed](size_t, T value) {
          // Weak compare-exchange operations may fail spuriously,
          // returning 0 when the contents of memory in expected and the
          // atomic are equal, it may return zero and store back to
          // expected the same memory contents that were originally there.
          if (value == expected_values[success_index]) {
            weak_exchange_failed = true;
            return true;
          }
          return value == desired_values[success_index];
        };
    kts::Reference1D<T> expected_in_reference =
        [expected_values](size_t index) { return expected_values[index]; };
    kts::Reference1D<T> expected_output_reference =
        [success_index, expected_values, desired_values](size_t, T value) {
          return value == expected_values[success_index] ||
                 value == desired_values[success_index];
        };
    kts::Reference1D<T> desired_reference = [desired_values](size_t index) {
      return desired_values[index];
    };
    kts::Reference1D<cl_int> bool_output_reference =
        [success_index, weak_exchange_failed](size_t index) {
          return index == success_index && !weak_exchange_failed;
        };

    // Set up the buffers.
    AddInOutBuffer(1, initializer_reference, output_reference);
    AddInOutBuffer(kts::N, expected_in_reference, expected_output_reference);
    AddInputBuffer(kts::N, desired_reference);
    AddOutputBuffer(kts::N, bool_output_reference);
    if (!local) {
      RunGeneric1D(kts::N);
    } else {
      AddLocalBuffer<T>(kts::localN);
      RunGeneric1D(kts::N, kts::localN);
    }
  }
};

TEST_P(WeakGlobalSingle,
       C11Atomics_73_Compare_Exchange_Weak_Global_Global_Single_Int) {
  doTest<cl_int>();
}
TEST_P(WeakGlobalSingle,
       C11Atomics_73_Compare_Exchange_Weak_Global_Global_Single_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>();
}
TEST_P(WeakGlobalSingle,
       C11Atomics_73_Compare_Exchange_Weak_Global_Global_Single_Uint) {
  doTest<cl_uint>();
}
TEST_P(WeakGlobalSingle,
       C11Atomics_73_Compare_Exchange_Weak_Global_Global_Single_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>();
}

TEST_P(WeakGlobalSingle,
       C11Atomics_74_Compare_Exchange_Weak_Global_Local_Single_Int) {
  doTest<cl_int>(/*local*/ true);
}
TEST_P(WeakGlobalSingle,
       C11Atomics_74_Compare_Exchange_Weak_Global_Local_Single_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>(/*local*/ true);
}
TEST_P(WeakGlobalSingle,
       C11Atomics_74_Compare_Exchange_Weak_Global_Local_Single_Uint) {
  doTest<cl_uint>(/*local*/ true);
}
TEST_P(WeakGlobalSingle,
       C11Atomics_74_Compare_Exchange_Weak_Global_Local_Single_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>(/*local*/ true);
}

TEST_P(WeakGlobalSingle,
       C11Atomics_75_Compare_Exchange_Weak_Global_Private_Single_Int) {
  doTest<cl_int>();
}
TEST_P(WeakGlobalSingle,
       C11Atomics_75_Compare_Exchange_Weak_Global_Private_Single_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>();
}
TEST_P(WeakGlobalSingle,
       C11Atomics_75_Compare_Exchange_Weak_Global_Private_Single_Uint) {
  doTest<cl_uint>();
}
TEST_P(WeakGlobalSingle,
       C11Atomics_75_Compare_Exchange_Weak_Global_Private_Single_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>();
}

UCL_EXECUTION_TEST_SUITE(WeakGlobalSingle, testing::ValuesIn(source_types));

class WeakLocalSingle : public C11AtomicTestBase {
 public:
  template <typename T>
  void doTest(bool local_local = false) {
    std::vector<size_t> success_indices(kts::N / kts::localN, 0);
    std::vector<bool> weak_exchanges_failed(kts::N / kts::localN, false);
    // Set up references.
    // Pick a random index in each work-group to hold the correct
    // expected value.
    ucl::Environment::instance->GetInputGenerator().GenerateIntData<size_t>(
        success_indices, 0, kts::localN - 1);
    // Calculate the global id of these indices
    for (unsigned i = 0; i < success_indices.size(); ++i) {
      success_indices[i] += i * kts::localN;
    }

    // We still need N expected values, there will be exactly one correct
    // value in each work-group. We also need N desired values where each
    // work-group has an empty intersection with its expected values. The
    // easiest way to do this is to just generate 2 * kts::N uniqueue values
    // and divide them between the two buffers.
    std::vector<T> expected_values(kts::N, T{});
    std::vector<T> desired_values(kts::N, T{});
    std::vector<T> all_values(expected_values.size() + desired_values.size(),
                              T{});

    ucl::Environment::instance->GetInputGenerator().GenerateUniqueIntData(
        all_values);
    expected_values.assign(std::begin(all_values),
                           std::next(std::begin(all_values), kts::N));
    desired_values.assign(std::next(std::begin(all_values), kts::N),
                          std::end(all_values));

    kts::Reference1D<T> initializer_reference =
        [&expected_values, &success_indices](size_t index) {
          return expected_values[success_indices[index]];
        };

    kts::Reference1D<T> output_reference =
        [&success_indices, &weak_exchanges_failed, &expected_values,
         &desired_values](size_t index, T value) {
          // Weak compare-exchange operations may fail spuriously, returning 0
          // when the contents of memory in expected and the atomic are equal,
          // it may return zero and store back to expected the same memory
          // contents that were originally there.
          if (value == expected_values[success_indices[index]]) {
            weak_exchanges_failed[index / kts::localN] = true;
            return true;
          }

          return value == desired_values[success_indices[index]];
        };

    kts::Reference1D<T> expected_in_reference =
        [&expected_values](size_t index) { return expected_values[index]; };

    kts::Reference1D<T> expected_output_reference =
        [&expected_values, &desired_values, &success_indices](size_t index,
                                                              T value) {
          // Expected output will contain its original value if at a success
          // index otherwise it will contain the value stored in the atomic
          // which will be either the initial value if the sucessful thread
          // hasn't executed the exchange yet, or the desired value of the
          // success index if it has.
          const size_t work_group = index / kts::localN;
          const size_t success_index_of_workgroup = success_indices[work_group];
          return value == expected_values[success_index_of_workgroup] ||
                 value == desired_values[success_index_of_workgroup];
        };
    kts::Reference1D<T> desired_reference = [desired_values](size_t index) {
      return desired_values[index];
    };
    kts::Reference1D<cl_int> bool_output_reference =
        [&success_indices, &weak_exchanges_failed](size_t index) {
          const size_t work_group = index / kts::localN;
          const size_t success_index_of_workgroup = success_indices[work_group];
          return (index == success_index_of_workgroup) &&
                 !weak_exchanges_failed[work_group];
        };
    // Set up the buffers.
    AddInOutBuffer(kts::N / kts::localN, initializer_reference,
                   output_reference);
    AddInOutBuffer(kts::N, expected_in_reference, expected_output_reference);
    AddInputBuffer(kts::N, desired_reference);
    AddOutputBuffer(kts::N, bool_output_reference);
    AddLocalBuffer<T>(1);
    if (local_local) {
      AddLocalBuffer<T>(kts::localN);
    }

    // Run the test.
    RunGeneric1D(kts::N, kts::localN);
  }
};

TYPED_TEST_SUITE_P(WeakLocalSingle);

TEST_P(WeakLocalSingle,
       C11Atomics_76_Compare_Exchange_Weak_Local_Global_Single_Int) {
  doTest<cl_int>();
}
TEST_P(WeakLocalSingle,
       C11Atomics_76_Compare_Exchange_Weak_Local_Global_Single_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>();
}
TEST_P(WeakLocalSingle,
       C11Atomics_76_Compare_Exchange_Weak_Local_Global_Single_Uint) {
  doTest<cl_uint>();
}
TEST_P(WeakLocalSingle,
       C11Atomics_76_Compare_Exchange_Weak_Local_Global_Single_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>();
}

TEST_P(WeakLocalSingle,
       C11Atomics_77_Compare_Exchange_Weak_Local_Local_Single_Int) {
  doTest<cl_int>(/*local_local*/ true);
}
TEST_P(WeakLocalSingle,
       C11Atomics_77_Compare_Exchange_Weak_Local_Local_Single_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>(/*local_local*/ true);
}
TEST_P(WeakLocalSingle,
       C11Atomics_77_Compare_Exchange_Weak_Local_Local_Single_Uint) {
  doTest<cl_uint>(/*local_local*/ true);
}
TEST_P(WeakLocalSingle,
       C11Atomics_77_Compare_Exchange_Weak_Local_Local_Single_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>(/*local_local*/ true);
}

TEST_P(WeakLocalSingle,
       C11Atomics_78_Compare_Exchange_Weak_Local_Private_Single_Int) {
  doTest<cl_int>();
}
TEST_P(WeakLocalSingle,
       C11Atomics_78_Compare_Exchange_Weak_Local_Private_Single_Long) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_long>();
}
TEST_P(WeakLocalSingle,
       C11Atomics_78_Compare_Exchange_Weak_Local_Private_Single_Uint) {
  doTest<cl_uint>();
}
TEST_P(WeakLocalSingle,
       C11Atomics_78_Compare_Exchange_Weak_Local_Private_Single_Ulong) {
  if (!UCL::hasAtomic64Support(device)) {
    GTEST_SKIP();
  }
  doTest<cl_ulong>();
}

UCL_EXECUTION_TEST_SUITE(WeakLocalSingle, testing::ValuesIn(source_types));
