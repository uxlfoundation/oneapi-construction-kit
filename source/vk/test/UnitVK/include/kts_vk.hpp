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

#ifndef UNITVK_KTS_VK_HPP_INCLUDED
#define UNITVK_KTS_VK_HPP_INCLUDED

template <typename T>
struct Validator<glsl::glsl_vec2<T>> {
  bool validate(glsl::glsl_vec2<T> &expected, glsl::glsl_vec2<T> &actual) {
    Validator<T> v;
    return v.validate(expected.data[0], actual.data[0]) &&
           v.validate(expected.data[1], actual.data[1]);
  }

  void print(std::stringstream &s, const glsl::glsl_vec2<T> &value) {
    s << value;
  }
};

template <typename T>
struct Validator<glsl::glsl_vec4<T>> {
  bool validate(glsl::glsl_vec4<T> &expected, glsl::glsl_vec4<T> &actual) {
    Validator<T> v;
    return v.validate(expected.data[0], actual.data[0]) &&
           v.validate(expected.data[1], actual.data[1]) &&
           v.validate(expected.data[2], actual.data[2]) &&
           v.validate(expected.data[3], actual.data[3]);
  }

  void print(std::stringstream &s, const glsl::glsl_vec4<T> &value) {
    s << value;
  }
};

#endif  // UNITVK_KTS_VK_HPP_INCLUDED
