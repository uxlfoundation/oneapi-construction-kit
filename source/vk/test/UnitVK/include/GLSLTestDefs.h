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

#ifndef GLSLTESTDEFS_H_INCLUDED
#define GLSLTESTDEFS_H_INCLUDED

#include <UnitVK.h>

#include <type_traits>

// The following could do with improved documentation: see JIRA: CA-399

// Namespace used to store types and helper functions
namespace glsl {

template <typename T, typename En = std::enable_if<std::is_arithmetic_v<T>>>
T abs(T x) {
  return (x >= 0) ? x : -x;
}

// All types in this namespace have an identical structure to the types running
// inside the shaders. This allows memory to be interpreted using
// reinterpret_cast<glsl::{type name}>.

typedef int32_t intTy;
typedef uint32_t uintTy;
typedef float floatTy;
typedef double doubleTy;

template <typename T, size_t N>
struct glsl_vec {
  // vec3 must be padded to vec4
  T data[N + (N % 2)];

  template <typename... Args>
  glsl_vec(Args... args) : data{args...} {}

  // Default constructor explicitly fills data with zeros.
  glsl_vec() { std::fill_n(data, N + (N % 2), 0); }

  glsl_vec(std::initializer_list<T> l) {
    assert(l.size() == N);
    std::copy(l.begin(), l.end(), data);
  }
  static_assert(N >= 2 && N <= 4, "vector can be only be of length 2, 3 or 4");

  bool operator==(const glsl_vec<T, N> &rhs) const {
    bool ret = true;
    for (size_t i = 0; i < N; i++) {
      ret &= (data[i] == rhs.data[i]);
    }
    return ret;
  }

  bool operator!=(const glsl_vec<T, N> &rhs) const { return !operator==(rhs); }
};

template <typename T>
using glsl_vec2 = glsl_vec<T, 2>;

template <typename T>
using glsl_vec3 = glsl_vec<T, 3>;

template <typename T>
using glsl_vec4 = glsl_vec<T, 4>;

enum class Order { RowMajor, ColumnMajor };

/// @brief Type for interacting with the std430 *mat* types in GLSL shaders
///
/// @tparam T Component type of the matrix. Valid types are float or double.
/// @tparam Columns Number of columns of the matrix
/// @tparam Rows Number of rows of the matrix
/// @tparam O Memory layout of the matrix
template <typename T, size_t Columns, size_t Rows, Order O = Order::ColumnMajor>
struct glsl_mat {
  static constexpr int vecSize = (O == Order::ColumnMajor) ? Rows : Columns;
  static constexpr int arrSize = (O == Order::ColumnMajor) ? Columns : Rows;
  static_assert(arrSize >= 2 && arrSize <= 4,
                "MatrixDimensions can only be 2, 3 or 4");
  glsl_vec<T, vecSize> data[arrSize];

  /// @brief Contructor
  ///
  /// The input are vectors that are copied to the memory of the array. Thus, if
  /// the memory is ColumnMajor, each argument must be a glsl_vec<T, Rows> and
  /// sizeof(args...) is Columns. For RowMajor the other way around.
  ///
  /// @param args Vectors that make up the matrix
  glsl_mat(std::initializer_list<glsl_vec<T, vecSize>> l) {
    assert(l.size() == arrSize);
    std::copy(l.begin(), l.end(), data);
  }

  /// @brief Compare matrix with rhs for equality.
  ///
  /// Uses the equality operator for each of its components, if the memory
  /// layouts match. Otherwise, walk the arrays and vectors and use the equality
  /// operator of T.
  ///
  /// @tparam ORHS Memory layout of rhs argument
  ///
  /// @param rhs glsl_mat matrix of the same dimensions and element type
  ///
  /// @return Whether each vector of the matrix is equal to the corresponding
  /// vector of rhs with regard to their equality operator
  /// @retval `true` The comparison of all vectors returned true.
  /// @retval `false` At least one of the comparisons of all vectors returned
  /// false.
  template <Order ORHS>
  bool operator==(const glsl_mat<T, Columns, Rows, ORHS> &rhs) {
    bool eq = true;
    int outer = (O == Order::ColumnMajor) ? Columns : Rows;
    int inner = (O == Order::ColumnMajor) ? Rows : Columns;
    for (int i = 0; i < outer; i++) {
      for (int j = 0; j < inner; j++) {
        eq &= (data[i].data[j] == rhs.data[j].data[i]);
      }
    }
    return eq;
  }

