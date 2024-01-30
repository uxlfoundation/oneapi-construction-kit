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

#include <cargo/argument_parser.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/raw_ostream.h>
#include <spirv-ll/builder.h>
#include <spirv-ll/context.h>
#include <spirv-ll/module.h>
#include <spirv/unified1/spirv.hpp>

#include <fstream>
#include <iostream>
#include <unordered_set>
#include <vector>

int outputSpecConstants(spirv_ll::Context &spvContext,
                        llvm::ArrayRef<uint32_t> spvCode,
                        llvm::raw_os_ostream &out);

llvm::Expected<spirv_ll::DeviceInfo> getDeviceInfo(
    cargo::string_view api, cargo::array_view<cargo::string_view> capabilities,
    cargo::array_view<cargo::string_view> extensions, cargo::string_view bits,
    bool enableAll);

int main(int argc, char **argv) {
  // usage: spirv-ll-tool [options] input
  cargo::argument_parser<8> parser{
      cargo::argument_parser_option::ACCEPT_POSITIONAL};
  // -h, --help
  bool help = false;
  if (auto error = parser.add_argument({"-h", help})) {
    return error;
  }
  if (auto error = parser.add_argument({"--help", help})) {
    return error;
  }
  // -o PATH, --output PATH
  cargo::string_view output;
  if (auto error = parser.add_argument({"-o", output})) {
    return error;
  }
  if (auto error = parser.add_argument({"--output", output})) {
    return error;
  }
  // -a [OpenCL,Vulkan], --api [OpenCL,Vulkan]
  cargo::string_view api;
  cargo::small_vector<cargo::string_view, 2> apiChoices;
  if (auto error = apiChoices.assign({"OpenCL", "Vulkan"})) {
    return error;
  }
  if (auto error = parser.add_argument({"-a", apiChoices, api})) {
    return error;
  }
  if (auto error = parser.add_argument({"--api", apiChoices, api})) {
    return error;
  }
  // -c NAME, --capability NAME
  cargo::small_vector<cargo::string_view, 4> capabilities;
  if (auto error = parser.add_argument({"-c", capabilities})) {
    return error;
  }
  if (auto error = parser.add_argument({"--capability", capabilities})) {
    return error;
  }
  // -e NAME, --extension NAME
  cargo::small_vector<cargo::string_view, 4> extensions;
  if (auto error = parser.add_argument({"-e", extensions})) {
    return error;
  }
  if (auto error = parser.add_argument({"--extension", extensions})) {
    return error;
  }
  // -E, --enable-all
  bool enableAll = false;
  if (auto error = parser.add_argument({"-E", enableAll})) {
    return error;
  }
  if (auto error = parser.add_argument({"--enable-all", enableAll})) {
    return error;
  }
  // -b BITS, --address-bits BITS
  cargo::string_view addressBits;
  cargo::small_vector<cargo::string_view, 2> addressBitsChoices;
  if (auto error = addressBitsChoices.assign({"32", "64"})) {
    return error;
  }
  if (auto error = parser.add_argument({"-b", addressBits})) {
    return error;
  }
  if (auto error = parser.add_argument({"--address-bits", addressBits})) {
    return error;
  }
  // -s, --spec-constants
  bool specConstants = false;
  if (auto error = parser.add_argument({"-s", specConstants})) {
    return error;
  }
  if (auto error = parser.add_argument({"--spec-constants", specConstants})) {
    return error;
  }

  const std::string usage =
      "usage: " + std::string{argv[0]} + " [options] input";
  if (auto error = parser.parse_args(argc, argv)) {
    std::cerr << "error: invalid arguments";
    std::cerr << "\n" << usage << "\n";
    return error;
  }
  if (help) {
    std::cout << usage << "\n"
              << R"(
Translate a SPIR-V binary file into LLVM-IR printed to stdout by default.

positional arguments:
        input           a SPIR-V binary file

optional arguments:
        -h, --help      display this help message and exit
        -o FILE, --output FILE
                        output file path for the LLVM-IR. Default value or '-'
                        outputs to stdout.
        -a {OpenCL,Vulkan}, --api {OpenCL,Vulkan}
                        api the SPIR-V binary is targeting, only OpenCL 1.2 and
                        Vulkan 1.0 compute modules are supported
        -c CAPABILITY, --capability CAPABILITY
                        name of capability to enable, multiple supported
        -e EXTENSION, --extension EXTENSION
                        name of extension to enable, multiple supported
        -E, --enable-all
                        enable all extensions and capabilities supported by the
                        chosen api
        -b {32,64}, --address-bits {32,64}
                        size of device address in bits
        -s, --spec-constants
                        output all specialization constants and exit
)";
    return 0;
  }

  auto positionalArgs = parser.get_positional_args();
  if (positionalArgs.size() == 0) {
    std::cerr << "error: missing argument: input\n";
    std::cerr << usage << "\n";
    return 1;
  } else if (positionalArgs.size() > 1) {
    std::cerr << "error: too many arguments: ";
    positionalArgs.pop_front();
    for (auto positionalArg : positionalArgs) {
      std::cerr << cargo::as<std::string>(positionalArg) << " ";
    }
    std::cerr << "\n" << usage << "\n";
    return 1;
  }
  auto input = cargo::as<std::string>(parser.get_positional_args()[0]);

  auto buffer = llvm::MemoryBuffer::getFile(input, /*IsText=*/false,
                                            /*RequiresNullTerminator=*/false);
  if (!buffer) {
    std::cerr << "error: could not open file \"" << input << "\"\n";
    return 1;
  }
  const llvm::ArrayRef<uint32_t> spvCode{
      reinterpret_cast<const uint32_t *>(buffer.get()->getBufferStart()),
      buffer.get()->getBufferSize() / sizeof(uint32_t)};

  // Dump the output file if present, otherwise dump to stdout.
  std::ofstream outputFile;
  if (!output.empty() && output != "-") {
    outputFile.open({output.data(), output.size()});
  }
  llvm::raw_os_ostream out(outputFile.is_open() ? outputFile : std::cout);

  // Create the required modules, contexts, and builders.
  spirv_ll::Context spvContext{};

  if (specConstants) {
    // Output all specialization constants and exit.
    return outputSpecConstants(spvContext, spvCode, out);
  }

  auto spvDeviceInfo =
      getDeviceInfo(api, capabilities, extensions, addressBits, enableAll);
  if (auto err = spvDeviceInfo.takeError()) {
    std::cerr << llvm::toString(std::move(err)) << "\n";
    return 1;
  }

  // When called from an api a spec constant offset map will be constructed and
  // passed here, since this is a debug/test tool we can just pass an empty map.
  spirv_ll::SpecializationInfo spvSpecializationInfo;

  auto spvModule =
      spvContext.translate(spvCode, *spvDeviceInfo, spvSpecializationInfo);
  if (!spvModule) {
    std::cerr << spvModule.error().message << "\n";
    return 1;
  }

  // Dump to the output file.
  spvModule->llvmModule->print(out, nullptr, false, false);

  // Make sure everything is in order
  if (llvm::verifyModule(*spvModule->llvmModule, &llvm::errs())) {
    std::cerr << "warning: module verification failed" << std::endl;
  }

  return 0;
}

