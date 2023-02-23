// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc -cl-options '-cl-fast-relaxed-math' %s -stage cl_snapshot_compilation_front_end | %filecheck %s

void kernel func_normalize_float(global float4* a) {
  *a = normalize(normalize(normalize(normalize(*a).xyz).xy).x);
}

void kernel func_distance_float(global float4* a, global float4* b) {
  (*a).x = distance(*a, *b);
  (*a).y = distance((*a).xyz, (*b).xyz);
  (*a).z = distance((*a).xy, (*b).xy);
  (*a).w = distance((*a).x, (*b).x);
}

void kernel func_length_float(global float4* a) {
  (*a).x = length(*a);
  (*a).y = length((*a).xyz);
  (*a).z = length((*a).xy);
  (*a).w = length((*a).x);
}

// CHECK: define {{(dso_local )?}}spir_kernel void @func_normalize_float
// CHECK: call spir_func <4 x float> @_Z14fast_normalizeDv4_f
// CHECK: call spir_func <3 x float> @_Z14fast_normalizeDv3_f
// CHECK: call spir_func <2 x float> @_Z14fast_normalizeDv2_f
// CHECK: call spir_func float @_Z14fast_normalizef

// CHECK: define {{(dso_local )?}}spir_kernel void @func_distance_float
// CHECK: call spir_func float @_Z13fast_distanceDv4_f
// CHECK: call spir_func float @_Z13fast_distanceDv3_f
// CHECK: call spir_func float @_Z13fast_distanceDv2_f
// CHECK: call spir_func float @_Z13fast_distancef

// CHECK: define {{(dso_local )?}}spir_kernel void @func_length_float
// CHECK: call spir_func float @_Z11fast_lengthDv4_f
// CHECK: call spir_func float @_Z11fast_lengthDv3_f
// CHECK: call spir_func float @_Z11fast_lengthDv2_f
// CHECK: call spir_func float @_Z11fast_lengthf
