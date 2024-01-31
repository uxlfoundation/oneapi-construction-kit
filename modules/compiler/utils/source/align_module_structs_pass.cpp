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

#include <compiler/utils/align_module_structs_pass.h>
#include <compiler/utils/attributes.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/pass_functions.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueMap.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <multi_llvm/llvm_version.h>

#include <cassert>

using namespace llvm;

namespace {
using ReplacementStructSP =
    compiler::utils::AlignModuleStructsPass::ReplacementStructSP;
using StructReplacementMap =
    compiler::utils::AlignModuleStructsPass::StructReplacementMap;

/// @brief Generate an alternative type for a given input type using old struct
///
/// A LLVM type may indirectly reference an old struct type via a pointer or
/// array over multiple layers. We want to create an alternative variant of
/// such a type which has the same layers but ultimately references a new
/// struct type instead.
///
/// @param[in] type Type we want to find new padded type for
/// @param[in] map Mapping of old struct types to new struct types
///
/// @return Alternative type if one could be found, null otherwise
Type *getNewType(Type *type, const StructReplacementMap &map) {
  // Use recursion to build up new types for arrays and pointers.
  if (type->isPointerTy()) {
    // We can't remap pointer types, and it doesn't really make sense, assuming
    // pointer address spaces will never change, anyway.
    return nullptr;
  }

  if (ArrayType *arrTy = dyn_cast<ArrayType>(type)) {
    Type *newType = getNewType(arrTy->getElementType(), map);
    if (!newType) {
      return nullptr;
    }
    return ArrayType::get(newType, arrTy->getNumElements());
  }

  // 'type' is an old struct type, return the matching padded struct
  if (const ReplacementStructSP newStructDetails = map.lookup(type)) {
    return newStructDetails->newStructType;
  }

  // vector types are also a composite type, so control flow could slip through
  // the initial check and reach here.
  return nullptr;
}

/// @brief Implementation of ValueMapTypeRemapper
///
/// A pointer to this pass is passed in to CloneFunctionInto() for
/// changing types when cloning instructions.
class StructTypeRemapper final : public ValueMapTypeRemapper {
 public:
  /// @brief Callback called when remapping values
  /// @param[in] srcType Current type of Value being cloned
  /// @return Alternative type if one could be found, existing type otherwise.
  Type *remapType(Type *srcType) override {
    Type *newStructType = getNewType(srcType, map);
    if (!newStructType) {
      return srcType;
    } else {
      return newStructType;
    }
  }

  /// @brief Constructor taking a struct map
  StructTypeRemapper(const StructReplacementMap &m) : map(m) {}

