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

namespace InputType {
enum Type { NOP = 0, NOBUILTINS, MATHBUILTINS };
};

struct CreateProgramData {
  cl_platform_id platform;
  cl_device_id device;
  cl_context context;
  CreateProgramData() {
    platform = benchcl::env::get()->platform;
    device = benchcl::env::get()->device;

    cl_int status = CL_SUCCESS;
    context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, status);
  }

  ~CreateProgramData() { clReleaseContext(context); }

  template <InputType::Type TYPE>
  std::vector<const char *> generate(unsigned s);
};

template <>
std::vector<const char *> CreateProgramData::generate<InputType::NOP>(
    unsigned s) {
  std::vector<const char *> data;
  for (unsigned i = 0; i < s; i++) {
    data.push_back("");
  }
  return data;
}

template <>
std::vector<const char *> CreateProgramData::generate<InputType::NOBUILTINS>(
    unsigned s) {
  std::vector<const char *> data;
  data.push_back("void kernel foo(global int* o, global int* i) {\n");
  data.push_back("  const size_t id = get_global_id(0);\n");
  data.push_back("  o[id] = i[id];\n");
  for (unsigned i = 0; i < s; i++) {
    data.push_back("  o[id] = o[id] * i[id];\n");
  }
  data.push_back("}\n");
  return data;
}

template <>
std::vector<const char *> CreateProgramData::generate<InputType::MATHBUILTINS>(
    unsigned s) {
  std::vector<const char *> data;
  data.push_back("void kernel foo(global float* o, global float* i) {\n");
  data.push_back("  const size_t id = get_global_id(0);\n");
  data.push_back("  o[id] = i[id];\n");
  for (unsigned i = 0; i < s; i++) {
    switch (i % 4) {
      default:
        break;  // shouldn't get here!
      case 0:
        data.push_back("  o[id] = sqrt(o[id]);\n");
        break;
      case 1:
        data.push_back("  o[id] = tan(o[id]);\n");
        break;
      case 2:
        data.push_back("  o[id] = pow(o[id], i[id]);\n");
        break;
      case 3:
        data.push_back("  o[id] = clamp(o[id], 0.0f, i[id]);\n");
        break;
    }
  }
  data.push_back("}\n");
  return data;
}

template <InputType::Type TYPE>
static void CreateMultiStringProgram(benchmark::State &state) {
  CreateProgramData cpd;

  std::vector<const char *> data(cpd.generate<TYPE>(state.range(0)));

  for (auto _ : state) {
    cl_program program = clCreateProgramWithSource(
        cpd.context, data.size(), data.data(), nullptr, nullptr);

    clReleaseProgram(program);
  }
}

template <InputType::Type TYPE>
static void CompileMultiStringProgram(benchmark::State &state) {
  CreateProgramData cpd;

  std::vector<const char *> data(cpd.generate<TYPE>(state.range(0)));

  cl_program program = clCreateProgramWithSource(cpd.context, data.size(),
                                                 data.data(), nullptr, nullptr);

  for (auto _ : state) {
    clCompileProgram(program, 0, nullptr, nullptr, 0, nullptr, nullptr, nullptr,
                     nullptr);
  }

  clReleaseProgram(program);
}

template <InputType::Type TYPE>
static void LinkMultiStringProgram(benchmark::State &state) {
  CreateProgramData cpd;

  std::vector<const char *> data(cpd.generate<TYPE>(state.range(0)));

  cl_program program = clCreateProgramWithSource(cpd.context, data.size(),
                                                 data.data(), nullptr, nullptr);

  clCompileProgram(program, 0, nullptr, nullptr, 0, nullptr, nullptr, nullptr,
                   nullptr);

  for (auto _ : state) {
    cl_program linkedProgram =
        clLinkProgram(cpd.context, 0, nullptr, nullptr, 1, &program, nullptr,
                      nullptr, nullptr);

    clReleaseProgram(linkedProgram);
  }

  clReleaseProgram(program);
}

