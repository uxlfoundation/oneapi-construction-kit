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

#include <compiler/utils/target_extension_types.h>
#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/Metadata.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/type_traits.h>
#include <multi_llvm/vector_type_helper.h>
#include <spirv-ll/assert.h>
#include <spirv-ll/builder.h>
#include <spirv-ll/context.h>
#include <spirv-ll/module.h>

#include <mutex>
#include <optional>

spirv_ll::Builder::Builder(spirv_ll::Context &context, spirv_ll::Module &module,
                           const spirv_ll::DeviceInfo &deviceInfo)
    : context(context),
      module(module),
      deviceInfo(deviceInfo),
      IRBuilder(*context.llvmContext),
      DIBuilder(*module.llvmModule),
      CurrentFunction(nullptr),
      CurrentFunctionLexicalScope(std::nullopt) {}

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

llvm::DIType *spirv_ll::Builder::getDIType(spv::Id tyID) {
  const llvm::DataLayout &DL =
      IRBuilder.GetInsertBlock()->getModule()->getDataLayout();

  auto *const llvmTy = module.getLLVMType(tyID);
  SPIRV_LL_ASSERT_PTR(llvmTy);
  auto *const opTy = module.get<OpType>(tyID);
  SPIRV_LL_ASSERT_PTR(opTy);

  const uint32_t align = DL.getABITypeAlign(llvmTy).value();

  const uint64_t size = DL.getTypeAllocSizeInBits(llvmTy);

  if (opTy->isArrayType()) {
    llvm::DIType *elem_type = getDIType(opTy->getTypeArray()->ElementType());
    return DIBuilder.createArrayType(llvmTy->getArrayNumElements(), align,
                                     elem_type, llvm::DINodeArray());
  }

  if (opTy->isVectorType()) {
    llvm::DIType *elem_type = getDIType(opTy->getTypeVector()->ComponentType());
    return DIBuilder.createVectorType(opTy->getTypeVector()->ComponentCount(),
                                      align, elem_type, llvm::DINodeArray());
  }

  if (opTy->isStructType()) {
    auto *const opStructTy = opTy->getTypeStruct();
    llvm::StructType *struct_type = llvm::cast<llvm::StructType>(llvmTy);

    llvm::SmallVector<llvm::Metadata *, 4> member_types;

    for (auto memberTyID : opStructTy->MemberTypes()) {
      member_types.push_back(getDIType(memberTyID));
    }

    // TODO: track line info for struct definitions, will require further
    // interface changes so for now just use 0
    return DIBuilder.createStructType(
        module.getCompileUnit(), struct_type->getName(), module.getDIFile(),
        /*LineNumber*/ 0, size, align, llvm::DINode::FlagZero, nullptr,
        DIBuilder.getOrCreateArray(member_types));
  }

  if (opTy->isIntType()) {
    return opTy->getTypeInt()->Signedness()
               ? DIBuilder.createBasicType("dbg_int_ty", size,
                                           llvm::dwarf::DW_ATE_signed)

               : DIBuilder.createBasicType("dbg_uint_ty", size,
                                           llvm::dwarf::DW_ATE_unsigned);
  }

  if (opTy->isFloatType()) {
    return DIBuilder.createBasicType("dbg_float_ty", size,
                                     llvm::dwarf::DW_ATE_float);
  }

  if (opTy->isPointerType()) {
    llvm::DIType *elem_type = getDIType(opTy->getTypePointer()->Type());
    return DIBuilder.createPointerType(elem_type, size, align);
  }

  llvm_unreachable("unsupported debug type");
}

llvm::Error spirv_ll::Builder::finishModuleProcessing() {
  // Add debug info, before we start replacing global builtin vars; the
  // instruction ranges we've recorded are on the current state of the basic
  // blocks. Replacing the global builtins will invalidate the iterators.
  addDebugInfoToModule();

  // Replace all global builtin vars with function local versions
  replaceBuiltinGlobals();

  // Set some default attributes on functions we've created.
  for (auto &function : module.llvmModule->functions()) {
    // We don't use exceptions
    if (!function.hasFnAttribute(llvm::Attribute::NoUnwind)) {
      function.addFnAttr(llvm::Attribute::NoUnwind);
    }
  }

  // Add any remaining metadata to llvm module
  finalizeMetadata();

  // Notify handlers that the module has been finished.
  for (auto &[set, handler] : ext_inst_handlers) {
    if (auto err = handler->finishModuleProcessing()) {
      return err;
    }
  }

  return llvm::Error::success();
}

std::optional<spirv_ll::Builder::LexicalScopeTy>
spirv_ll::Builder::getCurrentFunctionLexicalScope() const {
  return CurrentFunctionLexicalScope;
}

void spirv_ll::Builder::setCurrentFunctionLexicalScope(
    std::optional<LexicalScopeTy> scope) {
  CurrentFunctionLexicalScope = scope;
}

std::optional<spirv_ll::Builder::LineRangeBeginTy>
spirv_ll::Builder::getCurrentOpLineRange() const {
  return CurrentOpLineRange;
}

void spirv_ll::Builder::setCurrentOpLineRange(
    std::optional<LineRangeBeginTy> scope) {
  CurrentOpLineRange = scope;
}

void spirv_ll::Builder::addDebugInfoToModule() {
  // If any debug info was added to the module we will have at least a
  // `DICompileUnit`
  if (module.getCompileUnit()) {
    DIBuilder.finalize();
  }
}

