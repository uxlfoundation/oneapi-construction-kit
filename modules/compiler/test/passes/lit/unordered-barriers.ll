; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes barriers-pass,verify -S %s  | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; The purpose of this test is to ensure all barriers get properly deleted from
; the sub-kernels, even in this really weird edge case we seem to have found
; (see CA-3700)
; CHECK: i32 @unordered_barriers.mux-barrier-region(
; CHECK-NOT: call void @__mux_work_group_barrier()
; CHECK: ret i32

; CHECK: i32 @unordered_barriers.mux-barrier-region.1(
; CHECK-NOT: call void @__mux_work_group_barrier()
; CHECK: ret i32

; CHECK: i32 @unordered_barriers.mux-barrier-region.2(
; CHECK-NOT: call void @__mux_work_group_barrier()
; CHECK: ret i32

; CHECK: void @unordered_barriers.mux-barrier-wrapper(
; CHECK-NOT: call void @__mux_work_group_barrier()
; CHECK: ret void

define void @unordered_barriers(ptr addrspace(1) %srcptr, ptr addrspace(1) %dstptr) #0 {
entry:
  %call = tail call i64 @_Z12get_local_idj(i32 0) #6
  %conv = trunc i64 %call to i32
  %call1 = tail call i64 @_Z13get_global_idj(i32 0)
  %0 = add nsw i32 %conv, -1
  %1 = icmp ult i32 %0, 32
  br i1 %1, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %idxprom = zext i32 %0 to i64
  %arrayidx = getelementptr inbounds float, ptr addrspace(1) %srcptr, i64 %idxprom
  %2 = load float, ptr addrspace(1) %arrayidx, align 4
  %add6 = fadd float %2, 0.000000e+00
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %total_sum.0 = phi float [ %add6, %if.then ], [ 0.000000e+00, %entry ]
  tail call void @__mux_work_group_barrier(i32 0, i32 1, i32 272)
  %3 = icmp ult i32 %conv, 31
  br i1 %3, label %if.then17, label %if.end21

if.then17:                                        ; preds = %if.end
  %4 = add nuw nsw i64 %call, 1
  %arrayidx19 = getelementptr inbounds float, ptr addrspace(1) %srcptr, i64 %4
  %5 = load float, ptr addrspace(1) %arrayidx19, align 4
  %add20 = fadd float %total_sum.0, %5
  br label %if.end21

if.end21:                                         ; preds = %if.then17, %if.end
  %total_sum.2 = phi float [ %add20, %if.then17 ], [ %total_sum.0, %if.end ]
  tail call void @__mux_work_group_barrier(i32 1, i32 1, i32 272)
  %arrayidx23 = getelementptr inbounds float, ptr addrspace(1) %dstptr, i64 %call1
  store float %total_sum.2, ptr addrspace(1) %arrayidx23, align 4
  ret void
}

declare i64 @_Z12get_local_idj(i32 %x)

declare i64 @_Z13get_global_idj(i32 %x)

declare void @__mux_work_group_barrier(i32, i32, i32)

declare <16 x float> @__vecz_b_masked_gather_load4_Dv16_fDv16_u3ptrU3AS1Dv16_b(<16 x ptr addrspace(1)> %0, <16 x i1> %1)

declare <16 x float> @__vecz_b_masked_load4_Dv16_fu3ptrU3AS1Dv16_b(ptr addrspace(1) %0, <16 x i1> %1)

declare <16 x float> @llvm.masked.gather.v16f32.v16p1(<16 x ptr addrspace(1)>, i32 immarg, <16 x i1>, <16 x float>)

declare <16 x float> @llvm.masked.load.v16f32.p1(ptr addrspace(1), i32 immarg, <16 x i1>, <16 x float>)

declare i64 @__mux_get_local_id(i32 noundef)

declare i64 @__mux_get_global_id(i32 noundef)

declare i64 @__mux_get_local_size(i32)

declare i64 @__mux_get_group_id(i32)

declare i64 @__mux_get_global_offset(i32)

declare void @__mux_set_local_id(i32, i64)

attributes #0 = { "mux-kernel"="entry-point" }

!12 = !{i32 0}
!20 = !{i32 1}
