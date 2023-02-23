// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VECZ_IR_CLEANUP_H_INCLUDED
#define VECZ_IR_CLEANUP_H_INCLUDED

#include <llvm/ADT/SmallPtrSet.h>

namespace llvm {
class Instruction;
}

namespace vecz {
class IRCleanup {
 public:
  /// @brief Mark the instruction as needing deletion. It will only be deleted
  /// if it is unused. This is used to mark instructions with side-effects
  /// (e.g. call, load, store and leaves) that have been replaced and are no
  /// longer needed. Dead Code Elimination will not touch such instructions.
  ///
  /// @param[in] I Instruction to mark as needing deletion.
  void deleteInstructionLater(llvm::Instruction *I);

  /// @brief Get rid of instructions that have been marked for deletion.
  void deleteInstructions();

  /// @brief Immediately delete an instruction, and replace all uses with undef
  ///
  /// @param[in] I Instruction to delete.
  static void deleteInstructionNow(llvm::Instruction *I);

 private:
  /// @brief Instructions that have been marked for deletion.
  llvm::SmallPtrSet<llvm::Instruction *, 16> InstructionsToDelete;
};

}  // namespace vecz

#endif  // VECZ_VECTORIZATION_UNIT_H_INCLUDED
