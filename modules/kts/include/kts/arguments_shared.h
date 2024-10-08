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

#ifndef KTS_ARGUMENTS_SHARED_H_INCLUDED
#define KTS_ARGUMENTS_SHARED_H_INCLUDED

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace kts {

// Default global size for 1D kernels.
extern const size_t N;

// Default local size for 1D kernels that make use of work-groups.
extern size_t localN;

/// @brief Reference function in the form of a function pointer
///
/// This is a simple way to pass around a reference function of the form T(int).
/// It only works for normal functions and it will be converted into a
/// Reference1D internally.
///
/// @tparam T The return type of the function (i.e. the buffer type)
template <typename T>
using Reference1DPtr = T (*)(size_t);

/// @brief Class that works similar to std::function but with multiple types.
///
/// @tparam T The type that this reference function will return.
///
/// This class supports two different types of reference functions, one that
/// takes the index and returns the value for that index, and one that takes the
/// index and the value returned by the test and returns true if that value is
/// correct. Internally, it uses SFINAE to determine which of the two reference
/// functions has been passed to it. Internally, type erasure through
/// inheritance is used to store the appropriate std::function object.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4521)  // multiple copy constructors specified
#endif
template <typename T>
class Reference1D {
  /// @brief Helper class to determine if the reference is of type T(size_t)
  /// @tparam TT The type of the reference function
  template <typename TT>
  class HasOperatorSizeT {
    /// @brief This function will only be valid if it is valid to call U(0)
    /// @tparam U The type to check for operator(size_t)
    /// If it is is not possible, SFINAE will remove it. `std::true_type` has a
    /// static member `value` set to `true`.
    template <typename U, typename = decltype(std::declval<U>()(0))>
    static std::true_type test(U &&);
    /// @brief This function is always valid
    /// However, it is less specific than the one above it, so if the one above
    /// it is available it will be preferred. `std::false_type` has static
    /// member `value` set to `false`
    static std::false_type test(...);

   public:
    /// @brief This will be set to `true` if it is possible to call TT(size_t)
    enum { value = decltype(test(std::declval<TT>()))::value };
  };

  /// @brief Helper class to determine if the reference has type bool(size_t, T)
  /// @tparam TT The type of the reference function
  template <typename TT>
  class HasOperatorSizeTT {
    /// @brief This function will only be valid if it is valid to call U(0, T)
    /// @tparam U The type to check for operator(size_t)
    /// If it is is not possible, SFINAE will remove it. `std::true_type` has a
    /// static member `value` set to `true`.
    template <typename U,
              typename = decltype(std::declval<U>()(0, std::declval<T>()))>
    static std::true_type test(U &&);
    /// @brief This function is always valid
    /// However, it is less specific than the one above it, so if the one above
    /// it is available it will be preferred. `std::false_type` has static
    /// member `value` set to `false`
    static std::false_type test(...);

   public:
    /// @brief This will be set to `true` if we can call TT(size_t, T)
    enum { value = decltype(test(std::declval<TT>()))::value };
  };

 public:
  /// @brief What kind of reference function we are current holding
  enum RefType {
    /// @brief No function set
    Empty,
    /// @brief Ref is in the form of T(size_t) has been set (returns a value)
    Value,
    /// @brief Ref is in the form of bool(size_t, T) has been set (returns a
    /// bool)
    Boolean,
  };

  /// @brief Default (empty) constructor
  Reference1D() : Ref(nullptr), Type(Empty) {}
  /// @brief Copy constructor
  /// @param Other The reference to copy
  Reference1D(const Reference1D &Other) : Ref(Other.Ref), Type(Other.Type) {}
  /// @brief Copy constructor
  /// @param Other The reference to copy
  Reference1D(Reference1D &Other) : Ref(Other.Ref), Type(Other.Type) {}
  /// @brief Move constructor
  /// @param Other The reference to move
  Reference1D(Reference1D &&Other)
      : Ref(std::move(Other.Ref)), Type(Other.Type) {}
  /// @brief Constructor for function pointers T(size_t)
  /// @param RefPtr The function pointer
  Reference1D(Reference1DPtr<T> RefPtr)
      : Ref(std::make_shared<ReferenceFun<T(size_t)>>(RefPtr)), Type(Value) {}
  /// @brief Copy assignment
  /// @param Other The reference to copy
  /// @return `*this`
  Reference1D &operator=(const Reference1D &Other) {
    if (this != &Other) {
      Ref = Other.Ref;
      Type = Other.Type;
    }
    return *this;
  }
  /// @brief Move assignment
  /// @param Other The reference to copy
  /// @return `*this`
  Reference1D &operator=(Reference1D &&Other) {
    Ref = std::move(Other.Ref);
    Type = Other.Type;
    return *this;
  }

  // The following constructor are the ones that are used when passing in a
  // function or functor.

  /// @brief Generic constructor for anything that can be called as F(size_t)
  /// @tparam F The type of the function passed as the reference function
  /// @param f The function itself
  ///
  /// This constructor will accept any type that can be used as F(size_t) and it
  /// will construct a reference from it. The second argument is used purely to
  /// trigger SFINAE.
  template <typename F>
  Reference1D(F &&f, std::enable_if_t<HasOperatorSizeT<F>::value,
                                      std::remove_reference_t<F>> * = 0) {
    Ref = std::make_shared<ReferenceFun<T(size_t)>>(f);
    Type = Value;
  }

  /// @brief Generic constructor for anything that can be called as F(size_t, T)
  /// @tparam F The type of the function passed as the reference function
  /// @param f The function itself
  ///
  /// This constructor will accept any type that can be used as F(size_t) and it
  /// will construct a reference from it. The second argument is used purely to
  /// trigger SFINAE.
  template <typename F>
  Reference1D(F &&f, std::enable_if_t<HasOperatorSizeTT<F>::value,
                                      std::remove_reference_t<F>> * = 0) {
    Ref = std::make_shared<ReferenceFun<bool(size_t, T &)>>(f);
    Type = Boolean;
  }

  /// @brief Call the reference function with one argument, the index
  /// @param x The index/global ID
  /// @return The expected value at the given index
  T operator()(size_t x) const {
    // We want to avoid using RTTI
    assert(Type == Value);
    return static_cast<ReferenceFun<T(size_t)> *>(Ref.get())->Fun(x);
  }
  /// @brief Call the reference function with one argument, the index
  /// @param x The index/global ID
  /// @return The expected value at the given index
  T operator()(size_t x) {
    // We want to avoid using RTTI
    assert(Type == Value);
    return static_cast<ReferenceFun<T(size_t)> *>(Ref.get())->Fun(x);
  }

  /// @brief Call the reference function with two arguments (index, value)
  /// @param x The index/global ID
  /// @param v The value read from the buffer
  /// @return reference_function(x, v) (boolean);
  bool operator()(size_t x, T &v) const {
    // We want to avoid using RTTI
    assert(Type == Boolean);
    return static_cast<ReferenceFun<bool(size_t, T &)> *>(Ref.get())->Fun(x, v);
  }
  /// @brief Call the reference function with two arguments (index, value)
  /// @param x The index/global ID
  /// @param v The value read from the buffer
  /// @return reference_function(x, v) (boolean);
  bool operator()(size_t x, T &v) {
    // We want to avoid using RTTI
    assert(Type == Boolean);
    return static_cast<ReferenceFun<bool(size_t, T &)> *>(Ref.get())->Fun(x, v);
  }

  /// @brief Get the type of the reference function
  /// @return The type of the reference function
  RefType getType() const { return Type; };

  bool isValueType() const { return getType() == Value; };
  bool isBooleanType() const { return getType() == Boolean; };

 private:
  /// @brief Helper class to type erase the helper function using inheritance
  struct ReferenceFunBase {};
  /// @brief Helper class to type erase the helper function using inheritance
  template <typename F>
  struct ReferenceFun : public ReferenceFunBase {
    template <typename FF>
    ReferenceFun(FF &&f) : Fun(std::forward<FF>(f)) {}
    std::function<F> Fun;
  };

  /// @brief The reference function, type erased
  std::shared_ptr<ReferenceFunBase> Ref;
  /// @brief The type of the reference function
  RefType Type;
};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

// Builds a vector reference function from a scalar one.
template <typename VT, typename T>
Reference1D<VT> BuildVec2Reference1D(Reference1DPtr<T> ref) {
  return [ref](size_t x) {
    T s0 = ref((x * 2) + 0);
    T s1 = ref((x * 2) + 1);
    return VT{{s0, s1}};
  };
}

// Builds a vector reference function from a scalar one.
template <typename VT, typename T>
Reference1D<VT> BuildVec3Reference1D(Reference1DPtr<T> ref) {
  return [ref](size_t x) {
    T s0 = ref((x * 3) + 0);
    T s1 = ref((x * 3) + 1);
    T s2 = ref((x * 3) + 2);
    return VT{{s0, s1, s2}};
  };
}

// Builds a vector reference function from a scalar one.
template <typename VT, typename T>
Reference1D<VT> BuildVec4Reference1D(Reference1DPtr<T> ref) {
  return [ref](size_t x) {
    T s0 = ref((x * 4) + 0);
    T s1 = ref((x * 4) + 1);
    T s2 = ref((x * 4) + 2);
    T s3 = ref((x * 4) + 3);
    return VT{{s0, s1, s2, s3}};
  };
}

template <typename T>
struct Validator {
  bool validate(T &expected, T &actual) { return expected == actual; }
  void print(std::stringstream &s, const T &value) { s << value; }
};

template <>
inline void Validator<char>::print(std::stringstream &s, const char &value) {
  s << static_cast<int>(value);
}

template <>
inline void Validator<signed char>::print(std::stringstream &s,
                                          const signed char &value) {
  s << static_cast<int>(value);
}

template <>
inline void Validator<unsigned char>::print(std::stringstream &s,
                                            const unsigned char &value) {
  s << static_cast<unsigned int>(value);
}

template <>
struct Validator<double> {
  bool validate(double &expected, double &actual) {
    const testing::internal::FloatingPoint<double> lhs(expected), rhs(actual);

    // This compares the doubles within 4 ULPs
    return lhs.AlmostEquals(rhs) ||
           (std::isnan(expected) && std::isnan(actual));
  }

  void print(std::stringstream &s, double value) {
#if !defined(__GNUC__) || ((__GNUC__ == 5) && (__GNUC_MINOR__ >= 1)) || \
    (__GNUC__ > 5)
    s << std::hexfloat << value;
#else
    auto prev_precision = s.precision();
    s << std::setprecision(std::numeric_limits<double>::digits10 + 2) << value;
    s.precision(prev_precision);
#endif
  }
};

template <>
struct Validator<float> {
  bool validate(float &expected, float &actual) {
    const testing::internal::FloatingPoint<float> lhs(expected), rhs(actual);

    // This compares the doubles within 4 ULPs
    return lhs.AlmostEquals(rhs) ||
           (std::isnan(expected) && std::isnan(actual));
  }

  void print(std::stringstream &s, const float &value) {
#if !defined(__GNUC__) || ((__GNUC__ == 5) && (__GNUC_MINOR__ >= 1)) || \
    (__GNUC__ > 5)
    s << std::hexfloat << value;
#else
    auto prev_precision = s.precision();
    s << std::setprecision(std::numeric_limits<float>::digits10 + 2) << value;
    s.precision(prev_precision);
#endif
  }
};

class BufferStreamer;

template <typename T, typename V = Validator<T>, typename R = T>
class GenericStreamer;

// Describes how to create and validate an argument buffer.
struct BufferDesc {
  // Create an invalid buffer descriptor.
  BufferDesc();

  BufferDesc(size_t size, std::shared_ptr<BufferStreamer> streamer);

  BufferDesc(size_t size, std::shared_ptr<BufferStreamer> streamer,
             std::shared_ptr<BufferStreamer> streamer2);

  template <typename T, typename V = Validator<T>>
  BufferDesc(size_t size, Reference1D<T> ref, V validator = {})
      : size_(size),
        streamer_(new GenericStreamer<T, V>(ref, validator)),
        streamer2_(nullptr) {}

  template <typename T, typename V = Validator<T>>
  BufferDesc(size_t size, Reference1DPtr<T> ref, V validator = {})
      : size_(size),
        streamer_(new GenericStreamer<T, V>(Reference1D<T>(ref), validator)),
        streamer2_(nullptr) {}

  template <typename T, typename V = Validator<T>>
  BufferDesc(size_t size, Reference1D<T> ref, Reference1D<T> ref2,
             V validator = {})
      : size_(size),
        streamer_(new GenericStreamer<T, V>(ref, validator)),
        streamer2_(new GenericStreamer<T, V>(Reference1D<T>(ref2), validator)) {
  }

  template <typename T, typename V = Validator<T>>
  BufferDesc(size_t size, Reference1DPtr<T> ref, Reference1DPtr<T> ref2,
             V validator = {})
      : size_(size),
        streamer_(new GenericStreamer<T, V>(Reference1D<T>(ref), validator)),
        streamer2_(new GenericStreamer<T, V>(Reference1D<T>(ref2), validator)) {
  }

  template <typename T, typename V = Validator<T>>
  BufferDesc(size_t size, Reference1D<T> ref, Reference1DPtr<T> ref2,
             V validator = {})
      : size_(size),
        streamer_(new GenericStreamer<T, V>(ref, validator)),
        streamer2_(new GenericStreamer<T, V>(Reference1D<T>(ref2), validator)) {
  }

  template <typename T, typename V = Validator<T>>
  BufferDesc(size_t size, Reference1DPtr<T> ref, Reference1D<T> ref2,
             V validator = {})
      : size_(size),
        streamer_(new GenericStreamer<T, V>(Reference1D<T>(ref), validator)),
        streamer2_(new GenericStreamer<T, V>(ref2), validator) {}

  // Size of the buffer.
  size_t size_;
  // Streamer used to create or validate the buffer.
  std::shared_ptr<BufferStreamer> streamer_;
  // Streamer used to validate the out part of an in/out buffer.
  std::shared_ptr<BufferStreamer> streamer2_;
};

// Possible kinds of kernel arguments.
enum ArgKind {
  eInvalidArg = 0,
  eInputBuffer = 1,
  eOutputBuffer = 2,
  eInOutBuffer = 3,
  ePrimitive = 4,
  eSampler = 5,
  eInputImage = 6,
  eSampledImage = 7
};

class ArgumentBase {
 public:
  ArgumentBase(ArgKind kind, size_t index) : kind_(kind), index_(index) {}

  virtual ~ArgumentBase() = default;

  ArgKind GetKind() const { return kind_; }
  size_t GetIndex() const { return index_; }
  virtual uint8_t *GetBufferStoragePtr() = 0;
  virtual size_t GetBufferStorageSize() = 0;
  virtual void SetBufferStorageSize(size_t size) = 0;

 private:
  // Argument kind.
  ArgKind kind_;
  // Argument index.
  size_t index_;
};

// Populates input buffers with data and validate output buffers' data.
class BufferStreamer {
 public:
  virtual ~BufferStreamer() {}

  virtual void PopulateBuffer(ArgumentBase &arg, const BufferDesc &desc) = 0;
  virtual bool ValidateBuffer(ArgumentBase &arg, const BufferDesc &desc,
                              std::vector<std::string> *errors = nullptr) = 0;
  virtual size_t GetElementSize() = 0;
};

template <typename T>
struct MemoryAccessor {
  T LoadFromBuffer(void *Ptr, size_t Offset) {
    void *const PtrPlusOffset =
        static_cast<uint8_t *>(Ptr) + (Offset * sizeof(T));
    T Val;
    memcpy(&Val, PtrPlusOffset, sizeof(T));
    return Val;
  }

  void StoreToBuffer(const T &Val, void *Ptr, size_t Offset) {
    void *const PtrPlusOffset =
        static_cast<uint8_t *>(Ptr) + (Offset * sizeof(T));
    memcpy(PtrPlusOffset, &Val, sizeof(T));
  }
};

/// @brief Describes how to create and validate buffers
///
/// @tparam T Buffer element type
/// @tparam V Implementation of Validator to verify results with
/// @tparam R Reference element type to verify against, defaults to T but is
///           useful for verifying floating points against a more precision
///           representation.
template <typename T, typename V, typename R>
class GenericStreamer : public BufferStreamer {
 public:
  GenericStreamer(Reference1D<R> ref, V validator = {})
      : ref_(ref), validator(validator) {}
  GenericStreamer(Reference1D<R> ref, const std::vector<Reference1D<R>> &&f,
                  V validator = {})
      : ref_(ref), validator(validator), fallbacks_(f) {}

  virtual void PopulateBuffer(ArgumentBase &arg, const BufferDesc &desc) {
    arg.SetBufferStorageSize(desc.size_ * sizeof(T));

    MemoryAccessor<T> accessor;
    if ((arg.GetKind() == eInputBuffer) || (arg.GetKind() == eInOutBuffer) ||
        (arg.GetKind() == eInputImage) || (arg.GetKind() == eSampledImage)) {
      for (size_t j = 0; j < desc.size_; j++) {
        accessor.StoreToBuffer(ref_(j), arg.GetBufferStoragePtr(), j);
      }
    } else {
      for (size_t j = 0; j < desc.size_; j++) {
        accessor.StoreToBuffer(T(), arg.GetBufferStoragePtr(), j);
      }
    }
  }

  virtual bool ValidateBuffer(ArgumentBase &arg, const BufferDesc &desc,
                              std::vector<std::string> *errors) {
    if ((arg.GetKind() != eOutputBuffer) && (arg.GetKind() != eInOutBuffer)) {
      return true;
    }

    MemoryAccessor<T> accessor;
    for (size_t j = 0; j < desc.size_; j++) {
      T actual = accessor.LoadFromBuffer(arg.GetBufferStoragePtr(), j);
      if (ref_.isValueType()) {
        R expected = ref_(j);
        if (!validator.validate(expected, actual)) {
          // Try verifying against fallback references
          if (std::any_of(fallbacks_.begin(), fallbacks_.end(),
                          [this, &actual, j](const Reference1D<R> &r) {
                            R fallback = r(j);
                            return validator.validate(fallback, actual);
                          })) {
            continue;
          }

          if (CheckIfUndef(j)) {
            // Result is undefined at this index, skip
            continue;
          }

          if (errors) {
            std::stringstream ss;
            ss << "Result mismatch at index " << j;
            ss << " (";
            if (input_formatter_) {
              ss << "input: ";
              input_formatter_(ss, j);
              ss << ", ";
            }
            ss << "expected: ";
            validator.print(ss, expected);
            ss << ", actual: ";
            validator.print(ss, actual);
            ss << ")";
            errors->push_back(ss.str());
          }
          return false;
        }
      } else if (ref_.isBooleanType()) {
        R casted = static_cast<R>(actual);
        if (!ref_(j, casted)) {
          if (errors) {
            std::stringstream ss;
            ss << "Verification failed at index " << j
               << " (Reference function returned \"false\" for the value ";
            validator.print(ss, actual);
            ss << ")";
            errors->push_back(ss.str());
          }
          return false;
        }
      }
    }
    return true;
  }

  virtual size_t GetElementSize() { return sizeof(T); }

  bool CheckIfUndef(size_t index) const {
    if (undef_callback_) {
      return undef_callback_(index);
    }
    return false;
  }

  void SetUndefCallback(std::function<bool(size_t)> &&f) {
    undef_callback_ = std::move(f);
  }

  void SetInputFormatter(std::function<void(std::stringstream &, size_t)> &&f) {
    input_formatter_ = std::move(f);
  }

  /// @brief Expected value for each data point to verify against
  Reference1D<R> ref_;
  /// @brief Instance of the validator to verify results against.
  V validator;
  /// @brief References to try if the expected value fails, this is useful for
  ///        testing FTZ behaviour in floating point operations.
  const std::vector<Reference1D<R>> fallbacks_;
  /// @brief Callback to check if output result is undefined at parametrized
  ///        index
  std::function<bool(size_t)> undef_callback_;
  /// @brief Callback to format the input given a particular index.
  std::function<void(std::stringstream &, size_t)> input_formatter_;
};

struct Primitive {
  virtual ~Primitive() {}

  virtual void *GetAddress() = 0;
  virtual size_t GetSize() = 0;
};

template <typename T>
struct BoxedPrimitive : public Primitive {
  BoxedPrimitive(T value) : value_(value) {}

  virtual void *GetAddress() { return &value_; }
  virtual size_t GetSize() { return sizeof(T); }

  T value_;
};
}  // namespace kts

#endif  // KTS_ARGUMENTS_SHARED_H_INCLUDED
