// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "analysis/stride_analysis.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>

#include "analysis/uniform_value_analysis.h"
#include "debugging.h"
#include "memory_operations.h"
#include "offset_info.h"
#include "vectorization_context.h"
#include "vectorization_unit.h"

#define DEBUG_TYPE "vecz"

using namespace vecz;
using namespace llvm;

llvm::AnalysisKey StrideAnalysis::Key;

OffsetInfo &StrideAnalysisResult::analyze(Value *V) {
  auto const find = analyzed.find(V);
  if (find != analyzed.end()) {
    return find->second;
  }

  // We construct it on the stack first, and copy it into the map, because
  // the constructor itself can create more things in the map and constructing
  // it in-place could result in the storage being re-allocated while the
  // constructor is still running.
  auto const OI = OffsetInfo(*this, V);
  return analyzed.try_emplace(V, OI).first->second;
}

StrideAnalysisResult::StrideAnalysisResult(llvm::Function &f,
                                           UniformValueResult &uvr)
    : F(f), UVR(uvr), assumptions(F) {
  for (auto &BB : F) {
    for (auto &I : BB) {
      if (!UVR.isVarying(&I)) {
        continue;
      }

      if (auto mo = MemOp::get(&I)) {
        auto *const ptr = mo->getPointerOperand();
        analyze(ptr);
      }
    }
  }
}

void StrideAnalysisResult::manifestAll(IRBuilder<> &B) {
  auto const saved = B.GetInsertPoint();
  for (auto &info : analyzed) {
    info.second.manifest(B, *this);
  }
  B.SetInsertPoint(saved->getParent(), saved);
}

Value *StrideAnalysisResult::buildMemoryStride(IRBuilder<> &B, llvm::Value *Ptr,
                                               llvm::Type *EleTy) const {
  if (auto *const info = getInfo(Ptr)) {
    return info->buildMemoryStride(B, EleTy, &F.getParent()->getDataLayout());
  }
  return nullptr;
}

StrideAnalysisResult StrideAnalysis::run(llvm::Function &F,
                                         llvm::FunctionAnalysisManager &AM) {
  UniformValueResult &UVR = AM.getResult<UniformValueAnalysis>(F);
  return Result(F, UVR);
}
