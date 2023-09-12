// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
const std::array<const std::string, 11> supported_extensions = {
    {
        "SPV_KHR_no_integer_wrap_decoration",
        "SPV_INTEL_kernel_attributes",
        "SPV_EXT_shader_atomic_float_add",
        "SPV_EXT_shader_atomic_float_min_max",
        "SPV_KHR_expect_assume",
        "SPV_KHR_linkonce_odr",
        "SPV_KHR_uniform_group_instructions",
        "SPV_INTEL_arbitrary_precision_integers",
        "SPV_INTEL_optnone",
        "SPV_INTEL_memory_access_aliasing",
        "SPV_INTEL_subgroups",
    },
};

cargo::expected<compiler::spirv::DeviceInfo, cargo::result> getSPIRVDeviceInfo(
    mux_device_info_t device_info, cargo::string_view profile) {
  compiler::spirv::DeviceInfo spvDeviceInfo;
  cargo::result result = cargo::result::success;

  auto &spvCapabilities = spvDeviceInfo.capabilities;

  // A set of capabilities shared between the OpenCL profiles we support.
  static std::array<spv::Capability, 15> sharedCapabilities = {
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
      spv::CapabilityArbitraryPrecisionIntegersINTEL,
      spv::CapabilityOptNoneINTEL,
      spv::CapabilityMemoryAccessAliasingINTEL,
      spv::CapabilitySubgroupShuffleINTEL,
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
  if (device_info->max_sub_group_count) {
    // Note: This is something of a lie, as we don't support DeviceEnqueue,
    // but we must support the 'SubgroupSize' ExecutionMode for SYCL 2020 and
    // that is lumped in with SubgroupDispatch.
    if ((result = spvCapabilities.push_back(spv::CapabilitySubgroupDispatch))) {
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
