// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_extra.h>
#include <abacus/abacus_type_traits.h>

using namespace abacus::detail;

namespace {
template <typename T>
T bit_reverse(const T t) {
  typedef typename TypeTraits<T>::UnsignedType UnsignedType;
  typedef typename TypeTraits<UnsignedType>::ElementType UnsignedElementType;

  const abacus_ulong table[5] = {
      0x5555555555555555, 0x3333333333333333, 0x0f0f0f0f0f0f0f0f,
      0x00ff00ff00ff00ff, 0x0000ffff0000ffff,
  };

  const abacus_int halfBits = (sizeof(UnsignedElementType) * 8) / 2;

  UnsignedType result = cast::as<UnsignedType>(t);

  for (abacus_int i = 0; (1 << i) < halfBits; i++) {
    const UnsignedElementType mask =
        cast::convert<UnsignedElementType>(table[i]);
    const UnsignedElementType shift =
        cast::convert<UnsignedElementType>(1 << i);

    result = ((result >> shift) & mask) | ((result & mask) << shift);
  }

  const UnsignedElementType shift =
      cast::convert<UnsignedElementType>(halfBits);
  result = (result >> shift) | (result << shift);

  return cast::as<T>(result);
}
}  // namespace

#define DEF(TYPE) \
  TYPE ABACUS_API __abacus_bit_reverse(TYPE x) { return bit_reverse<>(x); }

DEF(abacus_char);
DEF(abacus_char2);
DEF(abacus_char3);
DEF(abacus_char4);
DEF(abacus_char8);
DEF(abacus_char16);

DEF(abacus_uchar);
DEF(abacus_uchar2);
DEF(abacus_uchar3);
DEF(abacus_uchar4);
DEF(abacus_uchar8);
DEF(abacus_uchar16);

DEF(abacus_short);
DEF(abacus_short2);
DEF(abacus_short3);
DEF(abacus_short4);
DEF(abacus_short8);
DEF(abacus_short16);

DEF(abacus_ushort);
DEF(abacus_ushort2);
DEF(abacus_ushort3);
DEF(abacus_ushort4);
DEF(abacus_ushort8);
DEF(abacus_ushort16);

DEF(abacus_int);
DEF(abacus_int2);
DEF(abacus_int3);
DEF(abacus_int4);
DEF(abacus_int8);
DEF(abacus_int16);

DEF(abacus_uint);
DEF(abacus_uint2);
DEF(abacus_uint3);
DEF(abacus_uint4);
DEF(abacus_uint8);
DEF(abacus_uint16);

DEF(abacus_long);
DEF(abacus_long2);
DEF(abacus_long3);
DEF(abacus_long4);
DEF(abacus_long8);
DEF(abacus_long16);

DEF(abacus_ulong);
DEF(abacus_ulong2);
DEF(abacus_ulong3);
DEF(abacus_ulong4);
DEF(abacus_ulong8);
DEF(abacus_ulong16);
