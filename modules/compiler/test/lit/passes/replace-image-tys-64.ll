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

; REQUIRES: llvm-17+
; Test the pass in various configurations: replacing all target types, only
; replacing samplers and only replacing images.
; RUN: muxc --passes replace-target-ext-tys,verify %s \
; RUN:   | FileCheck %s --check-prefixes CHECK,CHECK-IMG,CHECK-SAMP
; RUN: muxc --passes "replace-target-ext-tys<no-images>",verify %s \
; RUN:   | FileCheck %s --check-prefixes CHECK,CHECK-NOIMG,CHECK-SAMP
; RUN: muxc --passes "replace-target-ext-tys<no-samplers>",verify %s \
; RUN:   | FileCheck %s --check-prefixes CHECK,CHECK-IMG,CHECK-NOSAMP

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
; CHECK: define spir_kernel void @image_sampler(ptr addrspace(1) nocapture writeonly %v0,
; CHECK-IMG-SAME: ptr %img,
; CHECK-NOIMG-SAME: target("spirv.Image", void, 0, 0, 0, 0, 0, 0, 0) %img,
; CHECK-SAMP-SAME: i64 %sampler1, i64 %sampler2
; CHECK-NOSAMP-SAME: target("spirv.Sampler") %sampler1, target("spirv.Sampler") %sampler2
; CHECK-SAME: ) #0 !custom_metadata [[MD:\![0-9]+]] {
define spir_kernel void @image_sampler(ptr addrspace(1) nocapture writeonly %v0, target("spirv.Image", void, 0, 0, 0, 0, 0, 0, 0) %img, target("spirv.Sampler") %sampler1, target("spirv.Sampler") %sampler2) #0 !custom_metadata !9 {
; Check that a sampler stored to and loaded from a stack slot is also remapped
; CHECK-SAMP: alloca i64, align 8
  %v4 = alloca target("spirv.Sampler"), align 8
; CHECK-SAMP: store i64 %sampler2, ptr %v4, align 8
  store target("spirv.Sampler") %sampler2, ptr %v4, align 8
  %v5 = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %v6 = trunc i64 %v5 to i32
  %v7 = tail call spir_func float @_Z13convert_floati(i32 %v6)
  %v8 = tail call spir_func i64 @_Z15get_global_sizej(i32 0)
  %v9 = tail call spir_func float @_Z13convert_floatm(i64 %v8)
  %v10 = fmul float %v9, 5.000000e-01
  %v11 = fdiv float %v7, %v10
  %v12 = fadd float %v11, 0x3FA99999A0000000
; CHECK:  %v13 = tail call spir_func <4 x i32> @_Z12read_imageui11ocl_image1d11ocl_samplerf(
; CHECK-IMG-SAME: ptr %img,
; CHECK-SAMP-SAME: i64 %sampler1,
; CHECK-SAME: float %v12)
  %v13 = tail call spir_func <4 x i32> @_Z12read_imageui11ocl_image1d11ocl_samplerf(target("spirv.Image", void, 0, 0, 0, 0, 0, 0, 0) %img, target("spirv.Sampler") %sampler1, float %v12)
; CHECK-SAMP: %sampler2_reload = load i64, ptr %v4, align 8
  %sampler2_reload = load target("spirv.Sampler"), ptr %v4, align 8
; CHECK: %v14 = tail call spir_func <4 x i32> @_Z12read_imageui11ocl_image1d11ocl_samplerf(
; CHECK-IMG-SAME: ptr %img,
; CHECK-NOIMG-SAME: target("spirv.Image", void, 0, 0, 0, 0, 0, 0, 0) %img,
; CHECK-SAMP-SAME: i64 %sampler2_reload,
; CHECK-NOSAMP-SAME: target("spirv.Sampler") %sampler2_reload,
; CHECK-SAME: float %v12)
  %v14 = tail call spir_func <4 x i32> @_Z12read_imageui11ocl_image1d11ocl_samplerf(target("spirv.Image", void, 0, 0, 0, 0, 0, 0, 0) %img, target("spirv.Sampler") %sampler2_reload, float %v12)
; Check that a sampler introduced to the program by a function is also remapped
; CHECK-SAMP: %sampler3 = call i64 @__translate_sampler_initializer(i32 42)
  %sampler3 = call target("spirv.Sampler") @__translate_sampler_initializer(i32 42)
