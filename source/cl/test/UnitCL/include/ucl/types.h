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
/// @brief OpenCL scalar and vector type wrappers.
///
/// OpenCL defines 3-element vectors as a `typedef` to 4-element vector types
/// and half precision float types as `typedef`s to short integer types. This
/// causes problems with function overloading and template specialization, the
/// wrappers in this header aim to mitigate these issues.

#ifndef UNITCL_TYPES_H_INCLUDED
#define UNITCL_TYPES_H_INCLUDED

#include <CL/cl.h>
#include <gtest/gtest.h>
#include <ucl/assert.h>

#include <algorithm>
#include <initializer_list>
#include <ostream>
#include <type_traits>

namespace ucl {
/// @brief OpenCL scalar type wrapper class template.
///
/// Arithmetic operators are **not** defined. This wrapper's primary goal is to
/// provide an easy to initialize storage type which is uniquely identifiable
/// by the type system.
///
/// Comparison operators are defined for easy integration with GoogleTest
/// macros and validation code.
///
/// An output stream operator is defined for the class template for ease of
/// printing the values contained within, e.g. in error messages.
///
/// @tparam T OpenCL vector type to wrap.
/// @tparam Tag Tag type to disambiguate `cl_half` from `cl_short`.
template <class T, class Tag>
struct ScalarType {
  using cl_type = T;
  using value_type = T;

  ScalarType() = default;
  ScalarType(const ScalarType &) = default;
  ScalarType(ScalarType &&) = default;

  /// @brief Construct with an OpenCL scalar type.
  ///
  /// @param[in] value Value to construct with.
  ScalarType(const cl_type &value) : Value(value) {}

  ScalarType &operator=(const ScalarType &) = default;
  ScalarType &operator=(ScalarType &&) = default;

  /// @brief Assign with an OpenCL scalar type.
  ///
  /// @param[in] value Value to assign with.
  ///
  /// @return Returns a reference to this object.
  ScalarType &operator=(const cl_type &value) {
    new (this) ScalarType(value);
    return *this;
  }

  /// @brief Access the OpenCL scalar storage.
  ///
  /// @return Returns a reference to OpenCL scalar.
  cl_type &value() { return Value; }

  /// @brief Access the OpenCL scalar storage.
  ///
  /// @return Returns a `const` reference to OpenCL scalar.
  const cl_type &value() const { return Value; }

  /// @brief Implicit conversion to the OpenCL scalar type.
  ///
  /// @return Returns a copy of the storage OpenCL scalar.
  operator cl_type() const { return Value; }

  /// @brief API type name of the OpenCL scalar type.
  ///
  /// @return Returns a string containing the OpenCL API type name.
  static std::string api_name() {
    return testing::internal::GetTypeName<ScalarType>();
  }

  /// @brief OpenCL C source type name of the OpenCL scalar type.
  ///
  /// @return Returns a string containing the OpenCL source type name.
  static std::string source_name() {
    auto Name = api_name();
    Name.erase(0, 3);
    return Name;
  }

