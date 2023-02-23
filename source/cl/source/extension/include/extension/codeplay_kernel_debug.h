// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Implementation of cl_codeplay_kernel_debug extension.
///
/// @copyright
/// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef EXTENSION_CODEPLAY_KERNEL_DEBUG_H_INCLUDED
#define EXTENSION_CODEPLAY_KERNEL_DEBUG_H_INCLUDED

#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

/// @brief Definition of cl_codeplay_kernel_debug extension.
struct codeplay_kernel_debug final : extension {
  /// @brief Default constructor.
  codeplay_kernel_debug();

  /// @copydoc extension::extension::GetDeviceInfo
  cl_int GetDeviceInfo(cl_device_id device, cl_device_info param_name,
                       size_t param_value_size, void *param_value,
                       size_t *param_value_size_ret) const override;
};

/// @}
}  // namespace extension

#endif  // EXTENSION_CODEPLAY_KERNEL_DEBUG_H_INCLUDED
