; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %not %muxc --passes encode-kernel-metadata,verify -S %s 2>&1 | %filecheck %s --check-prefix INVALID_NAME
; RUN: %muxc --passes "encode-kernel-metadata<name=foo>,verify" -S %s | %filecheck %s --check-prefixes CHECK,DEFAULT

; RUN: %not %muxc --passes "encode-kernel-metadata<wibble>,verify" -S %s 2>&1 | %filecheck %s --check-prefix INVALID_OPTION

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; INVALID_NAME: EncodeKernelMetadataPass must be provided a 'name'
; INVALID_OPTION: invalid EncodeKernelMetadataPass parameter 'wibble'

; CHECK: declare spir_kernel void @foo() [[ATTRS:#[0-9]+]]
declare spir_kernel void @foo()

; CHECK-NOT: declare spir_kernel void @bar() {{#[0-9]+}}
declare spir_kernel void @bar()

; DEFAULT-DAG: attributes [[ATTRS]] = { "mux-kernel"="entry-point" "mux-orig-fn"="foo" }
