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
#include <BenchCL/utils.h>
#include <benchmark/benchmark.h>
#include <cargo/argument_parser.h>

#define CHECK(RESULT) \
  if (RESULT) {       \
    return -1;        \
  }                   \
  (void)0

benchcl::env *benchcl::env::instance = nullptr;

int main(int argc, char *argv[]) {
  cargo::argument_parser<1> parser(cargo::KEEP_UNRECOGNIZED);
  cargo::string_view device_name;
  CHECK(parser.add_argument({"--benchcl_device=", device_name}));

  bool help = false;
  CHECK(parser.add_argument({"--help", help}));

  if (auto error = parser.parse_args(argc, argv)) {
    return error;
  }

  if (help) {
    // If --help was passed print usage messages and exit.
    fprintf(stdout, "benchcl   [--benchcl_device=<OpenCL device name>]\n");
    benchmark::Initialize(&argc, argv);
    return 0;
  }

  // Otherwise we're actually going to run benchmarks, setup the environment.
  cl_platform_id platform;
  cl_device_id device;

  const cl_uint err = benchcl::get_device(device_name, platform, device);
  if (CL_SUCCESS != err) {
    return err;
  }

  benchcl::env environment(device_name, platform, device);
  benchcl::env::instance = &environment;

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  return 0;
}
