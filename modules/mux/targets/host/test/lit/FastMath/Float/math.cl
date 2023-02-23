// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc -cl-options '-cl-fast-relaxed-math' %s -stage cl_snapshot_compilation_front_end | %filecheck %s

#define helper(NAME) \
  void kernel func_##NAME(global float16* a) { \
    *a = NAME(NAME(NAME(NAME(NAME(NAME(*a).hi).hi).xyz).xy).x); \
  }

helper(cos)
helper(exp)
helper(exp2)
helper(exp10)
helper(log)
helper(log2)
helper(log10)
helper(rsqrt)
helper(sin)
helper(sqrt)
helper(tan)

// CHECK: define {{(dso_local )?}}spir_kernel void @func_cos
// CHECK: call spir_func <16 x float> @_Z10native_cosDv16_f
// CHECK: call spir_func <8 x float> @_Z10native_cosDv8_f
// CHECK: call spir_func <4 x float> @_Z10native_cosDv4_f
// CHECK: call spir_func <3 x float> @_Z10native_cosDv3_f
// CHECK: call spir_func <2 x float> @_Z10native_cosDv2_f
// CHECK: call spir_func float @_Z10native_cosf

// CHECK: define {{(dso_local )?}}spir_kernel void @func_exp
// CHECK: call spir_func <16 x float> @_Z10native_expDv16_f
// CHECK: call spir_func <8 x float> @_Z10native_expDv8_f
// CHECK: call spir_func <4 x float> @_Z10native_expDv4_f
// CHECK: call spir_func <3 x float> @_Z10native_expDv3_f
// CHECK: call spir_func <2 x float> @_Z10native_expDv2_f
// CHECK: call spir_func float @_Z10native_expf

// CHECK: define {{(dso_local )?}}spir_kernel void @func_exp2
// CHECK: call spir_func <16 x float> @_Z11native_exp2Dv16_f
// CHECK: call spir_func <8 x float> @_Z11native_exp2Dv8_f
// CHECK: call spir_func <4 x float> @_Z11native_exp2Dv4_f
// CHECK: call spir_func <3 x float> @_Z11native_exp2Dv3_f
// CHECK: call spir_func <2 x float> @_Z11native_exp2Dv2_f
// CHECK: call spir_func float @_Z11native_exp2f

// CHECK: define {{(dso_local )?}}spir_kernel void @func_exp10
// CHECK: call spir_func <16 x float> @_Z12native_exp10Dv16_f
// CHECK: call spir_func <8 x float> @_Z12native_exp10Dv8_f
// CHECK: call spir_func <4 x float> @_Z12native_exp10Dv4_f
// CHECK: call spir_func <3 x float> @_Z12native_exp10Dv3_f
// CHECK: call spir_func <2 x float> @_Z12native_exp10Dv2_f
// CHECK: call spir_func float @_Z12native_exp10f

// CHECK: define {{(dso_local )?}}spir_kernel void @func_log
// CHECK: call spir_func <16 x float> @_Z10native_logDv16_f
// CHECK: call spir_func <8 x float> @_Z10native_logDv8_f
// CHECK: call spir_func <4 x float> @_Z10native_logDv4_f
// CHECK: call spir_func <3 x float> @_Z10native_logDv3_f
// CHECK: call spir_func <2 x float> @_Z10native_logDv2_f
// CHECK: call spir_func float @_Z10native_logf

// CHECK: define {{(dso_local )?}}spir_kernel void @func_log2
// CHECK: call spir_func <16 x float> @_Z11native_log2Dv16_f
// CHECK: call spir_func <8 x float> @_Z11native_log2Dv8_f
// CHECK: call spir_func <4 x float> @_Z11native_log2Dv4_f
// CHECK: call spir_func <3 x float> @_Z11native_log2Dv3_f
// CHECK: call spir_func <2 x float> @_Z11native_log2Dv2_f
// CHECK: call spir_func float @_Z11native_log2f

// CHECK: define {{(dso_local )?}}spir_kernel void @func_log10
// CHECK: call spir_func <16 x float> @_Z12native_log10Dv16_f
// CHECK: call spir_func <8 x float> @_Z12native_log10Dv8_f
// CHECK: call spir_func <4 x float> @_Z12native_log10Dv4_f
// CHECK: call spir_func <3 x float> @_Z12native_log10Dv3_f
// CHECK: call spir_func <2 x float> @_Z12native_log10Dv2_f
// CHECK: call spir_func float @_Z12native_log10f

// CHECK: define {{(dso_local )?}}spir_kernel void @func_rsqrt
// CHECK: call spir_func <16 x float> @_Z12native_rsqrtDv16_f
// CHECK: call spir_func <8 x float> @_Z12native_rsqrtDv8_f
// CHECK: call spir_func <4 x float> @_Z12native_rsqrtDv4_f
// CHECK: call spir_func <3 x float> @_Z12native_rsqrtDv3_f
// CHECK: call spir_func <2 x float> @_Z12native_rsqrtDv2_f
// CHECK: call spir_func float @_Z12native_rsqrtf

// CHECK: define {{(dso_local )?}}spir_kernel void @func_sin
// CHECK: call spir_func <16 x float> @_Z10native_sinDv16_f
// CHECK: call spir_func <8 x float> @_Z10native_sinDv8_f
// CHECK: call spir_func <4 x float> @_Z10native_sinDv4_f
// CHECK: call spir_func <3 x float> @_Z10native_sinDv3_f
// CHECK: call spir_func <2 x float> @_Z10native_sinDv2_f
// CHECK: call spir_func float @_Z10native_sinf

// CHECK: define {{(dso_local )?}}spir_kernel void @func_sqrt
// CHECK: call spir_func <16 x float> @_Z11native_sqrtDv16_f
// CHECK: call spir_func <8 x float> @_Z11native_sqrtDv8_f
// CHECK: call spir_func <4 x float> @_Z11native_sqrtDv4_f
// CHECK: call spir_func <3 x float> @_Z11native_sqrtDv3_f
// CHECK: call spir_func <2 x float> @_Z11native_sqrtDv2_f
// CHECK: call spir_func float @_Z11native_sqrtf

// CHECK: define {{(dso_local )?}}spir_kernel void @func_tan
// CHECK: call spir_func <16 x float> @_Z10native_tanDv16_f
// CHECK: call spir_func <8 x float> @_Z10native_tanDv8_f
// CHECK: call spir_func <4 x float> @_Z10native_tanDv4_f
// CHECK: call spir_func <3 x float> @_Z10native_tanDv3_f
// CHECK: call spir_func <2 x float> @_Z10native_tanDv2_f
// CHECK: call spir_func float @_Z10native_tanf
