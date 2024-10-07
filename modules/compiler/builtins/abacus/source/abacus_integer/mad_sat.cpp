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

#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_integer.h>
#include <abacus/abacus_type_traits.h>

namespace {
template <typename T>
T mad_sat(const T x, const T y, const T z) {
  typedef typename TypeTraits<T>::LargerType LargerType;
  typedef typename TypeTraits<T>::ElementType ElementType;
  typedef typename TypeTraits<LargerType>::ElementType LargerElementType;
  const LargerType bX = abacus::detail::cast::convert<LargerType>(x);
  const LargerType bY = abacus::detail::cast::convert<LargerType>(y);
  const LargerType bZ = abacus::detail::cast::convert<LargerType>(z);
  const LargerType bR = (bX * bY) + bZ;
  return abacus::detail::cast::convert<T>(
      __abacus_clamp(bR,
                     abacus::detail::cast::convert<LargerElementType>(
                         TypeTraits<ElementType>::min()),
                     abacus::detail::cast::convert<LargerElementType>(
                         TypeTraits<ElementType>::max())));
}

struct ulonger {
  abacus_uint4 payload;

  ulonger() : payload(0) { ; }

  ulonger(const abacus_ulong val) : payload(0) {
    payload[2] = abacus::detail::cast::convert<abacus_uint>(val >> 32);
    payload[3] = abacus::detail::cast::convert<abacus_uint>(val & 0xFFFFFFFF);
  }

  ulonger(const abacus_uint i0, const abacus_uint i1, const abacus_uint i2,
          const abacus_uint i3)
      : payload(0) {
    payload[0] = i0;
    payload[1] = i1;
    payload[2] = i2;
    payload[3] = i3;
  }

  ulonger operator*(const ulonger &other) const {
    // Do the short multiplications
    const abacus_ulong lolo = (abacus_ulong)payload[3] * other.payload[3];
    const abacus_ulong lohi = (abacus_ulong)payload[3] * other.payload[2];
    const abacus_ulong hilo = (abacus_ulong)payload[2] * other.payload[3];
    const abacus_ulong hihi = (abacus_ulong)payload[2] * other.payload[2];

    // Shift the short multiplications to the correct place
    const ulonger lolo4 = ulonger(0, 0, lolo >> 32, lolo & 0xFFFFFFFF);
    const ulonger lohi4 = ulonger(0, lohi >> 32, lohi & 0xFFFFFFFF, 0);
    const ulonger hilo4 = ulonger(0, hilo >> 32, hilo & 0xFFFFFFFF, 0);
    const ulonger hihi4 = ulonger(hihi >> 32, hihi & 0xFFFFFFFF, 0, 0);

    // Finally, add all the partial results together
    const ulonger result = lolo4 + lohi4 + hilo4 + hihi4;

    return result;
  }

  ulonger operator+(const ulonger &other) const {
    const abacus_ulong4 thisUp =
        abacus::detail::cast::convert<abacus_ulong4>(payload);
    const abacus_ulong4 otherUp =
        abacus::detail::cast::convert<abacus_ulong4>(other.payload);
    const abacus_ulong4 add = thisUp + otherUp;

    abacus_ulong4 i0;
    i0[0] = add[1] >> 32;
    i0[1] = add[2] >> 32;
    i0[2] = add[3] >> 32;
    i0[3] = 0;
    const abacus_ulong4 r0 = (add & 0xFFFFFFFF) + i0;

    abacus_ulong4 i1;
    i1[0] = r0[1] >> 32;
    i1[1] = r0[2] >> 32;
    i1[2] = r0[3] >> 32;
    i1[3] = 0;
    const abacus_ulong4 r1 = (r0 & 0xFFFFFFFF) + i1;

    abacus_ulong4 i2;
    i2[0] = r1[1] >> 32;
    i2[1] = r1[2] >> 32;
    i2[2] = r1[3] >> 32;
    i2[3] = 0;
    const abacus_ulong4 r2 = (r1 & 0xFFFFFFFF) + i2;

    abacus_ulong4 i3;
    i3[0] = r2[1] >> 32;
    i3[1] = r2[2] >> 32;
    i3[2] = r2[3] >> 32;
    i3[3] = 0;
    const abacus_ulong4 r3 = (r2 & 0xFFFFFFFF) + i3;

    ulonger result;
    result.payload = abacus::detail::cast::convert<abacus_uint4>(r3);

    return result;
  }