namespace {

static llvm::DenseMap<uint32_t, llvm::StringRef> BuiltinFnNames = {
    {spv::BuiltInNumWorkgroups, "_Z14get_num_groupsj"},
    {spv::BuiltInWorkDim, "_Z12get_work_dimv"},
    {spv::BuiltInWorkgroupSize, "_Z14get_local_sizej"},
    {spv::BuiltInWorkgroupId, "_Z12get_group_idj"},
    {spv::BuiltInLocalInvocationId, "_Z12get_local_idj"},
    {spv::BuiltInGlobalInvocationId, "_Z13get_global_idj"},
    {spv::BuiltInGlobalSize, "_Z15get_global_sizej"},
    {spv::BuiltInGlobalOffset, "_Z17get_global_offsetj"},
    {spv::BuiltInSubgroupId, "_Z16get_sub_group_idv"},
    {spv::BuiltInSubgroupSize, "_Z18get_sub_group_sizev"},
    {spv::BuiltInSubgroupMaxSize, "_Z22get_max_sub_group_sizev"},
    {spv::BuiltInNumSubgroups, "_Z18get_num_sub_groupsv"},
    {spv::BuiltInNumEnqueuedSubgroups, "_Z27get_enqueued_num_sub_groupsv"},
    {spv::BuiltInSubgroupLocalInvocationId, "_Z22get_sub_group_local_idv"},
    {spv::BuiltInGlobalLinearId, "_Z20get_global_linear_idv"},
    {spv::BuiltInLocalInvocationIndex, "_Z19get_local_linear_idv"},
    {spv::BuiltInEnqueuedWorkgroupSize, "_Z23get_enqueued_local_sizej"},
};

llvm::StringRef getBuiltinName(uint32_t builtin) {
  // Return the mangled names here as there will be no OpCode's to pass to
  // createMangledBuiltinCall for use in name mangling.
  auto It = BuiltinFnNames.find(builtin);
  if (It == BuiltinFnNames.end()) {
    llvm_unreachable("invalid work item builtin");
  }
  return It->getSecond();
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
    case spv::BuiltInSubgroupLocalInvocationId:  // uint get_sub_group_local_id
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
    case spv::BuiltInNumWorkgroups:            // size_t get_num_groups(uint)
    case spv::BuiltInWorkgroupSize:            // size_t get_local_size(uint)
    case spv::BuiltInWorkgroupId:              // size_t get_group_id(uint)
    case spv::BuiltInLocalInvocationId:        // size_t get_local_id(uint)
    case spv::BuiltInGlobalInvocationId:       // size_t get_global_id(uint)
    case spv::BuiltInGlobalSize:               // size_t get_global_size(uint)
    case spv::BuiltInGlobalOffset:             // size_t get_global_offset(uint)
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
  llvm::SmallVector<llvm::User *, 4> Worklist(builtinGlobal->users());

  while (!Worklist.empty()) {
    auto *UI = Worklist.pop_back_val();

    // We may have addrspacecast constant expressions
    if (auto *CE = dyn_cast<llvm::ConstantExpr>(UI)) {
      if (CE->getOpcode() != llvm::Instruction::AddrSpaceCast) {
        // If we have a different kind of constant expression something funky is
        // going on and we should stick to the relative safety of an init block
        // for this one.
        return false;
      }
      Deletes.push_back(CE);
      Worklist.append(CE->user_begin(), CE->user_end());
      continue;
    }

    if (auto *const ASCast = dyn_cast<llvm::AddrSpaceCastInst>(UI)) {
      Deletes.push_back(ASCast);
      Worklist.append(ASCast->user_begin(), ASCast->user_end());
      continue;
    }

    if (llvm::isa<llvm::LoadInst>(UI)) {
      Deletes.push_back(UI);
      if (!builtinGlobal->getValueType()->isVectorTy()) {
        Uses.push_back(UI);
        continue;
      }
      for (auto *const LDUI : UI->users()) {
        // If we find that this module is trying to use a builtin variable as a
        // vector (i.e. not just extracting one element at a time after loading)
        // we can't replace all its uses with calls to the builtin function.
        if (!isa<llvm::ExtractElementInst>(LDUI)) {
          return false;
        }
        Uses.push_back(LDUI);
        Deletes.push_back(LDUI);
      }
      continue;
    }

    if (llvm::isa<llvm::GetElementPtrInst>(UI)) {
      Deletes.push_back(UI);
      for (auto *const GEPUI : UI->users()) {
        // Again, if this access isn't a simple GEP->load scenario give up on
        // this optimization.
        if (!llvm::isa<llvm::LoadInst>(GEPUI)) {
          return false;
        }
        Uses.push_back(GEPUI);
        Deletes.push_back(GEPUI);
      }
      continue;
    }

    // If we have neither of the above cases something funky is going on and
    // we should stick to the relative safety of an init block for this one.
    return false;
  }

  // If we've gotten this far we can replace all uses of this builtin global
  // with work item function calls, so get the name and type of the function.
  const llvm::StringRef builtinName = getBuiltinName(kind);
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
    if (auto *EEI = dyn_cast<llvm::ExtractElementInst>(use)) {
      index = EEI->getIndexOperand();
    } else if (auto *LDI = dyn_cast<llvm::LoadInst>(use)) {
      // In the case of a GEP->load the dim arg to our work item function is
      // the last index provided to the GEP instruction. If we aren't loading
      // a GEP then this must be a call to get_work_dim() - so there is no arg.
      if (auto *GEP =
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

  while (!Deletes.empty()) {
    auto *UI = Deletes.pop_back_val();
    if (auto *const CE = llvm::dyn_cast<llvm::ConstantExpr>(UI)) {
      assert(CE->use_empty());
      CE->destroyConstant();
    } else {
      assert(UI->use_empty());
      llvm::cast<llvm::Instruction>(UI)->eraseFromParent();
    }
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

    const spv::BuiltIn builtin = spv::BuiltIn(opDecorate->getValueAtOffset(3));

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
      llvm::BasicBlock *start_of_func = &user_function.first->front();

      llvm::BasicBlock *builtin_init_bb = llvm::BasicBlock::Create(
          *context.llvmContext, "init_builtin_var", user_function.first,
          &user_function.first->front());

      generateBuiltinInitBlock(builtin, varTy, builtin_init_bb);

      IRBuilder.SetInsertPoint(builtin_init_bb);
      IRBuilder.CreateBr(start_of_func);

      // This alloca instruction is what we will replace uses of the global in
      // this function with.
      llvm::AllocaInst *new_builtin_var =
          llvm::cast<llvm::AllocaInst>(&builtin_init_bb->front());

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
  if (func_name != SAMPLER_INIT_FN) {
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
  // Builtin functions also only read memory - set that attribute
  if (llvm::find_if(BuiltinFnNames, [name](const auto &pair) {
        return pair.getSecond() == name;
      }) != BuiltinFnNames.end()) {
    function->setOnlyReadsMemory();
  }
  auto *const call = IRBuilder.CreateCall(function, args);
  if (!name.equals(SAMPLER_INIT_FN)) {
    call->setCallingConv(llvm::CallingConv::SPIR_FUNC);
  }
  return call;
}

llvm::CallInst *spirv_ll::Builder::createConversionBuiltinCall(
    llvm::Value *value, llvm::ArrayRef<MangleInfo> argMangleInfo,
    llvm::Type *retTy, MangleInfo retMangleInfo, spv::Id resultId,
    bool saturated) {
  std::string builtin = "convert_";

  llvm::Type *scalarType = retTy;

  if (retTy->isVectorTy()) {
    scalarType = multi_llvm::getVectorElementType(retTy);
  }

  if (scalarType->isIntegerTy()) {
    const uint32_t signedness = retMangleInfo.getSignedness(module);
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
    if (auto roundingMode = module.getFirstDecoration(
            resultId, spv::DecorationFPRoundingMode)) {
      builtin += getFPRoundingModeSuffix(roundingMode->getValueAtOffset(3));
    }
  }

  return createMangledBuiltinCall(builtin, retTy, retMangleInfo, value,
                                  argMangleInfo);
}

llvm::CallInst *spirv_ll::Builder::createImageAccessBuiltinCall(
    std::string name, llvm::Type *retTy, MangleInfo retMangleInfo,
    llvm::ArrayRef<llvm::Value *> args,
    llvm::ArrayRef<MangleInfo> argMangleInfo,
    const spirv_ll::OpTypeVector *pixelTypeOp) {
  llvm::Type *pixelElementType =
      module.getLLVMType(pixelTypeOp->ComponentType());
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
  llvm::SmallVector<MangleInfo, 4> newArgMangleInfo(argMangleInfo.begin(),
                                                    argMangleInfo.end());
  if (name.find("write_") != std::string::npos) {
    newArgMangleInfo[1].forceSign = MangleInfo::ForceSignInfo::ForceSigned;
  } else if (name.find("read_") != std::string::npos) {
    newArgMangleInfo.back().forceSign = MangleInfo::ForceSignInfo::ForceSigned;
  }

  return createMangledBuiltinCall(name, retTy, retMangleInfo, args,
                                  newArgMangleInfo);
}

llvm::Value *spirv_ll::Builder::createOCLBuiltinCall(
    OpenCLLIB::Entrypoints opcode, spv::Id resTyId,
    llvm::ArrayRef<spv::Id> args) {
  llvm::Type *resultType = module.getLLVMType(resTyId);
  SPIRV_LL_ASSERT_PTR(resultType);

  llvm::SmallVector<llvm::Value *, 4> argVals;
  llvm::transform(args, std::back_inserter(argVals),
                  [&](const spv::Id id) { return module.getValue(id); });

  llvm::SmallVector<MangleInfo, 4> argInfo;
  llvm::transform(args, std::back_inserter(argInfo),
                  [&](const spv::Id id) { return MangleInfo(id); });

  std::string fnName;
  switch (opcode) {
#define _OCL_OP(LIB, OP) \
  case LIB:              \
    fnName = #OP;        \
    break;
    _OCL_OP(OpenCLLIB::Acos, acos)
    _OCL_OP(OpenCLLIB::Acosh, acosh)
    _OCL_OP(OpenCLLIB::Acospi, acospi)
    _OCL_OP(OpenCLLIB::Asin, asin)
    _OCL_OP(OpenCLLIB::Asinh, asinh)
    _OCL_OP(OpenCLLIB::Asinpi, asinpi)
    _OCL_OP(OpenCLLIB::Atan, atan)
    _OCL_OP(OpenCLLIB::Atan2, atan2)
    _OCL_OP(OpenCLLIB::Atanh, atanh)
    _OCL_OP(OpenCLLIB::Atanpi, atanpi)
    _OCL_OP(OpenCLLIB::Atan2pi, atan2pi)
    _OCL_OP(OpenCLLIB::Cbrt, cbrt)
    _OCL_OP(OpenCLLIB::Ceil, ceil)
    _OCL_OP(OpenCLLIB::Copysign, copysign)
    _OCL_OP(OpenCLLIB::Cos, cos)
    _OCL_OP(OpenCLLIB::Cosh, cosh)
    _OCL_OP(OpenCLLIB::Cospi, cospi)
    _OCL_OP(OpenCLLIB::Erfc, erfc)
    _OCL_OP(OpenCLLIB::Erf, erf)
    _OCL_OP(OpenCLLIB::Exp, exp)
    _OCL_OP(OpenCLLIB::Exp2, exp2)
    _OCL_OP(OpenCLLIB::Exp10, exp10)
    _OCL_OP(OpenCLLIB::Expm1, expm1)
    _OCL_OP(OpenCLLIB::Fabs, fabs)
    _OCL_OP(OpenCLLIB::Fdim, fdim)
    _OCL_OP(OpenCLLIB::Floor, floor)
    _OCL_OP(OpenCLLIB::Fma, fma)
    _OCL_OP(OpenCLLIB::Fmax, fmax)
    _OCL_OP(OpenCLLIB::Fmin, fmin)
    _OCL_OP(OpenCLLIB::Fmod, fmod)
    _OCL_OP(OpenCLLIB::Fract, fract)
    _OCL_OP(OpenCLLIB::Frexp, frexp)
    _OCL_OP(OpenCLLIB::Hypot, hypot)
    _OCL_OP(OpenCLLIB::Ilogb, ilogb)
    _OCL_OP(OpenCLLIB::Ldexp, ldexp)
    _OCL_OP(OpenCLLIB::Lgamma, lgamma)
    _OCL_OP(OpenCLLIB::Lgamma_r, lgamma_r)
    _OCL_OP(OpenCLLIB::Log, log)
    _OCL_OP(OpenCLLIB::Log2, log2)
    _OCL_OP(OpenCLLIB::Log10, log10)
    _OCL_OP(OpenCLLIB::Log1p, log1p)
    _OCL_OP(OpenCLLIB::Logb, logb)
    _OCL_OP(OpenCLLIB::Mad, mad)
    _OCL_OP(OpenCLLIB::Maxmag, maxmag)
    _OCL_OP(OpenCLLIB::Minmag, minmag)
    _OCL_OP(OpenCLLIB::Modf, modf)
    _OCL_OP(OpenCLLIB::Nan, nan)
    _OCL_OP(OpenCLLIB::Nextafter, nextafter)
    _OCL_OP(OpenCLLIB::Pow, pow)
    _OCL_OP(OpenCLLIB::Pown, pown)
    _OCL_OP(OpenCLLIB::Powr, powr)
    _OCL_OP(OpenCLLIB::Remainder, remainder)
    _OCL_OP(OpenCLLIB::Remquo, remquo)
    _OCL_OP(OpenCLLIB::Rint, rint)
    _OCL_OP(OpenCLLIB::Rootn, rootn)
    _OCL_OP(OpenCLLIB::Round, round)
    _OCL_OP(OpenCLLIB::Rsqrt, rsqrt)
    _OCL_OP(OpenCLLIB::Sin, sin)
    _OCL_OP(OpenCLLIB::Sincos, sincos)
    _OCL_OP(OpenCLLIB::Sinh, sinh)
    _OCL_OP(OpenCLLIB::Sinpi, sinpi)
    _OCL_OP(OpenCLLIB::Sqrt, sqrt)
    _OCL_OP(OpenCLLIB::Tan, tan)
    _OCL_OP(OpenCLLIB::Tanh, tanh)
    _OCL_OP(OpenCLLIB::Tanpi, tanpi)
    _OCL_OP(OpenCLLIB::Tgamma, tgamma)
    _OCL_OP(OpenCLLIB::Trunc, trunc)
    _OCL_OP(OpenCLLIB::Half_cos, half_cos)
    _OCL_OP(OpenCLLIB::Half_divide, half_divide)
    _OCL_OP(OpenCLLIB::Half_exp, half_exp)
    _OCL_OP(OpenCLLIB::Half_exp2, half_exp2)
    _OCL_OP(OpenCLLIB::Half_exp10, half_exp10)
    _OCL_OP(OpenCLLIB::Half_log, half_log)
    _OCL_OP(OpenCLLIB::Half_log2, half_log2)
    _OCL_OP(OpenCLLIB::Half_log10, half_log10)
    _OCL_OP(OpenCLLIB::Half_powr, half_powr)
    _OCL_OP(OpenCLLIB::Half_recip, half_recip)
    _OCL_OP(OpenCLLIB::Half_rsqrt, half_rsqrt)
    _OCL_OP(OpenCLLIB::Half_sin, half_sin)
    _OCL_OP(OpenCLLIB::Half_sqrt, half_sqrt)
    _OCL_OP(OpenCLLIB::Half_tan, half_tan)
    _OCL_OP(OpenCLLIB::Native_cos, native_cos)
    _OCL_OP(OpenCLLIB::Native_divide, native_divide)
    _OCL_OP(OpenCLLIB::Native_exp, native_exp)
    _OCL_OP(OpenCLLIB::Native_exp2, native_exp2)
    _OCL_OP(OpenCLLIB::Native_exp10, native_exp10)
    _OCL_OP(OpenCLLIB::Native_log, native_log)
    _OCL_OP(OpenCLLIB::Native_log2, native_log2)
    _OCL_OP(OpenCLLIB::Native_log10, native_log10)
    _OCL_OP(OpenCLLIB::Native_powr, native_powr)
    _OCL_OP(OpenCLLIB::Native_recip, native_recip)
    _OCL_OP(OpenCLLIB::Native_rsqrt, native_rsqrt)
    _OCL_OP(OpenCLLIB::Native_sin, native_sin)
    _OCL_OP(OpenCLLIB::Native_sqrt, native_sqrt)
    _OCL_OP(OpenCLLIB::Native_tan, native_tan)
    _OCL_OP(OpenCLLIB::SAbs, abs)
    _OCL_OP(OpenCLLIB::UAbs, abs)
    _OCL_OP(OpenCLLIB::SAbs_diff, abs_diff)
    _OCL_OP(OpenCLLIB::UAbs_diff, abs_diff)
    _OCL_OP(OpenCLLIB::SAdd_sat, add_sat)
    _OCL_OP(OpenCLLIB::UAdd_sat, add_sat)
    _OCL_OP(OpenCLLIB::SHadd, hadd)
    _OCL_OP(OpenCLLIB::UHadd, hadd)
    _OCL_OP(OpenCLLIB::SRhadd, rhadd)
    _OCL_OP(OpenCLLIB::URhadd, rhadd)
    _OCL_OP(OpenCLLIB::SClamp, clamp)
    _OCL_OP(OpenCLLIB::UClamp, clamp)
    _OCL_OP(OpenCLLIB::FClamp, clamp)
    _OCL_OP(OpenCLLIB::Clz, clz)
    _OCL_OP(OpenCLLIB::Ctz, ctz)
    _OCL_OP(OpenCLLIB::SMad_hi, mad_hi)
    _OCL_OP(OpenCLLIB::UMad_hi, mad_hi)
    _OCL_OP(OpenCLLIB::SMad_sat, mad_sat)
    _OCL_OP(OpenCLLIB::UMad_sat, mad_sat)
    _OCL_OP(OpenCLLIB::SMax, max)
    _OCL_OP(OpenCLLIB::UMax, max)
    _OCL_OP(OpenCLLIB::FMax_common, max)
    _OCL_OP(OpenCLLIB::SMin, min)
    _OCL_OP(OpenCLLIB::UMin, min)
    _OCL_OP(OpenCLLIB::FMin_common, min)
    _OCL_OP(OpenCLLIB::SMul_hi, mul_hi)
    _OCL_OP(OpenCLLIB::UMul_hi, mul_hi)
    _OCL_OP(OpenCLLIB::Rotate, rotate)
    _OCL_OP(OpenCLLIB::SSub_sat, sub_sat)
    _OCL_OP(OpenCLLIB::USub_sat, sub_sat)
    _OCL_OP(OpenCLLIB::S_Upsample, upsample)
    _OCL_OP(OpenCLLIB::U_Upsample, upsample)
    _OCL_OP(OpenCLLIB::Popcount, popcount)
    _OCL_OP(OpenCLLIB::SMad24, mad24)
    _OCL_OP(OpenCLLIB::UMad24, mad24)
    _OCL_OP(OpenCLLIB::SMul24, mul24)
    _OCL_OP(OpenCLLIB::UMul24, mul24)
    _OCL_OP(OpenCLLIB::Degrees, degrees)
    _OCL_OP(OpenCLLIB::Mix, mix)
    _OCL_OP(OpenCLLIB::Radians, radians)
    _OCL_OP(OpenCLLIB::Step, step)
    _OCL_OP(OpenCLLIB::Smoothstep, smoothstep)
    _OCL_OP(OpenCLLIB::Sign, sign)
    _OCL_OP(OpenCLLIB::Cross, cross)
    _OCL_OP(OpenCLLIB::Distance, distance)
    _OCL_OP(OpenCLLIB::Length, length)
    _OCL_OP(OpenCLLIB::Normalize, normalize)
    _OCL_OP(OpenCLLIB::Fast_distance, fast_distance)
    _OCL_OP(OpenCLLIB::Fast_length, fast_length)
    _OCL_OP(OpenCLLIB::Fast_normalize, fast_normalize)
    _OCL_OP(OpenCLLIB::Bitselect, bitselect)
    _OCL_OP(OpenCLLIB::Select, select)
    _OCL_OP(OpenCLLIB::Shuffle, shuffle)
    _OCL_OP(OpenCLLIB::Shuffle2, shuffle2)
    _OCL_OP(OpenCLLIB::Prefetch, prefetch)
    _OCL_OP(OpenCLLIB::Vloadn, vload)
    _OCL_OP(OpenCLLIB::Vload_half, vload_half)
    _OCL_OP(OpenCLLIB::Vload_halfn, vload_half)
    _OCL_OP(OpenCLLIB::Vloada_halfn, vloada_half)
    _OCL_OP(OpenCLLIB::Vstoren, vstore)
    _OCL_OP(OpenCLLIB::Vstore_half, vstore_half)
    _OCL_OP(OpenCLLIB::Vstore_halfn, vstore_half)
    _OCL_OP(OpenCLLIB::Vstorea_halfn, vstorea_half)
    _OCL_OP(OpenCLLIB::Vstore_half_r, vstore_half)
    _OCL_OP(OpenCLLIB::Vstore_halfn_r, vstore_half)
    _OCL_OP(OpenCLLIB::Vstorea_halfn_r, vstorea_half)
#undef _OCL_OP
    default:
      SPIRV_LL_ABORT("Unhandled OpenCL builtin");
      break;
  }

  MangleInfo resMangleInfo = resTyId;

  // Some builtins have constraints on their operands
  switch (opcode) {
    default:
      break;
    case OpenCLLIB::Frexp:
    case OpenCLLIB::Ldexp:
    case OpenCLLIB::Lgamma_r:
    case OpenCLLIB::Pown:
    case OpenCLLIB::Remquo:
    case OpenCLLIB::Rootn:
      // int* and intN* pointer operands are signed
      argInfo.back().forceSign = MangleInfo::ForceSignInfo::ForceSigned;
      break;
    case OpenCLLIB::SAdd_sat:
    case OpenCLLIB::SHadd:
    case OpenCLLIB::SRhadd:
    case OpenCLLIB::SClamp:
    case OpenCLLIB::SMad_hi:
    case OpenCLLIB::SMad_sat:
    case OpenCLLIB::SMax:
    case OpenCLLIB::SMin:
    case OpenCLLIB::SMul_hi:
    case OpenCLLIB::SSub_sat:
    case OpenCLLIB::SMul24:
    case OpenCLLIB::SMad24:
      resMangleInfo = MangleInfo::getSigned(resTyId);
      for (auto &arg : argInfo) {
        arg.forceSign = MangleInfo::ForceSignInfo::ForceSigned;
      }
      break;
    case OpenCLLIB::SAbs:
    case OpenCLLIB::SAbs_diff:
      // Both abs and abs_diff always have an unsigned return type
      resMangleInfo = MangleInfo::getUnsigned(resTyId);
      for (auto &arg : argInfo) {
        arg.forceSign = MangleInfo::ForceSignInfo::ForceSigned;
      }
      break;
    case OpenCLLIB::S_Upsample:
      resMangleInfo = MangleInfo::getSigned(resTyId);
      argInfo[0].forceSign = MangleInfo::ForceSignInfo::ForceSigned;
      argInfo[1].forceSign = MangleInfo::ForceSignInfo::ForceUnsigned;
      break;
    case OpenCLLIB::Prefetch:
      argInfo[0].typeQuals = MangleInfo::CONST;
      break;
    case OpenCLLIB::UAbs:
    case OpenCLLIB::UAbs_diff:
    case OpenCLLIB::UAdd_sat:
    case OpenCLLIB::UHadd:
    case OpenCLLIB::URhadd:
    case OpenCLLIB::UClamp:
    case OpenCLLIB::UMad_hi:
    case OpenCLLIB::UMad_sat:
    case OpenCLLIB::UMax:
    case OpenCLLIB::UMin:
    case OpenCLLIB::UMul_hi:
    case OpenCLLIB::USub_sat:
    case OpenCLLIB::U_Upsample:
    case OpenCLLIB::UMad24:
    case OpenCLLIB::UMul24:
      resMangleInfo = MangleInfo::getUnsigned(resTyId);
      for (auto &arg : argInfo) {
        arg.forceSign = MangleInfo::ForceSignInfo::ForceUnsigned;
      }
      break;
    case OpenCLLIB::Vloadn:
    case OpenCLLIB::Vload_half:
    case OpenCLLIB::Vload_halfn:
    case OpenCLLIB::Vloada_halfn:
      argInfo[1].typeQuals = MangleInfo::CONST;
      break;
  }

  //  The vstore and vload builtins variously change the builtin name. Handle
  //  those here.
  switch (opcode) {
    default:
      break;
    case OpenCLLIB::Vloadn:
    case OpenCLLIB::Vload_halfn:
    case OpenCLLIB::Vloada_halfn:
      // The last argument is 'N' - the vector width. This is a literal integer
      // in the binary format, which is passed to us as a spv::Id!
      argVals.pop_back();
      fnName += std::to_string(argInfo.pop_back_val().id);
      break;
    case OpenCLLIB::Vstoren:
    case OpenCLLIB::Vstore_half:
    case OpenCLLIB::Vstore_halfn:
    case OpenCLLIB::Vstorea_halfn:
      // The first operand has the data type
      if (argVals[0]->getType()->isVectorTy()) {
        fnName += std::to_string(
            multi_llvm::getVectorNumElements(argVals[0]->getType()));
      }
      break;
    case OpenCLLIB::Vstore_half_r:
    case OpenCLLIB::Vstore_halfn_r:
    case OpenCLLIB::Vstorea_halfn_r:
      // The first operand has the data type
      if (argVals[0]->getType()->isVectorTy()) {
        fnName += std::to_string(
            multi_llvm::getVectorNumElements(argVals[0]->getType()));
      }
      // The last operand is the rounding mode; pop that off and mangle it into
      // the name. The mode is a spv::Id which is actually a literal of enum
      // type spv::FPRoundingMode.
      argVals.pop_back();
      fnName += getFPRoundingModeSuffix(argInfo.pop_back_val().id);
      break;
  }

  SPIRV_LL_ASSERT(
      llvm::all_of(argVals, [](const llvm::Value *v) { return v != nullptr; }),
      "Can't call with a null value");

  return createMangledBuiltinCall(fnName, resultType, resMangleInfo, argVals,
                                  argInfo);
}

llvm::CallInst *spirv_ll::Builder::createMangledBuiltinCall(
    llvm::StringRef name, llvm::Type *retTy, MangleInfo retMangleInfo,
    llvm::ArrayRef<llvm::Value *> args,
    llvm::ArrayRef<MangleInfo> argMangleInfo, bool convergent) {
  // Transform the MangleInfo from value-based IDs to type-based IDs. Take a
  // copy first!
  llvm::SmallVector<MangleInfo> argTyMangleInfo(argMangleInfo.begin(),
                                                argMangleInfo.end());
  llvm::for_each(argTyMangleInfo, [&](MangleInfo &info) {
    if (info.id) {
      // Transform the value id to its id.
      info.id = module.getResultType(info.id)->IdResult();
    }
  });
  auto mangledBuiltInCall = createBuiltinCall(
      getMangledFunctionName(name.str(), args, argTyMangleInfo), retTy, args,
      convergent);
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
      const llvm::Attribute::AttrKind attribute =
          retMangleInfo.getSignedness(module) ? llvm::Attribute::AttrKind::SExt
                                              : llvm::Attribute::AttrKind::ZExt;
      mangledBuiltInCall->addRetAttr(attribute);
      calledFunction->addRetAttr(attribute);
    }
  }

  // Setting the attributes for the function arguments.
  for (size_t argno = 0; argno < args.size(); argno++) {
    auto argTy = args[argno]->getType();
    if (argTy->isIntegerTy()) {
      // If the type is i8 or i16, it requires an attribute (signext or
      // zeroext). Vectors containing i8 or i16 do not require parameter
      // attributes.
      const auto bitWidth = llvm::cast<llvm::IntegerType>(argTy)->getBitWidth();
      if (bitWidth == 8 || bitWidth == 16) {
        // Assume signed unless an OpCode was provided that says otherwise.
        llvm::Attribute::AttrKind attribute = llvm::Attribute::AttrKind::SExt;
        if (!argTyMangleInfo.empty()) {
          attribute = argTyMangleInfo[argno].getSignedness(module)
                          ? llvm::Attribute::AttrKind::SExt
                          : llvm::Attribute::AttrKind::ZExt;
        }
        mangledBuiltInCall->addParamAttr(argno, attribute);
        calledFunction->addParamAttr(argno, attribute);
      }
    }
  }

  return mangledBuiltInCall;
}

std::string spirv_ll::Builder::getMangledFunctionName(
    std::string name, llvm::ArrayRef<llvm::Value *> args,
    llvm::ArrayRef<MangleInfo> argMangleInfo) {
  // prefix the length of the function name
  name = applyMangledLength(name);

  // list of argument types which can be used as substitutes
  llvm::SmallVector<SubstitutableType, 16> subTys;

  // get the mangled argument name for each argument
  for (size_t index = 0; index < args.size(); index++) {
    auto argTy = args[index]->getType();

    std::optional<MangleInfo> mangleInfo;
    if (!argMangleInfo.empty()) {
      mangleInfo = argMangleInfo[index];
    }

    // append the mangled argument type name
    name += getMangledTypeName(argTy, mangleInfo, subTys);

    if (isSubstitutableArgType(argTy)) {
      // argument type is substitutable: add it to the substitutable list
      subTys.push_back({argTy, index, mangleInfo});
      // FIXME: We can't substitute pointer types unless we have IDs
      if (argTy->isPointerTy() && mangleInfo && mangleInfo->id) {
        llvm::Type *pointeeTy = nullptr;
        auto *const spvPtrTy = module.get<OpType>(mangleInfo->id);
        if (spvPtrTy->isPointerType()) {
          pointeeTy = module.getLLVMType(spvPtrTy->getTypePointer()->Type());
#if LLVM_VERSION_LESS(17, 0)
        } else if (spvPtrTy->isImageType() || spvPtrTy->isEventType() ||
                   spvPtrTy->isSamplerType()) {
          pointeeTy = module.getInternalStructType(spvPtrTy->IdResult());
#endif
        }
        SPIRV_LL_ASSERT_PTR(pointeeTy);
        if (!pointeeTy->isIntegerTy() && !pointeeTy->isFloatingPointTy()) {
          // attempt to get the OpCode object for our element type, this is
          // basically so we can check signedness if element type is a vector of
          // ints
          std::optional<MangleInfo> pointeeMangleInfo;
          if (spvPtrTy->isPointerType()) {
            pointeeMangleInfo = *mangleInfo;
            pointeeMangleInfo->typeQuals = 0;
            pointeeMangleInfo->id = spvPtrTy->getTypePointer()->Type();
          }
          subTys.push_back({pointeeTy, index, pointeeMangleInfo});
        }
      }
    }
  }

  return name;
}

const spirv_ll::Builder::SubstitutableType *spirv_ll::Builder::substitutableArg(
    llvm::Type *ty, const llvm::ArrayRef<SubstitutableType> &subTys,
    std::optional<MangleInfo> mangleInfo) {
  for (const SubstitutableType &subTy : subTys) {
    if (ty != subTy.ty) {
      continue;
    }

    // if the types are vectors, makes sure that both are signed/unsigned
    if (mangleInfo && ty->isVectorTy() &&
        multi_llvm::getVectorElementType(ty)->isIntegerTy()) {
      const bool tySignedness = mangleInfo->getSignedness(module);
      uint32_t subTySignedness = 1;
      if (subTy.mangleInfo) {
        subTySignedness = subTy.mangleInfo->getSignedness(module);
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
  return nullptr;
}

std::string spirv_ll::Builder::getMangledTypeName(
    llvm::Type *ty, std::optional<MangleInfo> mangleInfo,
    llvm::ArrayRef<SubstitutableType> subTys) {
  auto subTyArg = substitutableArg(ty, subTys, mangleInfo);
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
    std::optional<bool> signedness;
    if (mangleInfo && mangleInfo->id) {
      auto *const spvTy = module.get<OpType>(mangleInfo->id);
      if (spvTy->isBoolType()) {
        return "b";
      } else if (spvTy->isSamplerType()) {
        return "11ocl_sampler";
      } else {
        signedness = mangleInfo->getSignedness(module);
      }
    }
    // Assume signed integer when no opcode is provided
    return getMangledIntName(ty, signedness.value_or(true));
  } else if (ty->isVectorTy()) {
    std::optional<MangleInfo> componentMangleInfo;
    if (mangleInfo) {
      componentMangleInfo = *mangleInfo;
      componentMangleInfo->id =
          module.get<OpType>(mangleInfo->id)->getTypeVector()->ComponentType();
    }
    auto elementTy = multi_llvm::getVectorElementType(ty);
    return getMangledVecPrefix(ty) +
           getMangledTypeName(elementTy, componentMangleInfo, subTys);
  } else if (ty->isPointerTy()) {
#if LLVM_VERSION_LESS(17, 0)
    SPIRV_LL_ASSERT(mangleInfo,
                    "Must supply OpType to mangle pointer arguments");
    if (auto *structTy = module.getInternalStructType(mangleInfo->id)) {
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
    }
#endif
    SPIRV_LL_ASSERT(
        mangleInfo && module.get<OpType>(mangleInfo->id)->isPointerType(),
        "Parameter is not a pointer");

    const auto spvPointeeTy =
        module.get<OpType>(mangleInfo->id)->getTypePointer()->Type();
    auto *const elementTy = module.getLLVMType(spvPointeeTy);
    const std::string mangled =
        getMangledPointerPrefix(ty, mangleInfo->typeQuals);
    auto pointeeMangleInfo = *mangleInfo;
    pointeeMangleInfo.typeQuals = 0;
    pointeeMangleInfo.id = spvPointeeTy;
    return mangled + getMangledTypeName(elementTy, pointeeMangleInfo, subTys);
  } else if (ty->isArrayTy()) {
    std::optional<MangleInfo> eltMangleInfo;
    if (mangleInfo) {
      eltMangleInfo = *mangleInfo;
      eltMangleInfo->id =
          module.get<OpType>(mangleInfo->id)->getTypeArray()->ElementType();
    }
    return "P" +
           getMangledTypeName(ty->getArrayElementType(), eltMangleInfo, subTys);
#if LLVM_VERSION_GREATER_EQUAL(17, 0)
  } else if (auto *tgtExtTy = llvm::dyn_cast<llvm::TargetExtType>(ty)) {
    auto tyName = tgtExtTy->getName();
    if (tyName == "spirv.Event") {
      return "9ocl_event";
    }
    if (tyName == "spirv.Sampler") {
      return "11ocl_sampler";
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
          return arrayed ? "16ocl_image1darray" : "11ocl_image1d";
        case compiler::utils::tgtext::ImageDim2D:
          return arrayed ? "16ocl_image2darray" : "11ocl_image2d";
        case compiler::utils::tgtext::ImageDim3D:
          return "11ocl_image3d";
        case compiler::utils::tgtext::ImageDimBuffer:
          return "17ocl_image1dbuffer";
      }
    }
#endif
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

  // If the size of indexes is only one, this means we are not indexing into the
  // struct itself so exit in this case. This can happen for example if we are
  // using something like OpPtrAccessChain without an empty indexes input field.
  if (indexes.size() < 2) {
    return;
  }

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
      case llvm::Type::FixedVectorTyID:
        nextType = multi_llvm::getVectorElementType(traversed.back());
        break;
      case llvm::Type::PointerTyID: {
        auto *opTy = module.getFromLLVMTy<OpType>(traversed.back());
        SPIRV_LL_ASSERT(opTy && opTy->isPointerType(), "Type is not a pointer");
        nextType = module.getLLVMType(opTy->getTypePointer()->Type());
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
  const uint32_t member =
      cast<llvm::ConstantInt>(indexes[memberIndex])->getZExtValue();
  auto structType = module.getFromLLVMTy<OpTypeStruct>(accessedStructType);
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

  llvm::BasicBlock *firstBasicBlock = &function->front();

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
        llvm::Type *type = module.getLLVMType(op->IdResultType());
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
        llvm::Type *type = module.getLLVMType(op->IdResultType());
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
  if (operand->getType()->getTypeID() == llvm::Type::FixedVectorTyID) {
    return llvm::FixedVectorType::get(
        IRBuilder.getIntNTy(operand->getType()->getScalarSizeInBits()),
        multi_llvm::getVectorNumElements(operand->getType()));
  } else {
    return IRBuilder.getInt32Ty();
  }
}

bool spirv_ll::MangleInfo::getSignedness(const spirv_ll::Module &module) const {
  if (!id) {
    return true;
  }
  std::optional<bool> tySignedness;
  auto *const spvTy = module.get<OpType>(id);
  if (spvTy->isIntType()) {
    tySignedness = spvTy->getTypeInt()->Signedness();
  } else if (spvTy->isVectorType()) {
    auto *const spvVecEltTy =
        module.get<OpType>(spvTy->getTypeVector()->ComponentType());
    if (spvVecEltTy->isIntType()) {
      tySignedness = spvVecEltTy->getTypeInt()->Signedness();
    }
  }
  switch (forceSign) {
    default:
      break;
    case ForceSignInfo::ForceSigned:
      tySignedness = true;
      break;
    case ForceSignInfo::ForceUnsigned:
      tySignedness = false;
      break;
  }
  // The default is signed
  return tySignedness.value_or(true);
}

std::string spirv_ll::getIntTypeName(llvm::Type *ty, bool isSigned) {
  auto elemTy = ty->isVectorTy() ? multi_llvm::getVectorElementType(ty) : ty;
  SPIRV_LL_ASSERT(elemTy->isIntegerTy(), "not an integer type");
  std::string name;
  switch (elemTy->getIntegerBitWidth()) {
    case 1:
      name = isSigned ? "bool" : "ubool";
      break;
    case 8:
      name = isSigned ? "char" : "uchar";
      break;
    case 16:
      name = isSigned ? "short" : "ushort";
      break;
    case 32:
      name = isSigned ? "int" : "uint";
      break;
    case 64:
      name = isSigned ? "long" : "ulong";
      break;
    default:
      // Arbitrary precision integer
      name = (isSigned ? "" : "u") + std::string("int") +
             std::to_string(elemTy->getIntegerBitWidth()) + "_t";
  }
  if (ty->isVectorTy()) {
    const uint32_t numElements = multi_llvm::getVectorNumElements(ty);
    name += std::to_string(numElements);
  }
  return name;
}

std::string spirv_ll::getFPTypeName(llvm::Type *ty) {
  auto elemTy = ty->isVectorTy() ? multi_llvm::getVectorElementType(ty) : ty;
  SPIRV_LL_ASSERT(elemTy->isFloatingPointTy(), "not a floating point type");
  std::string name;
  switch (elemTy->getScalarSizeInBits()) {
    case 16:
      name = "half";
      break;
    case 32:
      name = "float";
      break;
    case 64:
      name = "double";
      break;
    default:
      llvm_unreachable("unsupported floating point bit width");
  }
  if (ty->isVectorTy()) {
    const uint32_t numElements = multi_llvm::getVectorNumElements(ty);
    name += std::to_string(numElements);
  }
  return name;
}
