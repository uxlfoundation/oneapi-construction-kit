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

#include <base/base_module_pass_machinery.h>
#include <compiler/utils/builtin_info.h>
#include <compiler/utils/cl_builtin_info.h>
#include <compiler/utils/lower_to_mux_builtins_pass.h>
#include <compiler/utils/mangling.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/Debug.h>
#include <multi_llvm/llvm_version.h>

#include <cstdint>
#include <cstring>
#include <string>

#include "common.h"
#include "compiler/module.h"

using namespace llvm;
using namespace compiler::utils;

class GroupOpsTest : public CompilerLLVMModuleTest {
 public:
  std::unique_ptr<PassMachinery> PassMach;

  void SetUp() override {
    CompilerLLVMModuleTest::SetUp();

    auto Callback = [](const llvm::Module &) {
      return compiler::utils::BuiltinInfo(
          compiler::utils::createCLBuiltinInfo(/*builtins*/ nullptr));
    };

    PassMach = std::make_unique<compiler::BaseModulePassMachinery>(
        Context, /*TM*/ nullptr, /*Info*/ std::nullopt, Callback,
        /*verify*/ false, /*logging level*/ DebugLogging::None,
        /*time passes*/ false);
    PassMach->initializeStart();
    PassMach->initializeFinish();
  }

  struct GroupOp {
    GroupOp(StringRef FnName, StringRef LLVMTy, GroupCollective C)
        : MangledFnName(FnName), LLVMTy(LLVMTy), Collective(C) {}

    std::string getLLVMFnString(StringRef ParamName = "%x") const {
      std::string FnStr =
          LLVMTy + " @" + MangledFnName + "(" + LLVMTy + " " + ParamName.str();
      if (Collective.isBroadcast()) {
        if (Collective.isSubGroupScope()) {
          FnStr += ", i32 %sg_lid";
        } else {
          FnStr += ", i64 %lid_x, i64 %lid_y, i64 %lid_z";
        }
      }
      FnStr += ")";
      return FnStr;
    }

    std::string MangledFnName;
    std::string LLVMTy;
    GroupCollective Collective;
  };

  static std::string getGroupBuiltinBaseName(GroupCollective::ScopeKind Scope) {
    return std::string(Scope == GroupCollective::ScopeKind::SubGroup ? "sub"
                       : Scope == GroupCollective::ScopeKind::VectorGroup
                           ? "vec"
                           : "work") +
           "_group_";
  }

  std::vector<GroupOp> getGroupBroadcasts(GroupCollective::ScopeKind Scope) {
    std::vector<GroupOp> GroupOps;
    std::string BaseName = getGroupBuiltinBaseName(Scope);

    NameMangler Mangler(&Context);
    Type *const I32Ty = IntegerType::getInt32Ty(Context);
    Type *const I64Ty = IntegerType::getInt64Ty(Context);
    Type *const FloatTy = IntegerType::getFloatTy(Context);

    GroupCollective Collective;
    Collective.IsLogical = false;
    Collective.Scope = Scope;
    // Broadcasts don't expect a recursion kind.
    Collective.Recurrence = RecurKind::None;
    Collective.Op = GroupCollective::OpKind::Broadcast;

    if (Scope == GroupCollective::ScopeKind::SubGroup ||
        Scope == GroupCollective::ScopeKind::VectorGroup) {
      std::string BuiltinName = BaseName + "broadcast";
      SmallVector<TypeQualifiers, 4> QualsVec;
      QualsVec.push_back(eTypeQualNone);
      // And another for the index
      QualsVec.push_back(eTypeQualNone);
      // float version
      GroupOps.emplace_back(
          GroupOp(Mangler.mangleName(BuiltinName, {FloatTy, I32Ty}, QualsVec),
                  "float", Collective));
      // unsigned version
      GroupOps.emplace_back(
          GroupOp(Mangler.mangleName(BuiltinName, {I32Ty, I32Ty}, QualsVec),
                  "i32", Collective));
      // signed version
      QualsVec[0] = eTypeQualSignedInt;
      GroupOps.emplace_back(
          GroupOp(Mangler.mangleName(BuiltinName, {I32Ty, I32Ty}, QualsVec),
                  "i32", Collective));
    } else {
      SmallVector<Type *, 4> Args;
      SmallVector<TypeQualifiers, 4> QualsVec;
      std::string BuiltinName = BaseName + "broadcast";

      // Qualifiers for the argument
      Args.push_back(nullptr);
      QualsVec.push_back(eTypeQualNone);
      // Qualifiers for the indices
      Args.push_back(I64Ty);
      QualsVec.push_back(eTypeQualNone);
      Args.push_back(I64Ty);
      QualsVec.push_back(eTypeQualNone);
      Args.push_back(I64Ty);
      QualsVec.push_back(eTypeQualNone);
      // float version
      Args[0] = FloatTy;
      GroupOps.emplace_back(
          GroupOp(Mangler.mangleName(BuiltinName, Args, QualsVec), "float",
                  Collective));
      // unsigned version
      Args[0] = I32Ty;
      GroupOps.emplace_back(GroupOp(
          Mangler.mangleName(BuiltinName, Args, QualsVec), "i32", Collective));

      // signed version
      Args[0] = I32Ty;
      QualsVec[0] = eTypeQualSignedInt;
      GroupOps.emplace_back(GroupOp(
          Mangler.mangleName(BuiltinName, Args, QualsVec), "i32", Collective));
    }

    return GroupOps;
  }

