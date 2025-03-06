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

#include "kts/relationals.h"

#include <cargo/utility.h>

#include <cstring>

#include "Common.h"
#include "kts/precision.h"
#include "ucl/enums.h"
#include "ucl/environment.h"

#define CHECK_CL_SUCCESS(X) \
  if (CL_SUCCESS != (X)) {  \
    return nullptr;         \
  }

using namespace kts::ucl;

void RelationalTest::SetUp() {
  UCL_RETURN_ON_FATAL_FAILURE(::ucl::CommandQueueTest::SetUp());
}

void RelationalTest::TearDown() {
  for (auto buffer : buffers_) {
    EXPECT_EQ(CL_SUCCESS, clReleaseMemObject(buffer));
  }
  for (auto kernel : kernels_) {
    EXPECT_EQ(CL_SUCCESS, clReleaseKernel(kernel));
  }
  for (auto program : programs_) {
    EXPECT_EQ(CL_SUCCESS, clReleaseProgram(program));
  }
  ::ucl::CommandQueueTest::TearDown();
}

const std::unordered_map<std::string, std::string>
    RelationalTest::out_type_map = {
        {"half", "int"},      {"half2", "short2"},  {"half3", "short3"},
        {"half4", "short4"},  {"half8", "short8"},  {"half16", "short16"},
        {"float", "int"},     {"float2", "int2"},   {"float3", "int3"},
        {"float4", "int4"},   {"float8", "int8"},   {"float16", "int16"},
        {"double", "int"},    {"double2", "long2"}, {"double3", "long3"},
        {"double4", "long4"}, {"double8", "long8"}, {"double16", "long16"}};

std::string OneArgRelational::source_fmt_string(std::string extension,
                                                std::string in_type,
                                                std::string out_type,
                                                std::string builtin) {
  return extension + "\n" + "void kernel RelationalKernel(global " + in_type +
         " *in, global " + out_type + " *out) {\n" +
         "  size_t gid = get_global_id(0);\n" + "  out[gid] = " + builtin +
         "(in[gid]);\n" + "}\n";
}

std::string TwoArgRelational::source_fmt_string(
    std::string extension, std::array<std::string, 2> in_types,
    std::string out_type, std::string builtin) {
  return extension + "\n" + "void kernel RelationalKernel(global " +
         in_types[0] + " *in1, global " + in_types[1] + " *in2, global " +
         out_type + " *out) {\n" + "  size_t gid = get_global_id(0);\n" +
         "  out[gid] = " + builtin + "(in1[gid], in2[gid]);\n" + "}\n";
}

std::string ThreeArgRelational::source_fmt_string(
    std::string extension, std::array<std::string, 3> in_types,
    std::string out_type, std::string builtin) {
  return extension + "\n" + "void kernel RelationalKernel(global " +
         in_types[0] + " *in1, global " + in_types[1] + " *in2, global " +
         in_types[2] + " *in3, global " + out_type + " *out) {\n" +
         "  size_t gid = get_global_id(0);\n" + "  out[gid] = " + builtin +
         "(in1[gid], in2[gid], in3[gid]);\n" + "}\n";
}

namespace {
unsigned GetVecWidth(const std::string &type) {
  const char last = type.back();
  switch (last) {
    case '2':
      return 2;
    case '3':
      return 3;
    case '4':
      return 4;
    case '8':
      return 8;
    case '6':
      return 16;
    default:
      return 1;
  }
}

template <class T>
void GetTestTypes(cargo::small_vector<std::string, 6> &types) {
  const std::string scalar_str(TypeInfo<T>::as_str);
  cargo::small_vector<const char *, 6> vec_widths;
  ASSERT_EQ(cargo::success, vec_widths.assign({"", "2", "3", "4", "8", "16"}));
  for (auto w : vec_widths) {
    ASSERT_EQ(cargo::success, types.push_back(scalar_str + w));
  }
}

// Based on CTS test conformance_test_select `check_float()` verification
// function in test_conformance/select/util_select.c
template <class T>
bool VerifyResult(const T reference, const T kernel) {
  if (std::isnan(reference) && std::isnan(kernel)) {
    return true;
  }

  return reference == kernel;
}

template <>
bool VerifyResult(const cl_half reference, const cl_half kernel) {
  if (IsNaN(reference) && IsNaN(kernel)) {
    return true;
  }

  return reference == kernel;
}
}  // namespace

