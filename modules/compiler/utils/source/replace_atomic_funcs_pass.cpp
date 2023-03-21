// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/utils/replace_atomic_funcs_pass.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <multi_llvm/multi_llvm.h>

#include <map>

using namespace llvm;

namespace {
const std::map<std::string, AtomicRMWInst::BinOp> atomicMap = {
    // Atomic add funcs...
    {"_Z8atom_addPU3AS1Vii", AtomicRMWInst::Add},
    {"_Z8atom_addPU3AS3Vii", AtomicRMWInst::Add},
    {"_Z8atom_addPU3AS4Vii", AtomicRMWInst::Add},
    {"_Z8atom_addPU3AS1Vjj", AtomicRMWInst::Add},
    {"_Z8atom_addPU3AS3Vjj", AtomicRMWInst::Add},
    {"_Z8atom_addPU3AS4Vjj", AtomicRMWInst::Add},
    {"_Z10atomic_addPU3AS1Vii", AtomicRMWInst::Add},
    {"_Z10atomic_addPU3AS3Vii", AtomicRMWInst::Add},
    {"_Z10atomic_addPU3AS4Vii", AtomicRMWInst::Add},
    {"_Z10atomic_addPU3AS1Vjj", AtomicRMWInst::Add},
    {"_Z10atomic_addPU3AS3Vjj", AtomicRMWInst::Add},
    {"_Z10atomic_addPU3AS4Vjj", AtomicRMWInst::Add},

    // Atomic and funcs...
    {"_Z8atom_andPU3AS1Vii", AtomicRMWInst::And},
    {"_Z8atom_andPU3AS3Vii", AtomicRMWInst::And},
    {"_Z8atom_andPU3AS4Vii", AtomicRMWInst::And},
    {"_Z8atom_andPU3AS1Vjj", AtomicRMWInst::And},
    {"_Z8atom_andPU3AS3Vjj", AtomicRMWInst::And},
    {"_Z8atom_andPU3AS4Vjj", AtomicRMWInst::And},
    {"_Z10atomic_andPU3AS1Vii", AtomicRMWInst::And},
    {"_Z10atomic_andPU3AS3Vii", AtomicRMWInst::And},
    {"_Z10atomic_andPU3AS4Vii", AtomicRMWInst::And},
    {"_Z10atomic_andPU3AS1Vjj", AtomicRMWInst::And},
    {"_Z10atomic_andPU3AS3Vjj", AtomicRMWInst::And},
    {"_Z10atomic_andPU3AS4Vjj", AtomicRMWInst::And},

    // Atomic cmpxchg funcs...
    {"_Z12atom_cmpxchgPU3AS1Viii", AtomicRMWInst::BAD_BINOP},
    {"_Z12atom_cmpxchgPU3AS3Viii", AtomicRMWInst::BAD_BINOP},
    {"_Z12atom_cmpxchgPU3AS4Viii", AtomicRMWInst::BAD_BINOP},
    {"_Z12atom_cmpxchgPU3AS1Vjjj", AtomicRMWInst::BAD_BINOP},
    {"_Z12atom_cmpxchgPU3AS3Vjjj", AtomicRMWInst::BAD_BINOP},
    {"_Z12atom_cmpxchgPU3AS4Vjjj", AtomicRMWInst::BAD_BINOP},
    {"_Z14atomic_cmpxchgPU3AS1Viii", AtomicRMWInst::BAD_BINOP},
    {"_Z14atomic_cmpxchgPU3AS3Viii", AtomicRMWInst::BAD_BINOP},
    {"_Z14atomic_cmpxchgPU3AS4Viii", AtomicRMWInst::BAD_BINOP},
    {"_Z14atomic_cmpxchgPU3AS1Vjjj", AtomicRMWInst::BAD_BINOP},
    {"_Z14atomic_cmpxchgPU3AS3Vjjj", AtomicRMWInst::BAD_BINOP},
    {"_Z14atomic_cmpxchgPU3AS4Vjjj", AtomicRMWInst::BAD_BINOP},

    // Atomic dec funcs...
    {"_Z8atom_decPU3AS1Vi", AtomicRMWInst::Sub},
    {"_Z8atom_decPU3AS3Vi", AtomicRMWInst::Sub},
    {"_Z8atom_decPU3AS4Vi", AtomicRMWInst::Sub},
    {"_Z8atom_decPU3AS1Vj", AtomicRMWInst::Sub},
    {"_Z8atom_decPU3AS3Vj", AtomicRMWInst::Sub},
    {"_Z8atom_decPU3AS4Vj", AtomicRMWInst::Sub},
    {"_Z10atomic_decPU3AS1Vi", AtomicRMWInst::Sub},
    {"_Z10atomic_decPU3AS3Vi", AtomicRMWInst::Sub},
    {"_Z10atomic_decPU3AS4Vi", AtomicRMWInst::Sub},
    {"_Z10atomic_decPU3AS1Vj", AtomicRMWInst::Sub},
    {"_Z10atomic_decPU3AS3Vj", AtomicRMWInst::Sub},
    {"_Z10atomic_decPU3AS4Vj", AtomicRMWInst::Sub},

    // Atomic inc funcs...
    {"_Z8atom_incPU3AS1Vi", AtomicRMWInst::Add},
    {"_Z8atom_incPU3AS3Vi", AtomicRMWInst::Add},
    {"_Z8atom_incPU3AS4Vi", AtomicRMWInst::Add},
    {"_Z8atom_incPU3AS1Vj", AtomicRMWInst::Add},
    {"_Z8atom_incPU3AS3Vj", AtomicRMWInst::Add},
    {"_Z8atom_incPU3AS4Vj", AtomicRMWInst::Add},
    {"_Z10atomic_incPU3AS1Vi", AtomicRMWInst::Add},
    {"_Z10atomic_incPU3AS3Vi", AtomicRMWInst::Add},
    {"_Z10atomic_incPU3AS4Vi", AtomicRMWInst::Add},
    {"_Z10atomic_incPU3AS1Vj", AtomicRMWInst::Add},
    {"_Z10atomic_incPU3AS3Vj", AtomicRMWInst::Add},
    {"_Z10atomic_incPU3AS4Vj", AtomicRMWInst::Add},

    // Atomic min funcs...
    {"_Z8atom_minPU3AS1Vii", AtomicRMWInst::Min},
    {"_Z8atom_minPU3AS3Vii", AtomicRMWInst::Min},
    {"_Z8atom_minPU3AS4Vii", AtomicRMWInst::Min},
    {"_Z8atom_minPU3AS1Vjj", AtomicRMWInst::UMin},
    {"_Z8atom_minPU3AS3Vjj", AtomicRMWInst::UMin},
    {"_Z8atom_minPU3AS4Vjj", AtomicRMWInst::UMin},
    {"_Z10atomic_minPU3AS1Vii", AtomicRMWInst::Min},
    {"_Z10atomic_minPU3AS3Vii", AtomicRMWInst::Min},
    {"_Z10atomic_minPU3AS4Vii", AtomicRMWInst::Min},
    {"_Z10atomic_minPU3AS1Vjj", AtomicRMWInst::UMin},
    {"_Z10atomic_minPU3AS3Vjj", AtomicRMWInst::UMin},
    {"_Z10atomic_minPU3AS4Vjj", AtomicRMWInst::UMin},

    // Atomic max funcs...
    {"_Z8atom_maxPU3AS1Vii", AtomicRMWInst::Max},
    {"_Z8atom_maxPU3AS3Vii", AtomicRMWInst::Max},
    {"_Z8atom_maxPU3AS4Vii", AtomicRMWInst::Max},
    {"_Z8atom_maxPU3AS1Vjj", AtomicRMWInst::UMax},
    {"_Z8atom_maxPU3AS3Vjj", AtomicRMWInst::UMax},
    {"_Z8atom_maxPU3AS4Vjj", AtomicRMWInst::UMax},
    {"_Z10atomic_maxPU3AS1Vii", AtomicRMWInst::Max},
    {"_Z10atomic_maxPU3AS3Vii", AtomicRMWInst::Max},
    {"_Z10atomic_maxPU3AS4Vii", AtomicRMWInst::Max},
    {"_Z10atomic_maxPU3AS1Vjj", AtomicRMWInst::UMax},
    {"_Z10atomic_maxPU3AS3Vjj", AtomicRMWInst::UMax},
    {"_Z10atomic_maxPU3AS4Vjj", AtomicRMWInst::UMax},

    // Atomic or funcs...
    {"_Z7atom_orPU3AS1Vii", AtomicRMWInst::Or},
    {"_Z7atom_orPU3AS3Vii", AtomicRMWInst::Or},
    {"_Z7atom_orPU3AS4Vii", AtomicRMWInst::Or},
    {"_Z7atom_orPU3AS1Vjj", AtomicRMWInst::Or},
    {"_Z7atom_orPU3AS3Vjj", AtomicRMWInst::Or},
    {"_Z7atom_orPU3AS4Vjj", AtomicRMWInst::Or},
    {"_Z9atomic_orPU3AS1Vii", AtomicRMWInst::Or},
    {"_Z9atomic_orPU3AS3Vii", AtomicRMWInst::Or},
    {"_Z9atomic_orPU3AS4Vii", AtomicRMWInst::Or},
    {"_Z9atomic_orPU3AS1Vjj", AtomicRMWInst::Or},
    {"_Z9atomic_orPU3AS3Vjj", AtomicRMWInst::Or},
    {"_Z9atomic_orPU3AS4Vjj", AtomicRMWInst::Or},

    // Atomic sub funcs...
    {"_Z8atom_subPU3AS1Vii", AtomicRMWInst::Sub},
    {"_Z8atom_subPU3AS3Vii", AtomicRMWInst::Sub},
    {"_Z8atom_subPU3AS4Vii", AtomicRMWInst::Sub},
    {"_Z8atom_subPU3AS1Vjj", AtomicRMWInst::Sub},
    {"_Z8atom_subPU3AS3Vjj", AtomicRMWInst::Sub},
    {"_Z8atom_subPU3AS4Vjj", AtomicRMWInst::Sub},
    {"_Z10atomic_subPU3AS1Vii", AtomicRMWInst::Sub},
    {"_Z10atomic_subPU3AS3Vii", AtomicRMWInst::Sub},
    {"_Z10atomic_subPU3AS4Vii", AtomicRMWInst::Sub},
    {"_Z10atomic_subPU3AS1Vjj", AtomicRMWInst::Sub},
    {"_Z10atomic_subPU3AS3Vjj", AtomicRMWInst::Sub},
    {"_Z10atomic_subPU3AS4Vjj", AtomicRMWInst::Sub},

    // Atomic xchg funcs...
    {"_Z9atom_xchgPU3AS1Vii", AtomicRMWInst::Xchg},
    {"_Z9atom_xchgPU3AS3Vii", AtomicRMWInst::Xchg},
    {"_Z9atom_xchgPU3AS4Vii", AtomicRMWInst::Xchg},
    {"_Z9atom_xchgPU3AS1Vjj", AtomicRMWInst::Xchg},
    {"_Z9atom_xchgPU3AS3Vjj", AtomicRMWInst::Xchg},
    {"_Z9atom_xchgPU3AS4Vjj", AtomicRMWInst::Xchg},
    {"_Z9atom_xchgPU3AS1Vff", AtomicRMWInst::Xchg},
    {"_Z9atom_xchgPU3AS3Vff", AtomicRMWInst::Xchg},
    {"_Z9atom_xchgPU3AS4Vff", AtomicRMWInst::Xchg},
    {"_Z11atomic_xchgPU3AS1Vii", AtomicRMWInst::Xchg},
    {"_Z11atomic_xchgPU3AS3Vii", AtomicRMWInst::Xchg},
    {"_Z11atomic_xchgPU3AS4Vii", AtomicRMWInst::Xchg},
    {"_Z11atomic_xchgPU3AS1Vjj", AtomicRMWInst::Xchg},
    {"_Z11atomic_xchgPU3AS3Vjj", AtomicRMWInst::Xchg},
    {"_Z11atomic_xchgPU3AS4Vjj", AtomicRMWInst::Xchg},
    {"_Z11atomic_xchgPU3AS1Vff", AtomicRMWInst::Xchg},
    {"_Z11atomic_xchgPU3AS3Vff", AtomicRMWInst::Xchg},
    {"_Z11atomic_xchgPU3AS4Vff", AtomicRMWInst::Xchg},

    // Atomic xor funcs...
    {"_Z8atom_xorPU3AS1Vii", AtomicRMWInst::Xor},
    {"_Z8atom_xorPU3AS3Vii", AtomicRMWInst::Xor},
    {"_Z8atom_xorPU3AS4Vii", AtomicRMWInst::Xor},
    {"_Z8atom_xorPU3AS1Vjj", AtomicRMWInst::Xor},
    {"_Z8atom_xorPU3AS3Vjj", AtomicRMWInst::Xor},
    {"_Z8atom_xorPU3AS4Vjj", AtomicRMWInst::Xor},
    {"_Z10atomic_xorPU3AS1Vii", AtomicRMWInst::Xor},
    {"_Z10atomic_xorPU3AS3Vii", AtomicRMWInst::Xor},
    {"_Z10atomic_xorPU3AS4Vii", AtomicRMWInst::Xor},
    {"_Z10atomic_xorPU3AS1Vjj", AtomicRMWInst::Xor},
    {"_Z10atomic_xorPU3AS3Vjj", AtomicRMWInst::Xor},
    {"_Z10atomic_xorPU3AS4Vjj", AtomicRMWInst::Xor},
};

/// @brief Change function call for atomic to instruction.
///
/// @param[in,out] call Call instruction for transformation.
///
/// @return bool Whether or not the pass changed anything.
bool RunOnInstruction(CallInst &call) {
  if (Function *callee = call.getCalledFunction()) {
    StringRef name = callee->getName();
    // Let's check mangled name. If spir's name mangling is changed, we also
    // need to check function's name changed. We need to check for two
    // variants of each mangled function because LLVM 3.8 backwards was
    // incorrectly mangling where the address space (AS<n>) was placed in the
    // mangled name

    auto find = atomicMap.find(name.str());
    if (atomicMap.end() == find) {
      return false;
    }

    const AtomicRMWInst::BinOp Kind = find->second;

    const AtomicOrdering ordering = AtomicOrdering::Monotonic;

    IRBuilder<> builder(&call);

    Value *value = nullptr;

    if (3 == call.arg_size()) {
      // only atomic_cmpxchg needs a different instruction kind
      value = multi_llvm::CreateAtomicCmpXchg(
          builder, call.getArgOperand(0), call.getArgOperand(1),
          call.getArgOperand(2), ordering, ordering);
      value = builder.CreateExtractValue(value, 0);
    } else {
      auto op0 = call.getArgOperand(0);
      auto op1 =
          (1 == call.arg_size()) ? builder.getInt32(1) : call.getArgOperand(1);

      if (call.getType()->isFloatTy()) {
        auto ptr = builder.getInt32Ty()->getPointerTo(
            op0->getType()->getPointerAddressSpace());
        op0 = builder.CreateBitCast(op0, ptr);
      }

      if (op1->getType()->isFloatTy()) {
        op1 = builder.CreateBitCast(op1, builder.getInt32Ty());
      }

      value = multi_llvm::CreateAtomicRMW(builder, Kind, op0, op1, ordering);

      if (call.getType()->isFloatTy()) {
        value = builder.CreateBitCast(value, call.getType());
      }
    }

    call.replaceAllUsesWith(value);
    call.eraseFromParent();
  }

  return true;
}

/// @brief Iterate instructions.
///
/// @param[in,out] block Basic block for checking.
///
/// @return Return whether or not the pass changed anything.
bool RunOnBasicBlock(BasicBlock &block) {
  bool result = false;
  BasicBlock::iterator iter = block.begin();
  while (iter != block.end()) {
    Instruction *Inst = &*(iter++);
    if (CallInst *call = dyn_cast<CallInst>(Inst)) {
      result |= RunOnInstruction(*call);
    }
  }

  return result;
}

/// @brief Iterate basic blocks.
///
/// @param[in,out] function Function for checking.
///
/// @return Whether or not the pass changed anything.
bool RunOnFunction(Function &function) {
  bool result = false;

  for (auto &basicBlock : function) {
    result |= RunOnBasicBlock(basicBlock);
  }

  return result;
}

}  // namespace

PreservedAnalyses compiler::utils::ReplaceAtomicFuncsPass::run(
    Module &M, ModuleAnalysisManager &) {
  bool Changed = false;

  for (auto &F : M) {
    Changed |= RunOnFunction(F);
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