 private:
  /// @brief Reference to map between old and new structs
  const StructReplacementMap &map;
};

/// @brief Creates replacement initializer for global variable
///
/// @param[in] oldInit Current initializer for old struct type
/// @param[in] typeMap Mapping of old to new struct types
/// @return New constant initializer for updated type
Constant *createInitializer(Constant *oldInit,
                            const StructReplacementMap &typeMap) {
  auto *initType = getNewType(oldInit->getType(), typeMap);
  // Default initializer to undef
  Constant *newInit = UndefValue::get(initType);

  // If the global is a struct type with a constant initializer we need
  // to create a new initializer matching our updated struct type. We
  // can just use null values for new padding members.
  if (auto *oldStruct = dyn_cast<ConstantStruct>(oldInit)) {
    auto *oldStructT = oldStruct->getType();
    auto *structT = cast<StructType>(initType);
    SmallVector<Constant *, 4> newConsts;

    // Get the mapping of old indices into the new struct
    const DenseMap<unsigned int, unsigned int> &indexMap =
        typeMap.lookup(oldStructT)->memberIndexMap;

    // Default all members to null, then go through can copy over the
    // relevant constants from the old initializer using our index mapping
    for (unsigned int i = 0; i < structT->getNumElements(); ++i) {
      auto *elemType = structT->getElementType(i);
      newConsts.push_back(UndefValue::get(elemType));
    }

    for (unsigned int i = 0; i < oldStructT->getNumElements(); ++i) {
      auto *operand = oldStruct->getOperand(i);
      assert(nullptr != operand && "Could not get operand");
      auto *mappedType = getNewType(operand->getType(), typeMap);
      if (mappedType) {
        // Recursive call to get initializer for padded member
        operand = createInitializer(operand, typeMap);
      }
      const unsigned newI = indexMap.lookup(i);
      newConsts[newI] = operand;
    }
    // Set new initializer
    newInit = ConstantStruct::get(structT, newConsts);
  }

  return newInit;
}

/// @brief Creates replacement global variables for those with struct types
///
/// Identifies global variables with struct types we have a padded alternative
/// for. Then generates a new global variable with matching padded type and
/// adds it to the value map to replace the uses later in the clone.
///
/// @param[in] global GlobalVariable to replace
/// @param[in] typeMap Mapping of old to new struct types
/// @param[in,out] valMap Mapping of values we want to replace in the clone
void replaceGlobalVariable(GlobalVariable *global,
                           const StructReplacementMap &typeMap,
                           ValueToValueMapTy &valMap) {
  // Global variable initializer
  Constant *initalizer = createInitializer(global->getInitializer(), typeMap);

  // Create new global, copy over the attributes  and add it to value map
  auto *newType = getNewType(global->getValueType(), typeMap);
  assert(nullptr != newType && "Could not get type");
  GlobalVariable *glob = new GlobalVariable(
      *global->getParent(), newType, global->isConstant(), global->getLinkage(),
      initalizer, global->getName(), nullptr, GlobalValue::NotThreadLocal,
      global->getType()->getPointerAddressSpace(),
      global->isExternallyInitialized());
  glob->copyAttributesFrom(global);
  glob->copyMetadata(global, 0);
  valMap.insert({global, glob});

  // We need to manually update GEP instruction indices later. These
  // can be constant so we can find them easily when iterating through
  // instructions. This is the reason we can't use a Materializer here for
  // replacing globals.
  SmallVector<User *, 8> users;
  users.append(global->users().begin(), global->users().end());

  for (auto *user : users) {
    if (auto *constant = dyn_cast<ConstantExpr>(user)) {
      compiler::utils::replaceConstantExpressionWithInstruction(constant);
    }
  }
}

/// @brief Update GEP instruction indices to match new struct type
///
/// We need to update GEP indices since any new padding members created for our
/// struct mean the original indices used the GEP will no longer point to the
/// intended target.
///
/// @param[in] gepInst GetElementPtr instruction to update
/// @param[in] typeMap Mapping of old to new struct types
void fixupGepIndices(GetElementPtrInst *gepInst,
                     const StructReplacementMap &typeMap) {
  // We push new indices to these vectors to walk the indirections looking for
  // structs we need to update the indices for
  SmallVector<Value *, 4> newIndices{gepInst->getOperand(1)};
  SmallVector<Value *, 4> oldIndices{gepInst->getOperand(1)};
  Type *sourceTy = gepInst->getSourceElementType();

  const unsigned numIndices = gepInst->getNumIndices();
  while (newIndices.size() < numIndices) {
    // Find type referenced by current indices
    Type *indexedTy = GetElementPtrInst::getIndexedType(sourceTy, oldIndices);
    assert(indexedTy && "Error calculating new GEP indices");

    Value *nextIdx = gepInst->getOperand(newIndices.size() + 1);
    Value *newIdx = nextIdx;
    if (const ReplacementStructSP newStructDetails =
            typeMap.lookup(indexedTy)) {
      // Check the mapping of indices from old to new structs
      const DenseMap<unsigned int, unsigned int> &indexMap =
          newStructDetails->memberIndexMap;

      ConstantInt *constIndex = cast<ConstantInt>(nextIdx);
      const auto newIndex = indexMap.lookup(constIndex->getZExtValue());

      // Create index for new struct type
      IRBuilder<> ir(gepInst);
      newIdx = ir.getIntN(constIndex->getBitWidth(), newIndex);
    }
    newIndices.push_back(newIdx);
    oldIndices.push_back(nextIdx);
  }

  // Update indices in place, adjust for first operand being the pointer
  for (unsigned i = 0u; i < numIndices; ++i) {
    gepInst->setOperand(i + 1, newIndices[i]);
  }
}

/// @brief Clones function, updating references to struct types
///
/// If an old struct type is found in the function signature, then a clone
/// of the function is made into a new function with a signature using new
/// alternative types.
///
/// @param[in] func Function to check signature of
/// @param[in] typeMap Mapping of old to new struct types
/// @param[in,out] valMap Mapping of values we want to replace in the clone
/// @return cloned function which has been generated
Function *cloneFunctionUpdatingTypes(Function &func,
                                     const StructReplacementMap &typeMap,
                                     ValueToValueMapTy &valMap) {
  const auto *funcTy = func.getFunctionType();

  // Check function return type
  Type *returnTy = funcTy->getReturnType();
  if (Type *newReturn = getNewType(returnTy, typeMap)) {
    returnTy = newReturn;
  }

  // Look through function arguments
  SmallVector<Type *, 4> argTypes;
  for (auto *arg : funcTy->params()) {
    if (Type *newArg = getNewType(arg, typeMap)) {
      argTypes.push_back(newArg);
    } else {
      argTypes.push_back(arg);
    }
  }

  // Create a new function signature
  auto *newFuncTy = FunctionType::get(returnTy, argTypes, funcTy->isVarArg());
  Function *newFunc =
      Function::Create(newFuncTy, func.getLinkage(), "", func.getParent());

  // Take attributes of old function
  newFunc->takeName(&func);
#if LLVM_VERSION_GREATER_EQUAL(18, 0)
  newFunc->updateAfterNameChange();
#else
  newFunc->recalculateIntrinsicID();
#endif
  newFunc->setCallingConv(func.getCallingConv());

  assert(func.isIntrinsic() == newFunc->isIntrinsic() &&
         "Lost intrinsic while remapping");

  if (func.isDeclaration()) {
    return newFunc;
  }

  // Map all original function arguments to the new function arguments
  auto newArgIterator = newFunc->arg_begin();
  for (Argument &arg : func.args()) {
    newArgIterator->setName(arg.getName());
    valMap[&arg] = &*(newArgIterator);
    newArgIterator++;
  }

  // Setup out struct type mapping callback
  StructTypeRemapper structMapper(typeMap);

  // Clone old function into new function
  SmallVector<ReturnInst *, 2> returns;

  const bool moduleLevelChanges =
      compiler::utils::funcContainsDebugMetadata(func, valMap);
  CloneFunctionChangeType changes;
  if (func.getParent() != newFunc->getParent()) {
    changes = CloneFunctionChangeType::DifferentModule;
  } else if (moduleLevelChanges) {
    changes = CloneFunctionChangeType::GlobalChanges;
  } else {
    changes = CloneFunctionChangeType::LocalChangesOnly;
  }
  CloneFunctionInto(newFunc, &func, valMap, changes, returns, "", nullptr,
                    &structMapper);

  // update the kernel metadata
  compiler::utils::replaceKernelInOpenCLKernelsMetadata(func, *newFunc,
                                                        *func.getParent());

  // take kernel-specific data from the old function.
  compiler::utils::takeIsKernel(*newFunc, func);

  // Check for ByVal parameter attributes that reference old struct types and
  // update them to reference the new struct types instead. In other words aim
  // to find and replace `%struct.new* byval(%struct.old) %foo` with
  // `%struct.new* byval(%struct.new) %foo`
  for (uint32_t argIndex = 0; argIndex < newFunc->arg_size(); argIndex++) {
    if (newFunc->hasParamAttribute(argIndex, Attribute::ByVal)) {
      Type *oldByValType = newFunc->getParamByValType(argIndex);
      Type *newByValType = getNewType(oldByValType, typeMap);
      if (newByValType) {
        auto attributeBuilder = llvm::AttrBuilder(newFunc->getContext());
        attributeBuilder.addByValAttr(newByValType);

        AttributeList attributeList = newFunc->getAttributes();
        attributeList = attributeList.removeParamAttribute(
            newFunc->getContext(), argIndex, Attribute::ByVal);
        attributeList = attributeList.addParamAttributes(
            newFunc->getContext(), argIndex, attributeBuilder);
        newFunc->setAttributes(attributeList);
      }
    }
  }

  // Remove instructions from old function
  func.deleteBody();
  return newFunc;
}

// Returns the newType of the Value, returning the new type of the
// GlobalVariable if it is one.
Type *getNewType(Value *v, const StructReplacementMap &typeMap) {
  if (auto *glob = dyn_cast<GlobalVariable>(v)) {
    return getNewType(glob->getValueType(), typeMap);
  }
  return getNewType(v->getType(), typeMap);
}

/// @brief Performs the update on the LLVM module to replace all the Values
///        using the old struct type with Values using our padded variant.
///
/// @param[in] typeMap Mapping of old to new struct types
/// @param[in] module Module to replace Values in
void replaceModuleTypes(const StructReplacementMap &typeMap, Module &module) {
  // Mapping of old Values to new ones
  ValueToValueMapTy valMap;

  // Find global variables referencing the old struct types
  SmallPtrSet<GlobalVariable *, 4> globals;
  for (auto &global : module.globals()) {
    if (getNewType(global.getValueType(), typeMap)) {
      globals.insert(&global);
    }
  }

  // Replace globals with new variants using updated padded struct types.
  for (auto *global : globals) {
    replaceGlobalVariable(global, typeMap, valMap);
  }

  // Identify all functions which use a struct type and need to be cloned,
  // this is to avoid unnecessary work in the clone.
  SmallVector<Function *, 4> funcs;
  for (auto &func : module.functions()) {
    bool usesStructType = false;
    for (BasicBlock &block : func) {
      Type *newType = nullptr;
      // Check each instruction carefully. With opaque pointers, we can't
      // easily catch everything using just the operands' types:
      //   store i8 0, ptr @glob
      // Here, the store's pointer operand is a struct type we need to remap,
      // but the type is just an opaque pointer we can't look through. Thus we
      // have to explicitly check certain instructions for globals.
      for (Instruction &inst : block) {
        if (auto *alloca = dyn_cast<AllocaInst>(&inst)) {
          newType = getNewType(alloca->getAllocatedType(), typeMap);
        } else if (auto *GEP = dyn_cast<GetElementPtrInst>(&inst)) {
          if (!(newType = getNewType(GEP->getSourceElementType(), typeMap))) {
            newType = getNewType(GEP->getPointerOperand(), typeMap);
          }
        } else if (auto *cast = dyn_cast<CastInst>(&inst)) {
          newType = getNewType(cast->getOperand(0), typeMap);
        } else if (auto *load = dyn_cast<LoadInst>(&inst)) {
          if (!(newType = getNewType(inst.getType(), typeMap))) {
            newType = getNewType(load->getPointerOperand(), typeMap);
          }
        } else if (auto *store = dyn_cast<StoreInst>(&inst)) {
          if (!(newType = getNewType(store->getValueOperand(), typeMap))) {
            newType = getNewType(store->getPointerOperand(), typeMap);
          }
        } else if (auto *cmpxchg = dyn_cast<AtomicCmpXchgInst>(&inst)) {
          newType = getNewType(cmpxchg->getPointerOperand(), typeMap);
        } else if (auto *atomicrmw = dyn_cast<AtomicRMWInst>(&inst)) {
          newType = getNewType(atomicrmw->getPointerOperand(), typeMap);
        } else if (auto *sel = dyn_cast<SelectInst>(&inst)) {
          if (!(newType = getNewType(sel->getTrueValue(), typeMap))) {
            newType = getNewType(sel->getFalseValue(), typeMap);
          }
        } else if (auto *phi = dyn_cast<PHINode>(&inst)) {
          for (auto &op : phi->incoming_values()) {
            if ((newType = getNewType(op, typeMap))) {
              break;
            }
          }
        } else if (auto *call = dyn_cast<CallInst>(&inst)) {
          for (auto &op : call->operands()) {
            if ((newType = getNewType(op, typeMap))) {
              break;
            }
          }
        } else {
          newType = getNewType(inst.getType(), typeMap);
        }
        if (newType) {
          usesStructType = true;
        }
      }
      // Avoid unnecessary work
      if (newType) {
        usesStructType = true;
      }
    }

    if (usesStructType) {
      funcs.push_back(&func);
      continue;
    }

    for (const Argument &arg : func.args()) {
      if (nullptr != getNewType(arg.getType(), typeMap)) {
        funcs.push_back(&func);
        break;
      }
    }
  }

  // Create cloned functions using our padded struct types.
  ValueMap<Function *, Function *> newToOldMap;
  for (auto *func : funcs) {
    // We need to fixup the indices for GEP instructions because we've added
    // new members for padding.
    for (BasicBlock &block : *func) {
      for (Instruction &inst : block) {
        if (auto *GEP = dyn_cast<GetElementPtrInst>(&inst)) {
          fixupGepIndices(GEP, typeMap);
        }
      }
    }

    Function *newFunc = cloneFunctionUpdatingTypes(*func, typeMap, valMap);
    newToOldMap[newFunc] = func;
  }

  for (const auto &pair : newToOldMap) {
    Function &newFunc = *pair.first;
    Function &oldFunc = *pair.second;
    compiler::utils::remapClonedCallsites(oldFunc, newFunc, false);
    oldFunc.eraseFromParent();
  }

  // Erase globals we've replaced
  for (auto *g : globals) {
    g->eraseFromParent();
  }
}
}  // namespace