cl_ulong RelationalTest::GetBufferLimit() {
  return HalfInputSizes::getInputSize(getEnvironment()->math_mode) * 2ul;
}

cl_kernel RelationalTest::BuildKernel(cl_program program) {
  cl_int errcode =
      clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr);
  // Dump build log on compilation failure
  if (CL_SUCCESS != errcode) {
    cl_uint err_log;
    size_t build_log_size = 0;
    std::string build_log;
    err_log = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0,
                                    nullptr, &build_log_size);
    CHECK_CL_SUCCESS(err_log);

    build_log.resize(build_log_size);
    err_log = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                                    build_log_size, (void *)build_log.data(),
                                    nullptr);
    CHECK_CL_SUCCESS(err_log);

    (void)fprintf(stderr, "Build program failed with error: %d\n", errcode);
    (void)fprintf(stderr, "Build log:\n\n");
    (void)fprintf(stderr, "%s", build_log.c_str());

    return nullptr;
  }

  if (cargo::success != programs_.push_back(program)) {
    return nullptr;
  }

  cl_kernel kernel = clCreateKernel(program, "RelationalKernel", &errcode);
  CHECK_CL_SUCCESS(errcode);
  if (cargo::success != kernels_.push_back(kernel)) {
    return nullptr;
  }

  for (unsigned i = 0; i < buffers_.size(); i++) {
    cl_mem buffer = buffers_[i];
    errcode = clSetKernelArg(kernel, i, sizeof(cl_mem), (void *)&buffer);
    CHECK_CL_SUCCESS(errcode);
  }
  return kernel;
}

void RelationalTest::EnqueueKernel(cl_kernel kernel, size_t work_items) {
  cl_int errcode =
      clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr, &work_items,
                             nullptr, 0, nullptr, nullptr);
  ASSERT_EQ(CL_SUCCESS, errcode);
  errcode = clFinish(command_queue);
  ASSERT_EQ(CL_SUCCESS, errcode);
}

template <class T>
void RelationalTest::FillInputBuffers(unsigned num_elements) {
  cl_int errcode = CL_SUCCESS;

  // The last buffer is for output
  const int input_buffers = buffers_.size() - 1;
  for (int i = 0; i < input_buffers; ++i) {
    // Map buffer and copy input data to it
    cl_mem buffer = buffers_[i];
    void *mapped_input =
        clEnqueueMapBuffer(command_queue, buffer, CL_TRUE, CL_MAP_WRITE, 0,
                           buffer_size_, 0, nullptr, nullptr, &errcode);
    ASSERT_EQ(CL_SUCCESS, errcode);

    // Fill mapped pointer with data by memcpying from vector
    std::vector<T> random_data(num_elements);
    getInputGenerator().GenerateFloatData(random_data);
    ASSERT_EQ(num_elements, random_data.size());
    memcpy(mapped_input, random_data.data(), num_elements * sizeof(T));

    clEnqueueUnmapMemObject(command_queue, buffer, mapped_input, 0, nullptr,
                            nullptr);
  }
}

template <unsigned N>
void RelationalTest::ReadMapBuffers(
    cargo::small_vector<void *, N> &mapped_ptrs) {
  ASSERT_TRUE(N <= buffers_.size());
  cl_int errcode = CL_SUCCESS;
  for (unsigned i = 0; i < N; i++) {
    cl_mem buffer = buffers_[i];
    void *mapped =
        clEnqueueMapBuffer(command_queue, buffer, CL_TRUE, CL_MAP_READ, 0,
                           buffer_size_, 0, nullptr, nullptr, &errcode);
    ASSERT_EQ(CL_SUCCESS, errcode);
    ASSERT_EQ(cargo::success, mapped_ptrs.push_back(mapped));
  }
}

template <unsigned N>
void RelationalTest::UnmapBuffers(cargo::small_vector<void *, N> &mapped_ptrs) {
  for (unsigned i = 0; i < N; ++i) {
    cl_mem buffer = buffers_[i];
    void *ptr = mapped_ptrs[i];
    clEnqueueUnmapMemObject(command_queue, buffer, ptr, 0, nullptr, nullptr);
  }
}

