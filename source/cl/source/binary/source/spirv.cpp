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
const std::array<const std::string, 8> supported_extensions = {
    {
        "SPV_KHR_no_integer_wrap_decoration",
#ifdef SPIRV_LL_EXPERIMENTAL
        "SPV_codeplay_usm_generic_storage_class",
#endif
        "SPV_INTEL_kernel_attributes",
        "SPV_EXT_shader_atomic_float_add",
        "SPV_EXT_shader_atomic_float_min_max",
        "SPV_KHR_expect_assume",
        "SPV_KHR_linkonce_odr",
        "SPV_KHR_uniform_group_instructions",
    },
};

cargo::expected<compiler::spirv::DeviceInfo, cargo::result> getSPIRVDeviceInfo(
    mux_device_info_t device_info, cargo::string_view profile) {
  compiler::spirv::DeviceInfo spvDeviceInfo;
  cargo::result result = cargo::result::success;

  auto &spvCapabilities = spvDeviceInfo.capabilities;

  // A set of capabilities shared between the OpenCL profiles we support.
  static std::array<spv::Capability, 11> sharedCapabilities = {
      spv::CapabilityAddresses,
      spv::CapabilityFloat16Buffer,
      spv::CapabilityGroups,
      spv::CapabilityInt8,
      spv::CapabilityInt16,
      spv::CapabilityKernel,
      spv::CapabilityLinkage,
      spv::CapabilityVector16,
      spv::CapabilityKernelAttributesINTEL,
      spv::CapabilityExpectAssumeKHR,
      spv::CapabilityGroupUniformArithmeticKHR,
  };

  if (profile == "FULL_PROFILE") {
    auto error_or = spvCapabilities.insert(spvCapabilities.end(),
                                           sharedCapabilities.begin(),
                                           sharedCapabilities.end());
    if (!error_or) {
      return cargo::make_unexpected(error_or.error());
    }
    if ((result = spvCapabilities.push_back(spv::CapabilityInt64))) {
      return cargo::make_unexpected(result);
    }
  } else if (profile == "EMBEDDED_PROFILE") {
    auto error_or = spvCapabilities.insert(spvCapabilities.end(),
                                           sharedCapabilities.begin(),
                                           sharedCapabilities.end());
    if (!error_or) {
      return cargo::make_unexpected(error_or.error());
    }
    if (device_info->integer_capabilities & mux_integer_capabilities_64bit) {
      if ((result = spvCapabilities.push_back(spv::CapabilityInt64))) {
        return cargo::make_unexpected(result);
      }
    }
  }
  if (device_info->image_support) {
    auto error_or = spvCapabilities.insert(spvCapabilities.end(),
                                           {
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
    if ((result = spvCapabilities.push_back(spv::CapabilityFloat16))) {
      return cargo::make_unexpected(result);
    }
  }
  if (device_info->float_capabilities) {
    auto error_or = spvCapabilities.insert(
        spvCapabilities.end(), {
                                   spv::CapabilityAtomicFloat32AddEXT,
                                   spv::CapabilityAtomicFloat32MinMaxEXT,
                               });
    if (!error_or) {
      return cargo::make_unexpected(error_or.error());
    }
  }
  if (device_info->double_capabilities) {
    auto error_or = spvCapabilities.insert(
        spvCapabilities.end(), {
                                   spv::CapabilityFloat64,
                                   spv::CapabilityAtomicFloat64AddEXT,
                                   spv::CapabilityAtomicFloat64MinMaxEXT,
                               });
    if (!error_or) {
      return cargo::make_unexpected(error_or.error());
    }
  }
  if (device_info->supports_generic_address_space) {
    if ((result = spvCapabilities.push_back(spv::CapabilityGenericPointer))) {
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