  // GroupOpKind = "" for reductions, "exclusive" for exclusive scans and
  // "inclusive" for inclusive scans.
  std::vector<GroupOp> getGroupScandAndReductions(
      GroupCollective::ScopeKind Scope, std::string GroupOpKind) {
    const std::string BaseName = getGroupBuiltinBaseName(Scope);

    NameMangler Mangler(&Context);
    Type *const I32Ty = IntegerType::getInt32Ty(Context);
    Type *const FloatTy = IntegerType::getFloatTy(Context);

    std::vector<GroupOp> GroupOps;

    // All sorts of reductions and scans
    for (StringRef OpKind : {"add", "mul", "max", "min", "and", "or", "xor",
                             "logical_and", "logical_or", "logical_xor"}) {
      GroupCollective Collective;
      Collective.IsLogical = false;
      Collective.Scope = Scope;

      std::string BuiltinName = BaseName;
      if (GroupOpKind.empty()) {
        BuiltinName += "reduce";
        Collective.Op = GroupCollective::OpKind::Reduction;
      } else {
        BuiltinName += "scan_";
        Collective.Op = GroupOpKind == "inclusive"
                            ? GroupCollective::OpKind::ScanInclusive
                            : GroupCollective::OpKind::ScanExclusive;
      }

      if (OpKind == "add") {
        Collective.Recurrence = RecurKind::Add;
      } else if (OpKind == "mul") {
        Collective.Recurrence = RecurKind::Mul;
      } else if (OpKind == "max") {
        Collective.Recurrence = RecurKind::UMax;
      } else if (OpKind == "min") {
        Collective.Recurrence = RecurKind::UMin;
      } else if (OpKind == "and") {
        Collective.Recurrence = RecurKind::And;
      } else if (OpKind == "or") {
        Collective.Recurrence = RecurKind::Or;
      } else if (OpKind == "xor") {
        Collective.Recurrence = RecurKind::Xor;
      } else if (OpKind == "logical_and") {
        Collective.IsLogical = true;
        Collective.Recurrence = RecurKind::And;
      } else if (OpKind == "logical_or") {
        Collective.IsLogical = true;
        Collective.Recurrence = RecurKind::Or;
      } else if (OpKind == "logical_xor") {
        Collective.IsLogical = true;
        Collective.Recurrence = RecurKind::Xor;
      } else {
        llvm_unreachable("unhandled op kind");
      }

      BuiltinName += GroupOpKind + "_" + OpKind.str();

      TypeQualifiers DefaultQuals;
      DefaultQuals.push_back(eTypeQualNone);

      TypeQualifiers SignedIntQuals;
      SignedIntQuals.push_back(eTypeQualSignedInt);

      if (OpKind == "add" || OpKind == "mul" || OpKind == "max" ||
          OpKind == "min") {
        // float version
        if (OpKind == "add") {
          Collective.Recurrence = RecurKind::FAdd;
        } else if (OpKind == "mul") {
          Collective.Recurrence = RecurKind::FMul;
        } else if (OpKind == "max") {
          Collective.Recurrence = RecurKind::FMax;
        } else if (OpKind == "min") {
          Collective.Recurrence = RecurKind::FMin;
        } else {
          llvm_unreachable("unhandled op kind");
        }
        GroupOps.emplace_back(
            GroupOp(Mangler.mangleName(BuiltinName, FloatTy, DefaultQuals),
                    "float", Collective));
      }

      // unsigned version
      if (OpKind == "add") {
        Collective.Recurrence = RecurKind::Add;
      } else if (OpKind == "mul") {
        Collective.Recurrence = RecurKind::Mul;
      } else if (OpKind == "max") {
        Collective.Recurrence = RecurKind::UMax;
      } else if (OpKind == "min") {
        Collective.Recurrence = RecurKind::UMin;
      }

      GroupOps.emplace_back(
          GroupOp(Mangler.mangleName(BuiltinName, I32Ty, DefaultQuals), "i32",
                  Collective));

      // signed version
      if (OpKind == "max") {
        Collective.Recurrence = RecurKind::SMax;
      } else if (OpKind == "min") {
        Collective.Recurrence = RecurKind::SMin;
      }
      GroupOps.emplace_back(
          GroupOp(Mangler.mangleName(BuiltinName, I32Ty, SignedIntQuals), "i32",
                  Collective));
    }

    return GroupOps;
  }

