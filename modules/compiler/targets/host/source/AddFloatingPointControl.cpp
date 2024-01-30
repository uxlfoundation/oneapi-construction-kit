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
#include <compiler/utils/metadata.h>
#include <compiler/utils/pass_functions.h>
#include <host/add_floating_point_control_pass.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicsARM.h>
#include <llvm/IR/IntrinsicsX86.h>
#include <multi_llvm/triple.h>

using namespace llvm;

namespace {

// Currently all this function does is set the FTZ bit of the MXCSR register,
// but could be extended in the future to set other configurations like rounding
// mode or DAZ.
void configX86FP(Function &wrapper, Function &function) {
  // create an IR builder with a single basic block in our wrapper
  IRBuilder<> ir(BasicBlock::Create(wrapper.getContext(), "", &wrapper));

  // x86 STMXCSR instruction stores the contents of the MXCSR register in the
  // destination operand. MXCSR contains flags for control and status
  // information regarding SSE instructions.
  Function *const st_mxcsr = Intrinsic::getDeclaration(
      wrapper.getParent(), Intrinsic::x86_sse_stmxcsr);

  // Loads the source operand into the MXCSR register
  Function *const ld_mxcsr = Intrinsic::getDeclaration(
      wrapper.getParent(), Intrinsic::x86_sse_ldmxcsr);

  // Allocas to store the old and new state
  const auto int32Ty = ir.getInt32Ty();
  auto original_mxcsr = ir.CreateAlloca(int32Ty);
  auto new_mxcsr = ir.CreateAlloca(int32Ty);

  // Store the original value in alloca
  auto st_mxcsr_call = ir.CreateCall(st_mxcsr, {original_mxcsr});
  st_mxcsr_call->setCallingConv(st_mxcsr->getCallingConv());

  // FTZ is 15th bit of MXCSR, counting from zero
  constexpr uint32_t ftz_bit = 1 << 15;
  auto orig_load =
      ir.CreateLoad(original_mxcsr->getAllocatedType(), original_mxcsr);
  auto or_mask = ir.CreateOr(orig_load, ir.getInt32(ftz_bit));
  ir.CreateStore(or_mask, new_mxcsr);

  // Set new MXCSR value via intrinsic call
  ir.CreateCall(ld_mxcsr, {new_mxcsr})
      ->setCallingConv(ld_mxcsr->getCallingConv());

  SmallVector<Value *, 1> args;
  for (auto &arg : wrapper.args()) {
    args.push_back(&arg);
  }

  // call the function we are wrapping
  compiler::utils::createCallToWrappedFunction(
      function, args, ir.GetInsertBlock(), ir.GetInsertPoint());

  // reset the MXCSR to the original value
  ir.CreateCall(ld_mxcsr, {original_mxcsr})
      ->setCallingConv(ld_mxcsr->getCallingConv());

  ir.CreateRetVoid();
}

// Currently all this function does is set the FTZ bit of the FRCP register
// but could be extended in the future to set other configurations like rounding
// mode.
void configAarch64FP(Function &wrapper, Function &function) {
  // create an IR builder with a single basic block in our wrapper
  IRBuilder<> ir(BasicBlock::Create(wrapper.getContext(), "", &wrapper));

  // Use inline assembly to get and set FPCR contents, since there isn't an
  // aarch64 intrinsic to do this.
  const auto int64Ty = Type::getInt64Ty(wrapper.getContext());
  FunctionType *get_fpcr_ty = FunctionType::get(int64Ty, {}, false);
  InlineAsm *get_fpcr_asm =
      InlineAsm::get(get_fpcr_ty, "mrs $0, fpcr", "=r", true);
  auto original_fpcr = ir.CreateCall(get_fpcr_asm, {});

  const auto voidTy = Type::getVoidTy(wrapper.getContext());
  FunctionType *set_fpcr_ty = FunctionType::get(voidTy, {int64Ty}, false);
  InlineAsm *set_fpcr_asm =
      InlineAsm::get(set_fpcr_ty, "msr fpcr, $0", "r", true);

  // FTZ is 24th bit of FPCR, counting from 0
  constexpr uint64_t ftz_bit = 1 << 24;
  auto or_mask = ir.CreateOr(original_fpcr, ir.getInt64(ftz_bit));
  ir.CreateCall(set_fpcr_asm, {or_mask});

  SmallVector<Value *, 1> args;
  for (auto &arg : wrapper.args()) {
    args.push_back(&arg);
  }

  // call the function we are wrapping
  auto *const ci = ir.CreateCall(&function, args);
  ci->setCallingConv(function.getCallingConv());
  ci->setAttributes(compiler::utils::getCopiedFunctionAttrs(function));

  // reset the FPCR to the original value
  ir.CreateCall(set_fpcr_asm, {original_fpcr});

  ir.CreateRetVoid();
}

// On arm we run single precision floats on neon, so setting the FPCSR here just
// affects doubles
void configArmFP(Function &wrapper, Function &function) {
  // create an IR builder with a single basic block in our wrapper
  IRBuilder<> ir(BasicBlock::Create(wrapper.getContext(), "", &wrapper));

  Function *const get_fpscr =
      Intrinsic::getDeclaration(wrapper.getParent(), Intrinsic::arm_get_fpscr);

  Function *const set_fpscr =
      Intrinsic::getDeclaration(wrapper.getParent(), Intrinsic::arm_set_fpscr);

  // call fpscr to store the original state
  auto original_fpscr = ir.CreateCall(get_fpscr);
  original_fpscr->setCallingConv(get_fpscr->getCallingConv());

  // bits [31..28] - N, Z, C, and V status flags, set to 0.
  // bits [27..25] - reserved, set to 0.
  // bits [24]     - flush to zero bit, set to 0 or 1.
  // bits [23..22] - rounding bits, set to 0 (round to nearest even).
  // bits [21..20] - vector stride, set to 0 (stride of 1).
  // bits [19]     - reserved, set to 0.
  // bits [18..16] - vector length, set to 0 (length of 1).
  // bits [15..13] - reserved, set to 0.
  // bits [12..8]  - exception trap bits, set to 0 (turn off traps).
  // bits [7..5]   - reserved, set to 0.
  // bits [4,,0]   - exception bits, set to 0 (traps are off anyway).
  const uint32_t fpscr_bits = 0x00000000;
  auto new_fpscr = ir.getInt32(fpscr_bits);

  // set the fpscr to the new masked value
  ir.CreateCall(set_fpscr, {new_fpscr})
      ->setCallingConv(set_fpscr->getCallingConv());

  SmallVector<Value *, 1> args;

  for (auto &arg : wrapper.args()) {
    args.push_back(&arg);
  }

  // call the function we are wrapping
  auto *const ci = ir.CreateCall(&function, args);
  ci->setCallingConv(function.getCallingConv());
  ci->setAttributes(compiler::utils::getCopiedFunctionAttrs(function));

  // reset the fpscr to the original
  ir.CreateCall(set_fpscr, {original_fpscr})
      ->setCallingConv(set_fpscr->getCallingConv());

  ir.CreateRetVoid();
}

Function *runOnFunction(Function &F, bool SetFTZ) {
  const Module &M = *F.getParent();
  // Setting floating point configuration is very architecture specific,
  // so find out which architecture specific helper we want to invoke.
  using helper_func_type = decltype(&configArmFP);
  helper_func_type arch_helper = nullptr;

  const Triple triple(M.getTargetTriple());
  switch (triple.getArch()) {
    case Triple::arm:
      arch_helper = configArmFP;
      break;
    case Triple::aarch64:
      if (SetFTZ) {
        arch_helper = configAarch64FP;
        break;
      }
      return nullptr;
    case Triple::x86:
    case Triple::x86_64:
      if (SetFTZ) {
        arch_helper = configX86FP;
        break;
      }
      return nullptr;
    default:
      return nullptr;
  }

  // create our new function
  Function *const newFunction =
      compiler::utils::createKernelWrapperFunction(F, ".host-fp-control");

  // Invoke architecture specific helper for setting floating point status
  // register, calling original function, then restoring original fp settings
  arch_helper(*newFunction, F);

  return newFunction;
}

}  // namespace

PreservedAnalyses host::AddFloatingPointControlPass::run(
    Module &M, ModuleAnalysisManager &) {
  SmallPtrSet<Function *, 4> newFunctions;
  for (auto &F : M.functions()) {
    if (compiler::utils::isKernelEntryPt(F) && !newFunctions.count(&F)) {
      if (auto *newFunction = runOnFunction(F, SetFTZ)) {
        newFunctions.insert(newFunction);
      }
    }
  }
  return newFunctions.empty() ? PreservedAnalyses::all()
                              : PreservedAnalyses::none();
}