  bool operator==(const glsl_mat<T, Columns, Rows, O> &rhs) {
    int arrSize = (O == Order::ColumnMajor) ? Columns : Rows;
    bool eq = true;
    for (int i = 0; i < arrSize; i++) {
      eq &= (data[i] == rhs.data[i]);
    }
    return eq;
  }

  /// @brief Compare matrix with rhs for inequality.
  ///
  /// Uses the equality operator of glsl_mat to determine inequality.
  ///
  /// @param rhs glsl_mat matrix of the same dimensions and element type
  ///
  /// @return The negated result of applying the equality
  ///         operator to the matrix and rhs.
  /// @retval `true` The comparison of the matrices returned false.
  /// @retval `false` The comparison of the matrices returned true.
  template <Order ORHS>
  bool operator!=(const glsl_mat<T, Columns, Rows, ORHS> &rhs) const {
    return !operator==(rhs);
  }
};

template <typename T, Order O = Order::ColumnMajor>
using glsl_mat2 = glsl_mat<T, 2, 2, O>;

template <typename T, Order O = Order::ColumnMajor>
using glsl_mat3 = glsl_mat<T, 3, 3, O>;

template <typename T, Order O = Order::ColumnMajor>
using glsl_mat4 = glsl_mat<T, 4, 4, O>;

// Definition of operators for "nice" error messages:
template <typename T, size_t N>
std::ostream &operator<<(std::ostream &os, const glsl_vec<T, N> &bar) {
  os << "{ " << bar.data[0];
  for (size_t i = 1; i < N; i++) {
    os << ", " << bar.data[i];
  }
  os << "}";
  return os;
}

/// @brief Overload the << operator to enable nicer printing of glsl_mat
/// matrices.
///
/// Uses the << operator of the glsl_vec class for RowMajor matrices, for
/// ColumnMajor we walk through the array and vectors to print the expected
/// layout.
///
/// @tparam T Component type of the matrices. Valid types are float or double.
/// @tparam Columns Number of columns of the matrices
/// @tparam Rows Number of rows of the matrices
/// @tparam O Memory layout of the matrices
///
/// @param os ostream to print to
/// @param rhs matrix to print
///
/// @return os with the matrix being written to
template <typename T, size_t Columns, size_t Rows, Order O>
std::ostream &operator<<(std::ostream &os,
                         const glsl_mat<T, Columns, Rows, O> &bar);

template <typename T, size_t Columns, size_t Rows>
std::ostream &operator<<(
    std::ostream &os,
    const glsl_mat<T, Columns, Rows, Order::ColumnMajor> &bar) {
  os << "{";
  for (size_t i = 0; i < Rows; i++) {
    os << "{ " << bar.data[0].data[i];
    for (size_t j = 1; j < Columns; j++) {
      os << ", " << bar.data[j].data[i];
    }
    os << " }";
    if (i != Rows - 1) {
      os << "\n ";
    }
  }
  os << "}\n";
  return os;
}

template <typename T, size_t Columns, size_t Rows>
std::ostream &operator<<(
    std::ostream &os, const glsl_mat<T, Columns, Rows, Order::RowMajor> &bar) {
  os << "{";
  for (size_t i = 0; i < Rows; i++) {
    os << bar.data[i];
    if (i != Rows - 1) {
      os << "\n ";
    }
  }
  os << "}\n";
  return os;
}

// Fuzzy comparison functions
inline bool fuzzyEq(const float a, const float b,
                    const float max_error = 0.001f) {
  return abs(a - b) < max_error;
}

inline bool fuzzyEq(const double a, const double b,
                    const double max_error = 0.001) {
  return abs(a - b) < max_error;
}

template <typename T, size_t N>
inline bool fuzzyEq(const glsl_vec<T, N> &lhs, const glsl_vec<T, N> &rhs,
                    T max_error = static_cast<T>(0.001)) {
  bool eq = true;
  for (size_t i = 0; i < N; i++) {
    eq &= (abs(lhs.data[i] - rhs.data[i]) < max_error);
  }
  return eq;
}

/// @brief Compare two glsl_mat matrices for equality within a margin of
/// error.
///
/// Compare using the fuzzyEq function on each of the vectors if the matrices
/// have the same memory layout. Otherwise walk the arrays and vectors and
/// compare the elements with fuzzyEq. Note: if one of the arguments is
/// constructed in the arguments with an intializer list (i.e. no memory layout
/// is specified), the specialization assuming that the memory layouts are
/// identical is called.
///
/// @tparam T Component type of the matrices. Valid types are float or double.
/// @tparam Columns Number of columns of the matrices
/// @tparam Rows Number of rows of the matrices
/// @tparam OLHS Memory layout of the lhs matrix
/// @tparam ORHS Memory layout of the rhs matrix
///
/// @param lhs first matrix of the comparison
/// @param rhs second matrix of the comparison
/// @param max_error maximal error passed to the comparisons of the vectors.
///
/// @return Whether all comparisons of the vectors returned true.
/// @retval `true` The comparison of all vectors returned true.
/// @retval `false` At least one of the comparisons of all vectors returned
/// false.
template <typename T, size_t Columns, size_t Rows, Order OLHS, Order ORHS>
inline bool fuzzyEq(const glsl_mat<T, Columns, Rows, OLHS> &lhs,
                    const glsl_mat<T, Columns, Rows, ORHS> &rhs,
                    T max_error = static_cast<T>(0.001)) {
  bool eq = true;
  int outer = (OLHS == Order::ColumnMajor) ? Columns : Rows;
  int inner = (OLHS == Order::ColumnMajor) ? Rows : Columns;
  for (int i = 0; i < outer; i++) {
    for (int j = 0; j < inner; j++) {
      eq &= fuzzyEq(lhs.data[i].data[j], rhs.data[j].data[i], max_error);
    }
  }
  return eq;
}

template <typename T, size_t Columns, size_t Rows, Order O>
inline bool fuzzyEq(const glsl_mat<T, Columns, Rows, O> &lhs,
                    const glsl_mat<T, Columns, Rows, O> &rhs,
                    T max_error = static_cast<T>(0.001)) {
  const int arrSize = (O == Order::ColumnMajor) ? Columns : Rows;
  bool eq = true;
  for (int i = 0; i < arrSize; i++) {
    eq &= fuzzyEq(lhs.data[i], rhs.data[i], max_error);
  }
  return eq;
}

template <typename IntegralTy>
struct glsl_ModfStruct {
  IntegralTy fract;
  IntegralTy whole;
};

template <typename SigTy, typename ExpTy>
struct glsl_FrexpStruct {
  SigTy significand;
  ExpTy exponent;
};

typedef glsl_vec2<float> vec2Ty;
typedef glsl_vec3<float> vec3Ty;
typedef glsl_vec4<float> vec4Ty;

typedef glsl_vec2<double> dvec2Ty;
typedef glsl_vec3<double> dvec3Ty;
typedef glsl_vec4<double> dvec4Ty;

typedef glsl_vec2<intTy> ivec2Ty;
typedef glsl_vec3<intTy> ivec3Ty;
typedef glsl_vec4<intTy> ivec4Ty;

typedef glsl_vec2<uintTy> uvec2Ty;
typedef glsl_vec3<uintTy> uvec3Ty;
typedef glsl_vec4<uintTy> uvec4Ty;

typedef glsl_mat2<float> mat2Ty;
typedef glsl_mat3<float> mat3Ty;
typedef glsl_mat4<float> mat4Ty;

typedef glsl_mat2<double> dmat2Ty;
typedef glsl_mat3<double> dmat3Ty;
typedef glsl_mat4<double> dmat4Ty;

// Struct types used by *Struct functions
typedef glsl_FrexpStruct<floatTy, intTy> FrexpStructfloatTy;
typedef glsl_FrexpStruct<vec2Ty, ivec2Ty> FrexpStructvec2Ty;
typedef glsl_FrexpStruct<vec3Ty, ivec3Ty> FrexpStructvec3Ty;
typedef glsl_FrexpStruct<vec4Ty, ivec4Ty> FrexpStructvec4Ty;
typedef glsl_FrexpStruct<doubleTy, intTy> FrexpStructdoubleTy;
typedef glsl_FrexpStruct<dvec2Ty, ivec2Ty> FrexpStructdvec2Ty;
typedef glsl_FrexpStruct<dvec3Ty, ivec3Ty> FrexpStructdvec3Ty;
typedef glsl_FrexpStruct<dvec4Ty, ivec4Ty> FrexpStructdvec4Ty;
typedef glsl_ModfStruct<floatTy> ModfStructfloatTy;
typedef glsl_ModfStruct<vec2Ty> ModfStructvec2Ty;
typedef glsl_ModfStruct<vec3Ty> ModfStructvec3Ty;
typedef glsl_ModfStruct<vec4Ty> ModfStructvec4Ty;
typedef glsl_ModfStruct<doubleTy> ModfStructdoubleTy;
typedef glsl_ModfStruct<dvec2Ty> ModfStructdvec2Ty;
typedef glsl_ModfStruct<dvec3Ty> ModfStructdvec3Ty;
typedef glsl_ModfStruct<dvec4Ty> ModfStructdvec4Ty;

// templates to check for doubles
template <class T>
struct is_double_vec : std::false_type {};

template <std::size_t N>
struct is_double_vec<glsl_vec<double, N>> : std::true_type {};

template <class Struct>
struct is_double_struct : std::false_type {};

template <>
struct is_double_struct<glsl_FrexpStruct<doubleTy, intTy>> : std::true_type {};

template <std::size_t N, class Ivec>
struct is_double_struct<glsl_FrexpStruct<glsl_vec<doubleTy, N>, Ivec>>
    : std::true_type {};

template <>
struct is_double_struct<glsl_ModfStruct<doubleTy>> : std::true_type {};

template <std::size_t N>
struct is_double_struct<glsl_ModfStruct<glsl_vec<doubleTy, N>>>
    : std::true_type {};

template <class T>
struct is_double_type
    : std::conditional_t<std::is_same_v<T, double> || is_double_vec<T>::value ||
                             is_double_struct<T>::value,
                         std::true_type, std::false_type> {};

template <class... Ts>
struct has_double_type : std::false_type {};

template <class T, class... Ts>
struct has_double_type<T, Ts...>
    : std::conditional_t<is_double_type<T>::value, std::true_type,
                         has_double_type<Ts...>> {};
}  // namespace glsl

