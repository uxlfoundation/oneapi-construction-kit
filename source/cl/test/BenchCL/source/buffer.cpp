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

#include <vector>

static void BufferReadRect(benchmark::State &state) {
  auto device = benchcl::env::get()->device;
  auto status = CL_SUCCESS;

  auto ctx = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

  auto qu = clCreateCommandQueue(ctx, device, 0, &status);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

  const size_t arg_size = static_cast<size_t>(state.range(0));
  const size_t count = arg_size * arg_size * arg_size;

  auto buf_mem = std::vector<char>(count);
  auto host_mem = std::vector<char>(count);

  auto buffer = clCreateBuffer(ctx, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               count, buf_mem.data(), &status);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

  size_t origin[3] = {};
  size_t region[3] = {arg_size, arg_size, arg_size};

  const size_t pitch = arg_size;
  const size_t slice = arg_size * arg_size;

  for (auto _ : state) {
    (void)_;
    clEnqueueReadBufferRect(qu, buffer, CL_FALSE, origin, origin, region, pitch,
                            slice, pitch, slice, host_mem.data(), 0, NULL,
                            NULL);

    ASSERT_EQ_ERRCODE(CL_SUCCESS, clFinish(qu));
  }
}
BENCHMARK(BufferReadRect)->Arg(1)->Arg(256)->Arg(512);

static void BufferWriteRect(benchmark::State &state) {
  auto device = benchcl::env::get()->device;
  auto status = CL_SUCCESS;

  auto ctx = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

  auto qu = clCreateCommandQueue(ctx, device, 0, &status);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

  const size_t arg_size = static_cast<size_t>(state.range(0));
  const size_t count = arg_size * arg_size * arg_size;

  auto buf_mem = std::vector<char>(count);
  auto host_mem = std::vector<char>(count);

  auto buffer = clCreateBuffer(ctx, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               count, buf_mem.data(), &status);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

  size_t origin[3] = {};
  size_t region[3] = {arg_size, arg_size, arg_size};

  const size_t pitch = arg_size;
  const size_t slice = arg_size * arg_size;

  for (auto _ : state) {
    (void)_;
    clEnqueueWriteBufferRect(qu, buffer, CL_FALSE, origin, origin, region,
                             pitch, slice, pitch, slice, host_mem.data(), 0,
                             NULL, NULL);

    ASSERT_EQ_ERRCODE(CL_SUCCESS, clFinish(qu));
  }
}
BENCHMARK(BufferWriteRect)->Arg(1)->Arg(256)->Arg(512);
