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

#include "kts/execution.h"

#include <cargo/allocator.h>
#include <cargo/string_algorithm.h>
#include <cargo/string_view.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <sstream>

#include "Common.h"
#include "Device.h"
#include "kts/arguments.h"

using namespace kts::ucl;

const std::array<SourceType, 4> &kts::ucl::getSourceTypes() {
  static const std::array<SourceType, 4> source_types = {
      {OPENCL_C, SPIRV, OFFLINE, OFFLINESPIRV}};
  return source_types;
}
const std::array<SourceType, 2> &kts::ucl::getOnlineSourceTypes() {
  static const std::array<SourceType, 2> source_types = {{OPENCL_C, SPIRV}};
  return source_types;
}
const std::array<SourceType, 2> &kts::ucl::getOfflineSourceTypes() {
  static const std::array<SourceType, 2> source_types = {
      {OFFLINE, OFFLINESPIRV}};
  return source_types;
}

UCL_EXECUTION_TEST_SUITE(Execution, testing::ValuesIn(getSourceTypes()))
UCL_EXECUTION_TEST_SUITE(ExecutionOnline,
                         testing::ValuesIn(getOnlineSourceTypes()))
UCL_EXECUTION_TEST_SUITE(ExecutionOpenCLC, testing::Values(OPENCL_C, OFFLINE))
UCL_EXECUTION_TEST_SUITE(ExecutionSPIRV, testing::Values(SPIRV, OFFLINESPIRV))

kts::ucl::BaseExecution::BaseExecution()
    : SharedExecution(),
      program_(nullptr),
      kernel_(nullptr),
      args_(new kts::ucl::ArgumentList()),
      source_type(OPENCL_C),
      fail_if_not_vectorized_(true) {
  if (UCL::isInterceptLayerPresent() &&
      UCL::isInterceptLayerControlEnabled("InjectProgramBinaries")) {
    fail_if_not_vectorized_ = false;
  }
}

kts::ucl::ArgumentList &kts::ucl::BaseExecution::GetArgumentList() {
  return *args_;
}

bool kts::ucl::BaseExecution::isSourceTypeIn(
    std::initializer_list<SourceType> source_types) {
  return std::find(source_types.begin(), source_types.end(), source_type) !=
         source_types.end();
}

void kts::ucl::BaseExecution::SetUp() {
  UCL_RETURN_ON_FATAL_FAILURE(::ucl::CommandQueueTest::SetUp());
  switch (source_type) {
    case OPENCL_C:
    case SPIRV:
      break;
    case OFFLINE:
    case OFFLINESPIRV:
      fail_if_not_vectorized_ = false;
  }
  if (isDeviceExtensionSupported("cl_codeplay_wfv")) {
    clGetKernelWFVInfoCODEPLAY =
        reinterpret_cast<clGetKernelWFVInfoCODEPLAY_fn>(
            clGetExtensionFunctionAddressForPlatform(
                platform, "clGetKernelWFVInfoCODEPLAY"));
  }
  if (source_type == SPIRV && isDeviceExtensionSupported("cl_khr_il_program")) {
    clCreateProgramWithILKHR = reinterpret_cast<clCreateProgramWithILKHR_fn>(
        clGetExtensionFunctionAddressForPlatform(platform,
                                                 "clCreateProgramWithILKHR"));
  }
}

void kts::ucl::BaseExecution::TearDown() {
  if (getEnvironment()->GetDoVectorizerCheck() && kernel_) {
    CheckVectorized();
  }

  if (kernel_ && CL_SUCCESS != clReleaseKernel(kernel_)) {
    FAIL() << "clReleaseKernel failed";
  }
  if (program_ && CL_SUCCESS != clReleaseProgram(program_)) {
    FAIL() << "clReleaseProgram failed";
  }
  ::ucl::CommandQueueTest::TearDown();
}

bool kts::ucl::BaseExecution::LoadSource(const std::string &path) {
  FILE *f = fopen(path.c_str(), "rb");
  if (!f) {
    return false;
  }
  if (fseek(f, 0, SEEK_END)) {
    (void)fclose(f);
    return false;
  }
  const long size = ftell(f);
  if (size < 0) {
    (void)fclose(f);
    return false;
  }
  source_.resize(size);
  if (fseek(f, 0, SEEK_SET)) {
    return false;
  }
  if (fread(source_.data(), 1, source_.size(), f) != source_.size()) {
    (void)fclose(f);
    return false;
  }
  (void)fclose(f);
  return true;
}

