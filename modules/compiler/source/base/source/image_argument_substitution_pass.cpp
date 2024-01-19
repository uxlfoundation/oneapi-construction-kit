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

#include <base/image_argument_substitution_pass.h>
#include <compiler/utils/attributes.h>
#include <compiler/utils/metadata.h>
#include <compiler/utils/pass_functions.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <multi_llvm/llvm_version.h>

#include <map>

using namespace llvm;

namespace {
const std::map<std::string, std::string> funcToFuncMap = {{
#define PIAS_IMAGE1D "14ocl_image1d"
#define PIAS_IMAGE2D "14ocl_image2d"
#define PIAS_IMAGE3D "14ocl_image3d"
#define PIAS_IMAGE1DARRAY "20ocl_image1d_array"
#define PIAS_IMAGE2DARRAY "20ocl_image2d_array"
#define PIAS_IMAGE1DBUFFER "21ocl_image1d_buffer"
#define PIAS_RO "_ro"
#define PIAS_WO "_wo"
#include "image_argument_substitution_pass.inc"

#define PIAS_IMAGE1D "11ocl_image1d"
#define PIAS_IMAGE2D "11ocl_image2d"
#define PIAS_IMAGE3D "11ocl_image3d"
#define PIAS_IMAGE1DARRAY "16ocl_image1darray"
#define PIAS_IMAGE2DARRAY "16ocl_image2darray"
#define PIAS_IMAGE1DBUFFER "17ocl_image1dbuffer"
#define PIAS_RO ""
#define PIAS_WO ""
#include "image_argument_substitution_pass.inc"
}};
}  // namespace