  ulonger operator-(const ulonger &other) const {
    const abacus_long4 thisUp =
        abacus::detail::cast::convert<abacus_long4>(payload);
    const abacus_long4 otherUp =
        abacus::detail::cast::convert<abacus_long4>(other.payload);
    const abacus_long4 sub = thisUp - otherUp;

    abacus_ulong4 payload = 0;

    // This works by doing simple carry subtraction. If any of the individual
    // payloads has underflowed, we want to subtract 1 from the next (more
    // significant) payload. In practice, the 32 MSBs in each individual
    // payload that has underflowed will be 1s, since we are only subtracting
    // two numbers at a time, but we don't need to check it that precisely.
    // Checking if any of the 32 MSBs are 1 is enough.
    payload[3] = sub[3];
    payload[2] = sub[2] - ((payload[3] >> 32) ? 1 : 0);
    payload[1] = sub[1] - ((payload[2] >> 32) ? 1 : 0);
    payload[0] = sub[0] - ((payload[1] >> 32) ? 1 : 0);

    // Reset all the underflow bits
    payload &= 0xFFFFFFFF;

    ulonger result;
    result.payload = abacus::detail::cast::convert<abacus_uint4>(payload);
    return result;
  }

  bool operator<(const ulonger &other) const {
    if (payload[0] < other.payload[0]) {
      return true;
    } else if (payload[0] > other.payload[0]) {
      return false;
    }

    if (payload[1] < other.payload[1]) {
      return true;
    } else if (payload[1] > other.payload[1]) {
      return false;
    }

    if (payload[2] < other.payload[2]) {
      return true;
    } else if (payload[2] > other.payload[2]) {
      return false;
    }

    if (payload[3] < other.payload[3]) {
      return true;
    }

    return false;
  }

  operator abacus_ulong() const {
    return (abacus::detail::cast::convert<abacus_ulong>(payload[2]) << 32) |
           abacus::detail::cast::convert<abacus_ulong>(payload[3]);
  }
};

template <>
abacus_long mad_sat(const abacus_long x, const abacus_long y,
                    const abacus_long z) {
  const abacus_long mulSign = (x ^ y) & 0x8000000000000000;
  const abacus_long zSign = (z) & 0x8000000000000000;

  const ulonger bX(__abacus_abs(x));
  const ulonger bY(__abacus_abs(y));
  const ulonger bZ(__abacus_abs(z));

  const ulonger mul = bX * bY;

  ulonger result;
  bool resultSign = true;
  if (mulSign && zSign) {
    result = mul + bZ;
    resultSign = true;
  } else if (mulSign && !zSign) {
    result = (mul < bZ) ? bZ - mul : mul - bZ;
    resultSign = !(mul < bZ);
  } else if (!mulSign && zSign) {
    result = (mul < bZ) ? bZ - mul : mul - bZ;
    resultSign = (mul < bZ);
  } else {
    result = mul + bZ;
    resultSign = false;
  }

  const ulonger max(resultSign ? 0x8000000000000000 : 0x7FFFFFFFFFFFFFFF);

  if (max < result) {
    result = max;
  }

  const abacus_long sR = result;

  return (resultSign) ? -sR : sR;
}

template <>
abacus_long2 mad_sat(const abacus_long2 x, const abacus_long2 y,
                     const abacus_long2 z) {
  abacus_long2 r;
  for (unsigned i = 0; i < 2; i++) {
    r[i] = mad_sat<>(x[i], y[i], z[i]);
  }
  return r;
}

template <>
abacus_long3 mad_sat(const abacus_long3 x, const abacus_long3 y,
                     const abacus_long3 z) {
  abacus_long3 r;
  for (unsigned i = 0; i < 3; i++) {
    r[i] = mad_sat<>(x[i], y[i], z[i]);
  }
  return r;
}

template <>
abacus_long4 mad_sat(const abacus_long4 x, const abacus_long4 y,
                     const abacus_long4 z) {
  abacus_long4 r;
  for (unsigned i = 0; i < 4; i++) {
    r[i] = mad_sat<>(x[i], y[i], z[i]);
  }
  return r;
}

template <>
abacus_long8 mad_sat(const abacus_long8 x, const abacus_long8 y,
                     const abacus_long8 z) {
  abacus_long8 r;
  for (unsigned i = 0; i < 8; i++) {
    r[i] = mad_sat<>(x[i], y[i], z[i]);
  }
  return r;
}

template <>
abacus_long16 mad_sat(const abacus_long16 x, const abacus_long16 y,
                      const abacus_long16 z) {
  abacus_long16 r;
  for (unsigned i = 0; i < 16; i++) {
    r[i] = mad_sat<>(x[i], y[i], z[i]);
  }
  return r;
}

template <>
abacus_ulong mad_sat(const abacus_ulong x, const abacus_ulong y,
                     const abacus_ulong z) {
  const abacus_ulong shift = 32u;
  const abacus_ulong mask = 0xffffffffu;

  const abacus_ulong xHi = x >> shift;
  const abacus_ulong xLo = x & mask;
  const abacus_ulong yHi = y >> shift;
  const abacus_ulong yLo = y & mask;
  const abacus_ulong zHi = z >> shift;
  const abacus_ulong zLo = z & mask;

  const abacus_ulong mulHiHi = xHi * yHi;
  const abacus_ulong mulHiLo = xHi * yLo;
  const abacus_ulong mulLoHi = xLo * yHi;
  const abacus_ulong mulLoLo = xLo * yLo;

  if (mulHiHi > 0 || mulHiLo > mask || mulLoHi > mask) {
    return TypeTraits<abacus_ulong>::max();
  }

  const abacus_ulong rLo = (mulLoLo & mask) + zLo;
  const abacus_ulong rHi = (mulHiLo & mask) + (mulLoHi & mask) +
                           (mulLoLo >> shift) + (rLo >> shift) + zHi;

  if (rHi > mask) {
    return TypeTraits<abacus_ulong>::max();
  }

  return (rLo & mask) | (rHi << shift);
}

template <>
abacus_ulong2 mad_sat(const abacus_ulong2 x, const abacus_ulong2 y,
                      const abacus_ulong2 z) {
  abacus_ulong2 r;
  for (unsigned i = 0; i < 2; i++) {
    r[i] = mad_sat<>(x[i], y[i], z[i]);
  }
  return r;
}

template <>
abacus_ulong3 mad_sat(const abacus_ulong3 x, const abacus_ulong3 y,
                      const abacus_ulong3 z) {
  abacus_ulong3 r;
  for (unsigned i = 0; i < 3; i++) {
    r[i] = mad_sat<>(x[i], y[i], z[i]);
  }
  return r;
}

template <>
abacus_ulong4 mad_sat(const abacus_ulong4 x, const abacus_ulong4 y,
                      const abacus_ulong4 z) {
  abacus_ulong4 r;
  for (unsigned i = 0; i < 4; i++) {
    r[i] = mad_sat<>(x[i], y[i], z[i]);
  }
  return r;
}

template <>
abacus_ulong8 mad_sat(const abacus_ulong8 x, const abacus_ulong8 y,
                      const abacus_ulong8 z) {
  abacus_ulong8 r;
  for (unsigned i = 0; i < 8; i++) {
    r[i] = mad_sat<>(x[i], y[i], z[i]);
  }
  return r;
}

template <>
abacus_ulong16 mad_sat(const abacus_ulong16 x, const abacus_ulong16 y,
                       const abacus_ulong16 z) {
  abacus_ulong16 r;
  for (unsigned i = 0; i < 16; i++) {
    r[i] = mad_sat<>(x[i], y[i], z[i]);
  }
  return r;
}
}  // namespace

