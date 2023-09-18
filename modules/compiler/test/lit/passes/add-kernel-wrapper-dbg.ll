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

; Check that wrapping kernels preserves the old DISubprogram and creates a new
; artificial one, which is used in a debug location for the wrapper call.
; RUN: muxc --passes add-kernel-wrapper,verify < %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: define internal spir_kernel void @foo(ptr addrspace(1) %x, ptr addrspace(1) %y)
; CHECK-SAME: [[ATTRS:#[0-9]+]] !dbg [[SP:\![0-9]+]] {
define spir_kernel void @foo(ptr addrspace(1) %x, ptr addrspace(1) %y) #0 !dbg !10 {
  ret void
}

; CHECK: define spir_kernel void @foo.mux-kernel-wrapper(ptr %packed-args)
; CHECK-SAME:  [[NEW_ATTRS:#[0-9]+]] !dbg [[NEW_SP:\![0-9]+]] !mux_scheduled_fn {{\![0-9]+}} {
; Check that when we call the original kernel we've attached a debug location.
; This is required by LLVM.
; CHECK: call spir_kernel void @foo({{.*}}) [[ATTRS]], !dbg [[LOC:\![0-9]+]]

; CHECK-DAG: attributes [[ATTRS]] = { alwaysinline }
; CHECK-DAG: attributes [[NEW_ATTRS]] = { nounwind "mux-base-fn-name"="foo"  "mux-kernel"="entry-point" }
attributes #0 = { "mux-kernel"="entry-point" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!6}

; The old kernel should not have an 'artificial' sub-program
; CHECK-DAG: [[SP]] = distinct !DISubprogram({{.*}}, flags: DIFlagPrototyped,
; The wrapper kernel should have an 'artificial' sub-program
; CHECK-DAG: [[NEW_SP]] = distinct !DISubprogram({{.*}}, flags: DIFlagArtificial | DIFlagPrototyped,
; The debug location should live in the wrapper's scope.
; CHECK-DAG: [[LOC]] = !DILocation(line: 0, scope: [[NEW_SP]])

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 16.0.4", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "add-kernel-wrapper-dbg.ll", directory: "/tmp")
!2 = !DISubroutineType(cc: DW_CC_LLVM_OpenCLKernel, types: !3)
!3 = !{!4, !4}
!4 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !5, size: 64, dwarfAddressSpace: 1)
!5 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!6 = !{i32 2, !"Debug Info Version", i32 3}

!10 = distinct !DISubprogram(name: "foo", scope: !1, file: !1, line: 2, type: !2, scopeLine: 2, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0)
