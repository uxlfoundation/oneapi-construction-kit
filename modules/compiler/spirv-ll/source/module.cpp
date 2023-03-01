// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cargo/endian.h>
#include <multi_llvm/llvm_version.h>
#include <spirv-ll/assert.h>
#include <spirv-ll/context.h>
#include <spirv-ll/module.h>

#include <algorithm>

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
      CurrentOpLineRange(nullptr, llvm::BasicBlock::iterator()),
      LoopControl(),
      SamplerID(0),
      specInfo(specInfo),
      PushConstantStructVariable(nullptr),
      PushConstantStructID{},
      WorkgroupSize({{1, 1, 1}}),
      BufferSizeArray(nullptr),
      deferredSpecConstantOps() {}

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
      CurrentOpLineRange(nullptr, llvm::BasicBlock::iterator()),
      LoopControl(),
      SamplerID(0),
      specInfo(),
      PushConstantStructVariable(nullptr),
      PushConstantStructID{},
      WorkgroupSize({{1, 1, 1}}),
      BufferSizeArray(nullptr),
      deferredSpecConstantOps() {}

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

void spirv_ll::Module::replaceID(OpResult const *Op, llvm::Value *V) {
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

std::string spirv_ll::Module::getDebugString(spv::Id id) const {
  return DebugStrings.lookup(id);
}

void spirv_ll::Module::setCurrentOpLineRange(llvm::DILocation *block,
                                             llvm::BasicBlock::iterator pos) {
  CurrentOpLineRange = std::make_pair(block, pos);
}

void spirv_ll::Module::addCompleteOpLineRange(
    llvm::DILocation *location,
    std::pair<llvm::BasicBlock::iterator, llvm::BasicBlock::iterator> range) {
  OpLineRanges.insert({location, range});
}

llvm::MapVector<llvm::DILocation *, std::pair<llvm::BasicBlock::iterator,
                                              llvm::BasicBlock::iterator>>
    &spirv_ll::Module::getOpLineRanges() {
  return OpLineRanges;
}

std::pair<llvm::DILocation *, llvm::BasicBlock::iterator>
spirv_ll::Module::getCurrentOpLineRange() const {
  return CurrentOpLineRange;
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

void spirv_ll::Module::addDebugFunctionScope(
    llvm::Function *function, llvm::DISubprogram *function_scope) {
  FunctionScopes.insert({function, function_scope});
}

llvm::DISubprogram *spirv_ll::Module::getDebugFunctionScope(
    llvm::Function *function) const {
  auto found = FunctionScopes.find(function);
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
    llvm::SmallVector<const OpDecorateBase *, 2> decorations({op});
    DecoratedStruct newDecoratedStruct;
    newDecoratedStruct.try_emplace(member, decorations);
    MemberDecorations.try_emplace(structType, newDecoratedStruct);
  } else {
    auto member_iter = iter->second.find(member);
    if (member_iter == iter->second.end()) {
      llvm::SmallVector<const OpDecorateBase *, 2> decorations({op});
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

bool spirv_ll::Module::addID(spv::Id id, OpCode const *Op, llvm::Value *V) {
  // SSA form forbids the reassignment of IDs
  auto existing = Values.find(id);
  if (existing != Values.end() && existing->second.Value != nullptr) {
    return false;
  }
  Values.insert({id, ValuePair(Op, V)});
  return true;
}

llvm::Type *spirv_ll::Module::getType(spv::Id id) const {
  return Types.lookup(id).Type;
}

void spirv_ll::Module::setSignedness(spv::Id id, uint32_t signedness) {
  signednessMap.insert({id, signedness});
}

uint32_t spirv_ll::Module::getSignedness(spv::Id id) const {
  auto found = signednessMap.find(id);
  SPIRV_LL_ASSERT(found != signednessMap.end(), "Bad signedness lookup!");
  return found->getSecond();
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
            memberTypes.push_back(getType(memberType));
          }
          llvm::StructType *structType =
              llvm::cast<llvm::StructType>(getType(iter.first->IdResult()));
          structType->setBody(memberTypes);
          // remove the now fully populated struct from the map
          IncompleteStructs.erase(IncompleteStructs.find(iter.first));
        }
        return;
      }
    }
  }
}

