// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <llvm/IR/Attributes.h>
#include <llvm/Support/type_traits.h>
#include <multi_llvm/optional_helper.h>
#include <multi_llvm/vector_type_helper.h>
#include <spirv-ll/assert.h>
#include <spirv-ll/builder.h>
#include <spirv-ll/context.h>
#include <spirv-ll/module.h>

#include <mutex>

spirv_ll::Builder::Builder(spirv_ll::Context &context, spirv_ll::Module &module,
                           const spirv_ll::DeviceInfo &deviceInfo)
    : context(context),
      module(module),
      deviceInfo(deviceInfo),
      IRBuilder(*context.llvmContext),
      DIBuilder(*module.llvmModule),
      CurrentFunction(nullptr),
      OpenCLBuilder(*this, module),
      GLSLBuilder(*this, module),
      GroupAsyncCopiesBuilder(*this, module) {}

llvm::IRBuilder<> &spirv_ll::Builder::getIRBuilder() { return IRBuilder; }

llvm::Function *spirv_ll::Builder::getCurrentFunction() {
  return CurrentFunction;
}

void spirv_ll::Builder::setCurrentFunction(llvm::Function *function) {
  CurrentFunction = function;
  CurrentFunctionArgs.clear();
  if (CurrentFunction) {
    for (llvm::Argument &arg : function->args()) {
      CurrentFunctionArgs.push_back(&arg);
    }
  }
}

llvm::Value *spirv_ll::Builder::popFunctionArg() {
  llvm::Value *arg = CurrentFunctionArgs.front();
  CurrentFunctionArgs.erase(CurrentFunctionArgs.begin());
  return arg;
}

llvm::DIType *spirv_ll::Builder::getDIType(llvm::Type *type) {
  const llvm::DataLayout &datalayout =
      IRBuilder.GetInsertBlock()->getModule()->getDataLayout();

  uint32_t align = datalayout.getABITypeAlignment(type);

  uint64_t size = datalayout.getTypeAllocSizeInBits(type);

  std::string name;

  if (type->isAggregateType()) {
    switch (type->getTypeID()) {
      case llvm::Type::ArrayTyID: {
        llvm::DIType *elem_type = getDIType(type->getArrayElementType());

        return DIBuilder.createArrayType(type->getArrayNumElements(), align,
                                         elem_type, llvm::DINodeArray());
      }
      case llvm::Type::StructTyID: {
        llvm::StructType *struct_type = llvm::cast<llvm::StructType>(type);

        llvm::SmallVector<llvm::Metadata *, 4> element_types;

        for (uint32_t elem_index = 0;
             elem_index < struct_type->getNumElements(); elem_index++) {
          element_types.push_back(
              getDIType(struct_type->getElementType(elem_index)));
        }

        // TODO: track line info for struct definitions, will require further
        // interface changes so for now just use 0
        return DIBuilder.createStructType(
            module.getCompileUnit(), struct_type->getName(), module.getDIFile(),
            0, size, align, llvm::DINode::FlagZero, nullptr,
            DIBuilder.getOrCreateArray(element_types));
      }
      case multi_llvm::FixedVectorTyID: {
        llvm::DIType *elem_type =
            getDIType(multi_llvm::getVectorElementType(type));

        return DIBuilder.createVectorType(
            multi_llvm::getVectorNumElements(type), align, elem_type,
            llvm::DINodeArray());
      }
      default:
        llvm_unreachable("unsupported debug type");
    }
  } else {
    switch (type->getTypeID()) {
      case llvm::Type::IntegerTyID:
        if (llvm::cast<llvm::IntegerType>(type)->getSignBit()) {
          name = "dbg_int_ty";
        } else {
          name = "dbg_uint_ty";
        }
        break;
      case llvm::Type::FloatTyID:
        name = "dbg_float_ty";
        break;
      case llvm::Type::PointerTyID: {
        auto *opTy = module.get<OpType>(type);
        SPIRV_LL_ASSERT(opTy && opTy->isPointerType(), "Type is not a pointer");
        llvm::DIType *elem_type =
            getDIType(module.getType(opTy->getTypePointer()->Type()));
        return DIBuilder.createPointerType(elem_type, size, align);
      }
      default:
        llvm_unreachable("unsupported debug type");
    }
  }

  return DIBuilder.createBasicType(name, size, align);
}

void spirv_ll::Builder::addDebugInfoToModule() {
  // If any debug info was added to the module we will have at least a
  // `DICompileUnit`
  if (module.getCompileUnit()) {
    DIBuilder.finalize();
  }

  for (auto op_line_info : module.getOpLineRanges()) {
    llvm::DebugLoc location = llvm::DebugLoc(op_line_info.first);
    auto range_pair = op_line_info.second;

    range_pair.first++;
    if (range_pair.second !=
        range_pair.first->getParent()->getInstList().end()) {
      range_pair.second++;
    }

    auto range = llvm::iterator_range<llvm::BasicBlock::iterator>(
        range_pair.first, range_pair.second);

    for (auto &inst : range) {
      inst.setDebugLoc(location);
    }
  }
}