PreservedAnalyses compiler::utils::AlignModuleStructsPass::run(
    Module &module, ModuleAnalysisManager &) {
  // Find all struct types which are user defined
  SmallVector<StructType *, 2> structTypes;

  // Identify structs in the module which need padding or reference a struct
  // type which needs padded. This excludes opaque structs since they don't
  // have any members yet
  for (auto *structTy : module.getIdentifiedStructTypes()) {
    if (!structTy->isOpaque()) {
      structTypes.push_back(structTy);
    }
  }

  // No structs were found, module not modified
  if (structTypes.empty()) {
    return PreservedAnalyses::all();
  }

  // Create a new struct type for each of the struct types identified.
  for (auto *structTy : structTypes) {
    // We still have to update the types of packed structs if they contain
    // a member struct which does need padded. This case is rare however, so
    // provide a quick out here by skipping packed structs without struct
    // members.
    if (structTy->isPacked()) {
      const bool hasStructMembers =
          std::any_of(structTy->elements().begin(), structTy->elements().end(),
                      [](Type *type) {
                        while (type->isArrayTy()) {
                          type = type->getArrayElementType();
                        }

                        return type->isStructTy();
                      });
      if (!hasStructMembers) {
        continue;
      }
    }
    generateNewStructType(structTy, module);
  }

  // Update structs members referencing other struct types
  fixupStructReferences();

  // Update instructions to use new padded struct types
  replaceModuleTypes(originalStructMap, module);

  PreservedAnalyses PA;
  PA.preserveSet<CFGAnalyses>();

  return PA;
}

