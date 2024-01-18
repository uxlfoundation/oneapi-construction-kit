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
/// @brief Argument parser.
///
/// This is not a fully featured command line argument parser, but it
/// provides enough to parse options of the following forms:
///
/// * `-<name>` - boolean option
/// * `-<name>{, ,=}<value>` - option with a value
/// * `-<name>{, ,=}{<value0>,<value1>}` - option with a value choices
/// * `-<name>{, ,=}<value0> -<name>{, ,=}<value1>` - option with value that
///   appends to a vector
/// * `-name[<string>]` - option with custom lambda handlers for parsing
/// * a set of positional arguments (e.g. filenames) passed apart from the
///   options, supporting `--` for positional arguments with same names as
///   other options
/// * optional passthrough of unrecognized options to a separate array
///
/// Above `<name>` is the option name, `{, ,=}` is a set of possible separators
/// between the option name and the option value, `<value>`, `<value0>`, and
/// `<value1>` are placeholders for option values.

#ifndef CARGO_ARGUMENT_PARSER_H_INCLUDED
#define CARGO_ARGUMENT_PARSER_H_INCLUDED

#include <cargo/array_view.h>
#include <cargo/error.h>
#include <cargo/small_vector.h>
#include <cargo/string_algorithm.h>
#include <cargo/string_view.h>

#include <functional>
#include <initializer_list>
#include <memory>
#include <string>

namespace cargo {
/// @addtogroup cargo
/// @{

/// @brief Description of an argument used with `cargo::argument_parser`.
class argument {
 public:
  /// @brief Parse result, used in `cargo::argument_parser::parse_args`.
  enum class parse : uint32_t {
    NOT_FOUND,   ///< Given argument was not found.
    INVALID,     ///< Given argument is invalid.
    INCOMPLETE,  ///< Given argument requires further parsing.
    COMPLETE,    ///< Given argument parsing completed.
  };

  /// @brief Type to store the bitset of parsing options.
  using option_bitset = uint8_t;
  /// @brief Enumeration of argument parsing options.
  enum option : uint8_t {
    NONE = 0,                ///< No optional behavior enabled.
    STORE_TRUE = 0x1 << 1,   ///< Store `bool` argument as `true`.
    STORE_FALSE = 0x1 << 2,  ///< Store `bool` argument as `false`.
  };

  /// @brief Function type for custom handlers.
  using custom_handler_function =
      std::function<cargo::argument::parse(cargo::string_view)>;

  /// @brief Construct a boolean argument.
  ///
  /// @param name Name of the argument.
  /// @param storage Boolean storage for the parse result.
  /// @param options Bitset of `cargo::argument::option` values.
  argument(cargo::string_view name, bool &storage,
           cargo::argument::option_bitset options = STORE_TRUE)
      : Bool(&storage), Name(name), Type(BOOL), Options(options) {}

  /// @brief Construct a value argument.
  ///
  /// @param name Name of the argument.
  /// @param storage `cargo::string_view` storage for the argument value.
  /// @param options Bitset of `cargo::argument::option` values.
  argument(cargo::string_view name, cargo::string_view &storage,
           cargo::argument::option_bitset options = NONE)
      : Value(&storage), Name(name), Type(VALUE), Options(options) {}

  /// @brief Construct a choices value argument.
  ///
  /// @param name Name of the argument.
  /// @param choices A list of possible accepted argument values.
  /// @param storage `cargo::string_view` storage for the argument value.
  /// @param options Bitset of `cargo::argument::option` values.
  argument(cargo::string_view name,
           cargo::array_view<cargo::string_view> choices,
           cargo::string_view &storage,
           cargo::argument::option_bitset options = NONE)
      : Choice({std::addressof(storage), choices}),
        Name(name),
        Type(CHOICES),
        Options(options) {}

  /// @brief Construct an append value argument.
  ///
  /// @param name Name of the argument.
  /// @param storage Vector storage for values to be appended to.
  /// @param options Bitset of `cargo::argument::option` values.
  argument(cargo::string_view name,
           cargo::small_vector<cargo::string_view, 4> &storage,
           cargo::argument::option_bitset options = NONE)
      : Values(std::addressof(storage)),
        Name(name),
        Type(APPEND),
        Options(options) {}

  /// @brief Construct a custom handler value argument.
  ///
  /// @param name Name of the argument.
  /// @param parse_argument Called when the argument is encountered, should
  /// return parse::INCOMPLETE if it expects a value after the argument.
  /// @param parse_value Called once the value of the argument is encountered,
  /// should return parse::INVALID if it was not supposed to have a value.
  /// @param options Bitset of `cargo::argument::option` values.
  argument(cargo::string_view name, custom_handler_function &&parse_argument,
           custom_handler_function &&parse_value,
           cargo::argument::option_bitset options = NONE)
      : CustomHandler({std::move(parse_argument), std::move(parse_value)}),
        Name(name),
        Type(CUSTOM),
        Options(options) {}

