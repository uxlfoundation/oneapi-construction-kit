// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc %s -cl-options '-cl-kernel-arg-info' -stage cl_snapshot_compilation_front_end | %filecheck %s

void kernel foo(const int a) {}

// CHECK: !{{{(ptr|void \(i32\)\*)}} @foo, [[ADDR_SPACE:![0-9]+]], [[ACCESS_QUAL:![0-9]+]], [[TYPE:![0-9]+]], [[BASE_TYPE:![0-9]+]], [[TYPE_QUAL:![0-9]+]], [[ARG_NAME:![0-9]+]]}
// CHECK: [[ADDR_SPACE]] = !{!"kernel_arg_addr_space", i32 0}
// CHECK: [[ACCESS_QUAL]] = !{!"kernel_arg_access_qual", !"none"}
// CHECK: [[TYPE]] = !{!"kernel_arg_type", !"int"}
// CHECK: [[BASE_TYPE]] = !{!"kernel_arg_base_type", !"int"}
// CHECK: [[TYPE_QUAL]] = !{!"kernel_arg_type_qual", !""}
// CHECK: [[ARG_NAME]] = !{!"kernel_arg_name", !"a"}
