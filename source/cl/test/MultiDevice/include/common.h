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

#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <gtest/gtest.h>

/// @brief Class to translate OpenCL error codes into readable output.
///
/// Class constructor takes int input and maps it to a description of the
/// associated OpenCL error code.
/// Class is used in test macros that build upon GTest macros.
/// Based on work by Benie.
///
class ErrcodeRetWrapper {
 public:
  ErrcodeRetWrapper(int errcode_ret) : errcode_ret_(errcode_ret) {}

  const char *description_c_str() const {
#define ERRCODE_RET_WRAPPER_CASE(ERRCODE) \
  case ERRCODE: {                         \
    return #ERRCODE;                      \
  } break

    switch (errcode_ret_) {
      ERRCODE_RET_WRAPPER_CASE(CL_SUCCESS);
      ERRCODE_RET_WRAPPER_CASE(CL_DEVICE_NOT_FOUND);
      ERRCODE_RET_WRAPPER_CASE(CL_DEVICE_NOT_AVAILABLE);
      ERRCODE_RET_WRAPPER_CASE(CL_COMPILER_NOT_AVAILABLE);
      ERRCODE_RET_WRAPPER_CASE(CL_MEM_OBJECT_ALLOCATION_FAILURE);
      ERRCODE_RET_WRAPPER_CASE(CL_OUT_OF_RESOURCES);
      ERRCODE_RET_WRAPPER_CASE(CL_OUT_OF_HOST_MEMORY);
      ERRCODE_RET_WRAPPER_CASE(CL_PROFILING_INFO_NOT_AVAILABLE);
      ERRCODE_RET_WRAPPER_CASE(CL_MEM_COPY_OVERLAP);
      ERRCODE_RET_WRAPPER_CASE(CL_IMAGE_FORMAT_MISMATCH);
      ERRCODE_RET_WRAPPER_CASE(CL_IMAGE_FORMAT_NOT_SUPPORTED);
      ERRCODE_RET_WRAPPER_CASE(CL_BUILD_PROGRAM_FAILURE);
      ERRCODE_RET_WRAPPER_CASE(CL_MAP_FAILURE);
      ERRCODE_RET_WRAPPER_CASE(CL_MISALIGNED_SUB_BUFFER_OFFSET);
      ERRCODE_RET_WRAPPER_CASE(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
      ERRCODE_RET_WRAPPER_CASE(CL_COMPILE_PROGRAM_FAILURE);
      ERRCODE_RET_WRAPPER_CASE(CL_LINKER_NOT_AVAILABLE);
      ERRCODE_RET_WRAPPER_CASE(CL_LINK_PROGRAM_FAILURE);
      ERRCODE_RET_WRAPPER_CASE(CL_DEVICE_PARTITION_FAILED);
      ERRCODE_RET_WRAPPER_CASE(CL_KERNEL_ARG_INFO_NOT_AVAILABLE);

      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_VALUE);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_DEVICE_TYPE);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_PLATFORM);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_DEVICE);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_CONTEXT);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_QUEUE_PROPERTIES);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_COMMAND_QUEUE);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_HOST_PTR);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_MEM_OBJECT);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_IMAGE_SIZE);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_SAMPLER);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_BINARY);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_BUILD_OPTIONS);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_PROGRAM);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_PROGRAM_EXECUTABLE);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_KERNEL_NAME);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_KERNEL_DEFINITION);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_KERNEL);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_ARG_INDEX);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_ARG_VALUE);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_ARG_SIZE);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_KERNEL_ARGS);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_WORK_DIMENSION);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_WORK_GROUP_SIZE);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_WORK_ITEM_SIZE);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_GLOBAL_OFFSET);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_EVENT_WAIT_LIST);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_EVENT);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_OPERATION);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_GL_OBJECT);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_BUFFER_SIZE);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_MIP_LEVEL);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_GLOBAL_WORK_SIZE);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_PROPERTY);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_IMAGE_DESCRIPTOR);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_COMPILER_OPTIONS);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_LINKER_OPTIONS);
      ERRCODE_RET_WRAPPER_CASE(CL_INVALID_DEVICE_PARTITION_COUNT);
      ERRCODE_RET_WRAPPER_CASE(CL_PLATFORM_NOT_FOUND_KHR);
      default: {
        std::stringstream strstr;
        strstr.str("");
        strstr << "Unknown error code: ";
        strstr << errcode_ret_;
        // Static member variable is prone to threading race conditions but is
        // mainly used to flag unknown error codes and allows this to be a
        // simple C string. As unknown error code problems should be fixed
        // immediately, the chance for a race shouldn't occur regularly.
        static const std::string str = strstr.str();
        return str.c_str();
      } break;
    }

