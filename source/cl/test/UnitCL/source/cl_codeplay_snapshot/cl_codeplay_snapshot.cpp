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

// Third party headers
#include <cstdio>
#include <cstring>
#include <vector>

// Access to the extension defines, types, etc. that are not yet added to
// CL/cl_ext.h.
#include <CL/cl_ext_codeplay.h>

struct cl_codeplay_program_snapshot_Test : ucl::ContextTest {
  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!isDeviceExtensionSupported("cl_codeplay_program_snapshot")) {
      GTEST_SKIP();
    }

    clRequestProgramSnapshotListCODEPLAY =
        reinterpret_cast<clRequestProgramSnapshotListCODEPLAY_fn>(
            clGetExtensionFunctionAddressForPlatform(
                platform, "clRequestProgramSnapshotListCODEPLAY"));
    ASSERT_NE(nullptr, clRequestProgramSnapshotListCODEPLAY);
    clRequestProgramSnapshotCODEPLAY =
        reinterpret_cast<clRequestProgramSnapshotCODEPLAY_fn>(
            clGetExtensionFunctionAddressForPlatform(
                platform, "clRequestProgramSnapshotCODEPLAY"));
    ASSERT_NE(nullptr, clRequestProgramSnapshotCODEPLAY);

    const char *source =
        "void kernel foo(global int * a, global int * b) {*a = *b;}";
    cl_int errorcode;
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    ASSERT_TRUE(nullptr != program);
    ASSERT_SUCCESS(errorcode);
  }

  void TearDown() {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  cl_program program = nullptr;
  bool callback_hit = false;

  clRequestProgramSnapshotCODEPLAY_fn clRequestProgramSnapshotCODEPLAY =
      nullptr;
  clRequestProgramSnapshotListCODEPLAY_fn clRequestProgramSnapshotListCODEPLAY =
      nullptr;
};

TEST_F(cl_codeplay_program_snapshot_Test, clRequestProgramSnapshotCODEPLAY) {
  const char *extension_fn_name = "clRequestProgramSnapshotCODEPLAY";
  void *uncast_extension_fn =
      clGetExtensionFunctionAddressForPlatform(platform, extension_fn_name);

  ASSERT_TRUE(nullptr != uncast_extension_fn);
}

TEST_F(cl_codeplay_program_snapshot_Test,
       clRequestProgramSnapshotListCODEPLAY) {
  const char *extension_fn_name = "clRequestProgramSnapshotListCODEPLAY";
  void *uncast_extension_fn =
      clGetExtensionFunctionAddressForPlatform(platform, extension_fn_name);

  ASSERT_TRUE(nullptr != uncast_extension_fn);
}

TEST_F(cl_codeplay_program_snapshot_Test, ListAllSnapshotStages) {
  std::vector<const char *> stages;
  cl_uint snapshot_stages = 0;
  cl_int retcode = clRequestProgramSnapshotListCODEPLAY(
      program, device, nullptr, &snapshot_stages);

  ASSERT_SUCCESS(retcode);
  ASSERT_GT(snapshot_stages, cl_uint(0));

  stages.resize(snapshot_stages);
  for (cl_uint i = 0; i < snapshot_stages; ++i) {
    stages[i] = nullptr;
  }

  retcode = clRequestProgramSnapshotListCODEPLAY(program, device, stages.data(),
                                                 &snapshot_stages);
  EXPECT_SUCCESS(retcode);

  for (cl_uint i = 0; i < snapshot_stages; ++i) {
    EXPECT_TRUE(nullptr != stages[i]);
  }
}

TEST_F(cl_codeplay_program_snapshot_Test, ListSingleSnapshotStage) {
  // Make sure we only set the first snapshot stage
  const char *stages[2] = {nullptr, nullptr};
  cl_uint snapshot_stages = 1;
  cl_int retcode = clRequestProgramSnapshotListCODEPLAY(program, device, stages,
                                                        &snapshot_stages);

  ASSERT_SUCCESS(retcode);
  ASSERT_TRUE(nullptr != stages[0]);
  ASSERT_TRUE(nullptr == stages[1]);
}

TEST_F(cl_codeplay_program_snapshot_Test, ListNullSnapshotSize) {
  std::vector<const char *> stages;
  cl_uint snapshot_stages = 0;
  cl_int retcode = clRequestProgramSnapshotListCODEPLAY(
      program, device, nullptr, &snapshot_stages);

  ASSERT_SUCCESS(retcode);
  ASSERT_GT(snapshot_stages, cl_uint(0));

  stages.resize(snapshot_stages);
  for (cl_uint i = 0; i < snapshot_stages; ++i) {
    stages[i] = nullptr;
  }

  // Pass in null for size of snapshot stages
  retcode = clRequestProgramSnapshotListCODEPLAY(program, device, stages.data(),
                                                 nullptr);
  EXPECT_SUCCESS(retcode);

  for (cl_uint i = 0; i < snapshot_stages; ++i) {
    EXPECT_TRUE(nullptr != stages[i]);
  }
}

