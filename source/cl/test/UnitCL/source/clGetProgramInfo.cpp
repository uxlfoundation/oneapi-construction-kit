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

class clGetProgramInfoTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    cl_int errorcode;
    const char *source =
        "void kernel foo(global int * a, global int * b) {*a = *b;}";
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  }

  void TearDown() override {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
};

class clGetProgramInfoProgramSourceTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    cl_int errorcode;
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
    sourceSize = strlen(source) + 1;
  }

  void TearDown() override {
    if (program) {
      ASSERT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
  size_t sourceSize = 0;
  const char *source =
      "void kernel foo(global int * a, global int * b) {*a = *b;}";
};

TEST_F(clGetProgramInfoTest, BadProgram) {
  ASSERT_EQ_ERRCODE(CL_INVALID_PROGRAM,
                    clGetProgramInfo(nullptr, 0, 0, nullptr, nullptr));
}

TEST_F(clGetProgramInfoTest, BadReturnPointers) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetProgramInfo(program, 0, 0, nullptr, nullptr));
}

TEST_F(clGetProgramInfoTest, RefCountDefault) {
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_REFERENCE_COUNT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint ref_count;
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_REFERENCE_COUNT, size,
                                  &ref_count, nullptr));
  ASSERT_EQ(1u, ref_count);
}

TEST_F(clGetProgramInfoTest, RefCountBadParamValue) {
  cl_uint ref_count;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetProgramInfo(program, CL_PROGRAM_REFERENCE_COUNT, 0,
                                     &ref_count, nullptr));
}

TEST_F(clGetProgramInfoTest, ProgramContextDefault) {
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_CONTEXT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_context), size);
  cl_context thisContext = nullptr;
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_CONTEXT, size,
                                  static_cast<void *>(&thisContext), nullptr));
  ASSERT_EQ(context, thisContext);
}

TEST_F(clGetProgramInfoTest, ProgramContextBadParamValue) {
  cl_context thisContext = nullptr;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetProgramInfo(program, CL_PROGRAM_CONTEXT, 0,
                       static_cast<void *>(&thisContext), nullptr));
}

TEST_F(clGetProgramInfoTest, ProgramNumDevicesDefault) {
  size_t size = 0;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint numDevices = 0;
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES, size,
                                  &numDevices, nullptr));
  ASSERT_EQ(1, numDevices);
}

TEST_F(clGetProgramInfoTest, ProgramNumDevicesBadParamValue) {
  cl_uint num_devices;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES, 0,
                                     &num_devices, nullptr));
}

TEST_F(clGetProgramInfoTest, ProgramDevicesDefault) {
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_DEVICES, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_device_id), size);
  UCL::Buffer<cl_device_id> devices(size / sizeof(cl_device_id));
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_DEVICES, size,
                                  static_cast<void *>(devices), nullptr));
  for (unsigned int i = 0; i < devices.size(); i++) {
    ASSERT_TRUE(devices[0]);
  }
}

TEST_F(clGetProgramInfoTest, ProgramDevicesBadParamValue) {
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_DEVICES, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_device_id), size);
  UCL::Buffer<cl_device_id> devices(size / sizeof(cl_device_id));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetProgramInfo(program, CL_PROGRAM_DEVICES, 0,
                                     static_cast<void *>(devices), nullptr));
}

TEST_F(clGetProgramInfoProgramSourceTest, ProgramSourceDefault) {
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_SOURCE, 0, nullptr, &size));
  UCL::Buffer<char> source(size);
  EXPECT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_SOURCE, size, source, nullptr));
  ASSERT_TRUE(size ? size == strlen(source) + 1 : true);
}

TEST_F(clGetProgramInfoProgramSourceTest, ProgramSourceBadParamValue) {
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_SOURCE, 0, nullptr, &size));
  UCL::Buffer<char> source(size);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetProgramInfo(program, CL_PROGRAM_SOURCE, 0, source, nullptr));
}

TEST_F(clGetProgramInfoTest, ProgramBinarySizesDefault) {
  // Redmine #5121: CL_PROGRAM_BINARY_SIZES test need updated when multiple
  // devices are
  // supported!
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, 0, nullptr, &size));
  UCL::Buffer<char> binarySizes(size);
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, size,
                                  binarySizes, nullptr));
}

TEST_F(clGetProgramInfoTest, ProgramBinarySizesBadParamValue) {
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, 0, nullptr, &size));
  UCL::Buffer<char> binarySizes(size);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, 0,
                                     binarySizes, nullptr));
}

