// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Test Brief:
// Basic check for presence of DI entry describing the source file the kernel
// originated from. Since kernels are created from a C string passed into an
// API call the runtime has to create the source location. For the moment this
// is filename 'kernel.opencl', with the directory oclc is being run from.

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

// Get CU file
// CHECK: !DICompileUnit(language: DW_LANG_C99, file: [[DI_FILE:![0-9]+]]

// Directory path will change depending on Windows/Linux.
// CHECK: [[DI_FILE]] = !DIFile(filename: "kernel.opencl", directory: "{{.*}}DebugInfo")

// Assert kernel DI entry is linked to the file DI entry
// CHECK: DISubprogram(
// CHECK-SAME: file: [[DI_FILE:![0-9]+]]