  /// @brief Parse a given argument, used by `cargo::argument_parser`.
  ///
  /// @param arg Given argument to parse.
  ///
  /// @return Returns an error from `cargo::small_vector` or the parse result.
  /// @retval `COMPLETE` argument was found with no further action required.
  /// @retval `INCOMPLETE` argument was found, requires a value.
  /// @retval `NOT_FOUND` argument was not found.
  /// @retval `INVALID` invalid argument, value not found.
  [[nodiscard]] cargo::error_or<cargo::argument::parse> parse_arg(
      cargo::string_view arg) {
    if (Name == arg) {  // "<name> <value>"
      switch (Type) {
        case BOOL:
          *Bool = (STORE_FALSE & Options) ? false : true;
          return cargo::argument::parse::COMPLETE;
        case VALUE:
        case CHOICES:
        case APPEND:
          return cargo::argument::parse::INCOMPLETE;
        case CUSTOM:
          return CustomHandler.ParseArgument(arg);
      }
    }
    if (arg.starts_with(Name)) {  // "<name><value>"
      auto value = arg.substr(Name.size(), cargo::string_view::npos);
      if (!value) {
        return cargo::argument::parse::NOT_FOUND;
      }
      if (Type == CUSTOM) {
        CustomHandler.ParseArgument(arg);
      }
      return parse_value(std::move(*value));
    }
    return cargo::argument::parse::NOT_FOUND;
  }

  /// @brief Parse an arguments value, used by `cargo::argument_parser`.
  ///
  /// @param value Given value to parse.
  ///
  /// @return Returns an error from `cargo::small_vector` or the parse result.
  /// @retval `COMPLETE` argument was found with no further action required.
  /// @retval `INVALID` invalid argument, value not found.
  [[nodiscard]] cargo::error_or<cargo::argument::parse> parse_value(
      cargo::string_view value) {
    switch (Type) {
      case BOOL:
        return cargo::argument::parse::INVALID;
      case VALUE:
        *Value = std::move(value);
        break;
      case CHOICES:
        if (std::none_of(Choice.Choices.begin(), Choice.Choices.end(),
                         [value](const string_view &choice) {
                           return value == choice;
                         })) {
          return cargo::argument::parse::INVALID;
        }
        *Choice.Value = std::move(value);
        break;
      case APPEND:
        if (auto error = Values->emplace_back(std::move(value))) {
          return error;
        }
        break;
      case CUSTOM:
        return CustomHandler.ParseValue(std::move(value));
        break;
    }
    return cargo::argument::parse::COMPLETE;
  }

 private:
  union {
    bool *Bool;
    cargo::string_view *Value;
    struct {
      cargo::string_view *Value;
      cargo::array_view<cargo::string_view> Choices;
    } Choice;
    cargo::small_vector<cargo::string_view, 4> *Values;
  };
  struct {
    custom_handler_function ParseArgument;
    custom_handler_function ParseValue;
  } CustomHandler;

  cargo::string_view Name;
  enum : uint8_t {
    BOOL,
    VALUE,
    CHOICES,
    APPEND,
    CUSTOM,
  } Type;
  option_bitset Options;
};

/// @brief Type to store the bitset of parser options.
using argument_parser_option_bitset = uint8_t;
/// @brief Enumeration of argument parser options.
enum argument_parser_option : uint8_t {
  /// No optional behavior enabled.
  NONE = 0,
  /// Keep unrecognized arguments instead of erroring
  KEEP_UNRECOGNIZED = 1,
  /// Accept positonal arguments (e.g. filenames) and support "--" to stop
  /// parsing other arguments
  ACCEPT_POSITIONAL = 2,
};

/// @brief Command line argument parser.
///
/// @tparam N Expected number of arguments, specifies SBO storage.
/// @tparam NP Expected number of positional arguments, specifies SBO storage.
/// @tparam NU Expected number of unrecognised arguments, specifies SBO storage.
///
/// Example usage:
///
/// ```cpp
/// cargo::argument_parser parser;
///
/// bool enable_opts = true;
/// parser.add_argument({"-enable-opts", enable_opts});
/// parser.add_argument({"-disable-opts", enable_opts,
///                      cargo::argument::STORE_FALSE});
///
/// std::array<cargo::array_view, 3> choices = {{
///     "always", "on-demand", "never"
/// }};
/// cargo::string_view choice;
/// parser.add_argument({"-cache=", choices, choice}});
///
/// cargo::small_vector<cargo::string_view, 4> includes;
/// parser.add_argument({"-I", includes});
///
/// if (auto error = parser.parse_args(arguments)) {
///   return error;
/// }
/// ```
template <size_t N, size_t NP = 1, size_t NU = 1>
class argument_parser {
 public:
  /// @brief Construct the argument parser.
  ///
  /// @param options Bitset of cargo::argument_parser_option values.
  argument_parser(argument_parser_option_bitset options = NONE)
      : Options(options) {}

  /// @brief Add an argument to the parser.
  ///
  /// @param arg Argument to be added to the parser.
  ///
  /// @return Returns result from `cargo::small_vector`.
  [[nodiscard]] cargo::result add_argument(cargo::argument arg) {
    return Args.emplace_back(std::move(arg));
  }

