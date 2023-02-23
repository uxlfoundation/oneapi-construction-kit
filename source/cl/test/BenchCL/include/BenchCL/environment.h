// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef BENCHCL_ENVIRONMENT_H_INCLUDED
#define BENCHCL_ENVIRONMENT_H_INCLUDED

#include <cargo/string_view.h>
#include <CL/cl.h>

namespace benchcl {
struct env {
  env(cargo::string_view device_name, cl_platform_id platform,
      cl_device_id device)
      : device_name(device_name), platform(platform), device(device) {}

  cargo::string_view device_name;
  cl_platform_id platform;
  cl_device_id device;

  static env* get() { return instance; }

  static env* instance;
};
}  // namespace benchcl

#endif // BENCHCL_ENVIRONMENT_H_INCLUDED