std::string kts::ucl::BaseExecution::GetSourcePath(
    const std::string &file_prefix, const std::string &kernel_name) {
  std::stringstream path;
  path << getEnvironment()->GetKernelDirectory();
  std::string ext;
  switch (source_type) {
    case OPENCL_C:
      ext = ".cl";
      break;
    case SPIRV:
      ext = ".spv" + std::to_string(getDeviceAddressBits());
      break;
    case OFFLINE:
    case OFFLINESPIRV:
      path << "_offline";
      ext = ".bin";
      break;
  }
  path << "/" << file_prefix << "_" << kernel_name << ext;
  return path.str();
}

bool kts::ucl::BaseExecution::BuildProgram() {
  const ::testing::UnitTest *test = ::testing::UnitTest::GetInstance();
  if (!test) {
    Fail("Could not get a reference to the current test.");
    return false;
  }
  const ::testing::TestInfo *test_info = test->current_test_info();
  if (!test_info) {
    Fail("Could not get a reference to the current test info.");
    return false;
  }

  const std::string test_name = test_info->name();
  std::string file_prefix;
  std::string kernel_name;
  if (!GetKernelPrefixAndName(test_name, file_prefix, kernel_name)) {
    return false;
  }

  // For offline testing the binaries are stored in a directory named after the
  // device they were compiled for. Previously the test name needed to start
  // with `offline`, now we can just check that the test_suite_name which will
  // be `OfflineExecution`.
  if (isSourceTypeIn({OFFLINE, OFFLINESPIRV})) {
    // Get the CL_DEVICE_NAME for the device being tested.
    size_t size;
    if (clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr, &size)) {
      Fail("Could not get the current devices CL_DEVICE_NAME info.");
    }
    std::string deviceName(size, '\0');
    if (clGetDeviceInfo(device, CL_DEVICE_NAME, size, deviceName.data(),
                        nullptr)) {
      Fail("Could not get the current devices CL_DEVICE_NAME info.");
    }

    // Strip trailing null terminators
    deviceName.resize(deviceName.find_first_of('\0'));

    std::string sub_dir;

    if (source_type == OFFLINESPIRV) {
      sub_dir = "spirv";
    }

    // Prepend the directory to file_prefix
    file_prefix = deviceName + "/" + sub_dir + "/" + file_prefix;
  }

  return BuildProgram(file_prefix, kernel_name);
}

