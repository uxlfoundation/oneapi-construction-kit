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
/// @brief Various string manipulation algorithms.

#ifndef CARGO_STRING_H_INCLUDED
#define CARGO_STRING_H_INCLUDED

#include <cargo/optional.h>
#include <cargo/string_view.h>

#include <algorithm>
#include <vector>

namespace cargo {
/// @addtogroup cargo
/// @{

/// @brief Split the given string at any of the given string delimiter.
///
/// When `string` contains consecutive occurances of `delimiter` these will be
/// merged together so the resulting list will not contain empty strings, if
/// this behaviour is undesirable see `cargo::split_all`.
///
/// @param string View of string to be split based on delimiter.
/// @param delimiter View of string to split the string on.
///
/// @return Returns a vector of split string views.
inline std::vector<cargo::string_view> split(cargo::string_view string,
                                             cargo::string_view delimiter) {
  if (string.empty()) {
    return {};
  }
  std::vector<cargo::string_view> items;
  size_t first = 0, last;
  do {
    last = string.find(delimiter, first);
    if (first != last) {
      items.push_back(
          *string.substr(first, std::min(last, string.size()) - first));
    }
    first = last + delimiter.size();
  } while (first < string.size() && last != cargo::string_view::npos);
  return items;
}

/// @brief Split the given string at any of the given string delimiter.
///
/// When `string` contains consecutive occurances of `delimiter` the resulting
/// list will contain empty strings, if this behaviour is undesirable see
/// `cargo::split`.
///
/// @param string View of string to be split based on delimiter.
/// @param delimiter View of string to split the string on.
///
/// @return Returns a vector of split string views.
inline std::vector<cargo::string_view> split_all(cargo::string_view string,
                                                 cargo::string_view delimiter) {
  if (string.empty()) {
    return {};
  }
  std::vector<cargo::string_view> items;
  size_t first = 0, last;
  do {
    last = string.find(delimiter, first);
    items.push_back(
        *string.substr(first, std::min(last, string.size()) - first));
    first = last + delimiter.size();
    if (first >= string.size()) {
      items.emplace_back();
      break;
    }
  } while (last != cargo::string_view::npos);
  return items;
}

/// @brief Split the given string at any of the given character delimiters.
///
/// When `string` contains consecutive occurances of one of `delimiters` these
/// will be merged together so the resulting list will not contain empty
/// strings, if this behaviour is undesirable see `cargo::split_all_of`.
///
/// @param string View of string to be split based on delimiters.
/// @param delimiters View of set of character delimiters to split the string
/// on.
///
/// @return Returns a vector of split string views.
inline std::vector<cargo::string_view> split_of(
    cargo::string_view string, cargo::string_view delimiters = " \t\n\v\f\r") {
  if (string.empty()) {
    return {};
  }
  std::vector<cargo::string_view> items;
  size_t first = 0, last;
  do {
    first = string.find_first_not_of(delimiters, first);
    last = string.find_first_of(delimiters, first);
    if (first != last) {
      items.push_back(*string.substr(first, last - first));
    }
    first = last + 1;
  } while (first < string.size() && last != cargo::string_view::npos);
  return items;
}

/// @brief Split the given string at any of the given delimiter characters.
///
/// When `string` contains consecutive occurances of one of `delimiters` the
/// resulting list will contain empty strings, if this behaviour is undesirable
/// see `cargo::split_of`.
///
/// @param string View of string to be split based on delimiters.
/// @param delimiters View of set of character delimiters to split the string
/// on.
///
/// @return Returns a vector of split string views.
inline std::vector<cargo::string_view> split_all_of(
    cargo::string_view string, cargo::string_view delimiters = " \t\n\v\f\r") {
  if (string.empty()) {
    return {};
  }
  std::vector<cargo::string_view> items;
  size_t first = 0, last;
  do {
    last = string.find_first_of(delimiters, first);
    items.push_back(*string.substr(first, last - first));
    first = last + 1;
    if (first >= string.size()) {
      items.emplace_back();
      break;
    }
  } while (last != cargo::string_view::npos);
  return items;
}

/// @brief Split the given string at any of the given character delimiters,
/// allowing quoted arguments with embedded delimiters.
///
/// When `string` contains consecutive occurances of one of `delimiters` these
/// will be merged together so the resulting list will not contain empty
/// strings, unless explicitly forced by quoting an empty string.
///
/// @param string View of string to be split based on delimiters.
/// @param delimiters View of set of character delimiters to split the string
/// on.
/// @param quotes View of the set of quoting characters - note that a quoted
/// string must be delimited by the same quoting character from both sides.
///
/// @return Returns a vector of split string views.
inline std::vector<cargo::string_view> split_with_quotes(
    cargo::string_view string, cargo::string_view delimiters = " \t\n\v\f\r",
    cargo::string_view quotes = "'\"") {
  if (string.empty()) {
    return {};
  }
  if (delimiters.empty()) {  // assume no splitting requested
    return {{string}};
  }
  std::vector<cargo::string_view> items;
  cargo::optional<char> quote;
  bool prev_was_delimiter = true;
  size_t first = 0, last = 0;
  // appends the range string[first...last] as an argument and moves the indices
  // to just after the argument.
  auto terminate_argument = [&string, &items, &first, &last]() {
    auto substr = string.substr(first, 1 + last - first);
    // the fact that 0 < first < last < string.size() is an invariant of the
    // main loop.
    CARGO_ASSERT(substr, "Indices out of bounds in split_with_quoting");
    items.push_back(*substr);
    first = last + 1;
    last = first;
  };
  do {
    char current = string[last];
    if (quote) {
      if (current == *quote) {
        last--;                // don't include the quote in the argument
        terminate_argument();  // advances last to quote
        last++;                // so skip it
        first = last;
        quote.reset();
        // prevent adding an unnecessary empty argument
        prev_was_delimiter = true;
      } else {
        last++;
      }
    } else {
      if (prev_was_delimiter &&
          quotes.find_first_of(current) != cargo::string_view::npos) {
        // store the quote type and skip it in the argument
        quote = current;
        first++;
        last = first;
        prev_was_delimiter = false;
      } else if (delimiters.find_first_of(current) !=
                 cargo::string_view::npos) {
        if (!prev_was_delimiter) {
          last--;  // don't include delimiter in the argument
          terminate_argument();
          prev_was_delimiter = true;
        }
        // skip delimiters
        first++;
        last = first;
      } else {
        last++;
        prev_was_delimiter = false;
      }
    }
  } while (last < string.size());
  if (first != last) {
    terminate_argument();
  }
  return items;
}

/// @brief Join a range of strings with a delimiter.
///
/// @tparam Iterator Type of the range iterator.
/// @param first Iterator pointing to the first string to join.
/// @param last Iterator pointing to one past the last string to join.
/// @param delimiter View of the delimiter string to join.
///
/// @return Returns the joined range of strings.
template <class Iterator>
inline std::string join(Iterator first, Iterator last,
                        cargo::string_view delimiter) {
  if (first == last) {
    return {};
  }
  std::string joined;
  cargo::string_view item(*first++);
  joined.append(item.begin(), item.end());
  for (; first != last; first++) {
    joined.append(delimiter.begin(), delimiter.end());
    item = cargo::string_view(*first);
    joined.append(item.begin(), item.end());
  }
  return joined;
}

/// @brief Remove white space from the left of a string.
///
/// @param string String to trim white space on the left.
/// @param delimiters Set of character delimiters to trim from the string.
///
/// @return Returns a new string trimmed on the left.
inline cargo::string_view trim_left(
    cargo::string_view string, cargo::string_view delimiters = " \t\n\v\f\r") {
  string.remove_prefix(
      std::min(string.find_first_not_of(delimiters), string.size()));
  return string;
}

/// @brief Remove white space from the right of a string.
///
/// @param string String to trim white space on the right.
/// @param delimiters Set of character delimiters to trim from the string.
///
/// @return Returns a new string trimmed on the right.
inline cargo::string_view trim_right(
    cargo::string_view string, cargo::string_view delimiters = " \t\n\v\f\r") {
  string.remove_suffix(string.size() -
                       (string.find_last_not_of(delimiters) + 1));
  return string;
}

/// @brief Remove white space from the left and right of a string.
///
/// @param string String to trim white space on the left and right.
/// @param delimiters Set of character delimiters to trim from the string.
///
/// @return Returns a new string trimmed on the left and right.
inline cargo::string_view trim(cargo::string_view string,
                               cargo::string_view delimiters = " \t\n\v\f\r") {
  return trim_left(trim_right(string, delimiters), delimiters);
}

/// @}
}  // namespace cargo

#endif  // CARGO_STRING_H_INCLUDED
