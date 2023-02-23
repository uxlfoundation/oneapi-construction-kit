// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "analysis/vectorization_unit_analysis.h"

#define DEBUG_TYPE "vecz-unit-analysis"

using namespace vecz;

llvm::AnalysisKey VectorizationUnitAnalysis::Key;

VectorizationUnitAnalysis::Result VectorizationUnitAnalysis::run(
    llvm::Function &F, llvm::FunctionAnalysisManager &) {
  return Result{Ctx.getActiveVU(&F)};
}

#undef DEBUG_TYPE
#define DEBUG_TYPE "vecz-context-analysis"

llvm::AnalysisKey VectorizationContextAnalysis::Key;

VectorizationContextAnalysis::Result VectorizationContextAnalysis::run(
    llvm::Function &, llvm::FunctionAnalysisManager &) {
  return Result{Context};
}