bool kts::ucl::BaseExecution::BuildProgram(std::string file_prefix,
                                           std::string kernel_name) {
  cl_int err = CL_SUCCESS;

  const bool offline = source_type == OFFLINE || source_type == OFFLINESPIRV;
  const bool spirv = source_type == SPIRV;

  // We take the default case if the source_type is anything other than SPIRV.
  // if (source_type != spirv) {

  // Skip offline tests for non-ComputeAorta implementations, except it is hard
  // to know if we're running against a ComputeAorta implementation so just
  // blacklist explicit implementations.
  if (offline &&
      (UCL::isDevice_IntelNeo(device) || UCL::isDevice_Oclgrind(device))) {
    printf(
        "ComputeAorta binary kernels incompatible with other OpenCL "
        "implementations, skipping.\n");
    return false;
  }

  if (!offline && !UCL::hasCompilerSupport(device)) {
    printf("Compiler is not available. Skipping.\n");
    return false;
  }

  if (spirv && !isDeviceExtensionSupported("cl_khr_il_program")) {
    printf("Extension cl_khr_il_program not supported, skipping\n");
    return false;
  }

#ifndef CA_CL_ENABLE_OFFLINE_KERNEL_TESTS
  if (offline) {
    printf("Offline kernels not built. Skipping.\n");
    return false;
  }
#endif

#ifndef UNITCL_SPIRV_ENABLED
  if (source_type == OFFLINESPIRV) {
    printf(
        "spirv-as was not found during CMake configuration, so offline SPIR-V "
        "tests are disabled. Skipping.\n");
    return false;
  }
#endif

  // Load the kernel source if this is the first call to BuildProgram().
  if (source_.empty()) {
    const std::string path = GetSourcePath(file_prefix, kernel_name);
    if (!LoadSource(path)) {
      // If loading the source failed try again one directory up, this fixes an
      // issue with relative paths when using the Visual Studio generator since
      // the executables might be in another folder.
      if (!LoadSource("../" + path)) {
        Fail("Could not load kernel source file '" + path + "'.");
        return false;
      }
    }
  }

  // Build the program if this is the first call to BuildProgram().
  if (!program_) {
    const char *source_data = source_.data();

    if (offline) {
      const size_t lengths = source_.size();
      program_ = clCreateProgramWithBinary(
          context, 1, &device, &lengths,
          reinterpret_cast<const unsigned char **>(&source_data), nullptr,
          &err);
    } else if (spirv) {
      program_ = clCreateProgramWithILKHR(context, source_.data(),
                                          source_.size(), &err);
    } else {
      program_ =
          clCreateProgramWithSource(context, 1, &source_data, nullptr, &err);
    }

    if (CL_SUCCESS != err) {
      if (fail_if_build_program_failed) {
        Fail("Could not create OpenCL program.", err);
      }
      return false;
    }

    std::stringstream options;
    if (!offline) {
      // Add macro definitions to the option string.
      for (const MacroDef &macro : macros_) {
        options << "-D" << macro.first << "=" << macro.second << " ";
      }

      // User options set from the command line
      options << getEnvironment()->GetKernelBuildOptions();
      // Options set in individual test
      if (!build_options_.empty()) {
        options << " " << build_options_;
      }
    }

    if (CL_SUCCESS != clBuildProgram(program_, 1, &device,
                                     offline ? nullptr : options.str().c_str(),
                                     nullptr, nullptr)) {
      size_t build_log_size = 0;
      std::string build_log;
      err = clGetProgramBuildInfo(program_, device, CL_PROGRAM_BUILD_LOG, 0,
                                  nullptr, &build_log_size);
      if (CL_SUCCESS != err) {
        Fail("Requesting the build log size failed");
        return false;
      }
      build_log.resize(build_log_size);
      err = clGetProgramBuildInfo(program_, device, CL_PROGRAM_BUILD_LOG,
                                  build_log_size, (void *)build_log.data(),
                                  nullptr);
      if (CL_SUCCESS != err) {
        Fail("Requesting the build log failed");
        return false;
      }

      if (fail_if_build_program_failed) {
        Fail("clBuildProgram failed. Build log:\n\n" + build_log);
      }
      return false;
    }
  }

  // Create the kernel if this is the first call to BuildProgram().
  if (!kernel_) {
    kernel_ = clCreateKernel(program_, kernel_name.c_str(), &err);
    if (CL_SUCCESS != err) {
      if (fail_if_build_program_failed) {
        Fail("Could not create OpenCL kernel '" + kernel_name + "'.");
      }
      return false;
    }
  }

  return true;
}

/// @brief Add a flag to the options passed when the program is built.
///
/// @param option String containing build option(s).
void kts::ucl::BaseExecution::AddBuildOption(std::string option) {
  build_options_ = build_options_ + " " + option;
}

/// @brief Define a macro that can be used inside the OpenCL kernel.
/// @param[in] name - Name of the macro.
/// @param[in] value - Value of the macro.
void kts::ucl::BaseExecution::AddMacro(std::string name, unsigned value) {
  std::stringstream ss;
  ss << value;
  AddMacro(name, ss.str());
}

/// @brief Define a macro that can be used inside the OpenCL kernel.
/// @param[in] name - Name of the macro.
/// @param[in] value - Value of the macro.
void kts::ucl::BaseExecution::AddMacro(const std::string &name,
                                       const std::string &value) {
  macros_.push_back(std::make_pair(name, value));
}

void kts::ucl::BaseExecution::AddInputBuffer(BufferDesc &&desc) {
  args_->AddInputBuffer(desc);
}

void kts::ucl::BaseExecution::AddOutputBuffer(BufferDesc &&desc) {
  args_->AddOutputBuffer(desc);
}

