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

#ifndef __ABACUS_INTERNAL_PAYNE_HANEK_H__
#define __ABACUS_INTERNAL_PAYNE_HANEK_H__

#include <abacus/abacus_cast.h>
#include <abacus/abacus_config.h>
#include <abacus/abacus_detail_cast.h>
#include <abacus/abacus_detail_common.h>
#include <abacus/abacus_integer.h>
#include <abacus/abacus_math.h>
#include <abacus/abacus_memory.h>
#include <abacus/abacus_misc.h>
#include <abacus/internal/floor_unsafe.h>
#include <abacus/internal/logb_unsafe.h>
#include <abacus/internal/math_defines.h>

/*
Payne-Hanek

Used in implementing sin-cos-tan
Returns x modulo pi/4, while also returning at least the last 3 bits of the
quotient.
If the quotient is odd, it returns pi/4 - (x modulo pi/4) instead.
The values it returns are the closest possible, without intermediate rounding
errors.


Derivation and motivation:
This is a range reduction algorithm for sin, cos and tan.

A handy reference for what follows:
https://en.wikipedia.org/wiki/Unit_circle

As per the unit circle, from which a lot of this algorithm gets it's
inspiration, sin, cos and tan are periodic, with a shared period of 2*pi.

When we want to calculate sin(x), cos(x), or tan(x), one of the first things
we want to do is reduce x to a suitable range, and then use a polynomial to
calculate the final answer.

Because of this periodic factor, given an input x, what we want to do is to
calculate

  x (mod 2*pi),

and from this calculate our sin(x), cos(x), or tan(x)

This comment is about the pitfalls of this and how to avoid them.


Naively, we could do this with a xReduced = fmod(x, 2*pi), however for large
values of x, the difference between the mathematical value of 2*pi and the
closest floating point value of 2*pi comes into effect in a large way, resulting
in totally meaningless answer for xReduced.

(As we'll see in a bit, even accurately reducing by 2*pi doesn't gives us enough
precision, and we end up using a factor 8 times smaller, pi/4, but for now we'll
work with 2*pi to see why it fails)

So we need to reduce by 2*pi. How can we do this?

Well nearly by definition, we can instead divide by 2*pi, chop off the integer
part of the result, then multiply back by 2*pi:

x (mod 2*pi) = 2*pi*frac( x / (2*pi) )

This is the essence of Payne-Hanek.

We shall use a worked example to illustrate the algorithm in motion.:
For this example, we'll try to find the value of

  cos(532.5)

in half precision, via payne-hanek range reduction.

For the first step, we divide by 2*pi. Because it's a constant, instead of
dividing by 2*pi we can change this into a multiply by 1/(2*pi)

Assuming for the moment we can multiply by 1/(2*pi) with as much accuracy as we
want, we have

  532.5/(2*pi) = 84.75000719643426629693060399586389778334976135

For the next step, chopping of the int part and multiplying by 2*pi gives us

  2*pi*0.75000719643426629693060399586389778334976135 =
  4.7124341969147359382759116

Storing this in the closest possible half gives us

  xReduced = 4.7109375

with

  cos(4.7109375) = -0.0014514923095703125

to the closest half.

The problem is that in reality:

  cos(532.5) = 0.0000452101230621337890625,

which is 2 orders of magnitude off our answer above. The only time we
intermediately rounded our answer to half precision floating point was when we
stored the already reduced value of 4.7109375.

So even though we're using the exactly correct reduced value of

  532.5 (modulo2*pi),

this stored value of 4.7109375 isn't good enough.

The issue becomes clear when we look at the unit circle,
an angle of 4.7109375 basically points straight down, and is very nearly equal
to 3*pi/2.

So when we get the cos of this, we get an answer that's very close to 0, orders
of magnitude smaller than the input.

So any error about x = 4.7109375 results in huge relative error in cos(x), as
the answer is so close to 0. Our small absolute error gets blown up into a
large relative error. (this is why 532.5 was chosen for this example)

So instead of just moduling by 2*pi, we need to take cases like this into
account. Since we want 532.5 to reduced to something close to 0, we use
(modulo pi/2) instead. This should have the effect of reducing 532.5 to
something much closer to 0.

(Basically we want the range reduction to work such that if sin(x),cos(x) or
tan(x) ~ 0, that payne-hanek reduces x to something ~ 0,
where floating point has more resolution.
sin(x), cos(x), and tan(x) are 0 at x = 0, pi/2, pi, 3*pi/2, so this is why we
pick pi/2 to reduce by, and any values that are close to a multiple of pi/2
should become close to 0)

Since doing this basically splits the unit circle in 4, for example all of

  0, pi/2, pi, 3*pi/2

now all reduced get to 0, where as the sin,cos, and tan of them will be
different, we need to save which quadrant of the unit circle the original value
was in.

So now we get 532.5 (modulo pi/2) instead. Following the same procedure:

  532.5/(pi/2) = 339.0000287857370651877224159834555911

(We can see after we chop off the integer bit there'll be a lot of 0's in the
mantissa, which was the cause of our woes before)

Chopping of the integer part (which we save as it tells us which quadrant x is
in), and multiplying by pi/2 gives us

  pi/2 * 0.0000287857370651877224159834555911 =

  0.00004521653004608058194653412426112858

Storing this in the closest possible half gives us xReduced =
  4.52101230621337890625e-5

Now, because we've reduced by pi/2 instead of 2*pi, the answer we seek is no
longer just cos(x). We need to know what quadrant this reduced angle falls into,
and adjust the answer accordingly.

Using the chopped off integer bits of 532.5/(pi/2), 339, tells us how many times
we've subtracted off pi/2 while modulo'ing 532.5

Since every 4 times we've subtracted off pi/2 is the same as subtracting off
2*pi, which is the same as doing a full loop around the unit circle, we only
need this 339 value mod 4, which is 3. Which tells us our input x was in the 3rd
quadrant before range reduction.

So now, we have that 532.5 is in the 3rd quadrant of the unit circle, and it's
4.52101230621337890625e-5 into this quadrant.

We now use some trig identities, in this case

  cos(3*pi/2 + x) = sin(x),

to finally get

  cos(532.5) = sin(4.52101230621337890625e-5) =
  0.0000452101230467325445041704899753825,

which is much closer to the correct answer of 0.0000452101230621337890625
(and indeed, is the correctly rounded result for half)


So, now we have our algorithm. Get x (modulo pi/2), taking note of which
quadrant it's in, (we have yet to get to how this is achieved),
and use the necessary trig identities get our answer out. All is well, at least
until we try to calculate something like this: cos(26.703125)

Doing our range reduction algorithm, we get

  xReduced = 1.570383771281654

in the 16th, or mod4, the 0th quadrant.

The closest representable half is 1.5703125, and

  cos(1.5703125) = 0.0004838267760202486938

(Don't need any trig identities in the first quadrant)

Meanwhile cos(26.703125) is actually

  0.0004125555015395612463420525,

which actually isn't too far off in absolute value, but in relative terms

4.838267760202486938e-4 vs
4.125555015395612463e-4,

is big.

What's happened here is that our range reduced value, 1.5703125, is crazily
close to pi/2, but isn't quite big enough to tick over the pi/2 mark and get
reduced more accurately. (This isn't the worst case scenario here, there are
other values that give much bigger relative errors)

To overcome these values, where cos(x) is still really close to 0, but x
modulo(pi/2) is really close to but not quite pi/2, we need something else.


To solve this, instead of calculating:

  xReduced ~ pi/2,

in the relevant cases we instead try to get:

  (pi/2 - xReduced) ~ 0,

and we can use the identity

  cos(pi/2 - x) = sin(x)

to return to the correct solution. Again we're using the extra resolution of
floating points around 0 to keep things accurate.

For the above example, this would result in

  xReduced = 4.12464141845703125e-4,
  sin(xReduced) = 4.12464141845703125e-4

which is indeed the correctly round result.

So when do we want to return [pi/2 - (x modulo pi/2)] instead of just
(x modulo pi/2)?

We want this when xReduced is close to pi/2, and to this end, we split this pi/2
interval in half, and when [xReduced > 0.5 * (pi/2)],
we return [pi/2 - (xmodulo pi/2)] instead.

This makes sure that and values of x such that sin(x), cos(x), or tan(x) ~ 0,
that xReduced is ~ 0 and we have no loss of precision.

Now, with this above scheme, by getting (x modulo pi/2) and splitting in half,
we're basically doing a (x modulo pi/4), and sometimes subtracting this off pi/4
if needs be.


This then is the final payne-hanek algorithm.

Get x modulo pi/4. This splits the unit circle into 8, so we need to remember
which octet it's in. As before with the quadrant, this is just the first 3 bits
of the integer part of x*(4/pi).

If the octet is odd, it means that we're in the part where we want

  (pi/4 - xReduced)

instead. We calculate this, and at the end multiply this result by pi/4 to get
either (x modulo pi/4) or [pi/4 - (x modulo pi/4)] depending on the octet.



For the implementation:

Glossed over before now was how to divide pi/4, without having to do it in crazy
precision.

To this end, rather than divide by pi/4 we instead multiply by 4/pi, the hex
digits of which we save in a big array.

We now move into fixed point maths.
Multiplying our x by 4/pi would give us an answer that looks something like in
binary:

    ...abcdefg.hijklmn...

For the integer bit of this, aka the octet, we don't care about 'abcd' here,
only the 3 bits 'efg' are needed. Every time you got past a multiple of 8 in
the integer part, it's the equivalent of going around the unit circle once, so
the answer is the same.
Similarly we don't care about the bits off the end after 'n' (or somewhere down
there), as it gives us too much precision.

We only pick out and multiply by the bits of 4/pi that are relevant to these

efg.hijklmn...

bits.

The bits of 4/pi we decide are important are intuitively related to the exponent
of our input x, as making x twice as big just moves the decimal point of
efg.hijklmn... up one.

So now we've picked out the relevant bits of 4/pi and multiplied them by the
mantissa of our input (with a big fixed point multiplication)

to get our efg.hijklmn... answer.

We strip off the integer part/octet 'efg' and save it, leaving us with

  0.hijklmn...

Now is the point we see if we want to return

  [xReduced]

or

  [pi/4 - xReduced].

To see which, if the octet is odd we want [pi/4 - xReduced].
Rather nicely, since our final xReduced is going to be

  (pi/4 * 0.hijklnm...),

we have:

    pi/4 - xReduced
  = pi/4 - (pi/4 * 0.hijklnm...)
  = pi/4 * (1 - 0.hijklmn...)

Which mean we only need to change

  (0.hijklnm...)
to
  (1 - 0.hijklmn...).

to get this change. But this is just flipping the bits! (The bits after the
decimal point).

Now that we've done this we can reconstruct this floating point number from this
fixed precision mantissa.
Sometimes there's quite a lot of 0's at the beginning of our

  0.hijklmn...

number, representing inputs that were really close to a multiple of pi/4. This
is fine, since we calculated a lot of extra bits. Indeed, this is why we have to
do such a big extended precision multiply by 4/pi in the first place, and why
payne-hanek exists at all.

So we shift the bits of

  0.hijklmn...

up until we get our leading 1 into a desirable place, and the amount we shift up
by is the exponent of our new reduced number.

And voila!

All we have to do now to get our full xReduced is to multiply by pi/4 to get

  (x mod pi/4)
or
  [pi/4 - (x mod pi/4)]


In reality, because this multiply loses a ulp of precision that we don't need to
lose, for the half implementation anyway we just incorporate this pi/4 factor
into our polynomial estimates, so instead of having a polynomial for sin(x) we
have a polynomial for sin(pi/4*x) instead.
*/