namespace {
llvm::StringRef getBuiltinName(uint32_t builtin) {
  // Return the mangled names here as there will be no OpCode's to pass to
  // createMangledBuiltinCall for use in name mangling.
  switch (builtin) {
    case spv::BuiltInNumWorkgroups:
      return "_Z14get_num_groupsj";
    case spv::BuiltInWorkDim:
      return "_Z12get_work_dimv";
    case spv::BuiltInWorkgroupSize:
      return "_Z14get_local_sizej";
    case spv::BuiltInWorkgroupId:
      return "_Z12get_group_idj";
    case spv::BuiltInLocalInvocationId:
      return "_Z12get_local_idj";
    case spv::BuiltInGlobalInvocationId:
      return "_Z13get_global_idj";
    case spv::BuiltInGlobalSize:
      return "_Z15get_global_sizej";
    case spv::BuiltInGlobalOffset:
      return "_Z17get_global_offsetj";
    case spv::BuiltInSubgroupId:
      return "_Z16get_sub_group_idv";
    case spv::BuiltInSubgroupSize:
      return "_Z18get_sub_group_sizev";
    case spv::BuiltInSubgroupMaxSize:
      return "_Z22get_max_sub_group_sizev";
    case spv::BuiltInNumSubgroups:
      return "_Z18get_num_sub_groupsv";
    case spv::BuiltInNumEnqueuedSubgroups:
      return "_Z27get_enqueued_num_sub_groupsv";
    case spv::BuiltInSubgroupLocalInvocationId:
#ifdef SPIRV_LL_EXPERIMENTAL
      // This is not the standard translation for SubgroupLocalInvocationId, the
      // ifdef here should be replaced with a proper extension mechanism. See
      // CA-3067.
      return "_Z21get_sub_group_item_idv";
#else
      return "_Z22get_sub_group_local_idv";
#endif
    case spv::BuiltInGlobalLinearId:
      return "_Z20get_global_linear_idv";
    case spv::BuiltInLocalInvocationIndex:
      return "_Z19get_local_linear_idv";
    case spv::BuiltInEnqueuedWorkgroupSize:
      return "_Z23get_enqueued_local_sizej";
    default:
      llvm_unreachable("invalid work item builtin");
  }
}
}  // namespace