 private:
  cl_type Value;
};

/// @brief Eqaulity comparison operator for ScalarType.
///
/// @param[in] left Value on the left hand side of the operator.
/// @param[in] right Value on the right hand side of the operator.
///
/// @return Returns `true` if the values are equal, `false` otherwise.
template <class T, class Tag>
inline bool operator==(const ScalarType<T, Tag> &left,
                       const ScalarType<T, Tag> &right) {
  return left.value() == right.value();
}

/// @brief Negative equality comparison operator for ScalarType.
///
/// @param[in] left Value on the left hand side of the operator.
/// @param[in] right Value on the right hand side of the operator.
///
/// @return Returns `true` if the values are not equal, `false` otherwise.
template <class T, class Tag>
inline bool operator!=(const ScalarType<T, Tag> &left,
                       const ScalarType<T, Tag> &right) {
  return left.value() != right.value();
}

/// @brief Less than comparison operator for ScalarType.
///
/// @param[in] left Value on the left hand side of the operator.
/// @param[in] right Value on the right hand side of the operator.
///
/// @return Returns `true` if `left` is less than `right`, `false` otherwise.
template <class T, class Tag>
inline bool operator<(const ScalarType<T, Tag> &left,
                      const ScalarType<T, Tag> &right) {
  return left.value() < right.value();
}

/// @brief Less than or equal to comparison operator for ScalarType.
///
/// @param[in] left Value on the left hand side of the operator.
/// @param[in] right Value on the right hand side of the operator.
///
/// @return Returns `true` if `left` is less or equal to than `right`, `false`
/// otherwise.
template <class T, class Tag>
inline bool operator<=(const ScalarType<T, Tag> &left,
                       const ScalarType<T, Tag> &right) {
  return left.value() <= right.value();
}

/// @brief Greater than comparison operator for ScalarType.
///
/// @param[in] left Value on the left hand side of the operator.
/// @param[in] right Value on the right hand side of the operator.
///
/// @return Returns `true` if `left` is greater than `right`, `false`
/// otherwise.
template <class T, class Tag>
inline bool operator>(const ScalarType<T, Tag> &left,
                      const ScalarType<T, Tag> &right) {
  return left.value() > right.value();
}

/// @brief Greather than or equal to comparison operator for ScalarType.
///
/// @param[in] left Value on the left hand side of the operator.
/// @param[in] right Value on the right hand side of the operator.
///
/// @return Returns `true` if `left` is greater than or equal to than `right`,
/// `false` otherwise.
template <class T, class Tag>
inline bool operator>=(const ScalarType<T, Tag> &left,
                       const ScalarType<T, Tag> &right) {
  return left.value() >= right.value();
}

/// @brief OpenCL vector type wrapper class template.
///
/// The OpenCL headers define 3-element vectors as an alias to 4-element
/// vectors. This is problematic for function overloading and template
/// specialization because the 4th element in a 3-element may be checked for an
/// expected value when its value is not expected to be defined.
///
/// Arithmetic operators are **not** defined. This wrapper's primary goal is to
/// provide an easy to initialize storage type which is uniquely identifiable
/// by the type system.
///
/// Comparison operators are defined for easy integration with GoogleTest
/// macros and validation code.
///
/// An output stream operator is defined for the class template for ease of
/// printing the values contained within, e.g. in error messages.
///
/// @tparam T OpenCL vector type to wrap.
/// @tparam N Number of accessible element in the vector type.
/// @tparam Tag Tag type to disambiguate `cl_half<n>` from `cl_short<n>`.
template <class T, size_t N, class Tag>
struct VectorType {
  using cl_type = T;
  using value_type =
      std::remove_reference_t<decltype(std::declval<cl_type>().s[0])>;
  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using iterator = pointer;
  using const_iterator = const_pointer;

  VectorType() = default;
  VectorType(const VectorType &) = default;
  VectorType(VectorType &&) = default;

  /// @brief Construct with the same scalar value in all elements.
  ///
  /// @param[in] value Value to construct all vector elements.
  explicit VectorType(const_reference value) {
    std::fill(begin(), end(), value);
  }

  /// @brief Construct from an OpenCL vector type.
  ///
  /// @param[in] vector OpenCL vector to copy construct from.
  explicit VectorType(const cl_type &vector) : Vector(vector) {}

  /// @brief Construct from an initializer list of values.
  ///
  /// @param[in] ilist List of values to construct from.
  VectorType(std::initializer_list<value_type> ilist) {
    UCL_ASSERT(ilist.size() == N, "invalid initializer list size");
    std::copy(ilist.begin(), ilist.end(), begin());
  }

  /// @brief Construct from a std::vector of values.
  ///
  /// @param[in] buffer Vector of values to construct from.
  VectorType(const std::vector<value_type> &buffer) {
    UCL_ASSERT(buffer.size() == N, "invalid vector size");
    std::copy(std::begin(buffer), std::end(buffer), std::begin(*this));
  }

  VectorType &operator=(const VectorType &) = default;
  VectorType &operator=(VectorType &&) = default;

  /// @brief Assign with the same scalar value in all elements.
  ///
  /// @param[in] value Value to assign to all vector elements.
  ///
  /// @return Returns a reference to this object.
  VectorType &operator=(const_reference value) {
    new (this) VectorType(value);
    return *this;
  }

