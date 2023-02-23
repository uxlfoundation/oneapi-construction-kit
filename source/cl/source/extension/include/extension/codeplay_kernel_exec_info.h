// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Implementation of `cl_codeplay_kernel_exec_info` extension.
///
/// @copyright
/// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef EXTENSION_CODEPLAY_KERNEL_EXEC_INFO_H_INCLUDED
#define EXTENSION_CODEPLAY_KERNEL_EXEC_INFO_H_INCLUDED

#include <CL/cl_ext_codeplay.h>
#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

/// @brief Definition of cl_codeplay_kernel_exec_info extension.
struct codeplay_kernel_exec_info : extension {
  /// @brief Default constructor.
  codeplay_kernel_exec_info();

  /// @brief Queries for the extension function associated with func_name.
  ///
  /// If extension is enabled, then makes the following extension function
  /// query-able:
  /// * "clSetKernelExecInfoCODEPLAY"
  ///
  /// @see clGetExtensionFunctionAddressForPlatform.
  ///
  /// @param[in] platform OpenCL platform func_name belongs to.
  /// @param[in] func_name name of the extension function to query for.
  ///
  /// @return Returns a pointer to the extension function or nullptr if no
  /// function with the name func_name exists.
  void *GetExtensionFunctionAddressForPlatform(
      cl_platform_id platform, const char *func_name) const override;
};

/// @}
}  // namespace extension

#endif  // EXTENSION_CODEPLAY_KERNEL_EXEC_INFO_H_INCLUDED
