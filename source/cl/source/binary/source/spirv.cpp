// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cl/binary/spirv.h>

#include <array>
#include <string>

namespace cl {
namespace binary {

// List of supported SPIR-V extensions. There isn't actually a way to report
// these through the API so they basically just get passed through to spirv-ll
// so it doesn't throw an error if it encounters them.
// TODO: add a proper mechanism for extending spirv-ll and reporting extension
// support. We don't actually support the generic storage class extension on
// all core targets. See CA-3067.
// TODO(CA-3067): Remove SPIRV_LL_EXPERIMENTAL when we have a proper mechanism
// for extending spirv-ll.
const std::array<const std::string, 3> supported_extensions = {
    {
        "SPV_KHR_no_integer_wrap_decoration",
#ifdef SPIRV_LL_EXPERIMENTAL
        "SPV_codeplay_usm_generic_storage_class",
#endif
        "SPV_INTEL_kernel_attributes",
    },
};

cargo::expected<compiler::spirv::DeviceInfo, cargo::result> getSPIRVDeviceInfo(
    mux_device_info_t device_info, cargo::string_view profile) {
  compiler::spirv::DeviceInfo spvDeviceInfo;
  cargo::result result = cargo::result::success;

  if (profile == "FULL_PROFILE") {
    if ((result = spvDeviceInfo.capabilities.assign(
             {spv::CapabilityAddresses, spv::CapabilityFloat16Buffer,
              spv::CapabilityGroups, spv::CapabilityInt64, spv::CapabilityInt16,
              spv::CapabilityInt8, spv::CapabilityKernel,
              spv::CapabilityLinkage, spv::CapabilityVector16,
              spv::CapabilityKernelAttributesINTEL}))) {
      return cargo::make_unexpected(result);
    }
  } else if (profile == "EMBEDDED_PROFILE") {
    if ((result = spvDeviceInfo.capabilities.assign(
             {spv::CapabilityAddresses, spv::CapabilityFloat16Buffer,
              spv::CapabilityGroups, spv::CapabilityInt16, spv::CapabilityInt8,
              spv::CapabilityKernel, spv::CapabilityLinkage,
              spv::CapabilityVector16,
              spv::CapabilityKernelAttributesINTEL}))) {
      return cargo::make_unexpected(result);
    }
    if (device_info->integer_capabilities & mux_integer_capabilities_64bit) {
      if ((result =
               spvDeviceInfo.capabilities.push_back(spv::CapabilityInt64))) {
        return cargo::make_unexpected(result);
      }
    }
  }
  if (device_info->image_support) {
    auto error_or = spvDeviceInfo.capabilities.insert(
        spvDeviceInfo.capabilities.end(), {
                                              spv::CapabilityImageBasic,
                                              spv::CapabilityLiteralSampler,
                                              spv::CapabilitySampled1D,
                                              spv::CapabilityImage1D,
                                              spv::CapabilitySampledBuffer,
                                              spv::CapabilityImageBuffer,
                                          });
    if (!error_or) {
      return cargo::make_unexpected(error_or.error());
    }
  }
  if (device_info->half_capabilities) {
    if ((result =
             spvDeviceInfo.capabilities.push_back(spv::CapabilityFloat16))) {
      return cargo::make_unexpected(result);
    }
  }
  if (device_info->double_capabilities) {
    if ((result =
             spvDeviceInfo.capabilities.push_back(spv::CapabilityFloat64))) {
      return cargo::make_unexpected(result);
    }
  }
  for (const std::string &extension : supported_extensions) {
    if ((result = spvDeviceInfo.extensions.push_back(extension))) {
      return cargo::make_unexpected(result);
    }
  }
  spvDeviceInfo.memory_model = spv::MemoryModelOpenCL;
  if (device_info->address_capabilities & mux_address_capabilities_bits32) {
    spvDeviceInfo.addressing_model = spv::AddressingModelPhysical32;
  } else if (device_info->address_capabilities &
             mux_address_capabilities_bits64) {
    spvDeviceInfo.addressing_model = spv::AddressingModelPhysical64;
  }
  return {std::move(spvDeviceInfo)};
}

}  // namespace binary
}  // namespace cl
