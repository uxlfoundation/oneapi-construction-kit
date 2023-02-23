; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes rename-builtins,verify -S %s | %filecheck %s

; Check that the rename-builtins pass correctly updates all __mux functions back to __core

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_func void @foo() {
entry:
    ; CHECK: {{%[0-9a-zA-Z]+}} = tail call spir_func i32 @__core_get_global_id(i32 0)
    %call = tail call spir_func i32 @__mux_get_global_id(i32 0)
    ret void
}

; CHECK: define spir_func i32 @__core_get_global_id(i32 %0) {
; CHECK: entry:
; CHECK: ret i32 0
; CHECK: }
define spir_func i32 @__mux_get_global_id(i32){
entry:
    ret i32 0
}

