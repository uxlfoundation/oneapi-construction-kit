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

#include <base/macros.h>
#include <base/program_metadata.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/pass_functions.h>
#include <compiler/utils/target_extension_types.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <multi_llvm/llvm_version.h>
#include <multi_llvm/vector_type_helper.h>

#include <sstream>

namespace compiler {
namespace {
bool isImageType(llvm::StringRef type_name, const char *const compare) {
  if (type_name == compare) {
    return true;
  }

  const llvm::StringRef ro = "__read_only ";
  if (type_name.starts_with(ro) && type_name.substr(ro.size()) == compare) {
    return true;
  }

  const llvm::StringRef wo = "__write_only ";
  if (type_name.starts_with(wo) && type_name.substr(wo.size()) == compare) {
    return true;
  }

  return false;
}

compiler::ArgumentType createIntegerOrSamplerType(
    uint32_t num_elements, uint32_t element_width,
    const llvm::MDString *metadata) {
  if (metadata) {
    if ((metadata->getString() == "sampler_t") && 1 == num_elements &&
        32 == element_width) {
      return {ArgumentKind::SAMPLER};
    }
  }
  switch (num_elements) {
    case 1:
      switch (element_width) {
        case 1:
          return {ArgumentKind::INT1};
        case 8:
          return {ArgumentKind::INT8};
        case 16:
          return {ArgumentKind::INT16};
        case 32:
          return {ArgumentKind::INT32};
        case 64:
          return {ArgumentKind::INT64};
        default:
          break;
      }
      break;
    case 2:
      switch (element_width) {
        case 8:
          return {ArgumentKind::INT8_2};
        case 16:
          return {ArgumentKind::INT16_2};
        case 32:
          return {ArgumentKind::INT32_2};
        case 64:
          return {ArgumentKind::INT64_2};
        default:
          break;
      }
      break;
    case 3:
      switch (element_width) {
        case 8:
          return {ArgumentKind::INT8_3};
        case 16:
          return {ArgumentKind::INT16_3};
        case 32:
          return {ArgumentKind::INT32_3};
        case 64:
          return {ArgumentKind::INT64_3};
        default:
          break;
      }
      break;
    case 4:
      switch (element_width) {
        case 8:
          return {ArgumentKind::INT8_4};
        case 16:
          return {ArgumentKind::INT16_4};
        case 32:
          return {ArgumentKind::INT32_4};
        case 64:
          return {ArgumentKind::INT64_4};
        default:
          break;
      }
      break;
    case 8:
      switch (element_width) {
        case 8:
          return {ArgumentKind::INT8_8};
        case 16:
          return {ArgumentKind::INT16_8};
        case 32:
          return {ArgumentKind::INT32_8};
        case 64:
          return {ArgumentKind::INT64_8};
        default:
          break;
      }
      break;
    case 16:
      switch (element_width) {
        case 8:
          return {ArgumentKind::INT8_16};
        case 16:
          return {ArgumentKind::INT16_16};
        case 32:
          return {ArgumentKind::INT32_16};
        case 64:
          return {ArgumentKind::INT64_16};
        default:
          break;
      }
      break;
    default:
      break;
  }

  CPL_ABORT("Unknown integer argument type.");

  return {};
}

ArgumentType createFloatingPointType(uint32_t num_elements,
                                     uint32_t element_width) {
  if (element_width == 16) {
    switch (num_elements) {
      case 1:
        return {ArgumentKind::HALF};
      case 2:
        return {ArgumentKind::HALF_2};
      case 3:
        return {ArgumentKind::HALF_3};
      case 4:
        return {ArgumentKind::HALF_4};
      case 8:
        return {ArgumentKind::HALF_8};
      case 16:
        return {ArgumentKind::HALF_16};
      default:
        break;
    }
  } else if (element_width == 32) {
    switch (num_elements) {
      case 1:
        return {ArgumentKind::FLOAT};
      case 2:
        return {ArgumentKind::FLOAT_2};
      case 3:
        return {ArgumentKind::FLOAT_3};
      case 4:
        return {ArgumentKind::FLOAT_4};
      case 8:
        return {ArgumentKind::FLOAT_8};
      case 16:
        return {ArgumentKind::FLOAT_16};
      default:
        break;
    }
  } else if (element_width == 64) {
    switch (num_elements) {
      case 1:
        return {ArgumentKind::DOUBLE};
      case 2:
        return {ArgumentKind::DOUBLE_2};
      case 3:
        return {ArgumentKind::DOUBLE_3};
      case 4:
        return {ArgumentKind::DOUBLE_4};
      case 8:
        return {ArgumentKind::DOUBLE_8};
      case 16:
        return {ArgumentKind::DOUBLE_16};
      default:
        break;
    }
  }

  CPL_ABORT("Unknown floating point argument type.");

  return {};
}

ArgumentType llvmArgToArgumentType(const llvm::Argument *arg,
                                   const llvm::MDString *metadata) {
  llvm::Type *Ty = arg->getType();

  // Handle pointer types.
  if (llvm::PointerType *PtrTy = llvm::dyn_cast<llvm::PointerType>(Ty)) {
    const auto type_name = metadata->getString();

    if (isImageType(type_name, "image2d_t")) {
      return {ArgumentKind::IMAGE2D};
    } else if (isImageType(type_name, "image3d_t")) {
      return {ArgumentKind::IMAGE3D};
    } else if (isImageType(type_name, "image2d_array_t")) {
      return {ArgumentKind::IMAGE2D_ARRAY};
    } else if (isImageType(type_name, "image1d_t")) {
      return {ArgumentKind::IMAGE1D};
    } else if (isImageType(type_name, "image1d_array_t")) {
      return {ArgumentKind::IMAGE1D_ARRAY};
    } else if (isImageType(type_name, "image1d_buffer_t")) {
      return {ArgumentKind::IMAGE1D_BUFFER};
    } else if (isImageType(type_name, "sampler_t")) {
      return {ArgumentKind::SAMPLER};
    } else {
      auto addressSpace = PtrTy->getAddressSpace();
      if (addressSpace == 0) {
        return {ArgumentKind::STRUCTBYVAL};
      }
      if (arg->hasAttribute(llvm::Attribute::Dereferenceable)) {
        auto deref_attr = arg->getAttribute(llvm::Attribute::Dereferenceable);
        return {addressSpace, deref_attr.getDereferenceableBytes()};
      }
      return {addressSpace};
    }
  }

  // Handle scalar types.
  llvm::IntegerType *IntTy = nullptr;
  switch (Ty->getTypeID()) {
    default:
      break;
    case llvm::Type::IntegerTyID:
      IntTy = llvm::cast<llvm::IntegerType>(Ty);
      return createIntegerOrSamplerType(1, IntTy->getBitWidth(), metadata);
    case llvm::Type::HalfTyID:
      return createFloatingPointType(1, 16);
    case llvm::Type::FloatTyID:
      return createFloatingPointType(1, 32);
    case llvm::Type::DoubleTyID:
      return createFloatingPointType(1, 64);
  }

  // Handle vector types.
  if (auto *VecTy = llvm::dyn_cast<llvm::FixedVectorType>(Ty)) {
    const uint32_t NumEle = VecTy->getNumElements();

    llvm::Type *EleTy = VecTy->getElementType();

    llvm::IntegerType *IntEleTy = nullptr;
    switch (EleTy->getTypeID()) {
      default:
        break;
      case llvm::Type::IntegerTyID:
        IntEleTy = llvm::cast<llvm::IntegerType>(EleTy);
        return createIntegerOrSamplerType(NumEle, IntEleTy->getBitWidth(),
                                          nullptr);
      case llvm::Type::HalfTyID:
        return createFloatingPointType(NumEle, 16);
      case llvm::Type::FloatTyID:
        return createFloatingPointType(NumEle, 32);
      case llvm::Type::DoubleTyID:
        return createFloatingPointType(NumEle, 64);
    }
  }

  if (auto *TgtTy = llvm::dyn_cast<llvm::TargetExtType>(Ty)) {
    auto TyName = TgtTy->getName();
    if (TyName == "spirv.Sampler") {
      return {ArgumentKind::SAMPLER};
    }

    if (TyName == "spirv.Image") {
      [[maybe_unused]] const auto type_name = metadata->getString();
      auto Dim =
          TgtTy->getIntParameter(utils::tgtext::ImageTyDimensionalityIdx);
      const bool Arrayed =
          TgtTy->getIntParameter(utils::tgtext::ImageTyArrayedIdx) ==
          utils::tgtext::ImageArrayed;
      switch (Dim) {
        default:
          CPL_ABORT("Unknown spirv.Image target extension type");
        case utils::tgtext::ImageDim1D:
          if (!Arrayed) {
            assert(isImageType(type_name, "image1d_t") &&
                   "Unexpected image type metadata");
            return {ArgumentKind::IMAGE1D};
          } else {
            assert(isImageType(type_name, "image1d_array_t") &&
                   "Unexpected image type metadata");
            return {ArgumentKind::IMAGE1D_ARRAY};
          }
        case utils::tgtext::ImageDim2D:
          if (!Arrayed) {
            assert(isImageType(type_name, "image2d_t") &&
                   "Unexpected image type metadata");
            return {ArgumentKind::IMAGE2D};
          } else {
            assert(isImageType(type_name, "image2d_array_t") &&
                   "Unexpected image type metadata");
            return {ArgumentKind::IMAGE2D_ARRAY};
          }
        case utils::tgtext::ImageDim3D:
          assert(isImageType(type_name, "image3d_t") &&
                 "Unexpected image type metadata");
          return {ArgumentKind::IMAGE3D};
        case utils::tgtext::ImageDimBuffer:
          assert(isImageType(type_name, "image1d_buffer_t") &&
                 "Unexpected image type metadata");
          return {ArgumentKind::IMAGE1D_BUFFER};
      }
    }

    CPL_ABORT("Unknown target extension type");
  }

  CPL_ABORT("Unknown argument type.");

  return {};
}

void populateRequiredWGSAttribute(KernelInfo &kernel_info, llvm::MDNode *node) {
  llvm::ValueAsMetadata *vmdOp1 =
      llvm::cast<llvm::ValueAsMetadata>(node->getOperand(1));
  llvm::ValueAsMetadata *vmdOp2 =
      llvm::cast<llvm::ValueAsMetadata>(node->getOperand(2));
  llvm::ValueAsMetadata *vmdOp3 =
      llvm::cast<llvm::ValueAsMetadata>(node->getOperand(3));
  llvm::ConstantInt *const operand1 =
      llvm::cast<llvm::ConstantInt>(vmdOp1->getValue());
  llvm::ConstantInt *const operand2 =
      llvm::cast<llvm::ConstantInt>(vmdOp2->getValue());
  llvm::ConstantInt *const operand3 =
      llvm::cast<llvm::ConstantInt>(vmdOp3->getValue());
  std::stringstream stream;
  kernel_info.reqd_work_group_size.emplace();
  (*kernel_info.reqd_work_group_size)[0] = *(operand1->getValue().getRawData());
  (*kernel_info.reqd_work_group_size)[1] = *(operand2->getValue().getRawData());
  (*kernel_info.reqd_work_group_size)[2] = *(operand3->getValue().getRawData());
  if (!kernel_info.attributes.empty()) {
    stream << kernel_info.attributes << " ";
  }
  stream << "reqd_work_group_size("
         << kernel_info.reqd_work_group_size.value()[0] << ","
         << kernel_info.reqd_work_group_size.value()[1] << ","
         << kernel_info.reqd_work_group_size.value()[2] << ")";
  kernel_info.attributes.assign(stream.str());
}

void populateWGSHintAttribute(KernelInfo &kernel_info, llvm::MDNode *node) {
  llvm::ValueAsMetadata *vmdOp1 =
      llvm::cast<llvm::ValueAsMetadata>(node->getOperand(1));
  llvm::ValueAsMetadata *vmdOp2 =
      llvm::cast<llvm::ValueAsMetadata>(node->getOperand(2));
  llvm::ValueAsMetadata *vmdOp3 =
      llvm::cast<llvm::ValueAsMetadata>(node->getOperand(3));
  llvm::ConstantInt *const operand1 =
      llvm::cast<llvm::ConstantInt>(vmdOp1->getValue());
  llvm::ConstantInt *const operand2 =
      llvm::cast<llvm::ConstantInt>(vmdOp2->getValue());
  llvm::ConstantInt *const operand3 =
      llvm::cast<llvm::ConstantInt>(vmdOp3->getValue());
  std::stringstream stream;
  size_t work_group_hint[3];
  work_group_hint[0] = *(operand1->getValue().getRawData());
  work_group_hint[1] = *(operand2->getValue().getRawData());
  work_group_hint[2] = *(operand3->getValue().getRawData());
  if (!kernel_info.attributes.empty()) {
    stream << kernel_info.attributes << " ";
  }
  stream << "work_group_size_hint(" << work_group_hint[0] << ","
         << work_group_hint[1] << "," << work_group_hint[2] << ")";
  kernel_info.attributes.assign(stream.str());
}

void populateVectorTypeHintAttribute(KernelInfo &kernel_info,
                                     llvm::MDNode *node) {
  std::stringstream stream;
  if (!kernel_info.attributes.empty()) {
    stream << kernel_info.attributes << " ";
  }
  stream << "vec_type_hint(";

  llvm::ValueAsMetadata *vmdOp1 =
      llvm::cast<llvm::ValueAsMetadata>(node->getOperand(1));
  llvm::ValueAsMetadata *vmdOp2 =
      llvm::cast<llvm::ValueAsMetadata>(node->getOperand(2));

  llvm::Type *type_hint = llvm::cast<llvm::Type>(vmdOp1->getType());
  // Can't use llvm::Type::getDescription(), have to do it manually
  const bool is_signed =
      1 == (llvm::cast<llvm::ConstantInt>(vmdOp2->getValue())->getZExtValue());
  llvm::Type *type_info = nullptr;
  if (type_hint->isVectorTy()) {
    type_info = multi_llvm::getVectorElementType(type_hint);
  } else {
    type_info = type_hint;
  }
  if (!is_signed && !type_info->isFloatingPointTy()) {
    stream << "u";
  }
  if (type_info->isDoubleTy()) {
    stream << "double";
  } else if (type_info->isFloatTy()) {
    stream << "float";
  } else if (type_info->isHalfTy()) {
    stream << "half";
  } else if (type_info->isIntegerTy(8)) {
    stream << "char";
  } else if (type_info->isIntegerTy(16)) {
    stream << "short";
  } else if (type_info->isIntegerTy(32)) {
    stream << "int";
  } else if (type_info->isIntegerTy(64)) {
    stream << "long";
  } else if (type_info->isIntegerTy(1)) {
    stream << "bool";
  } else if (type_info->isVoidTy()) {
    stream << "void";
  } else {  // Defaults to int
    stream << "int";
  }
  if (type_hint->isVectorTy()) {
    stream << multi_llvm::getVectorNumElements(type_hint);
  }
  stream << ")";
  kernel_info.attributes.assign(stream.str());
}

void populateAttributes(KernelInfo &kernel_info, llvm::MDNode *node) {
  for (uint32_t i = 1; i < node->getNumOperands(); ++i) {
    llvm::MDNode *const subNode = llvm::cast<llvm::MDNode>(node->getOperand(i));
    llvm::MDString *const operandName =
        llvm::cast<llvm::MDString>(subNode->getOperand(0));
    if (operandName->getString() == "reqd_work_group_size") {
      populateRequiredWGSAttribute(kernel_info, subNode);
    } else if (operandName->getString() == "work_group_size_hint") {
      populateWGSHintAttribute(kernel_info, subNode);
    } else if (operandName->getString() == "vec_type_hint") {
      populateVectorTypeHintAttribute(kernel_info, subNode);
    }
  }
}

/// @brief Populates kernel information from its LLVM IR.
///
/// @param[in] function The kernel function.
/// @param[in] node Kernel's subnode in the opencl.kernels metadata node.
/// @param[in] storeArgumentMetadata Whether to store additional argument
/// metadata as required by -cl-kernel-arg-info.
///
/// @return Return a KernelInfo object, or on errro a status code.
/// @retval `Result::SUCCESS` on success.
/// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
/// @retval `Result::FINALIZE_PROGRAM_FAILURE` when there was a problem with the
/// LLVM IR.
cargo::expected<KernelInfo, Result> populateKernelInfoFromFunction(
    llvm::Function *function, llvm::MDNode *node, bool storeArgumentMetadata) {
  KernelInfo kernel_info;
  kernel_info.name = function->getName().str();
  if (cargo::success !=
      kernel_info.argument_types.alloc(function->arg_size())) {
    return cargo::make_unexpected(Result::OUT_OF_MEMORY);
  }

  // Calculate the private memory size used by the kernel
  kernel_info.private_mem_size =
      compiler::utils::computeApproximatePrivateMemoryUsage(*function);

  // Find the index of arg base type node.
  bool kernel_arg_base_type_found = false;
  uint32_t kernel_arg_base_type_idx = 0;
  for (const auto &operand : node->operands()) {
    if (const llvm::MDNode *md_node =
            llvm::dyn_cast<const llvm::MDNode>(operand)) {
      if (const llvm::MDString *md_string =
              llvm::dyn_cast<const llvm::MDString>(md_node->getOperand(0))) {
        if (md_string->getString() == "kernel_arg_base_type") {
          kernel_arg_base_type_found = true;
          break;
        }
      }
    }
    ++kernel_arg_base_type_idx;
  }

  if (!kernel_arg_base_type_found) {
    return cargo::make_unexpected(Result::FINALIZE_PROGRAM_FAILURE);
  }

  // First operand contains kernel_arg_base_type.
  uint32_t arg_metadata_idx = 1;
  const llvm::MDNode *arg_node =
      llvm::dyn_cast<llvm::MDNode>(node->getOperand(kernel_arg_base_type_idx));
  assert(arg_node && "Could not get kernel arg base type node");
  for (llvm::Function::arg_iterator iter = function->arg_begin(),
                                    iter_end = function->arg_end();
       iter != iter_end; ++iter, ++arg_metadata_idx) {
    const llvm::MDString *arg_node_string = llvm::cast<const llvm::MDString>(
        arg_node->getOperand(arg_metadata_idx));
    kernel_info.argument_types[(*iter).getArgNo()] =
        llvmArgToArgumentType(&*iter, arg_node_string);
  }

  if (storeArgumentMetadata) {
    kernel_info.argument_info.emplace();

    for (uint32_t j = 1; j < node->getNumOperands(); ++j) {
      llvm::MDNode *const mdNode =
          llvm::cast<llvm::MDNode>(node->getOperand(j));
      const llvm::StringRef mdName =
          llvm::cast<llvm::MDString>(mdNode->getOperand(0))->getString();

      for (uint32_t k = 1; k < mdNode->getNumOperands(); ++k) {
        if (mdNode->getNumOperands() - 1 > kernel_info.argument_info->size()) {
          if (kernel_info.argument_info->resize(mdNode->getNumOperands() - 1) !=
              cargo::success) {
            kernel_info.argument_info = cargo::nullopt;
            return cargo::make_unexpected(Result::OUT_OF_MEMORY);
          }
        }
        KernelInfo::ArgumentInfo &info =
            kernel_info.argument_info.value()[k - 1];
        if ("kernel_arg_addr_space" == mdName) {
          llvm::ValueAsMetadata *vmdAddrQual =
              llvm::cast<llvm::ValueAsMetadata>(mdNode->getOperand(k));
          llvm::ConstantInt *const addressQual =
              llvm::cast<llvm::ConstantInt>(vmdAddrQual->getValue());

          switch (*addressQual->getValue().getRawData()) {
            case 0:
              info.address_qual = AddressSpace::PRIVATE;
              break;
            case 1:
              info.address_qual = AddressSpace::GLOBAL;
              break;
            case 2:
              info.address_qual = AddressSpace::CONSTANT;
              break;
            case 3:
              info.address_qual = AddressSpace::LOCAL;
              break;
            default:
              break;
          }
        } else if ("kernel_arg_access_qual" == mdName) {
          llvm::MDString *const accessQual =
              llvm::cast<llvm::MDString>(mdNode->getOperand(k));
          const llvm::StringRef access_qual_string = accessQual->getString();
          if ("none" == access_qual_string) {
            info.access_qual = KernelArgAccess::NONE;
          } else if ("read_only" == access_qual_string) {
            info.access_qual = KernelArgAccess::READ_ONLY;
          } else if ("write_only" == access_qual_string) {
            info.access_qual = KernelArgAccess::WRITE_ONLY;
          } else if ("read_write" == access_qual_string) {
            info.access_qual = KernelArgAccess::READ_WRITE;
          }
        } else if ("kernel_arg_type" == mdName) {
          llvm::MDString *const typeName =
              llvm::cast<llvm::MDString>(mdNode->getOperand(k));
          std::string name(typeName->getString().str());

          const std::string toErases[] = {"__read_only ", "__write_only "};

          for (const auto &erase : toErases) {
            auto pos = name.find(erase);
            if (std::string::npos != pos) {
              name.erase(pos, erase.size());
            }
          }

          info.type_name = name;
        } else if ("kernel_arg_type_qual" == mdName) {
          llvm::MDString *const typeQual =
              llvm::cast<llvm::MDString>(mdNode->getOperand(k));

          info.type_qual = KernelArgType::NONE;

          const llvm::StringRef typeAsString = typeQual->getString();

          if (typeAsString.find("const") != llvm::StringRef::npos) {
            info.type_qual |= KernelArgType::CONST;
          }
          if (typeAsString.find("restrict") != llvm::StringRef::npos) {
            info.type_qual |= KernelArgType::RESTRICT;
          }
          if (typeAsString.find("volatile") != llvm::StringRef::npos) {
            info.type_qual |= KernelArgType::VOLATILE;
          }
        } else if ("kernel_arg_name" == mdName) {
          llvm::MDString *const name =
              llvm::cast<llvm::MDString>(mdNode->getOperand(k));
          info.name = name->getString().str();
        }
      }
    }
  }

  populateAttributes(kernel_info, node);

  if (auto sub_group_size = compiler::utils::getReqdSubgroupSize(*function)) {
    kernel_info.reqd_sub_group_size = *sub_group_size;
  }

  return std::move(kernel_info);
}
}  // namespace

Result moduleToProgramInfo(ProgramInfo &program_info, llvm::Module *const M,
                           bool store_argument_metadata) {
  llvm::NamedMDNode *const node = M->getNamedMetadata("opencl.kernels");
  // Having no kernels isn't a failure.
  if (!node) {
    return Result::SUCCESS;
  }
  auto num_kernels = node->getNumOperands();
  if (num_kernels) {
    for (uint32_t i = 0, s = num_kernels; i < s; i++) {
      llvm::MDNode *const subNode = node->getOperand(i);
      llvm::ValueAsMetadata *vmd =
          llvm::cast<llvm::ValueAsMetadata>(subNode->getOperand(0));
      llvm::Function *const function =
          llvm::cast<llvm::Function>(vmd->getValue());

      auto kernel_info_result = populateKernelInfoFromFunction(
          function, subNode, store_argument_metadata);
      if (!kernel_info_result.has_value()) {
        return kernel_info_result.error();
      }
      if (!program_info.addNewKernel(std::move(*kernel_info_result))) {
        return Result::OUT_OF_MEMORY;
      }
    }
  }

  return Result::SUCCESS;
}
}  // namespace compiler
