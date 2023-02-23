// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef UNITCL_CALLBACKS_H_INCLUDED
#define UNITCL_CALLBACKS_H_INCLUDED

#include <CL/cl.h>

namespace ucl {
/// @brief Callback for use during context creation.
///
/// Prints the `errinfo` string to `stderr`, example usage:
///
/// ```cpp
/// cl_int error;
/// cl_context context = clCreateContext(nullptr, 1, &device,
///                                      UCL::contextCallback, nullptr, &error);
/// ```
///
/// @param errinfo is a pointer to an error string.
/// @param private_info unused pointer to binary info.
/// @param cb unused size of binary info.
/// @param user_data unused pointer to user data.
void CL_CALLBACK contextCallback(const char *errinfo, const void *private_info,
                                 size_t cb, void *user_data);

/// @brief Callback for clBuildProgram to print build log.
///
/// @param program Program to print build log for.
void CL_CALLBACK buildLogCallback(cl_program program, void *);
}  // namespace ucl

#endif  // UNITCL_CALLBACKS_H_INCLUDED