  /// @brief Parses the given array of arguments.
  ///
  /// @param args Array of arguments to parse.
  ///
  /// @return Returns result for `cargo::small_vector` or the parse result.
  /// @retval `cargo::success` parsing was successful.
  /// @retval `cargo::bad_argument` an invalid argument was found.
  /// @retval `cargo::bad_alloc` allocation failure.
  [[nodiscard]] cargo::result parse_args(
      cargo::array_view<cargo::string_view> args) {
    if (0 == args.size()) {
      return cargo::success;
    }
    bool AfterArgumentTerminator = false;
    const bool ErrorOnUnrecognized = !(Options & KEEP_UNRECOGNIZED);
    const bool AcceptPositionalArgs = bool(Options & ACCEPT_POSITIONAL);

    for (auto iter = args.begin(), end = args.end(); iter != end; iter++) {
      auto &arg = *iter;
      if (AfterArgumentTerminator) {
        if (auto result = PositionalArgs.push_back(arg)) {
          return result;
        }
        continue;
      }
      for (auto &Arg : Args) {
        auto result = Arg.parse_arg(arg);
        if (!result) {
          return result.error();
        }
        switch (*result) {
          case cargo::argument::parse::COMPLETE:
            goto next_argument;
          case cargo::argument::parse::INCOMPLETE: {
            if (end == iter + 1) {
              return cargo::bad_argument;
            }
            iter++;
            auto result = Arg.parse_value(*iter);
            if (!result) {
              return result.error();
            }
            switch (*result) {
              case cargo::argument::parse::COMPLETE:
                goto next_argument;
              case cargo::argument::parse::INCOMPLETE:
              case cargo::argument::parse::INVALID:
              case cargo::argument::parse::NOT_FOUND:
                return cargo::bad_argument;
            }
          } break;
          case cargo::argument::parse::INVALID:
            return cargo::bad_argument;
          case cargo::argument::parse::NOT_FOUND:
            continue;
        }
      }
      if (AcceptPositionalArgs && arg == "--") {
        AfterArgumentTerminator = true;
        continue;
      }
      if (AcceptPositionalArgs && (arg == "-" || !arg.starts_with("-"))) {
        if (auto result = PositionalArgs.push_back(arg)) {
          return result;
        }
        continue;
      }
      if (ErrorOnUnrecognized) {
        return cargo::bad_argument;
      } else {
        if (auto result = UnrecognizedArgs.push_back(arg)) {
          return result;
        }
      }
    next_argument:;
    }
    return cargo::success;
  }

  /// @brief Parse the given string of arguments.
  /// Arguments can be wrapped in single or double quotes to enclose spaces.
  ///
  /// @param arg_string String of arguments to parse.
  ///
  /// @return Returns result for `cargo::small_vector` or the parse result.
  /// @retval `cargo::success` parsing was successful.
  /// @retval `cargo::bad_argument` when an invalid argument was found.
  /// @retval `cargo::bad_alloc` when an allocation failed.
  [[nodiscard]] cargo::result parse_args(cargo::string_view arg_string) {
    auto args = cargo::split_with_quotes(arg_string, " ");
    return parse_args(cargo::array_view<cargo::string_view>(args));
  }

  /// @brief Parse arguments provided by `main`.
  ///
  /// The first argument in the array, representing the executable name, is
  /// ignored.
  ///
  /// @param argc Number of arguments.
  /// @param argv Pointer to array of C strings.
  ///
  /// @return Returns result for `cargo::small_vector` or the parse result.
  /// @retval `cargo::success` parsing was successful.
  /// @retval `cargo::bad_argument` when an invalid argument was found.
  /// @retval `cargo::bad_alloc` when an allocation failed.
  [[nodiscard]] cargo::result parse_args(const int argc,
                                         const char *const *argv) {
    cargo::small_vector<cargo::string_view, 16> args;
    if (auto error = args.reserve(argc - 1)) {
      return error;
    }
    for (auto argi = 1; argi < argc; argi++) {
      if (auto error = args.emplace_back(argv[argi])) {
        return error;
      }
    }
    return parse_args(args);
  }

  /// @brief Returns the positional arguments that were stored if the
  /// ACCEPT_POSITIONAL option was set
  ///
  /// @return View into the array of positional arguments.
  cargo::array_view<cargo::string_view> get_positional_args() {
    return PositionalArgs;
  }

  /// @brief Returns the unrecognized arguments that were stored if the
  /// KEEP_UNRECOGNIZED option was set
  ///
  /// @return View into the array of unrecognized arguments.
  cargo::array_view<cargo::string_view> get_unrecognized_args() {
    return UnrecognizedArgs;
  }

 private:
  cargo::small_vector<argument, N> Args;
  cargo::small_vector<cargo::string_view, NP> PositionalArgs;
  cargo::small_vector<cargo::string_view, NU> UnrecognizedArgs;
  argument_parser_option_bitset Options;
};

/// @}
}  // namespace cargo

#endif  // CARGO_ARGUMENT_PARSER_H_INCLUDED
