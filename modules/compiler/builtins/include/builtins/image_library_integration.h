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

#ifndef CODEPLAY_IMAGE_LIBRARY_INTEGRATION_H
#define CODEPLAY_IMAGE_LIBRARY_INTEGRATION_H

#ifdef __CODEPLAY_OCL_IMAGE_SUPPORT
#include "builtins.h"
#endif

#ifdef _MSC_VER
#define ABACUS_LIBRARY_STATIC
#endif

#include <abacus/abacus_cast.h>
#include <abacus/abacus_common.h>
#include <abacus/abacus_integer.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_relational.h>

namespace libimg {
/// @addtogroup builtins
/// @{

typedef bool Bool;

typedef abacus_char Char;
typedef abacus_char2 Char2;
typedef abacus_char4 Char4;

typedef abacus_uchar UChar;
typedef abacus_uchar2 UChar2;
typedef abacus_uchar4 UChar4;

typedef abacus_short Short;
typedef abacus_short2 Short2;
typedef abacus_short4 Short4;

typedef abacus_ushort UShort;
typedef abacus_ushort2 UShort2;
typedef abacus_ushort4 UShort4;

typedef abacus_int Int;
typedef abacus_int2 Int2;
typedef abacus_int4 Int4;

typedef abacus_uint UInt;
typedef abacus_uint2 UInt2;
typedef abacus_uint4 UInt4;

typedef abacus_float Float;
typedef abacus_float2 Float2;
typedef abacus_float4 Float4;

typedef abacus_ushort Half;
typedef abacus_ushort4 Half4;

typedef size_t Size;

enum class vec_elem { x = 0, y = 1, z = 2, w = 3 };

template <typename ElemType, typename VecType>
inline ElemType get_v2(const VecType &v, const vec_elem elem) {
  switch (elem) {
    default:
      return 0;
    case vec_elem::x:
      return v[0];
    case vec_elem::y:
      return v[1];
  }
}

template <typename ElemType, typename VecType>
inline ElemType get_v4(const VecType &v, const vec_elem elem) {
  switch (elem) {
    default:
      return 0;
    case vec_elem::x:
      return v[0];
    case vec_elem::y:
      return v[1];
    case vec_elem::z:
      return v[2];
    case vec_elem::w:
      return v[3];
  }
}

template <typename VecType, typename ElemType>
inline void set_v2(VecType &v, const ElemType val, const vec_elem elem) {
  switch (elem) {
    default:
      return;
    case vec_elem::x:
      v[0] = val;
      return;
    case vec_elem::y:
      v[1] = val;
      return;
  }
}

template <typename VecType, typename ElemType>
inline void set_v4(VecType &v, const ElemType val, const vec_elem elem) {
  switch (elem) {
    default:
      return;
    case vec_elem::x:
      v[0] = val;
      return;
    case vec_elem::y:
      v[1] = val;
      return;
    case vec_elem::z:
      v[2] = val;
      return;
    case vec_elem::w:
      v[3] = val;
      return;
  }
}

template <typename VecType, typename ElemType>
inline VecType make(ElemType x, ElemType y) {
  VecType ret;
  ret[0] = x;
  ret[1] = y;
  return ret;
}

template <typename VecType, typename ElemType>
inline VecType make(ElemType x, ElemType y, ElemType z, ElemType w) {
  VecType ret;
  ret[0] = x;
  ret[1] = y;
  ret[2] = z;
  ret[3] = w;
  return ret;
}

template <typename Type>
inline Char convert_char_sat(Type value) {
  return __abacus_convert_char_sat(value);
}
template <typename Type>
inline Char convert_char_sat_rte(Type value) {
  return __abacus_convert_char_sat_rte(value);
}
template <typename Type>
inline Char4 convert_char4_sat(Type value) {
  return __abacus_convert_char4_sat(value);
}

template <typename Type>
inline UChar convert_uchar_sat(Type value) {
  return __abacus_convert_uchar_sat(value);
}
template <typename Type>
inline UChar convert_uchar_sat_rte(Type value) {
  return __abacus_convert_uchar_sat_rte(value);
}
template <typename Type>
inline UChar4 convert_uchar4_sat(Type value) {
  return __abacus_convert_uchar4_sat(value);
}

template <typename Type>
inline Short convert_short_sat(Type value) {
  return __abacus_convert_short_sat(value);
}
template <typename Type>
inline Short convert_short_sat_rte(Type value) {
  return __abacus_convert_short_sat_rte(value);
}
template <typename Type>
inline Short4 convert_short4_sat(Type value) {
  return __abacus_convert_short4_sat(value);
}

template <typename Type>
inline UShort convert_ushort_sat(Type value) {
  return __abacus_convert_ushort_sat(value);
}
template <typename Type>
inline UShort convert_ushort_sat_rte(Type value) {
  return __abacus_convert_ushort_sat_rte(value);
}
template <typename Type>
inline UShort4 convert_ushort4_sat(Type value) {
  return __abacus_convert_ushort4_sat(value);
}

template <typename Type>
inline Int convert_int_rte(Type value) {
  return __abacus_convert_int_rte(value);
}

template <typename Type>
inline Float2 convert_float2(Type value) {
  return __abacus_convert_float2(value);
}

template <typename Type>
inline Float4 convert_float4(Type value) {
  return __abacus_convert_float4(value);
}

template <typename Type>
inline Type clamp(Type x, Type minval, Type maxval) {
  return __abacus_clamp(x, minval, maxval);
}

template <typename Type>
inline Type fabs(Type value) {
  return __abacus_fabs(value);
}

template <typename Type>
inline Type floor(Type value) {
  return __abacus_floor(value);
}

inline Int isinf(Float value) { return __abacus_isinf(value); }

template <typename Type>
inline Type min(Type a, Type b) {
  return __abacus_min(a, b);
}

template <typename Type>
inline Type max(Type a, Type b) {
  return __abacus_max(a, b);
}

template <typename Type>
inline Type rint(Type value) {
  return __abacus_rint(value);
}

template <typename Type>
inline Type pow(Type value, Type power) {
  return __abacus_pow(value, power);
}

#ifndef __OPENCL_VERSION__
namespace detail {
template <typename T>
struct TypeTraits;

template <>
struct TypeTraits<Float> {
  typedef Float ElementType;
  static const unsigned num_elements = 1;
};

template <>
struct TypeTraits<Float4> {
  typedef Float ElementType;
  static const unsigned num_elements = 4;
};

template <>
struct TypeTraits<Short> {
  typedef Short ElementType;
  static const unsigned num_elements = 1;
};

template <>
struct TypeTraits<Short4> {
  typedef Short ElementType;
  static const unsigned num_elements = 4;
};

template <>
struct TypeTraits<UShort> {
  typedef UShort ElementType;
  static const unsigned num_elements = 1;
};

template <>
struct TypeTraits<UShort4> {
  typedef UShort ElementType;
  static const unsigned num_elements = 4;
};

template <typename T, typename F>
struct HalfConvertHelper_rte {
  static_assert(TypeTraits<T>::num_elements == TypeTraits<F>::num_elements,
                "T and F must have the same number of elements");
  typedef typename TypeTraits<T>::ElementType TElem;
  typedef typename TypeTraits<F>::ElementType FElem;
  static const unsigned ELEMENTS = TypeTraits<T>::num_elements;
  static inline UShort RoundNearInfinity(bool sign);
  static inline UInt ShiftRightLogical(UInt x, UInt shift, bool sign);
  static T _(const F payload) {
    T t;
    TElem *tp = (TElem *)&t;
    const FElem *fp = (const FElem *)&payload;

    for (unsigned int i = 0; i < ELEMENTS; i++) {
      tp[i] = HalfConvertHelper_rte<TElem, FElem>::_(fp[i]);
    }

    return t;
  }
};

struct Shape {
  union Float16 {
    UShort f;
    struct {
      UInt Mantissa : 10;
      UInt Exponent : 5;
      UInt Sign : 1;
    } r;
  };
  union Float32 {
    Float f;
    struct {
      UInt Mantissa : 23;
      UInt Exponent : 8;
      UInt Sign : 1;
    } r;
  };
  static const UInt Float16Bias = 15;
  static const UInt Float32Bias = 127;
  static bool Zero(Float16 x) {
    return ((x.r.Exponent == 0) && (x.r.Mantissa == 0));
  }
  static bool Zero(Float32 x) {
    return ((x.r.Exponent == 0) && (x.r.Mantissa == 0));
  }
  static bool Denormal(Float16 x) {
    return ((x.r.Exponent == 0) && (x.r.Mantissa != 0));
  }
  static bool Denormal(Float32 x) {
    return ((x.r.Exponent == 0) && (x.r.Mantissa != 0));
  }
  static bool Inf(Float16 x) {
    return ((x.r.Exponent == 0x1f) && (x.r.Mantissa == 0));
  }
  static bool Inf(Float32 x) {
    return ((x.r.Exponent == 0xff) && (x.r.Mantissa == 0));
  }
  static bool NaN(Float16 x) {
    return ((x.r.Exponent == 0x1f) && (x.r.Mantissa != 0));
  }
  static bool NaN(Float32 x) {
    return ((x.r.Exponent == 0xff) && (x.r.Mantissa != 0));
  }
};

static inline UShort HalfDownConvertHelper_rte(const Float payload);

template <>
struct HalfConvertHelper_rte<UShort, Float> {
  static UShort RoundNearInfinity(bool sign) {
    Shape::Float16 out;

    out.r.Mantissa = 0;
    out.r.Exponent = 0x1f;
    out.r.Sign = sign;

    return out.f;
  }
  static UInt ShiftRightLogical(UInt x, UInt shift, bool sign) {
    (void)sign;
    if (shift > 32u) {
      return 0u;
    } else if (shift == 32u) {
      return x >> 31;
    }
    const UInt round = x & ((1u << shift) - 1);
    if (round < (1u << (shift - 1u))) {
      return x >> shift;
    } else if (round > (1u << (shift - 1u))) {
      return (x >> shift) + 1u;
    } else {
      const UInt tmp = x >> shift;
      return (tmp & 0x1) ? (tmp + 1u) : tmp;
    }
  }
  static UShort _(const float payload) {
    return HalfDownConvertHelper_rte(payload);
  }
};

static inline UShort HalfDownConvertHelper_rte(const Float payload) {
  Shape::Float32 in;
  Shape::Float16 out;
  in.f = payload;

  if (Shape::Zero(in)) {
    out.r.Mantissa = 0;
    out.r.Exponent = 0;
    out.r.Sign = in.r.Sign;
  } else if (Shape::NaN(in)) {
    out.r.Mantissa = (in.r.Mantissa >> (23 - 10)) | 0x1;
    out.r.Exponent = 0x1f;
    out.r.Sign = in.r.Sign;
  } else if (Shape::Inf(in)) {
    out.r.Mantissa = 0;
    out.r.Exponent = 0x1f;
    out.r.Sign = in.r.Sign;
  } else if ((in.r.Exponent + Shape::Float16Bias) <= Shape::Float32Bias) {
    const Int bias_exponent =
        in.r.Exponent + Shape::Float16Bias - Shape::Float32Bias;
    const UInt shift = 23u - 10u - bias_exponent + 1u;
    const UInt mantissa =
        HalfConvertHelper_rte<UShort, Float>::ShiftRightLogical(
            in.r.Mantissa | (1u << 23), shift, in.r.Sign);
    if (mantissa == (1u << 10)) {
      out.r.Mantissa = 0;
      out.r.Exponent = 0x1;
      out.r.Sign = in.r.Sign;
    } else {
      out.r.Mantissa = mantissa;
      out.r.Exponent = 0;
      out.r.Sign = in.r.Sign;
    }
  } else {
    const UInt shift = 23u - 10u;
    UInt mantissa = HalfConvertHelper_rte<UShort, Float>::ShiftRightLogical(
        in.r.Mantissa, shift, in.r.Sign);
    UInt exponent = Shape::Float16Bias - Shape::Float32Bias + in.r.Exponent;
    if (mantissa == (1u << 10)) {
      mantissa = 0;
      ++exponent;
    }
    if (exponent > 0x1e) {
      out.f =
          HalfConvertHelper_rte<UShort, Float>::RoundNearInfinity(in.r.Sign);
    } else {
      out.r.Mantissa = mantissa;
      out.r.Exponent = exponent;
      out.r.Sign = in.r.Sign;
    }
  }
  return out.f;
}

template <typename T, typename F>
inline T half_convert_rte(const F payload) {
  return HalfConvertHelper_rte<T, F>::_(payload);
}
}  // namespace detail

inline UShort convert_float_to_half(Float arg) {
  return detail::half_convert_rte<UShort, Float>(arg);
}

inline Float convert_half_to_float(UShort arg) {
  return detail::half_convert_rte<Float, UShort>(arg);
}

inline UShort4 convert_float4_to_half4_rte(Float4 arg) {
  return detail::half_convert_rte<UShort4, Float4>(arg);
}
#else
using ::convert_float4_to_half4_rte;
using ::convert_float_to_half;
using ::convert_half_to_float;
#endif

/// @}
}  // namespace libimg

#endif
