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

#ifndef OCL_CXXBUILTINS_H_INCLUDED
#define OCL_CXXBUILTINS_H_INCLUDED

namespace ocl {
/// @addtogroup builtins
/// @{

namespace Rounding {
enum Type { undefined, rte, rtz, rtp, rtn };
}

typedef char char32 __attribute__((ext_vector_type(32)));
typedef uchar uchar32 __attribute__((ext_vector_type(32)));
typedef short short32 __attribute__((ext_vector_type(32)));
typedef ushort ushort32 __attribute__((ext_vector_type(32)));
typedef int int32 __attribute__((ext_vector_type(32)));
typedef uint uint32 __attribute__((ext_vector_type(32)));
typedef long long32 __attribute__((ext_vector_type(32)));
typedef ulong ulong32 __attribute__((ext_vector_type(32)));
typedef float float32 __attribute__((ext_vector_type(32)));
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
typedef double double32 __attribute__((ext_vector_type(32)));
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
struct TypeTraits;

template <>
struct TypeTraits<uchar> {
  typedef uchar ElementType;
  typedef ushort LargerType;
  typedef char SignedType;
  typedef uchar UnsignedType;
};

template <>
struct TypeTraits<uchar2> {
  typedef uchar ElementType;
  typedef ushort2 LargerType;
  typedef char2 SignedType;
  typedef uchar2 UnsignedType;
};

template <>
struct TypeTraits<uchar3> {
  typedef uchar ElementType;
  typedef ushort3 LargerType;
  typedef char3 SignedType;
  typedef uchar3 UnsignedType;
};

template <>
struct TypeTraits<uchar4> {
  typedef uchar ElementType;
  typedef ushort4 LargerType;
  typedef char4 SignedType;
  typedef uchar4 UnsignedType;
};

template <>
struct TypeTraits<uchar8> {
  typedef uchar ElementType;
  typedef ushort8 LargerType;
  typedef char8 SignedType;
  typedef uchar8 UnsignedType;
};

template <>
struct TypeTraits<uchar16> {
  typedef uchar ElementType;
  typedef ushort16 LargerType;
  typedef char16 SignedType;
  typedef uchar16 UnsignedType;
};

template <>
struct TypeTraits<uchar32> {
  typedef uchar ElementType;
  // no larger type allowed for our 32 element type!
};

template <>
struct TypeTraits<ushort> {
  typedef ushort ElementType;
  typedef uint LargerType;
  typedef short SignedType;
  typedef ushort UnsignedType;
};

template <>
struct TypeTraits<ushort2> {
  typedef ushort ElementType;
  typedef uint2 LargerType;
  typedef short2 SignedType;
  typedef ushort2 UnsignedType;
};

template <>
struct TypeTraits<ushort3> {
  typedef ushort ElementType;
  typedef uint3 LargerType;
  typedef short3 SignedType;
  typedef ushort3 UnsignedType;
};

template <>
struct TypeTraits<ushort4> {
  typedef ushort ElementType;
  typedef uint4 LargerType;
  typedef short4 SignedType;
  typedef ushort4 UnsignedType;
};

template <>
struct TypeTraits<ushort8> {
  typedef ushort ElementType;
  typedef uint8 LargerType;
  typedef short8 SignedType;
  typedef ushort8 UnsignedType;
};

template <>
struct TypeTraits<ushort16> {
  typedef ushort ElementType;
  typedef uint16 LargerType;
  typedef short16 SignedType;
  typedef ushort16 UnsignedType;
};

template <>
struct TypeTraits<ushort32> {
  typedef ushort ElementType;
  // no larger type allowed for our 32 element type!
};

template <>
struct TypeTraits<uint> {
  typedef uint ElementType;
  typedef ulong LargerType;
  typedef int SignedType;
  typedef uint UnsignedType;
};

template <>
struct TypeTraits<uint2> {
  typedef uint ElementType;
  typedef ulong2 LargerType;
  typedef int2 SignedType;
  typedef uint2 UnsignedType;
};

template <>
struct TypeTraits<uint3> {
  typedef uint ElementType;
  typedef ulong3 LargerType;
  typedef int3 SignedType;
  typedef uint3 UnsignedType;
};

template <>
struct TypeTraits<uint4> {
  typedef uint ElementType;
  typedef ulong4 LargerType;
  typedef int4 SignedType;
  typedef uint4 UnsignedType;
};

template <>
struct TypeTraits<uint8> {
  typedef uint ElementType;
  typedef ulong8 LargerType;
  typedef int8 SignedType;
  typedef uint8 UnsignedType;
};

template <>
struct TypeTraits<uint16> {
  typedef uint ElementType;
  typedef ulong16 LargerType;
  typedef int16 SignedType;
  typedef uint16 UnsignedType;
};

template <>
struct TypeTraits<uint32> {
  typedef uint ElementType;
  // no larger type allowed for our 32 element type!
};

template <>
struct TypeTraits<ulong> {
  typedef ulong ElementType;
  // no larger type than 64 bit!
  typedef long SignedType;
  typedef ulong UnsignedType;
};

template <>
struct TypeTraits<ulong2> {
  typedef ulong ElementType;
  // no larger type than 64 bit!
  typedef long2 SignedType;
  typedef ulong2 UnsignedType;
};

template <>
struct TypeTraits<ulong3> {
  typedef ulong ElementType;
  // no larger type than 64 bit!
  typedef long3 SignedType;
  typedef ulong3 UnsignedType;
};

template <>
struct TypeTraits<ulong4> {
  typedef ulong ElementType;
  // no larger type than 64 bit!
  typedef long4 SignedType;
  typedef ulong4 UnsignedType;
};

template <>
struct TypeTraits<ulong8> {
  typedef ulong ElementType;
  // no larger type than 64 bit!
  typedef long8 SignedType;
  typedef ulong8 UnsignedType;
};

template <>
struct TypeTraits<ulong16> {
  typedef ulong ElementType;
  // no larger type than 64 bit!
  typedef long16 SignedType;
  typedef ulong16 UnsignedType;
};

template <>
struct TypeTraits<ulong32> {
  typedef ulong ElementType;
  // no larger type allowed for our 32 element type!
};

template <>
struct TypeTraits<char> {
  typedef char ElementType;
  typedef short LargerType;
  typedef char SignedType;
  typedef uchar UnsignedType;
};

template <>
struct TypeTraits<char2> {
  typedef char ElementType;
  typedef short2 LargerType;
  typedef char2 SignedType;
  typedef uchar2 UnsignedType;
};

template <>
struct TypeTraits<char3> {
  typedef char ElementType;
  typedef short3 LargerType;
  typedef char3 SignedType;
  typedef uchar3 UnsignedType;
};

template <>
struct TypeTraits<char4> {
  typedef char ElementType;
  typedef short4 LargerType;
  typedef char4 SignedType;
  typedef uchar4 UnsignedType;
};

template <>
struct TypeTraits<char8> {
  typedef char ElementType;
  typedef short8 LargerType;
  typedef char8 SignedType;
  typedef uchar8 UnsignedType;
};

template <>
struct TypeTraits<char16> {
  typedef char ElementType;
  typedef short16 LargerType;
  typedef char16 SignedType;
  typedef uchar16 UnsignedType;
};

template <>
struct TypeTraits<char32> {
  typedef char ElementType;
  // no larger type allowed for our 32 element type!
};

template <>
struct TypeTraits<short> {
  typedef short ElementType;
  typedef int LargerType;
  typedef short SignedType;
  typedef ushort UnsignedType;
};

template <>
struct TypeTraits<short2> {
  typedef short ElementType;
  typedef int2 LargerType;
  typedef short2 SignedType;
  typedef ushort2 UnsignedType;
};

template <>
struct TypeTraits<short3> {
  typedef short ElementType;
  typedef int3 LargerType;
  typedef short3 SignedType;
  typedef ushort3 UnsignedType;
};

template <>
struct TypeTraits<short4> {
  typedef short ElementType;
  typedef int4 LargerType;
  typedef short4 SignedType;
  typedef ushort4 UnsignedType;
};

template <>
struct TypeTraits<short8> {
  typedef short ElementType;
  typedef int8 LargerType;
  typedef short8 SignedType;
  typedef ushort8 UnsignedType;
};

template <>
struct TypeTraits<short16> {
  typedef short ElementType;
  typedef int16 LargerType;
  typedef short16 SignedType;
  typedef ushort16 UnsignedType;
};

template <>
struct TypeTraits<short32> {
  typedef short ElementType;
  // no larger type allowed for our 32 element type!
};

template <>
struct TypeTraits<int> {
  typedef int ElementType;
  typedef long LargerType;
  typedef int SignedType;
  typedef uint UnsignedType;
};

template <>
struct TypeTraits<int2> {
  typedef int ElementType;
  typedef long2 LargerType;
  typedef int2 SignedType;
  typedef uint2 UnsignedType;
};

template <>
struct TypeTraits<int3> {
  typedef int ElementType;
  typedef long3 LargerType;
  typedef int3 SignedType;
  typedef uint3 UnsignedType;
};

template <>
struct TypeTraits<int4> {
  typedef int ElementType;
  typedef long4 LargerType;
  typedef int4 SignedType;
  typedef uint4 UnsignedType;
};

template <>
struct TypeTraits<int8> {
  typedef int ElementType;
  typedef long8 LargerType;
  typedef int8 SignedType;
  typedef uint8 UnsignedType;
};

template <>
struct TypeTraits<int16> {
  typedef int ElementType;
  typedef long16 LargerType;
  typedef int16 SignedType;
  typedef uint16 UnsignedType;
};

template <>
struct TypeTraits<int32> {
  typedef int ElementType;
  // no larger type allowed for our 32 element type!
};

template <>
struct TypeTraits<long> {
  typedef long ElementType;
  // no larger type than 64 bit!
  typedef long SignedType;
  typedef ulong UnsignedType;
};

template <>
struct TypeTraits<long2> {
  typedef long ElementType;
  // no larger type than 64 bit!
  typedef long2 SignedType;
  typedef ulong2 UnsignedType;
};

template <>
struct TypeTraits<long3> {
  typedef long ElementType;
  // no larger type than 64 bit!
  typedef long3 SignedType;
  typedef ulong3 UnsignedType;
};

template <>
struct TypeTraits<long4> {
  typedef long ElementType;
  // no larger type than 64 bit!
  typedef long4 SignedType;
  typedef ulong4 UnsignedType;
};

template <>
struct TypeTraits<long8> {
  typedef long ElementType;
  // no larger type than 64 bit!
  typedef long8 SignedType;
  typedef ulong8 UnsignedType;
};

template <>
struct TypeTraits<long16> {
  typedef long ElementType;
  // no larger type than 64 bit!
  typedef long16 SignedType;
  typedef ulong16 UnsignedType;
};

template <>
struct TypeTraits<long32> {
  typedef long ElementType;
  // no larger type allowed for our 32 element type!
};

template <>
struct TypeTraits<float> {
  typedef float ElementType;
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
  typedef double LargerType;
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
  typedef int SignedType;
  typedef uint UnsignedType;
};

template <>
struct TypeTraits<float2> {
  typedef float ElementType;
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
  typedef double2 LargerType;
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
  typedef int2 SignedType;
  typedef uint2 UnsignedType;
};

template <>
struct TypeTraits<float3> {
  typedef float ElementType;
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
  typedef double3 LargerType;
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
  typedef int3 SignedType;
  typedef uint3 UnsignedType;
};

template <>
struct TypeTraits<float4> {
  typedef float ElementType;
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
  typedef double4 LargerType;
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
  typedef int4 SignedType;
  typedef uint4 UnsignedType;
};

template <>
struct TypeTraits<float8> {
  typedef float ElementType;
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
  typedef double8 LargerType;
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
  typedef int8 SignedType;
  typedef uint8 UnsignedType;
};

template <>
struct TypeTraits<float16> {
  typedef float ElementType;
#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
  typedef double16 LargerType;
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
  typedef int16 SignedType;
  typedef uint16 UnsignedType;
};

template <>
struct TypeTraits<float32> {
  typedef float ElementType;
  // no larger type allowed for our 32-element type!
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <>
struct TypeTraits<double> {
  typedef double ElementType;
  // no larger type allowed for our double type!
  typedef long SignedType;
  typedef ulong UnsignedType;
};

template <>
struct TypeTraits<double2> {
  typedef double ElementType;
  // no larger type allowed for our double type!
  typedef long2 SignedType;
  typedef ulong2 UnsignedType;
};

template <>
struct TypeTraits<double3> {
  typedef double ElementType;
  // no larger type allowed for our double type!
  typedef long3 SignedType;
  typedef ulong3 UnsignedType;
};

template <>
struct TypeTraits<double4> {
  typedef double ElementType;
  // no larger type allowed for our double type!
  typedef long4 SignedType;
  typedef ulong4 UnsignedType;
};

template <>
struct TypeTraits<double8> {
  typedef double ElementType;
  // no larger type allowed for our double type!
  typedef long8 SignedType;
  typedef ulong8 UnsignedType;
};

template <>
struct TypeTraits<double16> {
  typedef double ElementType;
  // no larger type allowed for our double type!
  typedef long16 SignedType;
  typedef ulong16 UnsignedType;
};

template <>
struct TypeTraits<double32> {
  typedef double ElementType;
  // no larger type allowed for our double type!
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
struct Bounds {
  static inline T min();
  static inline T max();
};

template <>
uchar Bounds<uchar>::min() {
  return 0;
}
template <>
uchar Bounds<uchar>::max() {
  return 0xFF;
}

template <>
char Bounds<char>::min() {
  return 0x80;
}
template <>
char Bounds<char>::max() {
  return 0x7F;
}

template <>
ushort Bounds<ushort>::min() {
  return 0;
}
template <>
ushort Bounds<ushort>::max() {
  return 0xFFFF;
}

template <>
short Bounds<short>::min() {
  return 0x8000;
}
template <>
short Bounds<short>::max() {
  return 0x7FFF;
}

template <>
uint Bounds<uint>::min() {
  return 0;
}
template <>
uint Bounds<uint>::max() {
  return 0xFFFFFFFF;
}

template <>
int Bounds<int>::min() {
  return 0x80000000;
}
template <>
int Bounds<int>::max() {
  return 0x7FFFFFFF;
}

template <>
ulong Bounds<ulong>::min() {
  return 0;
}
template <>
ulong Bounds<ulong>::max() {
  return 0xFFFFFFFFFFFFFFFF;
}

template <>
long Bounds<long>::min() {
  return 0x8000000000000000;
}
template <>
long Bounds<long>::max() {
  return 0x7FFFFFFFFFFFFFFF;
}

template <>
float Bounds<float>::min() {
  __builtin_unreachable();
}
template <>
float Bounds<float>::max() {
  __builtin_unreachable();
}

template <typename T>
struct GetNumElements {
  enum { Size = sizeof(T) / sizeof(typename TypeTraits<T>::ElementType) };
};

template <>
struct GetNumElements<char3> {
  enum { Size = 3 };
};

template <>
struct GetNumElements<uchar3> {
  enum { Size = 3 };
};

template <>
struct GetNumElements<short3> {
  enum { Size = 3 };
};

template <>
struct GetNumElements<ushort3> {
  enum { Size = 3 };
};

template <>
struct GetNumElements<int3> {
  enum { Size = 3 };
};

template <>
struct GetNumElements<uint3> {
  enum { Size = 3 };
};

template <>
struct GetNumElements<long3> {
  enum { Size = 3 };
};

template <>
struct GetNumElements<ulong3> {
  enum { Size = 3 };
};

template <>
struct GetNumElements<float3> {
  enum { Size = 3 };
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <>
struct GetNumElements<double3> {
  enum { Size = 3 };
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T, typename U>
T select(const T a, const T b, const U c);

template <ocl::Rounding::Type ROUNDING, typename T, typename F,
          unsigned int ELEMENTS = GetNumElements<T>::Size>
struct HalfConvertHelper {
  typedef typename TypeTraits<T>::ElementType TElem;
  typedef typename TypeTraits<F>::ElementType FElem;
  typedef typename TypeTraits<FElem>::UnsignedType UnsignedType;

