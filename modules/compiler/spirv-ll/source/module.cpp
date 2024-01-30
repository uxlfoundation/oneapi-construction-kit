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

#include <cargo/endian.h>
#include <llvm/Support/Error.h>
#include <multi_llvm/llvm_version.h>
#include <spirv-ll/assert.h>
#include <spirv-ll/builder.h>
#include <spirv-ll/context.h>
#include <spirv-ll/module.h>
#include <spirv/unified1/spirv.hpp>

#include <algorithm>
#include <unordered_set>

spirv_ll::ModuleHeader::ModuleHeader(llvm::ArrayRef<uint32_t> code)
    : code(code), endianSwap(code[0] == cargo::byte_swap(MAGIC)) {}

uint32_t spirv_ll::ModuleHeader::magic() const {
  return endianSwap ? cargo::byte_swap(code[0]) : code[0];
}

uint32_t spirv_ll::ModuleHeader::version() const {
  return endianSwap ? cargo::byte_swap(code[1]) : code[1];
}

uint32_t spirv_ll::ModuleHeader::generator() const {
  return endianSwap ? cargo::byte_swap(code[2]) : code[2];
}

uint32_t spirv_ll::ModuleHeader::bound() const {
  return endianSwap ? cargo::byte_swap(code[3]) : code[3];
}

uint32_t spirv_ll::ModuleHeader::schema() const {
  return endianSwap ? cargo::byte_swap(code[4]) : code[4];
}

spirv_ll::Module::Module(
    spirv_ll::Context &context, llvm::ArrayRef<uint32_t> code,
    cargo::optional<const spirv_ll::SpecializationInfo &> specInfo)
    : spirv_ll::ModuleHeader{code},
      context(context),
      llvmModule(new llvm::Module{"SPIR-V", *context.llvmContext}),
      fenceWrapperFcn(nullptr),
      barrierWrapperFcn(nullptr),
      capabilities(),
      ExtendedInstrSetBindings(),
      addressingModel(0),
      EntryPoints(),
      ExecutionModes(),
      sourceLanguage{},
      sourceMetadataString(),
      CompileUnit(nullptr),
      File(nullptr),
      LoopControl(),
      specInfo(specInfo),
      PushConstantStructVariable(nullptr),
      PushConstantStructID{},
      WorkgroupSize({{1, 1, 1}}),
      BufferSizeArray(nullptr),
      deferredSpecConstantOps(),
      ImplicitDebugScopes(true) {}

spirv_ll::Module::Module(spirv_ll::Context &context,
                         llvm::ArrayRef<uint32_t> code)
    : spirv_ll::ModuleHeader{code},
      context(context),
      llvmModule(nullptr),
      fenceWrapperFcn(nullptr),
      barrierWrapperFcn(nullptr),
      capabilities(),
      ExtendedInstrSetBindings(),
      addressingModel(0),
      EntryPoints(),
      ExecutionModes(),
      sourceLanguage{},
      sourceMetadataString(),
      CompileUnit(nullptr),
      File(nullptr),
      LoopControl(),
      specInfo(),
      PushConstantStructVariable(nullptr),
      PushConstantStructID{},
      WorkgroupSize({{1, 1, 1}}),
      BufferSizeArray(nullptr),
      deferredSpecConstantOps(),
      ImplicitDebugScopes(true) {}

void spirv_ll::Module::associateExtendedInstrSet(spv::Id id,
                                                 ExtendedInstrSet iset) {
  ExtendedInstrSetBindings.insert({id, iset});
}

spirv_ll::ExtendedInstrSet spirv_ll::Module::getExtendedInstrSet(
    spv::Id id) const {
  auto found = ExtendedInstrSetBindings.find(id);
  SPIRV_LL_ASSERT(found != ExtendedInstrSetBindings.end(),
                  "Bad extended instruction set lookup!");
  return found->getSecond();
}

void spirv_ll::Module::setAddressingModel(const uint32_t addrModel) {
  addressingModel = addrModel;
}

