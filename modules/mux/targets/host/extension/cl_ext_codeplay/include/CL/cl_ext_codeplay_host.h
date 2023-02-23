// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef CL_EXT_CODEPLAY_HOST_H_INCLUDED
#define CL_EXT_CODEPLAY_HOST_H_INCLUDED

#include <CL/cl.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// cl_codeplay_set_threads entry point function pointer
typedef cl_int(CL_API_CALL *clSetNumThreadsCODEPLAY_fn)(cl_device_id device,
                                                        cl_uint max_threads);

// cl_codeplay_set_threads entry point declaration
extern cl_int CL_API_CALL clSetNumThreadsCODEPLAY(cl_device_id device,
                                                  cl_uint max_threads);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // CL_EXT_CODEPLAY_HOST_H_INCLUDED
