// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef CL_BINARY_SPIRV_H_INCLUDED
#define CL_BINARY_SPIRV_H_INCLUDED

#include <cargo/string_view.h>
#include <compiler/module.h>
#include <mux/mux.h>

namespace cl {
namespace binary {

/// @brief Get device specific information needed to compile a SPIR-V module.
///
/// @param device_info Target device information to compile the module.
/// @param profile The OpenCL profile to target.
///
/// @return Returns device specific SPIR-V information, or a cargo failure if
/// there was an error.
cargo::expected<compiler::spirv::DeviceInfo, cargo::result> getSPIRVDeviceInfo(
    mux_device_info_t device_info, cargo::string_view profile);

}  // namespace binary
}  // namespace cl

#endif  // CL_BINARY_SPIRV_H_INCLUDED
