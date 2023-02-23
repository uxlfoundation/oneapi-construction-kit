// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <kts/precision.h>

#include <numeric>
#include <vector>

#include "Common.h"
#include "kts/reference_functions.h"

using namespace kts::ucl;

// The supported atomic types.
using AtomicTypes = testing::Types<ucl::Int, ucl::UInt, ucl::Float>;

template <typename T>
class InitTest : public BaseExecution {};

TYPED_TEST_SUITE_P(InitTest);

TYPED_TEST_P(InitTest, C11Atomics_01_Init_Global) {
  // The C11 atomic were introduced in 2.0 however here we only test
  // the minimum required subset for 3.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  using cl_type = typename TypeParam::cl_type;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  // Set up references.
  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  this->AddInputBuffer(kts::N, random_reference);
  this->AddOutputBuffer(kts::N, random_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(InitTest, C11Atomics_02_Init_Local) {
  // The C11 atomic were introduced in 2.0 however here we only test
  // the minimum required subset for 3.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  using cl_type = typename TypeParam::cl_type;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  // Set up references.
  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  this->AddInputBuffer(kts::N, random_reference);
  this->AddOutputBuffer(kts::N, random_reference);
  this->AddLocalBuffer(kts::N);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

REGISTER_TYPED_TEST_SUITE_P(InitTest, C11Atomics_01_Init_Global,
                            C11Atomics_02_Init_Local);

INSTANTIATE_TYPED_TEST_SUITE_P(AtomicTypes, InitTest, AtomicTypes);

class FenceTest : public BaseExecution {};

TEST_F(FenceTest, C11Atomics_03_Fence_Acquire_Release) {
  // The C11 atomic were introduced in 2.0 however here we only test
  // the minimum required subset for 3.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  this->AddBuildOption("-cl-std=CL3.0");

  // Set up the buffers.
  this->AddInputBuffer(kts::N, kts::Ref_Identity);
  kts::Reference1D<cl_int> zero_reference = [](size_t) { return cl_int{0}; };
  this->AddInputBuffer(kts::N, zero_reference);
  this->AddOutputBuffer(kts::N, kts::Ref_Identity);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TEST_F(FenceTest, C11Atomics_04_Fence_Acquire) {
  // The C11 atomic were introduced in 2.0 however here we only test
  // the minimum required subset for 3.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  this->AddBuildOption("-cl-std=CL3.0");

  // Set up the buffers.
  this->AddInputBuffer(kts::N, kts::Ref_Identity);
  kts::Reference1D<cl_int> zero_reference = [](size_t) { return cl_int{0}; };
  this->AddInputBuffer(kts::N, zero_reference);
  this->AddOutputBuffer(kts::N, kts::Ref_Identity);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TEST_F(FenceTest, C11Atomics_05_Fence_Release) {
  // The C11 atomic were introduced in 2.0 however here we only test
  // the minimum required subset for 3.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  this->AddBuildOption("-cl-std=CL3.0");

  // Set up the buffers.
  this->AddInputBuffer(kts::N, kts::Ref_Identity);
  kts::Reference1D<cl_int> zero_reference = [](size_t) { return cl_int{0}; };
  this->AddInputBuffer(kts::N, zero_reference);
  this->AddOutputBuffer(kts::N, kts::Ref_Identity);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TEST_F(FenceTest, C11Atomics_06_Fence_Relaxed) {
  // The C11 atomic were introduced in 2.0 however here we only test
  // the minimum required subset for 3.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Set up the buffers.
  this->AddInputBuffer(kts::N, kts::Ref_Identity);
  kts::Reference1D<cl_int> zero_reference = [](size_t) { return cl_int{0}; };
  this->AddInputBuffer(kts::N, zero_reference);
  this->AddOutputBuffer(kts::N, kts::Ref_Identity);

  this->AddBuildOption("-cl-std=CL3.0");

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TEST_F(FenceTest, C11Atomics_07_Fence_Global) {
  // The C11 atomic were introduced in 2.0 however here we only test
  // the minimum required subset for 3.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  this->AddBuildOption("-cl-std=CL3.0");

  // Set up the buffers.
  this->AddInputBuffer(kts::N, kts::Ref_Identity);
  kts::Reference1D<cl_int> zero_reference = [](size_t) { return cl_int{0}; };
  this->AddInputBuffer(kts::N, zero_reference);
  this->AddOutputBuffer(kts::N, kts::Ref_Identity);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TEST_F(FenceTest, C11Atomics_08_Fence_Local) {
  // The C11 atomic were introduced in 2.0 however here we only test
  // the minimum required subset for 3.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  this->AddBuildOption("-cl-std=CL3.0");

  // Set up the buffers.
  this->AddInputBuffer(kts::N, kts::Ref_Identity);
  this->AddOutputBuffer(kts::N, kts::Ref_Identity);
  this->AddLocalBuffer(kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

template <typename T>
class StoreTest : public BaseExecution {};

TYPED_TEST_SUITE_P(StoreTest);

TYPED_TEST_P(StoreTest, C11Atomics_09_Store_Local) {
  // The C11 atomic were introduced in 2.0 however here we only test
  // the minimum required subset for 3.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  using cl_type = typename TypeParam::cl_type;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  this->AddInputBuffer(kts::N, random_reference);
  this->AddOutputBuffer(kts::N, random_reference);
  this->AddLocalBuffer(kts::N);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(StoreTest, C11Atomics_10_Store_Global) {
  // The C11 atomic were introduced in 2.0 however here we only test
  // the minimum required subset for 3.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  using cl_type = typename TypeParam::cl_type;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  this->AddInputBuffer(kts::N, random_reference);
  this->AddOutputBuffer(kts::N, random_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

REGISTER_TYPED_TEST_SUITE_P(StoreTest, C11Atomics_09_Store_Local,
                            C11Atomics_10_Store_Global);

INSTANTIATE_TYPED_TEST_SUITE_P(AtomicTypes, StoreTest, AtomicTypes);

template <typename T>
class LoadTest : public BaseExecution {};

TYPED_TEST_SUITE_P(LoadTest);

TYPED_TEST_P(LoadTest, C11Atomics_11_Load_Local) {
  // The C11 atomic were introduced in 2.0 however here we only test
  // the minimum required subset for 3.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  using cl_type = typename TypeParam::cl_type;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  this->AddInputBuffer(kts::N, random_reference);
  this->AddOutputBuffer(kts::N, random_reference);
  this->AddLocalBuffer(kts::N);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(LoadTest, C11Atomics_12_Load_Global) {
  // The C11 atomic were introduced in 2.0 however here we only test
  // the minimum required subset for 3.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  using cl_type = typename TypeParam::cl_type;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  this->AddInputBuffer(kts::N, random_reference);
  this->AddOutputBuffer(kts::N, random_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

REGISTER_TYPED_TEST_SUITE_P(LoadTest, C11Atomics_11_Load_Local,
                            C11Atomics_12_Load_Global);

INSTANTIATE_TYPED_TEST_SUITE_P(AtomicTypes, LoadTest, AtomicTypes);

template <typename T>
class ExchangeTest : public BaseExecution {};

TYPED_TEST_SUITE_P(ExchangeTest);

TYPED_TEST_P(ExchangeTest, C11Atomics_13_Exchange_Local) {
  // The C11 atomic were introduced in 2.0 however here we only test
  // the minimum required subset for 3.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  using cl_type = typename TypeParam::cl_type;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  std::vector<cl_type> initializer_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(
      initializer_data);

  std::vector<cl_type> desired_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(desired_data);

  kts::Reference1D<cl_type> initializer_reference =
      [&initializer_data](size_t index) { return initializer_data[index]; };

  kts::Reference1D<cl_type> desired_reference = [&desired_data](size_t index) {
    return desired_data[index];
  };

  // Set up the buffers.
  // The initial values of the atomics are the input.
  // The desired values exchanged into the atomics are the expected output.
  this->AddInOutBuffer(kts::N, initializer_reference, desired_reference);
  // The desired values exchanged into the atomics are the input.
  this->AddInputBuffer(kts::N, desired_reference);
  // The initial atomic values exchanged out of the atomics are the expected
  // output.
  this->AddOutputBuffer(kts::N, initializer_reference);
  this->AddLocalBuffer(kts::N);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(ExchangeTest, C11Atomics_14_Exchange_Global) {
  // The C11 atomic were introduced in 2.0 however here we only test
  // the minimum required subset for 3.0.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  using cl_type = typename TypeParam::cl_type;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  std::vector<cl_type> initializer_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(
      initializer_data);

  std::vector<cl_type> desired_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(desired_data);

  kts::Reference1D<cl_type> initializer_reference =
      [&initializer_data](size_t index) { return initializer_data[index]; };

  kts::Reference1D<cl_type> desired_reference = [&desired_data](size_t index) {
    return desired_data[index];
  };

  // Set up the buffers.
  // The initial values of the atomics are the input.
  // The desired values exchanged into the atomics are the expected output.
  this->AddInOutBuffer(kts::N, initializer_reference, desired_reference);
  // The desired values exchanged into the atomics are the input.
  this->AddInputBuffer(kts::N, desired_reference);
  // The initial atomic values exchanged out of the atomics are the expected
  // output.
  this->AddOutputBuffer(kts::N, initializer_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

REGISTER_TYPED_TEST_SUITE_P(ExchangeTest, C11Atomics_13_Exchange_Local,
                            C11Atomics_14_Exchange_Global);

INSTANTIATE_TYPED_TEST_SUITE_P(AtomicTypes, ExchangeTest, AtomicTypes);

class FlagTest : public BaseExecution {
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

TEST_F(FlagTest, C11Atomics_17_Flag_Local_Clear_Set) {
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
  this->AddLocalBuffer(kts::N);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TEST_F(FlagTest, C11Atomics_18_Flag_Local_Set_Twice) {
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
  this->AddLocalBuffer(kts::N);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TEST_F(FlagTest, C11Atomics_19_Flag_Global_Clear) {
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

TEST_F(FlagTest, C11Atomics_20_Flag_Global_Set_Once) {
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

using AtomicIntegerTypes = testing::Types<ucl::Int, ucl::UInt>;

template <typename T>
class FetchTest : public BaseExecution {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(BaseExecution::SetUp());
    // The C11 atomic were introduced in 2.0 however here we only test
    // the minimum required subset for 3.0.
    if (!UCL::isDeviceVersionAtLeast({3, 0}) ||
        !UCL::hasCompilerSupport(device)) {
      GTEST_SKIP();
    }
  }
};

TYPED_TEST_SUITE_P(FetchTest);

TYPED_TEST_P(FetchTest, C11Atomics_21_Fetch_Global_Add_Check_Return) {
  using cl_type = typename TypeParam::cl_type;
  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  // The initial values of the atomics are the random input.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output values are the initial values loaded atomically.
  this->AddOutputBuffer(kts::N, random_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(FetchTest, C11Atomics_22_Fetch_Global_Sub_Check_Return) {
  using cl_type = typename TypeParam::cl_type;
  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  // The initial values of the atomics are the random input.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output values are the initial values loaded atomically.
  this->AddOutputBuffer(kts::N, random_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(FetchTest, C11Atomics_23_Fetch_Global_Or_Check_Return) {
  using cl_type = typename TypeParam::cl_type;
  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  // The initial values of the atomics are the random input.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output values are the initial values loaded atomically.
  this->AddOutputBuffer(kts::N, random_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(FetchTest, C11Atomics_24_Fetch_Global_Xor_Check_Return) {
  using cl_type = typename TypeParam::cl_type;
  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  // The initial values of the atomics are the random input.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output values are the initial values loaded atomically.
  this->AddOutputBuffer(kts::N, random_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(FetchTest, C11Atomics_25_Fetch_Global_And_Check_Return) {
  using cl_type = typename TypeParam::cl_type;
  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  // The initial values of the atomics are the random input.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output values are the initial values loaded atomically.
  this->AddOutputBuffer(kts::N, random_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(FetchTest, C11Atomics_26_Fetch_Global_Min_Check_Return) {
  using cl_type = typename TypeParam::cl_type;
  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  // The initial values of the atomics are the random input.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output values are the initial values loaded atomically.
  this->AddOutputBuffer(kts::N, random_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(FetchTest, C11Atomics_27_Fetch_Global_Max_Check_Return) {
  using cl_type = typename TypeParam::cl_type;
  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  // The initial values of the atomics are the random input.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output values are the initial values loaded atomically.
  this->AddOutputBuffer(kts::N, random_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(FetchTest, C11Atomics_28_Fetch_Local_Add_Check_Return) {
  using cl_type = typename TypeParam::cl_type;
  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  // The initial values of the atomics are the random input.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output values are the initial values loaded atomically.
  this->AddOutputBuffer(kts::N, random_reference);
  this->AddLocalBuffer(kts::N);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(FetchTest, C11Atomics_29_Fetch_Local_Sub_Check_Return) {
  using cl_type = typename TypeParam::cl_type;
  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  // The initial values of the atomics are the random input.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output values are the initial values loaded atomically.
  this->AddOutputBuffer(kts::N, random_reference);
  this->AddLocalBuffer(kts::N);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(FetchTest, C11Atomics_30_Fetch_Local_Or_Check_Return) {
  using cl_type = typename TypeParam::cl_type;
  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  // The initial values of the atomics are the random input.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output values are the initial values loaded atomically.
  this->AddOutputBuffer(kts::N, random_reference);
  this->AddLocalBuffer(kts::N);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(FetchTest, C11Atomics_31_Fetch_Local_Xor_Check_Return) {
  using cl_type = typename TypeParam::cl_type;
  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  // The initial values of the atomics are the random input.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output values are the initial values loaded atomically.
  this->AddOutputBuffer(kts::N, random_reference);
  this->AddLocalBuffer(kts::N);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(FetchTest, C11Atomics_32_Fetch_Local_And_Check_Return) {
  using cl_type = typename TypeParam::cl_type;
  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  // The initial values of the atomics are the random input.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output values are the initial values loaded atomically.
  this->AddOutputBuffer(kts::N, random_reference);
  this->AddLocalBuffer(kts::N);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(FetchTest, C11Atomics_33_Fetch_Local_Min_Check_Return) {
  using cl_type = typename TypeParam::cl_type;
  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  // The initial values of the atomics are the random input.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output values are the initial values loaded atomically.
  this->AddOutputBuffer(kts::N, random_reference);
  this->AddLocalBuffer(kts::N);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(FetchTest, C11Atomics_34_Fetch_Local_Max_Check_Return) {
  using cl_type = typename TypeParam::cl_type;
  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);

  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up the buffers.
  // The initial values of the atomics are the random input.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output values are the initial values loaded atomically.
  this->AddOutputBuffer(kts::N, random_reference);
  this->AddLocalBuffer(kts::N);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(FetchTest, C11Atomics_35_Fetch_Global_Add) {
  using cl_type = typename TypeParam::cl_type;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  // We need to be careful we don't overflow, so limit the min and max values.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  const cl_type min =
      std::numeric_limits<cl_type>::min() / static_cast<cl_type>(kts::N);
  const cl_type max =
      std::numeric_limits<cl_type>::max() / static_cast<cl_type>(kts::N);
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(input_data,
                                                                  min, max);
  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };
  kts::Reference1D<cl_type> zero_reference = [](size_t) { return cl_type{0}; };
  kts::Reference1D<cl_type> accumulate_reference = [&input_data](size_t) {
    return std::accumulate(std::begin(input_data), std::end(input_data),
                           cl_type{0});
  };

  // Set up the buffers.
  // The initial atomic value is zero.
  // The expected output is the sum of the values in all threads.
  this->AddInOutBuffer(1, zero_reference, accumulate_reference);
  // The input values to be summed.
  this->AddInputBuffer(kts::N, random_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(FetchTest, C11Atomics_36_Fetch_Local_Add) {
  using cl_type = typename TypeParam::cl_type;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  // We need to be careful we don't overflow, so limit the min and max values.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  const cl_type min =
      std::numeric_limits<cl_type>::min() / static_cast<cl_type>(kts::N);
  const cl_type max =
      std::numeric_limits<cl_type>::max() / static_cast<cl_type>(kts::N);
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(input_data,
                                                                  min, max);
  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  kts::Reference1D<cl_type> zero_reference = [](size_t) { return cl_type{0}; };
  kts::Reference1D<cl_type> accumulate_reference = [&input_data](size_t index) {
    auto start = std::next(std::begin(input_data), index * kts::localN);
    auto end = start + kts::localN;
    return std::accumulate(start, end, cl_type{0});
  };

  // Set up the buffers.
  // The input values to be summed.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output is the sum of the elements in each work group.
  this->AddOutputBuffer(kts::N / kts::localN, accumulate_reference);
  this->AddLocalBuffer(kts::N / kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(FetchTest, C11Atomics_37_Fetch_Global_Sub) {
  using cl_type = typename TypeParam::cl_type;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  // We need to be careful we don't overflow, so limit the min and max values.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  const cl_type min =
      std::numeric_limits<cl_type>::min() / static_cast<cl_type>(kts::N);
  const cl_type max =
      std::numeric_limits<cl_type>::max() / static_cast<cl_type>(kts::N);
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(input_data,
                                                                  min, max);
  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };
  kts::Reference1D<cl_type> zero_reference = [](size_t) { return cl_type{0}; };
  kts::Reference1D<cl_type> accumulate_reference = [&input_data](size_t) {
    return std::accumulate(std::begin(input_data), std::end(input_data),
                           cl_type{0});
  };

  // Set up the buffers.
  // The input is the sum of all the random values.
  // The expected output is subtracting all the random values from their sum
  // i.e. 0.
  this->AddInOutBuffer(1, accumulate_reference, zero_reference);
  // The input is the random values.
  this->AddInputBuffer(kts::N, random_reference);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(FetchTest, C11Atomics_38_Fetch_Local_Sub) {
  using cl_type = typename TypeParam::cl_type;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  // We need to be careful we don't overflow, so limit the min and max values.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  const cl_type min =
      std::numeric_limits<cl_type>::min() / static_cast<cl_type>(kts::N);
  const cl_type max =
      std::numeric_limits<cl_type>::max() / static_cast<cl_type>(kts::N);
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(input_data,
                                                                  min, max);
  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };
  kts::Reference1D<cl_type> accumulate_reference = [&input_data](size_t index) {
    auto start = std::next(std::begin(input_data), index * kts::localN);
    auto end = start + kts::localN;
    return std::accumulate(start, end, cl_type{0});
  };
  kts::Reference1D<cl_type> zero_reference = [](size_t) { return cl_type{0}; };

  // Set up the buffers.
  // The input is the random values.
  this->AddInputBuffer(kts::N, random_reference);
  // The output for each work-group is all of it's elements subracted from their
  // sum i.e. 0.
  this->AddOutputBuffer(kts::N / kts::localN, zero_reference);
  // The input for each work-group is the sum of the work-groups elements.
  this->AddInputBuffer(kts::N / kts::localN, accumulate_reference);
  this->AddLocalBuffer(kts::N / kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(FetchTest, C11Atomics_39_Fetch_Global_Or) {
  using cl_type = typename TypeParam::cl_type;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(input_data);

  // Set up references.
  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };
  kts::Reference1D<cl_type> or_reference = [&input_data](size_t) {
    return std::accumulate(std::next(std::begin(input_data), 1),
                           std::end(input_data), input_data[0],
                           [](cl_type lhs, cl_type rhs) { return lhs | rhs; });
  };
  kts::Reference1D<cl_type> intiailizer_reference = [&input_data](size_t) {
    return input_data[0];
  };

  // Set up the buffers.
  // The input is the first element.
  // The expected output is all the input bits or'd together.
  this->AddInOutBuffer(1, intiailizer_reference, or_reference);
  // The input is a random set of on and off bits.
  this->AddInputBuffer(kts::N, random_reference);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(FetchTest, C11Atomics_40_Fetch_Local_Or) {
  using cl_type = typename TypeParam::cl_type;

  // This test makes use of control flow in the kernel. Control flow conversion
  // is not supported for atomics so we need to make sure this isn't registered
  // as a failure when the vectorizer fails. See CA-3294.
  this->fail_if_not_vectorized_ = false;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(input_data);

  // Set up references.
  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };
  kts::Reference1D<cl_type> or_reference = [&input_data](size_t index) {
    auto start = std::next(std::begin(input_data), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return std::accumulate(std::next(start), end, *start,
                           [](cl_type lhs, cl_type rhs) { return lhs | rhs; });
  };

  // Set up the buffers.
  // The input is a random set of on and off bits.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output for each work-group is all the input bits in the
  // work-group or'd together.
  this->AddOutputBuffer(kts::N / kts::localN, or_reference);
  this->AddLocalBuffer(kts::N / kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(FetchTest, C11Atomics_41_Fetch_Global_Xor) {
  using cl_type = typename TypeParam::cl_type;

  // This test makes use of control flow in the kernel. Control flow conversion
  // is not supported for atomics so we need to make sure this isn't registered
  // as a failure when the vectorizer fails. See CA-3294.
  this->fail_if_not_vectorized_ = false;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(input_data);

  // Set up references.
  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };
  kts::Reference1D<cl_type> xor_reference = [&input_data](size_t) {
    return std::accumulate(std::next(std::begin(input_data), 1),
                           std::end(input_data), input_data[0],
                           [](cl_type lhs, cl_type rhs) { return lhs ^ rhs; });
  };
  kts::Reference1D<cl_type> intiailizer_reference = [&input_data](size_t) {
    return input_data[0];
  };

  // Set up the buffers.
  // The input is the first element (this is because xor isn't idempotent).
  // The expected output is all the input bits xor'd together.
  this->AddInOutBuffer(1, intiailizer_reference, xor_reference);
  // The input is a random set of on and off bits.
  this->AddInputBuffer(kts::N, random_reference);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(FetchTest, C11Atomics_42_Fetch_Local_Xor) {
  using cl_type = typename TypeParam::cl_type;

  // This test makes use of control flow in the kernel. Control flow conversion
  // is not supported for atomics so we need to make sure this isn't registered
  // as a failure when the vectorizer fails. See CA-3294.
  this->fail_if_not_vectorized_ = false;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(input_data);

  // Set up references.
  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };
  kts::Reference1D<cl_type> xor_reference = [&input_data](size_t index) {
    auto start = std::next(std::begin(input_data), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return std::accumulate(std::next(start), end, *start,
                           [](cl_type lhs, cl_type rhs) { return lhs ^ rhs; });
  };

  // Set up the buffers.
  // The input is a random set of on and off bits.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output for each work-group is all the input bits in the
  // work-group xor'd together.
  this->AddOutputBuffer(kts::N / kts::localN, xor_reference);
  this->AddLocalBuffer(kts::N / kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(FetchTest, C11Atomics_43_Fetch_Global_And) {
  using cl_type = typename TypeParam::cl_type;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(input_data);

  // Set up references.
  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };
  kts::Reference1D<cl_type> and_reference = [&input_data](size_t) {
    return std::accumulate(std::next(std::begin(input_data), 1),
                           std::end(input_data), input_data[0],
                           [](cl_type lhs, cl_type rhs) { return lhs & rhs; });
  };
  kts::Reference1D<cl_type> intiailizer_reference = [&input_data](size_t) {
    return input_data[0];
  };

  // Set up the buffers.
  // The input is the first element.
  // The expected output is all the input bits and'd together.
  this->AddInOutBuffer(1, intiailizer_reference, and_reference);
  // The input is a random set of on and off bits.
  this->AddInputBuffer(kts::N, random_reference);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(FetchTest, C11Atomics_44_Fetch_Local_And) {
  using cl_type = typename TypeParam::cl_type;

  // This test makes use of control flow in the kernel. Control flow conversion
  // is not supported for atomics so we need to make sure this isn't registered
  // as a failure when the vectorizer fails. See CA-3294.
  this->fail_if_not_vectorized_ = false;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(input_data);

  // Set up references.
  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };
  kts::Reference1D<cl_type> and_reference = [&input_data](size_t index) {
    auto start = std::next(std::begin(input_data), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return std::accumulate(std::next(start), end, *start,
                           [](cl_type lhs, cl_type rhs) { return lhs & rhs; });
  };

  // Set up the buffers.
  // The input is a random set of on and off bits.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output for each work-group is all the input bits in the
  // work-group and'd together.
  this->AddOutputBuffer(kts::N / kts::localN, and_reference);
  this->AddLocalBuffer(kts::N / kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(FetchTest, C11Atomics_45_Fetch_Global_Min) {
  using cl_type = typename TypeParam::cl_type;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(input_data);

  // Set up references.
  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };
  kts::Reference1D<cl_type> min_reference = [&input_data](size_t) {
    return *std::min_element(std::begin(input_data), std::end(input_data));
  };
  kts::Reference1D<cl_type> intiailizer_reference = [&input_data](size_t) {
    return input_data[0];
  };

  // Set up the buffers.
  // The input is the first element.
  // The expected output is the minimum value of all elements.
  this->AddInOutBuffer(1, intiailizer_reference, min_reference);
  // The input is a set of random values.
  this->AddInputBuffer(kts::N, random_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(FetchTest, C11Atomics_46_Fetch_Local_Min) {
  using cl_type = typename TypeParam::cl_type;

  // This test makes use of control flow in the kernel. Control flow conversion
  // is not supported for atomics so we need to make sure this isn't registered
  // as a failure when the vectorizer fails. See CA-3294.
  this->fail_if_not_vectorized_ = false;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(input_data);
  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up references.
  kts::Reference1D<cl_type> min_reference = [&input_data](size_t index) {
    auto start = std::next(std::begin(input_data), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return *std::min_element(start, end);
  };

  // Set up the buffers.
  // The input is a set of random values.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output for each work-group is the minimum of all elements in
  // that work-group.
  this->AddOutputBuffer(kts::N / kts::localN, min_reference);
  this->AddLocalBuffer(kts::N / kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(FetchTest, C11Atomics_47_Fetch_Global_Max) {
  using cl_type = typename TypeParam::cl_type;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(input_data);

  // Set up references.
  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };
  kts::Reference1D<cl_type> max_reference = [&input_data](size_t) {
    return *std::max_element(std::begin(input_data), std::end(input_data));
  };
  kts::Reference1D<cl_type> intiailizer_reference = [&input_data](size_t) {
    return input_data[0];
  };

  // Set up the buffers.
  // The input is the first element.
  // The expected output is the maximum value of all elements.
  this->AddInOutBuffer(1, intiailizer_reference, max_reference);
  // The input is a set of random values.
  this->AddInputBuffer(kts::N, random_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(FetchTest, C11Atomics_48_Fetch_Local_Max) {
  using cl_type = typename TypeParam::cl_type;

  // This test makes use of control flow in the kernel. Control flow conversion
  // is not supported for atomics so we need to make sure this isn't registered
  // as a failure when the vectorizer fails. See CA-3294.
  this->fail_if_not_vectorized_ = false;

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");

  // Generate the random input.
  std::vector<cl_type> input_data(kts::N, cl_type{});
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(input_data);
  kts::Reference1D<cl_type> random_reference = [&input_data](size_t index) {
    return input_data[index];
  };

  // Set up references.
  kts::Reference1D<cl_type> max_reference = [&input_data](size_t index) {
    auto start = std::next(std::begin(input_data), index * kts::localN);
    auto end = std::next(start, kts::localN);
    return *std::max_element(start, end);
  };

  // Set up the buffers.
  // The input is a set of random values.
  this->AddInputBuffer(kts::N, random_reference);
  // The expected output for each work-group is the maximum of all elements in
  // that work-group.
  this->AddOutputBuffer(kts::N / kts::localN, max_reference);
  this->AddLocalBuffer(kts::N / kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

// The following tests check the entire domain {{0, 1} x {0, 1}} for the logical
// operations. That is, it checks that for the atomic fetch logic operations the
// following truth tables hold:
//
// | 0 1   ^ 0 1   & 0 1
// 0 0 1   0 0 1   0 0 0
// 1 1 1   1 1 0   1 0 1
TYPED_TEST_P(FetchTest, C11Atomics_49_Fetch_Global_Or_Truth_Table) {
  using cl_type = typename TypeParam::cl_type;

  const std::array<std::pair<cl_type, cl_type>, 4> domain = {
      std::make_pair(0, 0), std::make_pair(0, 1), std::make_pair(1, 0),
      std::make_pair(1, 1)};

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());
  for (const auto &input : domain) {
    // Set up references.
    kts::Reference1D<cl_type> initializer_reference = [&input](size_t) {
      return input.first;
    };
    kts::Reference1D<cl_type> input_reference = [&input](size_t) {
      return input.second;
    };
    kts::Reference1D<cl_type> output_reference = [&input](size_t) {
      return input.first | input.second;
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
}

TYPED_TEST_P(FetchTest, C11Atomics_50_Fetch_Global_Xor_Truth_Table) {
  using cl_type = typename TypeParam::cl_type;

  const std::array<std::pair<cl_type, cl_type>, 4> domain = {
      std::make_pair(0, 0), std::make_pair(0, 1), std::make_pair(1, 0),
      std::make_pair(1, 1)};

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());
  for (const auto &input : domain) {
    // Set up references.
    kts::Reference1D<cl_type> initializer_reference = [&input](size_t) {
      return input.first;
    };
    kts::Reference1D<cl_type> input_reference = [&input](size_t) {
      return input.second;
    };
    kts::Reference1D<cl_type> output_reference = [&input](size_t) {
      return input.first ^ input.second;
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
}

TYPED_TEST_P(FetchTest, C11Atomics_51_Fetch_Global_And_Truth_Table) {
  using cl_type = typename TypeParam::cl_type;

  const std::array<std::pair<cl_type, cl_type>, 4> domain = {
      std::make_pair(0, 0), std::make_pair(0, 1), std::make_pair(1, 0),
      std::make_pair(1, 1)};

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());
  for (const auto &input : domain) {
    // Set up references.
    kts::Reference1D<cl_type> initializer_reference = [&input](size_t) {
      return input.first;
    };
    kts::Reference1D<cl_type> input_reference = [&input](size_t) {
      return input.second;
    };
    kts::Reference1D<cl_type> output_reference = [&input](size_t) {
      return input.first & input.second;
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
}

TYPED_TEST_P(FetchTest, C11Atomics_52_Fetch_Local_Or_Truth_Table) {
  using cl_type = typename TypeParam::cl_type;

  // This test makes use of control flow in the kernel. Control flow conversion
  // is not supported for atomics so we need to make sure this isn't registered
  // as a failure when the vectorizer fails. See CA-3294.
  this->fail_if_not_vectorized_ = false;

  std::array<std::string, 3> keys = {"or", "xor", "and"};
  const std::array<std::pair<cl_type, cl_type>, 4> domain = {
      std::make_pair(0, 0), std::make_pair(0, 1), std::make_pair(1, 0),
      std::make_pair(1, 1)};

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());
  for (const auto &input : domain) {
    // Set up references.
    kts::Reference1D<cl_type> input_reference = [&input](size_t index) {
      return index == 0 ? input.first : input.second;
    };
    kts::Reference1D<cl_type> output_reference = [&input](size_t) {
      return input.first | input.second;
    };

    // Set up the buffers.
    // Input is the two elements for the binary operation.
    this->AddInputBuffer(2, input_reference);
    // Expected output is the result of the binary operation.
    this->AddOutputBuffer(1, output_reference);
    this->AddLocalBuffer(2);

    // Run the test.
    this->RunGeneric1D(2);
  }
}

TYPED_TEST_P(FetchTest, C11Atomics_53_Fetch_Local_Xor_Truth_Table) {
  using cl_type = typename TypeParam::cl_type;

  // This test makes use of control flow in the kernel. Control flow conversion
  // is not supported for atomics so we need to make sure this isn't registered
  // as a failure when the vectorizer fails. See CA-3294.
  this->fail_if_not_vectorized_ = false;

  std::array<std::string, 3> keys = {"or", "xor", "and"};
  const std::array<std::pair<cl_type, cl_type>, 4> domain = {
      std::make_pair(0, 0), std::make_pair(0, 1), std::make_pair(1, 0),
      std::make_pair(1, 1)};

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());
  for (const auto &input : domain) {
    // Set up references.
    kts::Reference1D<cl_type> input_reference = [&input](size_t index) {
      return index == 0 ? input.first : input.second;
    };
    kts::Reference1D<cl_type> output_reference = [&input](size_t) {
      return input.first ^ input.second;
    };

    // Set up the buffers.
    // Input is the two elements for the binary operation.
    this->AddInputBuffer(2, input_reference);
    // Expected output is the result of the binary operation.
    this->AddOutputBuffer(1, output_reference);
    this->AddLocalBuffer(2);

    // Run the test.
    this->RunGeneric1D(2);
  }
}

// See the Global variant of this test for an explanation of what it tests.
TYPED_TEST_P(FetchTest, C11Atomics_54_Fetch_Local_And_Truth_Table) {
  using cl_type = typename TypeParam::cl_type;

  // This test makes use of control flow in the kernel. Control flow conversion
  // is not supported for atomics so we need to make sure this isn't registered
  // as a failure when the vectorizer fails. See CA-3294.
  this->fail_if_not_vectorized_ = false;

  std::array<std::string, 3> keys = {"or", "xor", "and"};
  const std::array<std::pair<cl_type, cl_type>, 4> domain = {
      std::make_pair(0, 0), std::make_pair(0, 1), std::make_pair(1, 0),
      std::make_pair(1, 1)};

  // Patch up the types in the kernel source.
  const std::string type = TypeParam::source_name();
  const std::string atomic_type = "atomic_" + type;
  this->AddMacro("TYPE", type);
  this->AddMacro("ATOMIC_TYPE", atomic_type);
  this->AddBuildOption("-cl-std=CL3.0");
  ASSERT_TRUE(this->BuildProgram());
  for (const auto &input : domain) {
    // Set up references.
    kts::Reference1D<cl_type> input_reference = [&input](size_t index) {
      return index == 0 ? input.first : input.second;
    };
    kts::Reference1D<cl_type> output_reference = [&input](size_t) {
      return input.first & input.second;
    };

    // Set up the buffers.
    // Input is the two elements for the binary operation.
    this->AddInputBuffer(2, input_reference);
    // Expected output is the result of the binary operation.
    this->AddOutputBuffer(1, output_reference);
    this->AddLocalBuffer(2);

    // Run the test.
    this->RunGeneric1D(2);
  }
}

REGISTER_TYPED_TEST_SUITE_P(
    FetchTest, C11Atomics_21_Fetch_Global_Add_Check_Return,
    C11Atomics_22_Fetch_Global_Sub_Check_Return,
    C11Atomics_23_Fetch_Global_Or_Check_Return,
    C11Atomics_24_Fetch_Global_Xor_Check_Return,
    C11Atomics_25_Fetch_Global_And_Check_Return,
    C11Atomics_26_Fetch_Global_Min_Check_Return,
    C11Atomics_27_Fetch_Global_Max_Check_Return,
    C11Atomics_28_Fetch_Local_Add_Check_Return,
    C11Atomics_29_Fetch_Local_Sub_Check_Return,
    C11Atomics_30_Fetch_Local_Or_Check_Return,
    C11Atomics_31_Fetch_Local_Xor_Check_Return,
    C11Atomics_32_Fetch_Local_And_Check_Return,
    C11Atomics_33_Fetch_Local_Min_Check_Return,
    C11Atomics_34_Fetch_Local_Max_Check_Return, C11Atomics_35_Fetch_Global_Add,
    C11Atomics_36_Fetch_Local_Add, C11Atomics_37_Fetch_Global_Sub,
    C11Atomics_38_Fetch_Local_Sub, C11Atomics_39_Fetch_Global_Or,
    C11Atomics_40_Fetch_Local_Or, C11Atomics_41_Fetch_Global_Xor,
    C11Atomics_42_Fetch_Local_Xor, C11Atomics_43_Fetch_Global_And,
    C11Atomics_44_Fetch_Local_And, C11Atomics_45_Fetch_Global_Min,
    C11Atomics_46_Fetch_Local_Min, C11Atomics_47_Fetch_Global_Max,
    C11Atomics_48_Fetch_Local_Max, C11Atomics_49_Fetch_Global_Or_Truth_Table,
    C11Atomics_50_Fetch_Global_Xor_Truth_Table,
    C11Atomics_51_Fetch_Global_And_Truth_Table,
    C11Atomics_52_Fetch_Local_Or_Truth_Table,
    C11Atomics_53_Fetch_Local_Xor_Truth_Table,
    C11Atomics_54_Fetch_Local_And_Truth_Table);

INSTANTIATE_TYPED_TEST_SUITE_P(AtomicIntegerTypes, FetchTest,
                               AtomicIntegerTypes);

template <typename T>
class CompareExchangeTest : public BaseExecution {
 public:
  using cl_type = typename T::cl_type;
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(BaseExecution::SetUp());
    // The C11 atomic were introduced in 2.0 however here we only test
    // the minimum required subset for 3.0.
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }

    // Patch up the types in the kernel source.
    const std::string type = T::source_name();
    const std::string atomic_type = "atomic_" + type;
    this->AddMacro("TYPE", type);
    this->AddMacro("ATOMIC_TYPE", atomic_type);
    this->AddBuildOption("-cl-std=CL3.0");
  }
};

template <typename T>
class Strong : public CompareExchangeTest<T> {
 public:
  using cl_type = typename T::cl_type;
  Strong() {
    // Generate the random input.
    std::vector<cl_type> input_data(kts::N, cl_type{});
    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
    std::vector<cl_type> desired_data(kts::N, cl_type{});
    ucl::Environment::instance->GetInputGenerator().GenerateData(desired_data);

    // Set up references.
    random_reference = [input_data](size_t index) { return input_data[index]; };
    expected_in_reference = [input_data](size_t index) {
      auto expected_value = input_data[index];
      // This ensures every other expected value matches the value in
      // the input.
      return (index % 2) ? expected_value : !expected_value;
    };
    output_reference = [input_data, desired_data](size_t index) {
      return (index % 2) ? desired_data[index] : input_data[index];
    };
    desired_reference = [desired_data](size_t index) {
      return desired_data[index];
    };
    bool_reference = [](size_t index) { return index % 2; };
  }

  kts::Reference1D<cl_type> random_reference;
  kts::Reference1D<cl_type> expected_in_reference;
  kts::Reference1D<cl_type> output_reference;
  kts::Reference1D<cl_type> desired_reference;
  kts::Reference1D<cl_int> bool_reference;
};

TYPED_TEST_SUITE_P(CompareExchangeTest);
TYPED_TEST_SUITE_P(Strong);

TYPED_TEST_P(Strong, C11Atomics_55_Compare_Exchange_Strong_Global_Global) {
  // Set up the buffers.
  this->AddInOutBuffer(kts::N, this->random_reference, this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->random_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(Strong, C11Atomics_56_Compare_Exchange_Strong_Global_Local) {
  // Set up the buffers.
  this->AddInOutBuffer(kts::N, this->random_reference, this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->random_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_reference);
  this->AddLocalBuffer(kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(Strong, C11Atomics_57_Compare_Exchange_Strong_Global_Private) {
  // Set up the buffers.
  this->AddInOutBuffer(kts::N, this->random_reference, this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->random_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(Strong, C11Atomics_58_Compare_Exchange_Strong_Local_Global) {
  // Set up the buffers.
  this->AddInOutBuffer(kts::N, this->random_reference, this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->random_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_reference);
  this->AddLocalBuffer(kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(Strong, C11Atomics_59_Compare_Exchange_Strong_Local_Local) {
  // Set up the buffers.
  this->AddInOutBuffer(kts::N, this->random_reference, this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->random_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_reference);
  this->AddLocalBuffer(kts::localN);
  this->AddLocalBuffer(kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(Strong, C11Atomics_60_Compare_Exchange_Strong_Local_Private) {
  // Set up the buffers.
  this->AddInOutBuffer(kts::N, this->random_reference, this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->random_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_reference);
  this->AddLocalBuffer(kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

REGISTER_TYPED_TEST_SUITE_P(
    Strong, C11Atomics_55_Compare_Exchange_Strong_Global_Global,
    C11Atomics_56_Compare_Exchange_Strong_Global_Local,
    C11Atomics_57_Compare_Exchange_Strong_Global_Private,
    C11Atomics_58_Compare_Exchange_Strong_Local_Global,
    C11Atomics_59_Compare_Exchange_Strong_Local_Local,
    C11Atomics_60_Compare_Exchange_Strong_Local_Private);
INSTANTIATE_TYPED_TEST_SUITE_P(AtomicIntegerTypes, Strong, AtomicIntegerTypes);

template <typename T>
class StrongGlobalSingle : public CompareExchangeTest<T> {
 public:
  using cl_type = typename T::cl_type;
  StrongGlobalSingle() {
    // Set up references.
    size_t success_index =
        ucl::Environment::instance->GetInputGenerator().GenerateInt<cl_type>(
            0, kts::N - 1);

    // We need the expected values to be unique, otherwise we won't be able to
    // determine which thread updates the atomic.
    // We also require the intersection of the expected and desired values to be
    // empty otherwise subsequent threads could update the atomic. The fastest
    // way to do this is to generate a buffer of unqique values of size 2 *
    // kts::N, then just split it evenly between the two.
    std::vector<cl_type> expected_values(kts::N, cl_type{});
    std::vector<cl_type> desired_values(kts::N, cl_type{});
    std::vector<cl_type> all_values(
        expected_values.size() + desired_values.size(), cl_type{});

    ucl::Environment::instance->GetInputGenerator()
        .GenerateUniqueIntData<cl_type>(all_values);
    expected_values.assign(std::begin(all_values),
                           std::next(std::begin(all_values), kts::N));
    desired_values.assign(std::next(std::begin(all_values), kts::N),
                          std::end(all_values));

    initializer_reference = [success_index, expected_values](size_t) {
      return expected_values[success_index];
    };
    output_reference = [success_index, desired_values](size_t) {
      return desired_values[success_index];
    };
    expected_in_reference = [expected_values](size_t index) {
      return expected_values[index];
    };
    expected_output_reference = [success_index, expected_values,
                                 desired_values](size_t index, cl_type value) {
      if (index == success_index) {
        return value == expected_values[index];
      }

      return value == expected_values[success_index] ||
             value == desired_values[success_index];
    };
    desired_reference = [desired_values](size_t index) {
      return desired_values[index];
    };
    bool_output_reference = [success_index](size_t index) {
      return index == success_index;
    };
  }

  kts::Reference1D<cl_type> initializer_reference;
  kts::Reference1D<cl_type> output_reference;
  kts::Reference1D<cl_type> expected_in_reference;
  kts::Reference1D<cl_type> expected_output_reference;
  kts::Reference1D<cl_type> desired_reference;
  kts::Reference1D<cl_int> bool_output_reference;
};
TYPED_TEST_SUITE_P(StrongGlobalSingle);

TYPED_TEST_P(StrongGlobalSingle,
             C11Atomics_61_Compare_Exchange_Strong_Global_Global_Single) {
  // Set up the buffers.
  this->AddInOutBuffer(1, this->initializer_reference, this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->expected_output_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_output_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(StrongGlobalSingle,
             C11Atomics_62_Compare_Exchange_Strong_Global_Local_Single) {
  // Set up the buffers.
  this->AddInOutBuffer(1, this->initializer_reference, this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->expected_output_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_output_reference);
  this->AddLocalBuffer(kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(StrongGlobalSingle,
             C11Atomics_63_Compare_Exchange_Strong_Global_Private_Single) {
  // Set up the buffers.
  this->AddInOutBuffer(1, this->initializer_reference, this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->expected_output_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_output_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

REGISTER_TYPED_TEST_SUITE_P(
    StrongGlobalSingle,
    C11Atomics_61_Compare_Exchange_Strong_Global_Global_Single,
    C11Atomics_62_Compare_Exchange_Strong_Global_Local_Single,
    C11Atomics_63_Compare_Exchange_Strong_Global_Private_Single);

INSTANTIATE_TYPED_TEST_SUITE_P(AtomicIntegerTypes, StrongGlobalSingle,
                               AtomicIntegerTypes);

template <typename T>
class StrongLocalSingle : public CompareExchangeTest<T> {
 public:
  using cl_type = typename T::cl_type;
  StrongLocalSingle() {
    // Set up references.
    const size_t work_group_count = kts::N / kts::localN;

    // Pick a random index in each work-group to hold the correct
    // expected value.
    std::vector<size_t> success_indicies(work_group_count, 0);
    ucl::Environment::instance->GetInputGenerator().GenerateIntData<size_t>(
        success_indicies, 0, kts::localN - 1);
    // Calculate the global id of these indicies
    for (unsigned i = 0; i < success_indicies.size(); ++i) {
      success_indicies[i] += i * kts::localN;
    }

    // We still need N expected values, there will be exactly one correct value
    // in each work-group.
    // We also need N desired values where each work-group has an empty
    // intersection with its expected values. The easiest way to do this is to
    // just generate 2 * kts::N uniqueue values and divide them between the two
    // buffers.
    std::vector<cl_type> expected_values(kts::N, cl_type{});
    std::vector<cl_type> desired_values(kts::N, cl_type{});
    std::vector<cl_type> all_values(
        expected_values.size() + desired_values.size(), cl_type{});

    ucl::Environment::instance->GetInputGenerator().GenerateUniqueIntData(
        all_values);
    expected_values.assign(std::begin(all_values),
                           std::next(std::begin(all_values), kts::N));
    desired_values.assign(std::next(std::begin(all_values), kts::N),
                          std::end(all_values));

    initializer_reference = [expected_values, success_indicies](size_t index) {
      return expected_values[success_indicies[index]];
    };

    output_reference = [desired_values, success_indicies](size_t index) {
      return desired_values[success_indicies[index]];
    };

    expected_in_reference = [expected_values](size_t index) {
      return expected_values[index];
    };

    expected_output_reference = [success_indicies, expected_values,
                                 desired_values](size_t index, cl_type value) {
      // Expected output will contain its original value if at a success index
      // otherwise it will contain the value stored in the atomic which will
      // be either the initial value if the sucessful thread hasn't executed
      // the exchange yet, or the desired value of the success index if it
      // has.
      const size_t work_group = index / kts::localN;
      const size_t success_index_of_workgroup = success_indicies[work_group];
      if (index == success_index_of_workgroup) {
        return value == expected_values[index];
      }

      return value == expected_values[success_index_of_workgroup] ||
             value == desired_values[success_index_of_workgroup];
    };
    desired_reference = [desired_values](size_t index) {
      return desired_values[index];
    };
    bool_output_reference = [success_indicies](size_t index) {
      const size_t work_group = index / kts::localN;
      const size_t success_index_of_workgroup = success_indicies[work_group];
      return index == success_index_of_workgroup;
    };
  }

  kts::Reference1D<cl_type> initializer_reference;
  kts::Reference1D<cl_type> output_reference;
  kts::Reference1D<cl_type> expected_in_reference;
  kts::Reference1D<cl_type> expected_output_reference;
  kts::Reference1D<cl_type> desired_reference;
  kts::Reference1D<cl_int> bool_output_reference;
};

TYPED_TEST_SUITE_P(StrongLocalSingle);

TYPED_TEST_P(StrongLocalSingle,
             C11Atomics_64_Compare_Exchange_Strong_Local_Global_Single) {
  // Set up the buffers.
  this->AddInOutBuffer(kts::N / kts::localN, this->initializer_reference,
                       this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->expected_output_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_output_reference);
  this->AddLocalBuffer(1);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(StrongLocalSingle,
             C11Atomics_65_Compare_Exchange_Strong_Local_Local_Single) {
  // Set up the buffers.
  this->AddInOutBuffer(kts::N / kts::localN, this->initializer_reference,
                       this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->expected_output_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_output_reference);
  this->AddLocalBuffer(1);
  this->AddLocalBuffer(kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(StrongLocalSingle,
             C11Atomics_66_Compare_Exchange_Strong_Local_Private_Single) {
  // Set up the buffers.
  this->AddInOutBuffer(kts::N / kts::localN, this->initializer_reference,
                       this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->expected_output_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_output_reference);
  this->AddLocalBuffer(1);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

REGISTER_TYPED_TEST_SUITE_P(
    StrongLocalSingle,
    C11Atomics_64_Compare_Exchange_Strong_Local_Global_Single,
    C11Atomics_65_Compare_Exchange_Strong_Local_Local_Single,
    C11Atomics_66_Compare_Exchange_Strong_Local_Private_Single);

INSTANTIATE_TYPED_TEST_SUITE_P(AtomicIntegerTypes, StrongLocalSingle,
                               AtomicIntegerTypes);

template <typename T>
class Weak : public CompareExchangeTest<T> {
 public:
  using cl_type = typename T::cl_type;
  Weak() : failed_comparison_indicies{} {
    // Generate the random input.
    std::vector<cl_type> input_data(kts::N, cl_type{});
    ucl::Environment::instance->GetInputGenerator().GenerateData(input_data);
    std::vector<cl_type> desired_data(kts::N, cl_type{});
    ucl::Environment::instance->GetInputGenerator().GenerateData(desired_data);

    // Set up references.
    random_reference = [input_data](size_t index) { return input_data[index]; };
    expected_in_reference = [input_data](size_t index) {
      auto expected_value = input_data[index];
      // This ensures every other expected value matches the value in the
      // input.
      return (index % 2) ? expected_value : !expected_value;
    };
    output_reference = [input_data, desired_data, this](size_t index,
                                                        cl_type value) {
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
        failed_comparison_indicies.push_back(index);
      }
      // Failure index and we need to check the memory wasn't updated.
      return value == input_data[index];
    };
    desired_reference = [desired_data](size_t index) {
      return desired_data[index];
    };
    bool_reference = [this](size_t index, cl_int value) {
      // Check if we are at success index that didn't fail.
      if ((index % 2) &&
          std::find(std::begin(failed_comparison_indicies),
                    std::end(failed_comparison_indicies),
                    index) == std::end(failed_comparison_indicies)) {
        return value == true;
      }

      // Otherwise we are either at failure index, or a success index that
      // failed.
      return value == false;
    };
  }

  std::vector<size_t> failed_comparison_indicies;
  kts::Reference1D<cl_type> random_reference;
  kts::Reference1D<cl_type> expected_in_reference;
  kts::Reference1D<cl_type> output_reference;
  kts::Reference1D<cl_type> desired_reference;
  kts::Reference1D<cl_int> bool_reference;
};

TYPED_TEST_SUITE_P(Weak);

TYPED_TEST_P(Weak, C11Atomics_67_Compare_Exchange_Weak_Global_Global) {
  // Set up the buffers.
  this->AddInOutBuffer(kts::N, this->random_reference, this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->random_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(Weak, C11Atomics_68_Compare_Exchange_Weak_Global_Local) {
  // Set up the buffers.
  this->AddInOutBuffer(kts::N, this->random_reference, this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->random_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_reference);
  this->AddLocalBuffer(kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(Weak, C11Atomics_69_Compare_Exchange_Weak_Global_Private) {
  // Set up the buffers.
  this->AddInOutBuffer(kts::N, this->random_reference, this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->random_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(Weak, C11Atomics_70_Compare_Exchange_Weak_Local_Global) {
  // Set up the buffers.
  this->AddInOutBuffer(kts::N, this->random_reference, this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->random_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_reference);
  this->AddLocalBuffer(kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(Weak, C11Atomics_71_Compare_Exchange_Weak_Local_Local) {
  // Set up the buffers.
  this->AddInOutBuffer(kts::N, this->random_reference, this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->random_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_reference);
  this->AddLocalBuffer(kts::localN);
  this->AddLocalBuffer(kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(Weak, C11Atomics_72_Compare_Exchange_Weak_Local_Private) {
  // Set up the buffers.
  this->AddInOutBuffer(kts::N, this->random_reference, this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->random_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_reference);
  this->AddLocalBuffer(kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

REGISTER_TYPED_TEST_SUITE_P(Weak,
                            C11Atomics_67_Compare_Exchange_Weak_Global_Global,
                            C11Atomics_68_Compare_Exchange_Weak_Global_Local,
                            C11Atomics_69_Compare_Exchange_Weak_Global_Private,
                            C11Atomics_70_Compare_Exchange_Weak_Local_Global,
                            C11Atomics_71_Compare_Exchange_Weak_Local_Local,
                            C11Atomics_72_Compare_Exchange_Weak_Local_Private);

INSTANTIATE_TYPED_TEST_SUITE_P(AtomicIntegerTypes, Weak, AtomicIntegerTypes);

template <typename T>
class WeakGlobalSingle : public CompareExchangeTest<T> {
 public:
  using cl_type = typename T::cl_type;
  WeakGlobalSingle() : weak_exchange_failed(false) {
    // Set up references.
    size_t success_index =
        ucl::Environment::instance->GetInputGenerator().GenerateInt<cl_type>(
            0, kts::N - 1);

    // We need the expected values to be unique, otherwise we won't be
    // able to determine which thread updates the atomic. We also require
    // the intersection of the expected and desired values to be empty
    // otherwise subsequent threads could update the atomic. The fastest
    // way to do this is to generate a buffer of unqique values of size 2
    // * kts::N, then just split it evenly between the two.
    std::vector<cl_type> expected_values(kts::N, cl_type{});
    std::vector<cl_type> desired_values(kts::N, cl_type{});
    std::vector<cl_type> all_values(
        expected_values.size() + desired_values.size(), cl_type{});

    ucl::Environment::instance->GetInputGenerator()
        .GenerateUniqueIntData<cl_type>(all_values);
    expected_values.assign(std::begin(all_values),
                           std::next(std::begin(all_values), kts::N));
    desired_values.assign(std::next(std::begin(all_values), kts::N),
                          std::end(all_values));

    initializer_reference = [success_index, expected_values](size_t) {
      return expected_values[success_index];
    };
    output_reference = [success_index, desired_values, this, expected_values](
                           size_t, cl_type value) {
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
    expected_in_reference = [expected_values](size_t index) {
      return expected_values[index];
    };
    expected_output_reference = [success_index, expected_values,
                                 desired_values](size_t, cl_type value) {
      return value == expected_values[success_index] ||
             value == desired_values[success_index];
    };
    desired_reference = [desired_values](size_t index) {
      return desired_values[index];
    };
    bool_output_reference = [success_index, this](size_t index) {
      return index == success_index && !weak_exchange_failed;
    };
  }

  bool weak_exchange_failed;
  kts::Reference1D<cl_type> initializer_reference;
  kts::Reference1D<cl_type> output_reference;
  kts::Reference1D<cl_type> expected_in_reference;
  kts::Reference1D<cl_type> expected_output_reference;
  kts::Reference1D<cl_type> desired_reference;
  kts::Reference1D<cl_int> bool_output_reference;
};

TYPED_TEST_SUITE_P(WeakGlobalSingle);

TYPED_TEST_P(WeakGlobalSingle,
             C11Atomics_73_Compare_Exchange_Weak_Global_Global_Single) {
  // Set up the buffers.
  this->AddInOutBuffer(1, this->initializer_reference, this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->expected_output_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_output_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

TYPED_TEST_P(WeakGlobalSingle,
             C11Atomics_74_Compare_Exchange_Weak_Global_Local_Single) {
  // Set up the buffers.
  this->AddInOutBuffer(1, this->initializer_reference, this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->expected_output_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_output_reference);
  this->AddLocalBuffer(kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(WeakGlobalSingle,
             C11Atomics_75_Compare_Exchange_Weak_Global_Private_Single) {
  // Set up the buffers.
  this->AddInOutBuffer(1, this->initializer_reference, this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->expected_output_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_output_reference);

  // Run the test.
  this->RunGeneric1D(kts::N);
}

REGISTER_TYPED_TEST_SUITE_P(
    WeakGlobalSingle, C11Atomics_73_Compare_Exchange_Weak_Global_Global_Single,
    C11Atomics_74_Compare_Exchange_Weak_Global_Local_Single,
    C11Atomics_75_Compare_Exchange_Weak_Global_Private_Single);

INSTANTIATE_TYPED_TEST_SUITE_P(AtomicIntegerTypes, WeakGlobalSingle,
                               AtomicIntegerTypes);

template <typename T>
class WeakLocalSingle : public CompareExchangeTest<T> {
  using cl_type = typename T::cl_type;

 public:
  WeakLocalSingle()
      : success_indicies(kts::N / kts::localN, 0),
        weak_exchanges_failed(kts::N / kts::localN, false) {
    // Set up references.
    // Pick a random index in each work-group to hold the correct
    // expected value.
    ucl::Environment::instance->GetInputGenerator().GenerateIntData<size_t>(
        success_indicies, 0, kts::localN - 1);
    // Calculate the global id of these indicies
    for (unsigned i = 0; i < success_indicies.size(); ++i) {
      success_indicies[i] += i * kts::localN;
    }

    // We still need N expected values, there will be exactly one correct
    // value in each work-group. We also need N desired values where each
    // work-group has an empty intersection with its expected values. The
    // easiest way to do this is to just generate 2 * kts::N uniqueue values
    // and divide them between the two buffers.
    std::vector<cl_type> expected_values(kts::N, cl_type{});
    std::vector<cl_type> desired_values(kts::N, cl_type{});
    std::vector<cl_type> all_values(
        expected_values.size() + desired_values.size(), cl_type{});

    ucl::Environment::instance->GetInputGenerator().GenerateUniqueIntData(
        all_values);
    expected_values.assign(std::begin(all_values),
                           std::next(std::begin(all_values), kts::N));
    desired_values.assign(std::next(std::begin(all_values), kts::N),
                          std::end(all_values));

    initializer_reference = [expected_values, this](size_t index) {
      return expected_values[success_indicies[index]];
    };

    output_reference = [desired_values, this, expected_values](size_t index,
                                                               cl_type value) {
      // Weak compare-exchange operations may fail spuriously, returning 0
      // when the contents of memory in expected and the atomic are equal,
      // it may return zero and store back to expected the same memory
      // contents that were originally there.
      if (value == expected_values[success_indicies[index]]) {
        weak_exchanges_failed[index / kts::localN] = true;
        return true;
      }

      return value == desired_values[success_indicies[index]];
    };

    expected_in_reference = [expected_values](size_t index) {
      return expected_values[index];
    };

    expected_output_reference = [this, expected_values, desired_values](
                                    size_t index, cl_type value) {
      // Expected output will contain its original value if at a success
      // index otherwise it will contain the value stored in the atomic
      // which will be either the initial value if the sucessful thread
      // hasn't executed the exchange yet, or the desired value of the
      // success index if it has.
      const size_t work_group = index / kts::localN;
      const size_t success_index_of_workgroup = success_indicies[work_group];
      return value == expected_values[success_index_of_workgroup] ||
             value == desired_values[success_index_of_workgroup];
    };
    desired_reference = [desired_values](size_t index) {
      return desired_values[index];
    };
    bool_output_reference = [this](size_t index) {
      const size_t work_group = index / kts::localN;
      const size_t success_index_of_workgroup = success_indicies[work_group];
      return (index == success_index_of_workgroup) &&
             !weak_exchanges_failed[work_group];
    };
  }

  std::vector<size_t> success_indicies;
  std::vector<bool> weak_exchanges_failed;
  kts::Reference1D<cl_type> initializer_reference;
  kts::Reference1D<cl_type> output_reference;
  kts::Reference1D<cl_type> expected_in_reference;
  kts::Reference1D<cl_type> expected_output_reference;
  kts::Reference1D<cl_type> desired_reference;
  kts::Reference1D<cl_int> bool_output_reference;
};

TYPED_TEST_SUITE_P(WeakLocalSingle);

TYPED_TEST_P(WeakLocalSingle,
             C11Atomics_76_Compare_Exchange_Weak_Local_Global_Single) {
  // Set up the buffers.
  this->AddInOutBuffer(kts::N / kts::localN, this->initializer_reference,
                       this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->expected_output_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_output_reference);
  this->AddLocalBuffer(1);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(WeakLocalSingle,
             C11Atomics_77_Compare_Exchange_Weak_Local_Local_Single) {
  // Set up the buffers.
  this->AddInOutBuffer(kts::N / kts::localN, this->initializer_reference,
                       this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->expected_output_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_output_reference);
  this->AddLocalBuffer(1);
  this->AddLocalBuffer(kts::localN);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

TYPED_TEST_P(WeakLocalSingle,
             C11Atomics_78_Compare_Exchange_Weak_Local_Private_Single) {
  // Set up the buffers.
  this->AddInOutBuffer(kts::N / kts::localN, this->initializer_reference,
                       this->output_reference);
  this->AddInOutBuffer(kts::N, this->expected_in_reference,
                       this->expected_output_reference);
  this->AddInputBuffer(kts::N, this->desired_reference);
  this->AddOutputBuffer(kts::N, this->bool_output_reference);
  this->AddLocalBuffer(1);

  // Run the test.
  this->RunGeneric1D(kts::N, kts::localN);
}

REGISTER_TYPED_TEST_SUITE_P(
    WeakLocalSingle, C11Atomics_76_Compare_Exchange_Weak_Local_Global_Single,
    C11Atomics_77_Compare_Exchange_Weak_Local_Local_Single,
    C11Atomics_78_Compare_Exchange_Weak_Local_Private_Single);

INSTANTIATE_TYPED_TEST_SUITE_P(AtomicIntegerTypes, WeakLocalSingle,
                               AtomicIntegerTypes);