uint32_t spirv_ll::Module::getAddressingModel() const {
  return addressingModel;
}

void spirv_ll::Module::addEntryPoint(const OpEntryPoint *op) {
  if (!EntryPoints.count(op->EntryPoint())) {
    EntryPoints.insert({op->EntryPoint(), op});
  }
}

const spirv_ll::OpEntryPoint *spirv_ll::Module::getEntryPoint(
    spv::Id id) const {
  auto found = EntryPoints.find(id);
  return found != EntryPoints.end() ? found->getSecond() : nullptr;
}

void spirv_ll::Module::replaceID(const OpResult *Op, llvm::Value *V) {
  auto found = Values.find(Op->IdResult());
  if (found != Values.end()) {
    Values.erase(found);
  }
  addID(Op->IdResult(), Op, V);
}

void spirv_ll::Module::addExecutionMode(const OpExecutionMode *executionMode) {
  ExecutionModes[executionMode->EntryPoint()].push_back(executionMode);
}

llvm::ArrayRef<const spirv_ll::OpExecutionMode *>
spirv_ll::Module::getExecutionModes(spv::Id entryPoint) const {
  auto found = ExecutionModes.find(entryPoint);
  if (ExecutionModes.end() != found) {
    return found->getSecond();
  }
  return {};
}

const spirv_ll::OpExecutionMode *spirv_ll::Module::getExecutionMode(
    spv::Id entryPoint, spv::ExecutionMode mode) const {
  auto executionModes = getExecutionModes(entryPoint);
  auto found = std::find_if(
      executionModes.begin(), executionModes.end(),
      [mode](const OpExecutionMode *op) { return op->Mode() == mode; });
  if (executionModes.end() != found) {
    return *found;
  }
  return nullptr;
}

#if LLVM_VERSION_LESS(17, 0)
void spirv_ll::Module::addInternalStructType(spv::Id ty,
                                             llvm::StructType *structTy) {
  InternalStructureTypes[ty] = structTy;
}

llvm::StructType *spirv_ll::Module::getInternalStructType(spv::Id ty) const {
  auto found = InternalStructureTypes.find(ty);
  if (InternalStructureTypes.end() != found) {
    return found->getSecond();
  }
  return nullptr;
}
#endif

void spirv_ll::Module::setSourceLanguage(spv::SourceLanguage sourceLang) {
  sourceLanguage = sourceLang;
}

spv::SourceLanguage spirv_ll::Module::getSourceLanguage() const {
  return sourceLanguage;
}

void spirv_ll::Module::setSourceMetadataString(const std::string &str) {
  sourceMetadataString = str;
}

void spirv_ll::Module::appendSourceMetadataString(const std::string &str) {
  sourceMetadataString.append(str);
}

const std::string &spirv_ll::Module::getSourceMetadataString() const {
  return sourceMetadataString;
}

void spirv_ll::Module::setCompileUnit(llvm::DICompileUnit *compile_unit) {
  CompileUnit = compile_unit;
}

llvm::DICompileUnit *spirv_ll::Module::getCompileUnit() const {
  return CompileUnit;
}

void spirv_ll::Module::setDIFile(llvm::DIFile *file) { File = file; };

llvm::DIFile *spirv_ll::Module::getDIFile() const { return File; }

bool spirv_ll::Module::addDebugString(spv::Id id, const std::string &string) {
  return DebugStrings.try_emplace(id, string).second;
}

std::optional<std::string> spirv_ll::Module::getDebugString(spv::Id id) const {
  if (auto iter = DebugStrings.find(id); iter != DebugStrings.end()) {
    return iter->getSecond();
  }
  return std::nullopt;
}

void spirv_ll::Module::addLexicalBlock(llvm::BasicBlock *b_block,
                                       llvm::DILexicalBlock *lex_block) {
  LexicalBlocks.insert({b_block, lex_block});
}

llvm::DILexicalBlock *spirv_ll::Module::getLexicalBlock(
    llvm::BasicBlock *block) const {
  auto found = LexicalBlocks.find(block);
  return found != LexicalBlocks.end() ? found->getSecond() : nullptr;
}

