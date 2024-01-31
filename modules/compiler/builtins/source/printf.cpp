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

#include <builtins/printf.h>

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

// TODO: Should we have a CA_ASSERT, if so where should it live?
#ifndef NDEBUG
#define ASSERT(CONDITION, MESSAGE)                                      \
  if (CONDITION) {                                                      \
    (void)fprintf(stderr, "%s: %d: %s\n", __FILE__, __LINE__, MESSAGE); \
    std::abort();                                                       \
  }                                                                     \
  (void)0
#else
#define ASSERT(CONDITION, MESSAGE)
#endif

namespace {

/// @brief Utility function to find the next specifier in a given string,
/// starting at a given position in the string, but skip %% specifiers. The
/// format string is assumed to be valid.
///
/// @param[in] str String in which to search for the specifier.
/// @param[in] pos Position at which to start searching for a specifier
/// (defaults to 0).
/// @return The index at which the specifier was found, or the index of the
/// last character if no specifier was found.
size_t FindNextSpecifier(const std::string &str, size_t pos = 0) {
  for (;;) {
    pos = str.find_first_of('%', (pos == 0) ? pos : pos + 1);
    if (std::string::npos == pos) {
      return str.size();
    }
    if ((pos + 1 < str.size()) && ('%' == str[pos + 1])) {
      ++pos;
      continue;
    }
    return pos;
  }
}

/// @brief Parse a valid printf specifier.
///
/// @param[in] str String with a specifier at the begining.
/// @param[in] pos the position of the specifier in the format string.
/// @param[out] w Width specified by the specifier string.
/// @param[out] p Precision specified by the specifier string.
/// @param[out] minus True if the '-' flag is used, otherwise false.
/// @param[out] plus True if the '+' flag is used, otherwise false.
/// @param[out] space True if the ' ' flag is used, otherwise false.
/// @param[out] alternate True if the '#' flag is used, otherwise false.
///
/// @return Index of the first character not part of the specifier in the
/// string.
size_t ParseSpecifier(std::string str, size_t pos, size_t &w, size_t &p,
                      bool &minus, bool &plus, bool &space, bool &alternate) {
  size_t i = pos + 1;  // for the %
  minus = false;
  plus = false;
  space = false;
  alternate = false;
  // eat up the flags
  while (strchr("-+ #0", str[i])) {
    if ('-' == str[i]) {
      minus = true;
    } else if ('+' == str[i]) {
      plus = true;
    } else if (' ' == str[i]) {
      space = true;
    } else if ('#' == str[i]) {
      alternate = true;
    }
    ++i;
  }

  // eat up the width
  std::string width;
  if ('*' == str[i]) {
    ++i;
  } else {
    while (isdigit(str[i])) {
      width += str[i];
      ++i;
    }
  }

  if (width.size() != 0) {
    w = std::stoul(width);
  } else {
    w = 0;
  }

  // eat up precision
  std::string precision;
  if ('.' == str[i]) {
    ++i;
    while (isdigit(str[i])) {
      precision += str[i];
      ++i;
    }
  }

  if (precision.size() != 0) {
    p = std::stoul(precision);
  } else {
    p = 0;
  }

  // eat the length modifier
  while (('h' == str[i]) || ('l' == str[i])) {
    ++i;
  }

  // eat the conversion specifier
  ++i;

  return i;
}

/// @brief Function used to print floating point values.
///
/// It uses the system `printf` for formatting unless the floating point is a
/// NaN or infinity in which case it prints them with the formatting mandated
/// by the OpenCL 1.2 specification.
///
/// @param[in] partial String starting with a valid `printf` format specifier
/// and containing only one `printf` specifier.
/// @param[in] d The floating point valule to print.
template <typename T>
void PrintFloatingPoint(std::string partial, T d) {
#if defined(__MINGW32__) || defined(__MINGW64__)
  // MinGW seems to follow MSVC pre-2015 formatting for %e/%E/%g/%G which did
  // not match the C++11 specification.  There is however, this method to set
  // conformant printing.  This method was removed in MSVC 2015 because it
  // became conformant by default, so if a future version of MinGW removes this
  // function then it is probably no longer required.
  unsigned int old_printf_format = _set_output_format(_TWO_DIGIT_EXPONENT);
#endif

  // manually format NaNs and Infinity as the system
  // printf doesn't format them properly on windows
  if (std::isnan(d) || std::isinf(d)) {
    std::string f;
    size_t min_width;
    size_t precision;
    bool minus;
    bool plus;
    bool space;
    bool alternate;
    const size_t start = FindNextSpecifier(partial);
    const size_t end = ParseSpecifier(partial, start, min_width, precision,
                                      minus, plus, space, alternate);

    // add the sign or the sign padding
    if (std::signbit(d)) {
      f += "-";
    } else if (plus) {
      f += "+";
    } else if (space) {
      f += " ";
    }

    if (std::isupper(partial[end - 1])) {
      f += std::isnan(d) ? "NAN" : "INF";
    } else {
      f += std::isnan(d) ? "nan" : "inf";
    }

    // add padding if necessary
    while (f.size() < min_width) {
      if (minus) {  // left justified
        f += " ";
      } else {  // right justified
        f.insert(0, " ");
      }
    }
    partial.replace(start, end, f);
    // KLOCWORK "NNTS.MUST" possible false positive
    // Klocwork doesn't realize c_str() returns a null-terminated string
    ::printf("%s", partial.c_str());
#if defined(__MINGW32__) || defined(__MINGW64__)
  } else if (static_cast<T>(0) == d) {
    // On MinGW 5.3 (other versions untested) calling printf on the value 0.0
    // with either the %a or %A format specifiers causes the printf function to
    // never return.  Work around this by special casing 0.0 (or -0.0) inputs
    // here.  If we find that the specifier was something other than %a or %A
    // we just fall back to printf.  For '%a' and '%A' we manually implement
    // the interpretation of the format string, but in a way that only works
    // for the values 0.0 or -0.0.
    size_t min_width;
    size_t precision;
    bool minus;
    bool plus;
    bool space;
    bool alternate;
    size_t start = FindNextSpecifier(partial);
    size_t end = ParseSpecifier(partial, start, min_width, precision, minus,
                                plus, space, alternate);

    if (partial[end - 1] == 'a' || partial[end - 1] == 'A') {
      std::string f;
      bool is_upper = partial[end - 1] == 'A';

      // add the sign or the sign padding
      if (std::signbit(d)) {
        f += "-";
      } else if (plus) {
        f += "+";
      } else if (space) {
        f += " ";
      }

      f += is_upper ? "0X0" : "0x0";

      if (precision > 0 || alternate) {
        f += ".";
      }

      while (precision > 0) {
        f += "0";
        precision--;
      }

      f += is_upper ? "P+0" : "p+0";

      // add padding if necessary
      while (f.size() < min_width) {
        if (minus) {  // left justified
          f += " ";
        } else {  // right justified
          f = " " + f;
        }
      }

      partial.replace(start, end, f);
      ::printf("%s", partial.c_str());
    } else {
      // All other cases work fine with MinGW printf.
      ::printf(partial.c_str(), d);
    }
#endif
  } else {
    // KLOCWORK "SV.FMTSTR.GENERIC" possible false positive
    // Possible vulnerability when the printf format string comes from the user
    ::printf(partial.c_str(), d);
  }

#if defined(__MINGW32__) || defined(__MINGW64__)
  _set_output_format(old_printf_format);
#endif
}
}  // namespace