static constexpr const char SPIR32DLStart[] = "e-p:32:32:32-";
static constexpr const char SPIR64DLStart[] = "e-p:64:64:64-";
// The shared common suffix across both SPIR data layouts. This was taken
// directly from the SPIR 1.2 specification. It's a little verbose as most of
// the vector specifiers are identical to LLVM's defaults, but being explicit
// is probably safest here.
static constexpr const char SPIRDLSuffix[] =
    "i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-"
    "v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:"
    "256-v256:256:256-v512:512:512-v1024:1024:1024";

void compiler::utils::AlignModuleStructsPass::generateNewStructType(
    StructType *unpadded, const Module &M) {
  LLVMContext &Ctx = M.getContext();
  SmallVector<Type *, 8> structElementTypes;
  DenseMap<unsigned int, unsigned int> indexMap;
  // Calculate OpenCL alignment using LLVM data layout APIs. Depending on
  // target this may require some coercion to meet OpenCL requirements.
  const DataLayout &DL = M.getDataLayout();
  // This is a bit of intuitition about which SPIR ABI we were originally
  // compiling for. Since we don't support compiling for architectures with a
  // different pointer size to that for which the IR was originally produced,
  // we can infer it from the current data layout.
  assert((DL.getPointerSizeInBits() == 32 || DL.getPointerSizeInBits() == 64) &&
         "Only support compilation for 32-bit or 64-bit targets");
  const DataLayout SPIRDL(std::string(DL.getPointerSizeInBits() == 32
                                          ? SPIR32DLStart
                                          : SPIR64DLStart) +
                          SPIRDLSuffix);

  bool changed = false;
  uint64_t cumulativePadding = 0;
  auto *const thisLayout = DL.getStructLayout(unpadded);

  for (unsigned i = 0, e = unpadded->getNumElements(); i != e; i++) {
    Type *const memberType = unpadded->getElementType(i);
    // Packed structs can contain members which are structs we've padded,
    // so we still need to replace their members in this function, but don't
    // do the padding here.
    if (!unpadded->isPacked()) {
      const uint64_t thisDLEltOffset =
          thisLayout->getElementOffset(i) + cumulativePadding;

      const Align reqdTyAlign = SPIRDL.getABITypeAlign(memberType);

      if (!isAligned(reqdTyAlign, thisDLEltOffset)) {
        // Calculate number of padding bytes
        const unsigned int reqdPadding =
            offsetToAlignment(thisDLEltOffset, reqdTyAlign);

        // Use a byte array to pad struct rather than trying to create an
        // arbitrary intNTy, since this may not be supported by the backend.
        auto *const padByteType = Type::getInt8Ty(Ctx);
        auto *const padByteArrayType = ArrayType::get(padByteType, reqdPadding);

        changed = true;
        cumulativePadding += reqdPadding;
        structElementTypes.push_back(padByteArrayType);
      }
    }

    // Record mapping of index of member with new index
    indexMap.insert({i, structElementTypes.size()});

    // Add a padded type to struct if appropriate
    Type *const updatedMemberType = getNewType(memberType, originalStructMap);
    Type *const paddedType = updatedMemberType ? updatedMemberType : memberType;
    changed |= updatedMemberType != nullptr;
    structElementTypes.push_back(paddedType);
  }

  // If there's nothing in this type that needs padding or aligning, we don't
  // need to generate a new struct type.
  if (!changed) {
    return;
  }

  // Create an opaque struct type and wrap all the details we've computed
  // into a ReplacementStructDetails object
  auto *const paddedStructTy = StructType::create(Ctx, unpadded->getName());
  const ReplacementStructSP structSP(new ReplacementStructDetails(
      paddedStructTy, indexMap, structElementTypes));
  if (structSP) {
    originalStructMap.insert(std::make_pair(unpadded, structSP));
  }
}

void compiler::utils::AlignModuleStructsPass::fixupStructReferences() {
  // Iterate over all the structs in our map
  for (auto &mapItr : originalStructMap) {
    // Get the padding struct type and iterate over members
    const ReplacementStructSP paddedStructDetails = mapItr.getSecond();
    if (!paddedStructDetails) {
      continue;
    }

    // Members for our padded struct
    SmallVector<Type *, 8> newElements;
    for (Type *memberType : paddedStructDetails->bodyElements) {
      Type *indirectTy = getNewType(memberType, originalStructMap);
      Type *newType = indirectTy ? indirectTy : memberType;
      newElements.push_back(newType);
    }
    // Set the body of struct
    auto *original = cast<StructType>(mapItr.getFirst());
    paddedStructDetails->newStructType->setBody(newElements,
                                                original->isPacked());
  }
}