bool spirv_ll::Module::useImplicitDebugScopes() const {
  return ImplicitDebugScopes;
}

void spirv_ll::Module::disableImplicitDebugScopes() {
  ImplicitDebugScopes = false;
}

void spirv_ll::Module::addDebugFunctionScope(
    spv::Id function_id, llvm::DISubprogram *function_scope) {
  FunctionScopes.insert({function_id, function_scope});
}

llvm::DISubprogram *spirv_ll::Module::getDebugFunctionScope(
    spv::Id function_id) const {
  auto found = FunctionScopes.find(function_id);
  return found != FunctionScopes.end() ? found->getSecond() : nullptr;
}

void spirv_ll::Module::setLoopControl(spv::Id latch, llvm::MDNode *md_node) {
  LoopControl.insert({latch, md_node});
}

void spirv_ll::Module::resolveLoopControl() {
  for (auto entry : LoopControl) {
    auto *latch = llvm::cast<llvm::BasicBlock>(getValue(entry.first));

    llvm::SmallVector<llvm::Metadata *, 4> Args;

    // Reserve operand 0 for loop id self reference.
    auto TempNode = llvm::MDNode::getTemporary(*context.llvmContext, {});
    Args.push_back(TempNode.get());
    Args.push_back(entry.second);

    // Set the first operand to itself.
    llvm::MDNode *LoopID = llvm::MDNode::get(*context.llvmContext, Args);
    LoopID->replaceOperandWith(0, LoopID);
    latch->getTerminator()->setMetadata(
        context.llvmContext->getMDKindID("llvm.loop"), LoopID);
  }
}

bool spirv_ll::Module::addName(spv::Id id, const std::string &name) {
  return Names.try_emplace(id, name).second;
}

std::string spirv_ll::Module::getName(spv::Id id) const {
  return Names.lookup(id);
}

std::string spirv_ll::Module::getName(llvm::Value *Value) const {
  auto it = std::find_if(Values.begin(), Values.end(), [Value](const auto &v) {
    return v.second.Value == Value;
  });

  if (it == Values.end()) {
    return std::string();
  }

  return Names.lookup(it->first);
}

void spirv_ll::Module::addDecoration(spv::Id id,
                                     const OpDecorateBase *decoration) {
  auto found = DecorationMap.find(id);
  if (found == DecorationMap.end()) {
    DecorationMap.insert({id, {decoration}});
  } else {
    auto &decorations = found->getSecond();
    if (std::find(decorations.begin(), decorations.end(), decoration) ==
        decorations.end()) {
      decorations.emplace_back(decoration);
    }
  }
}

llvm::ArrayRef<const spirv_ll::OpDecorateBase *>
spirv_ll::Module::getDecorations(spv::Id id) const {
  auto found = DecorationMap.find(id);
  if (DecorationMap.end() != found) {
    return found->getSecond();
  }
  return {};
}

llvm::SmallVector<const spirv_ll::OpDecorateBase *, 2>
spirv_ll::Module::getDecorations(spv::Id id, spv::Decoration decoration) const {
  llvm::SmallVector<const OpDecorateBase *, 2> matching;
  auto decorations = getDecorations(id);
  if (!decorations.empty()) {
    matching.reserve(decorations.size());
    for (auto op : decorations) {
      if (decoration == op->getDecoration()) {
        matching.push_back(op);
      }
    }
  }
  return matching;
}

const spirv_ll::OpDecorateBase *spirv_ll::Module::getFirstDecoration(
    spv::Id id, spv::Decoration decoration) const {
  for (auto op : getDecorations(id)) {
    if (decoration == op->getDecoration()) {
      return op;
    }
  }
  return nullptr;
}

