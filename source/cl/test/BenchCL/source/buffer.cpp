// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <BenchCL/error.h>
#include <BenchCL/environment.h>
#include <CL/cl.h>
#include <benchmark/benchmark.h>
#include <vector>

void BufferReadRect(benchmark::State& state) {

  auto device = benchcl::env::get()->device;
  auto status = CL_SUCCESS;

  auto ctx = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

  auto qu = clCreateCommandQueue(ctx, device, 0, &status);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, status);
  
  size_t arg_size = static_cast<size_t>(state.range(0));
  size_t count = arg_size * arg_size * arg_size;
  
  auto buf_mem = std::vector<char>(count);
  auto host_mem = std::vector<char>(count);

  auto buffer = clCreateBuffer(ctx, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               count, buf_mem.data(), &status);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

  size_t origin[3] = {};
  size_t region[3] = {arg_size, arg_size, arg_size};

  size_t pitch = arg_size;
  size_t slice = arg_size * arg_size;

  for (auto _ : state) {
    clEnqueueReadBufferRect(qu, buffer, CL_FALSE, origin, origin, region, pitch,
                            slice, pitch, slice, host_mem.data(), 0, NULL,
                            NULL);

    ASSERT_EQ_ERRCODE(CL_SUCCESS, clFinish(qu));
  }

}
BENCHMARK(BufferReadRect)->Arg(1)->Arg(256)->Arg(512);

void BufferWriteRect(benchmark::State& state) {

  auto device = benchcl::env::get()->device;
  auto status = CL_SUCCESS;

  auto ctx = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

  auto qu = clCreateCommandQueue(ctx, device, 0, &status);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

  size_t arg_size = static_cast<size_t>(state.range(0));
  size_t count = arg_size * arg_size * arg_size;
  
  auto buf_mem = std::vector<char>(count);
  auto host_mem = std::vector<char>(count);

  auto buffer = clCreateBuffer(ctx, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               count, buf_mem.data(), &status);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

  size_t origin[3] = {};
  size_t region[3] = {arg_size, arg_size, arg_size};

  size_t pitch = arg_size;
  size_t slice = arg_size * arg_size;

  for (auto _ : state) {
    clEnqueueWriteBufferRect(qu, buffer, CL_FALSE, origin, origin, region,
                             pitch, slice, pitch, slice, host_mem.data(), 0,
                             NULL, NULL);

    ASSERT_EQ_ERRCODE(CL_SUCCESS, clFinish(qu));
  }

}
BENCHMARK(BufferWriteRect)->Arg(1)->Arg(256)->Arg(512);