/// @brief Generic class used as base test fixture for all GLSL builtins
///
/// @tparam RetType The return type of the extended instruction
/// @tparam Args The types of the arguments passed to the extended instruction
template <typename RetType, typename... Args>
class GlslBuiltinTest : public uvk::SimpleKernelTest {
 public:
  /// @brief Constructor for GlslBuiltinTest
  ///
  /// @param shader The uvk::Shader ID of the shader to be tested
  GlslBuiltinTest(uvk::Shader shader)
      : SimpleKernelTest(isDoubleTest, shader, 128) {}

  /// @brief Executes the shader with the given arguments
  ///
  /// Every shader should read arguments from the input buffer (set = 0,
  /// binding = 0), execute the extended instruction with these arguments and
  /// then write the results to the output buffer (set = 0, binding = 1).
  /// It is assumed that there is no packing in either of these buffers (i.e.
  /// all members are aligned).
  ///
  /// @param args The arguments which are passed to the extended instruction
  /// via the input buffer.
  ///
  /// @return The result, stored in the output buffer, after executing the
  /// shader
  RetType RunWithArgs(Args... args) {
    InternalSetArg(0, args...);
    FlushToDevice();
    ExecuteAndWait();
    FlushFromDevice();
    return RefToMappedData<RetType>(OUTPUT_BUFFER, 0);
    // Note: this function assumes that the same command buffer can be
    // resubmitted multiple times, i.e. VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT
    // _BIT is not set.
  }

 private:
  /// @brief Terminator for InternalSetArg()
  void InternalSetArg(size_t) {}

  /// @brief Internal function for setting arguments
  ///
  /// @param offset The offset where first should be stored inside the buffer
  /// (ignoring alignment)
  /// @param first The next argument to be stored
  /// @param tail The remaining arguments
  template <typename First, typename... Tail>
  void InternalSetArg(size_t offset, First first, Tail... tail) {
    RefToMappedData<First>(INPUT_BUFFER, offset) = first;
    // TODO: This code does not ensure that arguments are aligned:
    // i.e:  If offset is not a multiple of sizeof(first), first will be stored
    // at a non-aligned offset. See CA-1020
    offset += sizeof(first);
    InternalSetArg(offset, tail...);
  }

  /// @brief Bool for skipping the test if the hardware doesn't support doubles
  static constexpr bool isDoubleTest = glsl::is_double_type<RetType>::value ||
                                       glsl::has_double_type<Args...>::value;
};

#endif
