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

// This pass will replace IR instructions with equivalent builtins for the
// riscv target.

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <multi_llvm/multi_llvm.h>
#include <multi_llvm/vector_type_helper.h>
#include <riscv/ir_to_builtins_pass.h>

using namespace llvm;

namespace riscv {

bool IRToBuiltinReplacementPass::replaceInstruction(Module &module,
                                                    unsigned opcode,
                                                    StringRef name) {
  bool modified = false;

  for (auto &func : module) {
    for (auto &block : func) {
      IRBuilder<> ir(&block);
      for (auto &ins : block) {
        if (ins.getOpcode() != opcode) {
          continue;
        }

        // This is a rather imperfect mangler but works for the cases so far
        // This code should be reviewed if we add more cases
        assert(
            ((ins.getNumOperands() == 2) &&
             (ins.getOperand(0)->getType() == ins.getOperand(1)->getType())) &&
            "Mangler not good enough for this instruction");
        assert(!isa<ScalableVectorType>(ins.getType()));
        Type *scalarType = ins.getOperand(0)->getType()->getScalarType();
        assert((scalarType->isFloatTy() || scalarType->isDoubleTy()) &&
               "Mangler not good enough for this instruction");

        std::string mangledName = name.str();
        const std::string baseTypeName = scalarType->isDoubleTy() ? "d" : "f";

        if (ins.getType()->isVectorTy()) {
          auto numElements = multi_llvm::getVectorNumElements(ins.getType());
          mangledName += "Dv";  // Vector Type
          mangledName += std::to_string(numElements);
          mangledName += "_";
          mangledName += baseTypeName;  // f or d for float or double
          mangledName += "S_";          // repeat last parameter
        } else {
          mangledName += baseTypeName;  // just f or d for the scalar types
          mangledName += baseTypeName;
        }

        auto *builtin = module.getFunction(mangledName);
        if (!builtin) {
          SmallVector<Type *, 4> argTypes;
          for (const auto &op : ins.operand_values()) {
            argTypes.push_back(op->getType());
          }

          builtin = cast<Function>(
              module
                  .getOrInsertFunction(
                      mangledName,
                      FunctionType::get(ins.getType(), argTypes, false))
                  .getCallee());
          builtin->setCallingConv(CallingConv::SPIR_FUNC);
        }
        const SmallVector<Value *, 4> callArgs(ins.operand_values());
        ir.SetInsertPoint(&ins);
        CallInst *call = ir.CreateCall(builtin, callArgs);
        call->setCallingConv(CallingConv::SPIR_FUNC);
        ins.replaceAllUsesWith(call);
        modified = true;
      }
    }
  }

  return modified;
}

PreservedAnalyses IRToBuiltinReplacementPass::run(
    llvm::Module &module, llvm::ModuleAnalysisManager &) {
  // Replace frem with call to demangled fmod.
  // Note that if other instruction are added, the demangler will need
  // improving
  const bool Changed =
      replaceInstruction(module, llvm::Instruction::FRem, "_Z4fmod");

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

}  // namespace riscv
