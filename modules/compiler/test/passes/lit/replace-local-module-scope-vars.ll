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

; RUN: muxc --passes replace-module-scope-vars,verify -S %s | FileCheck %s

; Check that wrapped kernels are given 'alwaysinline' attributes, unless they
; have 'noinline' attributes.

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: %localVarTypes = type { i16, [6 x i8], ptr addrspace(3), i32 }

@a = internal addrspace(3) global i16 undef, align 2
@b = internal addrspace(3) global [4 x float] undef, align 4
@c = internal addrspace(3) global i32 addrspace(3)* undef
@d = internal addrspace(3) global i32 undef

; CHECK: define internal spir_kernel void @add(ptr addrspace(1) %in, ptr addrspace(1) %out, ptr [[STRUCTPTR:%.*]]) #[[ATTRS:[0-9]+]]
; CHECK: [[GEP:%.*]] = getelementptr inbounds %localVarTypes, ptr [[STRUCTPTR]], i32 0, i32 0
; CHECK: [[ADDR:%.*]] = addrspacecast ptr [[GEP]] to ptr addrspace(3)
; CHECK: %ld = load i16, ptr addrspace(3) [[ADDR]], align 2
; CHECK: [[GEPC:%.*]] = getelementptr inbounds %localVarTypes, ptr [[STRUCTPTR]], i32 0, i32 2
; CHECK: [[ADDRC:%.*]] = addrspacecast ptr [[GEPC]] to ptr addrspace(3)
; CHECK: [[GEPD:%.*]] = getelementptr inbounds %localVarTypes, ptr [[STRUCTPTR]], i32 0, i32 3
; CHECK: [[ADDRD:%.*]] = addrspacecast ptr [[GEPD]] to ptr addrspace(3)
; CHECK: %val = cmpxchg ptr addrspace(3) [[ADDRC]], ptr addrspace(3) [[ADDRD]], ptr addrspace(3) [[ADDRD]] acq_rel monotonic

; CHECK: ret void
; CHECK: }


; CHECK: define spir_kernel void @foo.mux-local-var-wrapper(ptr addrspace(1) [[ARG0:%.*]], ptr addrspace(1) [[ARG1:%.*]]) #[[WRAPPER_ATTRS:[0-9]+]]
; The alignment of this alloca must be the maximum alignment of the new struct
; CHECK: [[ALLOCA:%.*]] = alloca %localVarTypes, align 8
; CHECK: call spir_kernel void @add(ptr addrspace(1) [[ARG0]], ptr addrspace(1) [[ARG1]], ptr [[ALLOCA]])
define spir_kernel void @add(i32 addrspace(1)* %in, i32 addrspace(1)* %out) #0 {
  %ld = load i16, i16 addrspace(3)* @a, align 2
  %val = cmpxchg i32 addrspace(3)* addrspace(3)* @c, i32 addrspace(3)* @d, i32 addrspace(3)* @d acq_rel monotonic
  ret void
}

; Check we haven't added alwaysinline, given that the kernel is marked
; noinline.
; Check also that we've preserved the original function name attribute
; CHECK: attributes #[[ATTRS]] = { noinline "mux-base-fn-name"="foo" }
; CHECK: attributes #[[WRAPPER_ATTRS]] = { noinline nounwind "mux-base-fn-name"="foo" "mux-kernel"="entry-point" }

attributes #0 = { noinline "mux-base-fn-name"="foo" "mux-kernel"="entry-point" }
