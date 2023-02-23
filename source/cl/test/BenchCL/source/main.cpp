// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <BenchCL/environment.h>
#include <BenchCL/utils.h>
#include <cargo/argument_parser.h>
#include <benchmark/benchmark.h>

#define CHECK(RESULT) \
  if (RESULT) {       \
    return -1;        \
  }                   \
  (void)0

benchcl::env* benchcl::env::instance = nullptr;

int main(int argc, char* argv[]) {
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

  cl_uint err = benchcl::get_device(device_name, platform, device);
  if (CL_SUCCESS != err) {
    return err;
  }

  benchcl::env environment(device_name, platform, device);
  benchcl::env::instance = &environment;

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  return 0;
}
