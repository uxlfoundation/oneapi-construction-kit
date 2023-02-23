// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Test Brief:
// Basic check for presence of DI entries describing source line locations
// of IR instructions.

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

// Assert we have DI entries for each of the source lines
// CHECK: {{[0-9]+}} = distinct !DIGlobalVariable(name: "count",
// CHECK-SAME: line: 10
// CHECK: {{[0-9]+}} = !DILocation(line: 11
// CHECK: {{[0-9]+}} = !DILocation(line: 12
// CHECK: {{[0-9]+}} = !DILocation(line: 13
// CHECK: {{[0-9]+}} = !DILocation(line: 14
// CHECK: {{[0-9]+}} = !DILocation(line: 16
// CHECK: {{[0-9]+}} = !DILocation(line: 17
