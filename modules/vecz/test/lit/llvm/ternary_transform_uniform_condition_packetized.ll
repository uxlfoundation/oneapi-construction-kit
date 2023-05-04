; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -k test_ternary -vecz-passes=ternary-transform,packetizer -vecz-simd-width=4 -S < %s | %filecheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_ternary(i64 %a, i64 %b, i64* %c) {
entry:
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  %gid_offset = add i64 %gid, 16
  %cond = icmp eq i64 %a, 0
  %c0 = getelementptr i64, i64* %c, i64 %gid
  store i64 %b, i64* %c0, align 4
  %c1 = getelementptr i64, i64* %c, i64 %gid_offset
  store i64 0, i64* %c1, align 4
  %c2 = select i1 %cond, i64* %c0, i64* %c1
  %c3 = getelementptr i64, i64* %c2, i64 0
  store i64 1, i64* %c3, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; This checks that the ternary transform is not applied when the condition is
; uniform and the two strides are equal, and that the result is a contiguous
; vector store.

; CHECK: %[[SELECT:.+]] = select i1 %cond, ptr %c0, ptr %c1
; CHECK: %[[BASE:.+]] = getelementptr i64, ptr %[[SELECT]], i64 0
; CHECK: store <4 x i64> <i64 1, i64 1, i64 1, i64 1>, ptr %[[BASE]], align 4
; CHECK: ret void