void spirv_ll::Builder::generateBuiltinInitBlock(spv::BuiltIn builtin,
                                                 llvm::Type *builtinType,
                                                 llvm::BasicBlock *initBlock) {
  IRBuilder.SetInsertPoint(initBlock);
  auto dataLayout = module.llvmModule->getDataLayout();

  switch (builtin) {
    case spv::BuiltInSubgroupId:                 // uint get_sub_group_id()
    case spv::BuiltInSubgroupSize:               // uint get_sub_group_size()
    case spv::BuiltInSubgroupLocalInvocationId:  // uint get_sub_group_local_id()
    case spv::BuiltInWorkDim: {                  // uint get_work_dim()
      auto builtinVal =
          IRBuilder.CreateAlloca(builtinType, dataLayout.getAllocaAddrSpace());

      // Builtin returns uint
      auto builtinRetTy = IRBuilder.getInt32Ty();

      // Create a call to the builtin
      auto initVal =
          createBuiltinCall(getBuiltinName(builtin), builtinRetTy, {});

      // Store the initializer in the builtin variable
      IRBuilder.CreateStore(initVal, builtinVal);
    } break;
    case spv::BuiltInNumWorkgroups:       // size_t get_num_groups(uint)
    case spv::BuiltInWorkgroupSize:       // size_t get_local_size(uint)
    case spv::BuiltInWorkgroupId:         // size_t get_group_id(uint)
    case spv::BuiltInLocalInvocationId:   // size_t get_local_id(uint)
    case spv::BuiltInGlobalInvocationId:  // size_t get_global_id(uint)
    case spv::BuiltInGlobalSize:          // size_t get_global_size(uint)
    case spv::BuiltInGlobalOffset:        // size_t get_global_offset(uint)
    case spv::BuiltInEnqueuedWorkgroupSize: {  // size_t
                                               // get_enqueued_local_size(uint)
      auto *builtinVal =
          IRBuilder.CreateAlloca(builtinType, dataLayout.getAllocaAddrSpace());

      // Builtin returns size_t, get the appropriate sized integer type.
      llvm::Type *builtinRetTy = nullptr;
      if (32 == dataLayout.getPointerSizeInBits()) {
        builtinRetTy = IRBuilder.getInt32Ty();
      } else {
        SPIRV_LL_ASSERT(64 == dataLayout.getPointerSizeInBits(),
                        "Datalayout is neither 32 nor 64 bits");
        builtinRetTy = IRBuilder.getInt64Ty();
      }
      SPIRV_LL_ASSERT_PTR(builtinRetTy);

      // Create an undefined vector to store the builtin initializer.
      llvm::Value *initVec = llvm::UndefValue::get(builtinType);

      // Loop over the vector elements, the assumption of 3 dimensions is
      // baked into SPIR-V even if only a single dimension is actually used.
      for (uint32_t index = 0; index < 3; index++) {
        // Create a call to the builtin.
        llvm::Value *initVal = createBuiltinCall(
            getBuiltinName(builtin), builtinRetTy, {IRBuilder.getInt32(index)});
        SPIRV_LL_ASSERT_PTR(initVal);

        // Vulkan defines some builtin variables as hard coded int32s, so there
        // is a chance we need to truncate returned values down to fit this.
        if (builtinRetTy->getScalarSizeInBits() >
            multi_llvm::getVectorElementType(builtinType)
                ->getScalarSizeInBits()) {
          initVal = IRBuilder.CreateTrunc(
              initVal, multi_llvm::getVectorElementType(builtinType));
        }

        initVec = IRBuilder.CreateInsertElement(initVec, initVal, index);
      }

      // Store the initializer in the builtin variable.
      IRBuilder.CreateStore(initVec, builtinVal);
    } break;
    case spv::BuiltInLocalInvocationIndex:  // size_t get_local_linear_id()
    case spv::BuiltInGlobalLinearId: {      // size_t get_global_linear_id()
      auto *builtinVal =
          IRBuilder.CreateAlloca(builtinType, dataLayout.getAllocaAddrSpace());

      // Builtin returns size_t, get the appropriate sized integer type.
      llvm::Type *builtinRetTy = nullptr;
      if (32 == dataLayout.getPointerSizeInBits()) {
        builtinRetTy = IRBuilder.getInt32Ty();
      } else {
        SPIRV_LL_ASSERT(64 == dataLayout.getPointerSizeInBits(),
                        "Datalayout is neither 32 nor 64 bits");
        builtinRetTy = IRBuilder.getInt64Ty();
      }
      SPIRV_LL_ASSERT_PTR(builtinRetTy);

      // Create a call to the builtin.
      llvm::Value *initVal =
          createBuiltinCall(getBuiltinName(builtin), builtinRetTy, {});
      SPIRV_LL_ASSERT_PTR(initVal);

      // Because the following two builtins have different return types:
      // GLSL: uint gl_LocalInvocationIndex
      // CL C: size_t get_enqueued_local_size( uint dimindx)
      // we need to make a cast on the GLSL path.
      if (builtinRetTy->getScalarSizeInBits() >
          builtinType->getScalarSizeInBits()) {
        initVal = IRBuilder.CreateTrunc(initVal, builtinType->getScalarType());
      }

      // Store the initializer in the builtin variable.
      IRBuilder.CreateStore(initVal, builtinVal);
    } break;
#define UNSUPPORTED(BUILTIN)  \
  case spv::BuiltIn##BUILTIN: \
    SPIRV_LL_ABORT("BuiltIn " #BUILTIN " not supported");
      UNSUPPORTED(SubgroupMaxSize)       // uint get_max_sub_group_size()
      UNSUPPORTED(NumSubgroups)          // uint get_num_sub_groups()
      UNSUPPORTED(NumEnqueuedSubgroups)  // uint get_enqueued_num_sub_groups()
#undef UNSUPPORTED
    default:
      SPIRV_LL_ABORT("BuiltIn unknown");
  }
}

bool spirv_ll::Builder::replaceBuiltinUsesWithCalls(
    llvm::GlobalVariable *builtinGlobal, spv::BuiltIn kind) {
  llvm::SmallVector<llvm::User *, 4> Deletes;
  llvm::SmallVector<llvm::User *, 4> Uses;
  for (auto UI = builtinGlobal->user_begin(), UE = builtinGlobal->user_end();
       UI != UE; ++UI) {
    llvm::Instruction *useInst = llvm::cast<llvm::Instruction>(*UI);
    if (auto ASCast = dyn_cast<llvm::AddrSpaceCastInst>(*UI)) {
      useInst = llvm::cast<llvm::Instruction>(*ASCast->user_begin());
      Deletes.push_back(ASCast);
    }
    if (llvm::isa<llvm::LoadInst>(useInst)) {
      if (!builtinGlobal->getValueType()->isVectorTy()) {
        Uses.push_back(useInst);
        Deletes.push_back(useInst);
        continue;
      }
      for (auto LDUI = useInst->user_begin(), LDUE = useInst->user_end();
           LDUI != LDUE; ++LDUI) {
        // If we find that this module is trying to use a builtin variable as a
        // vector (i.e. not just extracting one element at a time after loading)
        // we can't replace all its uses with calls to the builtin function.
        if (!isa<llvm::ExtractElementInst>(*LDUI)) {
          return false;
        }
        Uses.push_back(*LDUI);
        Deletes.push_back(*LDUI);
      }
      Deletes.push_back(useInst);
    } else if (llvm::isa<llvm::GetElementPtrInst>(useInst)) {
      for (auto GEPUI = useInst->user_begin(), GEPUE = useInst->user_end();
           GEPUI != GEPUE; ++GEPUI) {
        // Again, if this access isn't a simple GEP->load scenario give up on
        // this optimization.
        if (!llvm::isa<llvm::LoadInst>(*GEPUI)) {
          return false;
        }
        Uses.push_back(*GEPUI);
        Deletes.push_back(*GEPUI);
      }
      Deletes.push_back(useInst);
    } else {
      // If we have neither of the above cases something funky is going on and
      // we should stick to the relative safety of an init block for this one.
      return false;
    }
  }

  // If we've gotten this far we can replace all uses of this builtin global
  // with work item function calls, so get the name and type of the function.
  llvm::StringRef builtinName = getBuiltinName(kind);
  llvm::Type *funcRetTy = nullptr;
  // get_work_dim and sub-group builtins return a uint, all the other work item
  // functions return size_t
  const bool isSubGroupBuiltin =
      (spv::BuiltInSubgroupSize <= kind &&
       kind <= spv::BuiltInSubgroupLocalInvocationId);
  if (kind == spv::BuiltInWorkDim || isSubGroupBuiltin) {
    funcRetTy = IRBuilder.getInt32Ty();
  } else {
    auto dataLayout = module.llvmModule->getDataLayout();
    if (32 == dataLayout.getPointerSizeInBits()) {
      funcRetTy = IRBuilder.getInt32Ty();
    } else {
      SPIRV_LL_ASSERT(64 == dataLayout.getPointerSizeInBits(),
                      "Datalayout is neither 32 nor 64 bits");
      funcRetTy = IRBuilder.getInt64Ty();
    }
  }

  for (auto &use : Uses) {
    llvm::SmallVector<llvm::Value *, 1> arg;
    llvm::Value *index = nullptr;
    llvm::Instruction *useInst = llvm::cast<llvm::Instruction>(use);
    if (auto EEI = dyn_cast<llvm::ExtractElementInst>(use)) {
      index = EEI->getIndexOperand();
    } else if (auto LDI = dyn_cast<llvm::LoadInst>(use)) {
      // In the case of a GEP->load the dim arg to our work item function is
      // the last index provided to the GEP instruction. If we aren't loading
      // a GEP then this must be a call to get_work_dim() - so there is no arg.
      if (auto GEP =
              dyn_cast<llvm::GetElementPtrInst>(LDI->getPointerOperand())) {
        index = *(GEP->idx_end() - 1);
      }
    }

    if (index) {
      // Make sure our index is a 32 bit integer to match the work item function
      // signatures.
      if (index->getType()->getScalarSizeInBits() != 32) {
        index = llvm::CastInst::CreateIntegerCast(index, IRBuilder.getInt32Ty(),
                                                  false, "", useInst);
      }
      arg.push_back(index);
    }

    IRBuilder.SetInsertPoint(useInst);
    auto workItemCall = llvm::cast<llvm::Instruction>(
        createBuiltinCall(builtinName, funcRetTy, arg));
    // Cast the function call to the correct type for the use if necessary. This
    // is needed for VK modules sometimes because the GLSL builtin variables are
    // vectors of 32 bit ints whereas the CL work item functions return size_t.
    if (use->getType() != workItemCall->getType()) {
      workItemCall = llvm::CastInst::CreateIntegerCast(
          workItemCall, use->getType(), false, "", useInst);
    }
    workItemCall->takeName(useInst);
    useInst->replaceAllUsesWith(workItemCall);
  }
  for (auto &d : Deletes) {
    llvm::cast<llvm::Instruction>(d)->eraseFromParent();
  }

  builtinGlobal->eraseFromParent();
  return true;
}

void spirv_ll::Builder::replaceBuiltinGlobals() {
  for (const auto &ID : module.getBuiltInVarIDs()) {
    llvm::GlobalVariable *builtin_global =
        llvm::cast<llvm::GlobalVariable>(module.getValue(ID));

    // Erase the global and return early if it wasn't used.
    if (builtin_global->use_empty()) {
      builtin_global->eraseFromParent();
      return;
    }

    // To generate the init block we need the type of the builtin and which
    // builtin the variable was decorated with.
    llvm::Type *varTy = builtin_global->getValueType();

    auto *opDecorate = module.getFirstDecoration(ID, spv::DecorationBuiltIn);
    SPIRV_LL_ASSERT_PTR(opDecorate);

    spv::BuiltIn builtin = spv::BuiltIn(opDecorate->getValueAtOffset(3));

    // Before generating an init block see if we can replace all uses of the
    // builtin with calls to the work item function. If we can, skip this one.
    if (builtin != spv::BuiltInLocalInvocationIndex &&
        replaceBuiltinUsesWithCalls(builtin_global, builtin)) {
      continue;
    }

    // We need to track which functions the builtin is used in to insert the
    // init basic block, and each of the uses in those functions so we can
    // replace them.
    llvm::DenseMap<llvm::Function *, llvm::SmallVector<llvm::User *, 4>>
        user_functions;

    for (auto user : builtin_global->users()) {
      llvm::Function *use_function =
          llvm::cast<llvm::Instruction>(user)->getParent()->getParent();

      auto iter = user_functions.find(use_function);

      if (iter == user_functions.end()) {
        iter = user_functions
                   .insert(std::make_pair(use_function,
                                          llvm::SmallVector<llvm::User *, 4>()))
                   .first;
      }

      iter->getSecond().push_back(user);
    }

    // Finally insert the basic block wherever it is needed.
    for (auto &user_function : user_functions) {
      llvm::BasicBlock *start_of_func =
          &user_function.first->getBasicBlockList().front();

      llvm::BasicBlock *builtin_init_bb = llvm::BasicBlock::Create(
          *context.llvmContext, "init_builtin_var", user_function.first,
          &user_function.first->getBasicBlockList().front());

      generateBuiltinInitBlock(builtin, varTy, builtin_init_bb);

      IRBuilder.SetInsertPoint(builtin_init_bb);
      IRBuilder.CreateBr(start_of_func);

      // This alloca instruction is what we will replace uses of the global in
      // this function with.
      llvm::AllocaInst *new_builtin_var =
          llvm::cast<llvm::AllocaInst>(&builtin_init_bb->getInstList().front());

      // We have to move all the Allocas from the original entry block to the
      // start of the new entry block, for certain passes to work properly.
      auto it = start_of_func->begin();
      while (it != start_of_func->end()) {
        auto &Inst = *it++;
        if (!llvm::isa<llvm::AllocaInst>(Inst)) {
          break;
        }
        Inst.moveBefore(new_builtin_var);
      }

      for (llvm::User *user : user_function.second) {
        // Cast to Instruction so we can check which function this user is from,
        // since all we can do is ask for a list of all uses we always need to
        // check each use is in the current function.
        auto userVal = llvm::cast<llvm::Instruction>(user);
        if (userVal->getParent()->getParent() == user_function.first) {
          user->replaceUsesOfWith(builtin_global, new_builtin_var);
        }
      }
    }

    builtin_global->eraseFromParent();
  }
}

void spirv_ll::Builder::finalizeMetadata() {
  // Add source code metadata to module, if present
  if (!module.getSourceMetadataString().empty()) {
    auto ident = module.llvmModule->getOrInsertNamedMetadata("llvm.ident");

    auto md = llvm::MDString::get(*context.llvmContext,
                                  module.getSourceMetadataString());

    ident->addOperand(llvm::MDNode::get(*context.llvmContext,
                                        llvm::ArrayRef<llvm::Metadata *>(md)));
  }

  // Add !llvm.loop data
  module.resolveLoopControl();
}

void spirv_ll::Builder::pushBuiltinID(spv::Id id) {
  CurrentFunctionBuiltinIDs.push_back(id);
}

spirv_ll::BuiltinIDList &spirv_ll::Builder::getBuiltinIDList() {
  return CurrentFunctionBuiltinIDs;
}

// Creates a declaration for a builtin function
llvm::Function *spirv_ll::Builder::declareBuiltinFunction(
    const std::string &func_name, llvm::FunctionType *ty, bool convergent) {
  llvm::Function *func = llvm::Function::Create(
      ty, llvm::GlobalValue::LinkageTypes::ExternalLinkage, func_name,
      module.llvmModule.get());
  if (func_name != "__translate_sampler_initializer") {
    func->setCallingConv(llvm::CallingConv::SPIR_FUNC);
  }
  if (convergent) {
    func->setConvergent();
  }
  return func;
}

llvm::CallInst *spirv_ll::Builder::createBuiltinCall(
    llvm::StringRef name, llvm::Type *retTy, llvm::ArrayRef<llvm::Value *> args,
    bool convergent) {
  auto function = module.llvmModule->getFunction(name);
  SPIRV_LL_ASSERT(
      (nullptr == function) || (function->isConvergent() == convergent),
      "Function already exists but convergent attribute does not match");
  if (nullptr == function) {
    llvm::SmallVector<llvm::Type *, 16> argTys;
    for (auto arg : args) {
      argTys.push_back(arg->getType());
    }
    function = declareBuiltinFunction(
        name.str(), llvm::FunctionType::get(retTy, argTys, false), convergent);
  }
  auto call = IRBuilder.CreateCall(function, args);
  if (!name.equals("__translate_sampler_initializer")) {
    call->setCallingConv(llvm::CallingConv::SPIR_FUNC);
  }
  return call;
}

llvm::CallInst *spirv_ll::Builder::createConversionBuiltinCall(
    llvm::Value *value, llvm::ArrayRef<spv::Id> valueId, llvm::Type *retTy,
    llvm::Optional<spv::Id> retTyId, spv::Id resultId, bool saturated) {
  std::string builtin = "convert_";

  llvm::Type *scalarType = retTy;
  spv::Id scalarTypeId = multi_llvm::value_or(retTyId, 0);

  if (retTy->isVectorTy()) {
    scalarType = multi_llvm::getVectorElementType(retTy);
    if (retTyId.hasValue()) {
      const OpTypeVector *vectorTypeOp =
          module.get<OpTypeVector>(retTyId.getValue());
      scalarTypeId = vectorTypeOp->ComponentType();
    }
  }

  if (scalarType->isIntegerTy()) {
    // Assume signed unless explicitly told otherwise.
    uint32_t signedness = 1;
    if (scalarTypeId) {
      signedness = module.getSignedness(scalarTypeId);
    }
    builtin += getIntTypeName(scalarType, signedness);
  } else {
    builtin += getFPTypeName(scalarType);
  }

  if (retTy->isVectorTy()) {
    builtin += std::to_string(multi_llvm::getVectorNumElements(retTy));
  }

  // Check if this should be a saturated conversion.
  if ((module.hasCapability(spv::CapabilityKernel) &&
       module.getFirstDecoration(resultId,
                                 spv::DecorationSaturatedConversion)) ||
      saturated) {
    builtin += "_sat";
  }

  // Check if there is a rounding mode suffix we should be applying.
  if (module.hasCapabilityAnyOf({spv::CapabilityKernel,
                                 spv::CapabilityStorageUniformBufferBlock16,
                                 spv::CapabilityStorageUniform16,
                                 spv::CapabilityStoragePushConstant16,
                                 spv::CapabilityStorageInputOutput16})) {
    if (auto roundingMode =
            module.getFirstDecoration(resultId, spv::DecorationFPRoundingMode)) {
      builtin += getFPRoundingModeSuffix(roundingMode->getValueAtOffset(3));
    }
  }

  return createMangledBuiltinCall(builtin, retTy, retTyId, value, valueId);
}

llvm::CallInst *spirv_ll::Builder::createImageAccessBuiltinCall(
    std::string name, llvm::Type *retTy, llvm::Optional<spv::Id> retOp,
    llvm::ArrayRef<llvm::Value *> args, llvm::ArrayRef<spv::Id> ids,
    const spirv_ll::OpTypeVector *pixelTypeOp) {
  llvm::Type *pixelElementType = module.getType(pixelTypeOp->ComponentType());
  if (pixelElementType->isIntegerTy()) {
    // We need to look up the int type by ID because searching by `llvm::Type`
    // doesn't distinguish between signed and unsigned types, which can cause
    // incorrect mangling.
    auto integerOp = module.get<OpTypeInt>(pixelTypeOp->ComponentType());
    if (integerOp->Signedness()) {
      name += "i";
    } else {
      name += "ui";
    }
  } else if (pixelElementType->isFloatingPointTy()) {
    if (pixelElementType->getScalarSizeInBits() == 32) {
      name += "f";
    } else if (pixelElementType->getScalarSizeInBits() == 16) {
      name += "h";
    }
  }

  // To match the OpenCL builtin signatures we need to force the Coordinate
  // arguments to be signed, this is done by passing a null ID (0) so that
  // instead of looking up signedness of the ID the mangler assumes signed.
  llvm::SmallVector<spv::Id, 4> newIDs;
  if (name.find("write_") != std::string::npos) {
    newIDs = {ids[0], 0, ids[2]};
  } else if (name.find("read_") != std::string::npos) {
    newIDs.append(ids.begin(), ids.end());
    newIDs.back() = 0;
  }

  return createMangledBuiltinCall(name, retTy, retOp, args, newIDs);
}

llvm::CallInst *spirv_ll::Builder::createVectorDataBuiltinCall(
    std::string name, llvm::Type *dataType, llvm::Type *retTy,
    llvm::Optional<spv::Id> retOp, llvm::ArrayRef<llvm::Value *> args,
    llvm::ArrayRef<spv::Id> ids, llvm::Optional<spv::FPRoundingMode> mode,
    llvm::ArrayRef<TypeQualifier> typeQualifiers) {
  if (dataType->isVectorTy()) {
    name += std::to_string(multi_llvm::getVectorNumElements(dataType));
  }
  if (mode.hasValue()) {
    name += getFPRoundingModeSuffix(mode.getValue());
  }

  return createMangledBuiltinCall(name, retTy, retOp, args, ids,
                                  typeQualifiers);
}

llvm::CallInst *spirv_ll::Builder::createMangledBuiltinCall(
    llvm::StringRef name, llvm::Type *retTy, llvm::Optional<spv::Id> retOp,
    llvm::ArrayRef<llvm::Value *> args, llvm::ArrayRef<spv::Id> ops,
    llvm::ArrayRef<TypeQualifier> typeQualifiers, bool convergent) {
  auto mangledBuiltInCall = createBuiltinCall(
      getMangledFunctionName(name.str(), args, ops, typeQualifiers), retTy,
      args, convergent);
  auto calledFunction = mangledBuiltInCall->getCalledFunction();
  SPIRV_LL_ASSERT(nullptr != calledFunction, "Could not find function");

  // Setting the attribute for the function return type.
  if (retTy->isIntegerTy()) {
    // If the type is i8 or i16, it requires an attribute (signext or zeroext).
    // Vectors containing i8 or i16 do not require parameter attributes.
    const auto bitWidth = llvm::cast<llvm::IntegerType>(retTy)->getBitWidth();
    if (bitWidth == 8 || bitWidth == 16) {
      // Assume signed unless an OpCode was provided that says otherwise. We
      // assume signed here and below because a subset of OpenCL builtins treat
      // their parameters as signed, but creating a signed int type isn't
      // allowed by the OpenCL SPIR-V environment spec.
      llvm::Attribute::AttrKind attribute = llvm::Attribute::AttrKind::SExt;
      if (retOp.hasValue()) {
        if (!module.get<OpTypeInt>(retOp.getValue())->Signedness()) {
          attribute = llvm::Attribute::AttrKind::ZExt;
        }
      }
      mangledBuiltInCall->addRetAttr(attribute);
      calledFunction->addRetAttr(attribute);
    }
  }

  // Setting the attributes for the function arguments.
  for (size_t index = 0; index < args.size(); index++) {
    auto argTy = args[index]->getType();
    if (argTy->isIntegerTy()) {
      // If the type is i8 or i16, it requires an attribute (signext or
      // zeroext). Vectors containing i8 or i16 do not require parameter
      // attributes.
      const auto bitWidth = llvm::cast<llvm::IntegerType>(argTy)->getBitWidth();
      if (bitWidth == 8 || bitWidth == 16) {
        // Assume signed unless an OpCode was provided that says otherwise.
        llvm::Attribute::AttrKind attribute = llvm::Attribute::AttrKind::SExt;
        if (!ops.empty() && ops[index] != 0) {
          if (!cast<OpTypeInt>(module.getResultType(ops[index]))
                   ->Signedness()) {
            attribute = llvm::Attribute::AttrKind::ZExt;
          }
        }
        mangledBuiltInCall->addParamAttr(index, attribute);
        calledFunction->addParamAttr(index, attribute);
      }
    }
  }

  return mangledBuiltInCall;
}

std::string spirv_ll::Builder::getMangledFunctionName(
    std::string name, llvm::ArrayRef<llvm::Value *> args,
    llvm::ArrayRef<spv::Id> ids, llvm::ArrayRef<TypeQualifier> typeQualifiers) {
  // prefix the length of the function name
  name = applyMangledLength(name);

  // list of argument types which can be used as substitutes
  llvm::SmallVector<SubstitutableType, 16> subTys;

  // get the mangled argument name for each argument
  for (size_t index = 0; index < args.size(); index++) {
    auto argTy = args[index]->getType();

    const OpType *opTypeArg = nullptr;
    if (!ids.empty() && ids[index] != 0) {
      // get the opcode to take part in name mangling
      opTypeArg = module.getResultType(ids[index]);
    }

    TypeQualifier qualifier = NONE;

    if (!typeQualifiers.empty()) {
      qualifier = typeQualifiers[index];
    }

    // append the mangled argument type name
    name += getMangledTypeName(argTy, opTypeArg, subTys, qualifier);

    if (isSubstitutableArgType(argTy)) {
      // argument type is substitutable, add it to the substitutable list
      subTys.push_back({argTy, index, opTypeArg});
      // FIXME: We can't substitute pointer types unless we have IDs
      if (argTy->isPointerTy() && opTypeArg) {
        auto *argResultTy = opTypeArg;
        llvm::Type *pointeeTy = nullptr;
        if (argResultTy->isPointerType()) {
          pointeeTy = module.getType(argResultTy->getTypePointer()->Type());
        } else if (argResultTy->isImageType() || argResultTy->isEventType()) {
          pointeeTy = module.getInternalStructType(argResultTy->IdResult());
        }
        SPIRV_LL_ASSERT_PTR(pointeeTy);
        if (!pointeeTy->isIntegerTy() && !pointeeTy->isFloatingPointTy()) {
          const OpType *opTypeElement = nullptr;
          // attempt to get the OpCode object for our element type, this is
          // basically so we can check signedness if element type is a vector of
          // ints
          if (opTypeArg && opTypeArg->isPointerType()) {
            opTypeElement =
                module.get<OpType>(opTypeArg->getTypePointer()->Type());
            SPIRV_LL_ASSERT_PTR(opTypeElement);
          }
          subTys.push_back({pointeeTy, index, opTypeElement});
        }
      }
    }
  }

  return name;
}

const spirv_ll::Builder::SubstitutableType *spirv_ll::Builder::substitutableArg(
    llvm::Type *ty, const llvm::ArrayRef<SubstitutableType> &subTys,
    const OpType *op) {
  for (const SubstitutableType &subTy : subTys) {
    if (ty != subTy.ty) {
      continue;
    } else {
      // if the types are vectors, makes sure that both are signed/unsigned
      if (ty->isVectorTy() &&
          multi_llvm::getVectorElementType(ty)->isIntegerTy() && op) {
        uint32_t tySignedness =
            module.getSignedness(op->getTypeVector()->ComponentType());
        uint32_t subTySignedness = 1;
        if (subTy.op) {
          subTySignedness =
              module.getSignedness(subTy.op->getTypeVector()->ComponentType());
        }
        if (tySignedness != subTySignedness) {
          // if the vectors signs are different they should be
          // mangled separately and not substituted with the "S[index]_"
          continue;
        }
      }
      // if there is a match return the substitutable so that its *index* can be
      // used for the mangling string
      return &subTy;
    }
  }
  return nullptr;
}

std::string spirv_ll::Builder::getMangledTypeName(
    llvm::Type *ty, const OpType *op, llvm::ArrayRef<SubstitutableType> subTys,
    TypeQualifier qualifier) {
  auto subTyArg = substitutableArg(ty, subTys, op);
  if (subTyArg != nullptr) {
    // substitutable argument type has appeared before so, find its index
    if (0 == subTyArg->index) {
      // omit the index when the substitute type is the first argument
      return "S_";
    } else {
      // subsequent substitutions start at index 0 for the second argument
      return "S" + std::to_string(subTyArg->index - 1) + "_";
    }
  } else if (ty->isFloatingPointTy()) {
    return getMangledFPName(ty);
  } else if (ty->isIntegerTy()) {
    // assume signed integer when no opcode is provided
    uint32_t signedness = 2;
    if (op) {
      if (spv::OpTypeInt == op->code) {
        signedness = op->getTypeInt()->Signedness();
      } else if (spv::OpTypeBool == op->code) {
        return "i";
      } else if (spv::OpTypeSampler == op->code) {
        return "11ocl_sampler";
      } else if (spv::OpTypeVector == op->code) {
        signedness = module.getSignedness(op->getTypeVector()->ComponentType());
      } else {
        llvm_unreachable("unhandled interger op type!");
      }
    }
    return getMangledIntName(ty, signedness != 0);
  } else if (ty->isVectorTy()) {
    auto componentTypeOp =
        op ? module.get<OpType>(op->getTypeVector()->ComponentType()) : nullptr;
    auto elementTy = multi_llvm::getVectorElementType(ty);
    return getMangledVecPrefix(ty) +
           getMangledTypeName(elementTy, componentTypeOp, subTys, NONE);
  } else if (ty->isPointerTy()) {
    SPIRV_LL_ASSERT(op, "Must supply OpType to mangle pointer arguments");
    if (auto *structTy = module.getInternalStructType(op->IdResult())) {
      auto structName = structTy->getStructName();
      if (structName.contains("image1d_t")) {
        return "11ocl_image1d";
      } else if (structName.contains("image1d_array_t")) {
        return "16ocl_image1darray";
      } else if (structName.contains("image1d_buffer_t")) {
        return "17ocl_image1dbuffer";
      } else if (structName.contains("image2d_t")) {
        return "11ocl_image2d";
      } else if (structName.contains("image2d_array_t")) {
        return "16ocl_image2darray";
      } else if (structName.contains("image3d_t")) {
        return "11ocl_image3d";
      } else if (structName.contains("sampler_t")) {
        return "11ocl_sampler";
      } else if (structName.contains("event_t")) {
        return "9ocl_event";
      } else {
        std::fprintf(stderr,
                     "mangler: unknown pointer to struct argument type: %.*s\n",
                     static_cast<int>(structName.size()), structName.data());
        std::abort();
      }
    } else {
      SPIRV_LL_ASSERT(op->isPointerType(), "Parameter is not a pointer");
      auto *const elementTy = module.getType(op->getTypePointer()->Type());
      std::string mangled = getMangledPointerPrefix(ty, qualifier);
      auto typeOp = module.get<OpType>(op->getTypePointer()->Type());
      return mangled + getMangledTypeName(elementTy, typeOp, subTys, NONE);
    }
  } else if (ty->isArrayTy()) {
    auto elementTypeOp =
        op ? module.get<OpType>(op->getTypeArray()->ElementType()) : nullptr;
    return "P" + getMangledTypeName(ty->getArrayElementType(), elementTypeOp,
                                    subTys, NONE);
  }
  llvm_unreachable("mangler: unsupported argument type");
}

void spirv_ll::Builder::checkMemberDecorations(
    llvm::Type *accessedStructType,
    const llvm::SmallVector<llvm::Value *, 8> &indexes, spv::Id resultID) {
  // To figure out if the member being accessed has any decorations we first
  // need to know which struct type the member belongs to, i.e. the last struct
  // type in the chain before the final index. To get this we walk down the type
  // hierarchy following the index list until we hit something that isn't a
  // struct.
  // We also need to find out which of the indexes denotes struct member. This
  // isn't always the very last index, the member accessed could be an array for
  // instance, in which case there might be a further index after the member
  // index. Init to one because indexes has a 0 on the start just to dereference
  // the pointer.
  uint64_t memberIndex = 1;

  llvm::SmallVector<llvm::Type *, 4> traversed({accessedStructType});

  // Start at one for the reason described above.
  for (uint64_t i = 1; i < indexes.size(); i++) {
    llvm::Type *nextType = nullptr;
    switch (traversed.back()->getTypeID()) {
      case llvm::Type::StructTyID: {
        auto index = llvm::cast<llvm::ConstantInt>(indexes[i]);
        nextType =
            traversed.back()->getStructElementType(index->getZExtValue());
      } break;
      case llvm::Type::ArrayTyID:
        nextType = traversed.back()->getArrayElementType();
        break;
      case multi_llvm::FixedVectorTyID:
        nextType = multi_llvm::getVectorElementType(traversed.back());
        break;
      case llvm::Type::PointerTyID: {
        auto *opTy = module.get<OpType>(traversed.back());
        SPIRV_LL_ASSERT(opTy && opTy->isPointerType(), "Type is not a pointer");
        nextType = module.getType(opTy->getTypePointer()->Type());
        break;
      }
      default:
        // If we are here that means there is still another index left but the
        // last type in the chain can't be indexed into, thus: invalid SPIR-V.
        SPIRV_LL_ASSERT(nextType, "Bad type in OpAccessChain!");
        break;
    }
    traversed.push_back(nextType);

    // If we're at the last index walk backwards until we find the last struct
    // type.
    if (i == indexes.size() - 1) {
      // If the last type is a struct we need to go up a level to find the
      // containing struct type, as this struct is the member being accessed.
      if (traversed[i]->isStructTy()) {
        i--;
      }

      // Keep walking back until we find that struct.
      while (!traversed[i]->isStructTy()) {
        if (i == 0) {
          llvm_unreachable("Bad type being checked for member decorations!");
        }
        i--;
      }

      // This is the struct type whose member this access chain is accessing,
      // and the one we need to check for member decorations.
      accessedStructType = traversed[i];

      // Now the next index is the one that points to which member is being
      // accessed.
      memberIndex = i + 1;
      break;
    }
  }

  // Now we have the struct type and the member we can lookup decorations and
  // apply any that are there. Indexes into structs have to be OpConstantInt
  // according to the spec, so this cast is safe.
  uint32_t member =
      cast<llvm::ConstantInt>(indexes[memberIndex])->getZExtValue();
  auto structType = module.get<OpTypeStruct>(accessedStructType);
  const auto &memberDecorations =
      module.getMemberDecorations(structType->IdResult(), member);
  for (auto *opMemberDecorate : memberDecorations) {
    module.addDecoration(resultID, opMemberDecorate);
  }
}

void spirv_ll::Builder::generateSpecConstantOps() {
  auto deferredSpecConstants = module.getDeferredSpecConstants();
  if (deferredSpecConstants.empty()) {
    return;
  }

  // Define the offsets from an OpSpecConstantOp at which the instruction's
  // arguments can be found.
  const int firstArgIndex = 4;
  const int secondArgIndex = 5;

  llvm::Function *function = getCurrentFunction();

  // Save current insert point to reset to later.
  llvm::BasicBlock *oldBasicBlock = IRBuilder.GetInsertBlock();
  auto oldInsertPoint = IRBuilder.GetInsertPoint();

  llvm::BasicBlock *firstBasicBlock = &function->getBasicBlockList().front();

  // Create a new basic block at the very start of the function for the spec
  // constant instructions to be generated in.
  llvm::BasicBlock *specConstantBB = llvm::BasicBlock::Create(
      *context.llvmContext, "init_spec_constants", function, firstBasicBlock);

  IRBuilder.SetInsertPoint(specConstantBB);

  // Loop over the deferred instructions generating IR for each.
  for (const auto *op : deferredSpecConstants) {
    llvm::Value *result = nullptr;

    switch (op->Opcode()) {
      case spv::OpFMod: {
        llvm::Type *type = module.getType(op->IdResultType());
        SPIRV_LL_ASSERT_PTR(type);

        llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));
        SPIRV_LL_ASSERT_PTR(lhs);

        llvm::Value *rhs =
            module.getValue(op->getValueAtOffset(secondArgIndex));
        SPIRV_LL_ASSERT_PTR(rhs);

        // In order to be fully spec compliant we must use our FMod builtin and
        // then copysign the result with rhs to ensure the correct sign is
        // preserved.
        llvm::Value *modResult = createMangledBuiltinCall(
            "fmod", type, op->IdResultType(), {lhs, rhs},
            {op->getValueAtOffset(firstArgIndex),
             op->getValueAtOffset(secondArgIndex)});

        result = createMangledBuiltinCall("copysign", type, op->IdResultType(),
                                          {modResult, rhs}, {});
      } break;
      case spv::OpFRem: {
        llvm::Type *type = module.getType(op->IdResultType());
        SPIRV_LL_ASSERT_PTR(type);

        llvm::Value *lhs = module.getValue(op->getValueAtOffset(firstArgIndex));
        SPIRV_LL_ASSERT_PTR(lhs);

        llvm::Value *rhs =
            module.getValue(op->getValueAtOffset(secondArgIndex));
        SPIRV_LL_ASSERT_PTR(rhs);

        result = createMangledBuiltinCall(
            "fmod", type, op->IdResultType(), {lhs, rhs},
            {op->getValueAtOffset(firstArgIndex),
             op->getValueAtOffset(secondArgIndex)});
      } break;
      default:
        llvm_unreachable("Bad OpCode provided to OpSpecConstantOp");
        break;
    }

    // We need to use replaceID here because this may need to happen in
    // multiple functions and we need to make sure that value lookups always
    // get the relevant local value.
    module.replaceID(op, result);
  }

  // Finally link the new basic block to the top of the function.
  IRBuilder.CreateBr(firstBasicBlock);

  IRBuilder.SetInsertPoint(oldBasicBlock, oldInsertPoint);
}