  // Take the sign of a 16-bit floating point that has been numerically rounded
  // to an infinity, and return a 16-bit floating point number that has been
  // rounded appropriately for the ROUNDING mode.
  //
  // As the only possible outputs are -/+ versions of either the largest
  // representable value or infinity we don't actually need to known the input
  // number, just its sign.
  //
  // Different rounding modes handle rounding near infinities differently.
  // Specifically, when rounding a number puts it outside the range of
  // representable numbers some modes round this off to infinity, some round it
  // to the largest representable number, and some pick a behaviour based on
  // sign.  The relevant ROUNDING template specializations implement the
  // required behavior for that mode.
  static inline ushort RoundNearInfinity(bool sign);

  // Shift 'x' right by 'shift' bits (i.e. 'x >> shift'), but handle the
  // ROUNDING mode appropriately.  Some ROUNDING modes require the sign of the
  // number for correct behavior.
  //
  // When we're shifting a mantissa to the right (i.e. scaling it down to a
  // reduced range) we need to handle various cases differently based on the
  // rounding mode.  E.g. which direction to round, or how to break ties.  So,
  // provide an implementation for each ROUNDING template specialisation.
  static inline UnsignedType ShiftRightLogical(UnsignedType x, uint shift,
                                               bool sign);

  // Convert a T (of some vector) to F (of some vector), but looping over all
  // elements and invoking the scalar version of this templated function.
  //
  // Unfortunately, although other functions in this file use
  // __builtin_convertvector this function cannot as 16-bit floats are not
  // supported in C++ and thus cannot be expressed here.
  static T _(const F payload) {
    static_assert(GetNumElements<T>::Size == GetNumElements<F>::Size,
                  "Input and output vector sizes do not match.");

    T t;
    TElem *tp = (TElem *)&t;
    const FElem *fp = (const FElem *)&payload;

    for (unsigned int i = 0; i < ELEMENTS; i++) {
      tp[i] = HalfConvertHelper<ROUNDING, TElem, FElem, 1>::_(fp[i]);
    }

    return t;
  }
};

// Vector type overload used by `quantizeToF16`.
template <typename T, unsigned int ELEMENTS = GetNumElements<T>::Size>
struct QuantizeToF16Helper {
  using TElem = typename TypeTraits<T>::ElementType;

