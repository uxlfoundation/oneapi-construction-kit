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
#include <cl/binary/argument.h>
#include <cl/macros.h>

#include <regex>
#include <string>
#include <vector>

// Concatenate stuff into a std::string, and extract the C string
// TODO: CA-1648: Clean up assert message generation
#define CA_CAT(MESSAGE) (std::string() + (MESSAGE)).c_str()

namespace cl {
namespace binary {

std::pair<compiler::ArgumentType, std::string>
getArgumentTypeFromParameterTypeString(const cargo::string_view &str) {
  compiler::ArgumentType type_info;

  auto words_v = cargo::split(str, " ");

  // Expecting `str` to be at least `type`
  OCL_ASSERT(words_v.size() >= 1, CA_CAT("Expected at least 1 word in '" +
                                         cargo::as<std::string>(str) + "'"));

  // The last word is the type string
  auto &type_str = words_v.back();

  // If the type is unsigned, store a 'u' in the type name
  std::string type_name_str;
  if (type_str.starts_with('u')) {
    type_name_str = 'u';
    type_str.remove_prefix(1);  // Remove the 'u'
  } else if (words_v.size() >= 2 &&
             !(words_v.crend() - 2)->compare("unsigned")) {
    type_name_str = 'u';
  }

  // Store the rest of the type name including vector width (if present)
  type_name_str += cargo::as<std::string>(type_str);

  // Check for vector widths
  static const std::regex trailing_num_re(R"((\d+)$)", std::regex::optimize);
  std::match_results<cargo::string_view::const_iterator> match;
  if (std::regex_search(type_str.cbegin(), type_str.cend(), match,
                        trailing_num_re)) {
    type_info.vector_width = std::stoi(match.str(1));
    type_str.remove_suffix(match.str(1).size());
    OCL_ASSERT(2 == type_info.vector_width || 3 == type_info.vector_width ||
                   4 == type_info.vector_width || 8 == type_info.vector_width ||
                   16 == type_info.vector_width,
               CA_CAT("Found illegal vector width in '" +
                      cargo::as<std::string>(type_str) + "'"));
  }

  // If someone has a lot of free time, they could go through and order these
  // types based on the likelihood of them appearing as kernel arguments
  if (!type_str.compare("half")) {
    if (type_info.vector_width == 1) {
      type_info.kind = compiler::ArgumentKind::HALF;
    } else if (type_info.vector_width == 2) {
      type_info.kind = compiler::ArgumentKind::HALF_2;
    } else if (type_info.vector_width == 3) {
      type_info.kind = compiler::ArgumentKind::HALF_3;
    } else if (type_info.vector_width == 4) {
      type_info.kind = compiler::ArgumentKind::HALF_4;
    } else if (type_info.vector_width == 8) {
      type_info.kind = compiler::ArgumentKind::HALF_8;
    } else if (type_info.vector_width == 16) {
      type_info.kind = compiler::ArgumentKind::HALF_16;
    }
  } else if (!type_str.compare("float")) {
    if (type_info.vector_width == 1) {
      type_info.kind = compiler::ArgumentKind::FLOAT;
    } else if (type_info.vector_width == 2) {
      type_info.kind = compiler::ArgumentKind::FLOAT_2;
    } else if (type_info.vector_width == 3) {
      type_info.kind = compiler::ArgumentKind::FLOAT_3;
    } else if (type_info.vector_width == 4) {
      type_info.kind = compiler::ArgumentKind::FLOAT_4;
    } else if (type_info.vector_width == 8) {
      type_info.kind = compiler::ArgumentKind::FLOAT_8;
    } else if (type_info.vector_width == 16) {
      type_info.kind = compiler::ArgumentKind::FLOAT_16;
    }
  } else if (!type_str.compare("double")) {
    if (type_info.vector_width == 1) {
      type_info.kind = compiler::ArgumentKind::DOUBLE;
    } else if (type_info.vector_width == 2) {
      type_info.kind = compiler::ArgumentKind::DOUBLE_2;
    } else if (type_info.vector_width == 3) {
      type_info.kind = compiler::ArgumentKind::DOUBLE_3;
    } else if (type_info.vector_width == 4) {
      type_info.kind = compiler::ArgumentKind::DOUBLE_4;
    } else if (type_info.vector_width == 8) {
      type_info.kind = compiler::ArgumentKind::DOUBLE_8;
    } else if (type_info.vector_width == 16) {
      type_info.kind = compiler::ArgumentKind::DOUBLE_16;
    }
  } else if (!type_str.compare("char")) {
    if (type_info.vector_width == 1) {
      type_info.kind = compiler::ArgumentKind::INT8;
    } else if (type_info.vector_width == 2) {
      type_info.kind = compiler::ArgumentKind::INT8_2;
    } else if (type_info.vector_width == 3) {
      type_info.kind = compiler::ArgumentKind::INT8_3;
    } else if (type_info.vector_width == 4) {
      type_info.kind = compiler::ArgumentKind::INT8_4;
    } else if (type_info.vector_width == 8) {
      type_info.kind = compiler::ArgumentKind::INT8_8;
    } else if (type_info.vector_width == 16) {
      type_info.kind = compiler::ArgumentKind::INT8_16;
    }
  } else if (!type_str.compare("short")) {
    if (type_info.vector_width == 1) {
      type_info.kind = compiler::ArgumentKind::INT16;
    } else if (type_info.vector_width == 2) {
      type_info.kind = compiler::ArgumentKind::INT16_2;
    } else if (type_info.vector_width == 3) {
      type_info.kind = compiler::ArgumentKind::INT16_3;
    } else if (type_info.vector_width == 4) {
      type_info.kind = compiler::ArgumentKind::INT16_4;
    } else if (type_info.vector_width == 8) {
      type_info.kind = compiler::ArgumentKind::INT16_8;
    } else if (type_info.vector_width == 16) {
      type_info.kind = compiler::ArgumentKind::INT16_16;
    }
  } else if (!type_str.compare("int")) {
    if (type_info.vector_width == 1) {
      type_info.kind = compiler::ArgumentKind::INT32;
    } else if (type_info.vector_width == 2) {
      type_info.kind = compiler::ArgumentKind::INT32_2;
    } else if (type_info.vector_width == 3) {
      type_info.kind = compiler::ArgumentKind::INT32_3;
    } else if (type_info.vector_width == 4) {
      type_info.kind = compiler::ArgumentKind::INT32_4;
    } else if (type_info.vector_width == 8) {
      type_info.kind = compiler::ArgumentKind::INT32_8;
    } else if (type_info.vector_width == 16) {
      type_info.kind = compiler::ArgumentKind::INT32_16;
    }
  } else if (!type_str.compare("long")) {
    if (type_info.vector_width == 1) {
      type_info.kind = compiler::ArgumentKind::INT64;
    } else if (type_info.vector_width == 2) {
      type_info.kind = compiler::ArgumentKind::INT64_2;
    } else if (type_info.vector_width == 3) {
      type_info.kind = compiler::ArgumentKind::INT64_3;
    } else if (type_info.vector_width == 4) {
      type_info.kind = compiler::ArgumentKind::INT64_4;
    } else if (type_info.vector_width == 8) {
      type_info.kind = compiler::ArgumentKind::INT64_8;
    } else if (type_info.vector_width == 16) {
      type_info.kind = compiler::ArgumentKind::INT64_16;
    }
  } else {  // "other" type or an error
    if (!type_str.compare("image1d_array_t")) {
      type_info.kind = compiler::ArgumentKind::IMAGE1D_ARRAY;
    } else if (!type_str.compare("image1d_buffer_t")) {
      type_info.kind = compiler::ArgumentKind::IMAGE1D_BUFFER;
    } else if (!type_str.compare("image1d_t")) {
      type_info.kind = compiler::ArgumentKind::IMAGE1D;
    } else if (!type_str.compare("image2d_array_t")) {
      type_info.kind = compiler::ArgumentKind::IMAGE2D_ARRAY;
    } else if (!type_str.compare("image2d_t")) {
      type_info.kind = compiler::ArgumentKind::IMAGE2D;
    } else if (!type_str.compare("image3d_t")) {
      type_info.kind = compiler::ArgumentKind::IMAGE3D;
    } else if (!type_str.compare("sampler_t")) {
      type_info.kind = compiler::ArgumentKind::SAMPLER;
    } else {
      OCL_ASSERT(false, CA_CAT("Unknown type '" +
                               cargo::as<std::string>(type_str) + "'"));
    }
  }

  return std::make_pair(type_info, type_name_str);
}

}  // namespace binary
}  // namespace cl
