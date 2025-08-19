; Copyright (C) Codeplay Software Limited
;
; Licensed under the Apache License, Version 2.0 (the "License") with LLVM
; Exceptions; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

; RUN: muxc --passes work-item-loops,verify < %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

define void @foo(ptr %in, ptr %out) #0 !codeplay_ca_vecz.base !1 {
entry:
  %id = call i64 @__mux_get_local_id(i32 0)
  %inaddr = getelementptr inbounds i32, ptr %in, i64 %id
  %val = load i32, ptr %inaddr, align 4
  %scan = tail call i32 @__mux_work_group_scan_inclusive_mul_i32(i32 0, i32 %val)
  %outaddr = getelementptr inbounds i32, ptr %out, i64 %id
  store i32 %scan, ptr %outaddr, align 4
  ret void
}

declare i32 @__mux_work_group_scan_inclusive_mul_i32(i32, i32) #1

declare i64 @__mux_get_local_id(i32) #2

define void @__vecz_nxv4_foo(ptr %in, ptr %out) #3 !codeplay_ca_vecz.derived !3 {
entry:
  %id = call i64 @__mux_get_local_id(i32 0)
  %inaddr = getelementptr inbounds i32, ptr %in, i64 %id
  %0 = load <vscale x 4 x i32>, ptr %inaddr, align 4
  %1 = call <vscale x 4 x i32> @__vecz_b_sub_group_scan_inclusive_mul_u5nxv4j(<vscale x 4 x i32> %0) #6
  %2 = call i32 @llvm.vector.reduce.mul.nxv4i32(<vscale x 4 x i32> %0)
  %3 = call i32 @__mux_work_group_scan_exclusive_mul_i32(i32 0, i32 %2)
  %.splatinsert = insertelement <vscale x 4 x i32> poison, i32 %3, i64 0
  %.splat = shufflevector <vscale x 4 x i32> %.splatinsert, <vscale x 4 x i32> poison, <vscale x 4 x i32> zeroinitializer
  %4 = mul <vscale x 4 x i32> %1, %.splat
  %outaddr = getelementptr inbounds i32, ptr %out, i64 %id
  store <vscale x 4 x i32> %4, ptr %outaddr, align 4
  ret void
}

; CHECK: define void @__vecz_nxv4_foo.mux-barrier-wrapper(ptr %in, ptr %out)

; Check a linear loop structure, looping over all Zs and Ys, and in the inner
; loop doing the main and tail X items in sequence.
; CHECK-LABEL: loopIR11:
; CHECK: [[PHIZ:%.*]] = phi i32 [ 1, %sw.bb2 ], [ [[TAIL_MERGE:%.*]], %exitIR15 ]

; CHECK-LABEL: loopIR12:
; CHECK: [[PHIY:%.*]] = phi i32 [ [[PHIZ]], %loopIR11 ], [ [[TAIL_MERGE]], %ca_work_item_x_tail_exit ]

; Main loop
; CHECK-LABEL: loopIR13:
; CHECK: [[MPHIX:%.*]] = phi i32 [ [[PHIY]], %ca_work_item_x_main_preheader ], [ [[MACC:%.*]], %loopIR13 ]
; CHECK: [[MVAL:%.*]] = load i32, ptr %live_gep_, align 4
; CHECK: [[MACC]] = mul i32 [[MPHIX]], [[MVAL]]
; This is an exclusive scan, so pass the 'previous' value to the sub-kernel.
; CHECK: call i32 @__vecz_nxv4_foo.mux-barrier-region.1(ptr %in, ptr %out, i32 [[MPHIX]],

; CHECK-LABEL: ca_work_item_x_main_exit:
; CHECK: [[MERGE:%.*]] = phi i32 [ [[PHIY]], %loopIR12 ], [ [[MACC]], %loopIR13 ]

; Tail loop
; CHECK-LABEL: loopIR14:
; CHECK: [[TPHIX:%.*]] = phi i32 [ [[MERGE]], %ca_work_item_x_tail_preheader ],
; CHECK-SAME:                    [ [[TACC:%.*]], %loopIR14 ]
; CHECK: [[TVAL:%.*]] = load i32, ptr %live_gep_val, align 4
; CHECK: [[TACC]] = mul i32 [[TPHIX]], [[TVAL]]
; This is an inclusive scan, so pass the 'current' value to the sub-kernel.
; CHECK: call i32 @foo.mux-barrier-region.2(ptr %in, ptr %out, i32 [[TACC]],

; Note - vecz normally generates the body for this helper, but it's irrelevant
; for this test
declare <vscale x 4 x i32> @__vecz_b_sub_group_scan_inclusive_mul_u5nxv4j(<vscale x 4 x i32> %0)

declare i32 @llvm.vector.reduce.mul.nxv4i32(<vscale x 4 x i32>) #4

declare i32 @__mux_work_group_scan_exclusive_mul_i32(i32, i32) #1

declare <vscale x 4 x i32> @llvm.experimental.stepvector.nxv4i32() #4

declare i32 @llvm.vscale.i32() #4

declare <vscale x 4 x i32> @llvm.masked.gather.nxv4i32.nxv4p0(<vscale x 4 x ptr>, i32 immarg, <vscale x 4 x i1>, <vscale x 4 x i32>) #5

attributes #0 = { convergent norecurse nounwind "mux-kernel"="entry-point" }
attributes #1 = { alwaysinline convergent norecurse nounwind }
attributes #2 = { alwaysinline norecurse nounwind readonly }
attributes #3 = { convergent norecurse nounwind "mux-base-fn-name"="__vecz_nxv4_foo" "mux-kernel"="entry-point" }
attributes #4 = { nocallback nofree nosync nounwind willreturn readnone }
attributes #5 = { nocallback nofree nosync nounwind willreturn readonly }
attributes #6 = { nounwind }

!opencl.ocl.version = !{!0}

!0 = !{i32 3, i32 0}
!1 = !{!2, ptr @__vecz_nxv4_foo}
!2 = !{i32 4, i32 1, i32 0, i32 0}
!3 = !{!2, ptr @foo}