void OneArgRelational::SetUp() {
  UCL_RETURN_ON_FATAL_FAILURE(RelationalTest::SetUp());

  auto global_mem_size = getDeviceGlobalMemSize();
  auto mem_per_buffer = global_mem_size / 2;
  buffer_size_ = std::min(mem_per_buffer, GetBufferLimit());

  cl_int errcode = CL_SUCCESS;
  cl_mem input = clCreateBuffer(context, 0, buffer_size_, nullptr, &errcode);
  ASSERT_EQ(CL_SUCCESS, errcode);
  ASSERT_EQ(cargo::success, buffers_.push_back(input));

  cl_mem output = clCreateBuffer(context, 0, buffer_size_, nullptr, &errcode);
  ASSERT_EQ(CL_SUCCESS, errcode);
  ASSERT_EQ(cargo::success, buffers_.push_back(output));
}

cl_kernel OneArgRelational::ConstructProgram(const char *builtin,
                                             const std::string &test_type) {
  // Find return type of builtin
  const std::string out_type =
      RelationalTest::out_type_map.find(test_type)->second;

  // Programs which use half and halfn types should include a pragma directive
  const bool uses_half = 0 == test_type.compare(0, strlen("half"), "half");
  const char *extension_str =
      uses_half ? "#pragma OPENCL EXTENSION cl_khr_fp16 : enable" : "";

  // Substitute types and builtin name into OpenCL-C kernel string
  std::string program_string = OneArgRelational::source_fmt_string(
      extension_str, test_type, out_type, builtin);
  const char *program_cstr = program_string.data();

  // Call OpenCL APIs to compile kernel
  cl_int errcode = CL_SUCCESS;
  cl_program program =
      clCreateProgramWithSource(context, 1, &program_cstr, nullptr, &errcode);
  CHECK_CL_SUCCESS(errcode);

  cl_kernel kernel = BuildKernel(program);
  return kernel;
}

template <class T>
void OneArgRelational::TestAgainstReference(const char *builtin,
                                            const std::function<bool(T)> &ref) {
  // Check if we can compile the test
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }

  // Find all the types to test
  cargo::small_vector<std::string, 6> types;
  GetTestTypes<T>(types);
  ASSERT_TRUE(!types.empty());

  // Number of scalar input entries in the buffer
  const unsigned scalar_elems = (sizeof(T) < sizeof(cl_int))
                                    ? buffer_size_ / sizeof(cl_int)
                                    : buffer_size_ / sizeof(T);
  // Populate input buffer with test data
  FillInputBuffers<T>(scalar_elems);

  // Iterate over types to test
  for (auto &type_str : types) {
    // Build kernel
    cl_kernel kernel = ConstructProgram(builtin, type_str);
    ASSERT_TRUE(kernel != nullptr);

    // Size of each vector packet
    const unsigned vec_width = GetVecWidth(type_str);
    const unsigned vec_size = vec_width == 3 ? 4 : vec_width;

    // Number of complete vectors we can process
    const size_t work_items = scalar_elems / vec_size;

    // Run kernel
    EnqueueKernel(kernel, work_items);

    // Map buffers to verify results
    cargo::small_vector<void *, 2> mapped_ptrs;
    ReadMapBuffers<2>(mapped_ptrs);
    T *input = (T *)mapped_ptrs[0];
    void *output = mapped_ptrs[1];

    // Verify correct result
    for (size_t i = 0; i < scalar_elems; ++i) {
      // Ignore sneaky 4th element of vec3
      if ((vec_width == 3) && (i % 4) == 3) {
        continue;
      }

      // Run reference function
      const bool ref_result = ref(input[i]);

      // Output type is different depending on input type
      if (vec_size == 1) {
        auto kernel_casted = (cl_int *)output;
        const cl_int kernel_result = kernel_casted[i];
        if (ref_result != kernel_result) {
          FAIL() << type_str << " - Expected: " << ref_result << ", was "
                 << kernel_result << "\nInput: " << std::hex << input[i];
        }
      } else {
        auto kernel_casted = (typename TypeInfo<T>::AsSigned *)output;
        const typename TypeInfo<T>::AsSigned kernel_result = kernel_casted[i];
        const typename TypeInfo<T>::AsSigned expected_result =
            ref_result ? -1 : 0;
        if (expected_result != kernel_result) {
          FAIL() << type_str << " - Expected: " << expected_result << ", was "
                 << kernel_result << "\nInput: " << std::hex << input[i];
        }
      }
    }

    UnmapBuffers<2>(mapped_ptrs);
  }
}