  std::vector<GroupOp> getGroupBuiltins(GroupCollective::ScopeKind Scope,
                                        bool IncludeAnyAll = true,
                                        bool IncludeBroadcasts = true,
                                        bool IncludeReductions = true,
                                        bool IncludeScans = true) {
    std::vector<GroupOp> GroupOps;
    std::string BaseName = getGroupBuiltinBaseName(Scope);

    if (IncludeAnyAll) {
      GroupCollective Collective;
      Collective.Op = GroupCollective::OpKind::Any;
      Collective.Recurrence = RecurKind::Or;
      Collective.IsLogical = false;
      Collective.Scope = Scope;

      NameMangler Mangler(&Context);
      Type *const I32Ty = IntegerType::getInt32Ty(Context);

      GroupOps.emplace_back(GroupOp(
          Mangler.mangleName(BaseName + "any", I32Ty, {eTypeQualSignedInt}),
          "i32", Collective));

      Collective.Op = GroupCollective::OpKind::All;
      Collective.Recurrence = RecurKind::And;
      GroupOps.emplace_back(GroupOp(
          Mangler.mangleName(BaseName + "all", I32Ty, {eTypeQualSignedInt}),
          "i32", Collective));
    }

    if (IncludeBroadcasts) {
      auto Broadcasts = getGroupBroadcasts(Scope);
      GroupOps.insert(GroupOps.end(), Broadcasts.begin(), Broadcasts.end());
    }

    if (IncludeReductions) {
      auto Reductions = getGroupScandAndReductions(Scope, "");
      GroupOps.insert(GroupOps.end(), Reductions.begin(), Reductions.end());
    }

    if (IncludeScans) {
      auto InclusiveScans = getGroupScandAndReductions(Scope, "inclusive");
      GroupOps.insert(GroupOps.end(), InclusiveScans.begin(),
                      InclusiveScans.end());

      auto ExclusiveScans = getGroupScandAndReductions(Scope, "exclusive");
      GroupOps.insert(GroupOps.end(), ExclusiveScans.begin(),
                      ExclusiveScans.end());
    }

    return GroupOps;
  }

  std::string getTestModuleStr(const std::vector<std::string> &BuiltinCalls,
                               const std::vector<std::string> &BuiltinDecls) {
    std::string ModuleStr = R"(
target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
)";

    ModuleStr += R"(
define void @test_wrapper(i32 %i, float %f, i32 %sg_lid, i64 %lid_x, i64 %lid_y, i64 %lid_z) {
)";

    for (const auto &Call : BuiltinCalls) {
      ModuleStr += "  " + Call + "\n";
    }

