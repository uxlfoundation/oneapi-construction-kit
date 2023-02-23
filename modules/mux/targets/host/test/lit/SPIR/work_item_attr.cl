// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Bitcode files generated using Khronos's modified version of Clang:
//
// clang -cc1 -emit-llvm-bc -triple spir-unknown-unknown
//       -include path/to/opencl_spir.h -O0
//       -o work_item_attr.bc32
//          work_item_attr.cl
//
// clang -cc1 -emit-llvm-bc -triple spir64-unknown-unknown
//       -include path/to/opencl_spir.h -O0
//       -o work_item_attr.bc64
//          work_item_attr.cl

// RUN: %oclc %p/Inputs/work_item_attr%spir_extension -cl-options '-x spir -spir-std=1.2' -stage spir > %t
// RUN: %filecheck < %t %s
//
// each of the get_*() functions should be preceded by a readonly
//
// CHECK-NOT: readnone
// CHECK: readonly
// CHECK-NEXT: _Z15get_global_sizej
//
// CHECK-NOT: readnone
// CHECK: readonly
// CHECK-NEXT: _Z13get_global_idj
//
// CHECK-NOT: readnone
// CHECK: readonly
// CHECK-NEXT: _Z17get_global_offsetj
//
// CHECK-NOT: readnone
// CHECK: readonly
// CHECK-NEXT: _Z14get_local_sizej
//
// CHECK-NOT: readnone
// CHECK: readonly
// CHECK-NEXT: _Z12get_local_idj
//
// CHECK-NOT: readnone
// CHECK: readonly
// CHECK-NEXT: _Z14get_num_groupsj
//
// CHECK-NOT: readnone
// CHECK: readonly
// CHECK-NEXT: _Z12get_group_idj

kernel void k(__global size_t* out) {
  out[0] = get_global_size(0);
  out[1] = get_global_id(0);
  out[2] = get_global_offset(0);
  out[3] = get_local_size(0);
  out[4] = get_local_id(0);
  out[5] = get_num_groups(0);
  out[6] = get_group_id(0);
}
