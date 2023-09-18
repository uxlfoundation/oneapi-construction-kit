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

#include <compiler/utils/attributes.h>
#include <compiler/utils/builtin_info.h>
#include <compiler/utils/pass_functions.h>
#include <compiler/utils/scheduling.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/IRBuilder.h>
#include <refsi_g1_wi/refsi_wg_loop_pass.h>

using namespace llvm;

namespace refsi_g1_wi {

PreservedAnalyses RefSiWGLoopPass::run(Module &M, ModuleAnalysisManager &AM) {
  LLVMContext &context = M.getContext();
  auto &BI = AM.getResult<compiler::utils::BuiltinInfoAnalysis>(M);

  SmallVector<Function *, 4> kernels;

  for (auto &function : M.functions()) {
    if (compiler::utils::isKernelEntryPt(function)) {
      kernels.push_back(&function);
    }
  }

  for (auto *function : kernels) {
    // Create our new wrapper function
    Function *const newFunction = compiler::utils::createKernelWrapperFunction(
        *function, ".refsi-wg-loop-wrapper");

    // the function's main entry basic block
    BasicBlock *setupBlock = BasicBlock::Create(context, "entry", newFunction);
    IRBuilder<> ir(setupBlock);

    // get the arguments
    SmallVector<Value *, 8> args;

    for (auto &arg : newFunction->args()) {
      args.push_back(&arg);
    }

    auto *NumGroupsFn =
        BI.getOrDeclareMuxBuiltin(compiler::utils::eMuxBuiltinGetNumGroups, M);

    auto SchedParams = BI.getFunctionSchedulingParameters(*newFunction);

    SmallVector<Value *, 4> SchedArgs;
    transform(SchedParams, std::back_inserter(SchedArgs),
              [](const compiler::utils::BuiltinInfo::SchedParamInfo &I) {
                return I.ArgVal;
              });

    auto *MiniWGInfoStructTy =
        dyn_cast_or_null<StructType>(SchedParams[1].ParamPointeeTy);
    assert(MiniWGInfoStructTy &&
           MiniWGInfoStructTy == compiler::utils::getWorkGroupInfoStructTy(M));

    // the penultimate argument is the schedule information
    // the last argument is the mini work-group info
    auto *const MiniWGInfoParam = SchedArgs[1];

    SmallVector<Value *, 4> SchedArgs0 = {ir.getInt32(0)},
                            SchedArgs1 = {ir.getInt32(1)},
                            SchedArgs2 = {ir.getInt32(2)};
    SchedArgs0.append(SchedArgs);
    SchedArgs1.append(SchedArgs);
    SchedArgs2.append(SchedArgs);
    std::array<Value *, 3> numGroups = {
        ir.CreateCall(NumGroupsFn, SchedArgs0, "num_groups_x"),
        ir.CreateCall(NumGroupsFn, SchedArgs1, "num_groups_y"),
        ir.CreateCall(NumGroupsFn, SchedArgs2, "num_groups_z"),
    };

    auto *const groupIdIdx =
        ir.getInt32(compiler::utils::WorkGroupInfoStructField::group_id);
    auto *const dstGroupIdTy = MiniWGInfoStructTy->getTypeAtIndex(groupIdIdx);

    const uint32_t outer_dim = 2;
    const uint32_t middle_dim = 1;
    const uint32_t inner_dim = 0;

    auto *i32_0 = ir.getInt32(0);
    auto *zero = ConstantInt::get(compiler::utils::getSizeType(M), 0);

    // need to early exit before the loops if we don't have a slice to
    // process
    auto *isXZero = ir.CreateICmpEQ(numGroups[inner_dim], zero, "is_x_zero");
    auto *isYZero = ir.CreateICmpEQ(numGroups[middle_dim], zero, "is_y_zero");
    auto *isZZero = ir.CreateICmpEQ(numGroups[outer_dim], zero, "is_z_zero");

    auto *isAnyDimZero =
        ir.CreateOr(isXZero, ir.CreateOr(isYZero, isZZero, "is_x_or_y_zero"),
                    "is_any_zero");

    // a loop pre-header
    IRBuilder<> loopPreheaderIR(
        BasicBlock::Create(context, "loop-pre-header", newFunction));
    // an early exit block
    IRBuilder<> earlyExitIR(
        BasicBlock::Create(context, "early-exit", newFunction));
    earlyExitIR.CreateRetVoid();

    ir.CreateCondBr(
        ir.CreateICmpEQ(isAnyDimZero, Constant::getNullValue(ir.getInt1Ty())),
        loopPreheaderIR.GetInsertBlock(), earlyExitIR.GetInsertBlock());

    compiler::utils::CreateLoopOpts create_loop_opts;
    create_loop_opts.disableVectorize = true;

    // looping through num groups in the outermost dimension
    auto *const exitBlock = compiler::utils::createLoop(
        loopPreheaderIR.GetInsertBlock(), nullptr, zero, numGroups[outer_dim],
        {}, create_loop_opts,
        [&](BasicBlock *blockz, Value *z, ArrayRef<Value *>,
            MutableArrayRef<Value *>) -> BasicBlock * {
          IRBuilder<> ir(blockz);
          Value *dstGroupId = ir.CreateGEP(MiniWGInfoStructTy, MiniWGInfoParam,
                                           {i32_0, groupIdIdx});
          ir.CreateStore(z, ir.CreateGEP(dstGroupIdTy, dstGroupId,
                                         {i32_0, ir.getInt32(outer_dim)}));
          // looping through num groups in the middle dimension
          return compiler::utils::createLoop(
              blockz, nullptr, zero, numGroups[middle_dim], {},
              create_loop_opts,
              [&](BasicBlock *blocky, Value *y, ArrayRef<Value *>,
                  MutableArrayRef<Value *>) -> BasicBlock * {
                IRBuilder<> ir(blocky);
                ir.CreateStore(y,
                               ir.CreateGEP(dstGroupIdTy, dstGroupId,
                                            {i32_0, ir.getInt32(middle_dim)}));

                // looping through num groups in the x dimension
                return compiler::utils::createLoop(
                    blocky, nullptr, zero, numGroups[inner_dim], {},
                    create_loop_opts,
                    [&](BasicBlock *blockx, Value *x, ArrayRef<Value *>,
                        MutableArrayRef<Value *>) -> BasicBlock * {
                      IRBuilder<> ir(blockx);
                      ir.CreateStore(
                          x, ir.CreateGEP(dstGroupIdTy, dstGroupId,
                                          {i32_0, ir.getInt32(inner_dim)}));

                      auto ci = ir.CreateCall(function, args);
                      ci->setCallingConv(function->getCallingConv());
                      ci->setAttributes(
                          compiler::utils::getCopiedFunctionAttrs(*function));

                      // An inlinable function call in a function with debug
                      // info *must* be given a debug location.
                      if (auto *const SP = newFunction->getSubprogram()) {
                        auto *const wrapperDbgLoc = DILocation::get(
                            function->getContext(), /*line*/ 0, /*col*/ 0, SP);
                        ci->setDebugLoc(wrapperDbgLoc);
                      }

                      auto *local_barrier = BI.getOrDeclareMuxBuiltin(
                          compiler::utils::eMuxBuiltinWorkGroupBarrier, M);
                      assert(local_barrier);

                      // Create a barrier after each kernel to prevent
                      // work-items progressing to the next work-group while
                      // other work-items are still in the previous work-group.
                      ir.CreateCall(
                          local_barrier,
                          {ir.getInt32(/*BarrierID*/ 0xFFFFFFFF),
                           ir.getInt32(compiler::utils::BIMuxInfoConcept::
                                           MemScopeWorkGroup),
                           ir.getInt32(compiler::utils::BIMuxInfoConcept::
                                           MemSemanticsAcquireRelease |
                                       compiler::utils::BIMuxInfoConcept::
                                           MemSemanticsWorkGroupMemory)});

                      return blockx;
                    });
              });
        });

    // the last basic block in our function!
    IRBuilder<> exitIR(exitBlock);

    // the only thing we need to do now is exit
    exitIR.CreateRetVoid();

    earlyExitIR.GetInsertBlock()->moveAfter(exitBlock);
  }

  return kernels.empty() ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

}  // namespace refsi_g1_wi
