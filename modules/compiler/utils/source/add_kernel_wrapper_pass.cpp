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

#include <compiler/utils/add_kernel_wrapper_pass.h>
#include <compiler/utils/address_spaces.h>
#include <compiler/utils/attributes.h>
#include <compiler/utils/builtin_info.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/pass_functions.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>

using namespace llvm;

namespace {

bool isArgLocalBuffer(Type *const Ty) {
  return Ty->isPointerTy() &&
         Ty->getPointerAddressSpace() == compiler::utils::AddressSpace::Local;
}

}  // namespace

void compiler::utils::AddKernelWrapperPass::createNewFunctionArgTypes(
    Module &M, const Function &F,
    const SmallVectorImpl<compiler::utils::BuiltinInfo::SchedParamInfo>
        &schedParamInfo,
    StructType *structTy, SmallVectorImpl<KernelArgMapping> &argMappings,
    SmallVectorImpl<Type *> &argTypes) {
  // the first element is our new packed argument struct
  argTypes.push_back(structTy->getPointerTo());

  uint32_t index = 0;
  // Track which arguments are *not* packed, and which index each corresponds
  // to in the new wrapped function. Start off at one as we've pushed a packed
  // argument structure.
  uint32_t unpackedArgIndex = 1;
  SmallVector<Type *, 8> packedArgTypes;

  for (const auto &arg : F.args()) {
    auto *const type = arg.getType();
    KernelArgMapping argMapping;
    argMapping.OldArgIdx = arg.getArgNo();
    if (auto idx = compiler::utils::isSchedulingParameter(F, arg.getArgNo())) {
      // We found one of our scheduling parameter types. These may or may not
      // form part of the external kernel interface.
      // FIXME: If the scheduling parameter index is out of bounds, assume we
      // pass it externally. This suits the host target, which has
      // Must_schedule_info_s, which is a semi-scheduling parameter but doesn't
      // participate in builtin resolution.
      argMapping.SchedParamIdx = *idx;
      if (*idx >= schedParamInfo.size() ||
          schedParamInfo[*idx].PassedExternally) {
        argTypes.push_back(type);
        argMapping.NewArgIdx = unpackedArgIndex++;
      }
      argMappings.push_back(argMapping);
      continue;
    }

    Type *addType = nullptr;
    if (PassLocalBuffersBySize && isArgLocalBuffer(type)) {
      // Local scratch buffers are passed as a size_t, and this pass allocates
      // the memory required for it
      addType = compiler::utils::getSizeType(M);
    } else if (arg.hasByValAttr()) {
      addType = arg.getParamByValType();
    } else {
      addType = type;
    }

    assert(addType && "Unknown argument type to pack");
    // If it's not packed we align to rounded up power of 2 (size)
    // However this may not always work so add padding as needed
    // we never go beyond 128
    if (!IsPacked) {
      const auto &datalayout = M.getDataLayout();
      const uint64_t size =
          std::max((uint64_t)1,
                   (uint64_t)datalayout.getTypeAllocSizeInBits(addType) / 8);
      const uint32_t alignSize = std::min((uint64_t)128, PowerOf2Ceil(size));
      const uint32_t remainder = index % alignSize;

      if (remainder) {
        // Add padding
        const unsigned int padding = alignSize - remainder;
        // Use a byte array to pad struct rather than trying to create
        // an arbitrary intNTy, since this may not be supported by the
        // backend.
        auto *const padByteType = Type::getInt8Ty(M.getContext());
        auto *const padByteArrayType = ArrayType::get(padByteType, padding);
        packedArgTypes.push_back(padByteArrayType);
        index += padding;
      }
      index += size;
    }
    argMapping.NewArgIdx = 0;
    argMapping.PackedStructFieldIdx = packedArgTypes.size();
    argMappings.push_back(argMapping);
    packedArgTypes.push_back(addType);
  }

  structTy->setBody(packedArgTypes, /*IsPacked*/ IsPacked);
}