void kts::ucl::BaseExecution::AddInOutBuffer(BufferDesc &&desc) {
  args_->AddInOutBuffer(desc);
}

void kts::ucl::BaseExecution::AddLocalBuffer(size_t nelm, size_t elmsize) {
  assert(elmsize != 0 && "cannot allocate zero-sized elements");
  const size_t bytesize = nelm * elmsize;
  assert(bytesize / elmsize == nelm && "overflow in size computation");
  // UnitCL AddLocalBuffer requires this to be allocated with cargo::alloc.
  void *raw = cargo::alloc(sizeof(PointerPrimitive), alignof(PointerPrimitive));
  PointerPrimitive *pointer_primitive = new (raw) PointerPrimitive(bytesize);
  args_->AddLocalBuffer(pointer_primitive);
}

void kts::ucl::BaseExecution::AddSampler(cl_bool normalized_coords,
                                         cl_addressing_mode addressing_mode,
                                         cl_filter_mode filter_mode) {
  args_->AddSampler(normalized_coords, addressing_mode, filter_mode);
}

void kts::ucl::BaseExecution::RunGenericND(cl_uint numDims,
                                           const size_t *globalDims,
                                           const size_t *localDims) {
  cl_int err = CL_SUCCESS;
  if (!kernel_ && !BuildProgram()) {
    GTEST_SKIP();
  }

  if (localDims) {
    size_t totalWorkGroupSize = localDims[0];
    for (cl_uint i = 1; i < numDims; i++) {
      totalWorkGroupSize = totalWorkGroupSize * localDims[i];
    }
    const size_t maxWorkGroupSize = getDeviceMaxWorkGroupSize();
    if (totalWorkGroupSize > maxWorkGroupSize) {
      // Skip test as the max work group size requested is too small for this
      // device
      printf(
          "Work group size of %u not supported on this device (%u is max "
          "allowed), skipping test.\n",
          static_cast<unsigned int>(totalWorkGroupSize),
          static_cast<unsigned int>(maxWorkGroupSize));
      GTEST_SKIP();
    }

    // Check individual max work item sizes are not less than the requested
    // work sizes. This logic could be moved inside the Environment class in
    // the future but it is only required here
    size_t paramValueSizeRet = 0;
    if (clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, 0, nullptr,
                        &paramValueSizeRet) != CL_SUCCESS) {
      Fail(
          "clGetDeviceInfo returned an error on CL_DEVICE_MAX_WORK_ITEM_SIZES");
    }

    const std::size_t elem_count = paramValueSizeRet / sizeof(std::size_t);
    std::vector<std::size_t> maxValues(elem_count, 0);
    if (clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES,
                        paramValueSizeRet, maxValues.data(),
                        nullptr) != CL_SUCCESS) {
      Fail(
          "clGetDeviceInfo returned an error on "
          "CL_DEVICE_MAX_WORK_ITEM_SIZES");
    }
    for (cl_uint i = 0; i < numDims; i++) {
      if (maxValues[i] < localDims[i]) {
        printf(
            "Work group dimension %u size of %u not supported on this device "
            "(%u is max allowed), "
            "skipping test.\n",
            i, static_cast<unsigned int>(localDims[i]),
            static_cast<unsigned int>(maxValues[i]));
        GTEST_SKIP();
      }
    }
  }

  // Consume the arguments, so that the user can call RunGeneric1D multiple
  // times with different arguments.
  std::unique_ptr<ArgumentList> args(std::move(args_));
  args_.reset(new ArgumentList());

  // Prepare input data and buffers.
  for (unsigned i = 0; i < args->GetCount(); i++) {
    kts::ucl::Argument *arg = args->GetArg(i);
    assert(arg && "not sure this should really happen");

    if ((arg->GetKind() == kts::eInputBuffer) ||
        (arg->GetKind() == kts::eOutputBuffer) ||
        (arg->GetKind() == kts::eInOutBuffer)) {
      const kts::BufferDesc desc = args->GetBufferDescForArg(i);
      if (!desc.size_) {
        Fail("Empty buffer arguments are not supported");
        return;
      } else if (!desc.streamer_) {
        Fail("Could not get a streamer for the buffer argument");
        return;
      }

      desc.streamer_->PopulateBuffer(*arg, desc);

      cl_mem buffer = arg->GetBuffer();
      if (!buffer) {
        const cl_mem_flags flags = CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR;
        buffer = clCreateBuffer(context, flags, arg->GetBufferStorageSize(),
                                arg->GetBufferStoragePtr(), &err);
        if (!buffer || (CL_SUCCESS != err)) {
          Fail("Could not create buffer", err);
          return;
        }
        arg->SetBuffer(buffer);
      } else {
        const cl_int err = clEnqueueWriteBuffer(
            command_queue, buffer, CL_TRUE, 0, arg->GetBufferStorageSize(),
            arg->GetBufferStoragePtr(), 0, nullptr, nullptr);
        if (CL_SUCCESS != err) {
          Fail("Could not write to buffer", err);
          return;
        }
      }
      clSetKernelArg(kernel_, i, sizeof(cl_mem), static_cast<void *>(&buffer));
    } else if (arg->GetKind() == kts::ePrimitive) {
      kts::Primitive *primitive = arg->GetPrimitive();
      clSetKernelArg(kernel_, i, primitive->GetSize(), primitive->GetAddress());
    } else if (arg->GetKind() == kts::eSampler) {
      const auto &sampler_desc = arg->GetSamplerDesc();
      cl_sampler sampler = clCreateSampler(
          context, sampler_desc.normalized_coords_,
          sampler_desc.addressing_mode_, sampler_desc.filter_mode_, &err);
      if (CL_SUCCESS != err) {
        Fail("Could not create sampler", err);
        return;
      }
      clSetKernelArg(kernel_, i, sizeof(cl_sampler),
                     static_cast<void *>(&sampler));
      arg->SetSampler(sampler);
    } else if (arg->GetKind() == kts::eInputImage) {
      const kts::ucl::ImageDesc &image_desc = arg->GetImageDesc();
      const cl_mem_flags flags = CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR;
      const kts::BufferDesc buffer_desc = args->GetBufferDescForArg(i);
      // Todo : Check that the size is big enough for the image description
      if (!buffer_desc.size_) {
        Fail("Empty buffer arguments are not supported");
        return;
      } else if (!buffer_desc.streamer_) {
        Fail("Could not get a streamer for the buffer argument");
        return;
      }
      buffer_desc.streamer_->PopulateBuffer(*arg, buffer_desc);
      cl_mem image =
          clCreateImage(context, flags, &image_desc.format_, &image_desc.desc_,
                        arg->GetBufferStoragePtr(), &err);
      if (!image || (CL_SUCCESS != err)) {
        Fail("Could not create image", err);
        return;
      }
      arg->SetBuffer(image);
      clSetKernelArg(kernel_, i, sizeof(cl_mem), static_cast<void *>(&image));
    }
  }

  if (!UCL::hasLocalWorkSizeSupport(device, numDims, localDims)) {
    // If you've hit this message with a test then this means that the test
    // should be modified to do the same check as here, but just skip the test.
    Fail("Running a kernel with unsupported dimensions, test is wrong");
    return;
  }

  // Enqueue kernel
  err = clEnqueueNDRangeKernel(command_queue, kernel_, numDims, nullptr,
                               globalDims, localDims, 0, nullptr, nullptr);
  if (CL_SUCCESS != err) {
    Fail("Could not enqueue N-D range", err);
    return;
  }

  EnqueueDimensions dims;
  dims.count = numDims;

  if (globalDims != nullptr) {
    for (size_t i = 0; i < numDims; ++i) {
      dims.global.emplace_back(globalDims[i]);
    }
  }
  if (localDims != nullptr) {
    for (size_t i = 0; i < numDims; ++i) {
      dims.local.emplace_back(localDims[i]);
    }
  }

  dims_to_test_.emplace_back(dims);

  // Read and validate output data.
  for (unsigned i = 0; i < args->GetCount(); i++) {
    kts::ucl::Argument &arg = *args->GetArg(i);
    const kts::BufferDesc desc = args->GetBufferDescForArg(i);
    if ((arg.GetKind() != kts::eOutputBuffer) &&
        (arg.GetKind() != kts::eInOutBuffer)) {
      continue;
    }

    const cl_int err = clEnqueueReadBuffer(
        command_queue, arg.GetBuffer(), CL_TRUE, 0, arg.GetBufferStorageSize(),
        arg.GetBufferStoragePtr(), 0, nullptr, nullptr);
    if (CL_SUCCESS != err) {
      Fail("Could not read output buffer", err);
      return;
    }

    kts::BufferStreamer *streamer = (arg.GetKind() == kts::eOutputBuffer)
                                        ? desc.streamer_.get()
                                        : desc.streamer2_.get();
    std::vector<std::string> errors;
    if (streamer && !streamer->ValidateBuffer(arg, desc, &errors)) {
      if (errors.size() > 0) {
        std::stringstream ss;
        ss << "Invalid data when validating buffer " << i << ":";
        for (const std::string &error : errors) {
          ss << "\n" << error;
        }
        Fail(ss.str());
      } else {
        Fail("Invalid data");
      }
      return;
    }
  }

  // Reading output buffers above is blocking, but in case there are no outputs
  // (e.g. in the printf tests) we need to wait on the queue to complete.
  err = clFinish(command_queue);
  if (CL_SUCCESS != err) {
    Fail("Could not flush the execution queue", err);
    return;
  }
}