template <InputType::Type TYPE>
static void BuildMultiStringProgram(benchmark::State &state) {
  CreateProgramData cpd;

  std::vector<const char *> data(cpd.generate<TYPE>(state.range(0)));

  cl_program program = clCreateProgramWithSource(cpd.context, data.size(),
                                                 data.data(), nullptr, nullptr);

  for (auto _ : state) {
    clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr);
  }

  clReleaseProgram(program);
}

template <InputType::Type TYPE>
static void CreateSingleStringProgram(benchmark::State &state) {
  CreateProgramData cpd;

  const std::vector<const char *> data(cpd.generate<TYPE>(state.range(0)));

  std::string flattened;

  for (const char *str : data) {
    flattened += str;
  }

  const char *str = flattened.c_str();

  for (auto _ : state) {
    cl_program program =
        clCreateProgramWithSource(cpd.context, 1, &str, nullptr, nullptr);

    clReleaseProgram(program);
  }
}

template <InputType::Type TYPE>
static void CompileSingleStringProgram(benchmark::State &state) {
  CreateProgramData cpd;

  const std::vector<const char *> data(cpd.generate<TYPE>(state.range(0)));

  std::string flattened;

  for (const char *str : data) {
    flattened += str;
  }

  const char *str = flattened.c_str();

  cl_program program =
      clCreateProgramWithSource(cpd.context, 1, &str, nullptr, nullptr);

  for (auto _ : state) {
    clCompileProgram(program, 0, nullptr, nullptr, 0, nullptr, nullptr, nullptr,
                     nullptr);
  }

  clReleaseProgram(program);
}

template <InputType::Type TYPE>
static void LinkSingleStringProgram(benchmark::State &state) {
  CreateProgramData cpd;

  const std::vector<const char *> data(cpd.generate<TYPE>(state.range(0)));

  std::string flattened;

  for (const char *str : data) {
    flattened += str;
  }

  const char *str = flattened.c_str();

  cl_program program =
      clCreateProgramWithSource(cpd.context, 1, &str, nullptr, nullptr);

  clCompileProgram(program, 0, nullptr, nullptr, 0, nullptr, nullptr, nullptr,
                   nullptr);

  for (auto _ : state) {
    cl_program linkedProgram =
        clLinkProgram(cpd.context, 0, nullptr, nullptr, 1, &program, nullptr,
                      nullptr, nullptr);

    clReleaseProgram(linkedProgram);
  }

  clReleaseProgram(program);
}

template <InputType::Type TYPE>
static void BuildSingleStringProgram(benchmark::State &state) {
  CreateProgramData cpd;

  const std::vector<const char *> data(cpd.generate<TYPE>(state.range(0)));

  std::string flattened;

  for (const char *str : data) {
    flattened += str;
  }

  const char *str = flattened.c_str();

  cl_program program =
      clCreateProgramWithSource(cpd.context, 1, &str, nullptr, nullptr);

  for (auto _ : state) {
    clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr);
  }

  clReleaseProgram(program);
}

#define TEMPLATE_ARGS() Arg(1)->Arg(1024)->Arg(8192)

#define TEMPLATE_FOREACH(type)                                           \
  BENCHMARK_TEMPLATE(CreateSingleStringProgram, type)->TEMPLATE_ARGS();  \
  BENCHMARK_TEMPLATE(CompileSingleStringProgram, type)->TEMPLATE_ARGS(); \
  BENCHMARK_TEMPLATE(LinkSingleStringProgram, type)->TEMPLATE_ARGS();    \
  BENCHMARK_TEMPLATE(BuildSingleStringProgram, type)->TEMPLATE_ARGS();   \
  BENCHMARK_TEMPLATE(CreateMultiStringProgram, type)->TEMPLATE_ARGS();   \
  BENCHMARK_TEMPLATE(CompileMultiStringProgram, type)->TEMPLATE_ARGS();  \
  BENCHMARK_TEMPLATE(LinkMultiStringProgram, type)->TEMPLATE_ARGS();     \
  BENCHMARK_TEMPLATE(BuildMultiStringProgram, type)->TEMPLATE_ARGS()

TEMPLATE_FOREACH(InputType::NOP);
TEMPLATE_FOREACH(InputType::NOBUILTINS);
TEMPLATE_FOREACH(InputType::MATHBUILTINS);