PreservedAnalyses compiler::utils::AddKernelWrapperPass::run(
    Module &M, ModuleAnalysisManager &AM) {
  bool Changed = false;
  SmallPtrSet<Function *, 4> NewKernels;
  auto &DL = M.getDataLayout();
  auto &BI = AM.getResult<BuiltinInfoAnalysis>(M);

  const auto &schedParamInfo = BI.getMuxSchedulingParameters(M);

  for (auto &F : M.functions()) {
    // We only operate on previously-unseen kernel functions.
    if (!isKernel(F) || NewKernels.count(&F)) {
      continue;
    }

    // Previously we assumed that the first `name` was the main kernel
    // however this could lead to issues where multiple kernels were
    // declared in the source. As such we now turn our function arguments
    // into a packed pointer struct for every `name` in the list.
    SmallVector<Type *, 4> argTypes;
    SmallVector<KernelArgMapping, 4> argMappings;

    const std::string packedArgsTyName =
        (StringRef("MuxPackedArgs.") + getBaseFnNameOrFnName(F)).str();
    auto *const structType =
        StructType::create(F.getContext(), packedArgsTyName);
    createNewFunctionArgTypes(M, F, schedParamInfo, structType, argMappings,
                              argTypes);
    // create our new function
    Function *const newFunction =
        createKernelWrapperFunction(M, F, argTypes, ".mux-kernel-wrapper");

    // Reconstruct the scheduled function parameter metadata at this point.
    // This is conservative but correct.
    SmallVector<int> newSchedIndices(schedParamInfo.size(), -1);
    for (auto &info : argMappings) {
      if (info.SchedParamIdx >= 0 &&
          static_cast<unsigned>(info.SchedParamIdx) < newSchedIndices.size()) {
        newSchedIndices[info.SchedParamIdx] = info.NewArgIdx;
      }
    }

    setSchedulingParameterFunctionMetadata(*newFunction, newSchedIndices);

    // create an IR builder with a single basic block in our function
    IRBuilder<> ir(
        BasicBlock::Create(newFunction->getContext(), "", newFunction));

    auto *packedArgPtr = newFunction->getArg(0);
    packedArgPtr->setName("packed-args");
    // Add some helpful attributes to this argument.
    // FIXME: Could we also mandate alignment? Can we guarantee noalias on the
    // packed argument structure but not on the pointers it contains?
    // If there are no kernel arguments to pack, we don't require the runtime
    // to pass a valid pointer: it could be null.
    if (!structType->isEmptyTy()) {
      // It is invalid for a Mux runtime to pass a null or undef packed argument
      // struct.
      packedArgPtr->addAttr(Attribute::NoUndef);
      packedArgPtr->addAttr(Attribute::NonNull);
      // The packed argument struct must be fully dereferenceable.
      packedArgPtr->addAttr(Attribute::getWithDereferenceableBytes(
          newFunction->getContext(), DL.getTypeAllocSize(structType)));
    }

    assert(packedArgPtr->getType()->isPointerTy() &&
           "First argument should be pointer to the packed args structure");

    SmallVector<Value *, 8> params;
    const auto &oldAttrs = F.getAttributes();

    for (const auto &argMapping : argMappings) {
      assert(argMapping.OldArgIdx < F.arg_size());
      const Argument &arg = *F.getArg(argMapping.OldArgIdx);
      Type *const type = arg.getType();

      // Copy over parameter names and attributes from directly-mapped mapped
      // arguments - don't copy parameters from arguments mapped to the packed
      // argument struct.
      if (argMapping.NewArgIdx > 0) {
        auto *newArg = newFunction->getArg(argMapping.NewArgIdx);
        newArg->setName(arg.getName());
        AttrBuilder AB(M.getContext(),
                       oldAttrs.getParamAttrs(argMapping.OldArgIdx));
        newArg->addAttrs(AB);
      }

      if (argMapping.SchedParamIdx >= 0) {
        if (argMapping.NewArgIdx >= 0) {
          // Skip whichever scheduling parameters are passed arguments
          params.push_back(newFunction->getArg(argMapping.NewArgIdx));
        } else {
          // Initialize those that require it.
          const auto &info = schedParamInfo[argMapping.SchedParamIdx];
          auto *initializedParam = BI.initializeSchedulingParamForWrappedKernel(
              info, ir, *newFunction, F);
          assert(initializedParam && "Missing parameter initialization");
          params.push_back(initializedParam);
        }
        continue;
      }

      // Else, this must be a packed argument. Unpack it from the packed
      // argument parameter.
      assert(argMapping.PackedStructFieldIdx >= 0 &&
             static_cast<unsigned>(argMapping.PackedStructFieldIdx) <
                 structType->getStructNumElements() &&
             "Packed-arg struct field index does not match number of args");
      auto *gep = ir.CreateGEP(
          structType, packedArgPtr,
          {ir.getInt32(0), ir.getInt32(argMapping.PackedStructFieldIdx)});
      auto &dl = M.getDataLayout();
      const uint32_t alignmentNeeded =
          IsPacked ? 1 : dl.getABITypeAlign(type).value();
      auto llvmAlignment = Align(alignmentNeeded);

      if (PassLocalBuffersBySize && isArgLocalBuffer(type)) {
        // we need to alloca the size in the load for local pointer
        // arguments
        auto *intermediateTy = getSizeType(M);
        auto *load = ir.CreateAlignedLoad(intermediateTy, gep, llvmAlignment);
        auto *allocaInst = ir.CreateAlloca(ir.getInt8Ty(), load);
        allocaInst->setAlignment(llvm::Align(sizeof(uint64_t) * 16));

        params.push_back(ir.CreateAddrSpaceCast(allocaInst, type));
      } else if (arg.hasByValAttr()) {
        params.push_back(gep);
      } else {
        auto *arg_load = ir.CreateAlignedLoad(type, gep, llvmAlignment);
        // If the old argument was marked 'noundef', the result of the load
        // from it will also be noundef. Use metadata to convey that.
        if (F.getArg(argMapping.OldArgIdx)->hasAttribute(Attribute::NoUndef)) {
          MDNode *md = MDNode::get(newFunction->getContext(), std::nullopt);
          arg_load->setMetadata(LLVMContext::MD_noundef, md);
        }
        // If the old argument was marked 'nonnull', the result of the load
        // from it will also be nonnull. Use metadata to convey that.
        if (F.getArg(argMapping.OldArgIdx)->hasAttribute(Attribute::NonNull)) {
          MDNode *md = MDNode::get(newFunction->getContext(), std::nullopt);
          arg_load->setMetadata(LLVMContext::MD_nonnull, md);
        }
        params.push_back(arg_load);
      }
      // Set the name to help readability
      params.back()->setName(arg.getName());
    }

    createCallToWrappedFunction(F, params, ir.GetInsertBlock(),
                                ir.GetInsertPoint());

    ir.CreateRetVoid();

    Changed = true;
    NewKernels.insert(newFunction);
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
