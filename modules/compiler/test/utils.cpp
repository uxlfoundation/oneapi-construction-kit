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

#include <cstdint>
#include <cstring>
#include <tuple>

#include "common.h"
#include "compiler/module.h"

using namespace compiler::utils;

using CompilerUtilsTest = CompilerLLVMModuleTest;

TEST_F(CompilerUtilsTest, CreateKernelWrapper) {
  auto M = parseModule(R"(
  declare void @foo(i8 %a, i16 zeroext %b, i32 %c)
  declare void @bar(i8 %a, i16 zeroext %b, i32 %c) #0

  attributes #0 = { "mux-base-fn-name"="baz" }
  )");

  // Check equality of a function with its wrapper function
  auto CheckFns = [](llvm::Function *F, llvm::Function *WrapperF) {
    // TODO: We may want to check attributes are correctly copied, but it's
    // complicated by the wrapper receiving 'nounwind', occasionally
    // 'alwaysinline', as well as the base name attributes. It's therefore not a
    // straightforward equality check, and relies on implementation details.
    auto *const FTy = F->getFunctionType();
    EXPECT_EQ(FTy, WrapperF->getFunctionType());
    // Check all parameters are the same
    auto FAttrs = F->getAttributes();
    auto WrapperFAttrs = WrapperF->getAttributes();
    for (unsigned i = 0, e = FTy->getNumParams(); i != e; i++) {
      EXPECT_EQ(F->getArg(i)->getName(), WrapperF->getArg(i)->getName());
      EXPECT_EQ(FAttrs.getParamAttrs(i), WrapperFAttrs.getParamAttrs(i));
    }
    auto FBaseName = getBaseFnName(*F);
    auto WrapperFBaseName = getBaseFnName(*WrapperF);
    // The wrapper should always have a base name set, since it's inhereted the
    // old function's name.
    EXPECT_FALSE(WrapperFBaseName.empty());
    // Any base names should be identical, unless the original function didn't
    // have one.
    EXPECT_TRUE((FBaseName == WrapperFBaseName) ||
                (FBaseName.empty() && WrapperFBaseName == F->getName()));
  };

  // Check that we can create wrappers, leaving the old functions in place.
  //   { function name, expected wrapper function name }
  const std::tuple<const char *, const char *> TestData[2] = {
      {"foo", "foo.new"},
      {"bar", "baz.new"},
  };

  for (auto [Name, ExpName] : TestData) {
    auto *F = M->getFunction(Name);
    EXPECT_TRUE(F);

    auto *const NewF = createKernelWrapperFunction(*F, ".new");
    EXPECT_TRUE(NewF);
    EXPECT_EQ(NewF->getName(), ExpName);
    CheckFns(F, NewF);
  }

  // Now check that we can rename the old functions at the same time.
  //   { function name, expected wrapper function name }
  const std::tuple<const char *, const char *, const char *>
      RenameOldFnsTestData[2] = {
          {"foo", "foo.old", "foo.brand_new"},
          {"bar", "bar.old", "baz.brand_new"},
      };

  for (auto [Name, ExpOldName, ExpNewName] : RenameOldFnsTestData) {
    auto *const F = M->getFunction(Name);
    EXPECT_TRUE(F);

    auto *const NewF = createKernelWrapperFunction(*F, ".brand_new", ".old");
    EXPECT_TRUE(NewF);
    EXPECT_EQ(F->getName(), ExpOldName);
    EXPECT_EQ(NewF->getName(), ExpNewName);
    CheckFns(F, NewF);
  }
}

TEST_F(CompilerUtilsTest, ReplaceFunctionInMetadata) {
  auto M = parseModule(R"(
  declare void @foo(i64 %a, i32 %b)
  declare void @bar(i64 %a, i32 %b)

  !opencl.kernels = !{!0}

  !0 = !{ptr @foo, !1, !2, !3, !4, !5}
  !1 = !{!"kernel_arg_addr_space", i32 0, i32 0}
  !2 = !{!"kernel_arg_access_qual", !"none", !"none"}
  !3 = !{!"kernel_arg_type", !"long", !"uint"}
  !4 = !{!"kernel_arg_base_type", !"long", !"uint"}
  !5 = !{!"kernel_arg_type_qual", !"", !""}
  )");

  auto *const Foo = M->getFunction("foo");
  auto *const Bar = M->getFunction("bar");
  ASSERT_TRUE(Foo && Bar);

  replaceKernelInOpenCLKernelsMetadata(*Foo, *Bar, *M);

  llvm::SmallVector<KernelInfo, 2> Kernels;
  populateKernelList(*M, Kernels);

  ASSERT_EQ(Kernels.size(), 1);
  ASSERT_EQ(Kernels[0].Name, Bar->getName());
}