int outputSpecConstants(spirv_ll::Context &spvContext,
                        llvm::ArrayRef<uint32_t> spvCode,
                        llvm::raw_os_ostream &out) {
  // Parse the modules specializable constants.
  auto specializableConstantsMap =
      spvContext.getSpecializableConstants(spvCode);
  if (!specializableConstantsMap) {
    std::cerr << specializableConstantsMap.error().message << "\n";
    return 1;
  }

  // Sort specializableConstantsMap on the SpecId value.
  using SpecDescPair = std::pair<spv::Id, spirv_ll::SpecializationDesc>;
  llvm::SmallVector<SpecDescPair, 16> orderedSpecializableConstants;
  for (const auto &specConstant : *specializableConstantsMap) {
    orderedSpecializableConstants.push_back(specConstant);
  }
  std::sort(orderedSpecializableConstants.begin(),
            orderedSpecializableConstants.end(),
            [](const SpecDescPair &lhs, const SpecDescPair &rhs) {
              return lhs.first < rhs.first;
            });

  // Output the ordered specializable constants.
  for (const auto &specConstant : orderedSpecializableConstants) {
    out << "SpecId: " << specConstant.first << "\t";
    switch (specConstant.second.constantType) {
      case spirv_ll::SpecializationType::BOOL:
        out << "OpTypeBool";
        break;
      case spirv_ll::SpecializationType::INT:
        out << "OpTypeInt";
        break;
      case spirv_ll::SpecializationType::FLOAT:
        out << "OpTypeFloat";
        break;
    }
    out << "\t" << specConstant.second.sizeInBits << " bit\n";
  }

  return 0;
}

