// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc -cl-options '-cl-fast-relaxed-math' %s -stage cl_snapshot_compilation_front_end | %filecheck %s

void kernel func_normalize_double(global double4* a) {
  *a = normalize(normalize(normalize(normalize(*a).xyz).xy).x);
}

void kernel func_distance_double(global double4* a, global double4* b) {
  (*a).x = distance(*a, *b);
  (*a).y = distance((*a).xyz, (*b).xyz);
  (*a).z = distance((*a).xy, (*b).xy);
  (*a).w = distance((*a).x, (*b).x);
}

void kernel func_length_double(global double4* a) {
  (*a).x = length(*a);
  (*a).y = length((*a).xyz);
  (*a).z = length((*a).xy);
  (*a).w = length((*a).x);
}

// CHECK: define {{(dso_local )?}}spir_kernel void @func_normalize_double
// CHECK: call spir_func <4 x double> @_Z14fast_normalizeDv4_d
// CHECK: call spir_func <3 x double> @_Z14fast_normalizeDv3_d
// CHECK: call spir_func <2 x double> @_Z14fast_normalizeDv2_d
// CHECK: call spir_func double @_Z14fast_normalized

// CHECK: define {{(dso_local )?}}spir_kernel void @func_distance_double
// CHECK: call spir_func double @_Z13fast_distanceDv4_d
// CHECK: call spir_func double @_Z13fast_distanceDv3_d
// CHECK: call spir_func double @_Z13fast_distanceDv2_d
// CHECK: call spir_func double @_Z13fast_distanced

// CHECK: define {{(dso_local )?}}spir_kernel void @func_length_double
// CHECK: call spir_func double @_Z11fast_lengthDv4_d
// CHECK: call spir_func double @_Z11fast_lengthDv3_d
// CHECK: call spir_func double @_Z11fast_lengthDv2_d
// CHECK: call spir_func double @_Z11fast_lengthd