#undef ERRCODE_RET_WRAPPER_CASE
  }

  bool operator==(const ErrcodeRetWrapper &other) const {
    return errcode_ret_ == other.errcode_ret_;
  }

  int errcode_ret_;
};

inline std::ostream &operator<<(std::ostream &os,
                                const ErrcodeRetWrapper &errcode_ret_wrapper) {
  return os << errcode_ret_wrapper.description_c_str();
}

#ifndef ASSERT_EQ_ERRCODE
#define ASSERT_EQ_ERRCODE(val1, val2) \
  ASSERT_EQ(ErrcodeRetWrapper(val1), ErrcodeRetWrapper(val2))
#endif

#ifndef EXPECT_EQ_ERRCODE
#define EXPECT_EQ_ERRCODE(val1, val2) \
  EXPECT_EQ(ErrcodeRetWrapper(val1), ErrcodeRetWrapper(val2))
#endif

cl_platform_id getPlatform();

class MultiDeviceContext : public ::testing::Test {
 public:
  MultiDeviceContext() : platform(nullptr), devices(), context(nullptr) {}

  virtual void SetUp() override {
    platform = getPlatform();
    ASSERT_NE(nullptr, platform);
    cl_uint count;
    ASSERT_EQ_ERRCODE(CL_SUCCESS, clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL,
                                                 0, nullptr, &count));
    devices.resize(count);
    ASSERT_EQ_ERRCODE(CL_SUCCESS,
                      clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, count,
                                     devices.data(), nullptr));
    cl_int error;
    context = clCreateContext(nullptr, count, devices.data(), nullptr, nullptr,
                              &error);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, error);
  }

  virtual void TearDown() override {
    EXPECT_EQ(CL_SUCCESS, clReleaseContext(context));
  }

  /// @brief Checks if all devices in the context support images.
  ///
  /// @return Returns true if all devices support images, false otherwise.
  bool hasImageSupport() {
    for (auto device : devices) {
      cl_bool deviceSupported;
      if (clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool),
                          &deviceSupported, nullptr)) {
        return false;
      }
      if (!deviceSupported) {
        return false;
      }
    }
    return true;
  }

  /// @brief Checks if all devices in the context have a compiler.
  ///
  /// @return Returns true if all devices have a compiler, false otherwise.
  bool hasCompilerSupport() {
    for (auto device : devices) {
      cl_bool deviceSupported;
      if (CL_SUCCESS != clGetDeviceInfo(device, CL_DEVICE_COMPILER_AVAILABLE,
                                        sizeof(cl_bool), &deviceSupported,
                                        nullptr)) {
        return false;
      }
      if (!deviceSupported) {
        return false;
      }
    }
    return true;
  }

  cl_platform_id platform;
  std::vector<cl_device_id> devices;
  cl_context context;
};

class MultiDeviceCommandQueue : public MultiDeviceContext {
 public:
  MultiDeviceCommandQueue() {}

  virtual void SetUp() override {
    MultiDeviceContext::SetUp();
    cl_int error;
    for (auto device : devices) {
      command_queues.push_back(
          clCreateCommandQueue(context, device, 0, &error));
      ASSERT_EQ_ERRCODE(CL_SUCCESS, error);
    }
  }

  virtual void TearDown() override {
    for (auto command_queue : command_queues) {
      EXPECT_EQ(CL_SUCCESS, clReleaseCommandQueue(command_queue));
    }
    MultiDeviceContext::TearDown();
  }

  std::vector<cl_command_queue> command_queues;
};