TEST_F(clGetProgramInfoTest, ProgramBinarySizesNotLinkedDefault) {
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, 0, nullptr, &size));
  UCL::Buffer<size_t> binarySizes(size / sizeof(size_t));
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, size,
                                  binarySizes, nullptr));
}

TEST_F(clGetProgramInfoTest, ProgramBinarySizesParamValueSizeRet) {
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
}

TEST_F(clGetProgramInfoTest, ProgramBinariesDefault) {
  // Redmine #5121: CL_PROGRAM_BINARIES test need updated when multiple devices
  // are supported!
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, 0, nullptr, &size));
  UCL::Buffer<size_t> binarySizes(size / sizeof(size_t));
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, size,
                                  binarySizes, nullptr));

  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_BINARIES, 0, nullptr, &size));

  UCL::Buffer<unsigned char *> binaries(size / sizeof(unsigned char *));

  for (size_t i = 0; i < binaries.size(); ++i) {
    binaries[i] = new unsigned char[binarySizes[i]];
  }

  EXPECT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_BINARIES, size,
                                  static_cast<void *>(binaries), nullptr));

  for (size_t i = 0; i < binaries.size(); ++i) {
    delete[] binaries[i];
  }
}

TEST_F(clGetProgramInfoTest, ProgramBinaraiesBadParamValue) {
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_BINARIES, 0, nullptr, &size));
  UCL::Buffer<char> binaries(size);
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetProgramInfo(program, CL_PROGRAM_BINARIES, 0,
                                     static_cast<void *>(binaries), nullptr));
}

TEST_F(clGetProgramInfoProgramSourceTest, ProgramBinariesDefault) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection erroneously succeeds.
  }
  // Redmine #5121: CL_PROGRAM_BINARIES test need updated when multiple devices
  // are supported!
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, 0, nullptr, &size));
  UCL::Buffer<size_t> binarySizes(size / sizeof(size_t));
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, size,
                                  binarySizes, nullptr));

  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_BINARIES, 0, nullptr, &size));

  UCL::Buffer<unsigned char *> binaries(size / sizeof(unsigned char *));

  for (size_t i = 0; i < binaries.size(); ++i) {
    ASSERT_EQ(0u, binarySizes[i]);
    binaries[i] = nullptr;
  }

  EXPECT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_BINARIES, size,
                                  static_cast<void *>(binaries), nullptr));
}

TEST_F(clGetProgramInfoTest, ProgramBinariesParamValueSizeRet) {
  size_t size = 0;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_BINARIES, 0, nullptr, &size));
  // Redmine #5140: add binaries size check
  ASSERT_EQ(sizeof(char *), size);
}

TEST_F(clGetProgramInfoTest, ProgramNumKernelsDefault) {
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_NUM_KERNELS, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t num_kernels = 0;
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_NUM_KERNELS, size,
                                  &num_kernels, nullptr));
  ASSERT_EQ(1u, num_kernels);
}

TEST_F(clGetProgramInfoTest, ProgramNumKernelsBadParamValue) {
  cl_uint num_kernels;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetProgramInfo(program, CL_PROGRAM_NUM_KERNELS, 0,
                                     &num_kernels, nullptr));
}

TEST_F(clGetProgramInfoTest, ProgramReferenceCount) {
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_REFERENCE_COUNT, 0, nullptr, &size));
  ASSERT_EQ(4u, size);
  cl_uint ref;
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_REFERENCE_COUNT, size,
                                  &ref, nullptr));
  ASSERT_EQ(1u, ref);
}

TEST_F(clGetProgramInfoTest, ProgramKernelNamesDefault) {
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_KERNEL_NAMES, 0, nullptr, &size));
  ASSERT_EQ(strlen("foo") + 1, size);
  UCL::Buffer<char> kernelNames(size);
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_KERNEL_NAMES, size,
                                  kernelNames, nullptr));
  ASSERT_TRUE(size ? size == strlen(kernelNames) + 1 : true);
}

TEST_F(clGetProgramInfoTest, ProgramKernelNamesBadParamValue) {
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_KERNEL_NAMES, 0, nullptr, &size));
  UCL::Buffer<char> kernelNames(size);
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetProgramInfo(program, CL_PROGRAM_KERNEL_NAMES, 0,
                                     kernelNames, nullptr));
}