llvm::Expected<spirv_ll::DeviceInfo> getDeviceInfo(
    cargo::string_view api, cargo::array_view<cargo::string_view> capabilities,
    cargo::array_view<cargo::string_view> extensions, cargo::string_view bits,
    bool enableAll) {
  spirv_ll::DeviceInfo deviceInfo;
  if (api == "OpenCL") {
    deviceInfo.capabilities.assign({
        spv::CapabilityAddresses,
        spv::CapabilityFloat16,
        spv::CapabilityFloat16Buffer,
        spv::CapabilityGroups,
        spv::CapabilityInt64,
        spv::CapabilityInt16,
        spv::CapabilityInt8,
        spv::CapabilityKernel,
        spv::CapabilityLinkage,
        spv::CapabilityVector16,
        spv::CapabilityVector16,
        spv::CapabilityKernelAttributesINTEL,
        spv::CapabilityExpectAssumeKHR,
        spv::CapabilityOptNoneINTEL,
        spv::CapabilityMemoryAccessAliasingINTEL,
    });
    if (enableAll) {
      // Add the optional OpenCL capabilities if this flag was set.
      deviceInfo.capabilities.append({
          spv::CapabilityFloat64,
          spv::CapabilityImage1D,
          spv::CapabilityImageBasic,
          spv::CapabilityImageBuffer,
          spv::CapabilityImageReadWrite,
          spv::CapabilityLiteralSampler,
          spv::CapabilitySampled1D,
          spv::CapabilitySampledBuffer,
      });
    }
    deviceInfo.extInstImports.push_back("OpenCL.std");
    if (bits == "32") {
      deviceInfo.addressingModel = spv::AddressingModelPhysical32;
      deviceInfo.addressBits = 32;
    } else if (bits == "64") {
      deviceInfo.addressingModel = spv::AddressingModelPhysical64;
      deviceInfo.addressBits = 64;
    } else {
      deviceInfo.addressingModel = spv::AddressingModelMax;
      deviceInfo.addressBits = 0;
    }
    deviceInfo.memoryModel = spv::MemoryModelOpenCL;
  } else if (api == "Vulkan") {
    deviceInfo.capabilities.assign({
        spv::CapabilityMatrix,
        spv::CapabilityShader,
        spv::CapabilityInputAttachment,
        spv::CapabilitySampled1D,
        spv::CapabilityImage1D,
        spv::CapabilitySampledBuffer,
        spv::CapabilityImageBuffer,
        spv::CapabilityImageQuery,
        spv::CapabilityDerivativeControl,
    });
    if (enableAll) {
      // Add the optional Vulkan capabilities if this flag was set.
      deviceInfo.capabilities.append({
          spv::CapabilityFloat64,
          spv::CapabilityInt64,
          spv::CapabilityInt16,
          spv::CapabilityVariablePointers,
          spv::CapabilityVariablePointersStorageBuffer,
      });
    }
    deviceInfo.extInstImports.push_back("GLSL.std.450");
    deviceInfo.addressingModel = spv::AddressingModelLogical;
    deviceInfo.memoryModel = spv::MemoryModelGLSL450;
    if (bits == "32") {
      deviceInfo.addressBits = 32;
    } else if (bits == "64") {
      deviceInfo.addressBits = 64;
    } else {
      deviceInfo.addressBits = sizeof(void *) * 8;
    }
  } else {
    llvm_unreachable("Illegal API");
  }
  for (auto cap : capabilities) {
    if (auto capability = spirv_ll::getCapabilityFromString(cap.data())) {
      // SPIR-V 1.0 list of capabilities.
      static const std::unordered_set<spv::Capability>
          supported_v1_0_capabilities = {
              spv::CapabilityMatrix,
              spv::CapabilityShader,
              spv::CapabilityGeometry,
              spv::CapabilityTessellation,
              spv::CapabilityAddresses,
              spv::CapabilityLinkage,
              spv::CapabilityKernel,
              spv::CapabilityVector16,
              spv::CapabilityFloat16Buffer,
              spv::CapabilityFloat16,
              spv::CapabilityFloat64,
              spv::CapabilityInt64,
              spv::CapabilityInt64Atomics,
              spv::CapabilityImageBasic,
              spv::CapabilityImageReadWrite,
              spv::CapabilityImageMipmap,
              spv::CapabilityPipes,
              spv::CapabilityGroups,
              spv::CapabilityDeviceEnqueue,
              spv::CapabilityLiteralSampler,
              spv::CapabilityAtomicStorage,
              spv::CapabilityInt16,
              spv::CapabilityTessellationPointSize,
              spv::CapabilityGeometryPointSize,
              spv::CapabilityImageGatherExtended,
              spv::CapabilityStorageImageMultisample,
              spv::CapabilityUniformBufferArrayDynamicIndexing,
              spv::CapabilitySampledImageArrayDynamicIndexing,
              spv::CapabilityStorageBufferArrayDynamicIndexing,
              spv::CapabilityStorageImageArrayDynamicIndexing,
              spv::CapabilityClipDistance,
              spv::CapabilityCullDistance,
              spv::CapabilityImageCubeArray,
              spv::CapabilitySampleRateShading,
              spv::CapabilityImageRect,
              spv::CapabilitySampledRect,
              spv::CapabilityGenericPointer,
              spv::CapabilityInt8,
              spv::CapabilityInputAttachment,
              spv::CapabilitySparseResidency,
              spv::CapabilityMinLod,
              spv::CapabilitySampled1D,
              spv::CapabilityImage1D,
              spv::CapabilitySampledCubeArray,
              spv::CapabilitySampledBuffer,
              spv::CapabilityImageBuffer,
              spv::CapabilityImageMSArray,
              spv::CapabilityStorageImageExtendedFormats,
              spv::CapabilityImageQuery,
              spv::CapabilityDerivativeControl,
              spv::CapabilityInterpolationFunction,
              spv::CapabilityTransformFeedback,
              spv::CapabilityGeometryStreams,
              spv::CapabilityStorageImageReadWithoutFormat,
              spv::CapabilityStorageImageWriteWithoutFormat,
              spv::CapabilityMultiViewport,
              spv::CapabilitySubgroupBallotKHR,
              spv::CapabilityDrawParameters,
              spv::CapabilitySubgroupVoteKHR,
              spv::CapabilityStorageBuffer16BitAccess,
              spv::CapabilityStorageUniformBufferBlock16,
              spv::CapabilityUniformAndStorageBuffer16BitAccess,
              spv::CapabilityStorageUniform16,
              spv::CapabilityStoragePushConstant16,
              spv::CapabilityStorageInputOutput16,
              spv::CapabilityDeviceGroup,
              spv::CapabilityMultiView,
              spv::CapabilityVariablePointersStorageBuffer,
              spv::CapabilityVariablePointers,
              spv::CapabilityAtomicStorageOps,
              spv::CapabilitySampleMaskPostDepthCoverage,
              spv::CapabilityImageGatherBiasLodAMD,
              spv::CapabilityFragmentMaskAMD,
              spv::CapabilityStencilExportEXT,
              spv::CapabilityImageReadWriteLodAMD,
              spv::CapabilitySampleMaskOverrideCoverageNV,
              spv::CapabilityGeometryShaderPassthroughNV,
              spv::CapabilityShaderViewportIndexLayerEXT,
              spv::CapabilityShaderViewportIndexLayerNV,
              spv::CapabilityShaderViewportMaskNV,
              spv::CapabilityShaderStereoViewNV,
              spv::CapabilityPerViewAttributesNV,
              spv::CapabilitySubgroupShuffleINTEL,
              spv::CapabilitySubgroupBufferBlockIOINTEL,
              spv::CapabilitySubgroupImageBlockIOINTEL,
              spv::CapabilityExpectAssumeKHR,
              spv::CapabilityGroupUniformArithmeticKHR,
              spv::CapabilityAtomicFloat32AddEXT,
              spv::CapabilityAtomicFloat64AddEXT,
              spv::CapabilityAtomicFloat32MinMaxEXT,
              spv::CapabilityAtomicFloat64MinMaxEXT,
              spv::CapabilityArbitraryPrecisionIntegersINTEL,
              spv::CapabilityOptNoneINTEL,
              spv::CapabilityMemoryAccessAliasingINTEL};

      // SPIR-V 1.1 list of capabilities.
      static const std::unordered_set<spv::Capability>
          supported_v1_1_capabilities = {
              spv::CapabilitySubgroupDispatch,
          };

      if (supported_v1_0_capabilities.count(*capability) ||
          supported_v1_1_capabilities.count(*capability)) {
        deviceInfo.capabilities.push_back(*capability);
      } else {
        std::cerr << "error: unsupported capability: "
                  << cargo::as<std::string>(cap) << "\n";
        std::exit(1);
      }
    } else {
      std::cerr << "error: unknown capability: " << cargo::as<std::string>(cap)
                << "\n";
      std::exit(1);
    }
  }
  if (enableAll) {
    deviceInfo.extensions.append({
        "SPV_KHR_16bit_storage",
        "SPV_KHR_float_controls",
        "SPV_KHR_no_integer_wrap_decoration",
        "SPV_KHR_storage_buffer_storage_class",
        "SPV_KHR_variable_pointers",
        "SPV_KHR_vulkan_memory_model",
        "SPV_KHR_expect_assume",
        "SPV_KHR_linkonce_odr",
        "SPV_KHR_uniform_group_instructions",
        "SPV_INTEL_optnone",
        "SPV_INTEL_memory_access_aliasing",
        "SPV_INTEL_subgroups",
    });
  } else {
    for (auto extension : extensions) {
      deviceInfo.extensions.push_back(cargo::as<std::string>(extension));
    }
  }
  return deviceInfo;
}