// explicit instantiation to avoid linking errors
template void OneArgRelational::TestAgainstReference<cl_half>(
    const char *, const std::function<bool(cl_half)> &);
template void OneArgRelational::TestAgainstReference<cl_float>(
    const char *, const std::function<bool(cl_float)> &);
template void OneArgRelational::TestAgainstReference<cl_double>(
    const char *, const std::function<bool(cl_double)> &);

void TwoArgRelational::SetUp() {
  UCL_RETURN_ON_FATAL_FAILURE(RelationalTest::SetUp());
  const int num_buffers = 3;
  auto mem_per_buffer = getDeviceGlobalMemSize() / num_buffers;
  buffer_size_ = std::min(mem_per_buffer, GetBufferLimit());

  cl_int errcode = CL_SUCCESS;
  for (int i = 0; i < num_buffers; i++) {
    cl_mem buffer = clCreateBuffer(context, 0, buffer_size_, nullptr, &errcode);
    ASSERT_EQ(CL_SUCCESS, errcode);
    ASSERT_EQ(cargo::success, buffers_.push_back(buffer));
  }
}

cl_kernel TwoArgRelational::ConstructProgram(const char *builtin,
                                             const std::string &test_type) {
  // Find return type of builtin
  const std::string out_type =
      RelationalTest::out_type_map.find(test_type)->second;

  // Programs which use half and halfn types should include a pragma directive
  const bool uses_half = 0 == test_type.compare(0, strlen("half"), "half");
  const char *extension_str =
      uses_half ? "#pragma OPENCL EXTENSION cl_khr_fp16 : enable" : "";

  // Substitute types and builtin name into OpenCL-C kernel string
  std::string program_string = TwoArgRelational::source_fmt_string(
      extension_str, {test_type, test_type}, out_type, builtin);
  const char *program_cstr = program_string.data();

  // Call OpenCL APIs to compile kernel
  cl_int errcode = CL_SUCCESS;
  cl_program program =
      clCreateProgramWithSource(context, 1, &program_cstr, nullptr, &errcode);
  CHECK_CL_SUCCESS(errcode);

  cl_kernel kernel = BuildKernel(program);
  return kernel;
}

template <class T>
bool TwoArgRelational::FTZVerify(const std::function<bool(T, T)> &ref, T a, T b,
                                 bool result) {
  const bool ftz_a = IsDenormal(a);
  const bool ftz_b = IsDenormal(b);

  if (ftz_a && (ref(0, b) == result)) {
    return true;
  }

  if (ftz_b && (ref(a, 0) == result)) {
    return true;
  }

  if (ftz_a && ftz_b && (ref(0, 0) == result)) {
    return true;
  }

  return false;
}

