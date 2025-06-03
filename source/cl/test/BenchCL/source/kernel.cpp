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

#include <chrono>
#include <string>
#include <vector>

struct CreateData {
  cl_platform_id platform;
  cl_device_id device;
  cl_context context;
  cl_program program;

  CreateData(cl_platform_id plat, cl_device_id dev, cl_context con,
             cl_program prog)
      : platform(plat), device(dev), context(con), program(prog) {}

  ~CreateData() {
    ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseProgram(program));
    ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseContext(context));
  }
};

static CreateData create_data_from_source(const std::string &source) {
  cl_platform_id platform;
  cl_device_id device;
  cl_context context;
  cl_program program;

  platform = benchcl::env::get()->platform;
  device = benchcl::env::get()->device;

  cl_int status = CL_SUCCESS;
  context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

  const char *str = source.c_str();

  program = clCreateProgramWithSource(context, 1, &str, nullptr, &status);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

  ASSERT_EQ_ERRCODE(CL_SUCCESS, clBuildProgram(program, 0, nullptr, nullptr,
                                               nullptr, nullptr));

  return CreateData{platform, device, context, program};
}

static void KernelCreateFirstKernelInSource(benchmark::State &state) {
  std::string source;

  for (unsigned i = 0; i < state.range(0); i++) {
    source += "kernel void func" + std::to_string(i) + "() {}\n";
  }

  const CreateData cd = create_data_from_source(source);

  const std::string name = "func" + std::to_string(0);

  for (auto _ : state) {
    (void)_;
    cl_kernel kernel = clCreateKernel(cd.program, name.c_str(), nullptr);

    ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseKernel(kernel));
  }
}
BENCHMARK(KernelCreateFirstKernelInSource)->Arg(1)->Arg(256)->Arg(16384);

static void KernelCreateLastKernelInSource(benchmark::State &state) {
  std::string source;

  for (unsigned i = 0; i < state.range(0); i++) {
    source += "kernel void func" + std::to_string(i) + "() {}\n";
  }

  const CreateData cd = create_data_from_source(source);

  const std::string name = "func" + std::to_string(state.range(0) - 1);

  for (auto _ : state) {
    (void)_;
    cl_kernel kernel = clCreateKernel(cd.program, name.c_str(), nullptr);

    ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseKernel(kernel));
  }
}
BENCHMARK(KernelCreateLastKernelInSource)->Arg(1)->Arg(256)->Arg(16384);

static void KernelCreateWithRequiredWorkGroupSize(benchmark::State &state) {
  const std::string source =
      "kernel __attribute__((reqd_work_group_size(1, 1, 1)))"
      " void func() {}\n";
  const CreateData cd = create_data_from_source(source);

  const std::string name = "func";

  for (auto _ : state) {
    (void)_;
    cl_kernel kernel = clCreateKernel(cd.program, name.c_str(), nullptr);

    ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseKernel(kernel));
  }
}
BENCHMARK(KernelCreateWithRequiredWorkGroupSize);

static void KernelEnqueueEmpty(benchmark::State &state) {
  const std::string source = "kernel void empty() {}";
  const CreateData cd = create_data_from_source(source);

  const std::string name = "empty";

  cl_int success = CL_SUCCESS;
  cl_command_queue queue =
      clCreateCommandQueue(cd.context, cd.device, 0, &success);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, success);

  cl_kernel kernel = clCreateKernel(cd.program, name.c_str(), &success);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, success);

  const size_t global_size = 1;
  ASSERT_EQ_ERRCODE(CL_SUCCESS, clEnqueueNDRangeKernel(
                                    queue, kernel, 1, nullptr, &global_size,
                                    nullptr, 0, nullptr, nullptr));
  ASSERT_EQ_ERRCODE(CL_SUCCESS, clFinish(queue));

  for (auto _ : state) {
    (void)_;
    namespace chrono = std::chrono;
    auto start = chrono::high_resolution_clock::now();

    ASSERT_EQ_ERRCODE(CL_SUCCESS, clEnqueueNDRangeKernel(
                                      queue, kernel, 1, nullptr, &global_size,
                                      nullptr, 0, nullptr, nullptr));
    ASSERT_EQ_ERRCODE(CL_SUCCESS, clFinish(queue));

    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::duration<double>>(end - start);

    state.SetIterationTime(elapsed.count());
  }

  ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseKernel(kernel));
  ASSERT_EQ_ERRCODE(CL_SUCCESS, clFinish(queue));
  ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseCommandQueue(queue));
}
BENCHMARK(KernelEnqueueEmpty)->UseManualTime();