  /// @brief Assign from an OpenCL vector type.
  ///
  /// @param[in] vector OpenCL vector to copy assign from.
  ///
  /// @return Returns a reference to this object.
  VectorType &operator=(const cl_type &vector) {
    new (this) VectorType(vector);
    return *this;
  }

  /// @brief Assign from an initializer list of values.
  ///
  /// @param[in] ilist List of values to copy assign from.
  ///
  /// @return Returns a reference to this object.
  VectorType &operator=(std::initializer_list<value_type> ilist) {
    new (this) VectorType(std::move(ilist));
    return *this;
  }

  /// @brief Access a vector element by index.
  ///
  /// @param[in] index Index of the element to access.
  ///
  /// @return Returns a reference to the vector element.
  reference operator[](size_t index) {
    UCL_ASSERT(index < N, "index out of bounds");
    return Vector.s[index];
  }

  /// @brief Access a vector element by index.
  ///
  /// @param[in] index Index of the element to access.
  ///
  /// @return Returns a `const` reference to the vector element.
  const_reference operator[](size_t index) const {
    UCL_ASSERT(index < N, "index out of bounds");
    return Vector.s[index];
  }

  /// @brief Access the OpenCL vector storage.
  ///
  /// @return Returns a reference to the OpenCL vector.
  cl_type &value() { return Vector; }

  /// @brief Access the OpenCL vector storage.
  ///
  /// @return Returns a `const` reference to the OpenCL vector.
  const cl_type &value() const { return Vector; }

  /// @brief Directly access the vector data.
  ///
  /// @return Returns a pointer to the vector data.
  pointer data() { return Vector.s; }

  /// @brief Directly access the vector data.
  ///
  /// @return Returns a `const` pointer to the vector data.
  const_pointer data() const { return Vector.s; }

  /// @brief Access the first element.
  ///
  /// @return Returns a reference to the first element.
  reference front() { return Vector.s[0]; }

  /// @brief Access the first element.
  ///
  /// @return Returns a `const` reference to the first element.
  const_reference front() const { return Vector.s[0]; }

  /// @brief Access the last element.
  ///
  /// @return Returns a reference to the last element.
  reference back() { return Vector.s[N - 1]; }

  /// @brief Access the last element.
  ///
  /// @return Returns a `const` reference to the last element.
  const_reference back() const { return Vector.s[N - 1]; }

  /// @brief Iterator point to the beginning.
  ///
  /// @return Returns an iterator point to the beginning.
  iterator begin() { return Vector.s; }

  /// @brief Iterator point to the beginning.
  ///
  /// @return Returns an `const` iterator point to the beginning.
  const_iterator begin() const { return Vector.s; }

  /// @brief Iterator point to the end.
  ///
  /// @return Returns an iterator point to the end.
  iterator end() { return Vector.s + N; }

  /// @brief Iterator point to the end.
  ///
  /// @return Returns an `const` iterator point to the end.
  const_iterator end() const { return Vector.s + N; }

  /// @brief Number of accessible element in the vector.
  ///
  /// @return Returns the number of elements in the vector.
  static constexpr size_t size() { return N; }

  /// @brief Implicit conversion to the OpenCL vector type.
  ///
  /// @return Returns a copy of the stored OpenCL vector.
  operator cl_type() const { return Vector; }

  /// @brief API type name of the OpenCL vector type.
  ///
  /// @return Returns a string containing the OpenCL API type name.
  static std::string api_name() {
    return testing::internal::GetTypeName<VectorType>();
  }

  /// @brief OpenCL C source type name of the OpenCL vector type.
  ///
  /// @return Returns a string containing the OpenCL source type name.
  static std::string source_name() {
    auto Name = api_name();
    Name.erase(0, 3);
    return Name;
  }