  // Loop over the vector elements calling the scalar function on each.
  static T _(const T payload) {
    TElem *elems = (TElem *)&payload;

    for (unsigned int i = 0; i < ELEMENTS; i++) {
      elems[i] = QuantizeToF16Helper<TElem, 1>::_(elems[i]);
    }

    return payload;
  }
};

template <typename T>
struct FPBits;

// The number of bits in each component of an IEEE754 16-bit float.
template <>
struct FPBits<ushort> {
  static const uint Mantissa = 10;
  static const uint Exponent = 5;
  static const uint Sign = 1;
};

// The number of bits in each component of an IEEE754 32-bit float.
template <>
struct FPBits<float> {
  static const uint Mantissa = 23;
  static const uint Exponent = 8;
  static const uint Sign = 1;
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
// The number of bits in each component of an IEEE754 64-bit float.
template <>
struct FPBits<double> {
  static const uint Mantissa = 52;
  static const uint Exponent = 11;
  static const uint Sign = 1;
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

// Wrap code that is aware of the 'shape' of floating point numbers into one
// place.
template <typename T>
struct Shape {
  using Bits = FPBits<T>;

  // Types appropriate for doing calculations on this floating type, must be
  // able to fit the mantissa, which in practice means that it must be an
  // integer as large as the floating point type.
  using UnsignedType = typename TypeTraits<T>::UnsignedType;
  using SignedType = typename TypeTraits<T>::SignedType;

