// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Test Brief:
// Basic check for presence of DI entry describing program compile unit.

// RUN: %oclc -cl-options '-g -cl-opt-disable' %s -stage %stage -enqueue getsquare > %t
// RUN: %filecheck < %t %s

static __constant int count = 1;
kernel void getsquare(global int *in,
                      global int *out) {
  int g_id = get_global_id(0);
  if (g_id < count)
  {
    out[g_id] = in[g_id] * in[g_id];
  }
}

// Assert we have a compile unit
// CHECK: [[DI_CU:![0-9]+]] = distinct !DICompileUnit(language: DW_LANG_C99

// Assert CU has a source file
// CHECK-SAME: file: !{{[0-9]+}}

// Assert CU contains globals
// CHECK-SAME: globals: !{{[0-9]+}}

// Make sure function points to correct CU
// CHECK: !DISubprogram(name: "getsquare"
// CHECK-SAME:, unit: [[DI_CU]],
