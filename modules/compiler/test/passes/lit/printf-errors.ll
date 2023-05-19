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

; Unconditionally use opaque pointers to keep the IR simpler
; RUN: %muxc --passes replace-printf,verify -S %s  2>&1 | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

@invalid_length_modifier_z = private unnamed_addr addrspace(2) constant [9 x i8] c"id = %zu\00", align 1
@vector_without_length = private unnamed_addr addrspace(2) constant [9 x i8] c"v = %v2i\00", align 1
@ran_off_end_1 = private unnamed_addr addrspace(2) constant [6 x i8] c"v = %\00", align 1
@invalid_vec_length_1 = private unnamed_addr addrspace(2) constant [11 x i8] c"v = %v7hld\00", align 1
@invalid_vec_length_2 = private unnamed_addr addrspace(2) constant [12 x i8] c"v = %v17hld\00", align 1
@invalid_length_modifier_hl = private unnamed_addr addrspace(2) constant [10 x i8] c"id = %hld\00", align 1
@n_conversion_specifier = private unnamed_addr addrspace(2) constant [3 x i8] c"%n\00", align 1
@l_followed_by_c = private unnamed_addr addrspace(2) constant [4 x i8] c"%lc\00", align 1
@asterisk_width = private unnamed_addr addrspace(2) constant [4 x i8] c"%*d\00", align 1

; CHECK-DAG: warning: /tmp/printf.cl:3:0: invalid length modifier 'z'
; CHECK-DAG: warning: /tmp/printf.cl:4:0: vector specifiers must be supplied length modifiers
; CHECK-DAG: warning: /tmp/printf.cl:5:0: unexpected end of format string
; CHECK-DAG: warning: /tmp/printf.cl:6:0: invalid vector length modifier '7'
; CHECK-DAG: warning: /tmp/printf.cl:7:0: invalid vector length modifier '17'
; CHECK-DAG: warning: /tmp/printf.cl:8:0: the 'hl' length modifier may only be used with the vector specifier
; CHECK-DAG: warning: /tmp/printf.cl:9:0: the 'n' conversion specifier is not supported by OpenCL C but is reserved
; CHECK-DAG: warning: /tmp/printf.cl:10:0: the 'l' length modifier followed by a 'c' conversion specifier or 's' conversion specifier is not supported by OpenCL C
; CHECK-DAG: warning: /tmp/printf.cl:11:0: the '*' width sub-specifier is not supported
define spir_kernel void @do_printf(ptr addrspace(1) %a) {
entry:
  %id = call spir_func i64 @_Z13get_global_idj(i32 0)
  %c0 = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @invalid_length_modifier_z, i64 zeroinitializer), !dbg !9
  %c1 = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @vector_without_length, <2 x i32> zeroinitializer), !dbg !10
  %c2 = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @ran_off_end_1, i32 zeroinitializer), !dbg !11
  %c3 = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @invalid_vec_length_1, <7 x i32> zeroinitializer), !dbg !12
  %c4 = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @invalid_vec_length_2, <17 x i32> zeroinitializer), !dbg !13
  %c5 = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @invalid_length_modifier_hl, i32 zeroinitializer), !dbg !14
  %c6 = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @n_conversion_specifier, ptr zeroinitializer), !dbg !15
  %c7 = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @l_followed_by_c, i32 zeroinitializer), !dbg !16
  %c8 = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @asterisk_width, i32 4, i32 zeroinitializer), !dbg !17
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

declare spir_func i32 @printf(ptr addrspace(2), ...)

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !4, isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!2 = !{i32 2, !"Debug Info Version", i32 3}

!3 = distinct !DISubprogram(name: "do_printf", scope: !4, file: !4, line: 1, type: !5, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0)
!4 = !DIFile(filename: "/tmp/printf.cl", directory: "")
!5 = !DISubroutineType(cc: DW_CC_LLVM_OpenCLKernel, types: !6)
!6 = !{null, !7}
!7 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !8, size: 64, dwarfAddressSpace: 1)
!8 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)

!9 = !DILocation(line: 3, scope: !3)
!10 = !DILocation(line: 4, scope: !3)
!11 = !DILocation(line: 5, scope: !3)
!12 = !DILocation(line: 6, scope: !3)
!13 = !DILocation(line: 7, scope: !3)
!14 = !DILocation(line: 8, scope: !3)
!15 = !DILocation(line: 9, scope: !3)
!16 = !DILocation(line: 10, scope: !3)
!17 = !DILocation(line: 11, scope: !3)
