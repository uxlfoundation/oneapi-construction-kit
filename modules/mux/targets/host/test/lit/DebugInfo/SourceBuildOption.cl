// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Test Brief:
// Tests non spec build option '-S <path>'. Option sets the debug info
// source location of the kernel to the specified file path. This file is
// created by the runtime if it does not already exist.

// RUN: %oclc -cl-options '-g -S %T%separatorgenerated.cl' %s -enqueue getsquare -stage %stage > %t
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

// Get CU file
// CHECK: !DICompileUnit(language: DW_LANG_C99, file: [[DI_FILE:![0-9]+]]

// Directory path will change depending on Windows/Linux.
// '\5C' is expected file path delimiter for windows, used in the LLVM tests.
// CHECK: [[DI_FILE]] = !DIFile(filename: "generated.cl", directory: "{{.*(\\5C|\/)}}lit{{(\\5C|\/)}}DebugInfo{{(\\5C|\/)}}Output")

// Assert kernel DI entry is linked to the file DI entry
// CHECK: DISubprogram(
// CHECK-SAME: file: [[DI_FILE]]