void kts::ucl::BaseExecution::RunGeneric1D(size_t globalX, size_t localX) {
  const cl_uint dim = 1;
  size_t globalDims[dim] = {globalX};
  size_t localDims[dim] = {localX};

  RunGenericND(dim, globalDims, localX ? localDims : nullptr);
}

bool kts::ucl::BaseExecution::CheckVectorized() {
  cl_int err = CL_SUCCESS;
  // Check if either the workgroup size is too small to vectorize, or if the
  // vectorizeable dimension is too small to vectorize (e.g. a workgroup size
  // of {1, N, 1} may be large, but is not vectorizable.
  const size_t max_work_group_size = getDeviceMaxWorkGroupSize();
  const size_t max_work_item_size_x = getDeviceMaxWorkItemSizes()[0];
  if ((max_work_group_size < 2) || (max_work_item_size_x < 2)) {
    return true;
  } else if (isDeviceExtensionSupported("cl_codeplay_wfv")) {
    for (const auto &dims : dims_to_test_) {
      const auto global = dims.global.empty() ? nullptr : dims.global.data();
      const auto local = dims.local.empty() ? nullptr : dims.local.data();

      cl_kernel_wfv_status_codeplay status;
      err = clGetKernelWFVInfoCODEPLAY(kernel_, device, dims.count, global,
                                       local, CL_KERNEL_WFV_STATUS_CODEPLAY,
                                       sizeof(status), &status, nullptr);
      if (err != CL_SUCCESS) {
        Fail("Error while querying whole function vectorization status", err);
        return false;
      }
      if (fail_if_not_vectorized_ && status != CL_WFV_SUCCESS_CODEPLAY) {
        std::stringstream ss;
        ss << "The kernel was not vectorized (global=";
        if (dims.global.empty()) {
          ss << "unspecified";
        } else {
          auto it = dims.global.begin();
          ss << *it;
          for (++it; it != dims.global.end(); ++it) {
            ss << ',' << *it;
          }
        }
        ss << ",local=";
        if (dims.local.empty()) {
          ss << "unspecified";
        } else {
          auto it = dims.local.begin();
          ss << *it;
          for (++it; it != dims.local.end(); ++it) {
            ss << ',' << *it;
          }
        }
        ss << ')';

        Fail(ss.str());
        return false;
      }
    }
    // Seems that the kernel is vectorized, fall through.
  } else {
    size_t preferred_multiple = 1;
    err = clGetKernelWorkGroupInfo(
        kernel_, device, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
        sizeof(size_t), &preferred_multiple, nullptr);
    if (err != CL_SUCCESS) {
      Fail("Error while querying preferred work-group size multiple", err);
      return false;
    }
    if (fail_if_not_vectorized_ && preferred_multiple < 2) {
      Fail("The kernel was not vectorized");
      return false;
    }
    // Seems that the kernel is vectorized, fall through.
    // return true;
  }
  // The kernel is vectorized.
  return true;
}
