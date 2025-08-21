// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef CL_EXT_CODEPLAY_HOST_H_INCLUDED
#define CL_EXT_CODEPLAY_HOST_H_INCLUDED

#include <CL/cl.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// cl_codeplay_set_threads entry point function pointer
typedef cl_int(CL_API_CALL *clSetNumThreadsCODEPLAY_fn)(cl_device_id device,
                                                        cl_uint max_threads);

// cl_codeplay_set_threads entry point declaration
extern cl_int CL_API_CALL clSetNumThreadsCODEPLAY(cl_device_id device,
                                                  cl_uint max_threads);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CL_EXT_CODEPLAY_HOST_H_INCLUDED