void spirv_ll::Module::addCompletePointer(const OpTypePointer *op) {
  spv::Id typeId = op->Type();
  SPIRV_LL_ASSERT(!isForwardPointer(typeId), "typeId is a forward pointer");
  llvm::Type *type = getType(typeId);
  SPIRV_LL_ASSERT_PTR(type);

  // Pointer to void type isn't legal in llvm, so substitute char* in such
  // cases.
  if (type->isVoidTy()) {
    type = llvm::Type::getInt8Ty(llvmModule->getContext());
  }

  int AddressSpace = 0;

  switch (op->StorageClass()) {
    case spv::StorageClassFunction:
    case spv::StorageClassPrivate:
    case spv::StorageClassAtomicCounter:
    case spv::StorageClassInput:
    case spv::StorageClassOutput:
      // private
      break;
    case spv::StorageClassUniform:
    case spv::StorageClassCrossWorkgroup:
    case spv::StorageClassImage:
    case spv::StorageClassStorageBuffer:
      // global
      AddressSpace = 1;
      break;
    case spv::StorageClassUniformConstant:
    case spv::StorageClassPushConstant:
      // constant
      AddressSpace = 2;
      break;
    case spv::StorageClassWorkgroup:
      // local
      AddressSpace = 3;
      break;
// TODO: replace spirv-ll experimental with proper extension mechanism CA-3067
#ifdef SPIRV_LL_EXPERIMENTAL
    // See CA-2954
    case 5952:
      AddressSpace = 9;
      break;
#endif
    case spv::StorageClassGeneric: {
      if (isExtensionEnabled("SPV_codeplay_usm_generic_storage_class")) {
        AddressSpace = 0;
      } else {
        AddressSpace = 4;
      }
    } break;
    default:
      llvm_unreachable("unknown StorageClass");
  }

  llvm::Type *pointer_type = llvm::PointerType::get(type, AddressSpace);

  addID(op->IdResult(), op, pointer_type);

  if (isForwardPointer(op->IdResult())) {
    removeForwardPointer(op->IdResult());
    updateIncompleteStruct(op->IdResult());
    updateIncompletePointer(op->IdResult());
  }
}

void spirv_ll::Module::addIncompletePointer(const OpTypePointer *pointer_type,
                                            spv::Id missing_type) {
  IncompletePointers.insert({pointer_type, missing_type});
}

void spirv_ll::Module::updateIncompletePointer(spv::Id type_id) {
  for (auto it = IncompletePointers.begin(), end = IncompletePointers.end();
       it != end;) {
    // if the newly declared type id is found in an incomplete pointer,
    if (type_id == it->second) {
      // complete the pointer
      addCompletePointer(it->first);
      // and remove the now completed pointer from the map
      IncompletePointers.erase(it);
      it = IncompletePointers.begin();
    } else {
      ++it;
    }
  }
}

void spirv_ll::Module::setSampler(spv::Id sampler) { SamplerID = sampler; }

spv::Id spirv_ll::Module::getSampler() const { return SamplerID; }

void spirv_ll::Module::addSampledImage(spv::Id id, llvm::Value *image,
                                       llvm::Value *sampler) {
  Module::SampledImage sampledImage = Module::SampledImage(image, sampler);
  SampledImagesMap.insert({id, sampledImage});
}

spirv_ll::Module::SampledImage spirv_ll::Module::getSampledImage(
    spv::Id id) const {
  return SampledImagesMap.lookup(id);
}

bool spirv_ll::Module::addID(spv::Id id, OpCode const *Op, llvm::Type *T) {
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

cargo::optional<spv::Id> spirv_ll::Module::getParamTypeID(
    spv::Id F, unsigned argno) const {
  auto it = ParamTypeIDs.find(F);
  if (it == ParamTypeIDs.end()) {
    assert(false && "function type was not added before query");
    return cargo::nullopt;
  }
  auto &params = it->getSecond();
  if (argno >= params.size()) {
    assert(false && "invalid number of parameters for function");
    return cargo::nullopt;
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

cargo::optional<uint32_t> spirv_ll::Module::getSpecId(spv::Id id) const {
  auto found = SpecIDs.find(id);
  if (found == SpecIDs.end()) {
    return cargo::nullopt;
  }
  return found->second;
}

const cargo::optional<const spirv_ll::SpecializationInfo &>
    &spirv_ll::Module::getSpecInfo() {
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

const llvm::SmallVector<const spirv_ll::OpSpecConstantOp *, 2>
    &spirv_ll::Module::getDeferredSpecConstants() {
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
