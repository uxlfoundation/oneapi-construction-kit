; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --device "%default_device" --passes run-vecz,barriers-pass -S %s  | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; makes sure we've got exactly 3 members in the scalar barrier struct, ignoring final alignment padding
; CHECK-DAG: %minimal_barrier_live_mem_info = type { {{[^,]+}}, {{[^,]+}}, {{[^,]+}}{{(, \[[0-9]+ x i8\])?}} }

; makes sure we've got exactly 4 members in the vector barrier struct, ignoring final alignment padding
; CHECK-DAG: %__vecz_v4_minimal_barrier_live_mem_info = type { {{[^,]+}}, {{[^,]+}}, {{[^,]+}}, {{[^,]+}}{{(, \[[0-9]+ x i8\])?}} }

define spir_kernel void @minimal_barrier(i32 %min_0, i32 %min_1, i32 %stride, i32 %n0, i32 %n1, i32 %n2, ptr addrspace(1) %g, ptr addrspace(3) %shared) #0 !reqd_work_group_size !0 {
entry:
  %call = tail call spir_func i64 @_Z12get_group_idj(i32 1) #4
  %conv = trunc i64 %call to i32
  %call1 = tail call spir_func i64 @_Z12get_group_idj(i32 0) #4
  %conv2 = trunc i64 %call1 to i32
  %call3 = tail call spir_func i64 @_Z12get_local_idj(i32 1) #4
  %conv4 = trunc i64 %call3 to i32
  %call5 = tail call spir_func i64 @_Z12get_local_idj(i32 0) #4
  %conv6 = trunc i64 %call5 to i32
  %mul = shl nsw i32 %conv, 3
  %add = add nsw i32 %mul, %min_1
  %call7 = tail call spir_func i32 @_Z3minii(i32 %add, i32 %n0) #5
  %mul8 = shl nsw i32 %conv2, 4
  %add9 = add nsw i32 %mul8, %min_0
  %call10 = tail call spir_func i32 @_Z3minii(i32 %add9, i32 %n1) #5
  tail call spir_func void @__mux_work_group_barrier(i32 0, i32 1, i32 272)
  %cmp = icmp eq i32 %conv4, 0
  %cmp12 = icmp ult i32 %conv6, 16
  %or.cond = select i1 %cmp, i1 %cmp12, i1 false
  br i1 %or.cond, label %if.then, label %if.end

if.then:
  %mul14 = mul nuw nsw i32 %conv6, 20
  %add15 = add nuw nsw i32 %mul14, 9
  %add16 = add nuw nsw i32 %mul14, 10
  %0 = zext i32 %add16 to i64
  %1 = zext i32 %add15 to i64
  br label %for.body

for.body:
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body ], [ 0, %if.then ]
  %2 = sub nuw nsw i64 %1, %indvars.iv
  %arrayidx = getelementptr inbounds i32, ptr addrspace(3) %shared, i64 %2
  %3 = load i32, ptr addrspace(3) %arrayidx, align 4
  %4 = add nuw nsw i64 %indvars.iv, %0
  %arrayidx21 = getelementptr inbounds i32, ptr addrspace(3) %shared, i64 %4
  store i32 %3, ptr addrspace(3) %arrayidx21, align 4
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond.not = icmp eq i64 %indvars.iv.next, 10
  br i1 %exitcond.not, label %if.end, label %for.body

if.end:
  tail call spir_func void @__mux_work_group_barrier(i32 1, i32 1, i32 272)
  br i1 %cmp12, label %if.then24, label %if.end35

if.then24:
  %add26 = add nsw i32 %call7, %conv4
  %mul27 = mul nsw i32 %add26, %stride
  %sub25 = sub i32 %conv6, %n2
  %add28 = add i32 %sub25, %call10
  %add29 = add i32 %add28, %mul27
  %5 = load i32, ptr addrspace(3) %shared, align 64
  %arrayidx31 = getelementptr inbounds i32, ptr addrspace(3) %shared, i64 149
  %6 = load i32, ptr addrspace(3) %arrayidx31, align 4
  %add32 = add nsw i32 %6, %5
  %idxprom33 = sext i32 %add29 to i64
  %arrayidx34 = getelementptr inbounds i32, ptr addrspace(1) %g, i64 %idxprom33
  store i32 %add32, ptr addrspace(1) %arrayidx34, align 4
  br label %if.end35

if.end35:
  ret void
}

declare spir_func i64 @_Z12get_group_idj(i32)

declare spir_func i64 @_Z12get_local_idj(i32)

declare spir_func i32 @_Z3minii(i32, i32)

declare spir_func void @__mux_work_group_barrier(i32, i32, i32)

attributes #0 = { "mux-kernel"="entry-point" "vecz-mode"="auto" }

!0 = !{i32 5, i32 1, i32 1}
