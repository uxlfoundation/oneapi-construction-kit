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

; Test Brief:
; Check DI intrinsics are correct when __local address space variables are
; present in both a kernel being enqueued by a user and a callee kernel. A
; single struct containing these variables is created internally and the debug
; info entries should reference into that struct but retain the original
; function's scope.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: muxc --passes "replace-module-scope-vars,verify" -S %s | FileCheck %t

; TODO: It's not clear why @helper_kernel has one dbg.declare but @local_array
; has two. See CA-4534.

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

@my_constant = private unnamed_addr addrspace(2) constant i32 42, align 4, !dbg !0
@helper_kernel.data = internal addrspace(3) global [4 x i32] undef, align 4, !dbg !38
@local_array.cache = internal addrspace(3) global [4 x i32] undef, align 4, !dbg !50

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

; CHECK: define void @helper_kernel.mux-local-var-wrapper(
; CHECK-GE19: #dbg_declare({{(ptr|%localVarTypes\*)}} %{{[0-9]+}}
; CHECK-LT19: call void @llvm.dbg.declare(metadata {{(ptr|%localVarTypes\*)}} %{{[0-9]+}}
; CHECK-SAME: [[HK_DI_CACHE_VAR:![0-9]+]],
; CHECK-SAME: !DIExpression(DW_OP_plus_uconst, 0)
; CHECK-SAME: [[HK_DI_CACHE_LOCATION:![0-9]+]]

; Function Attrs: nounwind
define void @helper_kernel() #1 !dbg !27 {
  %addrx = getelementptr [4 x i32], [4 x i32] addrspace(3)* @helper_kernel.data, i32 0, i32 0
  %x = load i32, i32 addrspace(3)* %addrx, align 4
  ret void
}

; CHECK: define void @local_array.mux-local-var-wrapper(
; CHECK-GE19: #dbg_declare({{(ptr|%localVarTypes\*)}} %{{[0-9]+}},
; CHECK-LT19: call void @llvm.dbg.declare(metadata {{(ptr|%localVarTypes\*)}} %{{[0-9]+}},
; CHECK-SAME: [[LA_DI_DATA_VAR:![0-9]+]],
; CHECK-SAME: !DIExpression(DW_OP_plus_uconst, 0)
; CHECK-SAME: [[LA_DI_DATA_LOCATION:![0-9]+]]

; CHECK-GE19: #dbg_declare({{(ptr|%localVarTypes\*)}} %{{[0-9]+}},
; CHECK-LT19: call void @llvm.dbg.declare(metadata {{(ptr|%localVarTypes\*)}} %{{[0-9]+}},
; CHECK-SAME: [[LA_DI_CACHE_VAR:![0-9]+]],
; CHECK-SAME: !DIExpression(DW_OP_plus_uconst, 16)
; CHECK-SAME: [[LA_DI_CACHE_LOCATION:![0-9]+]]

; Function Attrs: nounwind
define void @local_array() #1 !dbg !34 {
  %addry = getelementptr [4 x i32], [4 x i32] addrspace(3)* @local_array.cache, i32 0, i32 0
  %y = load i32, i32 addrspace(3)* %addry, align 4
  ret void
}

; CHECK: [[DI_CONSTANT_EXPR:![0-9]+]] = !DIGlobalVariableExpression(var: [[DI_CONSTANT_VAR:![0-9]+]]
; CHECK: [[DI_CONSTANT_VAR]] = distinct !DIGlobalVariable(name: "my_constant"

; CHECK: [[DI_CU:![0-9]+]] = distinct !DICompileUnit(
; CHECK-SAME: globals: [[DI_GLOBALS:![0-9]+]]
; CHECK: [[DI_GLOBALS]] = !{[[DI_CONSTANT_EXPR]]}

; CHECK-DAG: [[DI_LOCAL_ARRAY_KERNEL:![0-9]+]] = distinct !DISubprogram(name: "local_array"
; CHECK-DAG: [[DI_HELPER_KERNEL:![0-9]+]] = distinct !DISubprogram(name: "helper_kernel"

; CHECK-DAG: [[HK_DI_CACHE_VAR]] = !DILocalVariable(name: "data", scope: [[DI_HELPER_KERNEL]]
; CHECK-DAG: [[HK_DI_CACHE_LOCATION]] = !DILocation(line: 19, scope: [[DI_HELPER_KERNEL]])

; CHECK-DAG: [[LA_DI_CACHE_VAR]] = !DILocalVariable(name: "cache", scope: [[DI_LOCAL_ARRAY_KERNEL]]
; CHECK-DAG: [[LA_DI_CACHE_LOCATION]] = !DILocation(line: 20, scope: [[DI_LOCAL_ARRAY_KERNEL]])

; CHECK-DAG: [[LA_DI_DATA_VAR]] = !DILocalVariable(name: "data", scope: [[DI_LOCAL_ARRAY_KERNEL]]
; CHECK-DAG: [[LA_DI_DATA_LOCATION]] = !DILocation(line: 19, scope: [[DI_LOCAL_ARRAY_KERNEL]]


attributes #0 = { nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #1 = { nounwind "mux-kernel"="entry-point" }

!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!7, !8}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression(DW_OP_constu, 2, DW_OP_swap, DW_OP_xderef))
!1 = distinct !DIGlobalVariable(name: "my_constant", scope: !2, file: !5, line: 15, type: !6, isLocal: false, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "clang version 14.0.3 (2e6de5cbfebb9c96c26fd7d9eb3763c83a1f9f77)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, globals: !4)
!3 = !DIFile(filename: "<stdin>", directory: "/ComputeAorta")
!4 = !{!0, !38, !50}
!5 = !DIFile(filename: "kernel.opencl", directory: "/ComputeAorta")
!6 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!7 = !{i32 2, !"Debug Info Version", i32 3}
!8 = !{i32 1, !"wchar_size", i32 4}
!27 = distinct !DISubprogram(name: "helper_kernel", scope: !5, file: !5, line: 18, type: !28, scopeLine: 18, flags: DIFlagArtificial | DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !31)
!28 = !DISubroutineType(cc: DW_CC_LLVM_OpenCLKernel, types: !29)
!29 = !{null, !30}
!30 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !6, size: 64, dwarfAddressSpace: 1)
!31 = !{}
!33 = !{i32 0, i32 0}
!34 = distinct !DISubprogram(name: "local_array", scope: !5, file: !5, line: 29, type: !35, scopeLine: 29, flags: DIFlagArtificial | DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !31)
!35 = !DISubroutineType(cc: DW_CC_LLVM_OpenCLKernel, types: !36)
!36 = !{null, !30, !30}
!37 = !{i32 0, i32 0}

!38 = !DIGlobalVariableExpression(var: !39, expr: !DIExpression(DW_OP_constu, 3, DW_OP_swap, DW_OP_xderef))
!39 = distinct !DIGlobalVariable(name: "data", scope: !40, file: !5, line: 19, type: !47, isLocal: true, isDefinition: true)
!40 = distinct !DISubprogram(name: "helper_kernel", scope: !41, file: !5, line: 18, type: !42, scopeLine: 18, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !46)
!41 = !DIFile(filename: "kernel.opencl", directory: "/ComputeAorta")
!42 = !DISubroutineType(cc: DW_CC_LLVM_OpenCLKernel, types: !43)
!43 = !{null, !44}
!44 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !45, size: 64, dwarfAddressSpace: 1)
!45 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!46 = !{}
!47 = !DICompositeType(tag: DW_TAG_array_type, baseType: !45, size: 128, elements: !48)
!48 = !{!49}
!49 = !DISubrange(count: 4)

!50 = !DIGlobalVariableExpression(var: !51, expr: !DIExpression(DW_OP_constu, 3, DW_OP_swap, DW_OP_xderef))
!51 = distinct !DIGlobalVariable(name: "cache", scope: !34, file: !5, line: 20, type: !59, isLocal: true, isDefinition: true)
!57 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!58 = !{}
!59 = !DICompositeType(tag: DW_TAG_array_type, baseType: !57, size: 128, elements: !60)
!60 = !{!61}
!61 = !DISubrange(count: 4)
