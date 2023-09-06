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

; Reduced from SYCL-CTS ./test_group_functions 'Group broadcast - BroadcastTypes - 7'
; RUN: muxc --passes work-item-loops,verify < %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

%"class.sycl::_V1::id" = type { %"class.sycl::_V1::detail::array" }
%"class.sycl::_V1::detail::array" = type { [1 x i64] }
%"class.sycl::_V1::marray" = type { [5 x float] }

define internal float @_Z13convert_floatm(i64 noundef %x) #0 {
entry:
  %conv.i.i.i.i.i = uitofp i64 %x to float
  ret float %conv.i.i.i.i.i
}

; CHECK: define internal i32 @_ZTS22broadcast_group_kernelILi1EN4sycl3_V16marrayIfLm5EEEE.mux-barrier-region(
define void @_ZTS22broadcast_group_kernelILi1EN4sycl3_V16marrayIfLm5EEEE(ptr addrspace(1) nocapture writeonly %_arg_res_acc, ptr nocapture readonly byval(%"class.sycl::_V1::id") %_arg_res_acc3) #1 !reqd_work_group_size !84 {
entry:
  %0 = load i64, ptr %_arg_res_acc3, align 8
  %1 = tail call i64 @__mux_get_local_size(i32 0) #4, !range !85
; Since we're broadcasting a value based on the local ID, ensure we haven't
; removed it from the barrier struct.
; CHECK: [[T0:%.*]] = tail call i64 @__mux_get_local_id(i32 0)
; CHECK: store i64 [[T0]],
  %2 = tail call i64 @__mux_get_local_id(i32 0) #4, !range !86
  %3 = add nuw nsw i64 %2, 1
  %4 = tail call float @_Z13convert_floatm(i64 %3)
  %add.ptr.i = getelementptr inbounds %"class.sycl::_V1::marray", ptr addrspace(1) %_arg_res_acc, i64 %0
  %5 = bitcast float %4 to i32
  %agg.tmp13.i.i.i.i.i.sroa.0.sroa.4.0.insert.ext = zext i32 %5 to i64
  %agg.tmp13.i.i.i.i.i.sroa.0.sroa.0.0.insert.insert = mul nuw i64 %agg.tmp13.i.i.i.i.i.sroa.0.sroa.4.0.insert.ext, 4294967297
  %6 = tail call i64 @__mux_work_group_broadcast_i64(i32 0, i64 %agg.tmp13.i.i.i.i.i.sroa.0.sroa.0.0.insert.insert, i64 0, i64 0, i64 0) #5
  %7 = tail call i64 @__mux_work_group_broadcast_i64(i32 1, i64 %agg.tmp13.i.i.i.i.i.sroa.0.sroa.0.0.insert.insert, i64 0, i64 0, i64 0) #5
  %8 = add nsw i64 %1, -1
  %9 = tail call i64 @__mux_work_group_broadcast_i64(i32 2, i64 %agg.tmp13.i.i.i.i.i.sroa.0.sroa.4.0.insert.ext, i64 0, i64 0, i64 0) #5
  %10 = icmp eq i64 %2, %8
  br i1 %10, label %if.then.i, label %if.end.i

if.then.i:                                        ; preds = %entry
  %11 = trunc i64 %9 to i32
  %ref.tmp3.i.sroa.5.sroa.5.0.extract.shift = lshr i64 %7, 32
  %ref.tmp3.i.sroa.5.sroa.5.0.extract.trunc = trunc i64 %ref.tmp3.i.sroa.5.sroa.5.0.extract.shift to i32
  %ref.tmp3.i.sroa.5.sroa.0.0.extract.trunc = trunc i64 %7 to i32
  %ref.tmp3.i.sroa.0.sroa.5.0.extract.shift = lshr i64 %6, 32
  %ref.tmp3.i.sroa.0.sroa.5.0.extract.trunc = trunc i64 %ref.tmp3.i.sroa.0.sroa.5.0.extract.shift to i32
  %ref.tmp3.i.sroa.0.sroa.0.0.extract.trunc = trunc i64 %6 to i32
  store i32 %ref.tmp3.i.sroa.0.sroa.0.0.extract.trunc, ptr addrspace(1) %add.ptr.i, align 4
  %local_var.i.sroa.18.0.add.ptr.i.sroa_idx = getelementptr inbounds i8, ptr addrspace(1) %add.ptr.i, i64 4
  store i32 %ref.tmp3.i.sroa.0.sroa.5.0.extract.trunc, ptr addrspace(1) %local_var.i.sroa.18.0.add.ptr.i.sroa_idx, align 4
  %local_var.i.sroa.27.0.add.ptr.i.sroa_idx = getelementptr inbounds i8, ptr addrspace(1) %add.ptr.i, i64 8
  store i32 %ref.tmp3.i.sroa.5.sroa.0.0.extract.trunc, ptr addrspace(1) %local_var.i.sroa.27.0.add.ptr.i.sroa_idx, align 4
  %local_var.i.sroa.36.0.add.ptr.i.sroa_idx = getelementptr inbounds i8, ptr addrspace(1) %add.ptr.i, i64 12
  store i32 %ref.tmp3.i.sroa.5.sroa.5.0.extract.trunc, ptr addrspace(1) %local_var.i.sroa.36.0.add.ptr.i.sroa_idx, align 4
  %local_var.i.sroa.45.0.add.ptr.i.sroa_idx = getelementptr inbounds i8, ptr addrspace(1) %add.ptr.i, i64 16
  store i32 %11, ptr addrspace(1) %local_var.i.sroa.45.0.add.ptr.i.sroa_idx, align 4
  br label %if.end.i

if.end.i:                                         ; preds = %if.then.i, %entry
  %12 = tail call i64 @__mux_work_group_broadcast_i64(i32 3, i64 %agg.tmp13.i.i.i.i.i.sroa.0.sroa.0.0.insert.insert, i64 %8, i64 0, i64 0) #5
  %13 = tail call i64 @__mux_work_group_broadcast_i64(i32 4, i64 %agg.tmp13.i.i.i.i.i.sroa.0.sroa.0.0.insert.insert, i64 %8, i64 0, i64 0) #5
  %14 = tail call i64 @__mux_work_group_broadcast_i64(i32 5, i64 %agg.tmp13.i.i.i.i.i.sroa.0.sroa.4.0.insert.ext, i64 %8, i64 0, i64 0) #5
  %15 = icmp eq i64 %2, 0
  br i1 %15, label %if.then20.i, label %if.end24.i

if.then20.i:                                      ; preds = %if.end.i
  %16 = trunc i64 %14 to i32
  %ref.tmp13.i.sroa.5.sroa.5.0.extract.shift = lshr i64 %13, 32
  %ref.tmp13.i.sroa.5.sroa.5.0.extract.trunc = trunc i64 %ref.tmp13.i.sroa.5.sroa.5.0.extract.shift to i32
  %ref.tmp13.i.sroa.5.sroa.0.0.extract.trunc = trunc i64 %13 to i32
  %ref.tmp13.i.sroa.0.sroa.5.0.extract.shift = lshr i64 %12, 32
  %ref.tmp13.i.sroa.0.sroa.5.0.extract.trunc = trunc i64 %ref.tmp13.i.sroa.0.sroa.5.0.extract.shift to i32
  %ref.tmp13.i.sroa.0.sroa.0.0.extract.trunc = trunc i64 %12 to i32
  %arrayidx.i61.i = getelementptr inbounds %"class.sycl::_V1::marray", ptr addrspace(1) %add.ptr.i, i64 1
  store i32 %ref.tmp13.i.sroa.0.sroa.0.0.extract.trunc, ptr addrspace(1) %arrayidx.i61.i, align 4
  %local_var.i.sroa.18.0.arrayidx.i61.i.sroa_idx = getelementptr inbounds i8, ptr addrspace(1) %arrayidx.i61.i, i64 4
  store i32 %ref.tmp13.i.sroa.0.sroa.5.0.extract.trunc, ptr addrspace(1) %local_var.i.sroa.18.0.arrayidx.i61.i.sroa_idx, align 4
  %local_var.i.sroa.27.0.arrayidx.i61.i.sroa_idx = getelementptr inbounds i8, ptr addrspace(1) %arrayidx.i61.i, i64 8
  store i32 %ref.tmp13.i.sroa.5.sroa.0.0.extract.trunc, ptr addrspace(1) %local_var.i.sroa.27.0.arrayidx.i61.i.sroa_idx, align 4
  %local_var.i.sroa.36.0.arrayidx.i61.i.sroa_idx = getelementptr inbounds i8, ptr addrspace(1) %arrayidx.i61.i, i64 12
  store i32 %ref.tmp13.i.sroa.5.sroa.5.0.extract.trunc, ptr addrspace(1) %local_var.i.sroa.36.0.arrayidx.i61.i.sroa_idx, align 4
  %local_var.i.sroa.45.0.arrayidx.i61.i.sroa_idx = getelementptr inbounds i8, ptr addrspace(1) %arrayidx.i61.i, i64 16
  store i32 %16, ptr addrspace(1) %local_var.i.sroa.45.0.arrayidx.i61.i.sroa_idx, align 4
  br label %if.end24.i

if.end24.i:                                       ; preds = %if.then20.i, %if.end.i
  %17 = tail call i64 @__mux_work_group_broadcast_i64(i32 6, i64 %agg.tmp13.i.i.i.i.i.sroa.0.sroa.0.0.insert.insert, i64 %8, i64 0, i64 0) #5
  %18 = tail call i64 @__mux_work_group_broadcast_i64(i32 7, i64 %agg.tmp13.i.i.i.i.i.sroa.0.sroa.0.0.insert.insert, i64 %8, i64 0, i64 0) #5
  %19 = tail call i64 @__mux_work_group_broadcast_i64(i32 8, i64 %agg.tmp13.i.i.i.i.i.sroa.0.sroa.4.0.insert.ext, i64 %8, i64 0, i64 0) #5
  br i1 %15, label %if.then34.i, label %_ZZZ15broadcast_groupILi1EN4sycl3_V16marrayIfLm5EEEEvRNS1_5queueEENKUlRNS1_7handlerEE_clES7_ENKUlNS1_7nd_itemILi1EEEE_clESA_.exit

if.then34.i:                                      ; preds = %if.end24.i
  %20 = trunc i64 %19 to i32
  %ref.tmp28.i.sroa.5.sroa.5.0.extract.shift = lshr i64 %18, 32
  %ref.tmp28.i.sroa.5.sroa.5.0.extract.trunc = trunc i64 %ref.tmp28.i.sroa.5.sroa.5.0.extract.shift to i32
  %ref.tmp28.i.sroa.5.sroa.0.0.extract.trunc = trunc i64 %18 to i32
  %ref.tmp28.i.sroa.0.sroa.5.0.extract.shift = lshr i64 %17, 32
  %ref.tmp28.i.sroa.0.sroa.5.0.extract.trunc = trunc i64 %ref.tmp28.i.sroa.0.sroa.5.0.extract.shift to i32
  %ref.tmp28.i.sroa.0.sroa.0.0.extract.trunc = trunc i64 %17 to i32
  %arrayidx.i79.i = getelementptr inbounds %"class.sycl::_V1::marray", ptr addrspace(1) %add.ptr.i, i64 2
  store i32 %ref.tmp28.i.sroa.0.sroa.0.0.extract.trunc, ptr addrspace(1) %arrayidx.i79.i, align 4
  %local_var.i.sroa.18.0.arrayidx.i79.i.sroa_idx = getelementptr inbounds i8, ptr addrspace(1) %arrayidx.i79.i, i64 4
  store i32 %ref.tmp28.i.sroa.0.sroa.5.0.extract.trunc, ptr addrspace(1) %local_var.i.sroa.18.0.arrayidx.i79.i.sroa_idx, align 4
  %local_var.i.sroa.27.0.arrayidx.i79.i.sroa_idx = getelementptr inbounds i8, ptr addrspace(1) %arrayidx.i79.i, i64 8
  store i32 %ref.tmp28.i.sroa.5.sroa.0.0.extract.trunc, ptr addrspace(1) %local_var.i.sroa.27.0.arrayidx.i79.i.sroa_idx, align 4
  %local_var.i.sroa.36.0.arrayidx.i79.i.sroa_idx = getelementptr inbounds i8, ptr addrspace(1) %arrayidx.i79.i, i64 12
  store i32 %ref.tmp28.i.sroa.5.sroa.5.0.extract.trunc, ptr addrspace(1) %local_var.i.sroa.36.0.arrayidx.i79.i.sroa_idx, align 4
  %local_var.i.sroa.45.0.arrayidx.i79.i.sroa_idx = getelementptr inbounds i8, ptr addrspace(1) %arrayidx.i79.i, i64 16
  store i32 %20, ptr addrspace(1) %local_var.i.sroa.45.0.arrayidx.i79.i.sroa_idx, align 4
  br label %_ZZZ15broadcast_groupILi1EN4sycl3_V16marrayIfLm5EEEEvRNS1_5queueEENKUlRNS1_7handlerEE_clES7_ENKUlNS1_7nd_itemILi1EEEE_clESA_.exit

_ZZZ15broadcast_groupILi1EN4sycl3_V16marrayIfLm5EEEEvRNS1_5queueEENKUlRNS1_7handlerEE_clES7_ENKUlNS1_7nd_itemILi1EEEE_clESA_.exit: ; preds = %if.then34.i, %if.end24.i
  ret void
}