namespace {
#ifdef __CA_BUILTINS_HALF_SUPPORT
// High precision 4/pi, 48 base 2 digits represented as hex  96 + 127 = 223
ABACUS_CONSTANT static abacus_ushort payloadH[4] = {0x0000, 0xA2F9, 0x836E,
                                                    0x4E44};
#endif  // __CA_BUILTINS_HALF_SUPPORT

// High precision 4/pi, 256 base 2 digits represented as hex  96 + 127 = 223
ABACUS_CONSTANT static abacus_uint payload[8] = {
    0x000000A2, 0xF9836E4E, 0x441529FC, 0x2757D1F5,
    0x34DDC0DB, 0x6295993C, 0x439041FE, 0x5163ABDE};

// High precision 1/pi, 2000 base 2 digits represented as hex
ABACUS_CONSTANT static abacus_ulong payloadD[20] = {
    0x0000000000000000, 0xA2F9836E4E441529, 0xFC2757D1F534DDC0,
    0xDB6295993C439041, 0xFE5163ABDEBBC561, 0xB7246E3A424DD2E0,
    0x06492EEA09D1921C, 0xFE1DEB1CB129A73E, 0xE88235F52EBB4484,
    0xE99C7026B45F7E41, 0x3991D639835339F4, 0x9C845F8BBDF9283B,
    0x1FF897FFDE05980F, 0xEF2F118B5A0A6D1F, 0x6D367ECF27CB09B7,
    0x4F463F669E5FEA2D, 0x7527BAC7EBE5F17B, 0x3D0739F78A5292EA,
    0x6BFB5FB11F8D5D08, 0x56033046FC7B6BAB};

template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct payne_hanek_helper;

template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct payne_hanek_helper_half;

template <typename T>
inline typename TypeTraits<T>::UnsignedType extractMantissa(const T &x) {
  return I_GET_MANT(
      abacus::detail::cast::as<typename TypeTraits<T>::UnsignedType>(x));
}

template <typename T>
inline T tan_naive_reduction(T x, typename TypeTraits<T>::SignedType *octet) {
  //'Naive' range reduction:
  // Reduce x in the interval [-pi/4,pi/4] by doing modulo of Pi/2 from the
  // original value.
  // reduced_x = x - floor( x/ (pi/4) ) * pi/4
  typedef typename TypeTraits<T>::SignedType SignedType;

  // Cody & Waite pre-computed constants
  const T FOUR_OVER_PI = 1.27323949337005615234375f;
  const T PI_OVER_FOUR_C0 = 0.78515625f;
  const T PI_OVER_FOUR_C1 = 2.4187564849853515625e-4f;
  const T PI_OVER_FOUR_C2 = 3.77489497744594108e-8f;

  const T xAbs = __abacus_fabs(x);

  // Use cody waite subtraction for this too?
  T x4bypi = xAbs * FOUR_OVER_PI;

#ifdef __CODEPLAY_RTZ__

  uint ix = __abacus_as_uint(x);
  // Need to handle rounding on round-to-zero architectures
  // This code is only used when the multiply is round-to-zero

  abacus_uint rounding = I_GET_MANT(ix) * I_4IPI_UINT;

  abacus_uint rounding_test_bit = 1 << 22;

  // if the normalization caused a shift right, we need to test right 1 bit

  if (I_GET_EXPONENT(__abacus_as_uint(x4bypi)) != I_GET_EXPONENT(ix)) {
    rounding_test_bit = 1 << F_MANT_SIZE;
  }

  if (rounding & rounding_test_bit) {
    x4bypi = __abacus_as_float(__abacus_as_uint(x4bypi) + 1);
  }

#endif

  // This needs to be more accurate to deal with the accurate pi/4 of later
  const SignedType octet_local = abacus::internal::floor_unsafe(x4bypi);

  // Get the octet x is in (if x is negative we need to flip octet)
  *octet = __abacus_select(octet_local, ~octet_local, x < 0);

  // if the integral is odd we need to return (pi/4 - xx) as opposed to just xx:
  const T foctet =
      abacus::detail::cast::convert<T>(octet_local + (octet_local & 0x1));

  // Cody & Waite subtraction method.
  const T cw = ((xAbs - foctet * PI_OVER_FOUR_C0) - foctet * PI_OVER_FOUR_C1) -
               foctet * PI_OVER_FOUR_C2;

  return __abacus_fabs(cw);
}

template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct ph_middle_filter_extract;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct ph_middle_filter_extract<T, abacus_ushort> {
  static void _(const T &index, T &i0, T &i1, T &i2, T &i3) {
    using SignedType = typename TypeTraits<T>::SignedType;

    const T p0 = payloadH[0];
    const T p1 = payloadH[1];
    const T p2 = payloadH[2];
    const T p3 = payloadH[3];

    // We only index at either 0 or 1, as ph_middle_filter<abacus_half>
    // shifts away all but the most significant exponent bit.
    const SignedType cond = (index != 0);
    i0 = __abacus_select(p0, p1, cond);
    i1 = __abacus_select(p1, p2, cond);
    i2 = __abacus_select(p2, p3, cond);
    i3 = __abacus_select(p3, T(0), cond);
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <>
struct ph_middle_filter_extract<abacus_uint, abacus_uint> {
  static void _(const abacus_uint &index, abacus_uint &i0, abacus_uint &i1,
                abacus_uint &i2, abacus_uint &i3) {
    i0 = payload[index + 0];
    i1 = payload[index + 1];
    i2 = payload[index + 2];
    i3 = payload[index + 3];
  }
};

template <typename T>
struct ph_middle_filter_extract<T, abacus_uint> {
  static void _(const T &index, T &i0, T &i1, T &i2, T &i3) {
    using SignedType = typename TypeTraits<T>::SignedType;
    // Doing this optimally requires a bit of lateral thinking (literally). The
    // problem is that a vector shuffle with variable mask produces a lot of
    // Extract/InsertElement instructions, which totally destroys performance.
    //
    // Note that the highest index we expect to get here is 127 >> 5 == 3, so
    // the 8th payload value is really redundant. This also means we only need
    // two levels of selects to shift the correct values into the result in a
    // completely vector way.
    const T p0 = payload[0];
    const T p1 = payload[1];
    const T p2 = payload[2];
    const T p3 = payload[3];
    const T p4 = payload[4];
    const T p5 = payload[5];
    const T p6 = payload[6];

    const SignedType cond2 = (index & 2u) != 0;
    const T q0 = __abacus_select(p0, p2, cond2);
    const T q1 = __abacus_select(p1, p3, cond2);
    const T q2 = __abacus_select(p2, p4, cond2);
    const T q3 = __abacus_select(p3, p5, cond2);
    const T q4 = __abacus_select(p4, p6, cond2);

    const SignedType cond1 = (index & 1u) != 0;
    i0 = __abacus_select(q0, q1, cond1);
    i1 = __abacus_select(q1, q2, cond1);
    i2 = __abacus_select(q2, q3, cond1);
    i3 = __abacus_select(q3, q4, cond1);
  }
};

template <>
struct ph_middle_filter_extract<abacus_ulong, abacus_ulong> {
  static void _(const abacus_ulong &index, abacus_ulong &i0, abacus_ulong &i1,
                abacus_ulong &i2, abacus_ulong &i3) {
    const abacus_ulong clampedIndex =
        abacus::detail::common::min(index, (abacus_ulong)16);

    i0 = payloadD[clampedIndex + 0];
    i1 = payloadD[clampedIndex + 1];
    i2 = payloadD[clampedIndex + 2];
    i3 = payloadD[clampedIndex + 3];
  }
};

template <typename T>
struct ph_middle_filter_extract<T, abacus_ulong> {
  static void _(const T &index, T &i0, T &i1, T &i2, T &i3) {
    const T clampedIndex = abacus::detail::common::min(index, (abacus_ulong)16);

    for (unsigned i = 0; i < TypeTraits<T>::num_elements; i++) {
      const typename TypeTraits<T>::ElementType e = clampedIndex[i];

      i0[i] = payloadD[e + 0];
      i1[i] = payloadD[e + 1];
      i2[i] = payloadD[e + 2];
      i3[i] = payloadD[e + 3];
    }
  }
};

template <typename T>
inline void ph_middle_filter(const T e, T &hi, T &mi, T &lo) {
  typedef typename TypeTraits<T>::ElementType ElementType;

  const ElementType size = sizeof(ElementType) * 8;
  const ElementType shiftBy = (size == 32) ? 5 : 6;

  const T firstInt = e >> shiftBy;
  const T shiftInt = e & (size - 1);
  const T invShiftInt = shiftInt ^ (size - 1);

  T i0, i1, i2, i3;
  ph_middle_filter_extract<T>::_(firstInt, i0, i1, i2, i3);

  // Note that a shift of "size" is UB, so we offset it by 1 so that the shifts
  // are in the range 0...(size-1)
  const T p0Hi = (i0 << shiftInt);
  const T p0Lo = ((i1 >> 1) >> invShiftInt);
  const T p1Hi = (i1 << shiftInt);
  const T p1Lo = ((i2 >> 1) >> invShiftInt);
  const T p2Hi = (i2 << shiftInt);
  const T p2Lo = ((i3 >> 1) >> invShiftInt);

  hi = p0Hi | p0Lo;
  mi = p1Hi | p1Lo;
  lo = p2Hi | p2Lo;
}

#ifdef __CA_BUILTINS_HALF_SUPPORT
// Should overload the half variations where we need less bits
template <typename T>
inline void ph_middle_filter_half(const T e, T &hi, T &mi, T &lo) {
  using ElementType = typename TypeTraits<T>::ElementType;
  using SignedType = typename TypeTraits<T>::SignedType;

  const ElementType size = sizeof(ElementType) * 8;
  const ElementType shiftBy = 4;

  // 16-bit cl_half has 5 exponent bits, shifting by 4 only leaves the highest
  // exponent bit, making firstInt either 1 or 0.
  const T firstInt = e >> shiftBy;
  const T LShiftInt = e & (size - 1);
  const T RShiftInt = size - LShiftInt;

  T i0, i1, i2, i3;
  ph_middle_filter_extract<T>::_(firstInt, i0, i1, i2, i3);

  // Shifting away all the bits is undefined behaviour, catch this case and set
  // bits to zero.
  const SignedType ZeroShift = RShiftInt == size;

  hi = (i0 << LShiftInt);
  hi |= __abacus_select(T(i1 >> RShiftInt), T(0), ZeroShift);

  mi = (i1 << LShiftInt);
  mi |= __abacus_select(T(i2 >> RShiftInt), T(0), ZeroShift);

  lo = (i2 << LShiftInt);
  lo |= __abacus_select(T(i3 >> RShiftInt), T(0), ZeroShift);
}
#endif

// For float accuracy we only need to return 64 bits, the first 3 are the sign,
// in the worst case we get 28 zeros at the start of the mantissa, and then 24
// bits for the mantissa itself.
// We still need plenty of digits of pi though as the overflow from the lower
// bits have an effect. (this is where we use mul_hi)
template <typename T>
inline void ph_reduce(const T &hi, const T &mi, const T &lo, const T &i, T &ohi,
                      T &olo) {
  // We want to multiply our hi/mi/lo float by i, discarding the top and bottom
  // as they are inaccurate
  typedef typename TypeTraits<T>::SignedType SignedType;

  T a = mi * i;
  T b = __abacus_mul_hi(lo, i);

  olo = a + b;

  const SignedType cond = (TypeTraits<T>::max() - a) > b;
  const T overflow = __abacus_select((T)1, (T)0, cond);

  ohi = (hi * i) + __abacus_mul_hi(mi, i) + overflow;
}

// This function extracts from a 64bit number the 32 bits starting at bit
// 'firstBit'.
template <typename T>
inline T ph_extract_slice(const T &hi, const T &lo, const T &firstBit) {
  // for floating point representation fistBit is always < 32
  const T shift = (T)32u - firstBit;
  const T invShiftMask = ~((T)0xffffffffu << firstBit);

  const T rHi = (hi << firstBit);
  const T rLo = ((lo >> shift) & invShiftMask);

  return rHi | rLo;
}

template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct ph_extract_fract;

// from a 64bit number that is the output of  __Codeplay__mul96, this gets which
// octet it's in and it's reduced value, or if the octet is odd (pi/2 - reduced)
// in float format
template <typename T>
struct ph_extract_fract<T, abacus_float> {
  static T _(typename TypeTraits<T>::UnsignedType &hi,
             typename TypeTraits<T>::UnsignedType &lo,
             typename TypeTraits<T>::SignedType *octet) {
    typedef typename TypeTraits<T>::SignedType SignedType;
    typedef typename TypeTraits<T>::UnsignedType UnsignedType;

    const abacus_float PI_OVER_FOUR = 0.7853981635f;

    T fract_local;
    // The decimal point is 3 bits into s (Seamus)
    // In the worst case highest_significant_bit = 31, so you could get the
    // octet and the highest_significant_bit from the first int (just about!)

    // octet (the first 3 bits)
    const SignedType octet_local =
        abacus::detail::cast::convert<SignedType>(hi >> 29);

    // if the octet is an odd number, reduce it to the range 0 -> pi/8
    const SignedType cond = (octet_local & 0x1) == 1;
    hi = __abacus_select(hi, ~hi, cond);
    lo = __abacus_select(lo, ~lo, cond);

    // Get rid of the octet and get the highest significant bit
    hi = hi & 0x1FFFFFFF;
    const UnsignedType highest_significant_bit = __abacus_clz(hi) - 3;

    // Extract the mantissa from this 64 bit number
    const UnsignedType AnsMant =
        ph_extract_slice(hi, lo, highest_significant_bit);

    // Put the mantissa into float format
    const UnsignedType u = ((AnsMant >> 5) & F_MANT_MASK) | F_NORM_EXP;

    // Take into account the fact that our highest significant bit changes
    // the exponent (TODO check highest_significant_bit as I dislike the cast
    // here!)
    T fract = __abacus_ldexp(
        abacus::detail::cast::as<T>(u),
        -abacus::detail::cast::convert<SignedType>(highest_significant_bit));

    T fractByPiO4 = fract * PI_OVER_FOUR;

// If we are using a round to zero architecture:
#ifdef __CODEPLAY_RTZ__
    uint iFract = __abacus_as_uint(fract);
    // uint multiplication by fractional part of pi/4
    uint fract_Ipi4 = 0x00c90fdb;
    abacus_uint rounding = I_GET_MANT(iFract) * fract_Ipi4;
    abacus_uint rounding_test_bit = 1 << 22;

    // unbiased exponent of fractByPiO4  ==  exponet of fract   (exp of fract <
    // 1)
    if (I_GET_EXPONENT(__abacus_as_uint(fractByPiO4)) == I_GET_EXPONENT(iFract))
    // if the normalization caused a shift right, we need to test right 1 bit
    {
      rounding_test_bit = 1 << F_MANT_SIZE;
    }

    if (rounding & rounding_test_bit) {
      fractByPiO4 = __abacus_as_float(__abacus_as_uint(fractByPiO4) + 1);
    }
#endif

    fract_local = fractByPiO4;
    *octet = octet_local;

    return fract_local;
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
// from a 128 bit number that is the output of  __Codeplay__mul192, this gets
// which octet it's in and it's reduced value, or if the octet is odd (pi/4 -
// reduced) in double format
template <typename T>
struct ph_extract_fract<T, abacus_double> {
  typedef typename TypeTraits<T>::SignedType SignedType;
  typedef typename TypeTraits<T>::UnsignedType UnsignedType;
  typedef typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type
      IntVectorType;
  static T _(UnsignedType &hi, UnsignedType &lo, IntVectorType *octet) {
    // The decimal point is 3 bits into hi
    const UnsignedType octet_local = hi >> 61;

    // if the octet is an odd number, reduce it to the range 0 -> pi/8
    const SignedType cond = (octet_local & 0x1) == 0x1;
    hi = __abacus_select(hi, ~hi, cond);
    lo = __abacus_select(lo, ~lo, cond);

    // Get rid of the octet and get the highest significant bit
    hi = hi & 0x1FFFFFFFFFFFFFFF;

    const UnsignedType leading_zeros = __abacus_clz(hi);

    const UnsignedType AnsMant =
        (hi << (leading_zeros)) | (lo >> ((UnsignedType)64 - leading_zeros));

    // Put the mantissa into float format
    UnsignedType u =
        ((AnsMant >> 11) & 0x000FFFFFFFFFFFFF) | 0x3FE0000000000000;

    // if the bit rounded off was a 1, add one. While not an exact RTE, Makes
    // the max error drop off a bit.
    u += ((AnsMant >> 10) & 0x1);

    // Unsafe ldexp (Should be fine on CPU's) Not sure about FTZ devices:
    const UnsignedType ldexp_factor = ((UnsignedType)1026 - leading_zeros)
                                      << 52;
    T fract = abacus::detail::cast::as<T>(u) *
              abacus::detail::cast::as<T>(ldexp_factor);

    *octet = abacus::detail::cast::convert<IntVectorType>(octet_local);

    return fract;
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <>
struct payne_hanek_helper<abacus_float, abacus_float> {
  static abacus_float _(abacus_float x, abacus_int *out_octet) {
    typedef abacus_float T;
    typedef abacus_int SignedType;
    typedef abacus_uint UnsignedType;

    if (!__abacus_isfinite(x)) {
      return ABACUS_NAN;
    }

    SignedType xExp;
    const abacus_float mantissa = __abacus_frexp(x, &xExp);

    xExp--;

    const UnsignedType xMantissa = extractMantissa(mantissa);

    // Check to see if our number is small enough to use a normal mod(pi/4) on
    const SignedType exp_threshold = 1;  // MUST BE > 0!

    if (xExp < exp_threshold) {
      return tan_naive_reduction(x, out_octet);
    }

    // Otherwise use the payne-hanek algorithm:

    // Get the relevent 96 binary digits of 4/pi depending on the exponent
    UnsignedType filterHi, filterMi, filterLo;
    ph_middle_filter(abacus::detail::cast::convert<abacus_uint>(xExp - 1),
                     filterHi, filterMi, filterLo);
    // Multiply the 96 digits of 4/pi by our Mantisa:
    UnsignedType rHi, rLo;
    ph_reduce(filterHi, filterMi, filterLo, xMantissa, rHi, rLo);
    // Get the relevent integral and mantissa
    SignedType octet;
    const T fract = ph_extract_fract<T>::_(rHi, rLo, &octet);

    *out_octet = __abacus_select(octet, ~octet, x < 0);

    return fract;
  }
};

template <typename T>
struct payne_hanek_helper<T, abacus_float> {
  typedef typename TypeTraits<T>::SignedType SignedType;
  typedef typename TypeTraits<T>::UnsignedType UnsignedType;

  static T _(T x, SignedType *out_octet) {
    SignedType xExp;
    const T mantissa = __abacus_frexp(x, &xExp);

    xExp -= 1;

    const UnsignedType xMantissa = extractMantissa(mantissa);

    // Get the relevent 96 binary digits of 4/pi depending on the exponent
    UnsignedType filterHi, filterMi, filterLo;
    ph_middle_filter(abacus::detail::cast::convert<UnsignedType>(xExp - 1),
                     filterHi, filterMi, filterLo);
    // Multiply the 96 digits of 4/pi by our Mantisa:
    UnsignedType rHi, rLo;
    ph_reduce(filterHi, filterMi, filterLo, xMantissa, rHi, rLo);
    // Get the relevent integral and mantissa
    SignedType phoctet;
    const T phfract = ph_extract_fract<T>::_(rHi, rLo, &phoctet);
    phoctet = __abacus_select(phoctet, ~phoctet, x < 0);

    // Check to see if our number is small enough to use a normal mod(pi/4) on
    const SignedType exp_threshold = 1;  // MUST BE > 0!
    const SignedType cond = xExp < exp_threshold;

    SignedType tanoctet;
    const T tanfract = tan_naive_reduction(x, &tanoctet);

    T result = __abacus_select(phfract, tanfract, cond);

    result = __abacus_select(ABACUS_NAN, result, __abacus_isfinite(x));

    *out_octet = __abacus_select(phoctet, tanoctet, cond);
    return result;
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <>
struct payne_hanek_helper<abacus_double, abacus_double> {
  typedef abacus_double T;
  typedef MakeType<abacus_int, TypeTraits<T>::num_elements>::type IntVectorType;
  static T _(T x, IntVectorType *octet) {
    typedef TypeTraits<T>::UnsignedType UnsignedType;

    const T PI_8 = 0.392699081698724154807830422;
    const T FOUR_PI =
        1.273239544735162686151070106980114896275677165923651589981338;

    if (!__abacus_isfinite(x)) {
      return ABACUS_NAN;
    }

    if (__abacus_fabs(x) < PI_8) {
      *octet = (x < 0) ? 7 : 0;
      return __abacus_fabs(x * FOUR_PI);
    }

    IntVectorType xExp;
    const T mantissa = __abacus_frexp(x, &xExp);

    // Add in the hidden bit to the mantissa
    const UnsignedType xMantissa =
        ((abacus::detail::cast::as<UnsignedType>(mantissa) &
          0x000FFFFFFFFFFFFF) |
         0x0010000000000000);

    // Get the relevent 3*64 = 192 binary digits of 4/pi depending on the
    // exponent
    UnsignedType filterHi, filterMi, filterLo;
    ph_middle_filter(abacus::detail::cast::convert<abacus_ulong>(xExp + 9),
                     filterHi, filterMi, filterLo);

    // Multiply the digits of 4/pi by our mantissa and discard the unneeded
    // bits:
    UnsignedType rHi, rLo;

    ph_reduce(filterHi, filterMi, filterLo, xMantissa, rHi, rLo);

    // Extract the answer
    IntVectorType phoctet;
    const T fract = ph_extract_fract<abacus_double>::_(rHi, rLo, &phoctet);

    const IntVectorType octetCond =
        abacus::detail::cast::convert<IntVectorType>(x < 0);
    *octet = __abacus_select(phoctet, ~phoctet, octetCond);

    return fract;
  }
};

template <typename T>
struct payne_hanek_helper<T, abacus_double> {
  typedef typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type
      IntVectorType;
  static T _(T x, IntVectorType *octet) {
    typedef typename TypeTraits<T>::SignedType SignedType;
    typedef typename TypeTraits<T>::UnsignedType UnsignedType;

    const T PI_8 = 0.392699081698724154807830422;
    const T FOUR_PI =
        1.273239544735162686151070106980114896275677165923651589981338;

    IntVectorType xExp;
    const T mantissa = __abacus_frexp(x, &xExp);

    // Add in the hidden bit to the mantissa
    const UnsignedType xMantissa =
        ((abacus::detail::cast::as<UnsignedType>(mantissa) &
          0x000FFFFFFFFFFFFF) |
         0x0010000000000000);

    // Get the relevent 3*64 = 192 binary digits of 4/pi depending on the
    // exponent
    UnsignedType filterHi, filterMi, filterLo;
    ph_middle_filter(abacus::detail::cast::convert<UnsignedType>(xExp + 9),
                     filterHi, filterMi, filterLo);

    // Multiply the digits of 4/pi by our mantissa and discard the unneeded
    // bits:
    UnsignedType rHi, rLo;

    ph_reduce(filterHi, filterMi, filterLo, xMantissa, rHi, rLo);

    // Extract the answer
    IntVectorType phoctet;
    T result = ph_extract_fract<T>::_(rHi, rLo, &phoctet);

    const IntVectorType octetCond =
        abacus::detail::cast::convert<IntVectorType>(x < 0);
    phoctet = __abacus_select(phoctet, ~phoctet, octetCond);

    const SignedType cond1 = __abacus_fabs(x) < PI_8;
    result = __abacus_select(result, __abacus_fabs(x * FOUR_PI), cond1);

    const IntVectorType octetExtremes =
        __abacus_select((IntVectorType)0, (IntVectorType)7, octetCond);
    phoctet =
        __abacus_select(phoctet, octetExtremes,
                        abacus::detail::cast::convert<IntVectorType>(cond1));

    const SignedType cond2 = ~__abacus_isfinite(x);
    result = __abacus_select(result, ABACUS_NAN, cond2);

    *octet = phoctet;

    return result;
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

#ifdef __CA_BUILTINS_HALF_SUPPORT
// Payne-Hanek algorithm for half scalar values.
// This is it's own template as the function signature is slightly
// different, instead of int* being passed it's a short*
// In theory the 32 and 64 bit versions could also just be a short, you only
// need the last 3 bits of this value.
template <>
// Scalar Payne-Hanek for halfs
struct payne_hanek_helper_half<abacus_half, abacus_half> {
  using Shape = FPShape<abacus_half>;
  static abacus_half _(abacus_half x, abacus_short *octet) {
    // Returns x (modulo pi/4), or pi/4 - (x (modulo pi/4)) depending on the
    // range reduction needs

    const abacus_short exp_bias = Shape::Bias();
    const abacus_short mantissa_bits = Shape::Mantissa();

    const abacus_short mant_mask = Shape::MantissaMask();
    const abacus_short exp_lsb = Shape::LeastSignificantExponentBit();

    if (!__abacus_isfinite(x)) {
      return ABACUS_NAN_H;
    }

    *octet = 0;

    abacus_half xAbs = __abacus_fabs(x);

    abacus_ushort xAbsUshort = abacus::detail::cast::as<abacus_ushort>(xAbs);

    abacus_short xExp = (xAbsUshort >> mantissa_bits) - exp_bias;
    abacus_ushort xMantissa = (xAbsUshort & mant_mask) | exp_lsb;

    // Early return for values that don't need to be reduced. (otherwise it'll
    // try to access out of bounds in the array)
    if (xAbs < ABACUS_PI_4_H) {
      *octet = 0;
      const abacus_half FOUR_OVER_PI = 1.27323949337f16;
      return xAbs * FOUR_OVER_PI;
    }

    // Get the bits we want of payloadH[] based on array_offset, which is
    // used to calculate how far into payloadH[] we want to start reading bits
    // from. xExp is at least -1 after ABACUS_PI_4_H check above, so adding 1
    // ensures array_offset is always > 0
    const abacus_ushort array_offset = abacus_ushort(xExp + 1);

    abacus_ushort pi_inverse_1;
    abacus_ushort pi_inverse_2;
    abacus_ushort pi_inverse_3;

    // Get the relevant bits of 1/pi, using the exponent of x
    ph_middle_filter_half(array_offset, pi_inverse_1, pi_inverse_2,
                          pi_inverse_3);
    // We now have a load of hex bits of 4/pi, saved in pi_inverse_[123]
    // We want to multiply by xMantissa, and get the floor and fractional part
    // of the answer

    abacus_ushort m_pi1_lo =
        pi_inverse_1 * xMantissa;  // Don't need the high part of this
                                   // multiplication, these bits change the
                                   // octet by a multiple of 8

    abacus_ushort m_pi2_hi = __abacus_mul_hi(pi_inverse_2, xMantissa);
    abacus_ushort m_pi2_lo = pi_inverse_2 * xMantissa;

    abacus_ushort m_pi3_hi = __abacus_mul_hi(
        pi_inverse_3, xMantissa);  // mant_by_pi3 >> 16; //mul_hi (dont need the
                                   // low bit of this multiplication, overly
                                   // accurate

    // When adding these m_pi_123_hi_lo's overlap, so we need to add the
    // corresponding components:

    abacus_ushort m_pi_lo = m_pi3_hi + m_pi2_lo;

    abacus_ushort overflow =
        (TypeTraits<abacus_ushort>::max() - m_pi3_hi < m_pi2_lo) ? 1 : 0;

    abacus_ushort m_pi = m_pi2_hi + m_pi1_lo + overflow;

    // We now have the bits of mantissa*pi quite accurately (to 26bits most of
    // the time, these bits being stored in [m_pi][m_pi_lo]
    // We need more than 16 bits to deal with catastrophic cancellation, aka
    // when x mod pi/4 becomes really tiny, and the results are ~0

    // Now that we have a load of bits of mantissa * 4/pi, see which ones are
    // needed to get which octet our input angle is in, and xReduced (how far
    // into that octet it is):

    // We know where we can get our floor value from, the decimal point in m *
    // 4/pi is in position 10 in m_pi (unrelated to mantissa_bits = 10)
    abacus_short floor_val = (m_pi >> 10);

    // If the octet is odd we want (pi/4 - xReduced) instead, doing it here is
    // equivalent to negating all the bits
    if (floor_val & 0x1) {
      m_pi = ~m_pi;
      m_pi_lo = ~m_pi_lo;
    }

    // Chop off the floor bits to get the mantissa:
    abacus_ushort mant_mask_with_hidden_bit =
        TypeTraits<abacus_ushort>::max() >> 5;

    m_pi &= mant_mask_with_hidden_bit;

    // We now shift this 2*short mantissa up so the leading 1 becomes the hidden
    // bit in m_pi.
    // We keep track of how far we've shifted up and adjust the exponent
    // correspondingly at the end.

    // In the case of catastrophic cancellation, or even just the odd weird
    // value that's very close to pi/4, our mantissa will have a lot of 0's at
    // the start. Sometimes the whole of m_pi is 0, we can save doing
    // a few individual shifts by just bumping m_pi_lo into mul_pi in one go,
    // making sure not to set past the leading bit. The most we can shift up by
    // to do this is 11.

    // The smallest mantissa for half value occurs for x = 177.5, with the
    // mantissa as 0x[m_pi][m_pi_lo] = 0x[0][507]
    abacus_short m_pi_lo_shift_amount = 0;
    if (m_pi == 0) {
      m_pi = m_pi_lo >> (Shape::NumBits() - 11);
      m_pi_lo <<= 11;
      m_pi_lo_shift_amount = 11;
    }

    // Now we count how many bits we need to shift up in m_pi:
    abacus_short m_pi_shift_amount = __abacus_clz(m_pi) - 5;

    // Shift our 0x[m_pi][m_lo] mantissa up by "m_pi_shift_amount":
    m_pi = (m_pi << m_pi_shift_amount) |
           (m_pi_lo >> (Shape::NumBits() - m_pi_shift_amount));
    m_pi_lo <<= m_pi_shift_amount;

    // Now the leading bit in m_pi is in the correct place for us to immediately
    // just cast mi_p to a half and have the mantissa in the correct place
    // Adjust the final exponent to reflect this shifting up:
    abacus_short exponent = -(m_pi_lo_shift_amount + m_pi_shift_amount);

    // We now have the exponent and the mantissa of our answer.
    // Construct the answer:
    abacus_ushort ansUshort =
        ((exponent + exp_bias) << mantissa_bits) | (m_pi & mant_mask);

    // See if we should round up by one for more accuracy by seeing if the first
    // bit off the end of the mantissa is a 1 or not:
    ansUshort += (m_pi_lo >= Shape::SignMask()) ? 1 : 0;

    // if x is +- 532.500000  then exponent is -15 aka denormal.
    // if x is +- 177.500000  then exponent is -16 aka denormal.
    if (exponent <= -exp_bias) {
      // Note, we don't need to check to do an extra check for rounding here,
      // these values work out
      ansUshort = m_pi >> (-exp_bias - exponent + 1);
    }

    abacus_half ans = abacus::detail::cast::as<abacus_half>(ansUshort);

    *octet = floor_val;
    return ans;
  }
};

// Vectorized half payne_hanek
template <typename T>
struct payne_hanek_helper_half<T, abacus_half> {
  using SignedType = typename TypeTraits<T>::SignedType;
  using UnsignedType = typename TypeTraits<T>::UnsignedType;
  using Shape = FPShape<T>;
  static T _(T x, SignedType *octet) {
    // Returns x (modulo pi/4), or pi/4 - (x (modulo pi/4)) depending on
    // range reduction needs

    const SignedType exp_bias = Shape::Bias();
    const SignedType mantissa_bits = Shape::Mantissa();

    const SignedType mant_mask = Shape::MantissaMask();
    const SignedType exp_lsb = Shape::LeastSignificantExponentBit();

    *octet = 0;

    // Get the relevant bits of 1/pi, using the exponent of x
    T xAbs = __abacus_fabs(x);

    UnsignedType xAbsUshort = abacus::detail::cast::as<UnsignedType>(xAbs);

    SignedType xExp =
        abacus::detail::cast::as<SignedType>(xAbsUshort >> mantissa_bits) -
        exp_bias;
    UnsignedType xMantissa = (xAbsUshort & mant_mask) | exp_lsb;

    // Get the bits we want of payloadH[] based on array_offset, which is
    // used to calculate how far into payloadH[] we want to start reading bits
    // from. We only use the result of ph_middle_filter_half() in the case that
    // xExp is at least -1, so adding 1 ensures array_offset is always > 0.
    //
    // Inputs with xExp < -1 are caught in a late abacus_select check against
    // 'xAbs < ABACUS_PI_4_H' which use a different algorithm for the return
    // value.
    const UnsignedType array_offset = UnsignedType(xExp + 1);

    UnsignedType pi_inverse_1;
    UnsignedType pi_inverse_2;
    UnsignedType pi_inverse_3;

    ph_middle_filter_half(array_offset, pi_inverse_1, pi_inverse_2,
                          pi_inverse_3);

    // We now have a load of hex bits of 4/pi.
    // We want to multiply by xMantissa, and get the floor and fractional part
    // of the answer

    UnsignedType m_pi1_lo =
        pi_inverse_1 * xMantissa;  // Don't need the high part of this
                                   // multiplication, these bits change the
                                   // octet by a multiple of 8

    UnsignedType m_pi2_hi = __abacus_mul_hi(pi_inverse_2, xMantissa);
    UnsignedType m_pi2_lo = pi_inverse_2 * xMantissa;

    UnsignedType m_pi3_hi = __abacus_mul_hi(
        pi_inverse_3, xMantissa);  // mant_by_pi3 >> 16; //mul_hi (dont need the
                                   // low bit of this multiplication, overly
                                   // accurate

    // When adding these m_pi_[123]_hi_lo's overlap, so we need to add the
    // corresponding components:
    UnsignedType m_pi_lo = m_pi3_hi + m_pi2_lo;

    // UnsignedType overflow = (0xFFFF - m_pi3_hi < m_pi2_lo) ? 1 : 0;
    UnsignedType overflow = __abacus_select(
        UnsignedType(0), UnsignedType(1),
        SignedType(TypeTraits<UnsignedType>::max() - m_pi3_hi < m_pi2_lo));

    UnsignedType m_pi = m_pi2_hi + m_pi1_lo + overflow;

    // We now know the bits of mantissa*pi quite accurately (to 26bits most of
    // the time, these bits being stored in [m_pi][m_pi_lo]
    // We need more than 16 bits to deal with catastrophic cancellation, aka
    // when x mod pi/4 becomes really tiny, and the results are ~0

    // Now we have a load of bits of mantissa * 4/pi, see which ones are needed
    // to get the octet and xReduced:

    // We know where we can get our floor value from, the decimal point in m *
    // 4/pi is in position 10 in m_pi
    SignedType floor_val = SignedType(m_pi >> 10);

    // If the octet is odd we want pi/4 - xReduced instead, doing it here is
    // equivalent to negating all the bits
    m_pi = __abacus_select(m_pi, ~m_pi, SignedType((floor_val & 0x1) != 0));
    m_pi_lo =
        __abacus_select(m_pi_lo, ~m_pi_lo, SignedType((floor_val & 0x1) != 0));

    // Chop off the floor bits to get the mantissa:
    UnsignedType mant_mask_with_hidden_bit =
        (TypeTraits<UnsignedType>::max() >> 5);

    m_pi &= mant_mask_with_hidden_bit;

    // We now shift this 2*short mantissa up so the leading 1 becomes the hidden
    // bit in m_pi
    // We keep track of how far we've shifted up and adjust the exponent
    // correspondingly at the end.

    // In the case of catastrophic cancellation, or even just the odd weird
    // value that's very close to pi/4, our mantissa will have a lot of 0's at
    // the start. Sometimes the whole of m_pi is 0, we can save doing
    // a few individual shifts by just bumping m_pi_lo into mul_pi in one go,
    // making sure not to set past the leading bit. The most we can shift up by
    // to do this is 11.

    // The smallest mantissa for and half value occurs for x = 177.5, with the
    // mantissa as 0x[m_pi][m_lo] = 0x[0][507]

    SignedType m_pi_lo_shift_amount = 0;

    SignedType Cond1 = (m_pi == 0);

    m_pi_lo_shift_amount =
        __abacus_select(SignedType(0), SignedType(11), Cond1);
    m_pi = __abacus_select(m_pi, m_pi_lo >> 5, Cond1);
    m_pi_lo = __abacus_select(m_pi_lo, m_pi_lo << 11, Cond1);

    // Count how many bits we need to shift up in m_pi:
    // m_pi is never 0 here, and indeed shouldn't be, so you can remove the
    // (m_pi != 0) check. The thought of a potential infinite loop in some
    // future template is the reason it's still checked for here:
    // TODO builtin for getting the highest set bit.

    SignedType m_pi_shift_amount = 0;
    while (any((m_pi != 0) && ((m_pi << m_pi_shift_amount) < exp_lsb))) {
      m_pi_shift_amount +=
          __abacus_select(SignedType(0), SignedType(1),
                          SignedType((m_pi << m_pi_shift_amount) < exp_lsb));
    }

    // Shift our 0x[m_pi][m_lo] mantissa up by "m_pi_shift_amount":
    m_pi = (m_pi << m_pi_shift_amount) |
           (m_pi_lo >> (Shape::NumBits() - m_pi_shift_amount));
    m_pi_lo <<= m_pi_shift_amount;

    // Now the leading bit in m_pi is in the correct place for us to immediately
    // just cast mi_p to a half and have the mantissa in the correct place
    // Adjust the final exponent to reflect this shifting up:
    SignedType exponent = -(m_pi_lo_shift_amount + m_pi_shift_amount);

    // We now have the exponent and the mantissa of our answer.

    // construct the answer:
    UnsignedType ansUshort =
        UnsignedType((exponent + exp_bias) << mantissa_bits) |
        (m_pi & mant_mask);

    // See if we should round up by one for more accuracy by seeing if the first
    // bit off the mantissa is a 1 or not:
    // ansUshort += (m_pi_lo >= 0x8000) ? 1 : 0;
    ansUshort += __abacus_select(UnsignedType(0), UnsignedType(1),
                                 SignedType(m_pi_lo >= Shape::SignMask()));

    // if x is +- 532.500000  then exponent is -15 aka denormal.
    // if x is +- 177.500000  then exponent is -16 aka denormal.
    ansUshort = __abacus_select(ansUshort, m_pi >> (-exp_bias - exponent + 1),
                                exponent <= -exp_bias);

    T ans = abacus::detail::cast::as<T>(ansUshort);

    floor_val = __abacus_select(floor_val, SignedType(0),
                                SignedType(xAbs < ABACUS_PI_4_H));

    const T FOUR_OVER_PI = 1.27323949337f16;

    ans = __abacus_select(ans, xAbs * FOUR_OVER_PI,
                          SignedType(xAbs < ABACUS_PI_4_H));

    ans = __abacus_select(ans, T(ABACUS_NAN_H), ~__abacus_isfinite(x));

    *octet = floor_val;

    return ans;
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT
}  // namespace

namespace abacus {
namespace internal {
template <typename T>
inline T payne_hanek(
    T x, typename MakeType<abacus_int, TypeTraits<T>::num_elements>::type
             *out_octet) {
  return payne_hanek_helper<T>::_(x, out_octet);
}

template <typename T>
inline T payne_hanek_half(T x, typename TypeTraits<T>::SignedType *out_octet) {
  return payne_hanek_helper_half<T>::_(x, out_octet);
}

// payne_hanek has a codegen bug on OpenCL for float3 types. Hack around it by
// casting the vec3 to a vec4 for the operation (see Redmine #8082).
#ifdef __OPENCL_VERSION__
inline abacus_float3 payne_hanek(abacus_float3 x, abacus_int3 *out_octet) {
  abacus_int4 o;
  const abacus_float4 r =
      payne_hanek_helper<abacus_float4>::_(__abacus_as_float4(x), &o);
  *out_octet = o.xyz;
  return r.xyz;
}
#endif
}  // namespace internal
}  // namespace abacus

#endif  //__ABACUS_INTERNAL_PAYNE_HANEK_H__