static void KernelTiledEnqueue(benchmark::State &state) {
  const std::string source = R"CL(
    __kernel void vector_addition(__global int *src1, __global int *src2,
                                  __global int *dst) {
      size_t gid = get_global_id(0);
      dst[gid] = src1[gid] + src2[gid];
    }
  )CL";

  constexpr size_t item_count = 1 << 24;
  const size_t tile_count = state.range(0);
  assert(item_count > tile_count);

  auto err = cl_int{CL_SUCCESS};
  const CreateData cd = create_data_from_source(source);

  constexpr size_t bytes = sizeof(cl_int) * item_count;

  auto &ctx = cd.context;

  cl_mem src1_buf = clCreateBuffer(ctx, CL_MEM_READ_ONLY, bytes, nullptr, &err);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, err);

  cl_mem src2_buf = clCreateBuffer(ctx, CL_MEM_READ_ONLY, bytes, nullptr, &err);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, err);

  cl_mem dst_buf = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, bytes, nullptr, &err);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, err);

  /* Create kernel and set arguments */
  cl_kernel ker = clCreateKernel(cd.program, "vector_addition", &err);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, err);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, clSetKernelArg(ker, 0, sizeof(src1_buf),
                                               static_cast<void *>(&src1_buf)));
  ASSERT_EQ_ERRCODE(CL_SUCCESS, clSetKernelArg(ker, 1, sizeof(src2_buf),
                                               static_cast<void *>(&src2_buf)));
  ASSERT_EQ_ERRCODE(CL_SUCCESS, clSetKernelArg(ker, 2, sizeof(dst_buf),
                                               static_cast<void *>(&dst_buf)));

  /* Create command queue */
  cl_command_queue qu = clCreateCommandQueue(ctx, cd.device, 0, &err);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, err);

  /* Enqueue source buffer writes */
  std::vector<cl_int> src1(item_count);
  std::vector<cl_int> src2(item_count);

  /* set data */
  for (size_t i = 0; i < item_count; ++i) {
    src1[i] = i;
    src2[i] = i + 1;
  }

  ASSERT_EQ_ERRCODE(CL_SUCCESS,
                    clEnqueueWriteBuffer(qu, src1_buf, CL_TRUE, 0, bytes,
                                         src1.data(), 0, nullptr, nullptr));
  ASSERT_EQ_ERRCODE(CL_SUCCESS,
                    clEnqueueWriteBuffer(qu, src2_buf, CL_TRUE, 0, bytes,
                                         src2.data(), 0, nullptr, nullptr));

  /* early call to build kernel */
  const size_t work_size = item_count / tile_count;
  ASSERT_EQ_ERRCODE(CL_SUCCESS,
                    clEnqueueNDRangeKernel(qu, ker, 1, nullptr, &work_size,
                                           nullptr, 0, nullptr, nullptr));
  ASSERT_EQ_ERRCODE(CL_SUCCESS, clFinish(qu));

  for (auto _ : state) {
    (void)_;
    namespace chrono = std::chrono;
    auto start = chrono::high_resolution_clock::now();

    for (size_t i = 0; i < tile_count; ++i) {
      const size_t offset = i * work_size;
      ASSERT_EQ_ERRCODE(CL_SUCCESS,
                        clEnqueueNDRangeKernel(qu, ker, 1, &offset, &work_size,
                                               nullptr, 0, nullptr, nullptr));
    }

    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::duration<double>>(end - start);

    state.SetIterationTime(elapsed.count());

    ASSERT_EQ_ERRCODE(CL_SUCCESS, clFinish(qu));
  }

  std::vector<cl_int> dst(item_count);
  ASSERT_EQ_ERRCODE(CL_SUCCESS,
                    clEnqueueReadBuffer(qu, dst_buf, CL_TRUE, 0, bytes,
                                        dst.data(), 0, nullptr, nullptr));

  /* Check the result */
  for (size_t i = 0; i < item_count; ++i) {
    if (dst[i] != src1[i] + src2[i]) {
      /* wrong result */
      assert(false);
    }
  }

  ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseKernel(ker));
  ASSERT_EQ_ERRCODE(CL_SUCCESS, clFinish(qu));
  ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseCommandQueue(qu));
}
BENCHMARK(KernelTiledEnqueue)
    ->Args({1 << 0})
    ->Args({1 << 5})
    ->Args({1 << 13})
    ->UseManualTime();
// Nothing special about these values, just more tiles.

static void KernelCreateEmptyKernelFromSource(benchmark::State &state) {
  const std::string source = "kernel void empty() {}";
  const CreateData cd = create_data_from_source(source);

  const std::string name = "empty";

  cl_int success = CL_SUCCESS;
  cl_command_queue queue =
      clCreateCommandQueue(cd.context, cd.device, 0, &success);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, success);

  for (auto _ : state) {
    (void)_;
    cl_kernel kernel = clCreateKernel(cd.program, name.c_str(), &success);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, success);

    const size_t global_size = 1;
    ASSERT_EQ_ERRCODE(CL_SUCCESS, clEnqueueNDRangeKernel(
                                      queue, kernel, 1, nullptr, &global_size,
                                      nullptr, 0, nullptr, nullptr));

    ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseKernel(kernel));
  }

  ASSERT_EQ_ERRCODE(CL_SUCCESS, clFinish(queue));
  ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseCommandQueue(queue));
}
BENCHMARK(KernelCreateEmptyKernelFromSource);