void spirv_ll::Module::addMemberDecoration(spv::Id structType, uint32_t member,
                                           const OpDecorateBase *op) {
  auto iter = MemberDecorations.find(structType);
  if (iter == MemberDecorations.end()) {
    const llvm::SmallVector<const OpDecorateBase *, 2> decorations({op});
    DecoratedStruct newDecoratedStruct;
    newDecoratedStruct.try_emplace(member, decorations);
    MemberDecorations.try_emplace(structType, newDecoratedStruct);
  } else {
    auto member_iter = iter->second.find(member);
    if (member_iter == iter->second.end()) {
      const llvm::SmallVector<const OpDecorateBase *, 2> decorations({op});
      iter->second.try_emplace(member, decorations);
    } else {
      member_iter->second.push_back(op);
    }
  }
}

llvm::SmallVector<const spirv_ll::OpDecorateBase *, 2>
spirv_ll::Module::getMemberDecorations(spv::Id structType, uint32_t member) {
  auto iter = MemberDecorations.find(structType);
  if (iter != MemberDecorations.end()) {
    auto memberIter = iter->second.find(member);
    if (memberIter != iter->second.end()) {
      return memberIter->second;
    }
  }
  return {};
}

void spirv_ll::Module::resolveDecorations(spv::Id id) {
  auto decorations = getDecorations(id);
  for (auto &decorateOp : decorations) {
    switch (decorateOp->getDecoration()) {
      default:
        break;
      case spv::Decoration::DecorationSpecId:  // Shader
        addSpecId(id, decorateOp->getValueAtOffset(3));
        break;
      case spv::Decoration::DecorationBinding:  // Shader
        addBinding(id, decorateOp->getValueAtOffset(3));
        break;
      case spv::Decoration::DecorationDescriptorSet:  // Shader
        addSet(id, decorateOp->getValueAtOffset(3));
        break;
    }
  }
}

void spirv_ll::Module::addSet(spv::Id id, uint32_t set) {
  // There are no spec rules on set or binding coming first, so account for both
  // eventualities.
  auto found = InterfaceBlocks.find(id);
  if (found == InterfaceBlocks.end()) {
    InterfaceBlock block;
    block.binding.set = set;
    InterfaceBlocks.insert({id, block});
  } else {
    found->second.binding.set = set;
  }
}

void spirv_ll::Module::addBinding(spv::Id id, uint32_t binding) {
  auto found = InterfaceBlocks.find(id);
  if (found == InterfaceBlocks.end()) {
    InterfaceBlock block;
    block.binding.binding = binding;
    InterfaceBlocks.insert({id, block});
  } else {
    found->second.binding.binding = binding;
  }
}

llvm::SmallVector<spv::Id, 4> spirv_ll::Module::getDescriptorBindingList()
    const {
  llvm::SmallVector<std::pair<spv::Id, InterfaceBlock>, 4> sorted_bindings;
  for (const auto &binding : InterfaceBlocks) {
    sorted_bindings.emplace_back(binding.first, binding.second);
  }
  std::sort(sorted_bindings.begin(), sorted_bindings.end());
  llvm::SmallVector<spv::Id, 4> binding_list;
  binding_list.reserve(sorted_bindings.size());
  for (auto &b : sorted_bindings) {
    binding_list.push_back(b.first);
  }
  return binding_list;
}

std::vector<spirv_ll::DescriptorBinding>
spirv_ll::Module::getUsedDescriptorBindings() const {
  std::vector<DescriptorBinding> bindingList;
  for (const auto &bindingPair : InterfaceBlocks) {
    bindingList.push_back(bindingPair.second.binding);
  }
  return bindingList;
}

bool spirv_ll::Module::hasDescriptorBindings() const {
  return !InterfaceBlocks.empty();
}

const spirv_ll::OpCode *spirv_ll::Module::getBindingOp(spv::Id id) const {
  auto found = InterfaceBlocks.find(id);
  SPIRV_LL_ASSERT(found != InterfaceBlocks.end(),
                  "Bad binding ID in InterfaceBlock lookup!");
  return found->second.op;
}

void spirv_ll::Module::addInterfaceBlockVariable(
    const spv::Id id, const spirv_ll::OpVariable *op, llvm::Type *variableTy,
    llvm::GlobalVariable *variable) {
  auto found = InterfaceBlocks.find(id);
  SPIRV_LL_ASSERT(found != InterfaceBlocks.end(),
                  "Bad ID given for interface block!");
  found->second.op = op;
  found->second.variable = variable;
  found->second.block_type = variableTy;
}

