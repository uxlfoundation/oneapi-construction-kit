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

#include <cargo/string_algorithm.h>
#include <cargo/string_view.h>
#include <cl/binary/kernel_info.h>
#include <cl/macros.h>

#include <algorithm>
#include <regex>
#include <string>

// Defined in OpenCL extension `cl_khr_extended_versioning` as exactly 64.
#define CL_MAX_KERNEL_NAME_SIZE 64

// Concatenate stuff into a std::string, and extract the C string
// TODO: CA-1648: Clean up assert message generation
#define CA_CAT(MESSAGE) (std::string() + (MESSAGE)).c_str()

namespace cl {
namespace binary {

bool kernelDeclStrToKernelInfo(compiler::KernelInfo &kernel_info,
                               const cargo::string_view decl,
                               bool store_arg_metadata) {
  // Do some quick linting of the declaration string. See "Built-In Kernel
  // Declaration Syntax" in the Core spec for details.
  OCL_ASSERT(cargo::string_view::npos == decl.find_first_of("[]"),
             CA_CAT("Found illegal character `[` or `]` in '" +
                    cargo::as<std::string>(decl) +
                    "'. Pointer arguments must not use `[]` notation."));
  OCL_ASSERT(
      cargo::string_view::npos == decl.find("__attribute__"),
      CA_CAT("Found '__attribute__' in '" + cargo::as<std::string>(decl) +
             "'. Attributes must not be used."));
  OCL_ASSERT(
      cargo::string_view::npos == decl.find_first_of("\t\n\v\f\r"),
      CA_CAT("Found illegal whitespace in '" + cargo::as<std::string>(decl) +
             "'. Only ' ' is supported."));

  // Split string into name and parameters
  static const std::regex kernel_and_params_re(
      R"(^\s*(\w+)\s*\(\s*(.*)\s*\)\s*$)", std::regex::optimize);
  std::match_results<cargo::string_view::const_iterator> match;

  // Split the declaration into a name and parameters
  const bool error = std::regex_search(decl.cbegin(), decl.cend(), match,
                                       kernel_and_params_re);
  // We're parsing hard-coded kernel declaration strings, so the assertions in
  // this file should never fire after built-in kernels have been tested
  OCL_ASSERT(error,
             CA_CAT("Could not parse kernel name and parameter list in '" +
                    cargo::as<std::string>(decl) + "'"));
#ifdef NDEBUG
  (void)error;
#endif

  kernel_info.name = match.str(1);
  OCL_ASSERT(kernel_info.name.length() < CL_MAX_KERNEL_NAME_SIZE,
             "Built in kernel name exceeds internal buffer of cl_name_version "
             "object which is defined by spec");
  const cargo::string_view params_str(match[2].first, match[2].length());

  // Create a vector, where each element is a string_view of one parameter
  auto params_str_v = cargo::split_all(params_str, ",");

  if (cargo::success != kernel_info.argument_types.alloc(params_str_v.size())) {
    return false;
  }

  if (store_arg_metadata) {
    kernel_info.argument_info.emplace();
  }

  // Take the last word from v, delimited by ` ` or `*`. Shorten v and
  // return the word.
  auto pop_word = [](cargo::string_view &v) {
    const size_t loc = v.find_last_of(" *");  // not a regex; just characters
    const cargo::string_view ret(v.cbegin() + loc + 1, v.size() - loc - 1);
    v.remove_suffix(ret.size());
    v = cargo::trim_right(v, " ");
    return ret;
  };

  // Like pop_word, but doesn't modify the paramter
  auto get_last_word = [](const cargo::string_view &v) {
    const size_t loc = v.find_last_of(" *");  // not a regex; just characters
    return cargo::string_view(v.cbegin() + loc + 1, v.size() - loc - 1);
  };

  // Return the first word from v and shorten v.
  auto deque_word = [](cargo::string_view &v) {
    const size_t loc = v.find(' ');
    if (cargo::string_view::npos == loc) {
      const cargo::string_view ret(v);
      v.remove_prefix(v.size());
      return ret;
    } else {
      const cargo::string_view ret(v.cbegin(), loc);
      v.remove_prefix(loc);
      v = cargo::trim_left(v, " ");
      return ret;
    }
  };

  // Loop over all the parameter strings.
  // Use a loop variable, because we need to access argument_types_ with it.
  for (size_t i = 0; i < params_str_v.size(); ++i) {
    auto &p = params_str_v[i];
    auto &arg_type = kernel_info.argument_types[i];
    compiler::KernelInfo::ArgumentInfo arg_info;

    p = cargo::trim(p, " ");

    // An empty string here means there was a stray `,` in the parameter list
    OCL_ASSERT(!p.empty(), CA_CAT("Stray `,` in '" +
                                  cargo::as<std::string>(params_str) + "'"));

    // Pick up parameter name
    arg_info.name = cargo::as<std::string>(pop_word(p));
    OCL_ASSERT(!arg_info.name.empty(), CA_CAT("Argument name not found in '" +
                                              cargo::as<std::string>(p) + "'"));

    // Check if it's a pointer. `*` is more likely to be toward the end of the
    // parameter.
    if (p.rfind('*', p.size()) != cargo::string_view::npos) {
      // Pick up address space qualifier
      const cargo::string_view addr_qual = deque_word(p);
      // Search in reverse because we don't care about leading `__`
      if (cargo::string_view::npos != addr_qual.rfind("global")) {
        arg_info.address_qual = compiler::AddressSpace::GLOBAL;
        arg_type = compiler::ArgumentType{compiler::AddressSpace::GLOBAL};
      } else if (cargo::string_view::npos != addr_qual.rfind("constant")) {
        arg_info.address_qual = compiler::AddressSpace::CONSTANT;
        arg_info.type_qual |= CL_KERNEL_ARG_TYPE_CONST;
        arg_type = compiler::ArgumentType{compiler::AddressSpace::CONSTANT};
      } else if (cargo::string_view::npos != addr_qual.rfind("local")) {
        arg_info.address_qual = compiler::AddressSpace::LOCAL;
        arg_type = compiler::ArgumentType{compiler::AddressSpace::LOCAL};
      } else {
        OCL_ASSERT(false, CA_CAT("Expected an address space qualifier in '" +
                                 cargo::as<std::string>(addr_qual) + "'"));
      }

      if (store_arg_metadata) {
        // Gobble trailing `const`s and `restrict`s. A const is a constant
        // pointer (a don't-care).
        while (true) {
          const cargo::string_view word(get_last_word(p));
          if (!word.compare("restrict")) {
            arg_info.type_qual |= CL_KERNEL_ARG_TYPE_RESTRICT;
          } else if (!word.compare("const")) {
            // Don't care
          } else {
            break;  // Done gobbling trailing words
          }
          p.remove_suffix(word.size());
          p = cargo::trim_right(p, " ");
        }

        // A pointer must have a `*` here. If not, it's an error.
        OCL_ASSERT('*' == *(p.cend() - 1),
                   CA_CAT("Expected an '*' at the end of '" +
                          cargo::as<std::string>(p) + "'"));
        p.remove_suffix(1);
        p = cargo::trim_right(p, " ");

        // Another `*` means it was a pointer-to-pointer. These aren't valid.
        OCL_ASSERT(cargo::string_view::npos == p.find('*'),
                   CA_CAT("More than one '*' found in '" +
                          cargo::as<std::string>(decl) + "'"));

        // We should now be left with the type and any qualifiers.
        // Store the type with a trailing `*`.
        auto words_v = cargo::split(p, " ");
        arg_info.type_name = cargo::as<std::string>(words_v.back()) + '*';
        words_v.pop_back();

        for (const auto &w : words_v) {
          if (!w.compare("const")) {
            arg_info.type_qual |= CL_KERNEL_ARG_TYPE_CONST;
          } else if (!w.compare("volatile")) {
            arg_info.type_qual |= CL_KERNEL_ARG_TYPE_VOLATILE;
          } else {
            OCL_ASSERT(false, CA_CAT("Unknown word: '" +
                                     cargo::as<std::string>(w) + "'"));
          }
        }
      }
    } else {  // Not a pointer
      const auto &info = getArgumentTypeFromParameterTypeString(p);
      arg_type = info.first;

      if (store_arg_metadata) {
        arg_info.type_name = info.second;
        switch (arg_type.kind) {
          default:
            // normal value type
            // There could be additional error checking here for illegal words
            break;
          case compiler::ArgumentKind::SAMPLER:
          case compiler::ArgumentKind::IMAGE1D:
          case compiler::ArgumentKind::IMAGE1D_ARRAY:
          case compiler::ArgumentKind::IMAGE1D_BUFFER:
          case compiler::ArgumentKind::IMAGE2D:
          case compiler::ArgumentKind::IMAGE2D_ARRAY:
          case compiler::ArgumentKind::IMAGE3D:
            // Image types, but not sampler_t, default to the global address
            // space
            if (compiler::ArgumentKind::SAMPLER != arg_type.kind) {
              arg_info.address_qual = compiler::AddressSpace::GLOBAL;
              // read_only is the default (section 6.6 of OpenCL 1.2 spec).
              arg_info.access_qual = compiler::KernelArgAccess::READ_ONLY;
            }
            // The remaining string should be fairly short. See if it contains
            // image access qualifiers.
            if (cargo::string_view::npos != p.find("read_write")) {
              arg_info.access_qual = compiler::KernelArgAccess::READ_WRITE;
            } else if (cargo::string_view::npos != p.find("write_only")) {
              arg_info.access_qual = compiler::KernelArgAccess::WRITE_ONLY;
            }
            // There could be additional error checking here for illegal words
            break;
        }
      }
    }

    if (store_arg_metadata) {
      if (cargo::success !=
          kernel_info.argument_info->push_back(std::move(arg_info))) {
        kernel_info.argument_info = cargo::nullopt;
        return false;
      }
    }
  }

  return true;
}
}  // namespace binary
}  // namespace cl