  // Note: These unions are used for type-punning, we should really be using
  // __builtin_memcpy to achieve this.  See redmine #5820.
  union Float {
    T f;
    struct {
      // Note that although only Mantissa requires UnsignedType, Exponent and
      // Sign could fit in uint for all cases, we use the same type everywhere
      // as these numbers will be used together in calculations.
      UnsignedType Mantissa : Bits::Mantissa;
      UnsignedType Exponent : Bits::Exponent;
      UnsignedType Sign : Bits::Sign;
    };
  };

  // The size of this type.
  static const uint NumBits = Bits::Mantissa + Bits::Exponent + Bits::Sign;
  static_assert(NumBits == (sizeof(T) * CHAR_BIT),
                "Unknown floating point bitwidth used");

  // Exponents are biased before being stored to allow negative and positive
  // exponents, but without having to use two's complement.  These numbers are
  // defined by the IEEE754 specification, but are 2^(BitsInExponent-1) - 1,
  // i.e. pre-bias 0 is in the middle of the range.
  // Spec values are half: 15, float: 127, double: 1023.
  static const uint Bias = (1u << (Bits::Exponent - 1u)) - 1u;

  // Setting a single bit, or all the exponent or mantissa bits to all 1's.
  static const UnsignedType ONE = 1u;
  static const UnsignedType ExponentOnes = (ONE << Bits::Exponent) - ONE;
  static const UnsignedType MantissaOnes = (ONE << Bits::Mantissa) - ONE;

