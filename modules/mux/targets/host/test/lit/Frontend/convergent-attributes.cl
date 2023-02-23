// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// During investigation of CA-3691 it was discovered that clang sets the
// `convergent` attribute on all functions when the source language is OpenCL.
// This is overly pessimistic. However, certain workgroup functions e.g.
// barrier() *do* need that attribute. This tests that we see convergent in some
// of the right places (it's not an exhaustive test)

// REQUIRES: llvm-12
// LLVM attributes are uniqued and shared, yet FileCheck doesn't support
// negative matches for DAG checks. Thus we need this rigmarole of multiple
// independent FileCheck runs
//
// RUN: %oclc -stage cl_snapshot_compilation_front_end %s -o %t.ll
// RUN: %filecheck %s --check-prefixes=CHECK-FUNCTION,CHECK < %t.ll
// RUN: %filecheck %s --check-prefixes=CHECK-GGID,CHECK < %t.ll
// RUN: %filecheck %s --check-prefixes=CHECK-GGID-CALL,CHECK < %t.ll
// RUN: %filecheck %s --check-prefixes=CHECK-GLID,CHECK < %t.ll
// RUN: %filecheck %s --check-prefixes=CHECK-GLID-CALL,CHECK < %t.ll
// RUN: %filecheck %s --check-prefixes=CHECK-KERNEL,CHECK < %t.ll
// RUN: %filecheck %s --check-prefixes=CHECK-BARRIER,CHECK < %t.ll
// RUN: %filecheck %s --check-prefixes=CHECK-BARRIER-CALL,CHECK < %t.ll

int __attribute__((noinline)) function(void) {
  return get_local_id(0) + 1;
}
// CHECK: define dso_local spir_func i32 @function() local_unnamed_addr #[[FUNCTION_ATTRS:[0-9]+]]
// CHECK-LABEL: }
// CHECK: declare spir_func {{.*}} @_Z12get_local_idj(i32) local_unnamed_addr #[[GLID_FN_ATTRS:[0-9]+]]

__kernel void do_stuff(int __global *out, const int __global *in)
{
// CHECK: define dso_local spir_kernel void @do_stuff{{.*}} #[[KERNEL_ATTRS:[0-9]+]]
// CHECK-LABEL: entry
// CHECK: call spir_func {{.*}}@_Z13get_global_idj(i32 0) #[[GGID_CALL_ATTRS:[0-9]+]]
// CHECK: call spir_func {{.*}}_Z7barrierj(i32 1) #[[BARRIER_CALL_ATTRS:[0-9]+]]
// CHECK: call spir_func {{.*}}function() #[[FUNCTION_CALL_ATTRS:[0-9]+]]
    size_t i = get_global_id(0);
    if (i) {
      barrier(CLK_LOCAL_MEM_FENCE);
      out[i] += in[function()];
    }
}
// CHECK: declare spir_func {{.*}}@_Z13get_global_idj(i32){{.*}} #[[GGID_FN_ATTRS:[0-9]+]]
// CHECK: declare spir_func {{.*}}@_Z7barrierj(i32){{.*}} #[[BARRIER_ATTRS:[0-9]+]]

// CHECK-FUNCTION-NOT: attributes #[[FUNCTION_ATTRS]] = {{.*}}convergent
// CHECK-GGID-NOT: attributes #[[GGID_CALL_ATTRS]] = {{.*}}convergent
// CHECK-GGID-CALL-NOT: attributes #[[GGID_CALL_ATTRS]] = {{.*}}convergent
// CHECK-GLID-NOT: attributes #[[GGID_CALL_ATTRS]] = {{.*}}convergent
// CHECK-GLID-CALL-NOT: attributes #[[GGID_CALL_ATTRS]] = {{.*}}convergent
// CHECK-KERNEL-NOT: attributes #[[KERNEL_ATTRS]] = {{.*}}convergent
// CHECK-BARRIER: attributes #[[BARRIER_ATTRS]] = {{{.*}}convergent
// CHECK-BARRIER-CALL: attributes #[[BARRIER_CALL_ATTRS]] = {{{.*}}convergent
