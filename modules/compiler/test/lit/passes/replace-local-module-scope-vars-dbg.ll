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

; RUN: muxc --passes replace-module-scope-vars,verify -S %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; This global has no !dbg but it recorded in the debug metadata's globals
; section. Ensure we don't crash.
@a = internal addrspace(3) global i32 poison, align 4

define spir_kernel void @func() #0 {
  %ld = load i32, i32 addrspace(3)* @a, align 4
  ret void
}

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!8}

attributes #0 = { "mux-kernel"="entry-point" }

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, globals: !2, runtimeVersion: 1)
!1 = !DIFile(filename: "foo.ll", directory: "/tmp")
!2 = !{!3}
!3 = !DIGlobalVariableExpression(var: !4, expr: !DIExpression(DW_OP_constu, 3, DW_OP_swap, DW_OP_xderef))
!4 = distinct !DIGlobalVariable(name: "a", scope: !5, file: !1, line: 11, type: !7, isLocal: true, isDefinition: true)
!5 = distinct !DISubprogram(name: "func", scope: !1, file: !1, flags: DIFlagPrototyped, unit: !0)
!7 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!8 = !{i32 1, !"Debug Info Version", i32 3}
