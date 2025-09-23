// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <compiler/utils/builtin_info.h>
#include <compiler/utils/target_extension_types.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DebugLoc.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/MathExtras.h>
#include <llvm/Support/TypeSize.h>
#include <multi_llvm/llvm_version.h>
#include <multi_llvm/vector_type_helper.h>
#include <spirv-ll/assert.h>
#include <spirv-ll/builder.h>
#include <spirv-ll/builder_debug_info.h>
#include <spirv-ll/builder_group_async_copies.h>
#include <spirv-ll/builder_opencl.h>
#include <spirv-ll/context.h>
#include <spirv-ll/module.h>
#include <spirv-ll/opcodes.h>
#include <spirv/unified1/spirv.hpp>

#include <optional>
#include <unordered_map>

namespace spirv_ll {

template <>
llvm::Error Builder::create<OpNop>(const OpNop *) {
  // Intentional no-op
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpUndef>(const OpUndef *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  module.addID(op->IdResult(), op, llvm::UndefValue::get(type));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSourceContinued>(const OpSourceContinued *op) {
  module.appendSourceMetadataString(op->ContinuedSource().str());
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSourceExtension>(const OpSourceExtension *) {
  // This instruction has no semantic impact and doesn't represent any
  // information that is currently relevant to us.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpModuleProcessed>(const OpModuleProcessed *op) {
  // This instruction has no semantic impact. Take and store the 'Process' in
  // case it it's useful for debug information. We only store the one; any
  // subsequent ones will overwrite this.
  module.setModuleProcess(op->Process().str());
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSource>(const OpSource *op) {
  module.setSourceLanguage(op->SourceLanguage());

  if (module.getSourceMetadataString().size() > 0) {
    finalizeMetadata();
    module.setSourceMetadataString("");
  }

  std::string source = "Source language: ";
  switch (op->SourceLanguage()) {
    case spv::SourceLanguage::SourceLanguageESSL:
      source += "ESSL";
      break;
    case spv::SourceLanguage::SourceLanguageGLSL:
      source += "GLSL";
      break;
    case spv::SourceLanguage::SourceLanguageOpenCL_C:
      source += "OpenCL C";
      break;
    case spv::SourceLanguage::SourceLanguageOpenCL_CPP:
      source += "OpenCL C++";
      break;
    default:
      source += "Unknown";
      break;
  }
  source += ", Version: " + std::to_string(op->Version());

  if (op->wordCount() > 3) {
    if (auto file_path = module.getDebugString(op->File())) {
      source += ", Source file: " + file_path.value() + "\r\n";
    }

    if (op->wordCount() > 4) {
      source += op->Source();
    }
  }
  module.setSourceMetadataString(source);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpName>(const OpName *op) {
  module.addName(op->Target(), op->Name().str());
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpMemberName>(const OpMemberName *) {
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpString>(const OpString *op) {
  module.addDebugString(op->IdResult(), op->String().str());
  return llvm::Error::success();
}

llvm::DIFile *Builder::getOrCreateDIFile(const OpLine *op_line) {
  if (llvm::DIFile *file = module.getDIFile()) {
    return file;
  }

  const std::string filePath =
      module.getDebugString(op_line->File()).value_or("");
  const std::string fileName =
      filePath.substr(filePath.find_last_of("\\/") + 1);
  const std::string fileDir = filePath.substr(0, filePath.find_last_of("\\/"));

  llvm::DIFile *file = DIBuilder.createFile(fileName, fileDir);

  module.setDIFile(file);
  return file;
}

llvm::DICompileUnit *Builder::getOrCreateDICompileUnit(const OpLine *op_line) {
  if (llvm::DICompileUnit *compile_unit = module.getCompileUnit()) {
    return compile_unit;
  }

  auto *di_file = getOrCreateDIFile(op_line);

  llvm::DICompileUnit *compile_unit = DIBuilder.createCompileUnit(
      llvm::dwarf::DW_LANG_OpenCL, di_file, "", false, "", 0, "");

  module.setCompileUnit(compile_unit);
  return compile_unit;
}

llvm::DILexicalBlock *Builder::getOrCreateDebugBasicBlockScope(
    llvm::BasicBlock &bb, const OpLine *op_line) {
  if (auto *const di_block = module.getLexicalBlock(&bb)) {
    return di_block;
  }

  auto *const di_file = getOrCreateDIFile(op_line);
  auto *const function_scope =
      getOrCreateDebugFunctionScope(*bb.getParent(), op_line);
  auto *const di_block = DIBuilder.createLexicalBlock(
      function_scope, di_file, op_line->Line(), op_line->Column());
  module.addLexicalBlock(&bb, di_block);

  return di_block;
}

llvm::DISubprogram *Builder::getOrCreateDebugFunctionScope(
    llvm::Function &function, const OpLine *op_line) {
  const OpFunction *opFunction = module.get<OpFunction>(&function);
  // If we have a llvm::Function we should have an OpFunction.
  SPIRV_LL_ASSERT_PTR(opFunction);
  const spv::Id function_id = opFunction->IdResult();

  if (auto *function_scope = module.getDebugFunctionScope(function_id)) {
    return function_scope;
  }

  llvm::SmallVector<llvm::Metadata *, 4> dbg_function_param_types;

  const OpTypeFunction *opTypeFunction =
      module.get<OpTypeFunction>(opFunction->FunctionType());

  for (auto spv_ty_id : opTypeFunction->ParameterTypes()) {
    dbg_function_param_types.push_back(getDIType(spv_ty_id));
  }

  llvm::DISubroutineType *dbg_function_type = DIBuilder.createSubroutineType(
      DIBuilder.getOrCreateTypeArray(dbg_function_param_types));

  auto *di_file = getOrCreateDIFile(op_line);
  auto *di_compile_unit = getOrCreateDICompileUnit(op_line);

  // TODO: pass mangled name here when we're mangling names
  auto *function_scope = DIBuilder.createFunction(
      di_compile_unit, module.getName(opFunction->IdResult()),
      function.getName(), di_file, op_line->Line(), dbg_function_type, 1,
      llvm::DINode::FlagZero, llvm::DISubprogram::SPFlagDefinition);

  // Set the function's debug sub-program
  function.setSubprogram(function_scope);

  // Track this sub-program for later
  module.addDebugFunctionScope(function_id, function_scope);

  return function_scope;
}

template <>
llvm::Error Builder::create<OpLine>(const OpLine *op) {
  // Close the current range, if applicable.
  // Note we don't close the current range afterwards, since we'll just
  // overwrite it with a new one a few lines down.
  applyDebugInfoAtClosedRangeOrScope();

  llvm::Function *current_function = getCurrentFunction();

  if (!current_function || !IRBuilder.GetInsertBlock()) {
    setCurrentOpLineRange(LineRangeBeginTy{op, /*range_begin*/ std::nullopt});
    return llvm::Error::success();
  }

  llvm::BasicBlock &bb = *IRBuilder.GetInsertBlock();

  // If there aren't any instructions in the basic block yet just go from the
  // start of the block.
  llvm::BasicBlock::iterator iter =
      bb.empty() ? bb.begin() : bb.back().getIterator();

  setCurrentOpLineRange(LineRangeBeginTy{op, iter});

  return llvm::Error::success();
}

void Builder::closeCurrentLexicalScope(bool closing_line_range) {
  // Apply debug info to the previous scope.
  applyDebugInfoAtClosedRangeOrScope();
  // Close the current op line range, unless this is a lexical scope. In this
  // case, we keep any OpLine/OpNoLine range that's active, as we may later
  // open a new lexical scope inside the same range:
  //  OpLine
  //    DebugScope
  //    DebugNoScope <- we may be here
  //    ...
  //    DebugScope
  //    DebugNoScope
  //  OpNoLine
  if (closing_line_range) {
    setCurrentOpLineRange(std::nullopt);
  }
  // Close any lexical scope that's active
  setCurrentFunctionLexicalScope(std::nullopt);
}

void Builder::applyDebugInfoAtClosedRangeOrScope() {
  auto line_range = getCurrentOpLineRange();
  // If we don't have line information, we can bail here.
  if (!line_range) {
    return;
  }

  const auto *op_line = line_range->op_line;
  llvm::BasicBlock *bb = IRBuilder.GetInsertBlock();

  // If we don't have a block of instructions to apply
  // debug information to, we can bail here.
  if (!bb || bb->empty()) {
    // If we have a function but haven't attached a sub-program to it, manifest
    // and attach one now. It's arguable how useful this is (in the case that
    // we only have empty line ranges in a function but attach a sub-program to
    // it anyway).
    if (auto *f = getCurrentFunction()) {
      getOrCreateDebugFunctionScope(*f, op_line);
    }
    return;
  }

  const llvm::BasicBlock::iterator range_begin =
      std::next(line_range->range_begin.value_or(bb->begin()));
  const llvm::BasicBlock::iterator range_end = IRBuilder.GetInsertPoint();

  llvm::Metadata *scope = nullptr;
  llvm::Metadata *inlined_at = nullptr;

  if (auto lexical_scope = getCurrentFunctionLexicalScope()) {
    scope = lexical_scope->scope;
    inlined_at = lexical_scope->inlined_at;
  } else if (module.useImplicitDebugScopes()) {
    scope = getOrCreateDebugBasicBlockScope(*bb, op_line);
  }

  if (scope) {
    auto *const di_loc =
        llvm::DILocation::get(*context.llvmContext, op_line->Line(),
                              op_line->Column(), scope, inlined_at);

    for (auto &inst : make_range(range_begin, range_end)) {
      inst.setDebugLoc(di_loc);
    }
  }

  // Update the current line range to start where the range currently ends -
  // we've added debug info to everything before this point.
  setCurrentOpLineRange(
      LineRangeBeginTy{line_range->op_line, std::prev(range_end)});
}

template <>
llvm::Error Builder::create<OpExtension>(const OpExtension *op) {
  auto extension = op->Name();
  if (std::none_of(deviceInfo.extensions.begin(), deviceInfo.extensions.end(),
                   [&](const std::string &deviceExtension) {
                     return extension == deviceExtension;
                   })) {
    return makeStringError("OpExtension " + extension.str() +
                           " not supported by device");
  }
  module.declareExtension(extension);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpExtInstImport>(const OpExtInstImport *op) {
  auto name = op->Name();
  if (name == "OpenCL.std") {
    registerExtInstHandler<OpenCLBuilder>(ExtendedInstrSet::OpenCL);
    module.associateExtendedInstrSet(op->IdResult(), ExtendedInstrSet::OpenCL);
  } else if (name == "Codeplay.GroupAsyncCopies" ||
             name == "NonSemantic.Codeplay.GroupAsyncCopies") {
    registerExtInstHandler<GroupAsyncCopiesBuilder>(
        ExtendedInstrSet::GroupAsyncCopies);
    module.associateExtendedInstrSet(op->IdResult(),
                                     ExtendedInstrSet::GroupAsyncCopies);
  } else if (name == "DebugInfo") {
    // Work around a known llvm-spirv bug, until it's generally fixed upstream.
    registerExtInstHandler<DebugInfoBuilder>(
        ExtendedInstrSet::DebugInfo,
        DebugInfoBuilder::Workarounds::
            TemplateTemplateSwappedWithParameterPack);
    module.disableImplicitDebugScopes();
    module.associateExtendedInstrSet(op->IdResult(),
                                     ExtendedInstrSet::DebugInfo);
  } else if (name == "OpenCL.DebugInfo.100") {
    // Work around a known llvm-spirv bug, until it's generally fixed upstream.
    registerExtInstHandler<DebugInfoBuilder>(
        ExtendedInstrSet::OpenCLDebugInfo100,
        DebugInfoBuilder::Workarounds::
            TemplateTemplateSwappedWithParameterPack);
    module.disableImplicitDebugScopes();
    module.associateExtendedInstrSet(op->IdResult(),
                                     ExtendedInstrSet::OpenCLDebugInfo100);
  } else {
    return makeStringError(name +
                           " extended instruction set is not supported!\n");
  }
  return llvm::Error::success();
}

spirv_ll::ExtInstSetHandler *Builder::getExtInstHandler(
    ExtendedInstrSet set) const {
  auto handler_it = ext_inst_handlers.find(set);
  if (handler_it == ext_inst_handlers.end()) {
    return nullptr;
  }
  return handler_it->second.get();
}

template <>
llvm::Error Builder::create<OpExtInst>(const OpExtInst *op) {
  if (auto *handler =
          getExtInstHandler(module.getExtendedInstrSet(op->Set()))) {
    return handler->create(*op);
  }
  return makeStringError("Could not find extended instruction set for ID " +
                         std::to_string(op->Set()));
}

template <>
llvm::Error Builder::create<OpMemoryModel>(const OpMemoryModel *op) {
  bool addressingModelValid;
  switch (deviceInfo.addressingModel) {
    case spv::AddressingModel::AddressingModelLogical:
    case spv::AddressingModel::AddressingModelPhysical32:
    case spv::AddressingModel::AddressingModelPhysical64:
      addressingModelValid =
          op->AddressingModel() == deviceInfo.addressingModel ||
          op->AddressingModel() == spv::AddressingModel::AddressingModelLogical;
      break;
    default:
      addressingModelValid = false;
      break;
  }
  if (!addressingModelValid) {
    return makeStringError("OpMemoryModel AddressingModel " +
                           std::to_string(op->AddressingModel()) +
                           " not supported by device");
  }
  switch (op->AddressingModel()) {
    case spv::AddressingModel::AddressingModelLogical:
      module.setAddressingModel(0);
      break;
    case spv::AddressingModel::AddressingModelPhysical32:
      module.setAddressingModel(32);
      break;
    case spv::AddressingModel::AddressingModelPhysical64:
      module.setAddressingModel(64);
      break;
    default:
      llvm_unreachable("Unsupported value provided for addressing model.");
  }

#if LLVM_VERSION_GREATER_EQUAL(21, 0)
  module.llvmModule->setTargetTriple(llvm::Triple("unknown-unknown-unknown"));
#else
  module.llvmModule->setTargetTriple("unknown-unknown-unknown");
#endif

  const char *dataLayout32 =
      "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:"
      "64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-"
      "v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024";
  const char *dataLayout64 =
      "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:"
      "64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-"
      "v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024";
  if (op->AddressingModel() == spv::AddressingModelPhysical32 ||
      (op->AddressingModel() == spv::AddressingModelLogical &&
       deviceInfo.addressBits == 32)) {
    module.llvmModule->setDataLayout(dataLayout32);
  } else if (op->AddressingModel() == spv::AddressingModelPhysical64 ||
             (op->AddressingModel() == spv::AddressingModelLogical &&
              deviceInfo.addressBits == 64)) {
    module.llvmModule->setDataLayout(dataLayout64);
  }
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpEntryPoint>(const OpEntryPoint *op) {
  module.addName(op->EntryPoint(), op->Name().str());
  module.addEntryPoint(op);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpExecutionMode>(const OpExecutionMode *op) {
  module.addExecutionMode(op);
  switch (op->Mode()) {
    case spv::ExecutionMode::ExecutionModeLocalSize: {
      llvm::SmallVector<uint32_t, 3> workgroup_size;
      for (uint32_t wgs_index = 0; wgs_index < 3; wgs_index++) {
        workgroup_size.push_back(op->getValueAtOffset(3 + wgs_index));
      }
      module.setWGS(workgroup_size[0], workgroup_size[1], workgroup_size[2]);
      break;
    }
    default:
      break;
  }
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpCapability>(const OpCapability *op) {
  auto capability = op->Capability();
  if (std::none_of(deviceInfo.capabilities.begin(),
                   deviceInfo.capabilities.end(),
                   [&](spv::Capability deviceCapability) {
                     return capability == deviceCapability;
                   })) {
    return makeStringError("OpCapability " + getCapabilityName(capability) +
                           " not supported by device");
  }
  module.enableCapability(capability);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTypeVoid>(const OpTypeVoid *op) {
  module.addID(op->IdResult(), op, IRBuilder.getVoidTy());
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTypeBool>(const OpTypeBool *op) {
  module.addID(op->IdResult(), op, IRBuilder.getInt1Ty());
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTypeInt>(const OpTypeInt *op) {
  module.addID(op->IdResult(), op, IRBuilder.getIntNTy(op->Width()));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTypeFloat>(const OpTypeFloat *op) {
  if (op->Width() == 16) {
    module.addID(op->IdResult(), op, IRBuilder.getHalfTy());
  } else if (op->Width() == 32) {
    module.addID(op->IdResult(), op, IRBuilder.getFloatTy());
  } else if (op->Width() == 64) {
    module.addID(op->IdResult(), op, IRBuilder.getDoubleTy());
  }
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTypeVector>(const OpTypeVector *op) {
  llvm::Type *componentType = module.getLLVMType(op->ComponentType());

  SPIRV_LL_ASSERT_PTR(componentType);

  module.addID(op->IdResult(), op,
               llvm::FixedVectorType::get(componentType, op->ComponentCount()));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTypeMatrix>(const OpTypeMatrix *op) {
  llvm::Type *columnType = module.getLLVMType(op->ColumnType());

  SPIRV_LL_ASSERT_PTR(columnType);

  module.addID(op->IdResult(), op,
               llvm::ArrayType::get(columnType, op->ColumnCount()));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTypeImage>(const OpTypeImage *op) {
  llvm::Type *imageType = nullptr;
  auto &ctx = *context.llvmContext;

  switch (op->Dim()) {
    default:
      break;
    case spv::Dim::Dim1D:
      if (op->Arrayed() == 1) {
        imageType = compiler::utils::tgtext::getImage1DArrayTy(ctx);
      } else {
        imageType = compiler::utils::tgtext::getImage1DTy(ctx);
      }
      break;
    case spv::Dim::Dim2D:
      if (op->Arrayed() == 1) {
        imageType = compiler::utils::tgtext::getImage2DArrayTy(ctx);
      } else {
        imageType = compiler::utils::tgtext::getImage2DTy(ctx);
      }
      break;
    case spv::Dim::Dim3D:
      imageType = compiler::utils::tgtext::getImage3DTy(ctx);
      break;
    case spv::Dim::DimBuffer:
      imageType = compiler::utils::tgtext::getImage1DBufferTy(ctx);
      break;
  }

  if (!imageType) {
    return makeStringError(llvm::formatv(
        "Unsupported type (Dim = {0}) passed to 'create<OpTypeImage>'\n",
        op->Dim()));
  }

  module.addID(op->IdResult(), op, imageType);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTypeSampler>(const OpTypeSampler *op) {
  module.addID(op->IdResult(), op,
               compiler::utils::tgtext::getSamplerTy(*context.llvmContext));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTypeSampledImage>(const OpTypeSampledImage *) {
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTypeArray>(const OpTypeArray *op) {
  llvm::Type *elementType = module.getLLVMType(op->ElementType());
  llvm::Value *length = module.getValue(op->Length());

  SPIRV_LL_ASSERT_PTR(elementType);
  SPIRV_LL_ASSERT_PTR(length);

  llvm::ConstantInt *lengthCst = llvm::cast<llvm::ConstantInt>(length);

  module.addID(op->IdResult(), op,
               llvm::ArrayType::get(elementType, lengthCst->getZExtValue()));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTypeRuntimeArray>(const OpTypeRuntimeArray *op) {
  llvm::Type *elementType = module.getLLVMType(op->ElementType());

  SPIRV_LL_ASSERT_PTR(elementType);

  module.addID(op->IdResult(), op, llvm::ArrayType::get(elementType, 0));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTypeStruct>(const OpTypeStruct *op) {
  bool forwardDeclared = false;
  llvm::SmallVector<spv::Id, 4> memberTypeIDs;
  llvm::SmallVector<spv::Id, 2> forwardPointerIDs;

  for (auto memberType : op->MemberTypes()) {
    if (module.isForwardPointer(memberType)) {
      forwardDeclared = true;
      forwardPointerIDs.push_back(memberType);
      continue;
    }
    memberTypeIDs.push_back(memberType);
  }

  if (forwardDeclared) {
    module.addIncompleteStruct(op, forwardPointerIDs);
    module.addID(op->IdResult(), op,
                 llvm::StructType::create(*context.llvmContext));
  } else {
    llvm::SmallVector<llvm::Type *, 4> memberTypes;

    for (auto memberTypeID : memberTypeIDs) {
      llvm::Type *memberType = module.getLLVMType(memberTypeID);

      SPIRV_LL_ASSERT_PTR(memberType);

      memberTypes.push_back(memberType);
    }

    bool isPacked = false;
    if (module.hasCapability(spv::CapabilityKernel) &&
        module.getFirstDecoration(op->IdResult(), spv::DecorationCPacked)) {
      isPacked = true;
    }
    auto structTy = llvm::StructType::create(
        memberTypes, module.getName(op->IdResult()), isPacked);
    SPIRV_LL_ASSERT_PTR(structTy);

    module.addID(op->IdResult(), op, structTy);
  }
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTypeOpaque>(const OpTypeOpaque *op) {
  module.addID(op->IdResult(), op,
               llvm::StructType::create(*context.llvmContext, op->Name()));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTypePointer>(const OpTypePointer *op) {
  const spv::Id typeId = op->Type();
  if (module.isForwardPointer(typeId)) {
    module.addIncompletePointer(op, typeId);
  } else {
    if (auto err = module.addCompletePointer(op)) {
      return err;
    }
  }
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTypeFunction>(const OpTypeFunction *op) {
  llvm::Type *returnType = module.getLLVMType(op->ReturnType());
  SPIRV_LL_ASSERT_PTR(returnType);

  llvm::SmallVector<llvm::Type *, 4> paramTypes;
  llvm::SmallVector<spv::Id, 4> paramTypeIds;

  for (int i = 0, n = op->wordCount() - 3; i < n; ++i) {
    auto opTyID = op->ParameterTypes()[i];
    llvm::Type *paramType = module.getLLVMType(opTyID);
    SPIRV_LL_ASSERT_PTR(paramType);

    paramTypes.push_back(paramType);
    paramTypeIds.push_back(opTyID);
  }

  llvm::FunctionType *functionType =
      llvm::FunctionType::get(returnType, paramTypes, false);
  const auto id = op->IdResult();

  module.setParamTypeIDs(id, paramTypeIds);
  module.addID(id, op, functionType);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTypeEvent>(const OpTypeEvent *op) {
  module.addID(op->IdResult(), op,
               compiler::utils::tgtext::getEventTy(*context.llvmContext));
  return llvm::Error::success();
}

static llvm::Error errorUnsupportedDeviceEnqueueOp(const std::string &opName) {
  // Capability DeviceEnqueue isn't supported by CL 1.2, see OpenCL SPIR-V
  // environment spec section 3.1 for supported capabilities.
  // It is, however, implicitly declared by SPIR-V 1.1 modules which declare the
  // SubgroupDispatch capability for CL 3.0 devices supporting subgroups. This
  // is highly dubious, and appears like a spec bug of some kind.
  return makeStringError(opName + " is not supported.");
}

template <>
llvm::Error Builder::create<OpTypeDeviceEvent>(const OpTypeDeviceEvent *) {
  return errorUnsupportedDeviceEnqueueOp("OpTypeDeviceEvent");
}

template <>
llvm::Error Builder::create<OpTypeReserveId>(const OpTypeReserveId *) {
  // Capability Pipes isn't supported by CL 1.2, see OpenCL SPIR-V
  // environment spec section 6.1 for supported capabilities.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTypeQueue>(const OpTypeQueue *) {
  return errorUnsupportedDeviceEnqueueOp("OpTypeQueue");
}

template <>
llvm::Error Builder::create<OpTypePipe>(const OpTypePipe *) {
  // Capability Pipes isn't supported by CL 1.2, see OpenCL SPIR-V
  // environment spec section 6.1 for supported capabilities.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTypeForwardPointer>(
    const OpTypeForwardPointer *op) {
  module.addForwardPointer(op->PointerType());
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpConstantTrue>(const OpConstantTrue *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());

  SPIRV_LL_ASSERT_PTR(type);

  llvm::Constant *constant = llvm::ConstantInt::get(type, 1);
  constant->setName(module.getName(op->IdResult()));

  module.addID(op->IdResult(), op, constant);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpConstantFalse>(const OpConstantFalse *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());

  SPIRV_LL_ASSERT_PTR(type);

  llvm::Constant *constant = llvm::ConstantInt::get(type, 0);
  constant->setName(module.getName(op->IdResult()));

  module.addID(op->IdResult(), op, constant);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpConstant>(const OpConstant *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());

  SPIRV_LL_ASSERT_PTR(type);

  uint64_t value;

  if (type->isDoubleTy() || type->isIntegerTy(64)) {
    value = op->Value64();
  } else {
    value = op->Value32();
  }

  llvm::Constant *constant = nullptr;

  if (type->isFloatingPointTy()) {
    SPIRV_LL_ASSERT(type->getScalarSizeInBits() == 16 ||
                        type->getScalarSizeInBits() == 32 ||
                        type->getScalarSizeInBits() == 64,
                    "Unsupported floating point type!");

    if (type->getScalarSizeInBits() == 32) {
      float fval;
      memcpy(&fval, &value, sizeof(float));
      constant = llvm::ConstantFP::get(type, fval);
    } else if (type->getScalarSizeInBits() == 64) {
      double dval;
      memcpy(&dval, &value, sizeof(double));
      constant = llvm::ConstantFP::get(type, dval);
    } else if (type->getScalarSizeInBits() == 16) {
      // For half we need to copy the bit pattern out and get it into an
      // `APFloat` with the appropriate `fltSemantics` for half.
      uint16_t hval = 0;
      memcpy(&hval, &value, sizeof(uint16_t));
      auto hvalAP =
          llvm::APFloat(llvm::APFloat::IEEEhalf(), llvm::APInt(16, hval));
      constant = llvm::ConstantFP::get(type, hvalAP);
    } else {
      llvm_unreachable("Constant floating point op has an impossible size");
    }
  } else if (type->isIntegerTy()) {
    constant = llvm::ConstantInt::get(type, value);
  } else {
    llvm_unreachable("Constant op has an impossible type");
  }

  constant->setName(module.getName(op->IdResult()));
  module.addID(op->IdResult(), op, constant);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpConstantComposite>(
    const OpConstantComposite *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());

  SPIRV_LL_ASSERT_PTR(type);

  llvm::SmallVector<llvm::Constant *, 4> constituents;

  // the number of elements in the list of IDs can be obtained by subtracting
  // the word count for the main instruction (3) from the total word count
  for (uint16_t cIndex = 0, cEnd = op->wordCount() - 3; cIndex < cEnd;
       cIndex++) {
    llvm::Value *c = module.getValue(op->Constituents()[cIndex]);
    SPIRV_LL_ASSERT_PTR(c);

    constituents.push_back(llvm::cast<llvm::Constant>(c));
  }

  llvm::Constant *constant = nullptr;

  if (type->isVectorTy()) {
    constant = llvm::ConstantVector::get(constituents);
  } else if (type->isStructTy()) {
    constant = llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(type),
                                         constituents);
  } else if (type->isArrayTy()) {
    constant = llvm::ConstantArray::get(llvm::cast<llvm::ArrayType>(type),
                                        constituents);
  } else {
    llvm_unreachable("Constant composite op has an impossible type");
  }

  constant->setName(module.getName(op->IdResult()));
  module.addID(op->IdResult(), op, constant);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpConstantSampler>(const OpConstantSampler *op) {
  // Translate SPIR-V enums into values from SPIR 1.2 spec Table 4
  // https://www.khronos.org/registry/SPIR/specs/spir_spec-1.2.pdf
  static const uint32_t addressingModes[] = {
      0x0000,  // CLK_ADDRESS_NONE
      0x0002,  // CLK_ADDRESS_CLAMP_TO_EDGE
      0x0004,  // CLK_ADDRESS_CLAMP
      0x0006,  // CLK_ADDRESS_REPEAT
      0x0008,  // CLK_ADDRESS_MIRRORED_REPEAT
  };
  static const uint32_t normalizedCoords[] = {
      0x0000,  // CLK_NORMALIZED_COORDS_FALSE
      0x0001,  // CLK_NORMALIZED_COORDS_TRUE
  };
  static const uint32_t filterModes[] = {
      0x0010,  // CLK_FILTER_NEAREST
      0x0020,  // CLK_FILTER_LINEAR
  };
  const uint32_t samplerValue = addressingModes[op->SamplerAddressingMode()] |
                                normalizedCoords[op->Param()] |
                                filterModes[op->SamplerFilterMode()];

  // Note that samplers should actually be pointers to target extension types
  // (or opaque structure types before LLVM 17).
  // We internally store constant samplers as their i32 initializers, then, in
  // the only place that can use them (OpSampledImage) we translate them to the
  // proper type via a builtin call.
  llvm::Constant *constSampler =
      llvm::ConstantInt::get(IRBuilder.getInt32Ty(), samplerValue);
  SPIRV_LL_ASSERT_PTR(constSampler);

  module.addID(op->IdResult(), op, constSampler);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpConstantNull>(const OpConstantNull *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());

  SPIRV_LL_ASSERT_PTR(type);

  llvm::Constant *constant = nullptr;

  switch (type->getTypeID()) {
    case llvm::Type::TypeID::HalfTyID:
    case llvm::Type::TypeID::FloatTyID:
    case llvm::Type::TypeID::DoubleTyID:
      constant = llvm::ConstantFP::get(type, 0);
      break;
    case llvm::Type::TypeID::IntegerTyID:
      constant = llvm::ConstantInt::get(type, 0);
      break;
    case llvm::Type::TypeID::StructTyID:
    case llvm::Type::TypeID::ArrayTyID:
      constant = llvm::ConstantAggregateZero::get(type);
      break;
    case llvm::Type::FixedVectorTyID: {
      auto *vecTy = llvm::cast<llvm::FixedVectorType>(type);
      llvm::Constant *element = nullptr;
      if (vecTy->getElementType()->isIntegerTy()) {
        element = llvm::ConstantInt::get(vecTy->getElementType(), 0);
      } else if (vecTy->getElementType()->isFloatingPointTy()) {
        element = llvm::ConstantFP::get(vecTy->getElementType(), 0.0);
      }
      const uint32_t numElements = vecTy->getNumElements();
      constant = llvm::ConstantVector::getSplat(
          llvm::ElementCount::getFixed(numElements), element);
      break;
    }
    case llvm::Type::TypeID::PointerTyID:
      constant =
          llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(type));
      break;
    case llvm::Type::TypeID::TargetExtTyID:
      // Only Events may be zero-initialized.
      if (llvm::cast<llvm::TargetExtType>(type)->getName() == "spirv.Event") {
        constant = llvm::ConstantTargetNone::get(
            llvm::cast<llvm::TargetExtType>(type));
        break;
      }
      [[fallthrough]];
    default:
      // TODO: the opencl types: device event, reservation ID and queue
      llvm_unreachable("Unsupported type provided to OpConstantNull");
  }
  constant->setName(module.getName(op->IdResult()));
  module.addID(op->IdResult(), op, constant);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSpecConstantTrue>(const OpSpecConstantTrue *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Constant *spec_constant = nullptr;
  if (auto specId = module.getSpecId(op->IdResult())) {
    if (auto specInfo = module.getSpecInfo()) {
      if (specInfo->isSpecialized(*specId)) {
        // Constant has been specialized, get value and create a new constant.
        if (module.hasCapability(spv::CapabilityKernel)) {
          // OpenCL SPIR-V spec constant bool is 8 bits.
          auto value = specInfo->getValue<uint8_t>(*specId);
          SPIRV_LL_ASSERT(value, value.error().message.c_str());
          spec_constant = llvm::ConstantInt::get(type, *value);
        } else {
          // Vulkan SPIR-V spec constant bool is 32 bits.
          auto value = specInfo->getValue<uint32_t>(*specId);
          SPIRV_LL_ASSERT(value, value.error().message.c_str());
          spec_constant = llvm::ConstantInt::get(type, *value);
        }
      }
    }
  }
  if (!spec_constant) {
    spec_constant = IRBuilder.getTrue();
  }

  module.addID(op->IdResult(), op, spec_constant);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSpecConstantFalse>(
    const OpSpecConstantFalse *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Constant *spec_constant = nullptr;
  if (auto specId = module.getSpecId(op->IdResult())) {
    if (auto specInfo = module.getSpecInfo()) {
      if (specInfo->isSpecialized(*specId)) {
        // Constant has been specialized, get value and create a new constant.
        if (module.hasCapability(spv::CapabilityKernel)) {
          // OpenCL SPIR-V spec constant bool is 8 bits.
          auto value = specInfo->getValue<uint8_t>(*specId);
          SPIRV_LL_ASSERT(value, value.error().message.c_str());
          spec_constant = llvm::ConstantInt::get(type, *value);
        } else {
          // Vulkan SPIR-V spec constant bool is 32 bits.
          auto value = specInfo->getValue<uint32_t>(*specId);
          SPIRV_LL_ASSERT(value, value.error().message.c_str());
          spec_constant = llvm::ConstantInt::get(type, *value);
        }
      }
    }
  }
  if (!spec_constant) {
    spec_constant = IRBuilder.getFalse();
  }

  module.addID(op->IdResult(), op, spec_constant);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSpecConstant>(const OpSpecConstant *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  uint64_t value;
  if (type->getScalarSizeInBits() > 32) {
    value = op->Value64();
  } else {
    value = op->Value32();
  }

  llvm::Constant *spec_constant = nullptr;

  if (auto specId = module.getSpecId(op->IdResult())) {
    if (auto specInfo = module.getSpecInfo()) {
      if (specInfo->isSpecialized(*specId)) {
        int size = type->getScalarSizeInBits();
        switch (size) {
          case 1:
            if (module.hasCapability(spv::CapabilityKernel)) {
              // OpenCL SPIR-V spec constant bool is 8 bits.
              size = 8;
            } else {
              // Vulkan SPIR-V spec constant bool is 32 bits.
              size = 32;
            }
            break;
          case 8:
            if (!module.hasCapability(spv::CapabilityKernel)) {
              // Vulkan SPIR-V does not support 8 bit integers.
              size = -1;
            }
            break;
          default:
            break;
        }
        // SpecializationInfo::getValue does not require the type to match, it
        // merely requires the type to have the correct size. Use integer types
        // for everything to avoid a need for the host compiler to support
        // device types.
        switch (size) {
          case 8: {
            auto specValue = specInfo->getValue<uint8_t>(*specId);
            SPIRV_LL_ASSERT(specValue, specValue.error().message.c_str());
            value = *specValue;
          } break;
          case 16: {
            auto specValue = specInfo->getValue<uint16_t>(*specId);
            SPIRV_LL_ASSERT(specValue, specValue.error().message.c_str());
            value = *specValue;
          } break;
          case 32: {
            auto specValue = specInfo->getValue<uint32_t>(*specId);
            SPIRV_LL_ASSERT(specValue, specValue.error().message.c_str());
            value = *specValue;
          } break;
          case 64: {
            auto specValue = specInfo->getValue<uint64_t>(*specId);
            SPIRV_LL_ASSERT(specValue, specValue.error().message.c_str());
            value = *specValue;
          } break;
          default:
            llvm_unreachable("Invalid type provided to OpSpecConstant");
            break;
        }
      }
    }
  }

  if (type->isIntegerTy()) {
    spec_constant = llvm::ConstantInt::get(type, value);
  } else if (type->isFloatingPointTy()) {
    spec_constant = llvm::ConstantFP::get(
        type, llvm::APFloat(type->getFltSemantics(),
                            llvm::APInt(type->getScalarSizeInBits(), value)));
  } else {
    llvm_unreachable("Invalid type provided to OpSpecConstant");
  }

  module.addID(op->IdResult(), op, spec_constant);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSpecConstantComposite>(
    const OpSpecConstantComposite *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::SmallVector<llvm::Constant *, 4> constituents;

  for (int32_t c_index = 0; c_index < op->wordCount() - 3; c_index++) {
    llvm::Constant *constituent = llvm::cast<llvm::Constant>(
        module.getValue(op->Constituents()[c_index]));
    constituents.push_back(constituent);
  }

  llvm::Constant *spec_constant_composite = nullptr;

  switch (type->getTypeID()) {
    case llvm::Type::FixedVectorTyID:
      spec_constant_composite = llvm::ConstantVector::get(constituents);
      break;
    case llvm::Type::ArrayTyID:
      spec_constant_composite = llvm::ConstantArray::get(
          llvm::cast<llvm::ArrayType>(type), constituents);
      break;
    case llvm::Type::StructTyID:
      spec_constant_composite = llvm::ConstantStruct::get(
          llvm::cast<llvm::StructType>(type), constituents);
      break;
    default:
      llvm_unreachable(
          "Non-composite type supplied to OpSpecConstantComposite");
      break;
  }

  if (auto op_decorate =
          module.getFirstDecoration(op->IdResult(), spv::DecorationBuiltIn)) {
    const spv::BuiltIn builtin = spv::BuiltIn(op_decorate->getValueAtOffset(3));
    if (builtin == spv::BuiltInWorkgroupSize) {
      SPIRV_LL_ASSERT(constituents.size() == 3,
                      "OpSpecConstantComposite invalid number of constituents");
      module.setWGS(
          llvm::cast<llvm::ConstantInt>(constituents[0])->getZExtValue(),
          llvm::cast<llvm::ConstantInt>(constituents[1])->getZExtValue(),
          llvm::cast<llvm::ConstantInt>(constituents[2])->getZExtValue());
    }
  }

  module.addID(op->IdResult(), op, spec_constant_composite);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSpecConstantOp>(const OpSpecConstantOp *op) {
  llvm::Type *result_type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(result_type);

  llvm::Value *result = nullptr;

  const int firstArgIndex = 4;
  const int secondArgIndex = 5;
  const int thirdArgIndex = 6;

  switch (op->Opcode()) {
    case spv::OpSConvert: {
      llvm::Value *operand =
          module.getValue(op->getValueAtOffset(firstArgIndex));

      result = IRBuilder.CreateIntCast(
          operand, result_type,
          llvm::cast<llvm::IntegerType>(result_type)->getSignBit());
      break;
    }
    case spv::OpFConvert: {
      llvm::Value *operand =
          module.getValue(op->getValueAtOffset(firstArgIndex));

      result = IRBuilder.CreateFPCast(operand, result_type);
      break;
    }
    case spv::OpSNegate: {
      llvm::Value *operand =
          module.getValue(op->getValueAtOffset(firstArgIndex));

      result = IRBuilder.CreateNeg(operand);
      break;
    }
    case spv::OpNot: {
      llvm::Value *operand =
          module.getValue(op->getValueAtOffset(firstArgIndex));

      result = IRBuilder.CreateNot(operand);
      break;
    }
    case spv::OpIAdd: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateAdd(lhs, rhs);
      break;
    }
    case spv::OpISub: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateSub(lhs, rhs);
      break;
    }
    case spv::OpIMul: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateMul(lhs, rhs);
      break;
    }
    case spv::OpUDiv: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateUDiv(lhs, rhs);
      break;
    }
    case spv::OpSDiv: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateSDiv(lhs, rhs);
      break;
    }
    case spv::OpUMod: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateURem(lhs, rhs);
      break;
    }
    case spv::OpSRem: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateSRem(lhs, rhs);
      break;
    }
    case spv::OpSMod: {
      llvm::Value *num = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *denom =
          module.getValue(op->getValueAtOffset(secondArgIndex));

      llvm::Constant *zero = llvm::ConstantInt::getSigned(result_type, 0);
      llvm::Value *cmp = IRBuilder.CreateICmpSLT(denom, zero);
      llvm::Value *absDenom =
          IRBuilder.CreateSelect(cmp, IRBuilder.CreateNeg(denom), denom);

      llvm::Value *sRem = IRBuilder.CreateSRem(num, denom);
      result = IRBuilder.CreateSelect(cmp, IRBuilder.CreateAdd(sRem, absDenom),
                                      sRem);
      break;
    }
    case spv::OpShiftRightLogical: {
      llvm::Value *value = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *shift =
          module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateLShr(value, shift);
      break;
    }
    case spv::OpShiftRightArithmetic: {
      llvm::Value *value = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *shift =
          module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateAShr(value, shift);
      break;
    }
    case spv::OpShiftLeftLogical: {
      llvm::Value *value = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *shift =
          module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateShl(value, shift);
      break;
    }
    case spv::OpBitwiseOr: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateOr(lhs, rhs);
      break;
    }
    case spv::OpBitwiseXor: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateXor(lhs, rhs);
      break;
    }
    case spv::OpBitwiseAnd: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateAnd(lhs, rhs);
      break;
    }
    case spv::OpVectorShuffle: {
      llvm::Value *v1 = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *v2 = module.getValue(op->getValueAtOffset(secondArgIndex));
      llvm::SmallVector<int, 4> components;
      // base word count is four plus the two used for the vector operands
      for (int comp_index = 0; comp_index < op->wordCount() - thirdArgIndex;
           comp_index++) {
        // FIXME: wording in the spec is weird here, all operands must be
        // IDs of
        // constants but in the actual shuffle vector instruction these are
        // literals, determine which applies here
        const int component =
            static_cast<int>(op->getValueAtOffset(thirdArgIndex + comp_index));
        components.push_back(component);
      }

      result = IRBuilder.CreateShuffleVector(v1, v2, components);
      break;
    }
    case spv::OpCompositeExtract: {
      llvm::Value *composite =
          module.getValue(op->getValueAtOffset(firstArgIndex));

      if (composite->getType()->isVectorTy()) {
        const uint32_t index = op->getValueAtOffset(secondArgIndex);

        result = IRBuilder.CreateExtractElement(composite, index);
      } else {
        llvm::SmallVector<uint32_t, 2> indexes;

        for (int i = 0; i < op->wordCount() - secondArgIndex; i++) {
          const uint32_t index = op->getValueAtOffset(secondArgIndex + i);

          indexes.push_back(index);
        }

        result = IRBuilder.CreateExtractValue(composite, indexes);
      }
      break;
    }
    case spv::OpCompositeInsert: {
      llvm::Value *object =
          module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *composite =
          module.getValue(op->getValueAtOffset(secondArgIndex));

      if (result_type->isVectorTy()) {
        const uint32_t index = op->getValueAtOffset(thirdArgIndex);

        result = IRBuilder.CreateInsertElement(composite, object, index);
      } else {
        llvm::SmallVector<uint32_t, 2> indexes;

        for (int i = 0; i < op->wordCount() - thirdArgIndex; i++) {
          const uint32_t index = op->getValueAtOffset(thirdArgIndex + i);

          indexes.push_back(index);
        }

        result = IRBuilder.CreateInsertValue(composite, object, indexes);
      }
      break;
    }
    case spv::OpLogicalOr: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateOr(lhs, rhs);
      break;
    }
    case spv::OpLogicalAnd: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateAnd(lhs, rhs);
      break;
    }
    case spv::OpLogicalNot: {
      llvm::Value *operand =
          module.getValue(op->getValueAtOffset(firstArgIndex));

      result = IRBuilder.CreateNot(operand);
      break;
    }
    case spv::OpLogicalEqual: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      // boolean values are represented as 1 bit integers so the icmp
      // instructions can be used
      result = IRBuilder.CreateICmpEQ(lhs, rhs);
      break;
    }
    case spv::OpLogicalNotEqual: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateICmpNE(lhs, rhs);
      break;
    }
    case spv::OpSelect: {
      llvm::Value *condition =
          module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *object_1 =
          module.getValue(op->getValueAtOffset(secondArgIndex));

      llvm::Value *object_2 =
          module.getValue(op->getValueAtOffset(thirdArgIndex));

      result = IRBuilder.CreateSelect(condition, object_1, object_2);
      break;
    }
    case spv::OpIEqual: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateICmpEQ(lhs, rhs);
      break;
    }
    case spv::OpINotEqual: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateICmpNE(lhs, rhs);
      break;
    }
    case spv::OpULessThan: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateICmpULT(lhs, rhs);
      break;
    }
    case spv::OpSLessThan: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateICmpSLT(lhs, rhs);
      break;
    }
    case spv::OpUGreaterThan: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateICmpUGT(lhs, rhs);
      break;
    }
    case spv::OpSGreaterThan: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateICmpSGT(lhs, rhs);
      break;
    }
    case spv::OpULessThanEqual: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateICmpULE(lhs, rhs);
      break;
    }
    case spv::OpSLessThanEqual: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateICmpSLE(lhs, rhs);
      break;
    }
    case spv::OpUGreaterThanEqual: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateICmpUGE(lhs, rhs);
      break;
    }
    case spv::OpSGreaterThanEqual: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateICmpSGE(lhs, rhs);
      break;
    }
    case spv::OpConvertFToS: {
      llvm::Value *val = module.getValue(op->getValueAtOffset(firstArgIndex));

      result = IRBuilder.CreateFPToSI(val, result_type);
      break;
    }
    case spv::OpConvertSToF: {
      llvm::Value *val = module.getValue(op->getValueAtOffset(firstArgIndex));

      result = IRBuilder.CreateSIToFP(val, result_type);
      break;
    }
    case spv::OpConvertFToU: {
      llvm::Value *val = module.getValue(op->getValueAtOffset(firstArgIndex));

      result = IRBuilder.CreateFPToUI(val, result_type);
      break;
    }
    case spv::OpConvertUToF: {
      llvm::Value *val = module.getValue(op->getValueAtOffset(firstArgIndex));

      result = IRBuilder.CreateUIToFP(val, result_type);
      break;
    }
    case spv::OpUConvert: {
      llvm::Value *val = module.getValue(op->getValueAtOffset(firstArgIndex));

      result = IRBuilder.CreateZExtOrTrunc(val, result_type);
      break;
    }
    case spv::OpConvertPtrToU: {
      llvm::Value *val = module.getValue(op->getValueAtOffset(firstArgIndex));

      result = IRBuilder.CreatePtrToInt(val, result_type);
      break;
    }
    case spv::OpConvertUToPtr: {
      llvm::Value *val = module.getValue(op->getValueAtOffset(firstArgIndex));

      result = IRBuilder.CreateIntToPtr(val, result_type);
      break;
    }
    case spv::OpGenericCastToPtr: {
      llvm::Value *val = module.getValue(op->getValueAtOffset(firstArgIndex));

      result = IRBuilder.CreatePointerCast(val, result_type);
      break;
    }
    case spv::OpPtrCastToGeneric: {
      llvm::Value *val = module.getValue(op->getValueAtOffset(firstArgIndex));

      result = IRBuilder.CreatePointerCast(val, result_type);
      break;
    }
    case spv::OpBitcast: {
      llvm::Value *val = module.getValue(op->getValueAtOffset(firstArgIndex));

      result = IRBuilder.CreateBitCast(val, result_type);
      break;
    }
    case spv::OpFNegate: {
      llvm::Value *val = module.getValue(op->getValueAtOffset(firstArgIndex));

      result = IRBuilder.CreateFNeg(val);
      break;
    }
    case spv::OpFAdd: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateFAdd(lhs, rhs);
      break;
    }
    case spv::OpFSub: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateFSub(lhs, rhs);
      break;
    }
    case spv::OpFMul: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateFMul(lhs, rhs);
      break;
    }
    case spv::OpFDiv: {
      llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));

      llvm::Value *rhs = module.getValue(op->getValueAtOffset(secondArgIndex));

      result = IRBuilder.CreateFDiv(lhs, rhs);
      break;
    }
    case spv::OpFRem: {
      // Defer the op so we can call the fmod builtin.
      module.deferSpecConstantOp(op);

      return llvm::Error::success();
      break;
    }
    case spv::OpAccessChain:
    case spv::OpPtrAccessChain:
    case spv::OpInBoundsAccessChain:
    case spv::OpInBoundsPtrAccessChain: {
      llvm::Value *base = module.getValue(op->getValueAtOffset(firstArgIndex));
      SPIRV_LL_ASSERT_PTR(base);
      auto *pointerTy = module.get<OpTypePointer>(op->IdResultType());
      SPIRV_LL_ASSERT(pointerTy, "Result type is not a pointer");

      llvm::SmallVector<llvm::Value *, 8> indexes;

      // If this isn't a PtrAccessChain we need an additional dereference at the
      // start.
      if (op->Opcode() == spv::OpAccessChain ||
          op->Opcode() == spv::OpInBoundsAccessChain) {
        indexes.push_back(IRBuilder.getInt32(0));
      }

      for (int32_t i = 0; i < op->wordCount() - secondArgIndex; i++) {
        llvm::Value *index =
            module.getValue(op->getValueAtOffset(secondArgIndex + i));

        indexes.push_back(index);
      }

      auto elementType =
          module.getLLVMType(pointerTy->getTypePointer()->Type());
      SPIRV_LL_ASSERT_PTR(elementType);
      if (elementType->isStructTy()) {
        checkMemberDecorations(elementType, indexes, op->IdResult());
      }

      result = IRBuilder.CreateGEP(elementType, base, indexes);

      // Set inbounds if this is an inbounds instruction.
      if (op->Opcode() == spv::OpInBoundsAccessChain ||
          op->Opcode() == spv::OpInBoundsPtrAccessChain) {
        llvm::cast<llvm::GetElementPtrInst>(result)->setIsInBounds();
      }

      break;
    }
    // FMod can't be translated here because a call to our copysign builtin is
    // needed, and builtin calls can't be generated outside the scope of a
    // function, so defer the translation.
    case spv::OpFMod: {
      module.deferSpecConstantOp(op);
      return llvm::Error::success();
    }
    default:
      llvm_unreachable("Invalid OpCode given to OpSpecConstantOp");
  }
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

static std::optional<std::pair<uint32_t, const char *>> getLinkage(
    Module &module, spv::Id id) {
  if (auto decoration =
          module.getFirstDecoration(id, spv::DecorationLinkageAttributes)) {
    // the actual linkage enum comes after a string literal, but it's the
    // last operand so just work backwards from the end
    auto linkageOffset = decoration->wordCount() - 1;
    return std::make_pair(
        decoration->getValueAtOffset(linkageOffset),
        spirv_ll::cast<const OpDecorate>(decoration)->getDecorationString());
  }
  return std::nullopt;
}

template <>
llvm::Error Builder::create<OpFunction>(const OpFunction *op) {
  // get function type
  auto *function_type = llvm::dyn_cast<llvm::FunctionType>(
      module.getLLVMType(op->FunctionType()));

  SPIRV_LL_ASSERT_PTR(function_type);

  // get name
  std::string name = module.getName(op->IdResult());

  // get linkage
  llvm::Function::LinkageTypes linkage =
      llvm::Function::LinkageTypes::PrivateLinkage;

  if (op->FunctionControl() & spv::FunctionControlInlineMask) {
    linkage = llvm::Function::LinkageTypes::LinkOnceODRLinkage;
  } else if (auto linkageInfo = getLinkage(module, op->IdResult())) {
    if (linkageInfo->first == spv::LinkageTypeImport ||
        linkageInfo->first == spv::LinkageTypeExport ||
        linkageInfo->first == spv::LinkageTypeLinkOnceODR) {
      if (linkageInfo->first == spv::LinkageTypeLinkOnceODR) {
        SPIRV_LL_ASSERT(
            module.isExtensionEnabled("SPV_KHR_linkonce_odr"),
            "SPV_KHR_linkonce_odr must be enabled to use LinkOnceODRLinkage");
        linkage = llvm::Function::LinkageTypes::LinkOnceODRLinkage;
      } else {
        linkage = llvm::Function::LinkageTypes::ExternalLinkage;
      }
      // always use the linkage name when we have one
      name = linkageInfo->second;
    }
  }

  llvm::Function *function = nullptr;
  llvm::Function *kernel_function = nullptr;

  if (auto ep_op = module.getEntryPoint(op->IdResult())) {
    // It is possible to identically name multiple functions through different
    // ways: OpName, OpEntryPoint, and OpDecorate LinkageAttributes.
    // We prioritize the naming of OpEntryPoints higher than that of other
    // functions, since those names are likely expected by an external party.
    // OpName is purely decorative, and we don't yet correctly handle the clash
    // between LinkageName and the othe two.
    // Thus, if the module already contains a function with the same name,
    // rename that other function.
    if (auto *old_fn = module.llvmModule->getFunction(name)) {
      old_fn->setName(old_fn->getName() + ".old");
    }

    switch (ep_op->ExecutionModel()) {
      case spv::ExecutionModelKernel: {
        kernel_function = llvm::Function::Create(
            function_type, llvm::GlobalValue::LinkageTypes::ExternalLinkage,
            name, module.llvmModule.get());
        kernel_function->setCallingConv(llvm::CallingConv::SPIR_KERNEL);

        // The kernel argument metadata will be populated in OpFunctionEnd when
        // all the information is available, setting these to empty lists here
        // so they exist when a kernel has no arguments and the order of
        // metadata matches the output of clang.
        kernel_function->setMetadata(
            "kernel_arg_addr_space",
            llvm::MDNode::get(*context.llvmContext, {}));
        kernel_function->setMetadata(
            "kernel_arg_access_qual",
            llvm::MDNode::get(*context.llvmContext, {}));
        kernel_function->setMetadata(
            "kernel_arg_type", llvm::MDNode::get(*context.llvmContext, {}));
        kernel_function->setMetadata(
            "kernel_arg_base_type",
            llvm::MDNode::get(*context.llvmContext, {}));
        kernel_function->setMetadata(
            "kernel_arg_type_qual",
            llvm::MDNode::get(*context.llvmContext, {}));
        kernel_function->setMetadata(
            "kernel_arg_name", llvm::MDNode::get(*context.llvmContext, {}));

        for (auto executionMode : module.getExecutionModes(op->IdResult())) {
          switch (executionMode->Mode()) {
            case spv::ExecutionModeLocalSize:
              // Specify the required work group size.
              kernel_function->setMetadata(
                  "reqd_work_group_size",
                  llvm::MDNode::get(
                      *context.llvmContext,
                      {
                          llvm::ConstantAsMetadata::get(IRBuilder.getInt32(
                              executionMode->getValueAtOffset(3))),
                          llvm::ConstantAsMetadata::get(IRBuilder.getInt32(
                              executionMode->getValueAtOffset(4))),
                          llvm::ConstantAsMetadata::get(IRBuilder.getInt32(
                              executionMode->getValueAtOffset(5))),
                      }));
              break;
            case spv::ExecutionModeLocalSizeHint:
              // Speficy the work group size hint.
              kernel_function->setMetadata(
                  "work_group_size_hint",
                  llvm::MDNode::get(
                      *context.llvmContext,
                      {
                          llvm::ConstantAsMetadata::get(IRBuilder.getInt32(
                              executionMode->getValueAtOffset(3))),
                          llvm::ConstantAsMetadata::get(IRBuilder.getInt32(
                              executionMode->getValueAtOffset(4))),
                          llvm::ConstantAsMetadata::get(IRBuilder.getInt32(
                              executionMode->getValueAtOffset(5))),
                      }));
              break;
            case spv::ExecutionModeVecTypeHint: {
              const uint32_t vectorType = executionMode->getValueAtOffset(3);
              //  The 16 high-order bits of Vector Type operand specify the
              //  number of components of the vector.
              uint16_t numElements = (vectorType & 0xFFFF0000) >> 16;
              // Supported vector Component Counts are 2, 3, 4, 8, or 16.
              // 0 or 1 represents a scalar hint
              SPIRV_LL_ASSERT(
                  numElements <= 16 &&
                      (numElements == 0 || llvm::isPowerOf2_32(numElements) ||
                       numElements == 3),
                  "OpExecutionMode VecTypeHint invalid number of components");
              // The 16 low-order bits of Vector Type operand specify the data
              // type of the vector.
              const uint16_t dataType = vectorType & 0x0000FFFF;
              // llvm-spirv encodes scalar hints as vectors of length 0 rather
              // than 1. This is an upsteam bug that may be resolved to encode
              // the legnth as 1, so here we handle both cases.
              numElements = std::max(numElements, static_cast<uint16_t>(1));
              llvm::Type *vecTypeHint;
              switch (dataType) {
                case 0:  // 8-bit integer value
                  vecTypeHint = llvm::FixedVectorType::get(
                      llvm::IntegerType::get(*context.llvmContext, 8),
                      numElements);
                  break;
                case 1:  // 16-bit integer value
                  vecTypeHint = llvm::FixedVectorType::get(
                      llvm::IntegerType::get(*context.llvmContext, 16),
                      numElements);
                  break;
                case 2:  // 32-bit integer value
                  vecTypeHint = llvm::FixedVectorType::get(
                      llvm::IntegerType::get(*context.llvmContext, 32),
                      numElements);
                  break;
                case 3:  // 64-bit integer value
                  vecTypeHint = llvm::FixedVectorType::get(
                      llvm::IntegerType::get(*context.llvmContext, 64),
                      numElements);
                  break;
                case 4:  // 16-bit float value
                  vecTypeHint = llvm::FixedVectorType::get(
                      llvm::Type::getHalfTy(*context.llvmContext), numElements);
                  break;
                case 5:  // 32-bit float value
                  vecTypeHint = llvm::FixedVectorType::get(
                      llvm::Type::getFloatTy(*context.llvmContext),
                      numElements);
                  break;
                case 6:  // 64-bit float value
                  vecTypeHint = llvm::FixedVectorType::get(
                      llvm::Type::getDoubleTy(*context.llvmContext),
                      numElements);
                  break;
                default:
                  llvm_unreachable(
                      "OpExecutionMode VecTypeHint invalid vector type");
              }

              kernel_function->setMetadata(
                  "vec_type_hint",
                  llvm::MDNode::get(
                      *context.llvmContext,
                      {
                          llvm::ConstantAsMetadata::get(
                              llvm::PoisonValue::get(vecTypeHint)),
                          // The OpenCL SPIR-V spec does not handle the
                          // signed integer case, so this flag is always 0.
                          llvm::ConstantAsMetadata::get(IRBuilder.getInt32(0)),
                      }));
            } break;
            case spv::ExecutionModeContractionOff:
              // Contraction is impossible at IR level as there are no
              // contracted operation instructions. In LLVM it is possible to
              // explicitly request that the backend attempt contraction, but
              // not to explicitly disallow it, so all we can do here is make
              // sure we aren't explicitly requesting contraction.
              if (IRBuilder.getFastMathFlags().allowContract()) {
                llvm::FastMathFlags newFlags(IRBuilder.getFastMathFlags());
                newFlags.setAllowContract(false);
                IRBuilder.setFastMathFlags(newFlags);
              }
              break;
            case spv::ExecutionModeMaxWorkDimINTEL: {
              const uint32_t maxDim = executionMode->getValueAtOffset(3);
              // Specify the max work group dim.
              kernel_function->setMetadata(
                  "max_work_dim",
                  llvm::MDNode::get(*context.llvmContext,
                                    {llvm::ConstantAsMetadata::get(
                                        IRBuilder.getInt32(maxDim))}));
            } break;
            case spv::ExecutionModeSubgroupSize: {
              const uint32_t sgSize = executionMode->getValueAtOffset(3);
              // Specify the required sub group size.
              kernel_function->setMetadata(
                  "intel_reqd_sub_group_size",
                  llvm::MDNode::get(*context.llvmContext,
                                    {llvm::ConstantAsMetadata::get(
                                        IRBuilder.getInt32(sgSize))}));
              break;
            }
            case spv::ExecutionModeSubgroupsPerWorkgroup:
              // We declare we support SubgroupDispatch but really we only do so
              // to handle SubgroupSize.
              return makeStringError(
                  "Execution Mode SubgroupsPerWorkgroup is not supported.");
            case spv::ExecutionModeSubgroupsPerWorkgroupId:
              // We declare we support SubgroupDispatch but really we only do so
              // to handle SubgroupSize.
              return makeStringError(
                  "Execution Mode SubgroupsPerWorkgroupId is not supported.");
            default:
              break;
          }
        }

        function = llvm::Function::Create(function_type, linkage, name,
                                          module.llvmModule.get());
        function->setCallingConv(llvm::CallingConv::SPIR_FUNC);

        llvm::SmallVector<llvm::Value *, 4> kernel_args;
        kernel_args.reserve(kernel_function->arg_size());
        for (auto &kernel_arg : kernel_function->args()) {
          kernel_args.push_back(&kernel_arg);
        }

        IRBuilder.SetInsertPoint(llvm::BasicBlock::Create(
            *context.llvmContext, "entry", kernel_function));
        auto *call = IRBuilder.CreateCall(function_type, function, kernel_args);
        call->setCallingConv(function->getCallingConv());
        IRBuilder.CreateRetVoid();
        IRBuilder.ClearInsertionPoint();
      } break;
      default:
        return makeStringError(
            llvm::formatv("Execution model (ID = {0}) is not supported",
                          ep_op->ExecutionModel()));
    }
  } else {
    // DPC++ rejects variadic functions in SYCL code, with the exception of
    // __builtin_printf which it accepts and generates invalid SPIR-V for that
    // calls printf, but declares printf as a non-variadic function (because
    // SPIR-V has no variadic functions) yet calls it with the normal arguments.
    // Patch this up.
    // We may not strictly infer for SPIR-V code that printf is the standard
    // library function printf, but we only aim to support OpenCL C and SYCL
    // which do allow us to make assumptions here, and SPIR-V generated from
    // GLSL which appends a "(" to function names so is not affected.
    if (name == "printf" && function_type->getNumParams() == 1 &&
        !function_type->isVarArg()) {
      function_type = llvm::FunctionType::get(function_type->getReturnType(),
                                              {function_type->getParamType(0)},
                                              /*isVarArg=*/true);
    }

    function = llvm::Function::Create(function_type, linkage, name,
                                      module.llvmModule.get());
    function->setCallingConv(llvm::CallingConv::SPIR_FUNC);
  }

  SPIRV_LL_ASSERT_PTR(function);

  if (op->FunctionControl() & spv::FunctionControlOptNoneINTELMask) {
    SPIRV_LL_ASSERT(module.hasCapability(spv::CapabilityOptNoneINTEL),
                    "CapabilityOptNoneINTEL must be enabled.");
    function->addFnAttr(llvm::Attribute::OptimizeNone);
    function->addFnAttr(llvm::Attribute::NoInline);
  }

  // For kernel entry points, all parameters can be decorated with noundef; it
  // is not valid for the host to pass undefined/poison bits to kernels. Note
  // that in a correct SPIR-V module, it is invalid for a function to call an
  // entry point, so entry points are truly reserved only for calling from the
  // host.
  // FIXME: This would ideally be done on all functions. However, we're
  // currently translating some well-defined programs SPIR-V to a "more
  // poisonous" LLVM IR (e.g., see how OpShiftRightLogical produces an
  // "undefined value" for out-of-bounds shifts, whereas LLVM's lshr produces a
  // poison value). We don't want to pass poison to a 'noundef' function
  // parameter when translating an otherwise correct SPIR-V module.
  if (kernel_function) {
    for (auto &arg : kernel_function->args()) {
      arg.addAttr(llvm::Attribute::NoUndef);
    }
  }

  setCurrentFunction(function, kernel_function);

  // Add the ID before calling getOrCreateDebugFunctionScope below, so we can
  // easily retrieve the OpFunction directly from the function.
  module.addID(op->IdResult(), op, function);

  if (auto *function_scope = module.getDebugFunctionScope(op->IdResult())) {
    // If we've created the scope before creating the function, link the two
    // together here
    function->setSubprogram(function_scope);
  } else if (auto current_range = getCurrentOpLineRange();
             current_range && module.useImplicitDebugScopes()) {
    // Else, if there's a line range currently open at this point, create and
    // attach a synthetic DISubprogram for this function. If there isn't, we'll
    // generate one on the fly when we hit an OpLine but it'll have that
    // OpLine's line/column information.
    getOrCreateDebugFunctionScope(*function, current_range->op_line);
  }

  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFunctionParameter>(
    const OpFunctionParameter *op) {
  SPIRV_LL_ASSERT_PTR(getCurrentFunction());
  auto *function_arg = popFunctionArg();
  SPIRV_LL_ASSERT_PTR(function_arg);

  llvm::SmallVector<llvm::Argument *, 2> args;

  args.push_back(function_arg);
  if (auto *kernel_function = getCurrentFunctionKernel()) {
    args.push_back(kernel_function->getArg(function_arg->getArgNo()));
  }

  for (auto *arg : args) {
    arg->setName(module.getName(op->IdResult()));
    llvm::AttrBuilder attrs(arg->getContext());

    if (module.hasCapability(spv::CapabilityKernel)) {
      for (auto funcParamAttr : module.getDecorations(
               op->IdResult(), spv::DecorationFuncParamAttr)) {
        // Attributes are only applicable to certain types.
        if (arg->getType()->isIntegerTy()) {
          switch (funcParamAttr->getValueAtOffset(3)) {
            case spv::FunctionParameterAttributeZext:
              attrs.addAttribute(llvm::Attribute::ZExt);
              break;
            case spv::FunctionParameterAttributeSext:
              attrs.addAttribute(llvm::Attribute::SExt);
              break;
            default:
              return makeStringError(
                  "Invalid function parameter attribute for integer type.");
          }
        } else if (arg->getType()->isPointerTy()) {
          auto *ty = module.get<OpType>(op->IdResultType());
          SPIRV_LL_ASSERT(ty->isPointerType(), "Parameter is not a pointer");
          auto *paramTy = module.getLLVMType(ty->getTypePointer()->Type());
          SPIRV_LL_ASSERT_PTR(paramTy);
          switch (funcParamAttr->getValueAtOffset(3)) {
            case spv::FunctionParameterAttributeByVal:
              attrs.addByValAttr(paramTy);
              attrs.addAlignmentAttr(1);
              break;
            case spv::FunctionParameterAttributeSret:
              attrs.addStructRetAttr(paramTy);
              attrs.addAlignmentAttr(1);
              break;
            case spv::FunctionParameterAttributeNoAlias:
              attrs.addAttribute(llvm::Attribute::NoAlias);
              break;
            case spv::FunctionParameterAttributeNoCapture:
#if LLVM_VERSION_GREATER_EQUAL(21, 0)
              attrs.addCapturesAttr(llvm::CaptureInfo::none());
#else
              attrs.addAttribute(llvm::Attribute::NoCapture);
#endif
              break;
            case spv::FunctionParameterAttributeNoWrite:
              attrs.addAttribute(llvm::Attribute::ReadOnly);
              break;
            case spv::FunctionParameterAttributeNoReadWrite:
              attrs.addAttribute(llvm::Attribute::WriteOnly);
              break;
            default:
              return makeStringError(
                  "Invalid function parameter attribute for pointer type.");
          }
        }
      }
      // Add Dereferenceable attribute to arg if SPIRV is decorated with
      // MaxByteOffset
      if (arg->getType()->isPointerTy()) {
        for (auto maxBufSize : module.getDecorations(
                 op->IdResult(), spv::DecorationMaxByteOffset)) {
          auto derefAttr = llvm::Attribute::get(
              arg->getContext(), llvm::Attribute::Dereferenceable,
              maxBufSize->getValueAtOffset(3));
          attrs.addAttribute(derefAttr);
        }
      }
    }

    // NonReadable and NonWritable only apply to OpTypeImage.
    auto opResultType = module.getResultType(op);
    if (opResultType->isImageType()) {
      if (module.getFirstDecoration(op->IdResult(),
                                    spv::DecorationNonReadable)) {
        attrs.addAttribute(llvm::Attribute::ReadNone);
      } else if (module.getFirstDecoration(op->IdResult(),
                                           spv::DecorationNonWritable)) {
        attrs.addAttribute(llvm::Attribute::ReadOnly);
      }
    }

    arg->addAttrs(attrs);
  }

  module.addID(op->IdResult(), op, function_arg);
  return llvm::Error::success();
}

namespace {
std::string getScalarTypeName(llvm::Type *ty, const OpCode *op) {
  std::string name;
  if (ty->isIntegerTy()) {
    // Pointer to void is represented as i8* so check for that here.
    if (spirv_ll::isa<OpTypeVoid>(op)) {
      name = "void";
    } else if (spirv_ll::isa<OpTypeBool>(op)) {
      name = "bool";
    } else {
      auto opTypeInt = spirv_ll::cast<OpTypeInt>(op);
      name = getIntTypeName(ty, 0 != opTypeInt->Signedness());
    }
  } else if (ty->isHalfTy()) {
    name = "half";
  } else if (ty->isFloatTy()) {
    name = "float";
  } else if (ty->isDoubleTy()) {
    name = "double";
  } else if (ty->isVoidTy()) {
    name = "void";
  }
  SPIRV_LL_ASSERT(!name.empty(), "failed to determine scalar type name");
  return name;
}

std::string retrieveArgTyMetadata(spirv_ll::Module &module, llvm::Type *argTy,
                                  spv::Id argTyID, bool isBaseTyName) {
  const std::optional<std::string> argBaseTy;
  if (argTy->isPointerTy()) {
    // If we haven't found a known pointer, keep trying.
    auto argTyOp = module.get<OpTypePointer>(argTyID);
    auto pointeeTyID = argTyOp->getTypePointer()->Type();
    auto *pointeeTy = module.getLLVMType(pointeeTyID);

    return retrieveArgTyMetadata(module, pointeeTy, pointeeTyID, isBaseTyName) +
           '*';
  }
  if (argTy->isArrayTy()) {
    // We give up on arrays for simplicity: they can't be specified as
    // parameters to OpenCL C kernels anyway. This also matches
    // SPIRV-LLVM-Translator's behaviour.
    return "array";
  }
  if (argTy->isVectorTy()) {
    auto *const elemTy = multi_llvm::getVectorElementType(argTy);
    const auto opElem = module.get<OpCode>(elemTy);
    const auto name = getScalarTypeName(elemTy, opElem);
    const auto numElements =
        std::to_string(multi_llvm::getVectorNumElements(argTy));
    return isBaseTyName
               ? name + " __attribute__((ext_vector_type(" + numElements + ")))"
               : name + numElements;
  }
  if (argTy->isStructTy()) {
    std::string structName = argTy->getStructName().str();
    std::replace(structName.begin(), structName.end(), '.', ' ');
    return structName;
  }
  if (argTy->isIntegerTy()) {
    auto argTyOp = module.get<OpType>(argTy);
    return getScalarTypeName(argTy, argTyOp);
  }
  if (auto *tgtExtTy = llvm::dyn_cast<llvm::TargetExtType>(argTy)) {
    auto tyName = tgtExtTy->getName();
    if (tyName == "spirv.Event") {
      return "event_t";
    }
    if (tyName == "spirv.Sampler") {
      return "sampler_t";
    }
    if (tyName == "spirv.Image") {
      // TODO: This only covers the small range of images we support.
      auto dim = tgtExtTy->getIntParameter(
          compiler::utils::tgtext::ImageTyDimensionalityIdx);
      auto arrayed =
          tgtExtTy->getIntParameter(compiler::utils::tgtext::ImageTyArrayedIdx);
      switch (dim) {
        default:
          break;
        case compiler::utils::tgtext::ImageDim1D:
          return arrayed ? "image1d_array_t" : "image1d_t";
        case compiler::utils::tgtext::ImageDim2D:
          return arrayed ? "image2d_array_t" : "image2d_t";
        case compiler::utils::tgtext::ImageDim3D:
          return "image3d_t";
        case compiler::utils::tgtext::ImageDimBuffer:
          return "image1d_buffer_t";
      }
    }
    SPIRV_LL_ABORT("Unknown Target Extension Type");
  }
  auto argOp = module.get<OpCode>(argTy);
  return getScalarTypeName(argTy, argOp);
}
}  // namespace

template <>
llvm::Error Builder::create<OpFunctionEnd>(const OpFunctionEnd *) {
  llvm::Function *function = getCurrentFunction();
  llvm::Function *kernel_function = getCurrentFunctionKernel();
  SPIRV_LL_ASSERT_PTR(function);

  const OpFunction *opFunction = module.get<OpFunction>(function);
  const OpTypeFunction *opTypeFunction =
      module.get<OpTypeFunction>(opFunction->FunctionType());
  const OpEntryPoint *opEntryPoint =
      module.getEntryPoint(opFunction->IdResult());
  if (opEntryPoint &&
      spv::ExecutionModelKernel == opEntryPoint->ExecutionModel()) {
    assert(kernel_function &&
           "kernel executionb model requires a kernel function");

    llvm::SmallVector<llvm::Metadata *, 8> argAddrSpaces;
    llvm::SmallVector<llvm::Metadata *, 8> argAccessQuals;
    llvm::SmallVector<llvm::Metadata *, 8> argTypes;
    llvm::SmallVector<llvm::Metadata *, 8> argBaseTypes;
    llvm::SmallVector<llvm::Metadata *, 8> argTypeQuals;
    llvm::SmallVector<llvm::Metadata *, 8> argNames;

    for (const llvm::Argument &arg : kernel_function->args()) {
      llvm::Type *argTy = arg.getType();
      const unsigned argNo = arg.getArgNo();
      auto argTyOr = module.getParamTypeID(opTypeFunction->IdResult(), argNo);
      if (!argTyOr.has_value()) {
        return makeStringError(
            "failed to lookup pointer type for formal parameter");
      }
      const spv::Id typeID = argTyOr.value();

      const std::string argTyName =
          retrieveArgTyMetadata(module, argTy, typeID, /*isBaseTyName*/ false);
      const std::string argBaseTyName =
          retrieveArgTyMetadata(module, argTy, typeID, /*isBaseTyName*/ true);

      // Address space
      const uint32_t argAddrSpace =
          argTy->isPointerTy() ? argTy->getPointerAddressSpace() : 0;
      // We don't set this field.
      const std::string argTyQualName = "";
      // Set access qualifiers
      std::string argAccessQual = "none";
      if (module.get<OpType>(typeID)->isImageType()) {
        argAccessQual = "read_write";
        auto *const opTypeImage = module.get<OpType>(typeID)->getTypeImage();
        if (opTypeImage->wordCount() > 9) {
          switch (opTypeImage->AccessQualifier()) {
            case spv::AccessQualifierReadOnly:
              argAccessQual = "read_only";
              break;
            case spv::AccessQualifierWriteOnly:
              argAccessQual = "write_only";
              break;
            case spv::AccessQualifierReadWrite:
              argAccessQual = "read_write";
              break;
            default:
              llvm_unreachable("invalid OpTypeImage Access Qualifier");
          }
        }
      }

      argAddrSpaces.push_back(
          llvm::ConstantAsMetadata::get(IRBuilder.getInt32(argAddrSpace)));
      argAccessQuals.push_back(
          llvm::MDString::get(*context.llvmContext, argAccessQual));
      argTypes.push_back(llvm::MDString::get(*context.llvmContext, argTyName));
      argBaseTypes.push_back(
          llvm::MDString::get(*context.llvmContext, argBaseTyName));
      argTypeQuals.push_back(
          llvm::MDString::get(*context.llvmContext, argTyQualName));
      argNames.push_back(
          llvm::MDString::get(*context.llvmContext, arg.getName()));
    }

    kernel_function->setMetadata(
        "kernel_arg_addr_space",
        llvm::MDNode::get(*context.llvmContext, argAddrSpaces));
    kernel_function->setMetadata(
        "kernel_arg_access_qual",
        llvm::MDNode::get(*context.llvmContext, argAccessQuals));
    kernel_function->setMetadata(
        "kernel_arg_type", llvm::MDNode::get(*context.llvmContext, argTypes));
    kernel_function->setMetadata(
        "kernel_arg_base_type",
        llvm::MDNode::get(*context.llvmContext, argBaseTypes));
    kernel_function->setMetadata(
        "kernel_arg_type_qual",
        llvm::MDNode::get(*context.llvmContext, argTypeQuals));
    kernel_function->setMetadata(
        "kernel_arg_name", llvm::MDNode::get(*context.llvmContext, argNames));
  }

  // If we've created a forward reference version of this function, we can now
  // replace all of its uses with the concrete function, and mark the forward
  // reference as resolved.
  if (auto *fwd_ref_fn = module.getForwardFnRef(opFunction->IdResult())) {
    llvm::for_each(fwd_ref_fn->users(), [function](llvm::User *user) {
      if (auto *const ci = llvm::dyn_cast<llvm::CallInst>(user)) {
        ci->setAttributes(function->getAttributes());
        ci->setCallingConv(function->getCallingConv());
      }
    });
    fwd_ref_fn->replaceAllUsesWith(function);
    fwd_ref_fn->eraseFromParent();
    module.resolveForwardFnRef(opFunction->IdResult());
  }

  setCurrentFunction(nullptr);

  return llvm::Error::success();
}

static inline llvm::Attribute getTypedAttr(llvm::LLVMContext &c,
                                           llvm::Attribute::AttrKind kind,
                                           llvm::Type *type) {
  if (llvm::Attribute::isTypeAttrKind(kind)) {
    return llvm::Attribute::get(c, kind, type);
  }
  return llvm::Attribute::get(c, kind);
}

template <>
llvm::Error Builder::create<OpFunctionCall>(const OpFunctionCall *op) {
  llvm::Function *callee;
  const unsigned n_args = op->wordCount() - 4;

  if (auto *const fn = module.getValue(op->Function())) {
    callee = cast<llvm::Function>(fn);
  } else {
    // If we haven't seen this function before (i.e., a forward reference),
    // create a call to an internal dummy function which we'll fix up during
    // the creation of the OpFunction, later on.
    llvm::SmallVector<llvm::Type *, 4> paramTypes;
    // First we must construct the called function's type. As per the SPIR-V
    // spec:
    //   Note: A forward call is possible because there is no missing type
    //   information: Result Type must match the Return Type of the function,
    //   and the calling argument types must match the formal parameter types.
    llvm::Type *resultType = module.getLLVMType(op->IdResultType());
    for (unsigned i = 0; i < n_args; ++i) {
      auto *const spv_ty = module.getResultType(op->Arguments()[i]);
      SPIRV_LL_ASSERT_PTR(spv_ty);
      auto *const llvm_ty = module.getLLVMType(spv_ty->IdResult());
      SPIRV_LL_ASSERT_PTR(llvm_ty);
      paramTypes.push_back(llvm_ty);
    }
    auto *const functionType =
        llvm::FunctionType::get(resultType, paramTypes, /*isVarArg*/ false);
    // Generate a special dummy name here, so that the 'real' function's name
    // isn't taken when it comes to creating it.
    const std::string dummyFnName =
        "__spirv.ll.forwardref." + module.getName(op->Function());
    callee = cast<llvm::Function>(
        module.llvmModule->getOrInsertFunction(dummyFnName, functionType)
            .getCallee());
    module.addForwardFnRef(op->Function(), callee);
  }
  SPIRV_LL_ASSERT_PTR(callee);

  llvm::SmallVector<llvm::Value *, 4> args;

  for (unsigned i = 0; i < n_args; ++i) {
    llvm::Value *arg = module.getValue(op->Arguments()[i]);

    SPIRV_LL_ASSERT_PTR(arg);
    args.push_back(arg);
  }
  auto call = IRBuilder.CreateCall(callee->getFunctionType(), callee, args);
  call->setCallingConv(llvm::cast<llvm::Function>(callee)->getCallingConv());

  // For each parameter we need to check whether to mirror any attributes on
  // the function itself: SPIR-V doesn't encode the attributes on the
  // argument operands, only on the formal parameter types
  // so make sure these are added where necessary
  const llvm::Attribute::AttrKind ptr_attr_kinds[] = {
      llvm::Attribute::ByRef,     llvm::Attribute::ByVal,
      llvm::Attribute::StructRet, llvm::Attribute::ReadOnly,
      llvm::Attribute::WriteOnly, llvm::Attribute::NoAlias,
#if LLVM_VERSION_GREATER_EQUAL(21, 0)
      llvm::Attribute::Captures,
#else
      llvm::Attribute::NoCapture,
#endif
  };
  const llvm::Attribute::AttrKind val_attr_kinds[] = {
      llvm::Attribute::ZExt,
      llvm::Attribute::SExt,
  };
  for (unsigned i = 0; i < n_args; ++i) {
    for (auto kind : ptr_attr_kinds) {
      if (!callee->hasParamAttribute(i, kind)) {
        continue;
      }
      llvm::Type *operand_ty = call->getArgOperand(i)->getType();
      (void)operand_ty;
      SPIRV_LL_ASSERT(operand_ty->isPointerTy(),
                      "arg operand type is not a pointer");
      auto &ctx = call->getContext();
      switch (kind) {
        case llvm::Attribute::ByVal:
          call->addParamAttr(
              i, llvm::Attribute::get(ctx, kind, call->getParamByValType(i)));
          break;
        case llvm::Attribute::ByRef:
          call->addParamAttr(
              i, llvm::Attribute::get(ctx, kind, callee->getParamByRefType(i)));
          break;
        case llvm::Attribute::StructRet:
          call->addParamAttr(i, llvm::Attribute::get(
                                    ctx, kind, call->getParamStructRetType(i)));
          break;
#if LLVM_VERSION_GREATER_EQUAL(21, 0)
        case llvm::Attribute::Captures:
          call->addParamAttr(i, llvm::Attribute::getWithCaptureInfo(
                                    ctx, call->getCaptureInfo(i)));
          break;
#endif
        default:
          SPIRV_LL_ASSERT(!llvm::Attribute::isTypeAttrKind(kind),
                          "Unhandled type attribute");
          call->addParamAttr(i, llvm::Attribute::get(ctx, kind));
          break;
      }
    }
    for (auto kind : val_attr_kinds) {
      if (!callee->hasParamAttribute(i, kind)) {
        continue;
      }
      llvm::Type *operand_ty = call->getArgOperand(i)->getType();
      call->addParamAttr(i, getTypedAttr(call->getContext(), kind, operand_ty));
    }
  }
  module.addID(op->IdResult(), op, call);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpVariable>(const OpVariable *op) {
  auto *resultTy = module.get<OpTypePointer>(op->IdResultType());
  SPIRV_LL_ASSERT(resultTy, "Result type is not a pointer");
  auto varTy = module.getLLVMType(resultTy->getTypePointer()->Type());

  llvm::Constant *initializer = nullptr;
  if (op->wordCount() > 4) {
    initializer =
        llvm::cast<llvm::Constant>(module.getValue(op->Initializer()));

    if (llvm::isa<llvm::GlobalVariable>(initializer)) {
      initializer = llvm::cast<llvm::Constant>(
          IRBuilder.CreatePtrToInt(initializer, varTy));
    }
  }

  if (!initializer && op->StorageClass() != spv::StorageClassFunction) {
    initializer = llvm::PoisonValue::get(varTy);
  }

  llvm::Value *value = nullptr;

  const std::string name = module.getName(op->IdResult());

  if (module.hasCapability(spv::CapabilityKernel)) {
    // FIXME: First check if the variable has the BuiltIn decoration since it
    // is possible for external SPIR-V producers to use an incorrect
    // StorageClass, this should be handled by the Input StorageClass below.
    if (module.getFirstDecoration(op->IdResult(), spv::DecorationBuiltIn)) {
      module.addBuiltInID(op->IdResult());
      auto addrSpaceOrError =
          module.translateStorageClassToAddrSpace(op->StorageClass());
      if (auto err = addrSpaceOrError.takeError()) {
        return err;
      }
      value = new llvm::GlobalVariable(
          *module.llvmModule, varTy,
          false,                               // isConstant
          llvm::GlobalValue::ExternalLinkage,  // Linkage
          llvm::PoisonValue::get(varTy),       // Initializer
          name,                                // Name
          nullptr,                             // InsertBefore
          llvm::GlobalValue::NotThreadLocal,   // TLMode
          addrSpaceOrError.get(),              // AddressSpace
          false                                // isExternallyInitialized
      );
    } else {
      // Following is the set of StorageClasses supported by the Kernel
      // capability.
      switch (op->StorageClass()) {
        case spv::StorageClassUniformConstant: {
          // Shared externally, visible across all functions in all invocations
          // in all work groups. Graphics uniform memory. OpenCL constant
          // memory. Variables declared with this storage class are read-only.
          // They may have initializers, as allowed by the client API.
          auto constantValue = new llvm::GlobalVariable(
              *module.llvmModule, varTy,
              true,                               // isConstant
              llvm::GlobalValue::PrivateLinkage,  // Linkage
              initializer,                        // Initializer
              name,                               // Name
              nullptr,                            // InsertBefore
              llvm::GlobalValue::NotThreadLocal,  // TLMode
              2,                                  // AddressSpace
              false                               // isExternallyInitialized
          );

          // The unnamed_addr attribute indicates that constant global
          // variables with identical initializers can be merged.
          constantValue->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
          value = constantValue;
        } break;
        case spv::StorageClassInput: {
          // Input from pipeline. Visible across all functions in the current
          // invocation. Variables declared with this storage class are
          // read-only, and cannot have initializers.
          // FIXME: These are handled in the BuiltIn decoration branch above
          // making this case a no-op. Once upstream producers correctly
          // specify the StorageClass for BuiltIn variables the branch can be
          // removed and the implementation moved here.
        } break;
        case spv::StorageClassWorkgroup: {
          // Shared across all invocations within a work group. Visible across
          // all functions. The OpenGL "shared" storage qualifier. OpenCL local
          // memory.
          value = new llvm::GlobalVariable(
              *module.llvmModule, varTy,
              false,                               // isConstant
              llvm::GlobalValue::InternalLinkage,  // Linkage
              initializer,                         // Initializer
              name,                                // Name
              nullptr,                             // InsertBefore
              llvm::GlobalValue::NotThreadLocal,   // TLMode
              3,                                   // AddressSpace
              false                                // isExternallyInitialized
          );
        } break;
        case spv::StorageClassCrossWorkgroup: {
          // Visible across all functions of all invocations of all work groups.
          // OpenCL global memory.
          auto globalValue = new llvm::GlobalVariable(
              *module.llvmModule, varTy,
              false,                               // isConstant
              llvm::GlobalValue::ExternalLinkage,  // Linkage
              initializer,                         // Initializer
              name,                                // Name
              nullptr,                             // InsertBefore
              llvm::GlobalValue::NotThreadLocal,   // TLMode
              1,                                   // AddressSpace
              false                                // isExternallyInitialized
          );
          value = globalValue;
        } break;
        case spv::StorageClassFunction: {
          // Visible only within the declaring function of the current
          // invocation. Regular function memory.
          if (!IRBuilder.GetInsertBlock()) {
            return makeStringError(
                "invalid SPIR-V: variables can not have a function[7] "
                "storage class outside of a function");
          }
          auto *alloca = IRBuilder.CreateAlloca(varTy);
          alloca->setName(name);
          if (initializer) {
            IRBuilder.CreateStore(initializer, alloca);
          }
          value = alloca;
        } break;
        case spv::StorageClassGeneric: {
          SPIRV_LL_ABORT("StorageClass Generic not supported for variables");
        } break;
        case spv::StorageClassImage: {
          // For holding image memory.
          SPIRV_LL_ABORT("StorageClass Image not implemented");
        } break;
        default:
          SPIRV_LL_ABORT("OpVariable invalid Kernel StorageClass");
      }
      if (auto alignment = module.getFirstDecoration(
              op->IdResult(), spv::DecorationAlignment)) {
        if (auto global_val = llvm::dyn_cast<llvm::GlobalVariable>(value)) {
          auto align = llvm::MaybeAlign(alignment->getValueAtOffset(3));
          global_val->setAlignment(align);
        } else if (auto alloca = llvm::dyn_cast<llvm::AllocaInst>(value)) {
          auto align =
              llvm::MaybeAlign(alignment->getValueAtOffset(3)).valueOrOne();
          alloca->setAlignment(align);
        }
      }
    }
  }

  // OpVariables can be given linkage, but we only allow LinkOnceODR to update
  // the linkage we've already given.
  if (auto global_val = llvm::dyn_cast<llvm::GlobalVariable>(value)) {
    auto *const entrypt = module.getEntryPoint(op->IdResult());
    // Kernel entry points must always have External linkage.
    if (!entrypt || entrypt->ExecutionModel() != spv::ExecutionModelKernel) {
      if (auto linkage = getLinkage(module, op->IdResult())) {
        if (linkage->first == spv::LinkageTypeLinkOnceODR) {
          SPIRV_LL_ASSERT(
              module.isExtensionEnabled("SPV_KHR_linkonce_odr"),
              "SPV_KHR_linkonce_odr must be enabled to use LinkOnceODRLinkage");
          global_val->setLinkage(llvm::GlobalVariable::LinkOnceODRLinkage);
        }
      }
    }
  }

  module.addID(op->IdResult(), op, value);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageTexelPointer>(const OpImageTexelPointer *) {
  // This instruction is only useful to Vulkan, as it produces a pointer with
  // storage class Image which is only to be used for atomic operations, but the
  // OpenCL SPIR-V environment spec forbids storage class Image for atomic
  // operations (see section 2.1). It will remain unimplemented until VK gets
  // image support.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpLoad>(const OpLoad *op) {
  llvm::Value *ptr = module.getValue(op->Pointer());
  llvm::Type *type = module.getLLVMType(op->IdResultType());

  SPIRV_LL_ASSERT_PTR(ptr);

  llvm::LoadInst *load = IRBuilder.CreateLoad(type, ptr);

  if (op->wordCount() > 4) {
    if (op->MemoryAccess() & spv::MemoryAccessVolatileMask) {
      load->setVolatile(true);
    }
    if (op->MemoryAccess() & spv::MemoryAccessAlignedMask) {
      auto alignment = llvm::MaybeAlign(op->getValueAtOffset(5)).valueOrOne();
      load->setAlignment(alignment);
    }
    // TODO: set non-temporal with metadata?
  }

  // check for kernel specified alignment
  if (module.hasCapability(spv::CapabilityKernel)) {
    if (auto align = module.getFirstDecoration(op->Pointer(),
                                               spv::DecorationAlignment)) {
      auto alignment =
          llvm::MaybeAlign(align->getValueAtOffset(3)).valueOrOne();
      load->setAlignment(alignment);
    }
  }

  // check for volatile decoration
  if (module.getFirstDecoration(op->Pointer(), spv::DecorationVolatile)) {
    load->setVolatile(true);
  }

  module.addID(op->IdResult(), op, load);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpStore>(const OpStore *op) {
  llvm::Value *ptr = module.getValue(op->Pointer());
  SPIRV_LL_ASSERT_PTR(ptr);

  llvm::Value *object = module.getValue(op->Object());
  SPIRV_LL_ASSERT_PTR(object);

  llvm::StoreInst *store = IRBuilder.CreateStore(object, ptr);

  if (op->wordCount() > 3) {
    if (op->MemoryAccess() & spv::MemoryAccessVolatileMask) {
      store->setVolatile(true);
    }
    if (op->MemoryAccess() & spv::MemoryAccessAlignedMask) {
      auto alignment = llvm::MaybeAlign(op->getValueAtOffset(4)).valueOrOne();
      store->setAlignment(alignment);
    }
  }

  // check for kernel specified alignment
  if (module.hasCapability(spv::CapabilityKernel)) {
    if (auto align = module.getFirstDecoration(op->Pointer(),
                                               spv::DecorationAlignment)) {
      auto alignment =
          llvm::MaybeAlign(align->getValueAtOffset(3)).valueOrOne();
      store->setAlignment(alignment);
    }
  }

  // check for volatile decoration
  if (module.getFirstDecoration(op->Pointer(), spv::DecorationVolatile)) {
    store->setVolatile(true);
  }
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpCopyMemory>(const OpCopyMemory *op) {
  llvm::Value *source = module.getValue(op->Source());
  SPIRV_LL_ASSERT_PTR(source);
  SPIRV_LL_ASSERT(source->getType()->isPointerTy(), "Source is not a pointer");

  llvm::Value *target = module.getValue(op->Target());
  SPIRV_LL_ASSERT_PTR(target);
  SPIRV_LL_ASSERT(target->getType()->isPointerTy(), "Target is not a pointer");

  auto *sourceOpType = module.getResultType(op->Source());
  SPIRV_LL_ASSERT_PTR(sourceOpType);
  auto *targetOpType = module.getResultType(op->Target());
  (void)targetOpType;
  SPIRV_LL_ASSERT_PTR(targetOpType);
  SPIRV_LL_ASSERT(
      sourceOpType->isPointerType() && targetOpType->isPointerType(),
      "Source and Target are not pointers");

  auto *pointeeType =
      module.getLLVMType(sourceOpType->getTypePointer()->Type());

  SPIRV_LL_ASSERT(sourceOpType->getTypePointer()->Type() ==
                      targetOpType->getTypePointer()->Type(),
                  "Source and Target don't point to the same type");

  const size_t size =
      module.llvmModule->getDataLayout().getTypeStoreSize(pointeeType);

  bool isVolatile = false;
  uint32_t alignment = 0;

  if (op->wordCount() > 3) {
    const uint32_t memoryAccess = op->MemoryAccess();

    if (spv::MemoryAccessVolatileMask & memoryAccess) {
      isVolatile = true;
    }
    if (spv::MemoryAccessAlignedMask & memoryAccess) {
      alignment = op->getValueAtOffset(4);
    }
  }

  IRBuilder.CreateMemCpy(target, llvm::MaybeAlign(alignment), source,
                         llvm::MaybeAlign(alignment), size, isVolatile);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpCopyMemorySized>(const OpCopyMemorySized *op) {
  llvm::Value *source = module.getValue(op->Source());
  SPIRV_LL_ASSERT_PTR(source);

  llvm::Value *target = module.getValue(op->Target());
  SPIRV_LL_ASSERT_PTR(target);

  llvm::Value *size = module.getValue(op->Size());
  SPIRV_LL_ASSERT_PTR(size);

  bool isVolatile = false;
  uint32_t alignment = 0;

  if (op->wordCount() > 4) {
    const uint32_t memoryAccess = op->MemoryAccess();

    if (spv::MemoryAccessVolatileMask & memoryAccess) {
      isVolatile = true;
    }
    if (spv::MemoryAccessAlignedMask & memoryAccess) {
      alignment = op->getValueAtOffset(5);
    }
  }

  // If we are copying from a constant integer array then this might be a
  // memset and we can generate a memset intrinsic accordingly.
  llvm::GlobalVariable *sourceGlobal =
      llvm::dyn_cast<llvm::GlobalVariable>(source->stripPointerCasts());
  auto *targetOpType = module.getResultType(op->Target());
  SPIRV_LL_ASSERT(targetOpType && targetOpType->isPointerType(),
                  "Target is not a pointer type");
  llvm::Type *targetElementType =
      module.getLLVMType(targetOpType->getTypePointer()->Type());
  if (sourceGlobal && sourceGlobal->isConstant() &&
      sourceGlobal->getInitializer() &&
      sourceGlobal->getInitializer()->getType()->isArrayTy() &&
      targetElementType->isIntegerTy()) {
    const uint32_t bitWidth = targetElementType->getScalarSizeInBits();
    uint32_t memsetVal = 0;
    if (!sourceGlobal->getInitializer()->isZeroValue()) {
      // If the global variable's initializer isn't an array of zeros we need to
      // make sure it's the same value all the way through. It seems highly
      // unlikely but someone might just be copying a const array of different
      // values into a buffer and that would be perfectly valid.
      llvm::ConstantDataArray *sourceConstantArray =
          llvm::cast<llvm::ConstantDataArray>(sourceGlobal->getInitializer());
      const uint32_t pattern = sourceConstantArray->getElementAsInteger(0);
      for (uint32_t i = 1; i < sourceConstantArray->getNumElements(); i++) {
        if (sourceConstantArray->getElementAsInteger(i) != pattern) {
          // We've hit a value that's different, so this is actually just a
          // really strange copy operation after all.
          IRBuilder.CreateMemCpy(target, llvm::MaybeAlign(alignment), source,
                                 llvm::MaybeAlign(alignment), size, isVolatile);
          return llvm::Error::success();
        }
      }
      // If we made it this far we now know that the whole intializer array
      // contains `pattern` and we can proceed with the memset.
      memsetVal = pattern;
    }
    IRBuilder.CreateMemSet(target, IRBuilder.getIntN(bitWidth, memsetVal), size,
                           llvm::MaybeAlign(alignment), isVolatile);
  } else {
    IRBuilder.CreateMemCpy(target, llvm::MaybeAlign(alignment), source,
                           llvm::MaybeAlign(alignment), size, isVolatile);
  }
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAccessChain>(const OpAccessChain *op) {
  llvm::Value *base = module.getValue(op->Base());
  SPIRV_LL_ASSERT_PTR(base);

  llvm::SmallVector<llvm::Value *, 8> indexes;
  indexes.push_back(IRBuilder.getInt32(0));
  for (auto id : op->Indexes()) {
    indexes.push_back(module.getValue(id));
  }

  auto *baseTy = module.getResultType(op->Base());
  SPIRV_LL_ASSERT(baseTy && baseTy->isPointerType(),
                  "Base is not a pointer type");

  auto *basePointeeTy = module.getLLVMType(baseTy->getTypePointer()->Type());
  auto inst = llvm::GetElementPtrInst::Create(basePointeeTy, base, indexes,
                                              module.getName(op->IdResult()),
                                              IRBuilder.GetInsertBlock());

  if (basePointeeTy->isStructTy()) {
    checkMemberDecorations(basePointeeTy, indexes, op->IdResult());
  }

  module.addID(op->IdResult(), op, inst);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpInBoundsAccessChain>(
    const OpInBoundsAccessChain *op) {
  auto base = module.getValue(op->Base());
  SPIRV_LL_ASSERT_PTR(base);

  llvm::SmallVector<llvm::Value *, 8> indexes;
  indexes.push_back(IRBuilder.getInt32(0));
  for (auto indexId : op->Indexes()) {
    indexes.push_back(module.getValue(indexId));
  }

  auto *baseTy = module.getResultType(op->Base());
  SPIRV_LL_ASSERT(baseTy && baseTy->isPointerType(),
                  "Base is not a pointer type");

  auto *basePointeeTy = module.getLLVMType(baseTy->getTypePointer()->Type());
  llvm::GetElementPtrInst *inst = llvm::GetElementPtrInst::Create(
      basePointeeTy, base, indexes, module.getName(op->IdResult()),
      IRBuilder.GetInsertBlock());
  inst->setIsInBounds();

  if (basePointeeTy->isStructTy()) {
    checkMemberDecorations(basePointeeTy, indexes, op->IdResult());
  }

  module.addID(op->IdResult(), op, inst);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpPtrAccessChain>(const OpPtrAccessChain *op) {
  auto base = module.getValue(op->Base());
  SPIRV_LL_ASSERT_PTR(base);

  auto element = module.getValue(op->Element());
  SPIRV_LL_ASSERT_PTR(element);

  llvm::SmallVector<llvm::Value *, 8> indexes;
  indexes.push_back(element);
  for (auto indexId : op->Indexes()) {
    indexes.push_back(module.getValue(indexId));
  }

  auto *baseTy = module.getResultType(op->Base());
  SPIRV_LL_ASSERT(baseTy && baseTy->isPointerType(),
                  "Base is not a pointer type");

  auto *basePointeeTy = module.getLLVMType(baseTy->getTypePointer()->Type());
  llvm::GetElementPtrInst *inst = llvm::GetElementPtrInst::Create(
      basePointeeTy, base, indexes, module.getName(op->IdResult()),
      IRBuilder.GetInsertBlock());

  if (basePointeeTy->isStructTy()) {
    checkMemberDecorations(basePointeeTy, indexes, op->IdResult());
  }

  module.addID(op->IdResult(), op, inst);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGenericPtrMemSemantics>(
    const OpGenericPtrMemSemantics *) {
  // The generic storage class requires the GenericPointer capability, which is
  // not supported by OpenCL 1.2, see the OpenCL SPIR-V environment spec section
  // 6.1.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpInBoundsPtrAccessChain>(
    const OpInBoundsPtrAccessChain *op) {
  auto base = module.getValue(op->Base());
  SPIRV_LL_ASSERT_PTR(base);

  auto element = module.getValue(op->Element());
  SPIRV_LL_ASSERT_PTR(element);

  llvm::SmallVector<llvm::Value *, 4> indexes;
  indexes.push_back(element);
  for (auto indexId : op->Indexes()) {
    indexes.push_back(module.getValue(indexId));
  }

  auto *baseTy = module.getResultType(op->Base());
  SPIRV_LL_ASSERT(baseTy && baseTy->isPointerType(),
                  "Base is not a pointer type");

  auto *basePointeeTy = module.getLLVMType(baseTy->getTypePointer()->Type());
  llvm::GetElementPtrInst *inst = llvm::GetElementPtrInst::Create(
      basePointeeTy, base, indexes, module.getName(op->IdResult()),
      IRBuilder.GetInsertBlock());
  inst->setIsInBounds();

  module.addID(op->IdResult(), op, inst);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpDecorate>(const OpDecorate *op) {
  module.addDecoration(op->Target(), op);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpMemberDecorate>(const OpMemberDecorate *op) {
  module.addMemberDecoration(op->StructureType(), op->Member(), op);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpDecorationGroup>(const OpDecorationGroup *) {
  // the way we track decorations means that we don't actually have to do
  // anything here
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupDecorate>(const OpGroupDecorate *op) {
  auto groupDecorations = module.getDecorations(op->DecorationGroup());

  for (auto decorateOp : groupDecorations) {
    for (auto targetID : op->Targets()) {
      module.addDecoration(targetID, decorateOp);
    }
  }
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupMemberDecorate>(
    const OpGroupMemberDecorate *op) {
  auto groupDecorations = module.getDecorations(op->DecorationGroup());

  for (auto target : op->Targets()) {
    for (auto decorateOp : groupDecorations) {
      auto memberDecorate = cast<OpDecorateBase>(decorateOp);
      module.addMemberDecoration(target.Id, target.Literal, memberDecorate);
    }
  }
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpVectorExtractDynamic>(
    const OpVectorExtractDynamic *op) {
  llvm::Value *vector = module.getValue(op->Vector());
  SPIRV_LL_ASSERT_PTR(vector);

  llvm::Value *index = module.getValue(op->Index());
  SPIRV_LL_ASSERT_PTR(index);

  llvm::Value *element = IRBuilder.CreateExtractElement(vector, index);
  element->setName(module.getName(op->IdResult()));

  module.addID(op->IdResult(), op, element);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpVectorInsertDynamic>(
    const OpVectorInsertDynamic *op) {
  llvm::Value *component = module.getValue(op->Component());
  SPIRV_LL_ASSERT_PTR(component);

  llvm::Value *vector = module.getValue(op->Vector());
  SPIRV_LL_ASSERT_PTR(vector);

  llvm::Value *index = module.getValue(op->Index());
  SPIRV_LL_ASSERT_PTR(index);

  llvm::Value *new_vec =
      IRBuilder.CreateInsertElement(vector, component, index);
  new_vec->setName(module.getName(op->IdResult()));

  module.addID(op->IdResult(), op, new_vec);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpVectorShuffle>(const OpVectorShuffle *op) {
  llvm::Value *v1 = module.getValue(op->Vector1());
  SPIRV_LL_ASSERT_PTR(v1);

  llvm::Value *v2 = module.getValue(op->Vector2());
  SPIRV_LL_ASSERT_PTR(v2);

  llvm::SmallVector<llvm::Constant *, 4> components;

  for (int16_t compIndex = 0; compIndex < op->wordCount() - 5; compIndex++) {
    const uint32_t component = op->Components()[compIndex];
    if (component == 0xFFFFFFFF) {
      components.push_back(llvm::PoisonValue::get(IRBuilder.getInt32Ty()));
    } else {
      components.push_back(IRBuilder.getInt32(component));
    }
  }

  llvm::SmallVector<int, 16> mask;
  llvm::ShuffleVectorInst::getShuffleMask(llvm::ConstantVector::get(components),
                                          mask);
  llvm::Value *new_vec = IRBuilder.CreateShuffleVector(v1, v2, mask);
  new_vec->setName(module.getName(op->IdResult()));

  module.addID(op->IdResult(), op, new_vec);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpCompositeConstruct>(
    const OpCompositeConstruct *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::SmallVector<llvm::Value *, 4> constituents;

  for (int16_t cIndex = 0; cIndex < op->wordCount() - 3; cIndex++) {
    llvm::Value *constituent = module.getValue(op->Constituents()[cIndex]);
    constituents.push_back(constituent);
  }

  // store this so we can add the last insert instruction to the module
  llvm::Value *lastConstituent = constituents.pop_back_val();

  int insertIndex = 0;

  if (type->getTypeID() == llvm::Type::FixedVectorTyID) {
    llvm::Value *vec = llvm::PoisonValue::get(type);

    for (auto c : constituents) {
      vec = IRBuilder.CreateInsertElement(vec, c, insertIndex);
      insertIndex++;
    }

    llvm::Value *new_vec =
        IRBuilder.CreateInsertElement(vec, lastConstituent, insertIndex);
    new_vec->setName(module.getName(op->IdResult()));

    module.addID(op->IdResult(), op, new_vec);
  } else {
    llvm::Value *composite = llvm::PoisonValue::get(type);

    for (auto c : constituents) {
      composite = IRBuilder.CreateInsertValue(composite, c, insertIndex);
      insertIndex++;
    }

    llvm::Value *new_composite =
        IRBuilder.CreateInsertValue(composite, lastConstituent, insertIndex);
    new_composite->setName(module.getName(op->IdResult()));

    module.addID(op->IdResult(), op, new_composite);
  }
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpCompositeExtract>(const OpCompositeExtract *op) {
  llvm::Value *composite = module.getValue(op->Composite());
  SPIRV_LL_ASSERT_PTR(composite);

  llvm::Type *type = composite->getType();

  if (type->isVectorTy()) {
    const uint32_t index = op->Indexes()[0];

    llvm::Value *element = IRBuilder.CreateExtractElement(composite, index);
    element->setName(module.getName(op->IdResult()));

    module.addID(op->IdResult(), op, element);
  } else {
    llvm::SmallVector<uint32_t, 4> indexes;

    for (int16_t index = 0; index < op->wordCount() - 4; index++) {
      indexes.push_back(op->Indexes()[index]);
    }
    llvm::Value *element = IRBuilder.CreateExtractValue(composite, indexes);
    element->setName(module.getName(op->IdResult()));

    module.addID(op->IdResult(), op, element);
  }
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpCompositeInsert>(const OpCompositeInsert *op) {
  llvm::Value *composite = module.getValue(op->Composite());
  SPIRV_LL_ASSERT_PTR(composite);

  llvm::Value *object = module.getValue(op->Object());
  SPIRV_LL_ASSERT_PTR(object);

  if (composite->getType()->getTypeID() == llvm::Type::FixedVectorTyID) {
    const uint32_t index = op->getValueAtOffset(5);

    llvm::Value *new_vec =
        IRBuilder.CreateInsertElement(composite, object, index);
    new_vec->setName(module.getName(op->IdResult()));

    module.addID(op->IdResult(), op, new_vec);
  } else {
    llvm::SmallVector<uint32_t, 4> indexes;

    for (int16_t index = 0; index < op->wordCount() - 5; index++) {
      indexes.push_back(op->Indexes()[index]);
    }

    llvm::Value *new_composite =
        IRBuilder.CreateInsertValue(composite, object, indexes);
    new_composite->setName(module.getName(op->IdResult()));

    module.addID(op->IdResult(), op, new_composite);
  }
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpCopyObject>(const OpCopyObject *op) {
  llvm::Value *object = module.getValue(op->Operand());
  SPIRV_LL_ASSERT_PTR(object);

  llvm::Value *new_object = nullptr;
  // if the object to copy is a pointer type (i.e. an OpVariable) the copy
  // should create a new pointer of the same type that contains the same value
  // as the original
  auto *opTy = module.getResultType(op->Operand());
  SPIRV_LL_ASSERT_PTR(opTy);
  if (opTy->isPointerType()) {
    auto *pointeeTy = module.getLLVMType(opTy->getTypePointer()->Type());
    SPIRV_LL_ASSERT_PTR(pointeeTy);
    new_object = IRBuilder.CreateAlloca(pointeeTy);

    // to complete the copy of a pointer object we must copy the contents of the
    // old pointer accross
    llvm::Value *contents = IRBuilder.CreateLoad(pointeeTy, object);
    IRBuilder.CreateStore(contents, new_object);
  } else {
    // if the value to be copied is not a variable we can just add it to the
    // module again with the new ID
    new_object = object;
  }

  module.addID(op->IdResult(), op, new_object);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpTranspose>(const OpTranspose *) {
  // TODO: transpose builtin
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSampledImage>(const OpSampledImage *op) {
  llvm::Value *image = module.getValue(op->Image());
  SPIRV_LL_ASSERT_PTR(image);

  llvm::Value *sampler = module.getValue(op->Sampler());
  SPIRV_LL_ASSERT_PTR(sampler);

  // If this is a OpConstantSampler, we've stored it as a constant i32.
  // Translate it to a proper sampler type through clang's built-in
  // __translate_sampler_initializer function.
  if (llvm::isa<llvm::ConstantInt>(sampler)) {
    auto *formalSamplerOpTy = module.getResultType(op->Sampler());
    SPIRV_LL_ASSERT_PTR(formalSamplerOpTy);
    auto formalSamplerTyID = formalSamplerOpTy->IdResult();
    auto *formalSamplerTy = module.getLLVMType(formalSamplerTyID);
    SPIRV_LL_ASSERT(sampler->getType() && sampler->getType()->isIntegerTy(32),
                    "Internal sampler error");
    SPIRV_LL_ASSERT(formalSamplerTy && formalSamplerTy->isTargetExtTy(),
                    "Internal sampler error");
    auto translate_func = module.llvmModule->getOrInsertFunction(
        SAMPLER_INIT_FN, formalSamplerTy, sampler->getType());
    sampler = IRBuilder.CreateCall(translate_func, sampler);
  }

  module.addSampledImage(op->IdResult(), image, sampler);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSampleImplicitLod>(
    const OpImageSampleImplicitLod *) {
  // This instruction is Shader capability so it will remain unimplemented until
  // VK has image support
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSampleExplicitLod>(
    const OpImageSampleExplicitLod *op) {
  auto retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  const Module::SampledImage sampledImage =
      module.getSampledImage(op->SampledImage());

  const OpSampledImage *sampledImageOp =
      module.get<OpSampledImage>(op->SampledImage());

  auto coord = module.getValue(op->Coordinate());
  SPIRV_LL_ASSERT_PTR(coord);

  auto result = createImageAccessBuiltinCall(
      "read_image", retTy, op->IdResultType(),
      {sampledImage.image, sampledImage.sampler, coord},
      {sampledImageOp->Image(), sampledImageOp->Sampler(), op->Coordinate()},
      module.get<OpTypeVector>(op->IdResultType()));

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSampleDrefImplicitLod>(
    const OpImageSampleDrefImplicitLod *) {
  // This instruction is Shader capability so it will remain unimplemented until
  // VK has image support
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSampleDrefExplicitLod>(
    const OpImageSampleDrefExplicitLod *) {
  // This instruction is Shader capability so it will remain unimplemented until
  // VK has image support
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSampleProjImplicitLod>(
    const OpImageSampleProjImplicitLod *) {
  // This instruction is Shader capability so it will remain unimplemented until
  // VK has image support
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSampleProjExplicitLod>(
    const OpImageSampleProjExplicitLod *) {
  // This instruction is Shader capability so it will remain unimplemented until
  // VK has image support
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSampleProjDrefImplicitLod>(
    const OpImageSampleProjDrefImplicitLod *) {
  // This instruction is Shader capability so it will remain unimplemented until
  // VK has image support
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSampleProjDrefExplicitLod>(
    const OpImageSampleProjDrefExplicitLod *) {
  // This instruction is Shader capability so it will remain unimplemented until
  // VK has image support
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageFetch>(const OpImageFetch *) {
  // This instruction is Vulkan exclusive as it requires the OpTypeImage to have
  // sampled set to 1, which the OpenCL SPIR-V environment spec forbids. For
  // this reason it will remain unimplemented until VK gets image support.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageGather>(const OpImageGather *) {
  // This instruction is Shader capability so it will remain unimplemented until
  // VK has image support
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageDrefGather>(const OpImageDrefGather *) {
  // This instruction is Shader capability so it will remain unimplemented until
  // VK has image support
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageRead>(const OpImageRead *op) {
  auto retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  auto image = module.getValue(op->Image());
  SPIRV_LL_ASSERT_PTR(image);

  auto coord = module.getValue(op->Coordinate());
  SPIRV_LL_ASSERT_PTR(coord);

  auto pixelTypeOp = module.get<OpTypeVector>(op->IdResultType());
  auto result = createImageAccessBuiltinCall(
      "read_image", retTy, op->IdResultType(), {image, coord},
      {op->Image(), op->Coordinate()}, pixelTypeOp);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageWrite>(const OpImageWrite *op) {
  auto image = module.getValue(op->Image());
  SPIRV_LL_ASSERT_PTR(image);

  auto coord = module.getValue(op->Coordinate());
  SPIRV_LL_ASSERT_PTR(coord);

  auto texel = module.getValue(op->Texel());
  SPIRV_LL_ASSERT_PTR(texel);

  auto voidTy = llvm::Type::getVoidTy(*context.llvmContext);

  auto pixelTypeID = module.get<OpResult>(op->Texel())->IdResultType();
  auto pixelTypeOp = module.get<OpTypeVector>(pixelTypeID);
  createImageAccessBuiltinCall(
      "write_image", voidTy, MangleInfo(0), {image, coord, texel},
      {op->Image(), op->Coordinate(), op->Texel()}, pixelTypeOp);

  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImage>(const OpImage *op) {
  auto sampledImage = module.getSampledImage(op->SampledImage());
  module.addID(op->IdResult(), op, sampledImage.image);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageQueryFormat>(const OpImageQueryFormat *op) {
  llvm::Type *resultType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *image = module.getValue(op->Image());
  SPIRV_LL_ASSERT_PTR(image);

  llvm::Value *result = createMangledBuiltinCall(
      "get_image_channel_data_type", resultType, MangleInfo(op->IdResultType()),
      {image}, {op->Image()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageQueryOrder>(const OpImageQueryOrder *op) {
  llvm::Type *resultType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::Value *image = module.getValue(op->Image());
  SPIRV_LL_ASSERT_PTR(image);

  llvm::Value *result = createMangledBuiltinCall(
      "get_image_channel_order", resultType, MangleInfo(op->IdResultType()),
      {image}, {op->Image()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageQuerySizeLod>(
    const OpImageQuerySizeLod *op) {
  llvm::Type *returnType = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(returnType);

  llvm::Type *returnScalarType = returnType->getScalarType();

  llvm::Value *image = module.getValue(op->Image());
  SPIRV_LL_ASSERT_PTR(image);

  SPIRV_LL_ASSERT(image->getType()->isTargetExtTy(), "Unknown image type");
  auto *const imgTy = llvm::cast<llvm::TargetExtType>(image->getType());
  const bool isArray =
      imgTy->getIntParameter(compiler::utils::tgtext::ImageTyArrayedIdx) ==
      compiler::utils::tgtext::ImageArrayed;
  const bool is2D = imgTy->getIntParameter(
                        compiler::utils::tgtext::ImageTyDimensionalityIdx) ==
                    compiler::utils::tgtext::ImageDim2D;
  const bool is3D = imgTy->getIntParameter(
                        compiler::utils::tgtext::ImageTyDimensionalityIdx) ==
                    compiler::utils::tgtext::ImageDim3D;

  llvm::Value *result = llvm::PoisonValue::get(returnType);

  if (isArray) {
    llvm::Type *sizeTType;
    if (module.getAddressingModel() == 64) {
      sizeTType = IRBuilder.getInt64Ty();
    } else {
      sizeTType = IRBuilder.getInt32Ty();
    }
    llvm::Value *imageArraySize =
        createMangledBuiltinCall("get_image_array_size", sizeTType,
                                 MangleInfo(0), {image}, {op->Image()});

    if (returnScalarType != sizeTType) {
      imageArraySize =
          IRBuilder.CreateZExtOrTrunc(imageArraySize, returnScalarType);
    }

    llvm::Constant *index =
        IRBuilder.getInt32(multi_llvm::getVectorNumElements(returnType) - 1);

    result = IRBuilder.CreateInsertElement(result, imageArraySize, index);
  }

  llvm::Value *resultWidth =
      createMangledBuiltinCall("get_image_width", IRBuilder.getInt32Ty(),
                               MangleInfo(0), {image}, {op->Image()});

  if (returnScalarType != IRBuilder.getInt32Ty()) {
    resultWidth = IRBuilder.CreateZExtOrTrunc(resultWidth, returnScalarType);
  }

  if (returnType->isVectorTy()) {
    result = IRBuilder.CreateInsertElement(result, resultWidth,
                                           IRBuilder.getInt32(0));
  } else {
    result = resultWidth;
  }

  if (is2D || is3D) {
    llvm::Value *resultHeight =
        createMangledBuiltinCall("get_image_height", IRBuilder.getInt32Ty(),
                                 MangleInfo(0), {image}, {op->Image()});

    if (returnScalarType != IRBuilder.getInt32Ty()) {
      resultHeight =
          IRBuilder.CreateZExtOrTrunc(resultHeight, returnScalarType);
    }

    result = IRBuilder.CreateInsertElement(result, resultHeight,
                                           IRBuilder.getInt32(1));

    if (is3D) {
      llvm::Value *resultDepth =
          createMangledBuiltinCall("get_image_depth", IRBuilder.getInt32Ty(),
                                   MangleInfo(0), {image}, {op->Image()});

      if (returnScalarType != IRBuilder.getInt32Ty()) {
        resultDepth =
            IRBuilder.CreateZExtOrTrunc(resultDepth, returnScalarType);
      }

      result = IRBuilder.CreateInsertElement(result, resultDepth,
                                             IRBuilder.getInt32(2));
    }
  }
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageQuerySize>(const OpImageQuerySize *) {
  // This instruction requires the ImageQuery capability which is not supported
  // by OpenCL 1.2 (see OpenCL SPIR-V environment spec section 6.1)
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageQueryLod>(const OpImageQueryLod *) {
  // This instruction requires the ImageQuery capability which is not supported
  // by OpenCL 1.2 (see OpenCL SPIR-V environment spec section 6.1)
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageQueryLevels>(const OpImageQueryLevels *) {
  // This instruction requires the ImageQuery capability which is not supported
  // by OpenCL 1.2 (see OpenCL SPIR-V environment spec section 6.1)
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageQuerySamples>(const OpImageQuerySamples *) {
  // This instruction requires the ImageQuery capability which is not supported
  // by OpenCL 1.2 (see OpenCL SPIR-V environment spec section 6.1)
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpConvertFToU>(const OpConvertFToU *op) {
  auto retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  auto value = module.getValue(op->FloatValue());
  SPIRV_LL_ASSERT_PTR(value);

  module.addID(
      op->IdResult(), op,
      createConversionBuiltinCall(value, MangleInfo(op->FloatValue()), retTy,
                                  op->IdResultType(), op->IdResult()));

  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpConvertFToS>(const OpConvertFToS *op) {
  auto retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  auto value = module.getValue(op->FloatValue());
  SPIRV_LL_ASSERT_PTR(value);

  // In this instruction, the result type is always signed.
  module.addID(op->IdResult(), op,
               createConversionBuiltinCall(
                   value, MangleInfo(op->FloatValue()), retTy,
                   MangleInfo::getSigned(op->IdResultType()), op->IdResult()));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpConvertSToF>(const OpConvertSToF *op) {
  auto retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  auto value = module.getValue(op->SignedValue());
  SPIRV_LL_ASSERT_PTR(value);

  // In this instruction, the value is always signed so don't pass its ID for
  // signedness lookup.
  module.addID(op->IdResult(), op,
               createConversionBuiltinCall(value, {}, retTy, op->IdResultType(),
                                           op->IdResult()));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpConvertUToF>(const OpConvertUToF *op) {
  auto retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  auto value = module.getValue(op->UnsignedValue());
  SPIRV_LL_ASSERT_PTR(value);

  module.addID(
      op->IdResult(), op,
      createConversionBuiltinCall(value, MangleInfo(op->UnsignedValue()), retTy,
                                  op->IdResultType(), op->IdResult()));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpUConvert>(const OpUConvert *op) {
  auto retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  auto value = module.getValue(op->UnsignedValue());
  SPIRV_LL_ASSERT_PTR(value);

  auto *const result = IRBuilder.CreateZExtOrTrunc(value, retTy);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSConvert>(const OpSConvert *op) {
  llvm::Type *retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  llvm::Value *value = module.getValue(op->SignedValue());
  SPIRV_LL_ASSERT_PTR(value);

  auto *const result = IRBuilder.CreateSExtOrTrunc(value, retTy);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFConvert>(const OpFConvert *op) {
  llvm::Value *value = module.getValue(op->FloatValue());
  SPIRV_LL_ASSERT_PTR(value);

  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  module.addID(op->IdResult(), op,
               createConversionBuiltinCall(value, {}, type, op->IdResult(),
                                           op->IdResult()));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpQuantizeToF16>(const OpQuantizeToF16 *op) {
  llvm::Value *val = module.getValue(op->Value());
  SPIRV_LL_ASSERT_PTR(val);

  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  module.addID(
      op->IdResult(), op,
      createMangledBuiltinCall("quantizeToF16", type, op->IdResultType(), {val},
                               MangleInfo(op->Value())));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpConvertPtrToU>(const OpConvertPtrToU *op) {
  llvm::Value *value = module.getValue(op->Pointer());
  SPIRV_LL_ASSERT_PTR(value);

  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *result = IRBuilder.CreatePtrToInt(value, type);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSatConvertSToU>(const OpSatConvertSToU *op) {
  SPIRV_LL_ASSERT(module.hasCapability(spv::CapabilityKernel),
                  "Kernel capability not enabled");

  auto retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  auto value = module.getValue(op->SignedValue());
  SPIRV_LL_ASSERT_PTR(value);

  // In this instruction value is always signed so don't pass its ID for
  // signedness lookup.
  auto result = createConversionBuiltinCall(
      value, {}, retTy, op->IdResultType(), op->IdResult(), true);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSatConvertUToS>(const OpSatConvertUToS *op) {
  SPIRV_LL_ASSERT(module.hasCapability(spv::CapabilityKernel),
                  "Kernel capability not enabled");

  auto retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  auto value = module.getValue(op->UnsignedValue());
  SPIRV_LL_ASSERT_PTR(value);

  // In this instruction result type is always signed so don't pass its ID for
  // signedness lookup.
  auto result = createConversionBuiltinCall(
      value, MangleInfo(op->UnsignedValue()), retTy,
      MangleInfo(op->IdResultType(), MangleInfo::ForceSignInfo::ForceSigned),
      op->IdResult(), true);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpConvertUToPtr>(const OpConvertUToPtr *op) {
  llvm::Value *value = module.getValue(op->IntegerValue());
  SPIRV_LL_ASSERT_PTR(value);

  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *result = IRBuilder.CreateIntToPtr(value, type);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpPtrCastToGeneric>(const OpPtrCastToGeneric *op) {
  llvm::Value *value = module.getValue(op->Pointer());
  SPIRV_LL_ASSERT_PTR(value);

  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *result = IRBuilder.CreatePointerCast(value, type);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGenericCastToPtr>(const OpGenericCastToPtr *op) {
  llvm::Value *value = module.getValue(op->Pointer());
  SPIRV_LL_ASSERT_PTR(value);

  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *result = IRBuilder.CreatePointerCast(value, type);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGenericCastToPtrExplicit>(
    const OpGenericCastToPtrExplicit *op) {
  llvm::Value *value = module.getValue(op->Pointer());
  SPIRV_LL_ASSERT_PTR(value);

  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *result = IRBuilder.CreatePointerCast(value, type);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpBitcast>(const OpBitcast *op) {
  llvm::Value *value = module.getValue(op->Operand());
  SPIRV_LL_ASSERT_PTR(value);

  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *result = IRBuilder.CreateBitCast(value, type);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSNegate>(const OpSNegate *op) {
  llvm::Value *value = module.getValue(op->Operand());

  SPIRV_LL_ASSERT_PTR(value);

  module.addID(op->IdResult(), op, IRBuilder.CreateNeg(value));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFNegate>(const OpFNegate *op) {
  llvm::Value *value = module.getValue(op->Operand());

  SPIRV_LL_ASSERT_PTR(value);

  module.addID(op->IdResult(), op, IRBuilder.CreateFNeg(value));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpIAdd>(const OpIAdd *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateAdd(lhs, rhs);
  module.addID(op->IdResult(), op, result);

  if (module.getFirstDecoration(op->IdResult(), spv::DecorationNoSignedWrap)) {
    llvm::cast<llvm::Instruction>(result)->setHasNoSignedWrap(true);
  } else if (module.getFirstDecoration(op->IdResult(),
                                       spv::DecorationNoUnsignedWrap)) {
    llvm::cast<llvm::Instruction>(result)->setHasNoUnsignedWrap(true);
  }

  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFAdd>(const OpFAdd *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateFAdd(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpISub>(const OpISub *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateSub(lhs, rhs);
  module.addID(op->IdResult(), op, result);

  if (module.getFirstDecoration(op->IdResult(), spv::DecorationNoSignedWrap)) {
    llvm::cast<llvm::Instruction>(result)->setHasNoSignedWrap(true);
  } else if (module.getFirstDecoration(op->IdResult(),
                                       spv::DecorationNoUnsignedWrap)) {
    llvm::cast<llvm::Instruction>(result)->setHasNoUnsignedWrap(true);
  }

  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFSub>(const OpFSub *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateFSub(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpIMul>(const OpIMul *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateMul(lhs, rhs);
  module.addID(op->IdResult(), op, result);

  if (module.getFirstDecoration(op->IdResult(), spv::DecorationNoSignedWrap)) {
    llvm::cast<llvm::Instruction>(result)->setHasNoSignedWrap(true);
  } else if (module.getFirstDecoration(op->IdResult(),
                                       spv::DecorationNoUnsignedWrap)) {
    llvm::cast<llvm::Instruction>(result)->setHasNoUnsignedWrap(true);
  }

  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFMul>(const OpFMul *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateFMul(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpUDiv>(const OpUDiv *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateUDiv(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSDiv>(const OpSDiv *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateSDiv(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFDiv>(const OpFDiv *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateFDiv(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpUMod>(const OpUMod *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateURem(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSRem>(const OpSRem *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateSRem(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSMod>(const OpSMod *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *num = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(num);

  llvm::Value *denom = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(denom);

  llvm::Constant *const zero = llvm::ConstantInt::getSigned(type, 0);
  llvm::Value *const cmp = IRBuilder.CreateICmpSLT(denom, zero);

  llvm::Value *const absDenom =
      IRBuilder.CreateSelect(cmp, IRBuilder.CreateNeg(denom), denom);

  llvm::Value *const sRem = IRBuilder.CreateSRem(num, denom);
  llvm::Value *const result =
      IRBuilder.CreateSelect(cmp, IRBuilder.CreateAdd(sRem, absDenom), sRem);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFRem>(const OpFRem *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = nullptr;

  llvm::Type *resultType = module.getLLVMType(op->IdResultType());
  result =
      createMangledBuiltinCall("fmod", resultType, op->IdResultType(),
                               {lhs, rhs}, {op->Operand1(), op->Operand2()});
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFMod>(const OpFMod *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = nullptr;
  llvm::Type *resultType = module.getLLVMType(op->IdResultType());
  result =
      createMangledBuiltinCall("fmod", resultType, op->IdResultType(),
                               {lhs, rhs}, {op->Operand1(), op->Operand2()});
  // Do copysign on the result with rhs because the spec for this instruction
  // dictates that non-zero results get their sign from rhs.
  llvm::Value *signCorrectedResult = createMangledBuiltinCall(
      "copysign", type, op->IdResultType(), {result, rhs}, {});

  module.addID(op->IdResult(), op, signCorrectedResult);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpVectorTimesScalar>(
    const OpVectorTimesScalar *op) {
  llvm::Value *scalarValue = module.getValue(op->Scalar());
  SPIRV_LL_ASSERT_PTR(scalarValue);

  llvm::Value *vectorValue = module.getValue(op->Vector());
  SPIRV_LL_ASSERT_PTR(vectorValue);

  auto *vectorType = llvm::dyn_cast<llvm::FixedVectorType>(
      module.getLLVMType(op->IdResultType()));
  SPIRV_LL_ASSERT_PTR(vectorType);

  llvm::Value *splatVector =
      IRBuilder.CreateVectorSplat(vectorType->getNumElements(), scalarValue);

  llvm::Value *result = IRBuilder.CreateFMul(splatVector, vectorValue);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpMatrixTimesScalar>(const OpMatrixTimesScalar *) {
  // TODO: To implement with abacus. See CA-341
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpVectorTimesMatrix>(const OpVectorTimesMatrix *) {
  // TODO: To implement with abacus. See CA-341
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpMatrixTimesVector>(const OpMatrixTimesVector *) {
  // TODO: To implement with abacus. See CA-341
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpMatrixTimesMatrix>(const OpMatrixTimesMatrix *) {
  // TODO: To implement with abacus. See CA-341
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpOuterProduct>(const OpOuterProduct *) {
  // TODO: To implement with abacus. See CA-341
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpDot>(const OpDot *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *lhs = module.getValue(op->Vector1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Vector2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result =
      createMangledBuiltinCall("dot", type, op->IdResultType(), {lhs, rhs},
                               {op->Vector1(), op->Vector2()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpIAddCarry>(const OpIAddCarry *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Type *operandType = rhs->getType();

  llvm::StructType *resultType =
      llvm::dyn_cast<llvm::StructType>(module.getLLVMType(op->IdResultType()));
  SPIRV_LL_ASSERT_PTR(resultType);

  std::string functionName;

  switch (operandType->getIntegerBitWidth()) {
    case 16:
    case 32:
    case 64:
      functionName = "llvm.uadd.with.overflow.i" +
                     std::to_string(operandType->getIntegerBitWidth());
      break;
    default:
      llvm_unreachable("Unsupported integer type passed to OpIAddCarry");
  }

  llvm::Function *intrinsic = module.llvmModule->getFunction(functionName);

  if (!intrinsic) {
    llvm::StructType *intrinsicRetType =
        llvm::StructType::get(operandType, IRBuilder.getInt1Ty());

    llvm::FunctionType *intrinsicFuncType = llvm::FunctionType::get(
        intrinsicRetType, {operandType, operandType}, false);

    intrinsic = llvm::Function::Create(intrinsicFuncType,
                                       llvm::GlobalValue::ExternalLinkage,
                                       functionName, module.llvmModule.get());
  }

  llvm::Value *intrinsicResult = IRBuilder.CreateCall(intrinsic, {lhs, rhs});

  llvm::Value *result = IRBuilder.CreateInsertValue(
      llvm::PoisonValue::get(resultType),
      IRBuilder.CreateExtractValue(intrinsicResult, 0), 0);

  // the llvm intrinsic returns {intTy, i1} whereas the SPIR-V is supposed to
  // return {intTy, intTy} so we need to do some casting on the second member
  llvm::Value *extBool = IRBuilder.CreateSExt(
      IRBuilder.CreateExtractValue(intrinsicResult, 1), operandType);

  result = IRBuilder.CreateInsertValue(result, extBool, 1);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpISubBorrow>(const OpISubBorrow *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Type *operandType = rhs->getType();

  llvm::StructType *resultType =
      llvm::dyn_cast<llvm::StructType>(module.getLLVMType(op->IdResultType()));
  SPIRV_LL_ASSERT_PTR(resultType);

  std::string functionName;

  switch (operandType->getIntegerBitWidth()) {
    case 16:
    case 32:
    case 64:
      functionName = "llvm.usub.with.overflow.i" +
                     std::to_string(operandType->getIntegerBitWidth());
      break;
    default:
      return makeStringError("Unsupported integer type passed to OpISubBorrow");
  }

  llvm::Function *intrinsic = module.llvmModule->getFunction(functionName);

  if (!intrinsic) {
    llvm::StructType *intrinsicRetType =
        llvm::StructType::get(operandType, IRBuilder.getInt1Ty());
    llvm::FunctionType *intrinsicFuncType = llvm::FunctionType::get(
        intrinsicRetType, {operandType, operandType}, false);

    intrinsic = llvm::Function::Create(intrinsicFuncType,
                                       llvm::GlobalValue::ExternalLinkage,
                                       functionName, module.llvmModule.get());
  }

  llvm::Value *intrinsicResult = IRBuilder.CreateCall(intrinsic, {lhs, rhs});

  llvm::Value *result = IRBuilder.CreateInsertValue(
      llvm::PoisonValue::get(resultType),
      IRBuilder.CreateExtractValue(intrinsicResult, 0), 0);

  // the llvm intrinsic returns {intTy, i1} whereas the SPIR-V is supposed to
  // return {intTy, intTy} so we need to do some casting on the second member
  llvm::Value *extBool = IRBuilder.CreateSExt(
      IRBuilder.CreateExtractValue(intrinsicResult, 1), operandType);

  result = IRBuilder.CreateInsertValue(result, extBool, 1);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpUMulExtended>(const OpUMulExtended *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Type *operandType = rhs->getType();

  llvm::Value *mul = IRBuilder.CreateMul(lhs, rhs);

  const unsigned nbBits = operandType->getPrimitiveSizeInBits();
  const uint64_t mask = (1 << (nbBits / 2)) - 1;
  llvm::Constant *lowOrderBitsMask = IRBuilder.getInt32(mask);
  llvm::Constant *highOrderBitsMask = IRBuilder.getInt32(~mask);

  llvm::Value *lowOrderBits = IRBuilder.CreateAnd(lowOrderBitsMask, mul);
  llvm::Value *highOrderBits = IRBuilder.CreateAnd(highOrderBitsMask, mul);

  llvm::StructType *type =
      llvm::dyn_cast<llvm::StructType>(module.getLLVMType(op->IdResultType()));
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *result =
      llvm::ConstantStruct::get(type, {llvm::PoisonValue::get(operandType),
                                       llvm::PoisonValue::get(operandType)});

  result = IRBuilder.CreateInsertValue(result, lowOrderBits, 0);
  result = IRBuilder.CreateInsertValue(result, highOrderBits, 1);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSMulExtended>(const OpSMulExtended *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Type *operandType = rhs->getType();

  llvm::Value *mul = IRBuilder.CreateMul(lhs, rhs);

  const unsigned nbBits = operandType->getPrimitiveSizeInBits();
  const uint64_t mask = (1 << (nbBits / 2)) - 1;
  llvm::Constant *lowOrderBitsMask = IRBuilder.getInt32(mask);
  llvm::Constant *highOrderBitsMask = IRBuilder.getInt32(~mask);

  llvm::Value *lowOrderBits = IRBuilder.CreateAnd(lowOrderBitsMask, mul);
  llvm::Value *highOrderBits = IRBuilder.CreateAnd(highOrderBitsMask, mul);

  llvm::StructType *type =
      llvm::dyn_cast<llvm::StructType>(module.getLLVMType(op->IdResultType()));
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *result =
      llvm::ConstantStruct::get(type, {llvm::PoisonValue::get(operandType),
                                       llvm::PoisonValue::get(operandType)});

  result = IRBuilder.CreateInsertValue(result, lowOrderBits, 0);
  result = IRBuilder.CreateInsertValue(result, highOrderBits, 1);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAny>(const OpAny *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *vector = module.getValue(op->Vector());
  SPIRV_LL_ASSERT_PTR(vector);

  const uint32_t num_elements =
      multi_llvm::getVectorNumElements(vector->getType());
  auto *extVectorType =
      llvm::FixedVectorType::get(IRBuilder.getInt32Ty(), num_elements);

  llvm::Value *extVector = IRBuilder.CreateSExt(vector, extVectorType);

  // The OpenCL version of 'any' takes an int type vector.
  // Custom mangle the builtin we're calling, so we mangle the arguments as a
  // vector of i32s. Ideally our mangling APIs would be able to handle this.
  const std::string mangledTy =
      getMangledVecPrefix(extVectorType) +
      getMangledIntName(IRBuilder.getInt32Ty(), /*isSigned*/ true);

  llvm::Value *result = createBuiltinCall(applyMangledLength("any") + mangledTy,
                                          IRBuilder.getInt32Ty(), {extVector});

  llvm::Value *truncResult = IRBuilder.CreateTrunc(result, type);

  module.addID(op->IdResult(), op, truncResult);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAll>(const OpAll *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *vector = module.getValue(op->Vector());
  SPIRV_LL_ASSERT_PTR(vector);

  const uint32_t num_elements =
      multi_llvm::getVectorNumElements(vector->getType());
  auto *extVectorType =
      llvm::FixedVectorType::get(IRBuilder.getInt32Ty(), num_elements);

  llvm::Value *extVector = IRBuilder.CreateSExt(vector, extVectorType);

  // The OpenCL version of 'all' takes an int type vector.
  // Custom mangle the builtin we're calling, so we mangle the arguments as a
  // vector of i32s. Ideally our mangling APIs would be able to handle this.
  const std::string mangledTy =
      getMangledVecPrefix(extVectorType) +
      getMangledIntName(IRBuilder.getInt32Ty(), /*isSigned*/ true);

  llvm::Value *result = createBuiltinCall(applyMangledLength("all") + mangledTy,
                                          IRBuilder.getInt32Ty(), {extVector});

  llvm::Value *truncResult = IRBuilder.CreateTrunc(result, type);

  module.addID(op->IdResult(), op, truncResult);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpIsNan>(const OpIsNan *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *resultType = getRelationalReturnType(x);

  llvm::Value *result = createMangledBuiltinCall(
      "isnan", resultType, MangleInfo::getSigned(op->IdResultType()), {x},
      {op->x()});

  result = IRBuilder.CreateTrunc(result, type);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpIsInf>(const OpIsInf *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *resultType = getRelationalReturnType(x);

  llvm::Value *result = createMangledBuiltinCall(
      "isinf", resultType, MangleInfo::getSigned(op->IdResultType()), {x},
      {op->x()});

  result = IRBuilder.CreateTrunc(result, type);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpIsFinite>(const OpIsFinite *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *resultType = getRelationalReturnType(x);

  llvm::Value *result = createMangledBuiltinCall(
      "isfinite", resultType, MangleInfo::getSigned(op->IdResultType()), {x},
      {op->x()});

  result = IRBuilder.CreateTrunc(result, type);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpIsNormal>(const OpIsNormal *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *resultType = getRelationalReturnType(x);

  llvm::Value *result = createMangledBuiltinCall(
      "isnormal", resultType, MangleInfo::getSigned(op->IdResultType()), {x},
      {op->x()});

  result = IRBuilder.CreateTrunc(result, type);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSignBitSet>(const OpSignBitSet *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Type *resultType = getRelationalReturnType(x);

  llvm::Value *result = createMangledBuiltinCall(
      "signbit", resultType, MangleInfo::getSigned(op->IdResultType()), {x},
      {op->x()});

  result = IRBuilder.CreateTrunc(result, type);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpLessOrGreater>(const OpLessOrGreater *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Type *resultType = getRelationalReturnType(x);

  llvm::Value *result = createMangledBuiltinCall(
      "islessgreater", resultType, MangleInfo::getSigned(op->IdResultType()),
      {x, y}, {op->x(), op->y()});

  result = IRBuilder.CreateTrunc(result, type);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpOrdered>(const OpOrdered *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Type *resultType = getRelationalReturnType(x);

  llvm::Value *result = createMangledBuiltinCall(
      "isordered", resultType, MangleInfo::getSigned(op->IdResultType()),
      {x, y}, {op->x(), op->y()});

  result = IRBuilder.CreateTrunc(result, type);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpUnordered>(const OpUnordered *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *x = module.getValue(op->x());
  SPIRV_LL_ASSERT_PTR(x);

  llvm::Value *y = module.getValue(op->y());
  SPIRV_LL_ASSERT_PTR(y);

  llvm::Type *resultType = getRelationalReturnType(x);

  llvm::Value *result = createMangledBuiltinCall(
      "isunordered", resultType, MangleInfo::getSigned(op->IdResultType()),
      {x, y}, {op->x(), op->y()});

  result = IRBuilder.CreateTrunc(result, type);

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpLogicalEqual>(const OpLogicalEqual *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateICmpEQ(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpLogicalNotEqual>(const OpLogicalNotEqual *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateICmpNE(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpLogicalOr>(const OpLogicalOr *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateOr(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpLogicalAnd>(const OpLogicalAnd *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateAnd(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpLogicalNot>(const OpLogicalNot *op) {
  llvm::Value *value = module.getValue(op->Operand());
  SPIRV_LL_ASSERT_PTR(value);

  llvm::Value *result = IRBuilder.CreateNot(value);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSelect>(const OpSelect *op) {
  llvm::Value *condition = module.getValue(op->Condition());
  SPIRV_LL_ASSERT_PTR(condition);

  llvm::Value *obj1 = module.getValue(op->Object1());
  SPIRV_LL_ASSERT_PTR(obj1);

  llvm::Value *obj2 = module.getValue(op->Object2());
  SPIRV_LL_ASSERT_PTR(obj2);

  llvm::Value *result = IRBuilder.CreateSelect(condition, obj1, obj2);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpIEqual>(const OpIEqual *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateICmpEQ(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpINotEqual>(const OpINotEqual *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateICmpNE(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpUGreaterThan>(const OpUGreaterThan *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateICmpUGT(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSGreaterThan>(const OpSGreaterThan *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateICmpSGT(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpUGreaterThanEqual>(
    const OpUGreaterThanEqual *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateICmpUGE(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSGreaterThanEqual>(
    const OpSGreaterThanEqual *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateICmpSGE(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpULessThan>(const OpULessThan *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateICmpULT(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSLessThan>(const OpSLessThan *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateICmpSLT(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpULessThanEqual>(const OpULessThanEqual *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateICmpULE(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSLessThanEqual>(const OpSLessThanEqual *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateICmpSLE(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFOrdEqual>(const OpFOrdEqual *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateFCmpOEQ(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFUnordEqual>(const OpFUnordEqual *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateFCmpUEQ(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFOrdNotEqual>(const OpFOrdNotEqual *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateFCmpONE(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFUnordNotEqual>(const OpFUnordNotEqual *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateFCmpUNE(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFOrdLessThan>(const OpFOrdLessThan *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateFCmpOLT(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFUnordLessThan>(const OpFUnordLessThan *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateFCmpULT(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFOrdGreaterThan>(const OpFOrdGreaterThan *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateFCmpOGT(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFUnordGreaterThan>(
    const OpFUnordGreaterThan *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateFCmpUGT(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFOrdLessThanEqual>(
    const OpFOrdLessThanEqual *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateFCmpOLE(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFUnordLessThanEqual>(
    const OpFUnordLessThanEqual *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateFCmpULE(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFOrdGreaterThanEqual>(
    const OpFOrdGreaterThanEqual *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateFCmpOGE(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFUnordGreaterThanEqual>(
    const OpFUnordGreaterThanEqual *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  llvm::Value *result = IRBuilder.CreateFCmpUGE(lhs, rhs);
  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpShiftRightLogical>(
    const OpShiftRightLogical *op) {
  llvm::Value *base = module.getValue(op->Base());

  SPIRV_LL_ASSERT_PTR(base);

  llvm::Value *shift = module.getValue(op->Shift());

  SPIRV_LL_ASSERT_PTR(shift);

  module.addID(op->IdResult(), op, IRBuilder.CreateLShr(base, shift));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpShiftRightArithmetic>(
    const OpShiftRightArithmetic *op) {
  llvm::Value *base = module.getValue(op->Base());

  SPIRV_LL_ASSERT_PTR(base);

  llvm::Value *shift = module.getValue(op->Shift());

  SPIRV_LL_ASSERT_PTR(shift);

  module.addID(op->IdResult(), op, IRBuilder.CreateAShr(base, shift));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpShiftLeftLogical>(const OpShiftLeftLogical *op) {
  llvm::Value *base = module.getValue(op->Base());
  SPIRV_LL_ASSERT_PTR(base);

  llvm::Value *shift = module.getValue(op->Shift());
  SPIRV_LL_ASSERT_PTR(shift);

  llvm::Value *result = IRBuilder.CreateShl(base, shift);

  module.addID(op->IdResult(), op, result);

  if (module.getFirstDecoration(op->IdResult(), spv::DecorationNoSignedWrap)) {
    llvm::cast<llvm::Instruction>(result)->setHasNoSignedWrap(true);
  } else if (module.getFirstDecoration(op->IdResult(),
                                       spv::DecorationNoUnsignedWrap)) {
    llvm::cast<llvm::Instruction>(result)->setHasNoUnsignedWrap(true);
  }

  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpBitwiseOr>(const OpBitwiseOr *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  module.addID(op->IdResult(), op, IRBuilder.CreateOr(lhs, rhs));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpBitwiseXor>(const OpBitwiseXor *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  module.addID(op->IdResult(), op, IRBuilder.CreateXor(lhs, rhs));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpBitwiseAnd>(const OpBitwiseAnd *op) {
  llvm::Value *lhs = module.getValue(op->Operand1());
  SPIRV_LL_ASSERT_PTR(lhs);

  llvm::Value *rhs = module.getValue(op->Operand2());
  SPIRV_LL_ASSERT_PTR(rhs);

  module.addID(op->IdResult(), op, IRBuilder.CreateAnd(lhs, rhs));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpNot>(const OpNot *op) {
  llvm::Value *operand = module.getValue(op->Operand());
  SPIRV_LL_ASSERT_PTR(operand);

  module.addID(op->IdResult(), op, IRBuilder.CreateNot(operand));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpBitFieldInsert>(const OpBitFieldInsert *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *base = module.getValue(op->Base());
  SPIRV_LL_ASSERT_PTR(base);

  llvm::Value *insert = module.getValue(op->Insert());
  SPIRV_LL_ASSERT_PTR(insert);

  llvm::Value *offset = module.getValue(op->Offset());
  SPIRV_LL_ASSERT_PTR(offset);

  llvm::Value *count = module.getValue(op->Count());
  SPIRV_LL_ASSERT_PTR(count);

  llvm::Value *one = nullptr;
  if (type->isVectorTy()) {
    const uint32_t num_elements = multi_llvm::getVectorNumElements(type);
    one = IRBuilder.CreateVectorSplat(
        num_elements,
        llvm::ConstantInt::get(multi_llvm::getVectorElementType(type), 1));
    offset = IRBuilder.CreateVectorSplat(num_elements, offset);
    count = IRBuilder.CreateVectorSplat(num_elements, count);
  } else {
    one = llvm::ConstantInt::get(type, 1);
  }

  // obtain a mask for the range in we wish to insert
  llvm::Value *insert_mask = IRBuilder.CreateShl(one, count);
  insert_mask = IRBuilder.CreateSub(insert_mask, one);
  // apply the mask
  insert = IRBuilder.CreateAnd(insert_mask, insert);
  // shift the resultant value to align with where it is to be inserted
  insert = IRBuilder.CreateShl(insert, offset);
  // now create a mask to zero the bits in base that are to be replaced with
  // the inserted value
  llvm::Value *shift_bmask = IRBuilder.CreateSub(offset, one);
  llvm::Value *base_mask = IRBuilder.CreateShl(insert_mask, shift_bmask);
  base_mask = IRBuilder.CreateNot(base_mask);

  // apply the mask, zeroing the bits
  base = IRBuilder.CreateAnd(base, base_mask);
  // or the base and the insert to arrive at the resultant combined bit field
  module.addID(op->IdResult(), op, IRBuilder.CreateOr(base, insert));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpBitFieldSExtract>(const OpBitFieldSExtract *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *base = module.getValue(op->Base());
  SPIRV_LL_ASSERT_PTR(base);

  llvm::Value *offset = module.getValue(op->Offset());
  SPIRV_LL_ASSERT_PTR(offset);

  llvm::Value *count = module.getValue(op->Count());
  SPIRV_LL_ASSERT_PTR(count);

  llvm::Constant *one;
  if (type->isVectorTy()) {
    one = llvm::ConstantInt::get(multi_llvm::getVectorElementType(type), 1);
  } else {
    one = llvm::ConstantInt::get(type, 1);
  }

  // create our mask by shifting 1 left and subtracting one from the result
  llvm::Value *mask = IRBuilder.CreateShl(one, count);
  mask = IRBuilder.CreateSub(mask, one);

  if (type->isIntegerTy()) {
    // shift right to make our desired range start in the lsb
    base = IRBuilder.CreateAShr(base, offset);
    // final result is base & mask
    module.addID(op->IdResult(), op, IRBuilder.CreateAnd(base, mask));
  } else if (type->isVectorTy()) {
    const uint32_t num_elements = multi_llvm::getVectorNumElements(type);
    llvm::Value *offset_vec = IRBuilder.CreateVectorSplat(num_elements, offset);
    llvm::Value *mask_vec = IRBuilder.CreateVectorSplat(num_elements, mask);
    // shift right to make our desired range start in the lsb
    base = IRBuilder.CreateAShr(base, offset_vec);
    module.addID(op->IdResult(), op, IRBuilder.CreateAnd(base, mask_vec));
  }
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpBitFieldUExtract>(const OpBitFieldUExtract *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *base = module.getValue(op->Base());
  SPIRV_LL_ASSERT_PTR(base);

  llvm::Value *offset = module.getValue(op->Offset());
  SPIRV_LL_ASSERT_PTR(offset);

  llvm::Value *count = module.getValue(op->Count());
  SPIRV_LL_ASSERT_PTR(count);

  llvm::Constant *one;
  if (type->isVectorTy()) {
    one = llvm::ConstantInt::get(multi_llvm::getVectorElementType(type), 1);
  } else {
    one = llvm::ConstantInt::get(type, 1);
  }

  // create our mask by shifting 1 left and subtracting one from the result
  llvm::Value *mask = IRBuilder.CreateShl(one, count);
  mask = IRBuilder.CreateSub(mask, one);

  if (type->isIntegerTy()) {
    // shift right to make our desired range start in the lsb
    base = IRBuilder.CreateLShr(base, offset);
    // final result is base & mask
    module.addID(op->IdResult(), op, IRBuilder.CreateAnd(base, mask));
  } else if (type->isVectorTy()) {
    const uint32_t num_elements = multi_llvm::getVectorNumElements(type);
    llvm::Value *offset_vec = IRBuilder.CreateVectorSplat(num_elements, offset);
    llvm::Value *mask_vec = IRBuilder.CreateVectorSplat(num_elements, mask);
    // shift right to make our desired range start in the lsb
    base = IRBuilder.CreateLShr(base, offset_vec);
    module.addID(op->IdResult(), op, IRBuilder.CreateAnd(base, mask_vec));
  }
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpBitReverse>(const OpBitReverse *) {
  // TODO: implement this as a builtin
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpBitCount>(const OpBitCount *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  llvm::Value *base = module.getValue(op->Base());
  SPIRV_LL_ASSERT_PTR(base);

  llvm::Value *result = createMangledBuiltinCall(
      "popcount", type, op->IdResultType(), {base}, {op->Base()});

  module.addID(op->IdResult(), op, result);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpDPdx>(const OpDPdx *) {
  // This instruction is only valid in the Fragment execuction model, which is
  // not supported.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpDPdy>(const OpDPdy *) {
  // This instruction is only valid in the Fragment execuction model, which is
  // not supported.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFwidth>(const OpFwidth *) {
  // This instruction is only valid in the Fragment execuction model, which is
  // not supported.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpDPdxFine>(const OpDPdxFine *) {
  // This instruction is only valid in the Fragment execuction model, which is
  // not supported.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpDPdyFine>(const OpDPdyFine *) {
  // This instruction is only valid in the Fragment execuction model, which is
  // not supported.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFwidthFine>(const OpFwidthFine *) {
  // This instruction is only valid in the Fragment execuction model, which is
  // not supported.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpDPdxCoarse>(const OpDPdxCoarse *) {
  // This instruction is only valid in the Fragment execuction model, which is
  // not supported.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpDPdyCoarse>(const OpDPdyCoarse *) {
  // This instruction is only valid in the Fragment execuction model, which is
  // not supported.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpFwidthCoarse>(const OpFwidthCoarse *) {
  // This instruction is only valid in the Fragment execuction model, which is
  // not supported.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpEmitVertex>(const OpEmitVertex *) {
  // This instruction requires the Geometry capability, which is not supported.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpEndPrimitive>(const OpEndPrimitive *) {
  // This instruction requires the Geometry capability, which is not supported.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpEmitStreamVertex>(const OpEmitStreamVertex *) {
  // This instruction requires the GeometryStreams capability, which is not
  // supported.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpEndStreamPrimitive>(
    const OpEndStreamPrimitive *) {
  // This instruction requires the GeometryStreams capability, which is not
  // supported.
  return llvm::Error::success();
}

static llvm::AttributeList getMuxBarrierAttrs(llvm::LLVMContext &ctx) {
  // Return a list of attributes that must be set on barrier builtins. Merging
  // and duplicating are not safe!
  llvm::AttributeList AL;
  AL = AL.addFnAttribute(ctx, llvm::Attribute::NoMerge);
  AL = AL.addFnAttribute(ctx, llvm::Attribute::NoDuplicate);
  return AL;
}

template <>
llvm::Error Builder::create<OpControlBarrier>(const OpControlBarrier *op) {
  auto *execution = module.getValue(op->Execution());
  SPIRV_LL_ASSERT_PTR(execution);

  auto *memory = module.getValue(op->Memory());
  SPIRV_LL_ASSERT_PTR(memory);

  auto *semantics = module.getValue(op->Semantics());
  SPIRV_LL_ASSERT_PTR(semantics);

  auto *defaultID = IRBuilder.getInt32(0);
  // We have one of two mux barriers to choose from: __mux_sub_group_barrier or
  // __mux_work_group_barrier. This may have to be decided at runtime,
  // depending on the 'execution' operand.

  // The mux enumeration values for 'scope' and 'semantics' are identical to
  // the SPIR-V ones, so we can just pass operands straight through.

  const auto wgBarrierName = "__mux_work_group_barrier";
  const auto sgBarrierName = "__mux_sub_group_barrier";
  // If it's constant (which is most likely is) emit the right barrier
  // directly.
  if (auto *exe_const = dyn_cast<llvm::ConstantInt>(execution)) {
    // Emit a sub-group barrier if instruction, else generate a work-group
    // barrier for all others. There are only two valid values for OpenCL
    // environments, so we could be stricter here.
    auto barrierName = exe_const->getZExtValue() == spv::ScopeSubgroup
                           ? sgBarrierName
                           : wgBarrierName;
    auto *const ci = createBuiltinCall(barrierName, IRBuilder.getVoidTy(),
                                       {defaultID, memory, semantics});
    ci->getCalledFunction()->setAttributes(
        getMuxBarrierAttrs(*context.llvmContext));
    return llvm::Error::success();
  }

  // If it's not a constant, emit a wrapper function which dynamically chooses
  // between the two barriers.
  if (!module.barrierWrapperFcn) {
    auto *barrierWrapperFcnTy = llvm::FunctionType::get(
        llvm::Type::getVoidTy(*context.llvmContext),
        {execution->getType(), memory->getType(), semantics->getType()},
        /* isVarArg */ false);
    module.barrierWrapperFcn = llvm::Function::Create(
        barrierWrapperFcnTy, llvm::Function::LinkageTypes::InternalLinkage,
        "barrier_wrapper", module.llvmModule.get());
    module.barrierWrapperFcn->setConvergent();

    auto insertPoint = IRBuilder.GetInsertPoint();
    auto *insertBB = IRBuilder.GetInsertBlock();

    auto *entry = llvm::BasicBlock::Create(*context.llvmContext, "entry",
                                           module.barrierWrapperFcn);
    IRBuilder.SetInsertPoint(entry);

    auto *executionArg = module.barrierWrapperFcn->getArg(0);
    auto *memoryArg = module.barrierWrapperFcn->getArg(1);
    auto *semanticsArg = module.barrierWrapperFcn->getArg(2);

    llvm::BasicBlock *subgroupBB = llvm::BasicBlock::Create(
        *context.llvmContext, "subgroup.barrier", module.barrierWrapperFcn);

    llvm::BasicBlock *workgroupBB = llvm::BasicBlock::Create(
        *context.llvmContext, "workgroup.barrier", module.barrierWrapperFcn);

    llvm::BasicBlock *exitBB = llvm::BasicBlock::Create(
        *context.llvmContext, "exit", module.barrierWrapperFcn);

    auto *cmp = IRBuilder.CreateICmpEQ(
        executionArg, IRBuilder.getInt32(spv::ScopeSubgroup), "is.sg");

    IRBuilder.CreateCondBr(cmp, subgroupBB, workgroupBB);

    {
      IRBuilder.SetInsertPoint(subgroupBB);
      auto *const ci = createBuiltinCall(sgBarrierName, IRBuilder.getVoidTy(),
                                         {defaultID, memoryArg, semanticsArg});
      ci->getCalledFunction()->setAttributes(
          getMuxBarrierAttrs(*context.llvmContext));
      IRBuilder.CreateBr(exitBB);
    }

    {
      IRBuilder.SetInsertPoint(workgroupBB);
      IRBuilder.SetInsertPoint(subgroupBB);
      auto *const ci = createBuiltinCall(wgBarrierName, IRBuilder.getVoidTy(),
                                         {defaultID, memoryArg, semanticsArg});
      ci->getCalledFunction()->setAttributes(
          getMuxBarrierAttrs(*context.llvmContext));
      IRBuilder.CreateBr(exitBB);
    }

    {
      IRBuilder.SetInsertPoint(exitBB);
      IRBuilder.CreateRetVoid();
    }

    module.barrierWrapperFcn->addFnAttr(llvm::Attribute::AlwaysInline);
    IRBuilder.SetInsertPoint(insertBB, insertPoint);
  }
  // Now we just have a regular function call in our original function.
  IRBuilder.CreateCall(module.barrierWrapperFcn,
                       {execution, memory, semantics});
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpMemoryBarrier>(const OpMemoryBarrier *op) {
  llvm::Value *semantics = module.getValue(op->Semantics());
  SPIRV_LL_ASSERT_PTR(semantics);

  llvm::Value *scope = module.getValue(op->Memory());
  SPIRV_LL_ASSERT_PTR(scope);

  auto *const ci = createBuiltinCall("__mux_mem_barrier", IRBuilder.getVoidTy(),
                                     {scope, semantics});
  ci->getCalledFunction()->setAttributes(
      getMuxBarrierAttrs(*context.llvmContext));

  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicLoad>(const OpAtomicLoad *op) {
  auto pointer = module.getValue(op->Pointer());
  SPIRV_LL_ASSERT_PTR(pointer);

  auto scope = module.getValue(op->Scope());
  SPIRV_LL_ASSERT_PTR(scope);

  auto semantics = module.getValue(op->Semantics());
  SPIRV_LL_ASSERT_PTR(semantics);

  auto retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  module.addID(
      op->IdResult(), op,
      createMangledBuiltinCall("atomic_load_explicit", retTy,
                               op->IdResultType(), {pointer, semantics, scope},
                               {MangleInfo(op->Pointer(), MangleInfo::VOLATILE),
                                op->Semantics(), op->Scope()}));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicStore>(const OpAtomicStore *op) {
  auto pointer = module.getValue(op->Pointer());
  SPIRV_LL_ASSERT_PTR(pointer);

  auto scope = module.getValue(op->Scope());
  SPIRV_LL_ASSERT_PTR(scope);

  auto semantics = module.getValue(op->Semantics());
  SPIRV_LL_ASSERT_PTR(semantics);

  auto value = module.getValue(op->Value());
  SPIRV_LL_ASSERT_PTR(value);

  createMangledBuiltinCall("atomic_store_explicit", IRBuilder.getVoidTy(),
                           spv::OpTypeVoid, {pointer, value, semantics, scope},
                           {MangleInfo(op->Pointer(), MangleInfo::VOLATILE),
                            op->Value(), op->Semantics(), op->Scope()});
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicExchange>(const OpAtomicExchange *op) {
  const auto retOp = op->IdResultType();
  // Atomic exchange can work on floats or integers.
  const auto *const type = module.getLLVMType(retOp);
  const auto is_signed =
      !type->isFloatingPointTy() && module.get<OpTypeInt>(retOp)->Signedness();
  generateBinaryAtomic(op, op->Pointer(), op->Value(), "atomic_xchg",
                       is_signed);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicCompareExchange>(
    const OpAtomicCompareExchange *op) {
  auto pointer = module.getValue(op->Pointer());
  SPIRV_LL_ASSERT_PTR(pointer);

  // storage class Function is valid but undefined behaviour, so just return the
  // orginal value as the instruction should
  if (pointer->getType()->getPointerAddressSpace() == 0) {
    auto *resultTy = module.getLLVMType(op->IdResultType());
    module.addID(op->IdResult(), op, IRBuilder.CreateLoad(resultTy, pointer));
    return llvm::Error::success();
  }

  auto value = module.getValue(op->Value());
  SPIRV_LL_ASSERT_PTR(value);

  auto cmp = module.getValue(op->Comparator());
  SPIRV_LL_ASSERT_PTR(cmp);

  auto retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  module.addID(
      op->IdResult(), op,
      createMangledBuiltinCall("atomic_cmpxchg", retTy, op->IdResultType(),
                               {pointer, cmp, value},
                               {MangleInfo(op->Pointer(), MangleInfo::VOLATILE),
                                op->Comparator(), op->Value()}));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicCompareExchangeWeak>(
    const OpAtomicCompareExchangeWeak *op) {
  // Has the same semantics as OpAtomicCompareExchange.
  return Builder::create<OpAtomicCompareExchange>(*op);
}

template <>
llvm::Error Builder::create<OpAtomicIIncrement>(const OpAtomicIIncrement *op) {
  llvm::Value *pointer = module.getValue(op->Pointer());
  SPIRV_LL_ASSERT_PTR(pointer);

  llvm::Type *retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  module.addID(op->IdResult(), op,
               createMangledBuiltinCall(
                   "atomic_inc", retTy, op->IdResultType(), pointer,
                   MangleInfo(op->Pointer(), MangleInfo::VOLATILE)));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicIDecrement>(const OpAtomicIDecrement *op) {
  llvm::Value *pointer = module.getValue(op->Pointer());
  SPIRV_LL_ASSERT_PTR(pointer);

  llvm::Type *retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  module.addID(op->IdResult(), op,
               createMangledBuiltinCall(
                   "atomic_dec", retTy, op->IdResultType(), pointer,
                   MangleInfo(op->Pointer(), MangleInfo::VOLATILE)));
  return llvm::Error::success();
}

void Builder::generateBinaryAtomic(const OpResult *op, spv::Id pointerID,
                                   spv::Id valueID, const std::string &function,
                                   bool args_are_signed) {
  auto pointer = module.getValue(pointerID);
  SPIRV_LL_ASSERT_PTR(pointer);

  // storage class Function is valid but undefined behaviour, so just return the
  // orginal value as the instruction should
  if (pointer->getType()->getPointerAddressSpace() == 0) {
    module.addID(
        op->IdResult(), op,
        IRBuilder.CreateLoad(module.getLLVMType(op->IdResultType()), pointer));
    return;
  }

  auto value = module.getValue(valueID);
  SPIRV_LL_ASSERT_PTR(value);

  llvm::Type *retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  llvm::Type *value_type = value->getType();
  const std::string mangled_value_type =
      value_type->isIntegerTy()
          ? getMangledIntName(value_type, args_are_signed)
          : getMangledTypeName(value_type, MangleInfo(valueID), {});
  std::string mangled_name = applyMangledLength(function);
  // We know that binary atomic operations have two arguments: a pointer, and an
  // int of the same type.
  mangled_name += getMangledPointerPrefix(pointer->getType()) + "V" +
                  mangled_value_type + mangled_value_type;
  module.addID(op->IdResult(), op,
               createBuiltinCall(mangled_name, retTy, {pointer, value}));
}

template <>
llvm::Error Builder::create<OpAtomicIAdd>(const OpAtomicIAdd *op) {
  const auto retOp = op->IdResultType();
  const auto is_signed = module.get<OpTypeInt>(retOp)->Signedness();
  generateBinaryAtomic(op, op->Pointer(), op->Value(), "atomic_add", is_signed);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicISub>(const OpAtomicISub *op) {
  const auto retOp = op->IdResultType();
  const auto is_signed = module.get<OpTypeInt>(retOp)->Signedness();
  generateBinaryAtomic(op, op->Pointer(), op->Value(), "atomic_sub", is_signed);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicSMin>(const OpAtomicSMin *op) {
  generateBinaryAtomic(op, op->Pointer(), op->Value(), "atomic_min", true);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicUMin>(const OpAtomicUMin *op) {
  generateBinaryAtomic(op, op->Pointer(), op->Value(), "atomic_min", false);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicSMax>(const OpAtomicSMax *op) {
  generateBinaryAtomic(op, op->Pointer(), op->Value(), "atomic_max", true);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicUMax>(const OpAtomicUMax *op) {
  generateBinaryAtomic(op, op->Pointer(), op->Value(), "atomic_max", false);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicFAddEXT>(const OpAtomicFAddEXT *op) {
  generateBinaryAtomic(op, op->Pointer(), op->Value(),
                       "atomic_fetch_add_explicit", true);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicFMinEXT>(const OpAtomicFMinEXT *op) {
  generateBinaryAtomic(op, op->Pointer(), op->Value(),
                       "atomic_fetch_min_explicit", true);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicFMaxEXT>(const OpAtomicFMaxEXT *op) {
  generateBinaryAtomic(op, op->Pointer(), op->Value(),
                       "atomic_fetch_max_explicit", true);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicAnd>(const OpAtomicAnd *op) {
  const auto retOp = op->IdResultType();
  const auto is_signed = module.get<OpTypeInt>(retOp)->Signedness();
  generateBinaryAtomic(op, op->Pointer(), op->Value(), "atomic_and", is_signed);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicOr>(const OpAtomicOr *op) {
  const auto retOp = op->IdResultType();
  const auto is_signed = module.get<OpTypeInt>(retOp)->Signedness();
  generateBinaryAtomic(op, op->Pointer(), op->Value(), "atomic_or", is_signed);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicXor>(const OpAtomicXor *op) {
  const auto retOp = op->IdResultType();
  const auto is_signed = module.get<OpTypeInt>(retOp)->Signedness();
  generateBinaryAtomic(op, op->Pointer(), op->Value(), "atomic_xor", is_signed);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpPhi>(const OpPhi *op) {
  const unsigned num_values = op->wordCount() - 3;
  llvm::Type *result_ty = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(result_ty);

  llvm::PHINode *phi = IRBuilder.CreatePHI(result_ty, num_values);

  // We will not be adding the incoming edges and values here since some of the
  // basic blocks might not exist yet. Instead they will be added later by
  // populatePhi()

  module.addID(op->IdResult(), op, phi);
  return llvm::Error::success();
}

void Builder::populatePhi(const OpPhi &op) {
  llvm::Value *value = module.getValue(op.IdResult());
  SPIRV_LL_ASSERT_PTR(value);
  llvm::PHINode *phi = llvm::dyn_cast<llvm::PHINode>(value);
  SPIRV_LL_ASSERT_PTR(phi);

  for (auto pair : op.VariableParent()) {
    llvm::Value *value = module.getValue(pair.Variable);
    SPIRV_LL_ASSERT_PTR(value);
    llvm::Value *block_val = module.getValue(pair.Parent);
    SPIRV_LL_ASSERT_PTR(block_val);
    llvm::BasicBlock *block = llvm::dyn_cast<llvm::BasicBlock>(block_val);
    SPIRV_LL_ASSERT_PTR(block);
    phi->addIncoming(value, block);
  }
}

template <>
llvm::Error Builder::create<OpLoopMerge>(const OpLoopMerge *op) {
  llvm::MDNode *loop_control = nullptr;

  // account for the technically legal combination of Unroll and DontUnroll
  // the casts are necessary because the spir-v headers overload operator| for
  // their enums and those overloads are not constexpr
  constexpr uint32_t UnrollDontUnroll =
      static_cast<uint32_t>(spv::LoopControlUnrollMask) |
      static_cast<uint32_t>(spv::LoopControlDontUnrollMask);

  switch (op->LoopControl()) {
    case spv::LoopControlMaskNone:
    case UnrollDontUnroll:
      break;
    case spv::LoopControlUnrollMask:
      loop_control = llvm::MDNode::get(
          *context.llvmContext,
          llvm::MDString::get(*context.llvmContext, "llvm.loop.unroll.enable"));
      break;
    case spv::LoopControlDontUnrollMask:
      loop_control =
          llvm::MDNode::get(*context.llvmContext,
                            llvm::MDString::get(*context.llvmContext,
                                                "llvm.loop.unroll.disable"));
      break;
    default:
      llvm_unreachable("Invalid loop control value provided to OpLoopMerge!");
  }

  if (loop_control) {
    module.setLoopControl(op->ContinueTarget(), loop_control);
  }

  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSelectionMerge>(const OpSelectionMerge *) {
  // This instruction does not have any semantic impact on the module, and
  // unlike what we can do with loop unrolling above there is no llvm mapping
  // for control flow flattening.
  return llvm::Error::success();
}

llvm::BasicBlock *Builder::getOrCreateBasicBlock(spv::Id label) {
  auto *bb = llvm::dyn_cast_or_null<llvm::BasicBlock>(module.getValue(label));
  if (bb) {
    return bb;
  }

  llvm::Function *current_function = getCurrentFunction();
  SPIRV_LL_ASSERT_PTR(current_function);

  bb = llvm::BasicBlock::Create(*context.llvmContext, module.getName(label),
                                current_function);
  module.addID(label, /*Op*/ nullptr, bb);
  return bb;
}

template <>
llvm::Error Builder::create<OpLabel>(const OpLabel *op) {
  llvm::Function *current_function = getCurrentFunction();
  SPIRV_LL_ASSERT_PTR(current_function);

  auto *bb = getOrCreateBasicBlock(op->IdResult());
  SPIRV_LL_ASSERT_PTR(bb);

  // If we've already created this basic block before reaching the OpLabel
  // (through a forward reference), then it's in the "wrong" place in terms of
  // the linear layout of the function. Remove and re-insert the basic block at
  // the end of the current function.
  if (bb->getIterator() != std::prev(current_function->end())) {
    bb->removeFromParent();
    current_function->insert(current_function->end(), bb);
  }

  IRBuilder.SetInsertPoint(bb);

  // If this was the first basic block in a function check for and add any spec
  // constant instructions that may have been deferred, and deal with any
  // interface blocks that need to be loaded/stored.
  if (current_function->size() == 1) {
    generateSpecConstantOps();
  }

  // If there's a line range currently open at this point, create and register
  // a DILexicalBlock for this function. If there isn't, we'll generate one on
  // the fly when we hit an OpLine but it'll have that OpLine's line/column
  // information.
  // Note that it's legal for there to be an open line range before the first
  // basic block in a function, but not any subsequent ones, because all blocks
  // must end in a block termination instruction, and those close line ranges.
  //   OpLine           <- new line range opens here
  //     OpFunction
  //       OpLine       <- new line range opens here; old one closes
  //         OpLabel
  //         OpBranch   <- line range closes here
  if (auto current_range = getCurrentOpLineRange();
      current_range && module.useImplicitDebugScopes()) {
    getOrCreateDebugBasicBlockScope(*bb, current_range->op_line);
  }

  module.addID(op->IdResult(), op, bb);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpBranch>(const OpBranch *op) {
  llvm::BasicBlock *bb = getOrCreateBasicBlock(op->TargetLabel());
  SPIRV_LL_ASSERT_PTR(bb);

  IRBuilder.CreateBr(bb);

  // This instruction ends a block, and thus a scope.
  closeCurrentLexicalScope();

  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpBranchConditional>(
    const OpBranchConditional *op) {
  llvm::BasicBlock *true_bb = getOrCreateBasicBlock(op->TrueLabel());
  SPIRV_LL_ASSERT_PTR(true_bb);
  llvm::BasicBlock *false_bb = getOrCreateBasicBlock(op->FalseLabel());
  SPIRV_LL_ASSERT_PTR(false_bb);
  llvm::Value *cond = module.getValue(op->Condition());
  SPIRV_LL_ASSERT_PTR(cond);

  auto branch_inst = IRBuilder.CreateCondBr(cond, true_bb, false_bb);

  // need to store the node and a stringref for the kind
  llvm::SmallVector<std::pair<llvm::MDNode *, llvm::StringRef>, 2> md_nodes;

  // check for branch weights
  auto branchWeights = op->BranchWeights();
  if (branchWeights.size() == 2) {
    const llvm::SmallVector<llvm::Metadata *, 3> mds = {
        llvm::MDString::get(*context.llvmContext, "branch_weights"),
        llvm::ConstantAsMetadata::get(IRBuilder.getInt32(branchWeights[0])),
        llvm::ConstantAsMetadata::get(IRBuilder.getInt32(branchWeights[1]))};

    md_nodes.push_back(
        std::make_pair(llvm::MDTuple::get(*context.llvmContext, mds), "prof"));
  }

  if (!md_nodes.empty()) {
    // if there was only one MDNode for this instruction, apply it with the
    // associated kind
    if (md_nodes.size() == 1) {
      branch_inst->setMetadata(md_nodes.front().second, md_nodes.front().first);
    } else {
      // if both possible nodes are needed create an `MDTuple` out of them
      llvm::Metadata *md_arr[] = {md_nodes[0].first, md_nodes[1].first};

      branch_inst->setMetadata(
          "MDTuple",
          llvm::MDTuple::get(*context.llvmContext, llvm::ArrayRef(md_arr)));
    }
  }

  // This instruction ends a block, and thus a scope.
  closeCurrentLexicalScope();

  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSwitch>(const OpSwitch *op) {
  llvm::Value *selector = module.getValue(op->Selector());
  SPIRV_LL_ASSERT_PTR(selector);

  llvm::BasicBlock *destBB = getOrCreateBasicBlock(op->Default());
  SPIRV_LL_ASSERT_PTR(destBB);
  llvm::SwitchInst *switchInst = IRBuilder.CreateSwitch(selector, destBB);

  // Check how many words long our literals are. They are the same width as
  // `selector`, so potentially up to 64 bits, or two words long.
  const uint16_t literalWords =
      std::max(1u, selector->getType()->getScalarSizeInBits() / 32);

  for (auto target : op->Target(literalWords)) {
    llvm::BasicBlock *caseBB = getOrCreateBasicBlock(target.Label);
    SPIRV_LL_ASSERT_PTR(caseBB);
    llvm::Constant *caseVal =
        llvm::ConstantInt::get(selector->getType(), target.Literal);

    switchInst->addCase(llvm::cast<llvm::ConstantInt>(caseVal), caseBB);
  }

  // This instruction ends a block, and thus a scope.
  closeCurrentLexicalScope();

  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpKill>(const OpKill *) {
  // This instruction is only valid in the Fragment execuction model, which is
  // not supported.

  // This instruction ends a block, and thus a scope.
  closeCurrentLexicalScope();

  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpReturn>(const OpReturn *) {
  SPIRV_LL_ASSERT_PTR(getCurrentFunction());
  IRBuilder.CreateRetVoid();

  // This instruction ends a block, and thus a scope.
  closeCurrentLexicalScope();

  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpReturnValue>(const OpReturnValue *op) {
  SPIRV_LL_ASSERT_PTR(getCurrentFunction());

  llvm::Value *value = module.getValue(op->Value());
  SPIRV_LL_ASSERT_PTR(value);

  IRBuilder.CreateRet(value);

  // This instruction ends a block, and thus a scope.
  closeCurrentLexicalScope();

  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpUnreachable>(const OpUnreachable *) {
  IRBuilder.CreateUnreachable();

  // This instruction ends a block, and thus a scope.
  closeCurrentLexicalScope();

  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpLifetimeStart>(const OpLifetimeStart *op) {
  auto pointer = module.getValue(op->Pointer());
  SPIRV_LL_ASSERT_PTR(pointer);

#if LLVM_VERSION_GREATER_EQUAL(22, 0)
  IRBuilder.CreateLifetimeStart(pointer);
#else
  const uint32_t size = op->Size();

  // IRBuilder handles size == nullptr as size of variable.
  llvm::ConstantInt *sizeConstant = nullptr;
  if (size > 0) {
    sizeConstant = IRBuilder.getInt64(size);
  }
  IRBuilder.CreateLifetimeStart(pointer, sizeConstant);
#endif
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpLifetimeStop>(const OpLifetimeStop *op) {
  auto pointer = module.getValue(op->Pointer());
  SPIRV_LL_ASSERT_PTR(pointer);

#if LLVM_VERSION_GREATER_EQUAL(22, 0)
  IRBuilder.CreateLifetimeEnd(pointer);
#else
  const uint32_t size = op->Size();

  // IRBuilder handles size == nullptr as size of variable.
  llvm::ConstantInt *sizeConstant = nullptr;
  if (size > 0) {
    sizeConstant = IRBuilder.getInt64(size);
  }
  IRBuilder.CreateLifetimeEnd(pointer, sizeConstant);
#endif
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupAsyncCopy>(const OpGroupAsyncCopy *op) {
  llvm::Type *eventTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(eventTy);

  llvm::Value *dst = module.getValue(op->Destination());
  SPIRV_LL_ASSERT_PTR(dst);

  llvm::Value *src = module.getValue(op->Source());
  SPIRV_LL_ASSERT_PTR(src);

  llvm::Value *numElements = module.getValue(op->NumElements());
  SPIRV_LL_ASSERT_PTR(numElements);

  llvm::Value *stride = module.getValue(op->Stride());
  SPIRV_LL_ASSERT_PTR(stride);

  llvm::Value *event = module.getValue(op->Event());
  SPIRV_LL_ASSERT_PTR(event);

  module.addID(
      op->IdResult(), op,
      createMangledBuiltinCall(
          "async_work_group_strided_copy", eventTy, op->IdResultType(),
          {dst, src, numElements, stride, event},
          {op->Destination(), MangleInfo(op->Source(), MangleInfo::CONST),
           op->NumElements(), op->Stride(), op->Event()},
          /*convergent*/ true));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupWaitEvents>(const OpGroupWaitEvents *op) {
  llvm::Value *numEvents = module.getValue(op->NumEvents());
  SPIRV_LL_ASSERT_PTR(numEvents);

  llvm::Value *eventsList = module.getValue(op->EventsList());
  SPIRV_LL_ASSERT_PTR(eventsList);

  SPIRV_LL_ASSERT(eventsList->getType()->isPointerTy(),
                  "Events List must be pointer to OpTypeEvent");
  const unsigned AS = eventsList->getType()->getPointerAddressSpace();
  SPIRV_LL_ASSERT(AS == 0 || AS == 4, "Only expecting address space 0 or 4");

  createBuiltinCall(AS == 0 ? "_Z17wait_group_eventsiP9ocl_event"
                            : "_Z17wait_group_eventsiPU3AS49ocl_event",
                    IRBuilder.getVoidTy(), {numEvents, eventsList},
                    /*convergent*/ true);
  return llvm::Error::success();
}

template <typename T>
void Builder::generateReduction(const T *op, const std::string &opName,
                                MangleInfo::ForceSignInfo signInfo) {
  auto retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  auto *const execution = module.getValue(op->Execution());
  SPIRV_LL_ASSERT_PTR(execution);

  const auto operation = op->Operation();
  std::string operationName;
  switch (operation) {
    default:
      SPIRV_LL_ABORT("unhandled scope");
      break;
    case spv::GroupOperationReduce:
      operationName = "reduce";
      break;
    case spv::GroupOperationExclusiveScan:
      operationName = "scan_exclusive";
      break;
    case spv::GroupOperationInclusiveScan:
      operationName = "scan_inclusive";
      break;
  }

  auto *x = module.getValue(op->X());
  SPIRV_LL_ASSERT_PTR(x);

  // Look up the wrapper function for the scan or reduction.
  // We have to call a llvm::Function that has similar semantics as the
  // OpGroup instruction because the execution scope is an argument but the CL
  // C builtins encode the scope in their symbol names so we need to branch
  // between them. We can't branch between the builtins in the original function
  // because this can generate invalid IR when the incoming edges to a phi node.
  //
  // In this case because the operation type e.g. reduce, scan, inclusive scan
  // etc. is constant we don't pass this as an arugment to the wrapper. Hence we
  // look up the wrapper function based on the operation type and the type
  // operated on.
  // Add in any required mangle information before we cache the reduction
  // wrapper. This is important for distinguishing between smin/smax, for
  // example.
  const char *prefix = "";
  if (signInfo == MangleInfo::ForceSignInfo::ForceSigned) prefix = "s";
  if (signInfo == MangleInfo::ForceSignInfo::ForceUnsigned) prefix = "u";
  const std::string cacheName = prefix + opName;
  auto *&reductionWrapper =
      module.reductionWrapperMap[operation][cacheName]
                                [module.getResultType(op->X())];

  // If it doesn't exist we need to create it.
  if (!reductionWrapper) {
    auto *const wrapperFcnTy = llvm::FunctionType::get(
        retTy, {execution->getType(), x->getType()}, /* isVarARg */ false);

    // LLVM will automatically append a suffix if this symbol already exists,
    // this is good since we need different overloads for the cartesian product
    // of {operation_type, exeuction_scope}.
    reductionWrapper = llvm::Function::Create(
        wrapperFcnTy, llvm::Function::LinkageTypes::InternalLinkage,
        "reduction_wrapper", module.llvmModule.get());

    // Cache the current insert point of the IR builder.
    auto insertPoint = IRBuilder.GetInsertPoint();
    auto *insertBB = IRBuilder.GetInsertBlock();

    // Now create a body that is equivalent to:
    // reductionWrapper(scope, x) {
    //   if (scope == work_group) {
    //    return work_group_operation(x)
    //   } else {
    //    return sub_group_operation(x)
    //   }
    // }
    auto *const entry = llvm::BasicBlock::Create(*context.llvmContext, "entry",
                                                 reductionWrapper);
    auto *const exit = llvm::BasicBlock::Create(*context.llvmContext, "exit",
                                                reductionWrapper);
    auto *const workGroup = llvm::BasicBlock::Create(
        *context.llvmContext, "workgroup", reductionWrapper);
    auto *const subGroup = llvm::BasicBlock::Create(
        *context.llvmContext, "subgroup", reductionWrapper);

    auto *executionArg = reductionWrapper->getArg(0);
    llvm::Value *xArg = reductionWrapper->getArg(1);

    IRBuilder.SetInsertPoint(entry);
    auto *const scopeCmp = IRBuilder.CreateICmpEQ(
        executionArg, IRBuilder.getInt32(SUB_GROUP), "scope.cmp");
    IRBuilder.CreateCondBr(scopeCmp, workGroup, subGroup);

    IRBuilder.SetInsertPoint(workGroup);
    llvm::Value *workGroupResult = createMangledBuiltinCall(
        "work_group_" + operationName + "_" + opName, xArg->getType(),
        op->IdResultType(), xArg, MangleInfo(op->X(), signInfo),
        /* convergent */ true);
    IRBuilder.CreateBr(exit);

    IRBuilder.SetInsertPoint(subGroup);
    llvm::Value *subGroupResult = createMangledBuiltinCall(
        "sub_group_" + operationName + "_" + opName, xArg->getType(),
        op->IdResultType(), xArg, MangleInfo(op->X(), signInfo),
        /* convergent */ true);
    IRBuilder.CreateBr(exit);

    IRBuilder.SetInsertPoint(exit);
    auto *const resultPhi = IRBuilder.CreatePHI(xArg->getType(), 2);
    resultPhi->addIncoming(subGroupResult, subGroup);
    resultPhi->addIncoming(workGroupResult, workGroup);

    IRBuilder.CreateRet(resultPhi);

    // Always inline the function, this means for constant execution scope the
    // optimizer can remove the branches.
    reductionWrapper->addFnAttr(llvm::Attribute::AlwaysInline);
    reductionWrapper->addFnAttr(llvm::Attribute::Convergent);
    // Restore the original insert point.
    IRBuilder.SetInsertPoint(insertBB, insertPoint);
  }

  // Now we just call the wrapper.
  auto *const result = IRBuilder.CreateCall(reductionWrapper, {execution, x});
  module.addID(op->IdResult(), op, result);
}

template <typename T>
void Builder::generatePredicate(const T *op, const std::string &opName) {
  // Result Type must be a boolean type, which maps to an i1 in LLVM IR.
  auto *const retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);
  SPIRV_LL_ASSERT(retTy == IRBuilder.getInt1Ty(),
                  "return type is not a boolean");

  auto *const execution = module.getValue(op->Execution());
  SPIRV_LL_ASSERT_PTR(execution);

  auto *const predicate = module.getValue(op->Predicate());
  SPIRV_LL_ASSERT_PTR(predicate);
  SPIRV_LL_ASSERT(retTy == predicate->getType(), "predicate is not boolean");

  // Look up the wrapper function for the predicate operation.
  // We have to call a llvm::Function that has the same semantics as the
  // OpGroup instruction because the execution scope is an argument but the CL
  // C builtins encode the scope in their symbol names so we need to branch
  // between them. We can't branch between the builtins in the original
  // function because this can generate invalid IR when the incoming edges to a
  // phi node.
  //
  // We look up the wrapper based on the type being broadcast so each builtin
  // overload gets a different wrapper.
  auto *&predicateWrapper = module.predicateWrapperMap[opName];

  // If it doesn't exist, we need to create it.
  if (!predicateWrapper) {
    auto *const wrapperFcnTy = llvm::FunctionType::get(
        retTy, {execution->getType(), predicate->getType()},
        /* isVarArg */ false);

    predicateWrapper = llvm::Function::Create(
        wrapperFcnTy, llvm::Function::LinkageTypes::InternalLinkage,
        "group_" + opName + "_wrapper", module.llvmModule.get());

    // Cache the current insert point of the IR builder.
    auto insertPoint = IRBuilder.GetInsertPoint();
    auto *insertBB = IRBuilder.GetInsertBlock();

    // Now create a body that is equivalent to:
    // predicateWrapper(scope, value) {
    //   if (scope == work_group) {
    //    return work_group_predicate(value)
    //   } else {
    //    return sub_group_predicate(value)
    //   }
    // }
    auto *const entry = llvm::BasicBlock::Create(*context.llvmContext, "entry",
                                                 predicateWrapper);
    auto *const exit = llvm::BasicBlock::Create(*context.llvmContext, "exit",
                                                predicateWrapper);
    auto *const workGroup = llvm::BasicBlock::Create(
        *context.llvmContext, "workgroup", predicateWrapper);
    auto *const subGroup = llvm::BasicBlock::Create(
        *context.llvmContext, "subgroup", predicateWrapper);

    auto *executionArg = predicateWrapper->getArg(0);
    llvm::Value *predicateArg = predicateWrapper->getArg(1);

    IRBuilder.SetInsertPoint(entry);
    // Predicate must be a boolean type in the spir-v spec, but the OpenCL spec
    // has the following builtin: int sub_group_any (int predicate) so here we
    // need to cast the type predicate to an i32.
    // It's safe to assume the i1 is signed since it is just one bit.
    predicateArg = IRBuilder.CreateIntCast(predicateArg, IRBuilder.getInt32Ty(),
                                           /* isSigned */ true);
    auto *const scopeCmp = IRBuilder.CreateICmpEQ(
        executionArg, IRBuilder.getInt32(SUB_GROUP), "scope.cmp");
    IRBuilder.CreateCondBr(scopeCmp, workGroup, subGroup);

    IRBuilder.SetInsertPoint(workGroup);
    // The OpenCL version of these builtins take an int/i32 type, not a boolean
    // type. Custom mangle the builtin we're calling. Ideally our mangling APIs
    // would be able to handle this.
    llvm::Value *workGroupResult = createBuiltinCall(
        applyMangledLength("work_group_" + opName) + "i",
        IRBuilder.getInt32Ty(), predicateArg, /* convergent */ true);
    IRBuilder.CreateBr(exit);

    IRBuilder.SetInsertPoint(subGroup);
    llvm::Value *subGroupResult = createBuiltinCall(
        applyMangledLength("sub_group_" + opName) + "i", IRBuilder.getInt32Ty(),
        predicateArg, /* convergent */ true);
    IRBuilder.CreateBr(exit);

    IRBuilder.SetInsertPoint(exit);
    auto *const resultPhi = IRBuilder.CreatePHI(IRBuilder.getInt32Ty(), 2);
    resultPhi->addIncoming(subGroupResult, subGroup);
    resultPhi->addIncoming(workGroupResult, workGroup);
    // Now we need to cast back to i1.
    auto *const result = IRBuilder.CreateIntCast(resultPhi, retTy,
                                                 /* isSigned */ true);
    IRBuilder.CreateRet(result);

    // Always inline the function, this means for constant execution scope the
    // optimizer can remove the branches.
    predicateWrapper->addFnAttr(llvm::Attribute::AlwaysInline);
    // Restore the original insert point.
    IRBuilder.SetInsertPoint(insertBB, insertPoint);
  }

  // Now we just call the wrapper.
  auto *const result =
      IRBuilder.CreateCall(predicateWrapper, {execution, predicate});
  module.addID(op->IdResult(), op, result);
}

template <>
llvm::Error Builder::create<OpGroupAll>(const OpGroupAll *op) {
  generatePredicate(op, "all");
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupAny>(const OpGroupAny *op) {
  generatePredicate(op, "any");
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupBroadcast>(const OpGroupBroadcast *op) {
  // Result Type must be a scalar or vector or floating-point type, integer type
  // or boolean type.
  auto retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);
  SPIRV_LL_ASSERT(retTy->isIntegerTy() || retTy->isFloatingPointTy(),
                  "return type is not float, integer or boolean");

  auto execution = module.getValue(op->Execution());
  SPIRV_LL_ASSERT_PTR(execution);

  // The type of Value must be the same as Result Type.
  auto value = module.getValue(op->Value());
  SPIRV_LL_ASSERT_PTR(value);
  SPIRV_LL_ASSERT(value->getType() == retTy,
                  "The Result Type operand does not match the type of the Value"
                  "operand.");

  auto localId = module.getValue(op->LocalId());
  SPIRV_LL_ASSERT_PTR(localId);

  // LocalId must be an integer datatype. It must be a scalar, a vector with 2
  // components, or a vector with 3 components.
  auto *const localIdType = localId->getType();
  auto dimensions = 1;
  if (auto *const localIdVecType =
          llvm::dyn_cast<llvm::FixedVectorType>(localIdType)) {
    dimensions = localIdVecType->getNumElements();
    SPIRV_LL_ASSERT(1 < dimensions && dimensions <= 3,
                    "Invalid number of elements in local ID vector argument");
  }
  SPIRV_LL_ASSERT((1 == dimensions) ? localIdType->isIntegerTy()
                                    : cast<llvm::FixedVectorType>(localIdType)
                                          ->getElementType()
                                          ->isIntegerTy(),
                  "LocalId operand is not integer type or vector of integers");

  // Look up the wrapper function for the broadcast.
  // We have to call a llvm::Function that has the same semantics as the
  // OpGroup instruction because the execution scope is an argument but the CL
  // C builtins encode the scope in their symbol names so we need to branch
  // between them. We can't branch between the builtins in the original function
  // because this can generate invalid IR when the incoming edges to a phi node.
  //
  // We look up the wrapper based on the type being broadcast and whether the
  // broadcast is 1D, 2D or 3D (for sub-groups it should always be 1D) so each
  // builtin overload gets a different wrapper.
  auto *&broadcastWrapper =
      module.broadcastWrapperMap[module.getResultType(op->Value())][dimensions];

  // In theory localId could have any integer type, we don't really want to
  // overload this function on all possible integers, so cast to i32 here.
  auto *const i32Ty =
      llvm::IntegerType::getInt32Ty(*module.context.llvmContext);
  auto *const localIdArgType =
      (dimensions == 1)
          ? cast<llvm::Type>(i32Ty)
          : cast<llvm::Type>(llvm::FixedVectorType::get(i32Ty, dimensions));

  // If it doesn't exist we need to create it.
  if (!broadcastWrapper) {
    auto *const wrapperFcnTy = llvm::FunctionType::get(
        retTy, {execution->getType(), value->getType(), localIdArgType},
        /* isVarArg */ false);

    // LLVM will automatically append a suffix if this symbol already exists so
    // we will get a different function for each type overload.
    broadcastWrapper = llvm::Function::Create(
        wrapperFcnTy, llvm::Function::LinkageTypes::InternalLinkage,
        "broadcast_wrapper", module.llvmModule.get());
    broadcastWrapper->setConvergent();

    // Cache the current insert point of the IR builder.
    auto insertPoint = IRBuilder.GetInsertPoint();
    auto *insertBB = IRBuilder.GetInsertBlock();

    // Now create a body that is equivalent to:
    // broadcastWrapper(scope, value, localId) {
    //   if (scope == work_group) {
    //    return work_group_operation(value, localId)
    //   } else {
    //    return sub_group_operation(value, localId)
    //   }
    // }
    auto *const entry = llvm::BasicBlock::Create(*context.llvmContext, "entry",
                                                 broadcastWrapper);
    auto *const exit = llvm::BasicBlock::Create(*context.llvmContext, "exit",
                                                broadcastWrapper);
    auto *const workGroup = llvm::BasicBlock::Create(
        *context.llvmContext, "workgroup", broadcastWrapper);
    auto *const subGroup = llvm::BasicBlock::Create(
        *context.llvmContext, "subgroup", broadcastWrapper);

    auto *executionArg = broadcastWrapper->getArg(0);
    llvm::Value *valueArg = broadcastWrapper->getArg(1);
    llvm::Value *localIdArg = broadcastWrapper->getArg(2);

    IRBuilder.SetInsertPoint(entry);
    // For booleans we need to do some casting.
    auto *const boolTy = IRBuilder.getInt1Ty();
    const auto isBoolean = valueArg->getType() == boolTy;
    if (isBoolean) {
      // It's safe to assume the i1 is signed since it is just one bit.
      valueArg = IRBuilder.CreateIntCast(valueArg, IRBuilder.getInt32Ty(),
                                         /* isSigned */ true);
    }

    // It's possible that the local ID is a 2 or 3 element
    // vector. In which case we need to unpack the elements.
    // This doesn't really make sense for sub-groups, where the OpenCL builtin
    // only accepts a single local ID argument - but the spir-v spec doesn't
    // actually make the restriction that if the execution scope == sub-group
    // then the local ID has to be scalar, so we rely on the producer not to do
    // this.
    llvm::SmallVector<llvm::Value *, 2> args{valueArg};
    llvm::SmallVector<MangleInfo, 2> argIds{op->Value()};
    if (const auto *const vectorTy =
            dyn_cast<llvm::FixedVectorType>(localIdArg->getType())) {
      const auto elementCount = vectorTy->getNumElements();
      for (unsigned element = 0; element < elementCount; ++element) {
        args.push_back(IRBuilder.CreateExtractElement(localIdArg, element));
        argIds.push_back(op->LocalId());
      }
    } else {
      args.push_back(localIdArg);
      argIds.push_back(op->LocalId());
    }

    auto *const scopeCmp = IRBuilder.CreateICmpEQ(
        executionArg, IRBuilder.getInt32(SUB_GROUP), "scope.cmp");
    IRBuilder.CreateCondBr(scopeCmp, workGroup, subGroup);

    IRBuilder.SetInsertPoint(subGroup);
    // sub_group_broadcast takes uint as its local ID argument, so no need to
    // cast here.
    llvm::Value *subGroupResult;
    if (isBoolean) {
      // With booleans, we need to mangle the function as 'int' but we only
      // have mangle info for 'bool'. Do custom mangling to account for this.
      // Ideally our mangling APIs would be flexible enough to account for
      // this.
      subGroupResult =
          createBuiltinCall(applyMangledLength("sub_group_broadcast") + "ij",
                            valueArg->getType(), args,
                            /* convergent */ true);
    } else {
      subGroupResult = createMangledBuiltinCall(
          "sub_group_broadcast", valueArg->getType(), op->IdResultType(), args,
          argIds, /* convergent */ true);
    }
    IRBuilder.CreateBr(exit);

    IRBuilder.SetInsertPoint(workGroup);
    // work_group_broadcast takes size_t as its local ID arguments. This may
    // not match the type passed to OpGroupBroadcast so here we do a cast to
    // ensure the call is valid.
    llvm::Type *const sizeTy = llvm::IntegerType::get(
        IRBuilder.getContext(), module.getAddressingModel());
    // Start one past the beginning since the first value is the broadcast.
    for (unsigned i = 1; i < args.size(); ++i) {
      args[i] = IRBuilder.CreateIntCast(args[i], sizeTy, /* isSigned */ false);
    }
    llvm::Value *workGroupResult;
    if (isBoolean) {
      // With booleans, we need to mangle the function as 'int/size_t' but we
      // only have mangle info for the 'bool' parameter. Do custom mangling to
      // account for this. Ideally our mangling APIs would be flexible enough to
      // account for this.
      workGroupResult =
          createBuiltinCall(applyMangledLength("work_group_broadcast") + "i" +
                                getIntTypeName(sizeTy, /*isSigned*/ false),
                            valueArg->getType(), args,
                            /* convergent */ true);
    } else {
      workGroupResult = createMangledBuiltinCall(
          "work_group_broadcast", valueArg->getType(), op->IdResultType(), args,
          argIds, /* convergent */ true);
    }
    IRBuilder.CreateBr(exit);

    IRBuilder.SetInsertPoint(exit);
    auto *const resultPhi = IRBuilder.CreatePHI(valueArg->getType(), 2);
    resultPhi->addIncoming(subGroupResult, subGroup);
    resultPhi->addIncoming(workGroupResult, workGroup);
    llvm::Value *result = resultPhi;
    if (isBoolean) {
      // Now we need to cast back to i1.
      result = IRBuilder.CreateIntCast(result, boolTy,
                                       /* isSigned */ true);
    }
    IRBuilder.CreateRet(result);

    // Always inline the function, this means for constant execution scope the
    // optimizer can remove the branches.
    broadcastWrapper->addFnAttr(llvm::Attribute::AlwaysInline);
    // Restore the original insert point.
    IRBuilder.SetInsertPoint(insertBB, insertPoint);
  }

  // Now we just call the wrapper remembering to potentially cast to i32.
  localId =
      IRBuilder.CreateIntCast(localId, localIdArgType, /* isSigned */ false);
  auto *const result =
      IRBuilder.CreateCall(broadcastWrapper, {execution, value, localId});
  module.addID(op->IdResult(), op, result);

  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupIAdd>(const OpGroupIAdd *op) {
  generateReduction(op, "add");
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupFAdd>(const OpGroupFAdd *op) {
  generateReduction(op, "add");
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupFMin>(const OpGroupFMin *op) {
  generateReduction(op, "min");
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupUMin>(const OpGroupUMin *op) {
  generateReduction(op, "min", MangleInfo::ForceSignInfo::ForceUnsigned);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupSMin>(const OpGroupSMin *op) {
  generateReduction(op, "min", MangleInfo::ForceSignInfo::ForceSigned);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupFMax>(const OpGroupFMax *op) {
  generateReduction(op, "max");
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupUMax>(const OpGroupUMax *op) {
  generateReduction(op, "max", MangleInfo::ForceSignInfo::ForceUnsigned);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupSMax>(const OpGroupSMax *op) {
  generateReduction(op, "max", MangleInfo::ForceSignInfo::ForceSigned);
  return llvm::Error::success();
}

// SPV_KHR_uniform_group_instructions
template <>
llvm::Error Builder::create<OpGroupIMulKHR>(const OpGroupIMulKHR *op) {
  generateReduction(op, "mul");
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupFMulKHR>(const OpGroupFMulKHR *op) {
  generateReduction(op, "mul");
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupBitwiseAndKHR>(
    const OpGroupBitwiseAndKHR *op) {
  generateReduction(op, "and");
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupBitwiseOrKHR>(
    const OpGroupBitwiseOrKHR *op) {
  generateReduction(op, "or");
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupBitwiseXorKHR>(
    const OpGroupBitwiseXorKHR *op) {
  generateReduction(op, "xor");
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupLogicalAndKHR>(
    const OpGroupLogicalAndKHR *op) {
  generateReduction(op, "logical_and");
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupLogicalOrKHR>(
    const OpGroupLogicalOrKHR *op) {
  generateReduction(op, "logical_or");
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupLogicalXorKHR>(
    const OpGroupLogicalXorKHR *op) {
  generateReduction(op, "logical_xor");
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSubgroupShuffle>(const OpSubgroupShuffle *op) {
  std::string muxBuiltinName = "__mux_sub_group_shuffle_";

  auto *data = module.getValue(op->Data());
  SPIRV_LL_ASSERT_PTR(data);

  auto *invocation_id = module.getValue(op->InvocationId());
  SPIRV_LL_ASSERT_PTR(invocation_id);

  auto retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  muxBuiltinName += compiler::utils::BuiltinInfo::getMangledTypeStr(retTy);

  auto *const ci = createBuiltinCall(
      muxBuiltinName, retTy, {data, invocation_id}, /*convergent*/ true);
  module.addID(op->IdResult(), op, ci);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSubgroupShuffleUp>(
    const OpSubgroupShuffleUp *op) {
  std::string muxBuiltinName = "__mux_sub_group_shuffle_up_";

  auto *previous = module.getValue(op->Previous());
  SPIRV_LL_ASSERT_PTR(previous);

  auto *current = module.getValue(op->Current());
  SPIRV_LL_ASSERT_PTR(current);

  auto *delta = module.getValue(op->Delta());
  SPIRV_LL_ASSERT_PTR(delta);

  auto retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  muxBuiltinName += compiler::utils::BuiltinInfo::getMangledTypeStr(retTy);

  auto *const ci = createBuiltinCall(
      muxBuiltinName, retTy, {previous, current, delta}, /*convergent*/ true);
  module.addID(op->IdResult(), op, ci);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSubgroupShuffleDown>(
    const OpSubgroupShuffleDown *op) {
  std::string muxBuiltinName = "__mux_sub_group_shuffle_down_";

  auto *current = module.getValue(op->Current());
  SPIRV_LL_ASSERT_PTR(current);

  auto *next = module.getValue(op->Next());
  SPIRV_LL_ASSERT_PTR(next);

  auto *delta = module.getValue(op->Delta());
  SPIRV_LL_ASSERT_PTR(delta);

  auto retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  muxBuiltinName += compiler::utils::BuiltinInfo::getMangledTypeStr(retTy);

  auto *const ci = createBuiltinCall(
      muxBuiltinName, retTy, {current, next, delta}, /*convergent*/ true);
  module.addID(op->IdResult(), op, ci);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpSubgroupShuffleXor>(
    const OpSubgroupShuffleXor *op) {
  std::string muxBuiltinName = "__mux_sub_group_shuffle_xor_";

  auto *data = module.getValue(op->Data());
  SPIRV_LL_ASSERT_PTR(data);

  auto *value = module.getValue(op->Value());
  SPIRV_LL_ASSERT_PTR(value);

  auto retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  muxBuiltinName += compiler::utils::BuiltinInfo::getMangledTypeStr(retTy);

  auto *const ci = createBuiltinCall(muxBuiltinName, retTy, {data, value},
                                     /*convergent*/ true);
  module.addID(op->IdResult(), op, ci);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpReadPipe>(const OpReadPipe *) {
  // Capability Pipes isn't supported by CL 1.2, see OpenCL SPIR-V
  // environment spec section 6.1 for supported capabilities.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpWritePipe>(const OpWritePipe *) {
  // Capability Pipes isn't supported by CL 1.2, see OpenCL SPIR-V
  // environment spec section 6.1 for supported capabilities.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpReservedReadPipe>(const OpReservedReadPipe *) {
  // Capability Pipes isn't supported by CL 1.2, see OpenCL SPIR-V
  // environment spec section 6.1 for supported capabilities.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpReservedWritePipe>(const OpReservedWritePipe *) {
  // Capability Pipes isn't supported by CL 1.2, see OpenCL SPIR-V
  // environment spec section 6.1 for supported capabilities.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpReserveReadPipePackets>(
    const OpReserveReadPipePackets *) {
  // Capability Pipes isn't supported by CL 1.2, see OpenCL SPIR-V
  // environment spec section 6.1 for supported capabilities.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpReserveWritePipePackets>(
    const OpReserveWritePipePackets *) {
  // Capability Pipes isn't supported by CL 1.2, see OpenCL SPIR-V
  // environment spec section 6.1 for supported capabilities.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpCommitReadPipe>(const OpCommitReadPipe *) {
  // Capability Pipes isn't supported by CL 1.2, see OpenCL SPIR-V
  // environment spec section 6.1 for supported capabilities.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpCommitWritePipe>(const OpCommitWritePipe *) {
  // Capability Pipes isn't supported by CL 1.2, see OpenCL SPIR-V
  // environment spec section 6.1 for supported capabilities.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpIsValidReserveId>(const OpIsValidReserveId *) {
  // Capability Pipes isn't supported by CL 1.2, see OpenCL SPIR-V
  // environment spec section 6.1 for supported capabilities.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGetNumPipePackets>(const OpGetNumPipePackets *) {
  // Capability Pipes isn't supported by CL 1.2, see OpenCL SPIR-V
  // environment spec section 6.1 for supported capabilities.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGetMaxPipePackets>(const OpGetMaxPipePackets *) {
  // Capability Pipes isn't supported by CL 1.2, see OpenCL SPIR-V
  // environment spec section 6.1 for supported capabilities.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupReserveReadPipePackets>(
    const OpGroupReserveReadPipePackets *) {
  // Capability Pipes isn't supported by CL 1.2, see OpenCL SPIR-V
  // environment spec section 6.1 for supported capabilities.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupReserveWritePipePackets>(
    const OpGroupReserveWritePipePackets *) {
  // Capability Pipes isn't supported by CL 1.2, see OpenCL SPIR-V
  // environment spec section 6.1 for supported capabilities.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupCommitReadPipe>(
    const OpGroupCommitReadPipe *) {
  // Capability Pipes isn't supported by CL 1.2, see OpenCL SPIR-V
  // environment spec section 6.1 for supported capabilities.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpGroupCommitWritePipe>(
    const OpGroupCommitWritePipe *) {
  // Capability Pipes isn't supported by CL 1.2, see OpenCL SPIR-V
  // environment spec section 6.1 for supported capabilities.
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpEnqueueMarker>(const OpEnqueueMarker *) {
  return errorUnsupportedDeviceEnqueueOp("OpEnqueueMarker");
}

template <>
llvm::Error Builder::create<OpEnqueueKernel>(const OpEnqueueKernel *) {
  return errorUnsupportedDeviceEnqueueOp("OpEnqueueKernel");
}

template <>
llvm::Error Builder::create<OpGetKernelNDrangeSubGroupCount>(
    const OpGetKernelNDrangeSubGroupCount *) {
  return errorUnsupportedDeviceEnqueueOp("OpGetKernelNDrangeSubGroupCount");
}

template <>
llvm::Error Builder::create<OpGetKernelNDrangeMaxSubGroupSize>(
    const OpGetKernelNDrangeMaxSubGroupSize *) {
  return errorUnsupportedDeviceEnqueueOp("OpGetKernelNDrangeMaxSubGroupSize");
}

template <>
llvm::Error Builder::create<OpGetKernelWorkGroupSize>(
    const OpGetKernelWorkGroupSize *) {
  return errorUnsupportedDeviceEnqueueOp("OpGetKernelWorkGroupSize");
}

template <>
llvm::Error Builder::create<OpGetKernelPreferredWorkGroupSizeMultiple>(
    const OpGetKernelPreferredWorkGroupSizeMultiple *) {
  return errorUnsupportedDeviceEnqueueOp(
      "OpGetKernelPreferredWorkGroupSizeMultiple");
}

template <>
llvm::Error Builder::create<OpRetainEvent>(const OpRetainEvent *) {
  return errorUnsupportedDeviceEnqueueOp("OpRetainEvent");
}

template <>
llvm::Error Builder::create<OpReleaseEvent>(const OpReleaseEvent *) {
  return errorUnsupportedDeviceEnqueueOp("OpReleaseEvent");
}

template <>
llvm::Error Builder::create<OpCreateUserEvent>(const OpCreateUserEvent *) {
  return errorUnsupportedDeviceEnqueueOp("OpCreateUserEvent");
}

template <>
llvm::Error Builder::create<OpIsValidEvent>(const OpIsValidEvent *) {
  return errorUnsupportedDeviceEnqueueOp("OpIsValidEvent");
}

template <>
llvm::Error Builder::create<OpSetUserEventStatus>(
    const OpSetUserEventStatus *) {
  return errorUnsupportedDeviceEnqueueOp("OpSetUserEventStatus");
}

template <>
llvm::Error Builder::create<OpCaptureEventProfilingInfo>(
    const OpCaptureEventProfilingInfo *) {
  return errorUnsupportedDeviceEnqueueOp("OpCaptureEventProfilingInfo");
}

template <>
llvm::Error Builder::create<OpGetDefaultQueue>(const OpGetDefaultQueue *) {
  return errorUnsupportedDeviceEnqueueOp("OpGetDefaultQueue");
}

template <>
llvm::Error Builder::create<OpBuildNDRange>(const OpBuildNDRange *) {
  return errorUnsupportedDeviceEnqueueOp("OpBuildNDRange");
}

template <>
llvm::Error Builder::create<OpGetKernelLocalSizeForSubgroupCount>(
    const OpGetKernelLocalSizeForSubgroupCount *) {
  return errorUnsupportedDeviceEnqueueOp(
      "OpGetKernelLocalSizeForSubgroupCount");
}

template <>
llvm::Error Builder::create<OpGetKernelMaxNumSubgroups>(
    const OpGetKernelMaxNumSubgroups *) {
  return errorUnsupportedDeviceEnqueueOp("OpGetKernelMaxNumSubgroups");
}

template <>
llvm::Error Builder::create<OpImageSparseSampleImplicitLod>(
    const OpImageSparseSampleImplicitLod *) {
  // This instruction requires the SparseResidency capability which is not
  // supported by OpenCL 1.2 (see OpenCL SPIR-V environment spec section 6.1)
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSparseSampleExplicitLod>(
    const OpImageSparseSampleExplicitLod *) {
  // This instruction requires the SparseResidency capability which is not
  // supported by OpenCL 1.2 (see OpenCL SPIR-V environment spec section 6.1)
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSparseSampleDrefImplicitLod>(
    const OpImageSparseSampleDrefImplicitLod *) {
  // This instruction requires the SparseResidency capability which is not
  // supported by OpenCL 1.2 (see OpenCL SPIR-V environment spec section 6.1)
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSparseSampleDrefExplicitLod>(
    const OpImageSparseSampleDrefExplicitLod *) {
  // This instruction requires the SparseResidency capability which is not
  // supported by OpenCL 1.2 (see OpenCL SPIR-V environment spec section 6.1)
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSparseSampleProjImplicitLod>(
    const OpImageSparseSampleProjImplicitLod *) {
  // This instruction requires the SparseResidency capability which is not
  // supported by OpenCL 1.2 (see OpenCL SPIR-V environment spec section 6.1)
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSparseSampleProjExplicitLod>(
    const OpImageSparseSampleProjExplicitLod *) {
  // This instruction requires the SparseResidency capability which is not
  // supported by OpenCL 1.2 (see OpenCL SPIR-V environment spec section 6.1)
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSparseSampleProjDrefImplicitLod>(
    const OpImageSparseSampleProjDrefImplicitLod *) {
  // This instruction requires the SparseResidency capability which is not
  // supported by OpenCL 1.2 (see OpenCL SPIR-V environment spec section 6.1)
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSparseSampleProjDrefExplicitLod>(
    const OpImageSparseSampleProjDrefExplicitLod *) {
  // This instruction requires the SparseResidency capability which is not
  // supported by OpenCL 1.2 (see OpenCL SPIR-V environment spec section 6.1)
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSparseFetch>(const OpImageSparseFetch *) {
  // This instruction requires the SparseResidency capability which is not
  // supported by OpenCL 1.2 (see OpenCL SPIR-V environment spec section 6.1)
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSparseGather>(const OpImageSparseGather *) {
  // This instruction requires the SparseResidency capability which is not
  // supported by OpenCL 1.2 (see OpenCL SPIR-V environment spec section 6.1)
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSparseDrefGather>(
    const OpImageSparseDrefGather *) {
  // This instruction requires the SparseResidency capability which is not
  // supported by OpenCL 1.2 (see OpenCL SPIR-V environment spec section 6.1)
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSparseTexelsResident>(
    const OpImageSparseTexelsResident *) {
  // This instruction requires the SparseResidency capability which is not
  // supported by OpenCL 1.2 (see OpenCL SPIR-V environment spec section 6.1)
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpNoLine>(const OpNoLine *) {
  applyDebugInfoAtClosedRangeOrScope();
  setCurrentOpLineRange(std::nullopt);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicFlagTestAndSet>(
    const OpAtomicFlagTestAndSet *op) {
  auto pointer = module.getValue(op->Pointer());
  SPIRV_LL_ASSERT_PTR(pointer);

  auto scope = module.getValue(op->Scope());
  SPIRV_LL_ASSERT_PTR(scope);

  auto semantics = module.getValue(op->Semantics());
  SPIRV_LL_ASSERT_PTR(semantics);

  auto retTy = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(retTy);

  module.addID(
      op->IdResult(), op,
      createMangledBuiltinCall("atomic_flag_test_and_set_explicit", retTy,
                               op->IdResultType(), {pointer, semantics, scope},
                               {MangleInfo(op->Pointer(), MangleInfo::VOLATILE),
                                op->Semantics(), op->Scope()}));
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAtomicFlagClear>(const OpAtomicFlagClear *op) {
  auto pointer = module.getValue(op->Pointer());
  SPIRV_LL_ASSERT_PTR(pointer);

  auto scope = module.getValue(op->Scope());
  SPIRV_LL_ASSERT_PTR(scope);

  auto semantics = module.getValue(op->Semantics());
  SPIRV_LL_ASSERT_PTR(semantics);

  createMangledBuiltinCall("atomic_flag_clear_explicit", IRBuilder.getVoidTy(),
                           spv::OpTypeVoid, {pointer, semantics, scope},
                           {MangleInfo(op->Pointer(), MangleInfo::VOLATILE),
                            op->Semantics(), op->Scope()});
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpImageSparseRead>(const OpImageSparseRead *) {
  // This instruction requires the SparseResidency capability which is not
  // supported by OpenCL 1.2 (see OpenCL SPIR-V environment spec section 6.1)
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpAssumeTrueKHR>(const OpAssumeTrueKHR *op) {
  auto condition = module.getValue(op->Condition());
  SPIRV_LL_ASSERT_PTR(condition);

  IRBuilder.CreateAssumption(condition);
  return llvm::Error::success();
}

template <>
llvm::Error Builder::create<OpExpectKHR>(const OpExpectKHR *op) {
  llvm::Type *type = module.getLLVMType(op->IdResultType());
  SPIRV_LL_ASSERT_PTR(type);

  auto value = module.getValue(op->Value());
  SPIRV_LL_ASSERT_PTR(value);
  auto expectedValue = module.getValue(op->ExpectedValue());
  SPIRV_LL_ASSERT_PTR(expectedValue);

  SPIRV_LL_ASSERT(type == value->getType(),
                  "The type of the Value operand must match the Result Type.");

  SPIRV_LL_ASSERT(value->getType() == expectedValue->getType(),
                  "The type of the ExpectedValue operand must match the type "
                  "of the Value operand.");

  module.addID(op->IdResult(), op,
               IRBuilder.CreateIntrinsic(llvm::Intrinsic::expect, type,
                                         {value, expectedValue}));
  return llvm::Error::success();
}

}  // namespace spirv_ll