  // If both exponent and mantissa is zero then this number is a zero.
  static inline bool Zero(Float x) {
    return ((x.Exponent == 0) && (x.Mantissa == 0));
  }

  // If the exponent is zero, but the mantissa is not then this is a denormal
  // (a.k.a. subnormal) number.
  static inline bool Denormal(Float x) {
    return ((x.Exponent == 0) && (x.Mantissa != 0));
  }

  // If the exponent is all ones, but the mantissa is zero this is an infinity.
  static inline bool Inf(Float x) {
    return ((x.Exponent == ExponentOnes) && (x.Mantissa == 0));
  }

  // If the exponent is all ones, but the mantissa is non-zero this is not a
  // number.
  static inline bool NaN(Float x) {
    return ((x.Exponent == ExponentOnes) && (x.Mantissa != 0));
  }
};

// Convert a half (16-bit floating point) to a float (32-bit or 64-bit floating
// point).  No rounding is required, the value of the half can always be
// directly represented as a float.  Denormal half values are correctly
// converted, and there are no possible half values that would require a
// denormal float for accurate representation.
//
// Note: The overall structure of this function, even algorithms used for each
// mode, are suboptimal. However its only usage currently is to implement
// `vload{a}_half` and `vstore{a}_half` on devices which don't have fp16
// support. Devices which support half use hardware conversion from
// 'abacus/internal/convert_helper.h' instead.
template <typename T>
struct HalfConvertHelper<ocl::Rounding::undefined, T, ushort, 1> {
  static T _(const ushort payload) {
    using From = Shape<ushort>;
    using To = Shape<T>;
    static_assert(From::NumBits < To::NumBits,
                  "From-type must be smaller than To-type");

    From::Float in;
    typename To::Float out;
    in.f = payload;

    if (From::Zero(in)) {
      // The input is zero, just preserve the sign.
      out.Mantissa = 0;
      out.Exponent = 0;
      out.Sign = in.Sign;
    } else if (From::Denormal(in)) {
      // The input is a denormal number, scale it up.  Keep on doubling the
      // mantissa and lowering the exponent by 1, until the mantissa does not
      // fit into the 10-bits provided for 16-bit float's mantissas, then take
      // the 10 bits left in the Mantissa as the new mantissa (padded with 13
      // or 42 more zero bits for float), and the produced exponent as the new
      // exponent (with bias factors applied).  This works because denormal
      // numbers are a fixed-point representation, i.e. linearly spaced, so
      // doubling mantissa and subtracting one from exponent is a mathematical
      // no-op.
      // Note: lowering the exponent is done via addition in the loop, because
      // it is subtracted at the end.
      From::SignedType Exponent = -1;
      From::UnsignedType Mantissa = in.Mantissa;
      do {
        Exponent++;
        Mantissa <<= 1u;
      } while ((Mantissa & (From::ONE << From::Bits::Mantissa)) == 0);

      uint shift = To::Bits::Mantissa - From::Bits::Mantissa;
      out.Mantissa = (Mantissa & From::MantissaOnes) << shift;
      out.Exponent = To::Bias - From::Bias - Exponent;
      out.Sign = in.Sign;
    } else if (From::Inf(in) || From::NaN(in)) {
      // The input is an inf or a NaN (it doesn't really matter which).  Set
      // all exponent bits of output to 1, and preserve sign and mantissa (with
      // scaling).
      typename To::UnsignedType Mantissa = in.Mantissa;  // Extend bitwidth
      out.Mantissa = Mantissa << (To::Bits::Mantissa - From::Bits::Mantissa);
      out.Exponent = To::ExponentOnes;
      out.Sign = in.Sign;
    } else {
      // This is just a normal number, scale appropriately.
      // Mantissa: copy, but with 13 or 42 extra zero bits in float.
      // Exponent: copy, but adjust for the difference in bias.
      typename To::UnsignedType Mantissa = in.Mantissa;  // Extend bitwidth
      out.Mantissa = Mantissa << (To::Bits::Mantissa - From::Bits::Mantissa);
      out.Exponent = To::Bias - From::Bias + in.Exponent;
      out.Sign = in.Sign;
    }

    return out.f;
  }
};

// Convert a float (32-bit or 64-bit floating point number) to a half (16-bit
// floating number) according to the ROUNDING mode.
//
// Note: Although the natural place for this function may seem to be the
// HalfCovertHelper struct it cannot go there as those structs are specialised
// based on the ROUNDING type (like this function), and thus every struct would
// need to provide this function (even if its contents were identical to this
// one).  Providing a generic implementation that calls back to specialised
// versions of HalfConvertHelper eliminates duplication.
template <ocl::Rounding::Type ROUNDING, typename F>
static inline ushort HalfDownConvertHelper(const F payload) {
  using From = Shape<F>;
  using To = Shape<ushort>;
  static_assert(To::NumBits < From::NumBits,
                "To-type must be smaller than From-type");

  typename From::Float in;
  To::Float out;
  in.f = payload;

  if (From::Zero(in)) {
    // The input is zero, just preserve the sign.
    out.Mantissa = 0;
    out.Exponent = 0;
    out.Sign = in.Sign;
  } else if (From::NaN(in)) {
    // The input is a NaN, so the output is a NaN.  We preserve the upper
    // mantissa bits in case they are used for signalling, but we force the
    // lower bit on to ensure we never have a zero mantissa (which would be
    // interpreted as an infinity).  Set all exponent bits of output to 1, and
    // preserve sign.
    uint shift = From::Bits::Mantissa - To::Bits::Mantissa;
    out.Mantissa = (in.Mantissa >> shift) | 0x1;
    out.Exponent = To::ExponentOnes;
    out.Sign = in.Sign;
  } else if (From::Inf(in)) {
    // The input is an inf, so the output is an inf.  Set all exponent bits of
    // output to 1, and all mantissa bits to 0, preserve sign.
    out.Mantissa = 0;
    out.Exponent = To::ExponentOnes;
    out.Sign = in.Sign;
  } else if ((in.Exponent + To::Bias) <= From::Bias) {
    // This floating point number is too small to be representable as a
    // normal-half, so try to create a denormal half.
    //
    // Note: Denormal float inputs will always end up in this case (due to
    // having a zero exponent).  Technically this code is handling them
    // incorrectly because we're treating them as normal numbers, but that just
    // means that they get calculated as a zero, which is numerically the
    // closest representable value as a 16-bit float.  I.e. the correct answer
    // (even if found the wrong way).  However, by doing this calculation here
    // instead of in a separate case we can handle the rounding correctly in
    // the RTN and RTP cases, where a denormal input may need to be rounded to
    // smallest representable denormal 16-bit float depending on the sign.

    // Unlike with normal numbers, the value of the input exponent affects the
    // output mantissa for denormal numbers.  So scale the mantissa by both the
    // difference in bits available, and the (biased) input exponent.
    //
    // Note: We add 1 to the shift to pair with setting the 24th or 53rd bit in
    // the mantissa before shifting below.
    uint bias_exponent = in.Exponent + To::Bias - From::Bias;
    uint shift = From::Bits::Mantissa - To::Bits::Mantissa - bias_exponent + 1u;

    // The mantissa is produced by shifting using the relevant ROUNDING mode.
    //
    // Note: Before shifting the input mantissa we set the next bit to one,
    // e.g. a 32-bit float has a 23-bit mantissa, so we set the 24th bit (1u <<
    // 23).  Countering this is why we added one to the shift above, but by
    // doing this we ensure that we round correctly for denormals (i.e. this is
    // not required for the normal case).
    To::UnsignedType mantissa =
        HalfConvertHelper<ROUNDING, ushort, F, 1>::ShiftRightLogical(
            in.Mantissa | (From::ONE << From::Bits::Mantissa), shift, in.Sign);

    if (mantissa == (To::ONE << To::Bits::Mantissa)) {
      // If the mantissa has been rounded up to 1024 (i.e. 1u << 10) it won't
      // fit into the 16-bit float mantissa of 10-bits, so we've actually
      // rounded up to the smallest representable normal number.
      out.Mantissa = 0;
      out.Exponent = To::ONE;
      out.Sign = in.Sign;
    } else {
      out.Mantissa = mantissa;
      out.Exponent = 0;
      out.Sign = in.Sign;
    }
  } else {
    // This input is just a normal number, scale appropriately.
    // Mantissa: Scale from 23- to 10-bits using ROUNDING mode.
    // Exponent: copy, but adjust for the difference in bias.
    uint shift = From::Bits::Mantissa - To::Bits::Mantissa;
    To::UnsignedType mantissa =
        HalfConvertHelper<ROUNDING, ushort, F, 1>::ShiftRightLogical(
            in.Mantissa, shift, in.Sign);
    To::UnsignedType exponent = To::Bias - From::Bias + in.Exponent;

    if (mantissa == (To::ONE << To::Bits::Mantissa)) {
      // If the mantissa has been rounded up to 1024 (i.e. 1u << 10) it won't
      // fit into the 16-bit float mantissa of 10-bits, so we round up to the
      // next representable number (i.e. bump the exponent).
      mantissa = 0;
      ++exponent;
    }

    if (exponent >= To::ExponentOnes) {
      // We may have produced a number larger than the largest representable
      // number, numerically this will have been calculated as a too large
      // exponent (e.g. overlapping with the inf/NaN representation).  What to
      // do here depends on the rounding mode, but the options are either to go
      // with an infinite or round down to the largest representable value.
      out.f =
          HalfConvertHelper<ROUNDING, ushort, F, 1>::RoundNearInfinity(in.Sign);
    } else {
      out.Mantissa = mantissa;
      out.Exponent = exponent;
      out.Sign = in.Sign;
    }
  }

  return out.f;
}

// Convert a float (32-bit or 64-bit floating point number) to a half (16-bit
// floating point number), using round-to-nearest-even.  Denormal half values
// will be produced for small float values.
template <typename F>
struct HalfConvertHelper<ocl::Rounding::rte, ushort, F, 1> {
  // In round-to-nearest-even mode all numbers that are between the largest
  // representable number and infinity round to infinity.
  static inline ushort RoundNearInfinity(bool sign) {
    Shape<ushort>::Float out;

    out.Mantissa = 0;
    out.Exponent = Shape<ushort>::ExponentOnes;
    out.Sign = sign;

    return out.f;
  }