 private:
  cl_type Vector;
};

/// @brief OpenCL 3-element vector wrapper for testing `vload3`/`vstore3`.
///
/// Provides a distinct type to be used in the Execution framework for testing
/// `vload3`/`vstore3` OpenCL builtins. The data held within this wrapper is
/// **not** packed, instead the Execution framework uses this type to
/// specialized the kts::MemoryAccessor<T>. This specialization loads and
/// stores the 3-element vector packed into unpadded contiguous memory which is
/// then passed as an argument to the kernel under test.
///
/// @tparam T OpenCL vector type to wrap.
template <class T, class Tag>
struct PackedVector3Type : VectorType<T, 3, Tag> {
  using cl_type = typename VectorType<T, 3, Tag>::cl_type;
  using value_type = typename VectorType<T, 3, Tag>::value_type;
  using reference = typename VectorType<T, 3, Tag>::reference;
  using const_reference = typename VectorType<T, 3, Tag>::const_reference;
  using pointer = typename VectorType<T, 3, Tag>::pointer;
  using const_pointer = typename VectorType<T, 3, Tag>::const_pointer;
  using iterator = typename VectorType<T, 3, Tag>::iterator;
  using const_iterator = typename VectorType<T, 3, Tag>::const_iterator;

  PackedVector3Type() = default;
  PackedVector3Type(const PackedVector3Type &other) = default;
  PackedVector3Type(PackedVector3Type &&other) = default;

  /// @brief Construct with the same scalar value in all elements.
  ///
  /// @param[in] value Value to construct all vector elements.
  explicit PackedVector3Type(const_reference value)
      : VectorType<T, 3, Tag>(value) {}

  /// @brief Construct from an OpenCL vector type.
  ///
  /// @param[in] vector OpenCL vector to copy construct from.
  explicit PackedVector3Type(const cl_type &vector)
      : VectorType<T, 3, Tag>(vector) {}

  /// @brief Construct from an initializer list of values.
  ///
  /// @param[in] ilist List of values to construct from.
  PackedVector3Type(std::initializer_list<value_type> ilist)
      : VectorType<T, 3, Tag>(ilist) {}

  PackedVector3Type &operator=(const PackedVector3Type &other) = default;
  PackedVector3Type &operator=(PackedVector3Type &&other) = default;

  /// @brief Assign with the same scalar value in all elements.
  ///
  /// @param[in] value Value to assign to all vector elements.
  ///
  /// @return Returns a reference to this object.
  PackedVector3Type &operator=(const_reference value) {
    new (this) PackedVector3Type(value);
    return *this;
  }

  /// @brief Assign from an OpenCL vector type.
  ///
  /// @param[in] vector OpenCL vector to copy assign from.
  ///
  /// @return Returns a reference to this object.
  PackedVector3Type &operator=(const cl_type &vector) {
    new (this) PackedVector3Type(vector);
    return *this;
  }

