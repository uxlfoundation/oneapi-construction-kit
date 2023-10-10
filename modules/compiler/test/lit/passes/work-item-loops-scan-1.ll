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

; RUN: muxc --passes work-item-loops,verify < %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

define void @foo(ptr %in, ptr %out) #0 {
entry:
  %id = call i64 @__mux_get_local_id(i32 0)
  %inaddr = getelementptr inbounds i32, ptr %in,  i64 %id
  %val = load i32, ptr %inaddr
  %scan = tail call i32 @__mux_work_group_scan_inclusive_mul_i32(i32 0, i32 %val) #4
  %outaddr = getelementptr inbounds i32, ptr %out,  i64 %id
  store i32 %scan, ptr %outaddr
  ret void
}

; CHECK: define void @foo.mux-barrier-wrapper(ptr %in, ptr %out)

; CHECK-LABEL: loopIR7:
; CHECK: [[PHIZ:%.*]] = phi i32 [ 1, %sw.bb2 ], [ [[ACC:%.*]], %exitIR11 ]

; CHECK-LABEL: loopIR8:
; CHECK: [[PHIY:%.*]] = phi i32 [ [[PHIZ]], %loopIR7 ], [ [[ACC]], %exitIR10 ]

; CHECK-LABEL: loopIR9:
; CHECK: [[PHIX:%.*]] = phi i32 [ [[PHIY]], %loopIR8 ], [ [[ACC]], %loopIR9 ]
; CHECK: [[VAL:%.*]] = load i32, ptr %live_gep_val, align 4
; CHECK: [[ACC]] = mul i32 [[PHIX]], [[VAL]]
; CHECK: call i32 @foo.mux-barrier-region.1(ptr %in, ptr %out, i32 [[ACC]],

declare i32 @__mux_work_group_scan_inclusive_mul_i32(i32, i32) #1

declare i64 @__mux_get_local_id(i32) #2

attributes #0 = { convergent norecurse nounwind "mux-kernel"="entry-point" }
attributes #1 = { alwaysinline convergent norecurse nounwind }
attributes #2 = { alwaysinline norecurse nounwind readonly }

!opencl.ocl.version = !{!0}

!0 = !{i32 3, i32 0}
