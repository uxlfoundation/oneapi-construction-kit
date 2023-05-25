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

; RUN: muxc --device "%default_device" --passes add-fp-control,verify -S %s  \
; RUN:   | FileCheck --check-prefix NOFTZ %s
; RUN: muxc --device "%default_device" --passes "add-fp-control<ftz>,verify" -S %s  \
; RUN:   | FileCheck --check-prefix FTZ %s

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "aarch64-linux-gnu-elf"

; FTZ: define internal spir_kernel void @add(ptr addrspace(1) readonly %in1, ptr addrspace(1) readonly %in2, ptr addrspace(1) %out, i8 signext %i) [[ATTRS:#[0-9]+]] !test [[MD:\![0-9]+]] {

; Check that we've copied function and parameter attributes over, as well as
; parameter names for readability
; Check that we carry over all metadata, but steal the entry point metadata
; FTZ: define spir_kernel void @baz.host-fp-control(ptr addrspace(1) readonly %in1, ptr addrspace(1) readonly %in2, ptr addrspace(1) %out, i8 signext %i) [[WRAPPER_ATTRS:#[0-9]+]] !test [[MD]] {

; FTZ: mrs $0, fpcr
; FTZ: 16777216
; FTZ: msr fpcr, $0", "r"
; Check that we call the original function with the correct parameter attributes
; FTZ: call spir_kernel void @add(ptr addrspace(1) readonly %in1, ptr addrspace(1) readonly %in2, ptr addrspace(1) %out, i8 signext %i)

; NOFTZ-NOT: mrs $0, fpcr
; NOFTZ-NOT: 16777216
; NOFTZ-NOT: msr fpcr, $0", "r"

define spir_kernel void @add(float addrspace(1)* readonly %in1, float addrspace(1)* readonly %in2, float addrspace(1)* %out, i8 signext %i) #0 !test !0 {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %in1, i64 %call
  %0 = load float, float addrspace(1)* %arrayidx, align 4
  %arrayidx2 = getelementptr inbounds float, float addrspace(1)* %in2, i64 %call
  %1 = load float, float addrspace(1)* %arrayidx2, align 4
  %add = fadd float %0, %1
  %arrayidx4 = getelementptr inbounds float, float addrspace(1)* %out, i64 %call
  store float %add, float addrspace(1)* %arrayidx4, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32 %x)

; check that we preserve the attributes on the old function, but add 'alwaysinline'
; FTZ-DAG: attributes [[ATTRS]] = { alwaysinline "foo"="bar" "mux-base-fn-name"="baz" }

; check that we carry over all attributes
; FTZ-DAG: attributes [[WRAPPER_ATTRS]] = { nounwind "foo"="bar" "mux-base-fn-name"="baz" "mux-kernel"="entry-point" }

; FTZ-DAG: [[MD]] = !{!"test"}

attributes #0 = { "foo"="bar" "mux-base-fn-name"="baz" "mux-kernel"="entry-point" }

!0 = !{!"test"}
