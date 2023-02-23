// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Test Brief:
// Check DI is preserved in the presence of kernel
// variables allocating data in the __private address space.

// RUN: %oclc -cl-options '-g -cl-opt-disable' %s -stage %stage -enqueue private_array > %t
// RUN: %filecheck < %t %s

#define SIZE 16
kernel void private_array(global int *in,
                          global int *out)
{
  size_t tid = get_global_id(0);
  private int array[SIZE];
  for (int i = 0; i < SIZE; i++)
  {
    array[i] = in[i];
  }

  int sum = 0;
  for (int i = 0; i < SIZE; i++)
  {
    sum += array[i];
  }
  out[tid] = sum;
}

// Assert the DI entry is attached to the correct IR function
// CHECK: void @__mux_host_0({{.*}} !dbg [[DI_SUBPROGRAM:![0-9]+]] {

// 'array' __private variable
// CHECK: call void @llvm.dbg.declare({{.*}} %array{{.*}}, metadata [[DI_ARRAY:![0-9]+]], metadata

// Assert we have all the necessary DI entries
// CHECK: [[DI_SUBPROGRAM]] = distinct !DISubprogram(name: "private_array",

// Assert kernel has a line number
// CHECK-SAME: line: 11,

// CHECK: [[DI_BASIC:![0-9]+]] = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)

// Assert we have the 'array' private address space variable
// CHECK: [[DI_ARRAY]] = !DILocalVariable(name: "array",
// CHECK-SAME: scope: [[DI_SUBPROGRAM]], file: !{{[0-9]+}}, line: 15, type: [[DI_COMPOSITE:![0-9]+]]

// Assert variable has the correct type
// CHECK: [[DI_COMPOSITE]] = !DICompositeType(tag: DW_TAG_array_type, baseType: [[DI_BASIC]], size: 512, elements: [[DI_ELEM:![0-9]+]])
// CHECK: [[DI_ELEM]] = !{[[DI_SUBRANGE:![0-9]+]]}
// CHECK: [[DI_SUBRANGE]] = !DISubrange(count: 16)
