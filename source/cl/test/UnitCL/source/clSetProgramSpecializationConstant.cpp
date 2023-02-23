// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <CL/cl_ext.h>
#include <Common.h>

// The clSetProgramSpecializationConstant.spv{32,64} binaries contain the
// following list of specialization constants:
//
// * SpecId: 0       OpTypeBool      1 bit     Default: true
// * SpecId: 1       OpTypeBool      1 bit     Default: false
// * SpecId: 2       OpTypeInt       8 bit     Default: 23
// * SpecId: 3       OpTypeInt       32 bit    Default: 23
// * SpecId: 4       OpTypeInt       16 bit    Default: 23
// * SpecId: 5       OpTypeInt       64 bit    Default: 23
// * SpecId: 6       OpTypeFloat     32 bit    Default: 23.0

struct clSetProgramSpecializationConstantTest : ucl::ContextTest {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ucl::ContextTest::SetUp());
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
    // Requires a compiler to compile the kernel.
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    auto code = getDeviceSpirvFromFile("clSetProgramSpecializationConstant");
    cl_int error;
    // TODO: clCreateProgramWithIL has not been implemented yet
    clCreateProgramWithILKHR = reinterpret_cast<clCreateProgramWithILKHR_fn>(
        clGetExtensionFunctionAddressForPlatform(platform,
                                                 "clCreateProgramWithILKHR"));
    ASSERT_NE(nullptr, clCreateProgramWithILKHR);
    program = clCreateProgramWithILKHR(context, code.data(),
                                       code.size() * sizeof(uint32_t), &error);
    ASSERT_SUCCESS(error);
  }

  void TearDown() override {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ucl::ContextTest::TearDown();
  }

  clCreateProgramWithILKHR_fn clCreateProgramWithILKHR = nullptr;
  cl_program program = nullptr;
};

TEST_F(clSetProgramSpecializationConstantTest, InvalidProgramNull) {
  cl_int value = 42;
  ASSERT_EQ_ERRCODE(CL_INVALID_PROGRAM, clSetProgramSpecializationConstant(
                                            nullptr, 0, sizeof(value), &value));
}

TEST_F(clSetProgramSpecializationConstantTest, InvalidProgramFromSource) {
  const char *source = R"OpenCLC(
kernel void test(global int* out) {
  size_t id = get_global_id(0);
  out[id] = (int)id;
}
)OpenCLC";
  const size_t length = strlen(source);
  cl_int error;
  UCL::Program sourceProgram =
      clCreateProgramWithSource(context, 1, &source, &length, &error);
  ASSERT_SUCCESS(error);
  cl_uchar value = false;
  ASSERT_EQ_ERRCODE(CL_INVALID_PROGRAM,
                    clSetProgramSpecializationConstant(sourceProgram, 0,
                                                       sizeof(value), &value));
}

TEST_F(clSetProgramSpecializationConstantTest, InvalidValueSizeTooSmall) {
  cl_int value = 42;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clSetProgramSpecializationConstant(program, 4, 1, &value));
}

TEST_F(clSetProgramSpecializationConstantTest, InvalidValueSizeTooLarge) {
  cl_int value = 42;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clSetProgramSpecializationConstant(program, 4, 8, &value));
}

