; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -S < %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @gep_duplication(i32 addrspace(1)* %out) {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %conv = trunc i64 %call to i32
  %and = and i32 %conv, 1
  %arrayidx9 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %call
  %0 = xor i32 %and, 1
  store i32 %0, i32 addrspace(1)* %arrayidx9, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; CHECK: spir_kernel void @__vecz_v{{[0-9]+}}_gep_duplication
; CHECK: entry:
; CHECK-GE15: getelementptr inbounds [2 x i32], ptr %myStruct, i{{32|64}} 0, i{{32|64}} 1
; CHECK-GE15-NOT: getelementptr {{.*}}%myStruct
; CHECK-LT15: getelementptr inbounds %struct.testStruct{{(\.[0-9])?}}, %struct.testStruct{{(\.[0-9])?}}* %myStruct, i{{32|64}} 0, i{{32|64}} 0, i{{32|64}} 0
; CHECK-LT15: getelementptr inbounds %struct.testStruct{{(\.[0-9])?}}, %struct.testStruct{{(\.[0-9])?}}* %myStruct, i{{32|64}} 0, i{{32|64}} 0, i{{32|64}} 1
; CHECK-LT15-NOT: getelementptr inbounds %struct.testStruct{{(\.[0-9])?}}, %struct.testStruct{{(\.[0-9])?}}* %myStruct, i{{32|64}} 0, i{{32|64}} 0, i{{32|64}} 0
; CHECK-LT15-NOT: getelementptr inbounds %struct.testStruct{{(\.[0-9])?}}, %struct.testStruct{{(\.[0-9])?}}* %myStruct, i{{32|64}} 0, i{{32|64}} 0, i{{32|64}} 1
