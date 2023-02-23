// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cargo/argument_parser.h>
#include <spirv-ll/builder.h>
#include <spirv-ll/context.h>
#include <spirv-ll/module.h>

#include <llvm/ADT/StringSwitch.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/raw_ostream.h>

#include <fstream>
#include <iostream>
#include <vector>

int outputSpecConstants(spirv_ll::Context &spvContext,
                        llvm::ArrayRef<uint32_t> spvCode,
                        llvm::raw_os_ostream &out);

cargo::expected<spirv_ll::DeviceInfo, std::string> getDeviceInfo(
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

  std::string usage = "usage: " + std::string{argv[0]} + " [options] input";
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
  llvm::ArrayRef<uint32_t> spvCode{
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
  if (!spvDeviceInfo) {
    std::cerr << spvDeviceInfo.error() << "\n";
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

cargo::expected<spirv_ll::DeviceInfo, std::string> getDeviceInfo(
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
          spv::CapabilityKernelAttributesINTEL,
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
      deviceInfo.addressBits = sizeof(void*) * 8;
    }
  } else {
    llvm_unreachable("Illegal API");
  }
  for (auto cap : capabilities) {
    // SPIR-V 1.0 list of capabilities.
    auto capability =
        llvm::StringSwitch<spv::Capability>(cargo::as<llvm::StringRef>(cap))
            .Case("Matrix", spv::CapabilityMatrix)
            .Case("Shader", spv::CapabilityShader)
            .Case("Geometry", spv::CapabilityGeometry)
            .Case("Tessellation", spv::CapabilityTessellation)
            .Case("Addresses", spv::CapabilityAddresses)
            .Case("Linkage", spv::CapabilityLinkage)
            .Case("Kernel", spv::CapabilityKernel)
            .Case("Vector16", spv::CapabilityVector16)
            .Case("Float16Buffer", spv::CapabilityFloat16Buffer)
            .Case("Float16", spv::CapabilityFloat16)
            .Case("Float64", spv::CapabilityFloat64)
            .Case("Int64", spv::CapabilityInt64)
            .Case("Int64Atomics", spv::CapabilityInt64Atomics)
            .Case("ImageBasic", spv::CapabilityImageBasic)
            .Case("ImageReadWrite", spv::CapabilityImageReadWrite)
            .Case("ImageMipmap", spv::CapabilityImageMipmap)
            .Case("Pipes", spv::CapabilityPipes)
            .Case("Groups", spv::CapabilityGroups)
            .Case("DeviceEnqueue", spv::CapabilityDeviceEnqueue)
            .Case("LiteralSampler", spv::CapabilityLiteralSampler)
            .Case("AtomicStorage", spv::CapabilityAtomicStorage)
            .Case("Int16", spv::CapabilityInt16)
            .Case("TessellationPointSize", spv::CapabilityTessellationPointSize)
            .Case("GeometryPointSize", spv::CapabilityGeometryPointSize)
            .Case("ImageGatherExtended", spv::CapabilityImageGatherExtended)
            .Case("StorageImageMultisample",
                  spv::CapabilityStorageImageMultisample)
            .Case("UniformBufferArrayDynamicIndexing",
                  spv::CapabilityUniformBufferArrayDynamicIndexing)
            .Case("SampledImageArrayDynamicIndexing",
                  spv::CapabilitySampledImageArrayDynamicIndexing)
            .Case("StorageBufferArrayDynamicIndexing",
                  spv::CapabilityStorageBufferArrayDynamicIndexing)
            .Case("StorageImageArrayDynamicIndexing",
                  spv::CapabilityStorageImageArrayDynamicIndexing)
            .Case("ClipDistance", spv::CapabilityClipDistance)
            .Case("CullDistance", spv::CapabilityCullDistance)
            .Case("ImageCubeArray", spv::CapabilityImageCubeArray)
            .Case("SampleRateShading", spv::CapabilitySampleRateShading)
            .Case("ImageRect", spv::CapabilityImageRect)
            .Case("SampledRect", spv::CapabilitySampledRect)
            .Case("GenericPointer", spv::CapabilityGenericPointer)
            .Case("Int8", spv::CapabilityInt8)
            .Case("InputAttachment", spv::CapabilityInputAttachment)
            .Case("SparseResidency", spv::CapabilitySparseResidency)
            .Case("MinLod", spv::CapabilityMinLod)
            .Case("Sampled1D", spv::CapabilitySampled1D)
            .Case("Image1D", spv::CapabilityImage1D)
            .Case("SampledCubeArray", spv::CapabilitySampledCubeArray)
            .Case("SampledBuffer", spv::CapabilitySampledBuffer)
            .Case("ImageBuffer", spv::CapabilityImageBuffer)
            .Case("ImageMSArray", spv::CapabilityImageMSArray)
            .Case("StorageImageExtendedFormats",
                  spv::CapabilityStorageImageExtendedFormats)
            .Case("ImageQuery", spv::CapabilityImageQuery)
            .Case("DerivativeControl", spv::CapabilityDerivativeControl)
            .Case("InterpolationFunction", spv::CapabilityInterpolationFunction)
            .Case("TransformFeedback", spv::CapabilityTransformFeedback)
            .Case("GeometryStreams", spv::CapabilityGeometryStreams)
            .Case("StorageImageReadWithoutFormat",
                  spv::CapabilityStorageImageReadWithoutFormat)
            .Case("StorageImageWriteWithoutFormat",
                  spv::CapabilityStorageImageWriteWithoutFormat)
            .Case("MultiViewport", spv::CapabilityMultiViewport)
            .Case("SubgroupBallotKHR", spv::CapabilitySubgroupBallotKHR)
            .Case("DrawParameters", spv::CapabilityDrawParameters)
            .Case("SubgroupVoteKHR", spv::CapabilitySubgroupVoteKHR)
            .Case("StorageBuffer16BitAccess",
                  spv::CapabilityStorageBuffer16BitAccess)
            .Case("StorageUniformBufferBlock16",
                  spv::CapabilityStorageUniformBufferBlock16)
            .Case("UniformAndStorageBuffer16BitAccess",
                  spv::CapabilityUniformAndStorageBuffer16BitAccess)
            .Case("StorageUniform16", spv::CapabilityStorageUniform16)
            .Case("StoragePushConstant16", spv::CapabilityStoragePushConstant16)
            .Case("StorageInputOutput16", spv::CapabilityStorageInputOutput16)
            .Case("DeviceGroup", spv::CapabilityDeviceGroup)
            .Case("MultiView", spv::CapabilityMultiView)
            .Case("VariablePointersStorageBuffer",
                  spv::CapabilityVariablePointersStorageBuffer)
            .Case("VariablePointers", spv::CapabilityVariablePointers)
            .Case("AtomicStorageOps", spv::CapabilityAtomicStorageOps)
            .Case("SampleMaskPostDepthCoverage",
                  spv::CapabilitySampleMaskPostDepthCoverage)
            .Case("ImageGatherBiasLodAMD", spv::CapabilityImageGatherBiasLodAMD)
            .Case("FragmentMaskAMD", spv::CapabilityFragmentMaskAMD)
            .Case("StencilExportEXT", spv::CapabilityStencilExportEXT)
            .Case("ImageReadWriteLodAMD", spv::CapabilityImageReadWriteLodAMD)
            .Case("SampleMaskOverrideCoverageNV",
                  spv::CapabilitySampleMaskOverrideCoverageNV)
            .Case("GeometryShaderPassthroughNV",
                  spv::CapabilityGeometryShaderPassthroughNV)
            .Case("ShaderViewportIndexLayerEXT",
                  spv::CapabilityShaderViewportIndexLayerEXT)
            .Case("ShaderViewportIndexLayerNV",
                  spv::CapabilityShaderViewportIndexLayerNV)
            .Case("ShaderViewportMaskNV", spv::CapabilityShaderViewportMaskNV)
            .Case("ShaderStereoViewNV", spv::CapabilityShaderStereoViewNV)
            .Case("PerViewAttributesNV", spv::CapabilityPerViewAttributesNV)
            .Case("SubgroupShuffleINTEL", spv::CapabilitySubgroupShuffleINTEL)
            .Case("SubgroupBufferBlockIOINTEL",
                  spv::CapabilitySubgroupBufferBlockIOINTEL)
            .Case("SubgroupImageBlockIOINTEL",
                  spv::CapabilitySubgroupImageBlockIOINTEL)
            .Default(static_cast<spv::Capability>(0));
    if (capability == 0) {
      std::cerr << "error: unknown capability: " << cargo::as<std::string>(cap)
                << "\n";
      std::exit(1);
    }
    deviceInfo.capabilities.push_back(capability);
  }
  if (enableAll) {
    deviceInfo.extensions.append({
        "SPV_KHR_16bit_storage",
        "SPV_KHR_float_controls",
        "SPV_KHR_no_integer_wrap_decoration",
        "SPV_KHR_storage_buffer_storage_class",
        "SPV_KHR_variable_pointers",
        "SPV_KHR_vulkan_memory_model",
    });
  } else {
    for (auto extension : extensions) {
      deviceInfo.extensions.push_back(cargo::as<std::string>(extension));
    }
  }
  return deviceInfo;
}
