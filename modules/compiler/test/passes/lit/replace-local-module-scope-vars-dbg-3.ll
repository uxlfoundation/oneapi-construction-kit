; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes replace-module-scope-vars,verify -S %s -opaque-pointers

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

@a = internal addrspace(3) global i32 undef, align 4, !dbg !8
@b = internal addrspace(3) global i32 undef, align 4, !dbg !9

define spir_kernel void @func() #0 {
  %ld.a = load i32, ptr addrspace(3) @a, align 4
  %ld.b = load i32, ptr addrspace(3) @b, align 4
  ret void
}

; This program has two compile units - ensure we wipe the record of globals from each of them.
!llvm.dbg.cu = !{!4, !5}
!llvm.module.flags = !{!0}

attributes #0 = { "mux-kernel"="entry-point" }

; CHECK: distinct !DICompileUnit(language: DW_LANG_C99, file: !{{.*}},
; CHECK-SAME:  isOptimized: false, runtimeVersion: 1, emissionKind: NoDebug, globals: [[GLOBALS:\![0-9]+]])
; CHECK-DAG: [[GLOBALS]] = !{}
; CHECK-DAG: distinct !DICompileUnit(language: DW_LANG_C99, file: !{{.*}},
; CHECK-SAME: isOptimized: false, runtimeVersion: 1, emissionKind: NoDebug, globals: [[GLOBALS]]

!0 = !{i32 1, !"Debug Info Version", i32 3}

!1 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)

!2 = !DIFile(filename: "foo.ll", directory: "/tmp")
!3 = !DIFile(filename: "bar.ll", directory: "/tmp")

!4 = distinct !DICompileUnit(language: DW_LANG_C99, file: !2, globals: !6, runtimeVersion: 1)
!5 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, globals: !7, runtimeVersion: 1)

!6 = !{!8}
!7 = !{!9}


!8 = !DIGlobalVariableExpression(var: !10, expr: !DIExpression(DW_OP_constu, 3, DW_OP_swap, DW_OP_xderef))
!9 = !DIGlobalVariableExpression(var: !11, expr: !DIExpression(DW_OP_constu, 2, DW_OP_swap, DW_OP_xderef))

!10 = distinct !DIGlobalVariable(name: "a", scope: !4, file: !2, line: 11, type: !1, isLocal: true, isDefinition: true)
!11 = distinct !DIGlobalVariable(name: "b", scope: !5, file: !3, line: 10, type: !1, isLocal: false, isDefinition: true)