PreservedAnalyses compiler::ImageArgumentSubstitutionPass::run(
    Module &M, ModuleAnalysisManager &) {
  bool module_modified = false;
  auto &Ctx = M.getContext();

  // On LLVM 17+, the target extension types corresponding to samplers should
  // have already been replaced before this pass.
#if LLVM_VERSION_LESS(17, 0)
  // First of all, adjust the ABIs of user-facing kernels by changing samplers
  // (currently pointers) to i32-typed parameters. Use use !opencl.kernels
  // metadata, as it's the only way to identify whether a pointer is in fact a
  // sampler.
  if (auto *KernelMD = M.getNamedMetadata("opencl.kernels")) {
    for (auto *KernelOp : KernelMD->operands()) {
      // We expect to find the kernel function as the first parameter, followed
      // subsequently by argument information.
      if (KernelOp->operands().empty()) {
        continue;
      }
      auto *const KernelFVal =
          cast_if_present<ValueAsMetadata>(KernelOp->getOperand(0))->getValue();
      auto *const KernelF = cast_if_present<Function>(KernelFVal);
      if (!KernelF || KernelF->arg_empty()) {
        // We couldn't find the kernel function, or we could and we know it's
        // not got any sampler parameters as it has no parameters.
        continue;
      }
      ArrayRef<MDOperand> ArgBaseTys;
      // Search for the kernel arguments' base types; we use this to identify
      // sampler parameters (a pointer just looks like a pointer, otherwise).
      for (auto &KernelArgOp : KernelOp->operands().drop_front()) {
        if (const auto *KernelArgMDOp = dyn_cast<const MDNode>(KernelArgOp)) {
          if (const MDString *KernelArgName =
                  dyn_cast<const MDString>(KernelArgMDOp->getOperand(0))) {
            if (KernelArgName->getString() == "kernel_arg_base_type") {
              ArgBaseTys = KernelArgMDOp->operands().drop_front();
              break;
            }
          }
        }
      }
      // If we didn't find them, skip this kernel.
      if (ArgBaseTys.empty()) {
        continue;
      }

      assert(ArgBaseTys.size() == cast<Function>(KernelF)->arg_size());

      SmallVector<Type *> KernelArgTys(
          KernelF->getFunctionType()->getNumParams());
      // Create the list of kernel parameters with all samplers replaced with
      // size_t types.
      transform(enumerate(KernelF->getFunctionType()->params()),
                KernelArgTys.begin(),
                [&M, ArgBaseTys](const auto &P) -> Type * {
                  const auto &MDOp = ArgBaseTys[P.index()];
                  if (const auto *BaseTyName = dyn_cast<const MDString>(MDOp)) {
                    if (BaseTyName->getString() == "sampler_t") {
                      return utils::getSizeType(M);
                    }
                  }
                  return P.value();
                });
      // If we've not found any samplers, skip to the next kernel
      if (equal(KernelF->getFunctionType()->params(), KernelArgTys)) {
        continue;
      }

      // At this point in the pipeline, we're not in a position to rename the
      // kernel from what the user expects. Therefore we steal the old
      // function's name, and rename that old function with a suffix.
      // FIXME: We should be able to rename kernels as we wish at any point in
      // the compilation pipeline.
      auto *WrapperKernel = compiler::utils::createKernelWrapperFunction(
          M, *KernelF, KernelArgTys, "", ".old");

      IRBuilder<> B(BasicBlock::Create(Ctx, "entry", WrapperKernel));

      SmallVector<Value *, 8> Args;
      // Our wrapper hasn't been created with any parameter attributes, as the
      // parameter types have changed. We must copy across all attributes from
      // the non-sampler arguments to maintain program semantics.
      const AttributeList KernelAttrs = KernelF->getAttributes();
      SmallVector<AttributeSet, 4> WrapperParamAttrs(KernelF->arg_size());

      for (auto [OldArg, NewArg] :
           zip(KernelF->args(), WrapperKernel->args())) {
        // Copy parameter names across
        const unsigned ArgIdx = Args.size();
        NewArg.setName(OldArg.getName());
        if (OldArg.getType() == NewArg.getType()) {
          Args.push_back(&NewArg);
          WrapperParamAttrs[ArgIdx] = KernelAttrs.getParamAttrs(ArgIdx);
        } else {
          // Must be a sampler: simply cast the size_t to a pointer. This
          // mirrors what we'll do down the line when changing the pointer back
          // to an size_t/i32 via the opposite operation.
          Args.push_back(B.CreateIntToPtr(&NewArg, OldArg.getType(),
                                          NewArg.getName() + ".ptrcast"));
          // Don't copy across any parameter attributes here, as this was a
          // pointer and is now an i32/i64 type.
          WrapperParamAttrs[ArgIdx] = AttributeSet();
        }
      }

      // Set our parameter attributes, but maintain the function/return
      // attributes the wrapper has already received.
      WrapperKernel->setAttributes(AttributeList::get(
          KernelF->getContext(), WrapperKernel->getAttributes().getFnAttrs(),
          WrapperKernel->getAttributes().getRetAttrs(), WrapperParamAttrs));

      utils::createCallToWrappedFunction(*KernelF, Args, B.GetInsertBlock(),
                                         B.GetInsertPoint());

      B.CreateRetVoid();

      // Update the kernel metadata
      compiler::utils::replaceKernelInOpenCLKernelsMetadata(*KernelF,
                                                            *WrapperKernel, M);

      module_modified = true;
    }
  }
#endif

  // we need to detect if the new sampler function is there and replace it if so
  {
    if (auto *const samplerInitFunc =
            M.getFunction("__translate_sampler_initializer")) {
      module_modified = true;

      IRBuilder<> builder(
          BasicBlock::Create(M.getContext(), "entry", samplerInitFunc));

      auto *arg = samplerInitFunc->getArg(0);
      assert(arg->getType()->isIntegerTy(32) &&
             "Expecting the sampler initializer to be i32");

      // FIXME: The fact that we're defining the function here is inflexible -
      // it relies on the underlying target type for samplers to have already
      // been chosen.
      // On LLVM 17+ this means we require the target extension type to have
      // been replaced with a size_t, and otherwise we expect the sampler to be
      // a pointer that's reinterpretable to an i32 or to be i32 (only in SPIR
      // 1.2). We should really leave this to the mux implementation, perhaps
      // in DefineMuxBuiltinsPass.
#if LLVM_VERSION_GREATER_EQUAL(17, 0)
      assert(utils::getSizeType(M) == samplerInitFunc->getReturnType() &&
             "Expecting samplers to already have been replaced with size_t");
      builder.CreateRet(builder.CreateZExtOrTrunc(
          arg, samplerInitFunc->getFunctionType()->getReturnType()));
#else
      assert((samplerInitFunc->getReturnType()->isPointerTy() ||
              samplerInitFunc->getReturnType()->isIntegerTy(32)) &&
             "Expecting samplers to be pointers or i32s");
      builder.CreateRet(builder.CreateIntToPtr(
          arg, samplerInitFunc->getFunctionType()->getReturnType()));
#endif
    }
  }

  SmallVector<Instruction *, 8> toRemoves;

  for (const auto &pair : funcToFuncMap) {
    auto *const srcFunc = M.getFunction(pair.first);

    // if we didn't find the image function, we skip this loop iteration (the
    // function simply wasn't used in our user's kernels)
    if (!srcFunc) {
      continue;
    }

    // we found the function, so we definitely are modifying the module!
    module_modified = true;

    for (const Use &use : srcFunc->uses()) {
      auto *const call = dyn_cast<CallInst>(use.getUser());

      assert(call && "User wasn't a call instruction!");

      auto *dstFunc = M.getFunction(pair.second);

      // if we haven't got a declaration for our replacement function, we need
      // to make one!
      if (!dstFunc) {
        SmallVector<Type *, 8> types;

        types.push_back(PointerType::getUnqual(Ctx));

        auto *const srcFuncType = srcFunc->getFunctionType();

        // by default, we'll just pass through the remaining arguments
        unsigned i = 1;

        // we need to change our approach for handling passing samplers into
        // libimg here too

        // have we got a function that has a sampler in its argument list?
        if (std::string::npos != pair.first.find("sampler")) {
          types.push_back(IntegerType::get(M.getContext(), 32));

          // the sampler will always be the second argument in the list
          i = 2;
        }

        for (const unsigned e = srcFuncType->getNumParams(); i < e; i++) {
          types.push_back(srcFuncType->getParamType(i));
        }

        auto *const dstFuncType =
            FunctionType::get(srcFunc->getReturnType(), types, false);

        dstFunc = Function::Create(dstFuncType, srcFunc->getLinkage(),
                                   pair.second, &M);
        dstFunc->setCallingConv(srcFunc->getCallingConv());
      }

      SmallVector<Value *, 8> args;

      IRBuilder<> Builder(call->getContext());
      Builder.SetInsertPoint(call->getParent(), call->getIterator());

      // we are changing the incoming image argument in argument 0 into a type
      // expected by our libimg builtins. These builtins expect the image as a
      // pointer in the default address space to a struct type (specifically to
      // mux_image_s, but an opaque pointer's an opaque pointer so we can't
      // enforce that), so we need to cast away any address spaces.
      assert(call->getArgOperand(0)->getType()->isPointerTy() &&
             "Image must be a pointer (assumed to be to mux_image_s)");
      args.push_back(Builder.CreateAddrSpaceCast(call->getArgOperand(0),
                                                 PointerType::getUnqual(Ctx)));

      // by default, we'll just pass through the remaining arguments
      unsigned i = 1;

      // have we got a function that has a sampler in its argument list?
      if (std::string::npos != pair.first.find("sampler")) {
#if LLVM_VERSION_GREATER_EQUAL(17, 0)
        // See the comment about __translate_sampler_initializer above.
        assert(utils::getSizeType(M) == call->getArgOperand(1)->getType() &&
               "Expecting samplers to already have been replaced with size_t");
        args.push_back(Builder.CreateZExtOrTrunc(
            call->getArgOperand(1),
            dstFunc->getFunctionType()->getParamType(1)));
#else
        // we need to change our approach for handling samplers into libimg here
        // too
        assert((call->getArgOperand(1)->getType()->isPointerTy() ||
                call->getArgOperand(1)->getType()->isIntegerTy(32)) &&
               "Expecting samplers to be pointers or i32");
        args.push_back(Builder.CreatePtrToInt(
            call->getArgOperand(1),
            dstFunc->getFunctionType()->getParamType(1)));
#endif

        // the sampler will always be the second argument in the list
        i = 2;
      }

      for (const unsigned e = call->arg_size(); i < e; i++) {
        args.push_back(call->getArgOperand(i));
      }

      auto *const ci = Builder.CreateCall(dstFunc, args);
      ci->setCallingConv(dstFunc->getCallingConv());
      call->replaceAllUsesWith(ci);
      toRemoves.push_back(call);
    }
  }

  // and remove all the calls that we replaced!
  for (auto remove : toRemoves) {
    remove->eraseFromParent();
  }

  // and finally remove the functions we have replaced
  for (const auto &pair : funcToFuncMap) {
    if (auto *const srcFunc = M.getFunction(pair.first)) {
      srcFunc->eraseFromParent();
    }
  }

  return module_modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
