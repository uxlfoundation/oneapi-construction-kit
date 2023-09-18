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
#include <compiler/utils/metadata.h>
#include <compiler/utils/pass_functions.h>
#include <compiler/utils/scheduling.h>
#include <host/add_entry_hook_pass.h>
#include <host/host_mux_builtin_info.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <functional>

using namespace llvm;

PreservedAnalyses AddEntryHookPass::run(Module &M, ModuleAnalysisManager &AM) {
  LLVMContext &context = M.getContext();
  auto &BI = AM.getResult<compiler::utils::BuiltinInfoAnalysis>(M);
  bool Changed = false;

  // First cache some constants that are used a lot in the inner loop
  auto *i32Ty = Type::getInt32Ty(context);
  auto getInt32 = [=](uint32_t val) -> Value * {
    return ConstantInt::get(i32Ty, val);
  };
  auto *i32_0 = getInt32(0);
  auto *zero = ConstantInt::get(compiler::utils::getSizeType(M), 0);

  SmallVector<Function *, 4> kernels;

  for (auto &function : M.functions()) {
    if (compiler::utils::isKernelEntryPt(function)) {
      kernels.push_back(&function);
    }
  }

  for (auto *function : kernels) {
    // Create our new wrapper function
    Function *const newFunction = compiler::utils::createKernelWrapperFunction(
        *function, ".host-entry-hook");

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
        dyn_cast_or_null<StructType>(SchedParams[2].ParamPointeeTy);
    auto *ScheduleInfoStructTy =
        dyn_cast_or_null<StructType>(SchedParams[1].ParamPointeeTy);
    // FIXME: Extra verification
    assert(MiniWGInfoStructTy && ScheduleInfoStructTy);

    // the penultimate argument is the schedule information
    Value *const ScheduleInfoParam = SchedArgs[1];
    // the last argument is the mini work-group info
    auto *const MiniWGInfoParam = SchedArgs[2];

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
    // User-specifiable Work Item Order has been removed.
    const uint32_t vec_dim = 0;
    // the slicing code below works as follows:
    // t = total number of slices
    // s = current slice (from [0..t))
    // g = num groups in the vectorization dimension (numGroups[vec_dim])
    // r = num groups rounded up
    // r = g + t
    // size = r / t
    // start = size * s
    // end = min(g, start + size)

    // gep the slice
    auto *const sliceIdx = ir.getInt32(host::ScheduleInfoStruct::slice);
    auto gepSlice = ir.CreateGEP(ScheduleInfoStructTy, ScheduleInfoParam,
                                 {i32_0, sliceIdx});

    // load the slice
    auto *slice = ir.CreateLoad(ScheduleInfoStructTy->getTypeAtIndex(sliceIdx),
                                gepSlice, "slice");

    // gep the total slices
    auto *const totalSlicesIdx =
        ir.getInt32(host::ScheduleInfoStruct::total_slices);
    auto *gepTotalSlices = ir.CreateGEP(ScheduleInfoStructTy, ScheduleInfoParam,
                                        {i32_0, totalSlicesIdx});

    // load the total slices
    auto *totalSlices =
        ir.CreateLoad(ScheduleInfoStructTy->getTypeAtIndex(totalSlicesIdx),
                      gepTotalSlices, "totalSlices");

    // round up the number of groups by the total number of slices
    auto *numGroupsRoundedUp =
        ir.CreateAdd(numGroups[vec_dim], totalSlices, "numGroupsRoundedUp");

    // get the size of the slice that each core will run
    auto *sliceSize =
        ir.CreateUDiv(numGroupsRoundedUp, totalSlices, "sliceSize");

    // get the start position of our slice
    auto *sliceStart = ir.CreateMul(sliceSize, slice, "sliceStart");

    // get the end position of our slice
    auto *sliceEnd = ir.CreateAdd(sliceStart, sliceSize, "sliceEnd");

    // but for the end we need to use a cmp against the original num groups
    auto *clampedSliceEnd =
        ir.CreateSelect(ir.CreateICmpULT(sliceEnd, numGroups[vec_dim]),
                        sliceEnd, numGroups[vec_dim], "clampedSliceEnd");

    // an early exit block
    IRBuilder<> earlyExitIR(
        BasicBlock::Create(context, "early-exit", newFunction));

    earlyExitIR.CreateRetVoid();

    // the loop's main basic block
    IRBuilder<> loopIR(BasicBlock::Create(context, "loop", newFunction));

    // need to early exit before the loops if we don't have a slice to
    // process
    ir.CreateCondBr(ir.CreateICmpULT(sliceStart, clampedSliceEnd),
                    loopIR.GetInsertBlock(), earlyExitIR.GetInsertBlock());

    auto *const groupIdIdx = ir.getInt32(host::MiniWGInfoStruct::group_id);
    auto *dstGroupIdTy = MiniWGInfoStructTy->getTypeAtIndex(groupIdIdx);

    const uint32_t outer_dim = 2;
    const uint32_t middle_dim = 1;

    compiler::utils::CreateLoopOpts opts;

    // looping through num groups in the outermost dimension
    auto exitBlock = compiler::utils::createLoop(
        loopIR.GetInsertBlock(), nullptr, zero, numGroups[outer_dim], {}, opts,
        [&](BasicBlock *blockz, Value *z, ArrayRef<Value *>,
            MutableArrayRef<Value *>) -> BasicBlock * {
          IRBuilder<> ir(blockz);
          Value *dstGroupId = ir.CreateGEP(MiniWGInfoStructTy, MiniWGInfoParam,
                                           {i32_0, groupIdIdx});
          ir.CreateStore(z, ir.CreateGEP(dstGroupIdTy, dstGroupId,
                                         {i32_0, ir.getInt32(outer_dim)}));
          // looping through num groups in the middle dimension
          return compiler::utils::createLoop(
              blockz, nullptr, zero, numGroups[middle_dim], {}, opts,
              [&](BasicBlock *blocky, Value *y, ArrayRef<Value *>,
                  MutableArrayRef<Value *>) -> BasicBlock * {
                IRBuilder<> ir(blocky);
                ir.CreateStore(y,
                               ir.CreateGEP(dstGroupIdTy, dstGroupId,
                                            {i32_0, ir.getInt32(middle_dim)}));

                // looping through num groups in the x dimension
                return compiler::utils::createLoop(
                    blocky, nullptr, sliceStart, clampedSliceEnd, {}, opts,
                    [&](BasicBlock *blockx, Value *x, ArrayRef<Value *>,
                        MutableArrayRef<Value *>) -> BasicBlock * {
                      IRBuilder<> ir(blockx);
                      ir.CreateStore(
                          x, ir.CreateGEP(dstGroupIdTy, dstGroupId,
                                          {i32_0, ir.getInt32(vec_dim)}));

                      compiler::utils::createCallToWrappedFunction(
                          *function, args, ir.GetInsertBlock(),
                          ir.GetInsertPoint());

                      return blockx;
                    });
              });
        });

    // the last basic block in our function!
    IRBuilder<> exitIR(exitBlock);

    // the only thing we need to do now is exit
    exitIR.CreateRetVoid();

    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