TEST_F(cl_codeplay_program_snapshot_Test, ListMoreSnapshotSize) {
  std::vector<const char *> stages;
  cl_uint snapshot_stages = 0;

  cl_int retcode = clRequestProgramSnapshotListCODEPLAY(
      program, device, nullptr, &snapshot_stages);

  ASSERT_SUCCESS(retcode);
  ASSERT_GT(snapshot_stages, cl_uint(0));

  const cl_uint stages_size = snapshot_stages + 10;
  stages.resize(stages_size);
  for (cl_uint i = 0; i < stages_size; ++i) {
    stages[i] = nullptr;
  }

  // Pass in null for size of snapshot stages
  retcode = clRequestProgramSnapshotListCODEPLAY(program, device, stages.data(),
                                                 (cl_uint *)&stages_size);
  EXPECT_SUCCESS(retcode);

  for (cl_uint i = 0; i < stages_size; ++i) {
    if (i < snapshot_stages) {
      EXPECT_TRUE(nullptr != stages[i]);
    } else {
      EXPECT_TRUE(nullptr == stages[i]);
    }
  }
}

void tnex_snapshot_callback(size_t snapshot_size, const char *snapshot_data,
                            void *callback_data, void *user_data) {
  if (callback_data) {
    // Unused
  }

  ASSERT_TRUE(nullptr != snapshot_data);
  ASSERT_TRUE(0 != snapshot_size);
  ASSERT_TRUE(nullptr != user_data);

  // set callback_hit_ to true
  *((bool *)user_data) = true;
}

TEST_F(cl_codeplay_program_snapshot_Test, SetSnapshotStageNonExistant) {
  cl_int retcode = clRequestProgramSnapshotCODEPLAY(
      program, device, "nonsense_stage", CL_PROGRAM_BINARY_FORMAT_TEXT_CODEPLAY,
      &tnex_snapshot_callback, &callback_hit);
  ASSERT_EQ_ERRCODE(CL_INVALID_ARG_VALUE, retcode);
}

TEST_F(cl_codeplay_program_snapshot_Test, SetSnapshotStageFirst) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, can't snapshot.
  }
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  const char *stages[1] = {nullptr};
  cl_uint snapshot_stages = 1;
  cl_int retcode = clRequestProgramSnapshotListCODEPLAY(program, device, stages,
                                                        &snapshot_stages);

  ASSERT_SUCCESS(retcode);
  ASSERT_TRUE(nullptr != stages[0]);

  retcode = clRequestProgramSnapshotCODEPLAY(
      program, device, stages[0], CL_PROGRAM_BINARY_FORMAT_BINARY_CODEPLAY,
      &tnex_snapshot_callback, &callback_hit);
  ASSERT_SUCCESS(retcode);

  // Make sure we can still build and run kernel
  ASSERT_SUCCESS(clBuildProgram(program, 0, nullptr, "", nullptr, nullptr));

  // Make sure callback was invoked.
  ASSERT_TRUE(callback_hit);
}

TEST_F(cl_codeplay_program_snapshot_Test, SetSnapshotStageHost) {
  if (UCL::isInterceptLayerPresent()) {
    GTEST_SKIP();  // Injection creates programs from binaries, can't snapshot.
  }
  if (!getDeviceCompilerAvailable()) {
    GTEST_SKIP();
  }
  std::vector<const char *> stages;
  cl_uint snapshot_stages = 0;

  cl_int retcode = clRequestProgramSnapshotListCODEPLAY(
      program, device, nullptr, &snapshot_stages);

  ASSERT_SUCCESS(retcode);
  ASSERT_GT(snapshot_stages, cl_uint(2));

  stages.resize(snapshot_stages);
  // Pass in null for size of snapshot stages
  retcode = clRequestProgramSnapshotListCODEPLAY(program, device, stages.data(),
                                                 nullptr);
  EXPECT_SUCCESS(retcode);

  // We want to test a snapshot in our host implementation, the
  // last listed stage should correspond to 'cl_snapshot_host_scheduled'.
  // This will not be the case however if we aren't targeting core,
  // in which case fall back to the first stage.
  const char *stage_cstr = stages[snapshot_stages - 1];
  if (std::strcmp("cl_snapshot_host_scheduled", stage_cstr) != 0) {
    stage_cstr = stages[0];
  }

  retcode = clRequestProgramSnapshotCODEPLAY(
      program, device, stage_cstr, CL_PROGRAM_BINARY_FORMAT_TEXT_CODEPLAY,
      &tnex_snapshot_callback, &callback_hit);
  ASSERT_SUCCESS(retcode);

  ASSERT_SUCCESS(clBuildProgram(program, 0, nullptr, "", nullptr, nullptr));

  cl_int status;
  cl_kernel kernel = clCreateKernel(program, "foo", &status);
  ASSERT_SUCCESS(status);

  cl_mem in_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_int),
                                    nullptr, &status);
  ASSERT_TRUE(nullptr != in_buffer);
  EXPECT_SUCCESS(status);

  cl_mem out_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_int),
                                     nullptr, &status);
  ASSERT_TRUE(nullptr != out_buffer);
  EXPECT_SUCCESS(status);

  EXPECT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), &out_buffer));
  EXPECT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem), &in_buffer));

  cl_command_queue command_queue =
      clCreateCommandQueue(context, device, 0, &status);
  ASSERT_TRUE(nullptr != command_queue);
  EXPECT_SUCCESS(status);

  cl_event taskEvent;
  ASSERT_SUCCESS(clEnqueueTask(command_queue, kernel, 0, nullptr, &taskEvent));
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_SUCCESS(clReleaseEvent(taskEvent));
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseMemObject(in_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(out_buffer));
  EXPECT_SUCCESS(clReleaseCommandQueue(command_queue));

  // Make sure callback was invoked.
  ASSERT_TRUE(callback_hit);
}
