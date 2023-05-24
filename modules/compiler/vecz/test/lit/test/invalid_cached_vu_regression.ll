; RUN: %not %veczc -k noduplicate:4,8 -S < %s 2>&1 | %filecheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @noduplicate(i32 addrspace(1)* %in1, i32 addrspace(1)* %out) {
entry:
  %tid = call spir_func i64 @_Z13get_global_idj(i32 0) #3
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in1, i64 %tid
  %i1 = load i32, i32 addrspace(1)* %arrayidx, align 16
  %dec = call i32 @llvm.loop.decrement.reg.i32(i32 %i1, i32 4)
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %tid
  store i32 %dec, i32 addrspace(1)* %arrayidx2, align 16
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)
declare i32 @llvm.loop.decrement.reg.i32(i32, i32)

;CHECK: Failed to vectorize function 'noduplicate'