; CHECK: %v23 = tail call spir_func <4 x i32> @_Z12read_imageui11ocl_image1d11ocl_samplerf(
; CHECK-IMG-SAME: ptr %img,
; CHECK-NOIMG-SAME: target("spirv.Image", void, 0, 0, 0, 0, 0, 0, 0) %img,
; CHECK-SAMP-SAME: i64 %sampler3,
; CHECK-NOSAMP-SAME: target("spirv.Sampler") %sampler3,
; CHECK-SAME: float %v12)
  %v23 = tail call spir_func <4 x i32> @_Z12read_imageui11ocl_image1d11ocl_samplerf(target("spirv.Image", void, 0, 0, 0, 0, 0, 0, 0) %img, target("spirv.Sampler") %sampler3, float %v12)
  %v15 = extractelement <4 x i32> %v13, i64 0
  %v16 = shl nsw i32 %v6, 1
  %v17 = sext i32 %v16 to i64
  %v18 = getelementptr inbounds i32, ptr addrspace(1) %v0, i64 %v17
  store i32 %v15, ptr addrspace(1) %v18, align 4
  %v19 = extractelement <4 x i32> %v14, i64 0
  %v20 = or i32 %v16, 1
  %v21 = sext i32 %v20 to i64
  %v22 = getelementptr inbounds i32, ptr addrspace(1) %v0, i64 %v21
  store i32 %v19, ptr addrspace(1) %v22, align 4
  %v24 = extractelement <4 x i32> %v23, i64 0
  %v25 = shl nsw i32 %v6, 2
  %v26 = sext i32 %v25 to i64
  %v27 = getelementptr inbounds i32, ptr addrspace(1) %v0, i64 %v26
  store i32 %v24, ptr addrspace(1) %v27, align 4
  ret void
}

declare spir_func float @_Z13convert_floati(i32) #0

declare spir_func float @_Z13convert_floatm(i64) #0

; CHECK-SAMP: declare i64 @__translate_sampler_initializer(i32) #0
declare target("spirv.Sampler") @__translate_sampler_initializer(i32) #0

; CHECK: declare spir_func <4 x i32> @_Z12read_imageui11ocl_image1d11ocl_samplerf(
; CHECK-IMG-SAME: ptr,
; CHECK-NOIMG-SAME: target("spirv.Image", void, 0, 0, 0, 0, 0, 0, 0),
; CHECK-SAMP-SAME: i64,
; CHECK-NOSAMP-SAME: target("spirv.Sampler"),
; CHECK-SAME: float) #0
declare spir_func <4 x i32> @_Z12read_imageui11ocl_image1d11ocl_samplerf(target("spirv.Image", void, 0, 0, 0, 0, 0, 0, 0), target("spirv.Sampler"), float) #0

declare spir_func i64 @_Z13get_global_idj(i32) #1

declare spir_func i64 @_Z15get_global_sizej(i32) #1

attributes #0 = { convergent nounwind }
attributes #1 = { nofree nounwind memory(read) }

!llvm.ident = !{!0}
!opencl.kernels = !{!1}
!opencl.ocl.version = !{!8}

; Check that the new functions have replaced the old ones in metadata
; CHECK-DAG:        = !{ptr @image_sampler,
; Check that the global metadata has been updated with the new types
; CHECK-SAMP-DAG: [[MD]] = !{!"custom_metadata", i64 0}
!0 = !{!"Source language: OpenCL C, Version: 102000"}
!1 = !{ptr @image_sampler, !2, !3, !4, !5, !6, !7}
!2 = !{!"kernel_arg_addr_space", i32 1, i32 0, i32 0, i32 0}
!3 = !{!"kernel_arg_access_qual", !"none", !"read_only", !"none", !"none"}
!4 = !{!"kernel_arg_type", !"uint*", !"image1d_t", !"sampler_t", !"sampler_t"}
!5 = !{!"kernel_arg_base_type", !"uint*", !"image1d_t", !"sampler_t", !"sampler_t"}
!6 = !{!"kernel_arg_type_qual", !"", !"", !"", !""}
!7 = !{!"kernel_arg_name", !"", !"", !"", !""}
!8 = !{i32 3, i32 0}
!9 = !{!"custom_metadata", target("spirv.Sampler") zeroinitializer}
