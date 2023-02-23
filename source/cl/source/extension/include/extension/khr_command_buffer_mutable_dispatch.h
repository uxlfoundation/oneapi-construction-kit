// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @brief Command-Buffer Mutable Dispatch.
///
/// This extension adds support for command-buffers to be modified after
/// recording. Allowing the same command-buffer to be replayed with minor
/// changes to kernel execution cinnabd configuration, rather than having to be
/// re-recorded. The `cl_khr_command_buffer` extension is required to support
/// this, as the specification is layered on top.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef CL_EXTENSION_KHR_MUTABLE_DISPATCH_H_INCLUDED
#define CL_EXTENSION_KHR_MUTABLE_DISPATCH_H_INCLUDED

#include <CL/cl_ext.h>
#include <cl/base.h>
#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

class khr_command_buffer_mutable_dispatch : public extension {
 public:
  khr_command_buffer_mutable_dispatch();
  /// @brief Queries for the extension function associated with
  /// `func_name`.
  ///
  /// New procedures and functions: `clUpdateMutableCommandsKHR`,
  /// `clRetainMutableCommandKHR`, `clReleaseMutableCommandKHR`,
  /// `clGetMutableCommandInfoKHR`.
  ///
  /// @see
  /// https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_Ext.html
  ///
  /// @param[in] platform OpenCL platform `func_name` belongs to.
  /// @param[in] func_name Name of the extension function to query for.
  ///
  /// @return Returns a pointer to the extension function with
  /// `func_name` or `nullptr` if it does not exist.
  void *GetExtensionFunctionAddressForPlatform(
      cl_platform_id platform, const char *func_name) const override;

  /// @copydoc extension::extension::GetDeviceInfo
  cl_int GetDeviceInfo(cl_device_id device, cl_device_info param_name,
                       size_t param_value_size, void *param_value,
                       size_t *param_value_size_ret) const override;
};

/// @}
}  // namespace extension

#endif  // CL_EXTENSION_KHR_MUTABLE_DISPATCH_H_INCLUDED
