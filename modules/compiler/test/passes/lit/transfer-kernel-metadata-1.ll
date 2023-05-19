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

; RUN: %muxc --passes transfer-kernel-metadata,verify -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: declare spir_kernel void @foo({{.*}}) [[FOO_ATTRS:#[0-9]+]]
declare spir_kernel void @foo(i32 addrspace(1)* %in, i32 addrspace(1)* %out)

; CHECK: declare !reqd_work_group_size [[BAR_WGSIZE:![0-9]+]] spir_kernel void @bar({{.*}}) [[BAR_ATTRS:#[0-9]+]]
declare spir_kernel void @bar(i32 addrspace(1)* %in, i32 addrspace(1)* %out)

; CHECK-DAG: attributes [[FOO_ATTRS]] = { "mux-kernel"="entry-point" "mux-orig-fn"="foo" }
; CHECK-DAG: attributes [[BAR_ATTRS]] = { "mux-kernel"="entry-point" "mux-orig-fn"="bar" }

; CHECK-DAG: [[BAR_WGSIZE]] = !{i32 5, i32 10, i32 1}

!opencl.kernels = !{!0, !1}

!0 = !{void (i32 addrspace(1)*, i32 addrspace(1)*)* @foo, !2, !3, !4, !5, !6}
!1 = !{void (i32 addrspace(1)*, i32 addrspace(1)*)* @bar, !2, !3, !4, !5, !6, !7}
!2 = !{!"kernel_arg_addr_space", i32 1, i32 1}
!3 = !{!"kernel_arg_access_qual", !"none", !"none"}
!4 = !{!"kernel_arg_type", !"int*", !"int*"}
!5 = !{!"kernel_arg_base_type", !"int*", !"int*"}
!6 = !{!"kernel_arg_type_qual", !"", !""}
!7 = !{!"reqd_work_group_size", i32 5, i32 10, i32 1}
