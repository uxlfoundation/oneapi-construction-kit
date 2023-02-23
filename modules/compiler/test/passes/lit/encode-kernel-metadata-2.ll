; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes "encode-kernel-metadata<name=foo;local-sizes=4:8:16>,verify" -S %s | %filecheck %s
; RUN: %not %muxc --passes "encode-kernel-metadata<name=foo;local-sizes=4:8>,verify" -S %s 2>&1 | %filecheck %s --check-prefix INVALID

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; INVALID: invalid local-sizes parameter to EncodeKernelMetadataPass - all 3 dimensions must be provided

; CHECK: declare !reqd_work_group_size [[LOCAL_SIZE:![0-9]+]] spir_kernel void @foo() [[ATTRS:#[0-9]+]]
declare spir_kernel void @foo()

; CHECK-NOT declare {{.*}} @bar() {{#.*}}
declare spir_kernel void @bar()

; CHECK-DAG: attributes [[ATTRS]] = { "mux-kernel"="entry-point" "mux-orig-fn"="foo" }

; CHECK-DAG: [[LOCAL_SIZE]] = !{i32 4, i32 8, i32 16}
