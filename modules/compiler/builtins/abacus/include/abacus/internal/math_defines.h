// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef __ABACUS_INTERNAL_MATH_DEFINES_H__
#define __ABACUS_INTERNAL_MATH_DEFINES_H__

#define F_EXP_BIAS 127
#define F_MAX_EXP 255
#define F_MANT_SIZE 23
#define F_SIZE 32
#define F_SIGN_MASK 0x80000000
#define F_NO_SIGN_MASK 0x7FFFFFFF
#define F_MANT_MASK 0x007FFFFF
#define F_NO_MANT_MASK 0xFF800000
#define F_MSB_MANTISSA 0x00400000
#define F_EXP_MASK 0x7F800000
#define F_NO_EXP_MASK 0x807FFFFF
#define F_NORM_EXP 0x3F000000
#define F_HIDDEN_BIT 0x00800000

#define I_GET_EXPONENT(x) (x & F_EXP_MASK)
#define I_IS_DENORM(x) \
  (((x & F_EXP_MASK) == 0x00000000) && ((x & F_MANT_MASK) != 0x00000000))

// TODO F_IS_DENORM should be able to use ^ to simplify calculation.
#define F_IS_DENORM(x) (I_IS_DENORM(as_uint(x)))
#define I_IS_ZERO(x_i) ((x_i & F_NO_SIGN_MASK) == 0)
#define I_IS_INF_OR_NAN(x_i) ((x_i & F_EXP_MASK) == F_EXP_MASK)

#define I_IS_EVEN(x) (x % 2 == 0)
#define I_IS_ODD(x) (x % 2)
#define F_HAS_FRACT_PART(x) \
  (((as_uint(x) & F_EXP_MASK) >> F_MANT_SIZE) < (F_EXP_BIAS + F_MANT_SIZE))
#define I_4IPI_UINT 0xA2F983U
#define I_GET_UNBIASED_EXPONENT(x)         \
  (((x & F_NO_SIGN_MASK) >> F_MANT_SIZE) - \
   F_EXP_BIAS);  // TODO couldn't we use _unsafe_ilogb? update vecmath.
#define I_GET_MANT(x) ((x & F_MANT_MASK) | F_HIDDEN_BIT)

#endif  //__ABACUS_INTERNAL_MATH_DEFINES_H__
