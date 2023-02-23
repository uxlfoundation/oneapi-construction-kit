// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Implementation of cl_codeplay_performance_counters extension.
///
/// @copyright
/// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef EXTENSION_CODEPLAY_PERFORMANCE_COUNTERS_H_INCLUDED
#define EXTENSION_CODEPLAY_PERFORMANCE_COUNTERS_H_INCLUDED

#include <extension/extension.h>

namespace extension {
struct codeplay_performance_counters : extension {
  /// @brief Default constructor.
  codeplay_performance_counters();

  /// @copydoc extension::extension::GetDeviceInfo
  cl_int GetDeviceInfo(cl_device_id device, cl_device_info param_name,
                       size_t param_value_size, void *param_value,
                       size_t *param_value_size_ret) const override;

  /// @copydoc extension::extension::ApplyPropertyToCommandQueue
  cargo::optional<cl_int> ApplyPropertyToCommandQueue(
      cl_command_queue command_queue, cl_queue_properties_khr property,
      cl_queue_properties_khr value) const override;

  /// @copydoc extension::extension::GetEventProfilingInfo
  cl_int GetEventProfilingInfo(cl_event event, cl_profiling_info param_name,
                               size_t param_value_size, void *param_value,
                               size_t *param_value_size_ret) const override;
};
}  // namespace extension

#endif  // EXTENSION_CODEPLAY_PERFORMANCE_COUNTERS_H_INCLUDED
