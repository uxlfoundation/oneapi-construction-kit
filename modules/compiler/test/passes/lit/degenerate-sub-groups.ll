; Copyright (C) Codeplay Software Limited. All Rights Reserved.
; RUN: %muxc --passes degenerate-sub-groups,verify -S %s | %filecheck %s

; Check that the DegenerateSubGroupPass correctly replaces sub-group
; builtins with work-group collective calls.

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

; CHECK: define spir_func i32 @sub_group_all_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_all_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @_Z14work_group_alli(i32 [[X]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @_Z13sub_group_alli(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @sub_group_any_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_any_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @_Z14work_group_anyi(i32 [[X]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @_Z13sub_group_anyi(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @sub_group_broadcast_test(i32 [[VAL:%.*]], i32 [[LID:%.*]])
define spir_func i32 @sub_group_broadcast_test(i32 %val, i32 %lid) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[LSXi64:%.*]] = call i64 @__mux_get_local_size(i32 0)
; CHECK: [[LSX:%.*]] = trunc i64 [[LSXi64]] to i32
; CHECK: [[LSYi64:%.*]] = call i64 @__mux_get_local_size(i32 1)
; CHECK: [[LSY:%.*]] = trunc i64 [[LSYi64]] to i32
; CHECK: [[X:%.*]] = urem i32 [[LID]], [[LSX]]
; CHECK: [[LIDSUBLSX:%.*]] = sub i32 [[LID]], [[X]]
; CHECK: [[LIDSUBLSXDI:%.*]] = udiv i32 [[LIDSUBLSX]], [[LSX]]
; CHECK: [[Y:%.*]] = urem i32 [[LIDSUBLSXDI]], [[LSY]]
; CHECK-DAG: [[LSXLSY:%.*]] = mul i32 [[LSX]], [[LSY]]
; CHECK-DAG: [[YLSX:%.*]] = mul i32 [[Y]], [[LSX]]
; CHECK-DAG: [[XADDYLSX:%.*]] = add i32 [[X]], [[YLSX]]
; CHECK-DAG: [[LIDSUBXADDYLSX:%.*]] = sub i32 [[LID]], [[XADDYLSX]]
; CHECK: [[Z:%.*]] = udiv i32 [[LIDSUBXADDYLSX]], [[LSXLSY]]
; CHECK: [[Xi64:%.*]] = zext i32 [[X]] to i64
; CHECK: [[Yi64:%.*]] = zext i32 [[Y]] to i64
; CHECK: [[Zi64:%.*]] = zext i32 [[Z]] to i64
; CHECK: [[RESULT:%.*]] = call i32 @_Z20work_group_broadcastimmm(i32 [[VAL]], i64 [[Xi64]], i64 [[Yi64]], i64 [[Zi64]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @_Z19sub_group_broadcastij(i32 %val, i32 %lid)
  ret i32 %call
}

; CHECK: define spir_func i32 @sub_group_reduce_add_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_reduce_add_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @_Z21work_group_reduce_addi(i32 [[X]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @_Z20sub_group_reduce_addi(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @sub_group_reduce_min_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_reduce_min_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @_Z21work_group_reduce_mini(i32 [[X]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @_Z20sub_group_reduce_mini(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @sub_group_reduce_max_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_reduce_max_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @_Z21work_group_reduce_maxi(i32 [[X]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @_Z20sub_group_reduce_maxi(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @sub_group_scan_exclusive_add_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_scan_exclusive_add_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @_Z29work_group_scan_exclusive_addi(i32 [[X]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @_Z28sub_group_scan_exclusive_addi(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @sub_group_scan_exclusive_min_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_scan_exclusive_min_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @_Z29work_group_scan_exclusive_mini(i32 [[X]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @_Z28sub_group_scan_exclusive_mini(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @sub_group_scan_exclusive_max_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_scan_exclusive_max_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @_Z29work_group_scan_exclusive_maxi(i32 [[X]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @_Z28sub_group_scan_exclusive_maxi(i32 %x)
  ret i32 %call
}
; CHECK: define spir_func i32 @sub_group_scan_inclusive_add_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_scan_inclusive_add_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @_Z29work_group_scan_inclusive_addi(i32 [[X]])
entry:
  %call = call spir_func i32 @_Z28sub_group_scan_inclusive_addi(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @sub_group_scan_inclusive_min_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_scan_inclusive_min_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @_Z29work_group_scan_inclusive_mini(i32 [[X]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @_Z28sub_group_scan_inclusive_mini(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @sub_group_scan_inclusive_max_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_scan_inclusive_max_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @_Z29work_group_scan_inclusive_maxi(i32 [[X]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @_Z28sub_group_scan_inclusive_maxi(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @get_sub_group_size_test()
define spir_func i32 @get_sub_group_size_test() #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[X:%.*]] = call spir_func i64 @__mux_get_local_size(i32 0)
; CHECK: [[LOCALSIZETMPA:%.*]] = mul i64 [[X]], 1
; CHECK: [[Y:%.*]] = call spir_func i64 @__mux_get_local_size(i32 1)
; CHECK: [[LOCALSIZETMPB:%.*]] = mul i64 [[Y]], [[LOCALSIZETMPA]]
; CHECK: [[Z:%.*]] = call spir_func i64 @__mux_get_local_size(i32 2)
; CHECK: [[LOCALSIZE:%.*]] = mul i64 [[Z]], [[LOCALSIZETMPB]]
; CHECK: [[RESULT:%.*]] = trunc i64 [[LOCALSIZE]] to i32
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @_Z18get_sub_group_sizev()
  ret i32 %call
}

; CHECK: define spir_func i32 @get_max_sub_group_size_test()
define spir_func i32 @get_max_sub_group_size_test() #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[X:%.*]] = call spir_func i64 @__mux_get_local_size(i32 0)
; CHECK: [[LOCALSIZETMPA:%.*]] = mul i64 [[X]], 1
; CHECK: [[Y:%.*]] = call spir_func i64 @__mux_get_local_size(i32 1)
; CHECK: [[LOCALSIZETMPB:%.*]] = mul i64 [[Y]], [[LOCALSIZETMPA]]
; CHECK: [[Z:%.*]] = call spir_func i64 @__mux_get_local_size(i32 2)
; CHECK: [[LOCALSIZE:%.*]] = mul i64 [[Z]], [[LOCALSIZETMPB]]
; CHECK: [[RESULT:%.*]] = trunc i64 [[LOCALSIZE]] to i32
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @_Z22get_max_sub_group_sizev()
  ret i32 %call
}

; CHECK: define spir_func i32 @get_num_sub_groups_test()
define spir_func i32 @get_num_sub_groups_test() #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: ret i32 1
entry:
  %call = call spir_func i32 @_Z18get_num_sub_groupsv()
  ret i32 %call
}

; CHECK: define spir_func i32 @get_enqueued_num_sub_groups_test()
define spir_func i32 @get_enqueued_num_sub_groups_test() #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: ret i32 1
entry:
  %call = call spir_func i32 @_Z27get_enqueued_num_sub_groupsv()
  ret i32 %call
}

; CHECK: define spir_func i32 @get_sub_group_id_test()
define spir_func i32 @get_sub_group_id_test() #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: ret i32 0
entry:
  %call = call spir_func i32 @_Z16get_sub_group_idv()
  ret i32 %call
}

; CHECK: define spir_func i32 @get_sub_group_local_id_test()
define spir_func i32 @get_sub_group_local_id_test() #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[LLID:%.*]] = call spir_func i64 @__mux_get_local_linear_id()
; CHECK: [[RESULT:%.*]] = trunc i64 [[LLID]] to i32
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @_Z22get_sub_group_local_idv()
  ret i32 %call
}

; CHECK: define spir_func void @sub_group_barrier_test(i32 [[FLAGS:%.*]], i32 [[SCOPE:%.*]])
define spir_func void @sub_group_barrier_test(i32 %flags, i32 %scope) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: call spir_func void @__mux_work_group_barrier(i32 -1, i32 [[FLAGS]], i32 [[SCOPE]])
; CHECK: ret void
entry:
  call spir_func void @__mux_sub_group_barrier(i32 -1, i32 %flags, i32 %scope)
  ret void
}

; CHECK-DAG: declare spir_func i32 @_Z14work_group_alli(i32)
declare spir_func i32 @_Z13sub_group_alli(i32)
; CHECK-DAG: declare spir_func i32 @_Z14work_group_anyi(i32)
declare spir_func i32 @_Z13sub_group_anyi(i32)
; CHECK-DAG: declare spir_func i32 @_Z20work_group_broadcastimmm(i32, i64, i64, i64)
declare spir_func i32 @_Z19sub_group_broadcastij(i32, i32)
; CHECK-DAG: declare spir_func i32 @_Z21work_group_reduce_addi(i32)
declare spir_func i32 @_Z20sub_group_reduce_addi(i32)
; CHECK-DAG: declare spir_func i32 @_Z21work_group_reduce_mini(i32)
declare spir_func i32 @_Z20sub_group_reduce_mini(i32)
; CHECK-DAG: declare spir_func i32 @_Z21work_group_reduce_maxi(i32)
declare spir_func i32 @_Z20sub_group_reduce_maxi(i32)
; CHECK-DAG: declare spir_func i32 @_Z29work_group_scan_exclusive_addi(i32)
declare spir_func i32 @_Z28sub_group_scan_exclusive_addi(i32)
; CHECK-DAG: declare spir_func i32 @_Z29work_group_scan_exclusive_mini(i32)
declare spir_func i32 @_Z28sub_group_scan_exclusive_mini(i32)
; CHECK-DAG: declare spir_func i32 @_Z29work_group_scan_exclusive_maxi(i32)
declare spir_func i32 @_Z28sub_group_scan_exclusive_maxi(i32)
; CHECK-DAG: declare spir_func i32 @_Z29work_group_scan_inclusive_addi(i32)
declare spir_func i32 @_Z28sub_group_scan_inclusive_addi(i32)
; CHECK-DAG: declare spir_func i32 @_Z29work_group_scan_inclusive_mini(i32)
declare spir_func i32 @_Z28sub_group_scan_inclusive_mini(i32)
; CHECK-DAG: declare spir_func i32 @_Z29work_group_scan_inclusive_maxi(i32)
declare spir_func i32 @_Z28sub_group_scan_inclusive_maxi(i32)
; CHECK-DAG: declare spir_func i64 @__mux_get_local_size(i32)
declare spir_func i32 @_Z18get_sub_group_sizev()
declare spir_func i32 @_Z22get_max_sub_group_sizev()
declare spir_func i32 @_Z18get_num_sub_groupsv()
declare spir_func i32 @_Z27get_enqueued_num_sub_groupsv()
declare spir_func i32 @_Z16get_sub_group_idv()
; CHECK-DAG: declare spir_func i64 @__mux_get_local_linear_id()
declare spir_func i32 @_Z22get_sub_group_local_idv()
; CHECK-DAG: declare spir_func void @__mux_work_group_barrier(i32, i32, i32)
declare spir_func void @__mux_sub_group_barrier(i32, i32, i32)

attributes #0 = { "mux-kernel"="entry-point" }
!0 = !{i32 13, i32 64, i32 64}
; CHECK: attributes #0 = { "mux-degenerate-subgroups" "mux-kernel"="entry-point" }
