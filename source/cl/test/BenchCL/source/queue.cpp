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

#include <BenchCL/environment.h>
#include <BenchCL/error.h>
#include <CL/cl.h>
#include <benchmark/benchmark.h>

#include <string>
#include <thread>

struct CreateData {
  enum { BUFFER_LENGTH = 16384, BUFFER_SIZE = BUFFER_LENGTH * sizeof(cl_int) };

  cl_platform_id platform;
  cl_device_id device;
  cl_context context;
  cl_program program;
  cl_kernel kernel;
  cl_mem out;
  cl_mem in;
  cl_command_queue queue;

  CreateData() {
    platform = benchcl::env::get()->platform;
    device = benchcl::env::get()->device;

    cl_int status = CL_SUCCESS;
    context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

    const char *str =
        "kernel void func(global int* o, global int* i) {\n"
        "  o[get_global_id(0)] = i[get_global_id(0)];\n"
        "}\n";

    program = clCreateProgramWithSource(context, 1, &str, nullptr, &status);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

    ASSERT_EQ_ERRCODE(CL_SUCCESS, clBuildProgram(program, 0, nullptr, nullptr,
                                                 nullptr, nullptr));

    kernel = clCreateKernel(program, "func", &status);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

    out = clCreateBuffer(context, CL_MEM_WRITE_ONLY, BUFFER_SIZE, nullptr,
                         &status);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

    in = clCreateBuffer(context, CL_MEM_READ_ONLY, BUFFER_SIZE, nullptr,
                        &status);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

    ASSERT_EQ_ERRCODE(CL_SUCCESS, clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                                 static_cast<void *>(&out)));
    ASSERT_EQ_ERRCODE(CL_SUCCESS, clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                                 static_cast<void *>(&in)));

    queue = clCreateCommandQueue(context, device, 0, &status);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, status);
  }

  ~CreateData() {
    ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseCommandQueue(queue));
    ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseMemObject(in));
    ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseMemObject(out));
    ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseKernel(kernel));
    ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseProgram(program));
    ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseContext(context));
  }
};

static void SingleThreadOneQueueNoDependencies(benchmark::State &state) {
  const CreateData cd;

  for (auto _ : state) {
    (void)_;
    for (unsigned i = 0; i < state.range(0); i++) {
      const size_t size = CreateData::BUFFER_LENGTH;
      clEnqueueNDRangeKernel(cd.queue, cd.kernel, 1, nullptr, &size, nullptr, 0,
                             nullptr, nullptr);
    }

    clFinish(cd.queue);
  }

  state.SetItemsProcessed(state.range(0));
}
BENCHMARK(SingleThreadOneQueueNoDependencies)->Arg(1)->Arg(256)->Arg(1024);

static void SingleThreadOneQueue(benchmark::State &state) {
  const CreateData cd;

  for (auto _ : state) {
    (void)_;
    const size_t size = CreateData::BUFFER_LENGTH;

    cl_event event;
    clEnqueueNDRangeKernel(cd.queue, cd.kernel, 1, nullptr, &size, nullptr, 0,
                           nullptr, &event);

    for (unsigned i = 1; i < state.range(0); i++) {
      clEnqueueNDRangeKernel(cd.queue, cd.kernel, 1, nullptr, &size, nullptr, 1,
                             &event, &event);
    }

    clFinish(cd.queue);

    clReleaseEvent(event);
  }

  state.SetItemsProcessed(state.range(0));
}
BENCHMARK(SingleThreadOneQueue)->Arg(1)->Arg(256)->Arg(1024);

static void MultiThreadOneQueueNoDependencies(benchmark::State &state) {
  const CreateData cd;

  for (auto _ : state) {
    (void)_;
    for (unsigned i = 0; i < state.range(0); i++) {
      const size_t size = CreateData::BUFFER_LENGTH;
      clEnqueueNDRangeKernel(cd.queue, cd.kernel, 1, nullptr, &size, nullptr, 0,
                             nullptr, nullptr);
    }

    clFinish(cd.queue);
  }

  state.SetItemsProcessed(std::thread::hardware_concurrency() * state.range(0));
}
BENCHMARK(MultiThreadOneQueueNoDependencies)
    ->Arg(1)
    ->Arg(256)
    ->Arg(1024)
    ->Threads(std::thread::hardware_concurrency());

static void MultiThreadOneQueue(benchmark::State &state) {
  const CreateData cd;

  for (auto _ : state) {
    (void)_;
    const size_t size = CreateData::BUFFER_LENGTH;

    cl_event event;
    clEnqueueNDRangeKernel(cd.queue, cd.kernel, 1, nullptr, &size, nullptr, 0,
                           nullptr, &event);

    for (unsigned i = 1; i < state.range(0); i++) {
      clEnqueueNDRangeKernel(cd.queue, cd.kernel, 1, nullptr, &size, nullptr, 1,
                             &event, &event);
    }

    clFinish(cd.queue);

    clReleaseEvent(event);
  }

  state.SetItemsProcessed(std::thread::hardware_concurrency() * state.range(0));
}
BENCHMARK(MultiThreadOneQueue)
    ->Arg(1)
    ->Arg(256)
    ->Arg(1024)
    ->Threads(std::thread::hardware_concurrency());

static void MultiThreadMultiQueueNoDependencies(benchmark::State &state) {
  const CreateData cd;

  cl_command_queue queue;

  if (0 == state.thread_index) {
    queue = cd.queue;
  } else {
    queue = clCreateCommandQueue(cd.context, cd.device, 0, nullptr);
  }

  for (auto _ : state) {
    (void)_;
    for (unsigned i = 0; i < state.range(0); i++) {
      const size_t size = CreateData::BUFFER_LENGTH;
      clEnqueueNDRangeKernel(queue, cd.kernel, 1, nullptr, &size, nullptr, 0,
                             nullptr, nullptr);
    }

    clFinish(queue);
  }

  if (0 != state.thread_index) {
    clReleaseCommandQueue(queue);
  }

  state.SetItemsProcessed(std::thread::hardware_concurrency() * state.range(0));
}
BENCHMARK(MultiThreadMultiQueueNoDependencies)
    ->Arg(1)
    ->Arg(256)
    ->Arg(1024)
    ->Threads(std::thread::hardware_concurrency());

static void MultiThreadMultiQueue(benchmark::State &state) {
  const CreateData cd;

  cl_command_queue queue;

  if (0 == state.thread_index) {
    queue = cd.queue;
  } else {
    queue = clCreateCommandQueue(cd.context, cd.device, 0, nullptr);
  }

  for (auto _ : state) {
    (void)_;
    const size_t size = CreateData::BUFFER_LENGTH;

    cl_event event;
    clEnqueueNDRangeKernel(queue, cd.kernel, 1, nullptr, &size, nullptr, 0,
                           nullptr, &event);

    for (unsigned i = 1; i < state.range(0); i++) {
      clEnqueueNDRangeKernel(queue, cd.kernel, 1, nullptr, &size, nullptr, 1,
                             &event, &event);
    }

    clFinish(queue);

    clReleaseEvent(event);
  }

  if (0 != state.thread_index) {
    clReleaseCommandQueue(queue);
  }

  state.SetItemsProcessed(std::thread::hardware_concurrency() * state.range(0));
}
BENCHMARK(MultiThreadMultiQueue)
    ->Arg(1)
    ->Arg(256)
    ->Arg(1024)
    ->Threads(std::thread::hardware_concurrency());
