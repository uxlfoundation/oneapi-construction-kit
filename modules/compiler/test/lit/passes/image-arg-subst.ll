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

; This tests certain behaviours of the pass that do not feature in LLVM 17+, as
; there we expect target extension types and not opaque pointers.
; UNSUPPORTED: llvm-17+
; RUN: muxc --passes image-arg-subst,verify %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: define internal spir_kernel void @image_sampler.old(ptr addrspace(1) nocapture writeonly align 4 %out, ptr addrspace(1) %img, ptr addrspace(2) %sampler1, ptr addrspace(2) %sampler2) [[OLD_ATTRS:#[0-9]+]] {
define spir_kernel void @image_sampler(ptr addrspace(1) nocapture writeonly align 4 %out, ptr addrspace(1) %img, ptr addrspace(2) %sampler1, ptr addrspace(2) %sampler2) #0 {
entry:
  %call = tail call i64 @__mux_get_global_id(i32 0) #3
  %conv = trunc i64 %call to i32
  %conv1 = sitofp i32 %conv to float
  %call2 = tail call i64 @__mux_get_global_size(i32 0) #3
  %conv3 = uitofp i64 %call2 to float
  %div = fmul float %conv3, 5.000000e-01
  %div4 = fdiv float %conv1, %div
  %add = fadd float %div4, 0x3FA99999A0000000
; Check we're now calling the 'codeplay' libimg functions
; CHECK: [[T0:%.*]] = addrspacecast ptr addrspace(1) %img to ptr
; CHECK: [[T1:%.*]] = ptrtoint ptr addrspace(2) %sampler1 to i32
; CHECK: = call spir_func <4 x i32> @_Z26__Codeplay_read_imageui_1dP5Imagejf(ptr [[T0]], i32 [[T1]], float %add)
  %call5 = tail call spir_func <4 x i32> @_Z12read_imageui14ocl_image1d_ro11ocl_samplerf(ptr addrspace(1) %img, ptr addrspace(2) %sampler1, float %add) #4
; CHECK: [[T2:%.*]] = addrspacecast ptr addrspace(1) %img to ptr
; CHECK: [[T3:%.*]] = ptrtoint ptr addrspace(2) %sampler2 to i32
; CHECK: %5 = call spir_func <4 x i32> @_Z26__Codeplay_read_imageui_1dP5Imagejf(ptr [[T2]], i32 [[T3]], float %add)
  %call6 = tail call spir_func <4 x i32> @_Z12read_imageui14ocl_image1d_ro11ocl_samplerf(ptr addrspace(1) %img, ptr addrspace(2) %sampler2, float %add) #4
  %0 = extractelement <4 x i32> %call5, i64 0
  %mul = shl nsw i32 %conv, 1
  %idxprom = sext i32 %mul to i64
  %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %out, i64 %idxprom
  store i32 %0, ptr addrspace(1) %arrayidx, align 4
  %1 = extractelement <4 x i32> %call6, i64 0
  %add8 = or i32 %mul, 1
  %idxprom9 = sext i32 %add8 to i64
  %arrayidx10 = getelementptr inbounds i32, ptr addrspace(1) %out, i64 %idxprom9
  store i32 %1, ptr addrspace(1) %arrayidx10, align 4
  ret void
}

; We've updated the old kernel to pass samplers as i32
; CHECK: define spir_kernel void @image_sampler(ptr addrspace(1) nocapture writeonly align 4 %out, ptr addrspace(1) %img, i32 %sampler1, i32 %sampler2) [[NEW_ATTRS:#[0-9]+]] {
; CHECK:   %sampler1.ptrcast = inttoptr i32 %sampler1 to ptr addrspace(2)
; CHECK:   %sampler2.ptrcast = inttoptr i32 %sampler2 to ptr addrspace(2)
; CHECK:   call spir_kernel void @image_sampler.old(
; CHECK-SAME:  ptr addrspace(1) nocapture writeonly align 4 %out, ptr addrspace(1) %img,
; CHECK-SAME:  ptr addrspace(2) %sampler1.ptrcast, ptr addrspace(2) %sampler2.ptrcast) #0
; CHECK:   ret void
; CHECK: }

declare i64 @__mux_get_global_id(i32) #1

declare i64 @__mux_get_global_size(i32) #1

declare spir_func <4 x i32> @_Z12read_imageui14ocl_image1d_ro11ocl_samplerf(ptr addrspace(1), ptr addrspace(2), float) #2

; CHECK-DAG: attributes [[OLD_ATTRS]] = { alwaysinline convergent mustprogress nofree norecurse nounwind willreturn "mux-base-fn-name"="image_sampler" }
; CHECK-DAG: attributes [[NEW_ATTRS]] = { convergent mustprogress nofree norecurse nounwind willreturn "mux-base-fn-name"="image_sampler" }

attributes #0 = { convergent mustprogress nofree norecurse nounwind willreturn }
attributes #1 = { mustprogress nofree nounwind readonly willreturn }
attributes #2 = { convergent mustprogress nofree nounwind readnone willreturn }
attributes #3 = { nobuiltin nounwind readonly willreturn }
attributes #4 = { nobuiltin nounwind readnone willreturn }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!opencl.kernels = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 2}
!3 = !{ptr @image_sampler, !4, !5, !6, !7, !8}
!4 = !{!"kernel_arg_addr_space", i32 1, i32 1, i32 0, i32 0}
!5 = !{!"kernel_arg_access_qual", !"none", !"read_only", !"none", !"none"}
!6 = !{!"kernel_arg_type", !"uint*", !"image1d_t", !"sampler_t", !"sampler_t"}
!7 = !{!"kernel_arg_base_type", !"uint*", !"image1d_t", !"sampler_t", !"sampler_t"}
!8 = !{!"kernel_arg_type_qual", !"", !"", !"", !""}