  // Round-to-nearest even mode is a little complicated when shifting the
  // mantissa, as the rounding bias is heavily determined by the input value,
  // so there are many cases to handle.
  using UnsignedType = typename Shape<F>::UnsignedType;
  static inline UnsignedType ShiftRightLogical(UnsignedType x, uint shift,
                                               bool sign) {
    (void)sign;  // We do not need to know the sign to calculate this.

    // Shifting >#bits trivially results in a zero output.  Shifting by exactly
    // #bits requires preserving the MSB for rounding purposes (even though it
    // should get discarded by the shift) -- this case would not be handled by
    // the <#bits shift code below as that would attempt to left-shift by
    // #bits.
    if (shift > Shape<F>::NumBits) {
      return 0u;
    } else if (shift == Shape<F>::NumBits) {
      return x >> (Shape<F>::NumBits - 1u);
    }

    // Only bits that will be discarded affect rounding.
    UnsignedType round = x & ((Shape<F>::ONE << shift) - Shape<F>::ONE);

    if (round < (Shape<F>::ONE << (shift - 1u))) {
      // Closer to the number below, round down.
      return x >> shift;
    } else if (round > (Shape<F>::ONE << (shift - 1u))) {
      // Closer to the number above, round up.
      return (x >> shift) + Shape<F>::ONE;
    } else {
      // Exactly between two numbers, round in the direction of even.
      UnsignedType tmp = x >> shift;
      return (tmp & 0x1) ? (tmp + Shape<F>::ONE) : tmp;
    }
  }

