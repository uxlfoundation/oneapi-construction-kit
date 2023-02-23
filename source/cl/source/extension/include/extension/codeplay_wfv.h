// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Implementation of `cl_codeplay_wfv` extension.
///
/// @copyright
/// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef EXTENSION_CODEPLAY_WFV_H_INCLUDED
#define EXTENSION_CODEPLAY_WFV_H_INCLUDED

#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

/// @brief Definition of cl_codeplay_wfv extension.
class codeplay_wfv final : public extension {
 public:
  /// @brief Default constructor.
  codeplay_wfv();

  /// @brief Queries for the extension function associated with func_name.
  ///
  /// If extension is enabled, then makes an extensions function query-able:
  /// * `clGetKernelWFVInfoCODEPLAY`
  ///
  /// @see clGetExtensionFunctionAddressForPlatform.
  ///
  /// @param[in,out] platform OpenCL platform func_name belongs to.
  /// @param[in] func_name name of the extension function to query for.
  /// Supported function name if extension is enabled:
  /// * "clGetKernelWFVInfoCODEPLAY"
  /// @return Returns a pointer to the extension function or nullptr if no
  /// function with the name func_name exists.
  void *GetExtensionFunctionAddressForPlatform(
      cl_platform_id platform, const char *func_name) const override;

  /// @copydoc extension::extension::GetDeviceInfo
  cl_int GetDeviceInfo(cl_device_id device, cl_device_info param_name,
                       size_t param_value_size, void *param_value,
                       size_t *param_value_size_ret) const override;
};

/// @}
}  // namespace extension

namespace wfv {
/// @brief Checks if an OpenCL device can support vectorization, a mandatory
/// feature of the extension specification.
///
/// @param[in] device OpenCL device to query support for.
///
/// @return True if device can support vectorization, false otherwise.
bool deviceSupportsVectorization(cl_device_id device);
}  // namespace wfv

#endif  // EXTENSION_CODEPLAY_WFV_H_INCLUDED