class clGetProgramInfoCompiledProgram : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (UCL::isInterceptLayerPresent()) {
      GTEST_SKIP();  // Injection creates programs from binaries, can't compile.
    }
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    cl_int errorcode;
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
    sourceSize = strlen(source) + 1;
    ASSERT_SUCCESS(clCompileProgram(program, 0, nullptr, nullptr, 0, nullptr,
                                    nullptr, nullptr, nullptr));
  }

  void TearDown() override {
    if (program) {
      ASSERT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  void BuildProgram() {
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  }

  cl_program program = nullptr;
  size_t sourceSize = 0;
  const char *source =
      "void kernel foo(global int * a, global int * b) {*a = *b;}";
};

TEST_F(clGetProgramInfoCompiledProgram, ProgramNumKernelsProgramDefault) {
  this->BuildProgram();
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_NUM_KERNELS, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t num_kernels = 0;
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_NUM_KERNELS, size,
                                  &num_kernels, nullptr));
  ASSERT_EQ(1u, num_kernels);
}

TEST_F(clGetProgramInfoCompiledProgram, ProgramKernelNamesProgramDefault) {
  this->BuildProgram();
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_KERNEL_NAMES, 0, nullptr, &size));
  ASSERT_EQ(strlen("foo") + 1, size);
  UCL::Buffer<char> kernelNames(size);
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_KERNEL_NAMES, size,
                                  kernelNames, nullptr));
}

// CL_INVALID_PROGRAM_EXECUTABLE if param_name is CL_PROGRAM_NUM_KERNELS or
// CL_PROGRAM_KERNEL_NAMES and a successful program executable has not been
// built for at least one device in the list of devices associated with program.
TEST_F(clGetProgramInfoCompiledProgram,
       ProgramNumKernelsProgramWithNoExecutableType) {
  size_t size;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_PROGRAM_EXECUTABLE,
      clGetProgramInfo(program, CL_PROGRAM_NUM_KERNELS, 0, nullptr, &size));
}

TEST_F(clGetProgramInfoCompiledProgram,
       ProgramKernelNamesProgramWithNoExecutableType) {
  size_t size;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_PROGRAM_EXECUTABLE,
      clGetProgramInfo(program, CL_PROGRAM_KERNEL_NAMES, 0, nullptr, &size));
}

struct clGetProgramInfoInvalidProgramTest : ucl::ContextTest {
  cl_program program = nullptr;

  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());

    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }

    const char *source = R"OpenCL(
void bar(int a, int b);
void kernel foo(global int * a, global int * b) {
  bar(a, b);
};
    )OpenCL";
    cl_int error;
    program = clCreateProgramWithSource(context, 1, &source, nullptr, &error);
    ASSERT_SUCCESS(error);
    ASSERT_EQ_ERRCODE(
        CL_BUILD_PROGRAM_FAILURE,
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));
  }

  void TearDown() {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }
};

TEST_F(clGetProgramInfoInvalidProgramTest, ProgramInfo) {
  size_t num_devices = 0;
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES,
                                  sizeof(num_devices), &num_devices, nullptr));

  std::vector<size_t> binary_sizes(num_devices);
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES,
                                  num_devices * sizeof(size_t),
                                  binary_sizes.data(), nullptr));

  std::vector<std::vector<unsigned char>> storage(num_devices);
  std::vector<unsigned char *> binaries(num_devices);
  for (cl_uint i = 0; i < num_devices; i++) {
    storage.resize(binary_sizes[i]);
    binaries[i] = storage[i].data();
  }

  ASSERT_EQ_ERRCODE(
      CL_INVALID_PROGRAM,
      clGetProgramInfo(program, CL_PROGRAM_BINARIES,
                       num_devices * sizeof(unsigned char *),
                       static_cast<void *>(binaries.data()), nullptr));
}

struct clGetProgramInfoBuiltinTest : ucl::ContextTest {
  cl_program program = nullptr;

  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());

    size_t size;
    ASSERT_SUCCESS(
        clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, 0, nullptr, &size));
    if (size == 0) {  // Skip if no builtins.
      GTEST_SKIP();
    }

    std::vector<char> kernels(size);
    ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_BUILT_IN_KERNELS, size,
                                   kernels.data(), nullptr));

    auto sep = std::find(std::begin(kernels), std::end(kernels), ';');
    if (sep != std::end(kernels)) {
      kernels.resize(sep - std::begin(kernels));
      kernels.push_back('\0');
    } else {
      GTEST_SKIP();
    }

    cl_int error;
    program = clCreateProgramWithBuiltInKernels(context, 1, &device,
                                                kernels.data(), &error);
    ASSERT_SUCCESS(error);
  }

  void TearDown() {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }
};