  static ushort _(const F payload) {
    return HalfDownConvertHelper<ocl::Rounding::rte>(payload);
  }
};

// Convert a float (32-bit or 64-bit floating point number) to a half (16-bit
// floating point number), using default rounding mode (round to even).
template <typename F>
struct HalfConvertHelper<ocl::Rounding::undefined, ushort, F, 1> {
  static ushort _(const F payload) {
    // The default rounding mode is RTE.
    return HalfConvertHelper<ocl::Rounding::rte, ushort, F, 1>::_(payload);
  }
};

// Convert a float (32-bit or 64-bit floating point number) to a half (16-bit
// floating point number), using round-to-zero.  Denormal half values will be
// produced for small float values.
template <typename F>
struct HalfConvertHelper<ocl::Rounding::rtz, ushort, F, 1> {
  // In round-to-zero mode all numbers that are between the largest
  // representable number and infinity round to the largest representable
  // number.
  static inline ushort RoundNearInfinity(bool sign) {
    Shape<ushort>::Float out;

    out.Mantissa = Shape<ushort>::MantissaOnes;
    out.Exponent = Shape<ushort>::ExponentOnes - 1u;
    out.Sign = sign;

    return out.f;
  }

  // For cases where the shift is <#bits the round-to-zero SRL matches the
  // behaviour of the C >> operator, so use it directly.
  using UnsignedType = typename Shape<F>::UnsignedType;
  static inline UnsignedType ShiftRightLogical(UnsignedType x, uint shift,
                                               bool sign) {
    (void)sign;  // We do not need to know the sign to calculate this.

    // Shifting >#bits trivially results in a zero output.  Shifting by exactly
    // #bits requires preserving the MSB for rounding purposes (even though it
    // should get discarded by the shift).
    if (shift > Shape<F>::NumBits) {
      return 0u;
    } else if (shift == Shape<F>::NumBits) {
      return x >> (Shape<F>::NumBits - 1u);
    }

    return x >> shift;
  }

  static ushort _(const F payload) {
    return HalfDownConvertHelper<ocl::Rounding::rtz>(payload);
  }
};

// Convert a float (32-bit or 64-bit floating point number) to a half (16-bit
// floating point number), using round-to-negative-infinity.  Denormal half
// values will be produced for small float values.
template <typename F>
struct HalfConvertHelper<ocl::Rounding::rtn, ushort, F, 1> {
  // In round-to-negative-infinity mode all numbers that are between the
  // largest representable number and infinity round down (i.e. whether the
  // output is infinity depends on the sign on the input number).
  static inline ushort RoundNearInfinity(bool sign) {
    Shape<ushort>::Float out;

    // If the input is a positive number, round to the largest representable
    // 16-bit floating point number (i.e. round down).  If the input is a
    // negative number, round to negative infinity (i.e. round down).
    out.Mantissa = sign ? 0 : Shape<ushort>::MantissaOnes;
    out.Exponent = Shape<ushort>::ExponentOnes - (sign ? 0 : 1u);
    out.Sign = sign;

    return out.f;
  }