template <class T>
void TwoArgRelational::TestAgainstReference(
    const char *builtin, const std::function<bool(T, T)> &ref) {
  // Check if we can compile the test
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }

  // Find types to test
  cargo::small_vector<std::string, 6> types;
  GetTestTypes<T>(types);
  ASSERT_TRUE(!types.empty());

  // Number of scalar input entries in the buffer
  const unsigned scalar_elems = (sizeof(T) < sizeof(cl_int))
                                    ? buffer_size_ / sizeof(cl_int)
                                    : buffer_size_ / sizeof(T);
  FillInputBuffers<T>(scalar_elems);

  // Query device for denormal number support
  cl_device_info float_type = CL_DEVICE_SINGLE_FP_CONFIG;
  if (std::is_same_v<T, cl_half>) {
    float_type = CL_DEVICE_HALF_FP_CONFIG;
  } else if (std::is_same_v<T, cl_double>) {
    float_type = CL_DEVICE_DOUBLE_FP_CONFIG;
  }
  const bool denorms_supported = UCL::hasDenormSupport(device, float_type);

  // Iterate over types to test
  for (auto &type_str : types) {
    cl_kernel kernel = ConstructProgram(builtin, type_str);
    ASSERT_TRUE(kernel != nullptr);

    // Size of each vector packet
    const unsigned vec_width = GetVecWidth(type_str);
    const unsigned vec_size = vec_width == 3 ? 4 : vec_width;

    // Number of complete vectors we can process
    const size_t work_items = scalar_elems / vec_size;

    // Run Kernel
    EnqueueKernel(kernel, work_items);

    // Map buffers to verify data
    cargo::small_vector<void *, 3> mapped_ptrs;
    ReadMapBuffers<3>(mapped_ptrs);
    T *input1 = (T *)mapped_ptrs[0];
    T *input2 = (T *)mapped_ptrs[1];
    void *output = mapped_ptrs[2];

    for (size_t i = 0; i < scalar_elems; ++i) {
      // Ignore sneaky 4th element of vec3
      if ((vec_width == 3) && (i % 4) == 3) {
        continue;
      }

      // Run reference function
      const bool ref_result = ref(input1[i], input2[i]);

      // Output is different type depending on vector width
      if (vec_width == 1) {
        auto kernel_casted = (cl_int *)output;
        const cl_int kernel_result = kernel_casted[i];
        if (ref_result != kernel_result) {
          // Try flushing denormal inputs to zero
          if (!denorms_supported &&
              TwoArgRelational::FTZVerify(ref, input1[i], input2[i],
                                          kernel_result)) {
            continue;
          }

          FAIL() << type_str << " - Expected: " << ref_result << ", was "
                 << kernel_result << "\nInput: " << std::hex << input1[i]
                 << ", " << input2[i];
        }
      } else {
        auto kernel_casted = (typename TypeInfo<T>::AsSigned *)output;
        // For vector types true is returned as -1(i.e. all bits set)
        const bool kernel_result = (-1 == kernel_casted[i]);
        if (ref_result != kernel_result) {
          // Try flushing denormal inputs to zero
          if (!denorms_supported &&
              TwoArgRelational::FTZVerify(ref, input1[i], input2[i],
                                          kernel_result)) {
            continue;
          }

          FAIL() << type_str << " - Expected: " << ref_result << ", was "
                 << kernel_result << "\nInput: " << std::hex << input1[i]
                 << ", " << input2[i];
        }
      }
    }

    UnmapBuffers<3>(mapped_ptrs);
  }
}

// explicit instantiation to avoid linking errors
template void TwoArgRelational::TestAgainstReference<cl_half>(
    const char *, const std::function<bool(cl_half, cl_half)> &);
template void TwoArgRelational::TestAgainstReference<cl_float>(
    const char *, const std::function<bool(cl_float, cl_float)> &);
template void TwoArgRelational::TestAgainstReference<cl_double>(
    const char *, const std::function<bool(cl_double, cl_double)> &);

void ThreeArgRelational::SetUp() {
  UCL_RETURN_ON_FATAL_FAILURE(RelationalTest::SetUp());
  const int num_buffers = 4;
  auto mem_per_buffer = getDeviceGlobalMemSize() / num_buffers;
  buffer_size_ = std::min(mem_per_buffer, GetBufferLimit());
  cl_int errcode = CL_SUCCESS;
  for (int i = 0; i < num_buffers; i++) {
    cl_mem buffer = clCreateBuffer(context, 0, buffer_size_, nullptr, &errcode);
    ASSERT_EQ(CL_SUCCESS, errcode);
    ASSERT_EQ(cargo::success, buffers_.push_back(buffer));
  }
}

cl_kernel BitSelectTest::ConstructProgram(const char *test_type) {
  // Programs which use half and halfn types should include a pragma directive
  const bool uses_half = 0 == strncmp(test_type, "half", strlen("half"));
  const char *extension_str =
      uses_half ? "#pragma OPENCL EXTENSION cl_khr_fp16 : enable" : "";

  std::string program_string = ThreeArgRelational::source_fmt_string(
      extension_str, {test_type, test_type, test_type}, test_type, "bitselect");
  const char *program_cstr = program_string.data();

  // Call OpenCL APIs to compile kernel
  cl_int errcode = CL_SUCCESS;
  cl_program program =
      clCreateProgramWithSource(context, 1, &program_cstr, nullptr, &errcode);
  CHECK_CL_SUCCESS(errcode);

  cl_kernel kernel = BuildKernel(program);
  return kernel;
}