    ModuleStr += "  ret void\n}\n\n";

    for (const auto &Decl : BuiltinDecls) {
      ModuleStr += Decl + "\n";
    }

    ModuleStr += R"(
!opencl.ocl.version = !{!0}

!0 = !{i32 3, i32 0}
)";

    return ModuleStr;
  }

  // This tests:
  // * auto-generates all possible OpenCL group builtins and calls them in a
  // single test function
  // * runs the LowerToMuxBuiltinsPass to replace calls to the mux builtins
  // * tests a round-trip between identifying and declaring those mux builtins
  template <GroupCollective::ScopeKind GroupScope>
  void doTestBody() {
    auto GroupOps = getGroupBuiltins(GroupScope);

    std::vector<std::string> BuiltinDecls;
    std::vector<std::string> BuiltinCalls;
    unsigned Idx = 0;
    for (const auto &Op : GroupOps) {
      BuiltinDecls.push_back("declare " + Op.getLLVMFnString());

      StringRef ParamName =
          Op.LLVMTy == "float" ? "%f" : (Op.LLVMTy == "i32" ? "%i" : "<err>");
      BuiltinCalls.push_back("%call" + std::to_string(Idx) + " = call " +
                             Op.getLLVMFnString(ParamName));
      ++Idx;
    }

    std::string ModuleStr = getTestModuleStr(BuiltinCalls, BuiltinDecls);

    auto M = parseModule(ModuleStr);

    ModulePassManager PM;
    PM.addPass(LowerToMuxBuiltinsPass());

    PM.run(*M, PassMach->getMAM());

    auto &BI = PassMach->getMAM().getResult<BuiltinInfoAnalysis>(*M);

    auto *TestFn = M->getFunction("test_wrapper");
    ASSERT_TRUE(TestFn && !TestFn->empty());

    auto &BB = TestFn->front();

    DenseSet<Function *> MuxBuiltins;
    DenseSet<BuiltinID> MuxBuiltinIDs;
    // Note we expect the called functions in the basic block to be in the same
    // order as the group operations we generated earlier.
    unsigned GroupOpIdx = 0;
    for (auto &I : BB) {
      auto const *CI = dyn_cast<CallInst>(&I);
      if (!CI) {
        continue;
      }
      auto *const CalledFn = CI->getCalledFunction();
      EXPECT_TRUE(CalledFn);
      MuxBuiltins.insert(CalledFn);

      auto Builtin = BI.analyzeBuiltin(*CalledFn);
      std::string InfoStr = " for function " + CalledFn->getName().str() +
                            " identified as ID " + std::to_string(Builtin.ID);
      EXPECT_NE(Builtin.ID, eBuiltinInvalid) << InfoStr;
      EXPECT_TRUE(BI.isMuxBuiltinID(Builtin.ID)) << InfoStr;

      // Do a get-or-declare, and make sure we're getting back the exact same
      // function.
      auto *const BuiltinDecl =
          BI.getOrDeclareMuxBuiltin(Builtin.ID, *M, Builtin.mux_overload_info);
      EXPECT_TRUE(BuiltinDecl && BuiltinDecl == CalledFn) << InfoStr;

      auto Info = BI.isMuxGroupCollective(Builtin.ID);
      ASSERT_TRUE(Info) << InfoStr;

      // Now check that the returned values are what we expect.
      assert(Info && "Asserting the optional to silence a compiler warning");
      EXPECT_EQ(Info->Op, GroupOps[GroupOpIdx].Collective.Op) << InfoStr;
      EXPECT_EQ(Info->Scope, GroupOps[GroupOpIdx].Collective.Scope) << InfoStr;
      EXPECT_EQ(Info->IsLogical, GroupOps[GroupOpIdx].Collective.IsLogical)
          << InfoStr;
      EXPECT_EQ(Info->Recurrence, GroupOps[GroupOpIdx].Collective.Recurrence)
          << InfoStr;

      EXPECT_EQ(Builtin.ID, BI.getMuxGroupCollective(*Info)) << InfoStr;
      EXPECT_EQ(Builtin.ID,
                BI.getMuxGroupCollective(GroupOps[GroupOpIdx].Collective))
          << InfoStr;

      ++GroupOpIdx;
    }
  }
};