struct clSetProgramSpecializationConstantSuccessTest
    : clSetProgramSpecializationConstantTest {
  void SetUp() final {
    UCL_RETURN_ON_FATAL_FAILURE(
        clSetProgramSpecializationConstantTest::SetUp());
    cl_int error;
    commandQueue = clCreateCommandQueue(context, device, 0, &error);
    ASSERT_SUCCESS(error);
    boolBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(bool) * 2,
                                nullptr, &error);
    ASSERT_SUCCESS(error);
    charBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_char),
                                nullptr, &error);
    ASSERT_SUCCESS(error);
    shortBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_short),
                                 nullptr, &error);
    ASSERT_SUCCESS(error);
    intBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_int),
                               nullptr, &error);
    ASSERT_SUCCESS(error);
    longBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_long),
                                nullptr, &error);
    ASSERT_SUCCESS(error);
    floatBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_float),
                                 nullptr, &error);
    ASSERT_SUCCESS(error);
  }

  void TearDown() final {
    if (floatBuffer) {
      ASSERT_SUCCESS(clReleaseMemObject(floatBuffer));
    }
    if (longBuffer) {
      ASSERT_SUCCESS(clReleaseMemObject(longBuffer));
    }
    if (shortBuffer) {
      ASSERT_SUCCESS(clReleaseMemObject(shortBuffer));
    }
    if (intBuffer) {
      ASSERT_SUCCESS(clReleaseMemObject(intBuffer));
    }
    if (boolBuffer) {
      ASSERT_SUCCESS(clReleaseMemObject(boolBuffer));
    }
    if (charBuffer) {
      ASSERT_SUCCESS(clReleaseMemObject(charBuffer));
    }
    if (kernel) {
      ASSERT_SUCCESS(clReleaseKernel(kernel));
    }
    if (commandQueue) {
      ASSERT_SUCCESS(clReleaseCommandQueue(commandQueue));
    }
    clSetProgramSpecializationConstantTest::TearDown();
  }

  void getResults() {
    ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), &boolBuffer));
    ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem), &charBuffer));
    ASSERT_SUCCESS(clSetKernelArg(kernel, 2, sizeof(cl_mem), &shortBuffer));
    ASSERT_SUCCESS(clSetKernelArg(kernel, 3, sizeof(cl_mem), &intBuffer));
    ASSERT_SUCCESS(clSetKernelArg(kernel, 4, sizeof(cl_mem), &longBuffer));
    ASSERT_SUCCESS(clSetKernelArg(kernel, 5, sizeof(cl_mem), &floatBuffer));
    cl_event taskEvent;
    ASSERT_SUCCESS(clEnqueueTask(commandQueue, kernel, 0, nullptr, &taskEvent));
    std::array<cl_event, 6> resultEvents;
    ASSERT_SUCCESS(clEnqueueReadBuffer(commandQueue, boolBuffer, CL_FALSE, 0,
                                       sizeof(bool) * 2, boolResults.data(), 1,
                                       &taskEvent, &resultEvents[0]));
    ASSERT_SUCCESS(clEnqueueReadBuffer(commandQueue, charBuffer, CL_FALSE, 0,
                                       sizeof(cl_char), &charResult, 1,
                                       &taskEvent, &resultEvents[1]));
    ASSERT_SUCCESS(clEnqueueReadBuffer(commandQueue, shortBuffer, CL_FALSE, 0,
                                       sizeof(cl_short), &shortResult, 1,
                                       &taskEvent, &resultEvents[2]));
    ASSERT_SUCCESS(clEnqueueReadBuffer(commandQueue, intBuffer, CL_FALSE, 0,
                                       sizeof(cl_int), &intResult, 1,
                                       &taskEvent, &resultEvents[3]));
    ASSERT_SUCCESS(clEnqueueReadBuffer(commandQueue, longBuffer, CL_FALSE, 0,
                                       sizeof(cl_long), &longResult, 1,
                                       &taskEvent, &resultEvents[4]));
    ASSERT_SUCCESS(clEnqueueReadBuffer(commandQueue, floatBuffer, CL_FALSE, 0,
                                       sizeof(cl_float), &floatResult, 1,
                                       &taskEvent, &resultEvents[5]));
    ASSERT_SUCCESS(clWaitForEvents(resultEvents.size(), resultEvents.data()));

    std::for_each(
        std::begin(resultEvents), std::end(resultEvents),
        [](cl_event event) { EXPECT_SUCCESS(clReleaseEvent(event)); });
    EXPECT_SUCCESS(clReleaseEvent(taskEvent));
  }

  cl_command_queue commandQueue = nullptr;
  cl_kernel kernel = nullptr;
  cl_mem boolBuffer = nullptr;
  cl_mem charBuffer = nullptr;
  cl_mem shortBuffer = nullptr;
  cl_mem intBuffer = nullptr;
  cl_mem longBuffer = nullptr;
  cl_mem floatBuffer = nullptr;
  std::array<bool, 2> boolResults;
  cl_char charResult;
  cl_short shortResult;
  cl_int intResult;
  cl_long longResult;
  cl_float floatResult;
};

