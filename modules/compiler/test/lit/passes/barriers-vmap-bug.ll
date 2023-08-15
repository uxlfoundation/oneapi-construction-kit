; Copyright (C) Codeplay Software Limited
;
; Licensed under the Apache License, Version 2.0 (the "License") with LLVM
; Exceptions; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

; Check we don't crash. See CA-3708 for details.
; RUN: muxc --passes barriers-pass,verify -S %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

@barrier_bug.tmp = internal addrspace(3) global [1024 x float] undef, align 4

define void @barrier_bug(float addrspace(1)* %data, i32 %n) #0 {
entry:
  %cmp3.not = icmp eq i32 %n, 0
  br i1 %cmp3.not, label %for.cond.cleanup, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  %sub.i = add i32 %n, 31
  %shl.mask.i = and i32 %sub.i, 31
  br label %for.body

for.cond.cleanup:                                 ; preds = %wibble.exit, %entry
  ret void

for.body:                                         ; preds = %wibble.exit, %for.body.preheader
  %i.04 = phi i32 [ %add, %wibble.exit ], [ 0, %for.body.preheader ]
  %call.i = tail call i64 @__mux_get_local_id(i32 0) #5
  %conv.i = trunc i64 %call.i to i32
  %i.0.highbits35.i = lshr i32 %conv.i, %shl.mask.i
  %cmp36.i = icmp eq i32 %i.0.highbits35.i, 0
  br i1 %cmp36.i, label %for.body.lr.ph.i, label %wibble.exit

for.body.lr.ph.i:                                 ; preds = %for.body
  %cmp833.i = icmp ult i32 %conv.i, 1024
  %0 = and i32 %conv.i, -512
  %1 = and i64 %call.i, 4294967295
  br label %for.body.i

for.body.i:                                       ; preds = %for.cond.cleanup10.i, %for.body.lr.ph.i
  %i.037.i = phi i32 [ %conv.i, %for.body.lr.ph.i ], [ %add20.i, %for.cond.cleanup10.i ]
  tail call void @__mux_work_group_barrier(i32 0, i32 1, i32 272)
  br label %for.body6.i

for.cond.cleanup5.i:                              ; preds = %for.body6.i
  br i1 %cmp833.i, label %for.body11.lr.ph.i, label %for.cond.cleanup10.i

for.body11.lr.ph.i:                               ; preds = %for.cond.cleanup5.i
  %2 = shl i32 %i.037.i, 1
  %mul.i = and i32 %2, -1024
  %add12.i = sub i32 %mul.i, %0
  br label %for.body11.i

for.body6.i:                                      ; preds = %for.body6.i, %for.body.i
  %p.032.i = phi i32 [ 0, %for.body.i ], [ %inc.i, %for.body6.i ]
  tail call void @__mux_work_group_barrier(i32 1, i32 1, i32 272)
  %inc.i = add nuw nsw i32 %p.032.i, 1
  %exitcond.not.i = icmp eq i32 %inc.i, 16
  br i1 %exitcond.not.i, label %for.cond.cleanup5.i, label %for.body6.i

for.cond.cleanup10.i:                             ; preds = %for.body11.i, %for.cond.cleanup5.i
  tail call void @__mux_work_group_barrier(i32 2, i32 1, i32 528)
  %add20.i = add i32 %i.037.i, 512
  %i.0.highbits.i = lshr i32 %add20.i, %shl.mask.i
  %cmp.i = icmp eq i32 %i.0.highbits.i, 0
  br i1 %cmp.i, label %for.body.i, label %wibble.exit

for.body11.i:                                     ; preds = %for.body11.i, %for.body11.lr.ph.i
  %indvars.iv.i = phi i64 [ %indvars.iv.next.i, %for.body11.i ], [ %1, %for.body11.lr.ph.i ]
  %arrayidx.i = getelementptr inbounds [1024 x float], [1024 x float] addrspace(3)* @barrier_bug.tmp, i32 0, i64 %indvars.iv.i
  %3 = load float, float addrspace(3)* %arrayidx.i, align 4
  %4 = trunc i64 %indvars.iv.i to i32
  %sub13.i = add i32 %add12.i, %4
  %idxprom14.i = zext i32 %sub13.i to i64
  %arrayidx15.i = getelementptr inbounds float, float addrspace(1)* %data, i64 %idxprom14.i
  store float %3, float addrspace(1)* %arrayidx15.i, align 4
  %indvars.iv.next.i = add nuw nsw i64 %indvars.iv.i, 256
  %cmp8.i = icmp ult i64 %indvars.iv.i, 768
  br i1 %cmp8.i, label %for.body11.i, label %for.cond.cleanup10.i

wibble.exit:                                      ; preds = %for.cond.cleanup10.i, %for.body
  %add = add nuw i32 %i.04, 1
  %exitcond.not = icmp eq i32 %add, %n
  br i1 %exitcond.not, label %for.cond.cleanup, label %for.body
}

declare void @__mux_work_group_barrier(i32, i32, i32)

declare i64 @__mux_get_local_id(i32) #3

declare i64 @__mux_get_local_size(i32)

declare void @__mux_set_local_id(i32, i64)

attributes #0 = { "mux-kernel"="entry-point" }
