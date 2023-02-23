// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Support for the `cl_khr_create_command_queue` extension.
///
/// @copyright
/// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef EXTENSION_KHR_CREATE_COMMAND_QUEUE_H_INCLUDED
#define EXTENSION_KHR_CREATE_COMMAND_QUEUE_H_INCLUDED

#include <extension/extension.h>

namespace extension {
/// @addtogroup cl_extension
/// @{

class khr_create_command_queue final : public extension {
 public:
  /// @brief Default constructor.
  khr_create_command_queue();

  /// @brief Queries for the extension function associated with func_name.
  ///
  /// If extension is enabled, then makes an extensions function query-able:
  /// * `clCreateCommandQueueWithPropertiesKHR`
  ///
  /// @see `cl::GetExtensionFunctionAddressForPlatform`.
  ///
  /// @param[in,out] platform OpenCL platform func_name belongs to.
  /// @param[in] func_name name of the extension function to query for.
  /// Supported function name if extension is enabled:
  /// * `clCreateCommandQueueWithPropertiesKHR`
  ///
  /// @return Returns a pointer to the extension function or nullptr if no
  /// function with the name func_name exists.
  void *GetExtensionFunctionAddressForPlatform(
      cl_platform_id platform, const char *func_name) const override;
};

/// @}
}  // namespace extension

#endif  // EXTENSION_KHR_CREATE_COMMAND_QUEUE_H_INCLUDED