TEST_F(clGetProgramInfoBuiltinTest, NumKernels) {
  size_t size = 0;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_NUM_KERNELS, 0, nullptr, &size));
  size_t num_kernels = 0;
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_NUM_KERNELS, size,
                                  &num_kernels, nullptr));
  ASSERT_EQ(num_kernels, 1);
}

TEST_F(clGetProgramInfoBuiltinTest, Binary) {
  // CL_PROGRAM_BINARY_SIZES: The size of the array is the number of devices
  // associated with program.
  cl_uint num_devices;
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES,
                                  sizeof(num_devices), &num_devices, nullptr));
  ASSERT_GE(1u, num_devices);
  std::vector<size_t> binary_sizes(num_devices);

  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, 0, nullptr, &size));
  ASSERT_EQ(num_devices * sizeof(size_t), size);

  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, size,
                                  binary_sizes.data(), nullptr));

  std::vector<std::vector<unsigned char>> storage(num_devices);
  std::vector<unsigned char *> binaries(num_devices);
  for (cl_uint i = 0; i < num_devices; i++) {
    // CL_PROGRAM_BINARY_SIZES: If program is created using
    // clCreateProgramWithBuiltInKernels, the implementation may return zero in
    // any entries of the returned array.
    storage[i].resize(std::max(binary_sizes[i], size_t{1}));
    binaries[i] = storage[i].data();
  }

  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_BINARIES, 0, nullptr, &size));
  ASSERT_EQ(num_devices * sizeof(unsigned char *), size);

  // There is no error code for querying CL_PROGRAM_BINARIES on programs from
  // clCreateProgramWithBuiltInKernels, so while no binaries exist this is
  // still expected to succeed.
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_BINARIES, size,
                                  static_cast<void *>(binaries.data()),
                                  nullptr));
}

TEST_F(clGetProgramInfoBuiltinTest, Source) {
  size_t size;
  ASSERT_SUCCESS(
      clGetProgramInfo(program, CL_PROGRAM_SOURCE, 0, nullptr, &size));
  // CL_PROGRAM_SOURCE: If program is created using
  // clCreateProgramWithBuiltInKernels, a null string or the appropriate
  // program source code.
  ASSERT_EQ(1, size);

  std::vector<char> source(size);
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_SOURCE, size,
                                  source.data(), nullptr));
  ASSERT_EQ('\0', source[0]);
}

#if defined(CL_VERSION_3_0)
class clGetProgramInfoTestParam : public clGetProgramInfoTest,
                                  public ::testing::WithParamInterface<
                                      std::tuple<size_t, cl_program_info>> {};

TEST_P(clGetProgramInfoTestParam, Query30) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  // Unpack the paramters.
  auto value_size_in_bytes = std::get<0>(GetParam());
  auto query_enum_value = std::get<1>(GetParam());

  // Query for size of value.
  size_t size{};
  ASSERT_SUCCESS(
      clGetProgramInfo(program, query_enum_value, 0, nullptr, &size));
  EXPECT_EQ(size, value_size_in_bytes);

  // Query for the value.
  UCL::Buffer<char> value_buffer{value_size_in_bytes};
  EXPECT_SUCCESS(clGetProgramInfo(program, query_enum_value,
                                  value_buffer.size(), value_buffer.data(),
                                  nullptr));

  // Query for the value with buffer that is too small.
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetProgramInfo(program, query_enum_value, value_buffer.size() - 1,
                       value_buffer.data(), nullptr));
}

INSTANTIATE_TEST_CASE_P(
    ProgramQuery, clGetProgramInfoTestParam,
    ::testing::Values(std::make_tuple(sizeof(cl_bool),
                                      CL_PROGRAM_SCOPE_GLOBAL_CTORS_PRESENT),
                      std::make_tuple(sizeof(cl_bool),
                                      CL_PROGRAM_SCOPE_GLOBAL_DTORS_PRESENT)),
    [](const testing::TestParamInfo<clGetProgramInfoTestParam::ParamType>
           &info) {
      return UCL::programQueryToString(std::get<1>(info.param));
    });
#endif