llvm::Type *spirv_ll::Module::getBlockType(const spv::Id id) const {
  auto iter = InterfaceBlocks.find(id);
  if (iter != InterfaceBlocks.end()) {
    return iter->second.block_type;
  } else {
    return nullptr;
  }
}

bool spirv_ll::Module::addID(spv::Id id, const OpCode *Op, llvm::Value *V) {
  // If the ID has a name attached to it, try to set it here if it wasn't
  // already set. reference might not have been able to take a name (e.g., if
  // it was a undef/poison constant).
  if (auto name = getName(id); !name.empty() && V && !V->hasName()) {
    V->setName(name);
  }
  // SSA form forbids the reassignment of IDs
  auto existing = Values.find(id);
  if (existing != Values.end() && existing->second.Value != nullptr) {
    return false;
  }
  Values.insert({id, ValuePair(Op, V)});
  return true;
}

llvm::Type *spirv_ll::Module::getLLVMType(spv::Id id) const {
  return Types.lookup(id).Type;
}

void spirv_ll::Module::addForwardPointer(spv::Id id) {
  ForwardPointers.insert(id);
}

bool spirv_ll::Module::isForwardPointer(spv::Id id) const {
  return ForwardPointers.count(id);
}

void spirv_ll::Module::removeForwardPointer(spv::Id id) {
  ForwardPointers.erase(id);
}

void spirv_ll::Module::addForwardFnRef(spv::Id id, llvm::Function *fn) {
  auto res = ForwardFnRefs.try_emplace(id, fn);
  // If we didn't insert a new forward ref, check that we're not trying to
  // associate a different function with the existing one.
  if (!res.second) {
    SPIRV_LL_ASSERT(res.first->second == fn,
                    "Overwriting existing function forward reference");
  }
}

llvm::Function *spirv_ll::Module::getForwardFnRef(spv::Id id) const {
  auto found = ForwardFnRefs.find(id);
  if (found == ForwardFnRefs.end()) {
    return nullptr;
  }
  return found->getSecond();
}

void spirv_ll::Module::resolveForwardFnRef(spv::Id id) {
  // Don't actually remove it, but track it as being zero. If we try and add
  // another forward reference to the same function, we'll know that something
  // is wrong.
  auto found = ForwardFnRefs.find(id);
  if (found != ForwardFnRefs.end()) {
    found->second = nullptr;
  }
}

void spirv_ll::Module::addIncompleteStruct(
    const OpTypeStruct *struct_type,
    const llvm::SmallVector<spv::Id, 2> &missing_types) {
  IncompleteStructs.insert({struct_type, missing_types});
}

void spirv_ll::Module::updateIncompleteStruct(spv::Id member_id) {
  for (auto &iter : IncompleteStructs) {
    for (auto idIter = iter.second.begin(); idIter != iter.second.end();
         idIter++) {
      // if the newly declared type id is found in an incomplete struct, delete
      // it from that struct's list of undefined types
      if (member_id == *idIter) {
        iter.second.erase(idIter);
        // if that was the last undefined type in the struct we can populate it
        if (iter.second.empty()) {
          llvm::SmallVector<llvm::Type *, 4> memberTypes;
          for (auto memberType : iter.first->MemberTypes()) {
            memberTypes.push_back(getLLVMType(memberType));
          }
          llvm::StructType *structType =
              llvm::cast<llvm::StructType>(getLLVMType(iter.first->IdResult()));
          structType->setBody(memberTypes);
          // remove the now fully populated struct from the map
          IncompleteStructs.erase(IncompleteStructs.find(iter.first));
        }
        return;
      }
    }
  }
}