void builtins::printf::print(uint8_t *pack, size_t max_length,
                             const std::vector<descriptor> &printf_calls,
                             std::vector<uint32_t> &group_offsets) {
  uint8_t *data;
  const size_t num_groups = group_offsets.size();
  for (size_t i = 0; i < num_groups; ++i) {
    data = pack + i * max_length;

    // retrieve the size in bytes written to the printf buffer for
    // the work item
    uint32_t size;
    std::memcpy(&size, data, 4);
    uint32_t read = 4;  // the 4 bytes for the size

    // read the amount of overflow bytes in size
    uint32_t overflow;
    std::memcpy(&overflow, data + read, 4);
    read += 4;

    if (size < overflow) {
      ASSERT(size >= overflow,
             "The printf buffer stored length is smaller than the stored "
             "overflow length, the printf buffer is likely to be corrupt.");
      return;
    }

    // adjust the size to ignore overflow
    size = size - overflow;

    if (size > max_length) {
      ASSERT(size <= max_length,
             "The printf buffer stored length is bigger than the size of "
             "the buffer, the buffer is likely to be corrupt.");
      return;
    }

    // If we've printed from this buffer before, start after the previous data
    // printed
    read = (group_offsets[i] == 0) ? read : group_offsets[i];
    while (read < size) {
      // read the id of the printf call
      uint32_t id;
      std::memcpy(&id, data + read, 4);
      read += 4;

      if (id >= printf_calls.size()) {
        ASSERT(id < printf_calls.size(),
               "The printf call id does not match a printf descriptor.");
        return;
      }

      // get the printf descriptor matching this printf call
      const builtins::printf::descriptor &printf_desc = printf_calls[id];

      // if the call doesn't have any parameters just print the format string
      if (printf_desc.types.size() == 0) {
        ::printf("%s", printf_desc.format_string.c_str());
        continue;
      }

      // if the printf call has parameters we unpack them from the buffer and
      // print them as we go, to do that we also have to split the format string
      // in pieces containing only one specifier at a time.
      std::string partial;
      size_t previous = 0;
      size_t stringindex = 0;

      // get the position of the first specifier
      size_t pos = FindNextSpecifier(printf_desc.format_string);

      for (auto ty : printf_desc.types) {
        // find the position of the next specifier
        pos = FindNextSpecifier(printf_desc.format_string, pos + 1);

        // the string we will use for printing contains a specifier and stops
        // either just before the next specifier or at the end of the string
        partial = printf_desc.format_string.substr(previous, pos - previous);
        previous = pos;

        // unpack and print the argument
        // KLOCWORK "SV.FMTSTR.GENERIC" possible false positive
        // Possible vulnerability when the printf format string comes from the
        // user
        switch (ty) {
          case builtins::printf::type::DOUBLE:
            double d;
            std::memcpy(&d, data + read, 8);
            PrintFloatingPoint(partial, d);
            read += 8;
            break;
          case builtins::printf::type::FLOAT:
            float f;
            std::memcpy(&f, data + read, 4);
            PrintFloatingPoint(partial, f);
            read += 4;
            break;
          case builtins::printf::type::LONG:
            uint64_t l;
            std::memcpy(&l, data + read, 8);
            ::printf(partial.c_str(), l);
            read += 8;
            break;
          case builtins::printf::type::INT:
            uint32_t i;
            std::memcpy(&i, data + read, 4);
            ::printf(partial.c_str(), i);
            read += 4;
            break;
          case builtins::printf::type::SHORT:
            uint16_t s;
            std::memcpy(&s, data + read, 2);
            ::printf(partial.c_str(), s);
            read += 2;
            break;
          case builtins::printf::type::CHAR:
            uint8_t c;
            std::memcpy(&c, data + read, 1);
            ::printf(partial.c_str(), c);
            read += 1;
            break;
          case builtins::printf::type::STRING:
            ::printf(partial.c_str(), printf_desc.strings[stringindex].c_str());
            ++stringindex;
            break;
        }
      }
    }
    // Set offset to start printing from next time this function is called
    group_offsets[i] = read;
  }
}
