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
; RUN: muxc --passes replace-target-ext-tys,verify %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @test_kernel(ptr addrspace(1) nocapture readonly align 4 %in, ptr addrspace(1) nocapture writeonly align 4 %out) !dbg !5 {
entry:
  call void @llvm.dbg.value(metadata ptr addrspace(1) %in, metadata !12, metadata !DIExpression(DW_OP_constu, 0, DW_OP_swap, DW_OP_xderef)), !dbg !14
  call void @llvm.dbg.value(metadata ptr addrspace(1) %out, metadata !13, metadata !DIExpression(DW_OP_constu, 0, DW_OP_swap, DW_OP_xderef)), !dbg !14
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

; Check we've remapped debug info in-place and that we haven't introduced any
; new DICompileUnits (especially not those orphaned from !llvm.dbg.cu)
; CHECK: !llvm.dbg.cu = !{!0}
; CHECK: !0 = distinct !DICompileUnit
; CHECK-NOT: DICompileUnit

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3}
!opencl.ocl.version = !{!4}
!opencl.spir.version = !{!4}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 17.0.0", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "<stdin>", directory: "/tmp")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = !{i32 1, !"wchar_size", i32 4}
!4 = !{i32 1, i32 2}
!5 = distinct !DISubprogram(name: "test_kernel", scope: !6, file: !6, line: 34, scopeLine: 34, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !11)
!6 = !DIFile(filename: "kernel.opencl", directory: "/tmp")
!9 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !10, size: 64, dwarfAddressSpace: 1)
!10 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!11 = !{!12, !13}
!12 = !DILocalVariable(name: "in", arg: 1, scope: !5, file: !6, line: 34, type: !9)
!13 = !DILocalVariable(name: "out", arg: 2, scope: !5, file: !6, line: 34, type: !9)
!14 = !DILocation(line: 0, scope: !5)
