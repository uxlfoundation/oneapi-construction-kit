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

; REQUIRES: llvm-17+
; RUN: muxc --passes replace-target-ext-tys,image-arg-subst,verify %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: define spir_kernel void @image_sampler(ptr addrspace(1) writeonly align 4 %out, ptr %img, i64 %sampler1, i64 %sampler2) {{#[0-9]+}} {
define spir_kernel void @image_sampler(ptr addrspace(1) writeonly align 4 %out, target("spirv.Image", void, 0, 0, 0, 0, 0, 0, 0) %img, target("spirv.Sampler") %sampler1, target("spirv.Sampler") %sampler2) #0 {
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
; CHECK: [[T0:%.*]] = trunc i64 %sampler1 to i32
; CHECK: = call spir_func <4 x i32> @_Z26__Codeplay_read_imageui_1dP5Imagejf(ptr %img, i32 [[T0]], float %add)
  %call5 = tail call spir_func <4 x i32> @_Z12read_imageui14ocl_image1d_ro11ocl_samplerf(target("spirv.Image", void, 0, 0, 0, 0, 0, 0, 0) %img, target("spirv.Sampler") %sampler1, float %add) #4
; CHECK: [[T1:%.*]] = trunc i64 %sampler2 to i32
; CHECK: = call spir_func <4 x i32> @_Z26__Codeplay_read_imageui_1dP5Imagejf(ptr %img, i32 [[T1]], float %add)
  %call6 = tail call spir_func <4 x i32> @_Z12read_imageui14ocl_image1d_ro11ocl_samplerf(target("spirv.Image", void, 0, 0, 0, 0, 0, 0, 0) %img, target("spirv.Sampler") %sampler2, float %add) #4
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

declare i64 @__mux_get_global_id(i32) #1

declare i64 @__mux_get_global_size(i32) #1

declare spir_func <4 x i32> @_Z12read_imageui14ocl_image1d_ro11ocl_samplerf(ptr addrspace(1), ptr addrspace(2), float) #2

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
