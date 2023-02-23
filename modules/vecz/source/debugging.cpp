// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "debugging.h"

#include <llvm/Analysis/OptimizationRemarkEmitter.h>

using namespace llvm;

namespace vecz {

/// @brief Create the std::string containing the message for the remark
///
/// @param[in] V The value (can be `nullptr`) to be included in the remark
/// @param[in] Msg The main remark message
/// @return The remark message as it is to be printed
static std::string createRemarkMessage(const Value *V, StringRef Msg) {
  std::string helper_str("Vecz: ");
  raw_string_ostream helper_stream(helper_str);
  helper_stream << Msg;
  if (V) {
    if (isa<Instruction>(V)) {
      // Instructions are already prefixed by two spaces when printed
      V->print(helper_stream, true);
    } else if (const Function *F = dyn_cast<Function>(V)) {
      // Printing a functions leads to it's whole body being printed
      helper_stream << " function \"" << F->getName() << "\"";
    } else {
      helper_stream << " ";
      V->print(helper_stream, true);
    }
  }
  helper_stream << '\n';

  return helper_stream.str();
}

void emitVeczRemarkMissed(const Function *F, const Value *V, StringRef Msg) {
  const Instruction *I = V ? dyn_cast<Instruction>(V) : nullptr;
  auto RemarkMsg = createRemarkMessage(V, Msg);
  OptimizationRemarkEmitter ORE(F);
  if (I) {
    ORE.emit(OptimizationRemarkMissed("vecz", "vecz", I) << RemarkMsg);
  } else {
    DebugLoc D = I ? DebugLoc(I->getDebugLoc()) : DebugLoc();
    ORE.emit(OptimizationRemarkMissed("vecz", "vecz", D, &(F->getEntryBlock()))
             << RemarkMsg);
  }
}

void emitVeczRemarkMissed(const Function *F, StringRef Msg) {
  emitVeczRemarkMissed(F, nullptr, Msg);
}

void emitVeczRemark(const Function *F, const Value *V, StringRef Msg) {
  const Instruction *I = V ? dyn_cast<Instruction>(V) : nullptr;
  DebugLoc D = I ? DebugLoc(I->getDebugLoc()) : DebugLoc();

  auto RemarkMsg = createRemarkMessage(V, Msg);
  OptimizationRemarkEmitter ORE(F);
  ORE.emit(OptimizationRemark("vecz", "vecz", F) << RemarkMsg);
}

void emitVeczRemark(const Function *F, StringRef Msg) {
  emitVeczRemark(F, nullptr, Msg);
}
}  // namespace vecz