template <class T, class U>
void BitSelectTest::TestAgainstReference(const std::function<U(U, U, U)> &ref) {
  static_assert(
      std::is_same_v<typename TypeInfo<T>::AsUnsigned, U>,
      "Lambda must return unsigned int type of same size as float input");
  // Check if we can compile the test
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }

  // Find types to test
  cargo::small_vector<std::string, 6> types;
  GetTestTypes<T>(types);
  ASSERT_TRUE(!types.empty());

  // Find input buffers with test data
  const unsigned scalar_elems = buffer_size_ / sizeof(T);
  FillInputBuffers<T>(scalar_elems);

  // Iterate over types to test
  for (auto &type_str : types) {
    // Build OpenCL kernel
    cl_kernel kernel = ConstructProgram(type_str.c_str());
    ASSERT_TRUE(kernel != nullptr);

    // Size of each vector packet
    const unsigned vec_width = GetVecWidth(type_str);
    const unsigned vec_size = vec_width == 3 ? 4 : vec_width;

    // Number of complete vectors we can process
    const size_t work_items = scalar_elems / vec_size;

    // Run kernel
    EnqueueKernel(kernel, work_items);

    // Map buffers so we can verify results
    cargo::small_vector<void *, 4> mapped_ptrs;
    ReadMapBuffers<4>(mapped_ptrs);
    U *input1 = (U *)mapped_ptrs[0];
    U *input2 = (U *)mapped_ptrs[1];
    U *input3 = (U *)mapped_ptrs[2];
    U *output = (U *)mapped_ptrs[3];

    for (size_t i = 0; i < scalar_elems; ++i) {
      // Ignore sneaky 4th element of vec3
      if ((vec_width == 3) && (i % 4) == 3) {
        continue;
      }

      // Run reference function
      U ref_result = ref(input1[i], input2[i], input3[i]);

      // Result from kernel on device
      U kernel_result = output[i];

      if (ref_result != kernel_result) {
        FAIL() << type_str << " - Expected: " << std::hex << ref_result
               << ", was " << kernel_result << "\nInput: " << input1[i] << ", "
               << input2[i] << ", " << input3[i];
      }
    }

    UnmapBuffers<4>(mapped_ptrs);
  }
}

// explicit instantiation to avoid linking errors
template void BitSelectTest::TestAgainstReference<cl_half, cl_ushort>(
    const std::function<cl_ushort(cl_ushort, cl_ushort, cl_ushort)> &);
template void BitSelectTest::TestAgainstReference<cl_float, cl_uint>(
    const std::function<cl_uint(cl_uint, cl_uint, cl_uint)> &);
template void BitSelectTest::TestAgainstReference<cl_double, cl_ulong>(
    const std::function<cl_ulong(cl_ulong, cl_ulong, cl_ulong)> &);

cl_kernel SelectTest::ConstructProgram(const char *float_type,
                                       const char *int_type) {
  // Programs which use half and halfn types should include a pragma directive
  const bool uses_half = 0 == strncmp(float_type, "half", strlen("half"));
  const char *extension_str =
      uses_half ? "#pragma OPENCL EXTENSION cl_khr_fp16 : enable" : "";

  std::string program_string = ThreeArgRelational::source_fmt_string(
      extension_str, {float_type, float_type, int_type}, float_type, "select");
  const char *program_cstr = program_string.data();

  // Call OpenCL APIs to compile kernel
  cl_int errcode = CL_SUCCESS;
  cl_program program =
      clCreateProgramWithSource(context, 1, &program_cstr, nullptr, &errcode);
  CHECK_CL_SUCCESS(errcode);

  cl_kernel kernel = BuildKernel(program);
  return kernel;
}

