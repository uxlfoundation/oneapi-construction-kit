// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Test Brief:
// Basic check for presence of DI entries describing variable types.

// RUN: %oclc -cl-options '-g -cl-opt-disable' %s -enqueue getsquare -stage %stage > %t
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

// Assert we have derived pointer type from basic int type
// CHECK: [[DI_BASE:![0-9]+]] = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
// CHECK: !{{[0-9]+}} = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: [[DI_BASE:![0-9]+]], size: {{32|64}})