TEST_F(clSetProgramSpecializationConstantSuccessTest, None) {
  ASSERT_SUCCESS(
      clBuildProgram(program, 1, &device, "", ucl::buildLogCallback, nullptr));
  cl_int error;
  kernel = clCreateKernel(program, "test", &error);
  ASSERT_SUCCESS(error);
  UCL_RETURN_ON_FATAL_FAILURE(getResults());
  ASSERT_EQ(true, boolResults[0]);          // SpecId: 0
  ASSERT_EQ(false, boolResults[1]);         // SpecId: 1
  ASSERT_EQ(cl_char(23), charResult);       // SpecId: 2
  ASSERT_EQ(cl_short(23), shortResult);     // SpecId: 3
  ASSERT_EQ(cl_int(23), intResult);         // SpecId: 4
  ASSERT_EQ(cl_long(23), longResult);       // SpecId: 5
  ASSERT_EQ(cl_float(23.0f), floatResult);  // SpecId: 6
}

TEST_F(clSetProgramSpecializationConstantSuccessTest,
       SpecId0OpSpecConstantTrue) {
  bool value = false;
  ASSERT_SUCCESS(
      clSetProgramSpecializationConstant(program, 0, sizeof(value), &value));
  ASSERT_SUCCESS(
      clBuildProgram(program, 1, &device, "", ucl::buildLogCallback, nullptr));
  cl_int error;
  kernel = clCreateKernel(program, "test", &error);
  ASSERT_SUCCESS(error);
  UCL_RETURN_ON_FATAL_FAILURE(getResults());
  ASSERT_EQ(value, boolResults[0]);         // SpecId: 0
  ASSERT_EQ(false, boolResults[1]);         // SpecId: 1
  ASSERT_EQ(cl_char(23), charResult);       // SpecId: 2
  ASSERT_EQ(cl_short(23), shortResult);     // SpecId: 3
  ASSERT_EQ(cl_int(23), intResult);         // SpecId: 4
  ASSERT_EQ(cl_long(23), longResult);       // SpecId: 5
  ASSERT_EQ(cl_float(23.0f), floatResult);  // SpecId: 6
}

TEST_F(clSetProgramSpecializationConstantSuccessTest,
       SpecId1OpSpecConstantFalse) {
  bool value = true;
  ASSERT_SUCCESS(
      clSetProgramSpecializationConstant(program, 1, sizeof(value), &value));
  ASSERT_SUCCESS(
      clBuildProgram(program, 1, &device, "", ucl::buildLogCallback, nullptr));
  cl_int error;
  kernel = clCreateKernel(program, "test", &error);
  ASSERT_SUCCESS(error);
  UCL_RETURN_ON_FATAL_FAILURE(getResults());
  ASSERT_EQ(true, boolResults[0]);          // SpecId: 0
  ASSERT_EQ(value, boolResults[1]);         // SpecId: 1
  ASSERT_EQ(cl_char(23), charResult);       // SpecId: 2
  ASSERT_EQ(cl_short(23), shortResult);     // SpecId: 3
  ASSERT_EQ(cl_int(23), intResult);         // SpecId: 4
  ASSERT_EQ(cl_long(23), longResult);       // SpecId: 5
  ASSERT_EQ(cl_float(23.0f), floatResult);  // SpecId: 6
}

