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

/// @file
///
/// @brief Helper functions for pipeline parsing

#ifndef COMPILER_UTILS_PIPELINE_PARSE_HELPERS_H_INCLUDED
#define COMPILER_UTILS_PIPELINE_PARSE_HELPERS_H_INCLUDED

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FormatVariadic.h>

namespace compiler {
namespace utils {

// Note that parseSinglePassOption(), parsePassParameters() and
// checkParametrizedPassName() helper functions come from llvm's PassBuilder.cpp
static llvm::Expected<bool> parseSinglePassOption(llvm::StringRef Params,
                                                  llvm::StringRef OptionName,
                                                  llvm::StringRef PassName) {
  bool Result = false;
  while (!Params.empty()) {
    llvm::StringRef ParamName;
    std::tie(ParamName, Params) = Params.split(';');

    if (ParamName == OptionName) {
      Result = true;
    } else {
      return llvm::make_error<llvm::StringError>(
          llvm::formatv("invalid {1} pass parameter '{0}' ", ParamName,
                        PassName)
              .str(),
          llvm::inconvertibleErrorCode());
    }
  }
  return Result;
}

template <typename ParametersParseCallableT>
static auto parsePassParameters(ParametersParseCallableT &&Parser,
                                llvm::StringRef Name, llvm::StringRef PassName)
    -> decltype(Parser(llvm::StringRef{})) {
  using ParametersT = typename decltype(Parser(llvm::StringRef{}))::value_type;

  llvm::StringRef Params = Name;
  if (!Params.consume_front(PassName)) {
    assert(false &&
           "unable to strip pass name from parametrized pass specification");
  }
  if (!Params.empty() &&
      (!Params.consume_front("<") || !Params.consume_back(">"))) {
    assert(false && "invalid format for parametrized pass name");
  }

  llvm::Expected<ParametersT> Result = Parser(Params);
  assert((Result || Result.template errorIsA<llvm::StringError>()) &&
         "Pass parameter parser can only return StringErrors.");
  return Result;
}

static bool checkParametrizedPassName(llvm::StringRef Name,
                                      llvm::StringRef PassName) {
  if (!Name.consume_front(PassName)) return false;
  // normal pass name w/o parameters == default parameters
  if (Name.empty()) return true;
  return Name.starts_with("<") && Name.ends_with(">");
}

inline llvm::Expected<llvm::StringRef> parseSinglePassStringRef(
    llvm::StringRef Params) {
  llvm::StringRef Result = "";
  while (!Params.empty()) {
    llvm::StringRef ParamName;
    std::tie(ParamName, Params) = Params.split(';');

    Result = ParamName;
  }
  return Result;
}

}  // namespace utils
}  // namespace compiler

#endif  // COMPILER_UTILS_PIPELINE_PARSE_HELPERS_H_INCLUDED
