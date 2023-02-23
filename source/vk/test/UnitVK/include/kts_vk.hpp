// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
