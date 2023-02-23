// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Functions for cl_khr_fp16 extension.
///
/// See The OpenCL Extension Specification, Version: 1.2.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef EXTENSION_KHR_FP16_H_INCLUDED
#define EXTENSION_KHR_FP16_H_INCLUDED

#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

/// @brief Definition of the cl_khr_fp16 extension.
class khr_fp16 final : public extension {
 public:
  /// @brief Default constructor.
  khr_fp16();

  /// @brief Queries for extension provided device info.
  ///
  /// If enabled, then `GetDeviceInfo` queries for `CL_DEVICE_EXTENSIONS` return
  /// `"cl_khr_fp16"` as the query value. Queries for `CL_DEVICE_HALF_FP_CONFIG`
  /// return a `cl_device_fp_config` value with the half precision capabilities.
  ///
  /// @see `::extension::extension::GetDeviceInfo` for more detailed
  /// explanations.
  ///
  /// @param[in] device OpenCL device to query.
  /// @param[in] param_name Information to query for, e.g.:
  /// * CL_DEVICE_EXTENSIONS
  /// * CL_DEVICE_HALF_FP_CONFIG
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

#endif  // EXTENSION_KHR_FP16_H_INCLUDED