template <class T, class U>
void SelectTest::TestAgainstReference(const std::function<T(T, T, U)> &ref,
                                      bool is_scalar) {
  // Check if we can compile the test
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }

  cl_int errcode = CL_SUCCESS;

  // Fill buffers with data
  const unsigned scalar_elems = buffer_size_ / sizeof(T);

  // Fill floating point buffers
  for (int i = 0; i < 2; ++i) {
    cl_mem buffer = buffers_[i];
    void *mapped_input =
        clEnqueueMapBuffer(command_queue, buffer, CL_TRUE, CL_MAP_WRITE, 0,
                           buffer_size_, 0, nullptr, nullptr, &errcode);
    ASSERT_EQ(CL_SUCCESS, errcode);

    // Fill mapped pointer with data by memcpying from vector
    std::vector<T> random_data(scalar_elems);
    getInputGenerator().GenerateFloatData(random_data);
    ASSERT_EQ(scalar_elems, random_data.size());
    memcpy(mapped_input, random_data.data(), scalar_elems * sizeof(T));

    clEnqueueUnmapMemObject(command_queue, buffer, mapped_input, 0, nullptr,
                            nullptr);
  }

  // Fill 3rd buffer which takes an integer
  cl_mem buffer = buffers_[2];
  void *mapped_input =
      clEnqueueMapBuffer(command_queue, buffer, CL_TRUE, CL_MAP_WRITE, 0,
                         buffer_size_, 0, nullptr, nullptr, &errcode);

  // Fill mapped pointer with data by memcpying from vector
  std::vector<U> random_data(scalar_elems);
  getInputGenerator().GenerateIntData(random_data);
  ASSERT_EQ(scalar_elems, random_data.size());
  memcpy(mapped_input, random_data.data(), scalar_elems * sizeof(U));

  clEnqueueUnmapMemObject(command_queue, buffer, mapped_input, 0, nullptr,
                          nullptr);

  // Iterate over types to test
  cargo::small_vector<std::string, 6> types;
  GetTestTypes<T>(types);
  ASSERT_TRUE(!types.empty());

  for (auto &type_str : types) {
    const unsigned vec_width = GetVecWidth(type_str);
    if (is_scalar && vec_width != 1) {
      continue;
    } else if (!is_scalar && vec_width == 1) {
      continue;
    }

    std::string out_type = RelationalTest::out_type_map.find(type_str)->second;
    if (std::is_unsigned_v<U>) {
      out_type.insert(0, 1, 'u');
    }

    if (1 == vec_width) {
      out_type = Stringify<U>::as_str;
    }

    // Build Program
    cl_kernel kernel = ConstructProgram(type_str.c_str(), out_type.c_str());
    ASSERT_TRUE(kernel != nullptr);

    // Size of each vector packet
    const unsigned vec_size = vec_width == 3 ? 4 : vec_width;

    // Number of complete vectors we can process
    const size_t work_items = scalar_elems / vec_size;

    // Run Kernel
    EnqueueKernel(kernel, work_items);

    // Map Buffers for reading
    cargo::small_vector<void *, 4> mapped_ptrs;
    ReadMapBuffers<4>(mapped_ptrs);
    T *input1 = (T *)mapped_ptrs[0];
    T *input2 = (T *)mapped_ptrs[1];
    U *input3 = (U *)mapped_ptrs[2];
    T *output = (T *)mapped_ptrs[3];

    for (size_t i = 0; i < scalar_elems; ++i) {
      // Ignore sneaky 4th element of vec3
      if ((vec_width == 3) && (i % 4) == 3) {
        continue;
      }

      // Run reference function
      const T ref_result = ref(input1[i], input2[i], input3[i]);

      // Result from device
      const T kernel_result = output[i];

      if (!VerifyResult(ref_result, kernel_result)) {
        FAIL() << type_str << " - Expected " << i << ": " << std::hex << "0x"
               << matchingType(ref_result) << ", was 0x"
               << matchingType(kernel_result) << "\nInput: 0x"
               << matchingType(input1[i]) << ", 0x" << matchingType(input2[i])
               << ", 0x" << input3[i];
      }
    }
    UnmapBuffers<4>(mapped_ptrs);
  }
}

// explicit instantiation to avoid linking errors
template void SelectTest::TestAgainstReference<cl_half, cl_ushort>(
    const std::function<cl_half(cl_half, cl_half, cl_ushort)> &, bool);
template void SelectTest::TestAgainstReference<cl_half, cl_short>(
    const std::function<cl_half(cl_half, cl_half, cl_short)> &, bool);
template void SelectTest::TestAgainstReference<cl_float, cl_uint>(
    const std::function<cl_float(cl_float, cl_float, cl_uint)> &, bool);
template void SelectTest::TestAgainstReference<cl_float, cl_int>(
    const std::function<cl_float(cl_float, cl_float, cl_int)> &, bool);
template void SelectTest::TestAgainstReference<cl_double, cl_ulong>(
    const std::function<cl_double(cl_double, cl_double, cl_ulong)> &, bool);
template void SelectTest::TestAgainstReference<cl_double, cl_long>(
    const std::function<cl_double(cl_double, cl_double, cl_long)> &, bool);
