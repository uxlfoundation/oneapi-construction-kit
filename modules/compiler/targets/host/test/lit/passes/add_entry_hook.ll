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

; RUN: %muxc --device "%default_device" --passes add-entry-hook,verify -S %s  | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: define void @bar.host-entry-hook(i8 signext %x, ptr %wi-info, ptr %sched-info, ptr %wg-info) [[BAR_ATTRS:#[0-9]+]] !test [[FOO_TEST:\![0-9]+]] !mux_scheduled_fn [[FOO_SCHED_FN:\![0-9]+]] {
; CHECK-LABEL: entry:
; CHECK: [[NGPSX:%.*]] = call i64 @__mux_get_num_groups(i32 0, ptr %wi-info, ptr %sched-info, ptr %wg-info)
; CHECK: [[NGPSY:%.*]] = call i64 @__mux_get_num_groups(i32 1, ptr %wi-info, ptr %sched-info, ptr %wg-info)
; CHECK: [[NGPSZ:%.*]] = call i64 @__mux_get_num_groups(i32 2, ptr %wi-info, ptr %sched-info, ptr %wg-info)
; CHECK: [[T0:%.*]] = getelementptr %Mux_schedule_info_s, ptr %sched-info, i32 0, i32 3
; CHECK: [[SLICE:%.*]] = load i64, ptr [[T0]], align 8
; CHECK: [[T1:%.*]] = getelementptr %Mux_schedule_info_s, ptr %sched-info, i32 0, i32 4
; CHECK: [[TTL_SLICES:%.*]] = load i64, ptr [[T1]], align 8
; CHECK: [[NGPS_RNDUP:%.*]] = add i64 [[NGPSX]], [[TTL_SLICES]]
; CHECK: [[SLICE_SZ:%.*]] = udiv i64 [[NGPS_RNDUP]], [[TTL_SLICES]]
; CHECK: [[SLICE_BEG:%.*]] = mul i64 [[SLICE_SZ]], [[SLICE]]
; CHECK: [[SLICE_END:%.*]] = add i64 [[SLICE_BEG]], [[SLICE_SZ]]
; CHECK: [[T2:%.*]] = icmp ult i64 [[SLICE_END]], [[NGPSX]]
; CHECK: [[CLMPD_SLICE_END:%.*]] = select i1 [[T2]], i64 [[SLICE_END]], i64 [[NGPSX]]
; CHECK: [[T3:%.*]] = icmp ult i64 [[SLICE_BEG]], [[CLMPD_SLICE_END]]
; CHECK: br i1 [[T3]], label %[[LOOP:.*]], label %[[EARLY_EXIT:.*]]

; CHECK: [[EARLY_EXIT]]:
; CHECK: ret void

; CHECK: [[LOOP]]:
; CHECK: br label %[[LOOPZ:.*]]

; CHECK: [[LOOPZ]]:
; CHECK: [[PHIZ:%.*]] = phi i64 [ 0, %[[LOOP]] ], [ [[INCZ:%.*]], %[[EXITZ:.*]] ]
; CHECK: [[GEPGPIDS:%.*]] = getelementptr %MiniWGInfo, ptr %wg-info, i32 0, i32 0
; CHECK: [[GEPGPIDZ:%.*]] = getelementptr [3 x i64], ptr [[GEPGPIDS]], i32 0, i32 2
; CHECK: store i64 [[PHIZ]], ptr [[GEPGPIDZ]], align 8
; CHECK: br label %[[LOOPY:.*]]

; CHECK: [[LOOPY]]:
; CHECK: [[PHIY:%.*]] = phi i64 [ 0, %[[LOOPZ]] ], [ [[INCY:%.*]], %[[EXITY:.*]] ]
; CHECK: [[GEPGPIDY:%.*]] = getelementptr [3 x i64], ptr [[GEPGPIDS]], i32 0, i32 1
; CHECK: store i64 [[PHIY]], ptr [[GEPGPIDY]], align 8
; CHECK: br label %[[LOOPX:.*]]

; CHECK: [[LOOPX]]:
; CHECK: [[PHIX:%.*]] = phi i64 [ [[SLICE_BEG]], %[[LOOPY]] ], [ [[INCX:%.*]], %[[LOOPX]] ]
; CHECK: [[GEPGPIDX:%.*]] = getelementptr [3 x i64], ptr [[GEPGPIDS]], i32 0, i32 0
; CHECK: store i64 [[PHIX]], ptr [[GEPGPIDX]], align 8
; CHECK: call void @foo(i8 signext %x, ptr %wi-info, ptr %sched-info, ptr %wg-info) [[FOO_ATTRS:#.*]]
; CHECK: [[INCX]] = add i64 [[PHIX]], 1
; CHECK: [[CMPX:%.*]] = icmp ult i64 [[INCX]], [[CLMPD_SLICE_END]]
; CHECK: br i1 [[CMPX]], label %[[LOOPX]], label %[[EXITY]]

; CHECK: [[EXITY]]:
; CHECK: [[INCY]] = add i64 [[PHIY]], 1
; CHECK: [[CMPY:%.*]] = icmp ult i64 [[INCY]], [[NGPSY]]
; CHECK: br i1 [[CMPY]], label %[[LOOPY]], label %[[EXITZ]]

; CHECK: [[EXITZ]]:
; CHECK: [[INCZ]] = add i64 [[PHIZ]], 1
; CHECK: [[CMPZ:%.*]] = icmp ult i64 [[INCZ]], [[NGPSZ]]
; CHECK: br i1 [[CMPZ]], label %[[LOOPZ]], label %[[EXIT:.*]]

; CHECK: [[EXIT]]:
; CHECK: ret void
define void @foo(i8 signext %x, ptr %wi-info, ptr %sched-info, ptr %wg-info) #0 !test !1 !mux_scheduled_fn !2 {
  ret void
}

; Check we've copied over the function's attributes
; CHECK-DAG: attributes [[BAR_ATTRS]] = { nounwind "mux-base-fn-name"="bar" "mux-kernel"="entry-point" "test"="x" }
; CHECK-DAG: attributes [[FOO_ATTRS]] = { alwaysinline "mux-base-fn-name"="bar" "test"="x" }

; Check the mux_scheduled_fn metadata has been correct updated: we've dropped
; the work-group info (1 -> -1) and added a custom scheduling struct at index 1
; CHECK-DAG: [[FOO_SCHED_FN]] = !{i32 1, i32 2, i32 3}
; CHECK-DAG: [[FOO_TEST]] = !{!"foo"}

attributes #0 = { "mux-base-fn-name"="bar" "mux-kernel"="entry-point" "test"="x" }

!mux-scheduling-params = !{!0}

!0 = !{!"MuxWorkItemInfo", !"Mux_schedule_info_s", !"MiniWGInfo"}

!1 = !{!"foo"}
!2 = !{i32 1, i32 2, i32 3}