  /// @brief Assign from an initializer list of values.
  ///
  /// @param[in] ilist List of values to copy assign from.
  ///
  /// @return Returns a reference to this object.
  PackedVector3Type &operator=(std::initializer_list<value_type> ilist) {
    new (this) PackedVector3Type(std::move(ilist));
    return *this;
  }
};

/// @brief Eqaulity comparison operator for VectorType.
///
/// @param[in] left Value on the left hand side of the operator.
/// @param[in] right Value on the right hand side of the operator.
///
/// @return Returns `true` if the values are equal, `false` otherwise.
template <class T, size_t N, class Tag>
inline bool operator==(const VectorType<T, N, Tag> &left,
                       const VectorType<T, N, Tag> &right) {
  return std::equal(left.begin(), left.end(), right.begin());
}

/// @brief Negative eqaulity comparison operator for VectorType.
///
/// @param[in] left Value on the left hand side of the operator.
/// @param[in] right Value on the right hand side of the operator.
///
/// @return Returns `true` if the values are not equal, `false` otherwise.
template <class T, size_t N, class Tag>
inline bool operator!=(const VectorType<T, N, Tag> &left,
                       const VectorType<T, N, Tag> &right) {
  return !std::equal(left.begin(), left.end(), right.begin());
}

/// @brief Less than comparison operator for VectorType.
///
/// @param[in] left Value on the left hand side of the operator.
/// @param[in] right Value on the right hand side of the operator.
///
/// @return Returns `true` if `left` is less than `right`, `false` otherwise.
template <class T, size_t N, class Tag>
inline bool operator<(const VectorType<T, N, Tag> &left,
                      const VectorType<T, N, Tag> &right) {
  return std::lexicographical_compare(
      left.begin(), left.end(), right.begin(), right.end(),
      [](const T &a, const T &b) -> bool { return a < b; });
}

/// @brief Less than or equal to comparison operator for VectorType.
///
/// @param[in] left Value on the left hand side of the operator.
/// @param[in] right Value on the right hand side of the operator.
///
/// @return Returns `true` if `left` is less or equal to than `right`, `false`
/// otherwise.
template <class T, size_t N, class Tag>
inline bool operator<=(const VectorType<T, N, Tag> &left,
                       const VectorType<T, N, Tag> &right) {
  return std::lexicographical_compare(
      left.begin(), left.end(), right.begin(), right.end(),
      [](const T &a, const T &b) -> bool { return a <= b; });
}

/// @brief Greater than comparison operator for VectorType.
///
/// @param[in] left Value on the left hand side of the operator.
/// @param[in] right Value on the right hand side of the operator.
///
/// @return Returns `true` if `left` is greater than `right`, `false`
/// otherwise.
template <class T, size_t N, class Tag>
inline bool operator>(const VectorType<T, N, Tag> &left,
                      const VectorType<T, N, Tag> &right) {
  return std::lexicographical_compare(
      left.begin(), left.end(), right.begin(), right.end(),
      [](const T &a, const T &b) -> bool { return a > b; });
}

/// @brief Greater than or equal to comparison operator for VectorType.
///
/// @param[in] left Value on the left hand side of the operator.
/// @param[in] right Value on the right hand side of the operator.
///
/// @return Returns `true` if `left` is greater or equal to than `right`,
/// `false` otherwise.
template <class T, size_t N, class Tag>
inline bool operator>=(const VectorType<T, N, Tag> &left,
                       const VectorType<T, N, Tag> &right) {
  return std::lexicographical_compare(
      left.begin(), left.end(), right.begin(), right.end(),
      [](const T &a, const T &b) -> bool { return a >= b; });
}

#define TYPES(NAME, TYPE)                                        \
  struct NAME##Tag {};                                           \
  using NAME = ScalarType<TYPE, NAME##Tag>;                      \
  using NAME##2 = VectorType<TYPE##2, 2, NAME##Tag>;             \
  using NAME##3 = VectorType<TYPE##3, 3, NAME##Tag>;             \
  using Packed##NAME##3 = PackedVector3Type<TYPE##3, NAME##Tag>; \
  using NAME##4 = VectorType<TYPE##4, 4, NAME##Tag>;             \
  using NAME##8 = VectorType<TYPE##8, 8, NAME##Tag>;             \
  using NAME##16 = VectorType<TYPE##16, 16, NAME##Tag>

TYPES(Char, cl_char);
TYPES(UChar, cl_uchar);
TYPES(Short, cl_short);
TYPES(UShort, cl_ushort);
TYPES(Int, cl_int);
TYPES(UInt, cl_uint);
TYPES(Long, cl_long);
TYPES(ULong, cl_ulong);
TYPES(Half, cl_half);
TYPES(Float, cl_float);
TYPES(Double, cl_double);

#undef TYPES

/// @brief Output stream operator for ScalarType.
///
/// @param[in] out Output stream to write the value to.
/// @param[in] scalar Value to write to the output stream.
///
/// @return Returns a refernece to the output stream.
template <class T, class Tag>
inline std::ostream &operator<<(std::ostream &out,
                                const ScalarType<T, Tag> &scalar) {
  out << scalar.value();
  return out;
}

/// @brief Output stream operator for ScalarType<cl_half>.
///
/// @param[in] out Output stream to write the value to.
/// @param[in] scalar Value to write to the output stream.
///
/// @return Returns a refernece to the output stream.
template <>
inline std::ostream &operator<<(std::ostream &out,
                                const ScalarType<Half, HalfTag> &scalar);

/// @brief Output stream operator for VectorType.
///
/// @param[in] out Output stream to write the value to.
/// @param[in] vector Value to write to the output stream.
///
/// @return Returns a refernece to the output stream.
template <class T, size_t N, class Tag>
inline std::ostream &operator<<(std::ostream &out,
                                const VectorType<T, N, Tag> &vector) {
  out << '{';
  for (size_t index = 0; index < vector.size() - 1; index++) {
    out << vector[index] << ", ";
  }
  out << vector.back() << '}';
  return out;
}

/// @brief Output stream operator for VectorType<cl_half>.
///
/// @param[in] out Output stream to write the value to.
/// @param[in] vector Value to write to the output stream.
///
/// @return Returns a refernece to the output stream.
template <class T, size_t N>
inline std::ostream &operator<<(std::ostream &out,
                                const VectorType<T, N, HalfTag> &vector) {
  out << '{';
  for (size_t index = 0; index < vector.size() - 1; index++) {
    out << Half(vector[index]) << ", ";
  }
  out << vector.back() << '}';
  return out;
}

template <typename>
struct is_scalar : public std::false_type {};

template <typename ValTy, typename TagTy>
struct is_scalar<ucl::ScalarType<ValTy, TagTy>> : public std::true_type {};

template <typename>
struct is_vector : public std::false_type {};

template <typename T, std::size_t Size, typename TagTy>
struct is_vector<ucl::VectorType<T, Size, TagTy>> : public std::true_type {};

}  // namespace ucl