  // Rounding during SRL for round-to-negative-infinity depends on the sign of
  // the input, as we always round to negative infinity we need to round the
  // mantissa up or down for a negative or positive number respectively.
  using UnsignedType = typename Shape<F>::UnsignedType;
  static inline UnsignedType ShiftRightLogical(UnsignedType x, uint shift,
                                               bool sign) {
    if (shift >= Shape<F>::NumBits) {
      return sign;
    }

    // Only bits that will be discarded affect rounding.
    UnsignedType round = x & ((Shape<F>::ONE << shift) - Shape<F>::ONE);

    if (round == 0) {
      // The input is exactly representable after shifting.
      return x >> shift;
    } else {
      // We are rounding the number, so round towards -inf.
      return (x >> shift) + sign;
    }
  }

  static ushort _(const F payload) {
    return HalfDownConvertHelper<ocl::Rounding::rtn>(payload);
  }
};

// Convert a float (32-bit or 64-bit floating point number) to a half (16-bit
// floating point number), using round-to-positive-infinity.  Denormal half
// values will be produced for small float values.
template <typename F>
struct HalfConvertHelper<ocl::Rounding::rtp, ushort, F, 1> {
  // In round-to-positive-infinity mode all numbers that are between the
  // largest representable number and infinity round up (i.e. whether the
  // output is infinity depends on the sign on the input number).
  static inline ushort RoundNearInfinity(bool sign) {
    Shape<ushort>::Float out;

    // If the input is a positive number, round to infinity (i.e. round up).
    // If the input is a negative number, round to the largest representable
    // negative 16-bit floating point number (i.e. round up).
    out.Mantissa = sign ? Shape<ushort>::MantissaOnes : 0;
    out.Exponent = Shape<ushort>::ExponentOnes - sign;
    out.Sign = sign;

    return out.f;
  }

  // Rounding during SRL for round-to-positive-infinity depends on the sign of
  // the input, as we always round to positive infinity we need to round the
  // mantissa up or down for a positive or negative number respectively.
  using UnsignedType = typename Shape<F>::UnsignedType;
  static inline UnsignedType ShiftRightLogical(UnsignedType x, uint shift,
                                               bool sign) {
    if (shift >= Shape<F>::NumBits) {
      return sign ? 0 : Shape<F>::ONE;
    }

    // Only bits that will be discarded affect rounding.
    UnsignedType round = x & ((Shape<F>::ONE << shift) - Shape<F>::ONE);

    if (round == 0) {
      // The input is exactly representable after shifting.
      return x >> shift;
    } else {
      // We are rounding the number, so round towards +inf.
      return (x >> shift) + (sign ? 0u : Shape<F>::ONE);
    }
  }

  static ushort _(const F payload) {
    return HalfDownConvertHelper<ocl::Rounding::rtp>(payload);
  }
};

// Just a wrapper around HalfConvertHelper in effect, except that
// HalfConvertHelper will infer the vector width of T and F.
template <ocl::Rounding::Type ROUNDING, typename T, typename F>
T half_convert(const F payload) {
  return HalfConvertHelper<ROUNDING, T, F>::_(payload);
}

// Reduce the precision of a 32-bit float to what can be represented by a half
// (16-bit float).
template <typename T>
struct QuantizeToF16Helper<T, 1> {
  static T _(const T payload) {
    using Float = Shape<T>;

    typename Float::Float in;
    in.f = payload;

    if (Float::NaN(in) || Float::Inf(in)) {
      // If we get a NaN or an infinity all we need to do is return it.
      return in.f;
    } else if (in.f < -65504 || in.f > 65504) {
      // If the number's magnitude is too great to represent as a half the
      // result is a sign preserved infinity. The values in the if represent the
      // largest numbers representable in a half (all mantissa bits on, exponent
      // set to +15).
      in.Mantissa = 0;
      in.Exponent = Float::ExponentOnes;
    } else if ((0 < in.f && in.f < 0.000061035) ||
               (-0.000061035 < in.f && in.f < 0)) {
      // If the number is too small to be represented as a normalized half set
      // it to zero. The values in the if are the smallest representable in a
      // normal half (all mantissa bits off, exponent set to -14).
      in.Exponent = 0;
      in.Mantissa = 0;
    } else {
      // Otherwise simply trim down the precision by only keeping the ten most
      // significant bits of the mantissa.
      in.Mantissa >>= 13;
      in.Mantissa <<= 13;
    }

    return in.f;
  }
};

// Wrapper around QuatizeToF16Helper to facilitate handling vector types
// correctly.
template <typename T>
T quantizeToF16(const T payload) {
  return QuantizeToF16Helper<T>::_(payload);
}

/// @}
}  // namespace ocl

#endif  // OCL_CXXBUILTINS_H_INCLUDED