TEST_F(GroupOpsTest, OpenCLSubgroupOps) {
  doTestBody<GroupCollective::ScopeKind::SubGroup>();
}

TEST_F(GroupOpsTest, OpenCLWorkgroupOps) {
  doTestBody<GroupCollective::ScopeKind::WorkGroup>();
}

TEST_F(GroupOpsTest, SubgroupShuffles) {
  auto M = std::make_unique<llvm::Module>("test", Context);
  auto &BI = PassMach->getMAM().getResult<BuiltinInfoAnalysis>(*M);
  ASSERT_TRUE(M);

  Type *const I32Ty = Type::getInt32Ty(Context);
  Type *const F16Ty = Type::getHalfTy(Context);

  auto *Shuff =
      BI.getOrDeclareMuxBuiltin(eMuxBuiltinSubgroupShuffle, *M, {F16Ty});
  ASSERT_TRUE(Shuff);
  EXPECT_TRUE(Shuff->getReturnType() == F16Ty && Shuff->arg_size() == 2 &&
              Shuff->getArg(0)->getType() == F16Ty &&
              Shuff->getArg(1)->getType() == I32Ty);
  auto BIShuff = BI.analyzeBuiltin(*Shuff);
  EXPECT_TRUE(BIShuff.isValid() && BIShuff.ID == eMuxBuiltinSubgroupShuffle &&
              BIShuff.mux_overload_info.size() == 1 &&
              *BIShuff.mux_overload_info.begin() == F16Ty);

  auto *ShuffXor =
      BI.getOrDeclareMuxBuiltin(eMuxBuiltinSubgroupShuffleXor, *M, {F16Ty});
  ASSERT_TRUE(ShuffXor);
  EXPECT_TRUE(ShuffXor->getReturnType() == F16Ty && ShuffXor->arg_size() == 2 &&
              ShuffXor->getArg(0)->getType() == F16Ty &&
              ShuffXor->getArg(1)->getType() == I32Ty);
  auto BIXor = BI.analyzeBuiltin(*ShuffXor);
  EXPECT_TRUE(BIXor.isValid() && BIXor.ID == eMuxBuiltinSubgroupShuffleXor &&
              BIXor.mux_overload_info.size() == 1 &&
              *BIXor.mux_overload_info.begin() == F16Ty);

  auto *ShuffUp =
      BI.getOrDeclareMuxBuiltin(eMuxBuiltinSubgroupShuffleUp, *M, {F16Ty});
  ASSERT_TRUE(ShuffUp);
  EXPECT_TRUE(ShuffUp->getReturnType() == F16Ty && ShuffUp->arg_size() == 3 &&
              ShuffUp->getArg(0)->getType() == F16Ty &&
              ShuffUp->getArg(1)->getType() == F16Ty &&
              ShuffUp->getArg(2)->getType() == I32Ty);
  auto BIUp = BI.analyzeBuiltin(*ShuffUp);
  EXPECT_TRUE(BIUp.isValid() && BIUp.ID == eMuxBuiltinSubgroupShuffleUp &&
              BIUp.mux_overload_info.size() == 1 &&
              *BIUp.mux_overload_info.begin() == F16Ty);

  auto *ShuffDown =
      BI.getOrDeclareMuxBuiltin(eMuxBuiltinSubgroupShuffleDown, *M, {F16Ty});
  ASSERT_TRUE(ShuffDown);
  EXPECT_TRUE(ShuffDown->getReturnType() == F16Ty &&
              ShuffDown->arg_size() == 3 &&
              ShuffDown->getArg(0)->getType() == F16Ty &&
              ShuffDown->getArg(1)->getType() == F16Ty &&
              ShuffDown->getArg(2)->getType() == I32Ty);
  auto BIDown = BI.analyzeBuiltin(*ShuffDown);
  EXPECT_TRUE(BIDown.isValid() && BIDown.ID == eMuxBuiltinSubgroupShuffleDown &&
              BIDown.mux_overload_info.size() == 1 &&
              *BIDown.mux_overload_info.begin() == F16Ty);
}
