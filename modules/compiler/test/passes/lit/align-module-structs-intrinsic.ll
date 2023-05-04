; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes align-module-structs,verify -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"

; Note: this custom(!) datalayout has preferred ABI alignments:
;   i32 - 64 bits
;   i64 - 128 bits
; We use these to trigger the struct alignment pass
target datalayout = "e-p:64:64:64-m:e-i32:32-i64:32"

; This test simply checks that if ever need to remap an intrinsic - a rare
; occurrance - we correctly preserve the fact that the intrinsic is an
; intrinsic. This is actually done via an internal assertion, but this test
; case covers that.

; CHECK: declare ptr @llvm.preserve.struct.access.index.p0.p0(ptr, i32 immarg, i32 immarg)

%structTyA = type { i32, i64 }

declare i32* @llvm.preserve.struct.access.index.p0i32.p0s_structTyAs(%structTyA*, i32, i32)
