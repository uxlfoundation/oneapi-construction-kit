// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Host builtin kernel support.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef HOST_BUILTIN_KERNEL_H_INCLUDED
#define HOST_BUILTIN_KERNEL_H_INCLUDED

#include <host/kernel.h>
#include <mux/mux.h>

#include <map>
#include <string>

namespace host {
/// @addtogroup host
/// @{

using builtin_kernel_map =
    std::map<std::string, ::host::kernel_variant_s::entry_hook_t>;

/// @brief Get the map of supported builtin kernels.
///
/// @param[in] device_info Pointer to device information.
///
/// @return Returns the builtin kernel map if present.
builtin_kernel_map getBuiltinKernels(mux_device_info_t device_info);

/// @}
}  // namespace host

#endif  // HOST_BUILTIN_KERNEL_H_INCLUDED