llvm::Expected<unsigned> spirv_ll::Module::translateStorageClassToAddrSpace(
    uint32_t storage_class) const {
  switch (storage_class) {
    default:
      return makeStringError("Unknown StorageClass " +
                             std::to_string(storage_class));
    case spv::StorageClassFunction:
    case spv::StorageClassPrivate:
    case spv::StorageClassAtomicCounter:
    case spv::StorageClassInput:
    case spv::StorageClassOutput:
      // private
      return 0;
    case spv::StorageClassUniform:
    case spv::StorageClassCrossWorkgroup:
    case spv::StorageClassImage:
    case spv::StorageClassStorageBuffer:
      // global
      return 1;
    case spv::StorageClassUniformConstant:
    case spv::StorageClassPushConstant:
      // constant
      return 2;
    case spv::StorageClassWorkgroup:
      // local
      return 3;
    case spv::StorageClassGeneric:
      if (isExtensionEnabled("SPV_codeplay_usm_generic_storage_class")) {
        return 0;
      }
      return 4;
  }
}

llvm::Error spirv_ll::Module::addCompletePointer(const OpTypePointer *op) {
  const spv::Id type_id = op->Type();
  SPIRV_LL_ASSERT(!isForwardPointer(type_id), "type_id is a forward pointer");
  llvm::Type *type = getLLVMType(type_id);
  SPIRV_LL_ASSERT_PTR(type);

  // Pointer to void type isn't legal in llvm, so substitute char* in such
  // cases.
  if (type->isVoidTy()) {
    type = llvm::Type::getInt8Ty(llvmModule->getContext());
  }

  auto addrspace_or_error =
      translateStorageClassToAddrSpace(op->StorageClass());
  if (auto err = addrspace_or_error.takeError()) {
    return err;
  }

  llvm::Type *pointer_type =
      llvm::PointerType::get(type, addrspace_or_error.get());

  addID(op->IdResult(), op, pointer_type);

  if (isForwardPointer(op->IdResult())) {
    removeForwardPointer(op->IdResult());
    updateIncompleteStruct(op->IdResult());
    if (auto err = updateIncompletePointer(op->IdResult())) {
      return err;
    }
  }
  return llvm::Error::success();
}

void spirv_ll::Module::addIncompletePointer(const OpTypePointer *pointer_type,
                                            spv::Id missing_type) {
  IncompletePointers.insert({pointer_type, missing_type});
}

llvm::Error spirv_ll::Module::updateIncompletePointer(spv::Id type_id) {
  for (auto it = IncompletePointers.begin(), end = IncompletePointers.end();
       it != end;) {
    // if the newly declared type id is found in an incomplete pointer,
    if (type_id == it->second) {
      // complete the pointer
      if (auto err = addCompletePointer(it->first)) {
        return err;
      }
      // and remove the now completed pointer from the map
      IncompletePointers.erase(it);
      it = IncompletePointers.begin();
    } else {
      ++it;
    }
  }
  return llvm::Error::success();
}

void spirv_ll::Module::addSampledImage(spv::Id id, llvm::Value *image,
                                       llvm::Value *sampler) {
  const Module::SampledImage sampledImage =
      Module::SampledImage(image, sampler);
  SampledImagesMap.insert({id, sampledImage});
}

spirv_ll::Module::SampledImage spirv_ll::Module::getSampledImage(
    spv::Id id) const {
  return SampledImagesMap.lookup(id);
}

bool spirv_ll::Module::addID(spv::Id id, const OpCode *Op, llvm::Type *T) {
  // SSA form forbids the reassignment of IDs
  auto existing = Types.find(id);
  if (existing != Types.end() && existing->second.Type != nullptr) {
    return false;
  }
  Types.insert({id, TypePair(Op, T)});
  return true;
}

void spirv_ll::Module::setParamTypeIDs(spv::Id F, llvm::ArrayRef<spv::Id> IDs) {
  ParamTypeIDs[F].assign(IDs.begin(), IDs.end());
}