namespace testing {
namespace internal {
// WARNING: These are specializations of the testing::internal::GetTypeName()
// function template from googletest which returning "<type>" by default as we
// disable RTTI. This could is likely to break if the googletest API is
// changed.

template <>
inline std::string GetTypeName<ucl::Char>() {
  return "cl_char";
}
template <>
inline std::string GetTypeName<ucl::Char2>() {
  return "cl_char2";
}
template <>
inline std::string GetTypeName<ucl::Char3>() {
  return "cl_char3";
}
template <>
inline std::string GetTypeName<ucl::Char4>() {
  return "cl_char4";
}
template <>
inline std::string GetTypeName<ucl::Char8>() {
  return "cl_char8";
}
template <>
inline std::string GetTypeName<ucl::Char16>() {
  return "cl_char16";
}

template <>
inline std::string GetTypeName<ucl::UChar>() {
  return "cl_uchar";
}
template <>
inline std::string GetTypeName<ucl::UChar2>() {
  return "cl_uchar2";
}
template <>
inline std::string GetTypeName<ucl::UChar3>() {
  return "cl_uchar3";
}
template <>
inline std::string GetTypeName<ucl::UChar4>() {
  return "cl_uchar4";
}
template <>
inline std::string GetTypeName<ucl::UChar8>() {
  return "cl_uchar8";
}
template <>
inline std::string GetTypeName<ucl::UChar16>() {
  return "cl_uchar16";
}

template <>
inline std::string GetTypeName<ucl::Short>() {
  return "cl_short";
}
template <>
inline std::string GetTypeName<ucl::Short2>() {
  return "cl_short2";
}
template <>
inline std::string GetTypeName<ucl::Short3>() {
  return "cl_short3";
}
template <>
inline std::string GetTypeName<ucl::Short4>() {
  return "cl_short4";
}
template <>
inline std::string GetTypeName<ucl::Short8>() {
  return "cl_short8";
}
template <>
inline std::string GetTypeName<ucl::Short16>() {
  return "cl_short16";
}

template <>
inline std::string GetTypeName<ucl::UShort>() {
  return "cl_ushort";
}
template <>
inline std::string GetTypeName<ucl::UShort2>() {
  return "cl_ushort2";
}
template <>
inline std::string GetTypeName<ucl::UShort3>() {
  return "cl_ushort3";
}
template <>
inline std::string GetTypeName<ucl::UShort4>() {
  return "cl_ushort4";
}
template <>
inline std::string GetTypeName<ucl::UShort8>() {
  return "cl_ushort8";
}
template <>
inline std::string GetTypeName<ucl::UShort16>() {
  return "cl_ushort16";
}

template <>
inline std::string GetTypeName<ucl::Int>() {
  return "cl_int";
}
template <>
inline std::string GetTypeName<ucl::Int2>() {
  return "cl_int2";
}
template <>
inline std::string GetTypeName<ucl::Int3>() {
  return "cl_int3";
}
template <>
inline std::string GetTypeName<ucl::Int4>() {
  return "cl_int4";
}
template <>
inline std::string GetTypeName<ucl::Int8>() {
  return "cl_int8";
}
template <>
inline std::string GetTypeName<ucl::Int16>() {
  return "cl_int16";
}

template <>
inline std::string GetTypeName<ucl::UInt>() {
  return "cl_uint";
}
template <>
inline std::string GetTypeName<ucl::UInt2>() {
  return "cl_uint2";
}
template <>
inline std::string GetTypeName<ucl::UInt3>() {
  return "cl_uint3";
}
template <>
inline std::string GetTypeName<ucl::UInt4>() {
  return "cl_uint4";
}
template <>
inline std::string GetTypeName<ucl::UInt8>() {
  return "cl_uint8";
}
template <>
inline std::string GetTypeName<ucl::UInt16>() {
  return "cl_uint16";
}

template <>
inline std::string GetTypeName<ucl::Long>() {
  return "cl_long";
}
template <>
inline std::string GetTypeName<ucl::Long2>() {
  return "cl_long2";
}
template <>
inline std::string GetTypeName<ucl::Long3>() {
  return "cl_long3";
}
template <>
inline std::string GetTypeName<ucl::Long4>() {
  return "cl_long4";
}
template <>
inline std::string GetTypeName<ucl::Long8>() {
  return "cl_long8";
}
template <>
inline std::string GetTypeName<ucl::Long16>() {
  return "cl_long16";
}

template <>
inline std::string GetTypeName<ucl::ULong>() {
  return "cl_ulong";
}
template <>
inline std::string GetTypeName<ucl::ULong2>() {
  return "cl_ulong2";
}
template <>
inline std::string GetTypeName<ucl::ULong3>() {
  return "cl_ulong3";
}
template <>
inline std::string GetTypeName<ucl::ULong4>() {
  return "cl_ulong4";
}
template <>
inline std::string GetTypeName<ucl::ULong8>() {
  return "cl_ulong8";
}
template <>
inline std::string GetTypeName<ucl::ULong16>() {
  return "cl_ulong16";
}

template <>
inline std::string GetTypeName<ucl::Half>() {
  return "cl_half";
}
template <>
inline std::string GetTypeName<ucl::Half2>() {
  return "cl_half2";
}
template <>
inline std::string GetTypeName<ucl::Half3>() {
  return "cl_half3";
}
template <>
inline std::string GetTypeName<ucl::Half4>() {
  return "cl_half4";
}
template <>
inline std::string GetTypeName<ucl::Half8>() {
  return "cl_half8";
}
template <>
inline std::string GetTypeName<ucl::Half16>() {
  return "cl_half16";
}

template <>
inline std::string GetTypeName<ucl::Float>() {
  return "cl_float";
}
template <>
inline std::string GetTypeName<ucl::Float2>() {
  return "cl_float2";
}
template <>
inline std::string GetTypeName<ucl::Float3>() {
  return "cl_float3";
}
template <>
inline std::string GetTypeName<ucl::Float4>() {
  return "cl_float4";
}
template <>
inline std::string GetTypeName<ucl::Float8>() {
  return "cl_float8";
}
template <>
inline std::string GetTypeName<ucl::Float16>() {
  return "cl_float16";
}

template <>
inline std::string GetTypeName<ucl::Double>() {
  return "cl_double";
}
template <>
inline std::string GetTypeName<ucl::Double2>() {
  return "cl_double2";
}
template <>
inline std::string GetTypeName<ucl::Double3>() {
  return "cl_double3";
}
template <>
inline std::string GetTypeName<ucl::Double4>() {
  return "cl_double4";
}
template <>
inline std::string GetTypeName<ucl::Double8>() {
  return "cl_double8";
}
template <>
inline std::string GetTypeName<ucl::Double16>() {
  return "cl_double16";
}
}  // namespace internal
}  // namespace testing

#endif  // UNITCL_TYPES_H_INCLUDED