declare i64 @__mux_work_group_broadcast_i64(i32, i64, i64, i64, i64) #2

declare i64 @__mux_get_local_id(i32) #3

declare i64 @__mux_get_local_size(i32) #3

attributes #0 = { inlinehint mustprogress nofree norecurse nosync nounwind willreturn readnone "no-trapping-math"="true" "stack-protector-buffer-size"="8" "stackrealign" }
attributes #1 = { convergent nounwind "mux-degenerate-subgroups" "mux-kernel"="entry-point" "mux-local-mem-usage"="0" "mux-orig-fn"="_ZTS22broadcast_group_kernelILi1EN4sycl3_V16marrayIfLm5EEEE" "vecz-mode"="never" }
attributes #2 = { alwaysinline convergent norecurse nounwind "vecz-mode"="never" }
attributes #3 = { alwaysinline norecurse nounwind readonly "vecz-mode"="never" }
attributes #4 = { alwaysinline norecurse nounwind readonly }
attributes #5 = { alwaysinline convergent norecurse nounwind }

!opencl.kernels = !{!61}
!opencl.ocl.version = !{!82}

!1 = !{!"kernel_arg_addr_space", i32 1, i32 0}
!2 = !{!"kernel_arg_access_qual", !"none", !"none"}
!5 = !{!"kernel_arg_type_qual", !"", !""}
!6 = !{!"kernel_arg_name", !"_arg_res_acc", !"_arg_res_acc3"}
!32 = !{!"kernel_arg_type", !"class sycl::_V1::marray*", !"class sycl::_V1::id*"}
!33 = !{!"kernel_arg_base_type", !"class sycl::_V1::marray*", !"class sycl::_V1::id*"}
!61 = !{ptr @_ZTS22broadcast_group_kernelILi1EN4sycl3_V16marrayIfLm5EEEE, !1, !2, !32, !33, !5, !6}
!82 = !{i32 3, i32 0}
!84 = !{i32 1024, i32 1, i32 1}
!85 = !{i64 1, i64 1025}
!86 = !{i64 0, i64 1024}