void spirv_ll::Builder::handleGlobalParameters() {
  auto functionOp = module.get<OpFunction>(getCurrentFunction());
  auto uniformGlobals = module.getGlobalArgs();
  if (module.getEntryPoint(functionOp->IdResult())) {
    for (const auto &iter : uniformGlobals) {
      auto var = module.getValue(iter.first);
      IRBuilder.CreateStore(var, iter.second);
    }
  } else {
    for (const auto &iter : uniformGlobals) {
      auto paramOp = module.get<OpResult>(iter.first);
      auto loaded =
          IRBuilder.CreateLoad(iter.second->getValueType(), iter.second);
      module.replaceID(paramOp, loaded);
    }
  }
}

llvm::Type *spirv_ll::Builder::getRelationalReturnType(llvm::Value *operand) {
  // If the operand is a vector the result of the builtin will be a vector of
  // ints of the same size as the operand's scalar type, e.g. double2 will
  // return long2. Otherwise the return type is always an int32.
  if (operand->getType()->getTypeID() == multi_llvm::FixedVectorTyID) {
    return multi_llvm::FixedVectorType::get(
        IRBuilder.getIntNTy(operand->getType()->getScalarSizeInBits()),
        multi_llvm::getVectorNumElements(operand->getType()));
  } else {
    return IRBuilder.getInt32Ty();
  }
}
