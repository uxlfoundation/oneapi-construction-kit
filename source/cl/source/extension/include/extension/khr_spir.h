// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Functions for cl_khr_spir extension.
///
/// See The OpenCL Extension Specification, Version: 1.2.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef EXTENSION_KHR_SPIR_H_INCLUDED
#define EXTENSION_KHR_SPIR_H_INCLUDED

#include <cl/program.h>
#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

/// @brief Extension cl_khr_spir object.
///
/// If enabled, then `GetDeviceInfo` queries for `CL_DEVICE_EXTENSIONS` return
/// `"cl_khr_spir"` as the query value.  Queries for `CL_DEVICE_SPIR_VERSIONS`
/// return a C string with the supported SPIR version.
///
/// Provides static member functions required to implement changes to the
/// OpenCL 1.2 specification made by the SPIR extension.
///
/// Check if the SPIR extension is enabled via:
/// * ocl::extension::ExtensionIsEnabled<ExtensionClKhrSpir>()
class khr_spir final : public extension {
 public:
  /// @brief Default constructor.
  khr_spir();

  /// @brief Queries for extension provided device info.
  ///
  /// If extension is enabled, then clGetDeviceInfo queries for
  /// CL_DEVICE_EXTENSIONS return "cl_khr_spir" as the query value. Queries for
  /// CL_DEVICE_SPIR_VERSIONS return a C string with the supported SPIR version.
  ///
  /// @see `::extension::extension::GetDeviceInfo` for more detailed
  /// explanations.
  ///
  /// @param[in] device OpenCL device to query.
  /// @param[in] param_name Information to query for, e.g.:
  /// * CL_DEVICE_EXTENSIONS
  /// * CL_DEVICE_SPIR_VERSIONS
  /// @param[in] param_value_size Size in bytes of the memory area param_value
  /// points to.
  /// @param[in] param_value Memory area to copy the queried info in or nullptr
  /// to not receive the info value.
  /// @param[out] param_value_size_ret References variable where to store the
  /// size of the info value or nullptr to not receive the info value.
  ///
  /// @return Returns an OpenCL error code.
  /// @retval `CL_SUCCESS` if the extension accepts the param_name query
  /// and the supplied argument values.
  /// @retval `CL_INVALID_VALUE` if the extension does not accept the
  /// `param_name` query.
  cl_int GetDeviceInfo(cl_device_id device, cl_device_info param_name,
                       size_t param_value_size, void *param_value,
                       size_t *param_value_size_ret) const override;
};

/// @}
}  // namespace extension

#endif  // EXTENSION_KHR_SPIR_H_INCLUDED