TEST_F(clSetProgramSpecializationConstantSuccessTest,
       SpecId2OpSpecConstantChar) {
  cl_char value = 42;
  ASSERT_SUCCESS(
      clSetProgramSpecializationConstant(program, 2, sizeof(value), &value));
  ASSERT_SUCCESS(
      clBuildProgram(program, 1, &device, "", ucl::buildLogCallback, nullptr));
  cl_int error;
  kernel = clCreateKernel(program, "test", &error);
  ASSERT_SUCCESS(error);
  UCL_RETURN_ON_FATAL_FAILURE(getResults());
  ASSERT_EQ(true, boolResults[0]);          // SpecId: 0
  ASSERT_EQ(false, boolResults[1]);         // SpecId: 1
  ASSERT_EQ(value, charResult);             // SpecId: 2
  ASSERT_EQ(cl_short(23), shortResult);     // SpecId: 3
  ASSERT_EQ(cl_int(23), intResult);         // SpecId: 4
  ASSERT_EQ(cl_long(23), longResult);       // SpecId: 5
  ASSERT_EQ(cl_float(23.0f), floatResult);  // SpecId: 6
}

TEST_F(clSetProgramSpecializationConstantSuccessTest,
       SpecId3OpSpecConstantShort) {
  cl_short value = 42;
  ASSERT_SUCCESS(
      clSetProgramSpecializationConstant(program, 3, sizeof(value), &value));
  ASSERT_SUCCESS(
      clBuildProgram(program, 1, &device, "", ucl::buildLogCallback, nullptr));
  cl_int error;
  kernel = clCreateKernel(program, "test", &error);
  ASSERT_SUCCESS(error);
  UCL_RETURN_ON_FATAL_FAILURE(getResults());
  ASSERT_EQ(true, boolResults[0]);          // SpecId: 0
  ASSERT_EQ(false, boolResults[1]);         // SpecId: 1
  ASSERT_EQ(cl_char(23), charResult);       // SpecId: 2
  ASSERT_EQ(value, shortResult);            // SpecId: 3
  ASSERT_EQ(cl_int(23), intResult);         // SpecId: 4
  ASSERT_EQ(cl_long(23), longResult);       // SpecId: 5
  ASSERT_EQ(cl_float(23.0f), floatResult);  // SpecId: 6
}

TEST_F(clSetProgramSpecializationConstantSuccessTest,
       SpecId4OpSpecConstantInt) {
  cl_int value = 42;
  ASSERT_SUCCESS(
      clSetProgramSpecializationConstant(program, 4, sizeof(value), &value));
  ASSERT_SUCCESS(
      clBuildProgram(program, 1, &device, "", ucl::buildLogCallback, nullptr));
  cl_int error;
  kernel = clCreateKernel(program, "test", &error);
  ASSERT_SUCCESS(error);
  UCL_RETURN_ON_FATAL_FAILURE(getResults());
  ASSERT_EQ(true, boolResults[0]);          // SpecId: 0
  ASSERT_EQ(false, boolResults[1]);         // SpecId: 1
  ASSERT_EQ(cl_char(23), charResult);       // SpecId: 2
  ASSERT_EQ(cl_short(23), shortResult);     // SpecId: 3
  ASSERT_EQ(value, intResult);              // SpecId: 4
  ASSERT_EQ(cl_long(23), longResult);       // SpecId: 5
  ASSERT_EQ(cl_float(23.0f), floatResult);  // SpecId: 6
}