#define DEF(TYPE)                                            \
  TYPE ABACUS_API __abacus_mad_sat(TYPE x, TYPE y, TYPE z) { \
    return mad_sat<>(x, y, z);                               \
  }

DEF(abacus_char)
DEF(abacus_char2)
DEF(abacus_char3)
DEF(abacus_char4)
DEF(abacus_char8)
DEF(abacus_char16)

DEF(abacus_uchar)
DEF(abacus_uchar2)
DEF(abacus_uchar3)
DEF(abacus_uchar4)
DEF(abacus_uchar8)
DEF(abacus_uchar16)

DEF(abacus_short)
DEF(abacus_short2)
DEF(abacus_short3)
DEF(abacus_short4)
DEF(abacus_short8)
DEF(abacus_short16)

DEF(abacus_ushort)
DEF(abacus_ushort2)
DEF(abacus_ushort3)
DEF(abacus_ushort4)
DEF(abacus_ushort8)
DEF(abacus_ushort16)

DEF(abacus_int)
DEF(abacus_int2)
DEF(abacus_int3)
DEF(abacus_int4)
DEF(abacus_int8)
DEF(abacus_int16)

DEF(abacus_uint)
DEF(abacus_uint2)
DEF(abacus_uint3)
DEF(abacus_uint4)
DEF(abacus_uint8)
DEF(abacus_uint16)

DEF(abacus_long)
DEF(abacus_long2)
DEF(abacus_long3)
DEF(abacus_long4)
DEF(abacus_long8)
DEF(abacus_long16)

DEF(abacus_ulong)
DEF(abacus_ulong2)
DEF(abacus_ulong3)
DEF(abacus_ulong4)
DEF(abacus_ulong8)
DEF(abacus_ulong16)
