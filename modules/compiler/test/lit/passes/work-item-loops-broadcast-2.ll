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

; Reduced from SYCL-CTS ./test_group_functions 'Group broadcast - BroadcastTypes - 4'
; RUN: muxc --passes work-item-loops,verify < %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

%"class.sycl::_V1::id" = type { %"class.sycl::_V1::detail::array" }
%"class.sycl::_V1::detail::array" = type { [1 x i64] }

; CHECK: define internal i32 @_ZTS22broadcast_group_kernelILi1EbE.mux-barrier-region(
define void @_ZTS22broadcast_group_kernelILi1EbE(ptr addrspace(1) nocapture writeonly %_arg_res_acc, ptr nocapture readonly byval(%"class.sycl::_V1::id") %_arg_res_acc3) #0 !reqd_work_group_size !84 {
entry:
  %0 = load i64, ptr %_arg_res_acc3, align 8
  %add.ptr.i = getelementptr inbounds i8, ptr addrspace(1) %_arg_res_acc, i64 %0
  %1 = tail call i64 @__mux_get_local_size(i32 0) #3, !range !85
; Since we're broadcasting a value based on the local ID, ensure we haven't
; removed it from the barrier struct.
; CHECK: [[T0:%.*]] = tail call i64 @__mux_get_local_id(i32 0)
; CHECK: store i64 [[T0]],
  %2 = tail call i64 @__mux_get_local_id(i32 0) #3, !range !86
  %3 = add nsw i64 %1, -1
; CHECK:  [[T1:%.*]] = and i64 [[T0]], 1
  %4 = and i64 %2, 1
; CHECK:  [[T2:%.*]] = icmp eq i64 [[T1]], 0
  %5 = icmp eq i64 %4, 0
; CHECK:  [[T3:%.*]] = zext i1 [[T2]] to i32
; CHECK:  store i32 [[T3]],
  %6 = zext i1 %5 to i32
  %7 = tail call i32 @__mux_work_group_broadcast_i32(i32 0, i32 %6, i64 0, i64 0, i64 0) #4
  %8 = icmp eq i64 %2, %3
  br i1 %8, label %if.then.i, label %if.end.i

if.then.i:                                        ; preds = %entry
  %.not2 = icmp ne i32 %7, 0
  %9 = zext i1 %.not2 to i8
  store i8 %9, ptr addrspace(1) %add.ptr.i, align 1
  br label %if.end.i

if.end.i:                                         ; preds = %if.then.i, %entry
  %10 = tail call i32 @__mux_work_group_broadcast_i32(i32 1, i32 %6, i64 %3, i64 0, i64 0) #4
  %11 = icmp eq i64 %2, 0
  br i1 %11, label %if.then25.i, label %if.end31.i

if.then25.i:                                      ; preds = %if.end.i
  %.not1 = icmp ne i32 %10, 0
  %12 = zext i1 %.not1 to i8
  %arrayidx.i68.i = getelementptr inbounds i8, ptr addrspace(1) %add.ptr.i, i64 1
  store i8 %12, ptr addrspace(1) %arrayidx.i68.i, align 1
  br label %if.end31.i

if.end31.i:                                       ; preds = %if.then25.i, %if.end.i
  %13 = tail call i32 @__mux_work_group_broadcast_i32(i32 2, i32 %6, i64 %3, i64 0, i64 0) #4
  br i1 %11, label %if.then43.i, label %_ZZZ15broadcast_groupILi1EbEvRN4sycl3_V15queueEENKUlRNS1_7handlerEE_clES5_ENKUlNS1_7nd_itemILi1EEEE_clES8_.exit

if.then43.i:                                      ; preds = %if.end31.i
  %.not = icmp ne i32 %13, 0
  %14 = zext i1 %.not to i8
  %arrayidx.i78.i = getelementptr inbounds i8, ptr addrspace(1) %add.ptr.i, i64 2
  store i8 %14, ptr addrspace(1) %arrayidx.i78.i, align 1
  br label %_ZZZ15broadcast_groupILi1EbEvRN4sycl3_V15queueEENKUlRNS1_7handlerEE_clES5_ENKUlNS1_7nd_itemILi1EEEE_clES8_.exit

_ZZZ15broadcast_groupILi1EbEvRN4sycl3_V15queueEENKUlRNS1_7handlerEE_clES5_ENKUlNS1_7nd_itemILi1EEEE_clES8_.exit: ; preds = %if.then43.i, %if.end31.i
  ret void
}

declare i32 @__mux_work_group_broadcast_i32(i32, i32, i64, i64, i64) #1

declare i64 @__mux_get_local_id(i32) #2

declare i64 @__mux_get_local_size(i32) #2

attributes #0 = { convergent norecurse nounwind "mux-degenerate-subgroups" "mux-kernel"="entry-point" "mux-local-mem-usage"="0" "mux-orig-fn"="_ZTS22broadcast_group_kernelILi1EbE" "vecz-mode"="never" }
attributes #1 = { alwaysinline convergent norecurse nounwind "vecz-mode"="never" }
attributes #2 = { alwaysinline norecurse nounwind readonly "vecz-mode"="never" }
attributes #3 = { alwaysinline norecurse nounwind readonly }
attributes #4 = { alwaysinline convergent norecurse nounwind }

!opencl.kernels = !{!78}
!opencl.ocl.version = !{!82}

!1 = !{!"kernel_arg_addr_space", i32 1, i32 0}
!2 = !{!"kernel_arg_access_qual", !"none", !"none"}
!5 = !{!"kernel_arg_type_qual", !"", !""}
!6 = !{!"kernel_arg_name", !"_arg_res_acc", !"_arg_res_acc3"}
!24 = !{!"kernel_arg_type", !"uchar*", !"class sycl::_V1::id*"}
!25 = !{!"kernel_arg_base_type", !"uchar*", !"class sycl::_V1::id*"}
!78 = !{ptr @_ZTS22broadcast_group_kernelILi1EbE, !1, !2, !24, !25, !5, !6}
!82 = !{i32 3, i32 0}
!84 = !{i32 1024, i32 1, i32 1}
!85 = !{i64 1, i64 1025}
!86 = !{i64 0, i64 1024}