std::optional<spv::Id> spirv_ll::Module::getParamTypeID(spv::Id F,
                                                        unsigned argno) const {
  auto it = ParamTypeIDs.find(F);
  if (it == ParamTypeIDs.end()) {
    assert(false && "function type was not added before query");
    return std::nullopt;
  }
  auto &params = it->getSecond();
  if (argno >= params.size()) {
    assert(false && "invalid number of parameters for function");
    return std::nullopt;
  }
  return params[argno];
}

llvm::Value *spirv_ll::Module::getValue(spv::Id id) const {
  return Values.lookup(id).Value;
}

void spirv_ll::Module::addBuiltInID(spv::Id id) { BuiltInVarIDs.push_back(id); }

const llvm::SmallVector<spv::Id, 4> &spirv_ll::Module::getBuiltInVarIDs()
    const {
  return BuiltInVarIDs;
}

void spirv_ll::Module::addSpecId(spv::Id id, spv::Id spec_id) {
  SpecIDs.insert({id, spec_id});
}

std::optional<uint32_t> spirv_ll::Module::getSpecId(spv::Id id) const {
  auto found = SpecIDs.find(id);
  if (found == SpecIDs.end()) {
    return std::nullopt;
  }
  return found->second;
}

const cargo::optional<const spirv_ll::SpecializationInfo &> &
spirv_ll::Module::getSpecInfo() {
  return specInfo;
}

llvm::Type *spirv_ll::Module::getPushConstantStructType() const {
  return PushConstantStructVariable ? PushConstantStructVariable->getValueType()
                                    : nullptr;
}

spv::Id spirv_ll::Module::getPushConstantStructID() const {
  return PushConstantStructID;
}

llvm::Value *spirv_ll::Module::getBufferSizeArray() const {
  return BufferSizeArray;
}

void spirv_ll::Module::setPushConstantStructVariable(
    spv::Id id, llvm::GlobalVariable *variable) {
  PushConstantStructID = id;
  PushConstantStructVariable = variable;
}

void spirv_ll::Module::setWGS(uint32_t x, uint32_t y, uint32_t z) {
  WorkgroupSize = {{x, y, z}};
}

const std::array<uint32_t, 3> &spirv_ll::Module::getWGS() const {
  return WorkgroupSize;
}

void spirv_ll::Module::setBufferSizeArray(llvm::Value *buffer_size_array) {
  BufferSizeArray = buffer_size_array;
}

void spirv_ll::Module::deferSpecConstantOp(
    const spirv_ll::OpSpecConstantOp *op) {
  deferredSpecConstantOps.push_back(op);
}

const llvm::SmallVector<const spirv_ll::OpSpecConstantOp *, 2> &
spirv_ll::Module::getDeferredSpecConstants() {
  return deferredSpecConstantOps;
}

llvm::SmallVector<std::pair<spv::Id, llvm::GlobalVariable *>, 4>
spirv_ll::Module::getGlobalArgs() const {
  llvm::SmallVector<std::pair<spv::Id, llvm::GlobalVariable *>, 4> globals;

  for (const auto &iter : InterfaceBlocks) {
    globals.emplace_back(iter.first, iter.second.variable);
  }

  if (PushConstantStructVariable) {
    globals.emplace_back(PushConstantStructID, PushConstantStructVariable);
  }

  return globals;
}

const std::string &spirv_ll::Module::getModuleProcess() const {
  return ModuleProcess;
}

void spirv_ll::Module::setModuleProcess(const std::string &str) {
  ModuleProcess = str;
}

bool spirv_ll::Module::isOpExtInst(
    spv::Id id, const std::unordered_set<uint32_t> &opcodes,
    const std::unordered_set<ExtendedInstrSet> &sets) const {
  const auto *op = get_or_null<OpExtInst>(id);
  if (!op) {
    return false;
  }
  if (!sets.count(getExtendedInstrSet(op->Set()))) {
    return false;
  }
  return opcodes.count(op->Instruction());
}

bool spirv_ll::Module::isOpExtInst(
    spv::Id id, uint32_t opcode,
    const std::unordered_set<ExtendedInstrSet> &sets) const {
  const std::unordered_set<uint32_t> opcodes = {opcode};
  return isOpExtInst(id, opcodes, sets);
}