TEST_F(clSetProgramSpecializationConstantSuccessTest,
       SpecId5OpSpecConstantLong) {
  cl_long value = 42;
  ASSERT_SUCCESS(
      clSetProgramSpecializationConstant(program, 5, sizeof(value), &value));
  ASSERT_SUCCESS(
      clBuildProgram(program, 1, &device, "", ucl::buildLogCallback, nullptr));
  cl_int error;
  kernel = clCreateKernel(program, "test", &error);
  ASSERT_SUCCESS(error);
  UCL_RETURN_ON_FATAL_FAILURE(getResults());
  ASSERT_EQ(true, boolResults[0]);          // SpecId: 0
  ASSERT_EQ(false, boolResults[1]);         // SpecId: 1
  ASSERT_EQ(cl_char(23), charResult);       // SpecId: 2
  ASSERT_EQ(cl_short(23), shortResult);     // SpecId: 3
  ASSERT_EQ(cl_int(23), intResult);         // SpecId: 4
  ASSERT_EQ(value, longResult);             // SpecId: 5
  ASSERT_EQ(cl_float(23.0f), floatResult);  // SpecId: 6
}

TEST_F(clSetProgramSpecializationConstantSuccessTest,
       SpecId6OpSpecConstantFloat) {
  cl_float value = 42.0f;
  ASSERT_SUCCESS(
      clSetProgramSpecializationConstant(program, 6, sizeof(value), &value));
  ASSERT_SUCCESS(
      clBuildProgram(program, 1, &device, "", ucl::buildLogCallback, nullptr));
  cl_int error;
  kernel = clCreateKernel(program, "test", &error);
  ASSERT_SUCCESS(error);
  UCL_RETURN_ON_FATAL_FAILURE(getResults());
  ASSERT_EQ(true, boolResults[0]);       // SpecId: 0
  ASSERT_EQ(false, boolResults[1]);      // SpecId: 1
  ASSERT_EQ(cl_char(23), charResult);    // SpecId: 2
  ASSERT_EQ(cl_short(23), shortResult);  // SpecId: 3
  ASSERT_EQ(cl_int(23), intResult);      // SpecId: 4
  ASSERT_EQ(cl_long(23), longResult);    // SpecId: 5
  ASSERT_EQ(value, floatResult);         // SpecId: 6
}

TEST_F(clSetProgramSpecializationConstantSuccessTest, All) {
  bool bool0Value = false;
  ASSERT_SUCCESS(clSetProgramSpecializationConstant(
      program, 0, sizeof(bool0Value), &bool0Value));
  bool bool1Value = true;
  ASSERT_SUCCESS(clSetProgramSpecializationConstant(
      program, 1, sizeof(bool1Value), &bool1Value));
  cl_char charValue = 42;
  ASSERT_SUCCESS(clSetProgramSpecializationConstant(
      program, 2, sizeof(charValue), &charValue));
  cl_short shortValue = 42;
  ASSERT_SUCCESS(clSetProgramSpecializationConstant(
      program, 3, sizeof(shortValue), &shortValue));
  cl_int intValue = 42;
  ASSERT_SUCCESS(clSetProgramSpecializationConstant(
      program, 4, sizeof(intValue), &intValue));
  cl_long longValue = 42;
  ASSERT_SUCCESS(clSetProgramSpecializationConstant(
      program, 5, sizeof(longValue), &longValue));
  cl_float floatValue = 42.0f;
  ASSERT_SUCCESS(clSetProgramSpecializationConstant(
      program, 6, sizeof(floatValue), &floatValue));
  ASSERT_SUCCESS(
      clBuildProgram(program, 1, &device, "", ucl::buildLogCallback, nullptr));
  cl_int error;
  kernel = clCreateKernel(program, "test", &error);
  ASSERT_SUCCESS(error);
  UCL_RETURN_ON_FATAL_FAILURE(getResults());
  ASSERT_EQ(bool0Value, boolResults[0]);  // SpecId: 0
  ASSERT_EQ(bool1Value, boolResults[1]);  // SpecId: 1
  ASSERT_EQ(charValue, charResult);       // SpecId: 2
  ASSERT_EQ(shortValue, shortResult);     // SpecId: 3
  ASSERT_EQ(intValue, intResult);         // SpecId: 4
  ASSERT_EQ(longValue, longResult);       // SpecId: 5
  ASSERT_EQ(floatValue, floatResult);     // SpecId: 6
}
