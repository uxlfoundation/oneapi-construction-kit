// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef BENCHCL_UTILS_H_INCLUDED
#define BENCHCL_UTILS_H_INCLUDED

#include <cargo/string_view.h>
#include <CL/cl.h>

namespace benchcl {
cl_uint get_device(cargo::string_view device_name, cl_platform_id& platform,
                   cl_device_id& device);
}  // namespace benchcl

#endif // BENCHCL_UTILS_H_INCLUDED
