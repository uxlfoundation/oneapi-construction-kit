; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; FIXME: The AddEntryHookPass crashes if the host platform doesn't match the
; DataLayout's pointer size. See CA-4604.
; REQUIRES: codegen_x86_64
; RUN: %muxc --device "%default_device" --passes add-sched-params,add-entry-hook,verify -S %s -opaque-pointers | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: define void @bar.host-entry-hook(i8 signext %x, ptr [[WIATTRS:noalias nonnull align 8 dereferenceable\(40\)]] %wi-info, ptr [[SIATTRS:noalias nonnull align 8 dereferenceable\(96\)]] %sched-info, ptr [[WGATTRS:noalias nonnull align 8 dereferenceable\(48\)]] %mini-wg-info) [[BAR_ATTRS:#[0-9]+]] !test [[FOO_TEST:\![0-9]+]] !mux_scheduled_fn [[FOO_SCHED_FN:\![0-9]+]] {
; CHECK-LABEL: entry:
; CHECK: [[NGPSX:%.*]] = call i64 @__mux_get_num_groups(i32 0, ptr %wi-info, ptr %sched-info, ptr %mini-wg-info)
; CHECK: [[NGPSY:%.*]] = call i64 @__mux_get_num_groups(i32 1, ptr %wi-info, ptr %sched-info, ptr %mini-wg-info)
; CHECK: [[NGPSZ:%.*]] = call i64 @__mux_get_num_groups(i32 2, ptr %wi-info, ptr %sched-info, ptr %mini-wg-info)
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
; CHECK: [[GEPGPIDS:%.*]] = getelementptr %MiniWGInfo, ptr %mini-wg-info, i32 0, i32 0
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
; CHECK: call void @foo.mux-sched-wrapper(i8 signext %x, ptr [[WIATTRS]] %wi-info, ptr [[SIATTRS]] %sched-info, ptr [[WGATTRS]] %mini-wg-info) [[FOO_ATTRS:#.*]]
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
define void @foo(i8 signext %x) #0 !test !0 {
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

!0 = !{!"foo"}
