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

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: muxc --passes "work-item-loops<debug>,verify" -S %s | FileCheck %t

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; Function Attrs: norecurse nounwind
define void @barrier_test(i32 addrspace(1)* %input, i32 addrspace(1)* %output) #0 !dbg !13 {
entry:
  %input.addr = alloca i32 addrspace(1)*, align 8
  %output.addr = alloca i32 addrspace(1)*, align 8
  %global_id = alloca i64, align 8
  %local_id = alloca i64, align 8
  %multiplied = alloca i64, align 8
  %result = alloca i32, align 4
  store i32 addrspace(1)* %input, i32 addrspace(1)** %input.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %input.addr, metadata !24, metadata !DIExpression(DW_OP_constu, 0, DW_OP_swap, DW_OP_xderef)), !dbg !25
  store i32 addrspace(1)* %output, i32 addrspace(1)** %output.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %output.addr, metadata !26, metadata !DIExpression(DW_OP_constu, 0, DW_OP_swap, DW_OP_xderef)), !dbg !25
  call void @llvm.dbg.declare(metadata i64* %global_id, metadata !27, metadata !DIExpression(DW_OP_constu, 0, DW_OP_swap, DW_OP_xderef)), !dbg !32
  %call = call i64 @__mux_get_global_id(i32 0) #5, !dbg !32, !range !33
  store i64 %call, i64* %global_id, align 8, !dbg !32
  call void @llvm.dbg.declare(metadata i64* %local_id, metadata !34, metadata !DIExpression(DW_OP_constu, 0, DW_OP_swap, DW_OP_xderef)), !dbg !35
  %call1 = call i64 @__mux_get_local_id(i32 0) #5, !dbg !35, !range !33
  store i64 %call1, i64* %local_id, align 8, !dbg !35
  %0 = load i64, i64* %global_id, align 8, !dbg !36
  %call2 = call i64 @__mux_get_global_size(i32 0) #5, !dbg !36, !range !38
  %cmp = icmp ult i64 %0, %call2, !dbg !36
  br i1 %cmp, label %if.then, label %if.end, !dbg !39

if.then:                                          ; preds = %entry
  call void @llvm.dbg.declare(metadata i64* %multiplied, metadata !40, metadata !DIExpression(DW_OP_constu, 0, DW_OP_swap, DW_OP_xderef)), !dbg !42
  %1 = load i64, i64* %global_id, align 8, !dbg !42
  %2 = load i64, i64* %local_id, align 8, !dbg !42
  %mul = mul i64 %1, %2, !dbg !42
  store i64 %mul, i64* %multiplied, align 8, !dbg !42
  call void @__mux_work_group_barrier(i32 0, i32 1, i32 272), !dbg !43
  call void @llvm.dbg.declare(metadata i32* %result, metadata !44, metadata !DIExpression(DW_OP_constu, 0, DW_OP_swap, DW_OP_xderef)), !dbg !45
  %3 = load i64, i64* %multiplied, align 8, !dbg !45
  %4 = load i32 addrspace(1)*, i32 addrspace(1)** %input.addr, align 8, !dbg !45
  %5 = load i64, i64* %global_id, align 8, !dbg !45
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %4, i64 %5, !dbg !45
  %6 = load i32, i32 addrspace(1)* %arrayidx, align 4, !dbg !45
  %conv = sext i32 %6 to i64, !dbg !45
  %add = add i64 %3, %conv, !dbg !45
  %conv3 = trunc i64 %add to i32, !dbg !45
  store i32 %conv3, i32* %result, align 4, !dbg !45
  %7 = load i32, i32* %result, align 4, !dbg !46
  %8 = load i32 addrspace(1)*, i32 addrspace(1)** %output.addr, align 8, !dbg !46
  %9 = load i64, i64* %global_id, align 8, !dbg !46
  %arrayidx4 = getelementptr inbounds i32, i32 addrspace(1)* %8, i64 %9, !dbg !46
  store i32 %7, i32 addrspace(1)* %arrayidx4, align 4, !dbg !46
  br label %if.end, !dbg !47

if.end:                                           ; preds = %if.then, %entry
  ret void, !dbg !48
}

; Stub function called on barrier entry for debugging
; CHECK: Function Attrs: noinline
; CHECK-NEXT: void @__barrier_entry(i32{{( %0)?}}) {{.*}} !dbg [[DI_FUNC_ENTRY:![0-9]+]]
; CHECK-NEXT: entry:
; CHECK-NEXT:  ret void
; CHECK-NEXT: }
;
; Stub function called on barrier exit for debugging
; CHECK: Function Attrs: noinline
; CHECK-NEXT: void @__barrier_exit(i32{{( %0)?}}) {{.*}} !dbg [[DI_FUNC_EXIT:![0-9]+]]
; CHECK-NEXT: entry:
; CHECK-NEXT:  ret void
; CHECK-NEXT: }
;
; First function created from splitting kernel, check inlined
; CHECK: Function Attrs: alwaysinline
; CHECK-NEXT: i32 @barrier_test.mux-barrier-region({{.*}} {
;
; Test all the variables are loaded from the live vars struct, important to assert that there are no allocas left.
; Contains the 4 variables defined before the barrier and 'result' defined after, there is also a padding element in the struct.
; CHECK-DAG: %live_gep_input.addr = getelementptr inbounds %barrier_test_live_mem_info, ptr %2, i32 0, i32 {{[0-5]}}
; CHECK-DAG: %live_gep_output.addr = getelementptr inbounds %barrier_test_live_mem_info, ptr %2, i32 0, i32 {{[0-5]}}
; CHECK-DAG: %live_gep_global_id = getelementptr inbounds %barrier_test_live_mem_info, ptr %2, i32 0, i32 {{[0-5]}}
; CHECK-DAG: %live_gep_local_id = getelementptr inbounds %barrier_test_live_mem_info, ptr %2, i32 0, i32 {{[0-5]}}
; CHECK-DAG: %live_gep_multiplied = getelementptr inbounds %barrier_test_live_mem_info, ptr %2, i32 0, i32 {{[0-5]}}
;
; Call debug stub to signify entry to barrier
; CHECK: call void @__barrier_entry(i32 0), !dbg [[DI_LOC_B1:![0-9]+]]
;
; Second function created from splitting kernel, check inlined
; CHECK: Function Attrs: alwaysinline
; CHECK-NEXT: i32 @barrier_test.mux-barrier-region.1({{.*}} {
;
; Call debug stub to signify exit from barrier
; CHECK-DAG: call {{.*}}void @__barrier_exit(i32 0), !dbg [[DI_LOC_B2:![0-9]+]]
;
; Load values in to kernel from live variable struct, don't need to load 'local_id' since it's not live any more
; These are all DAG checks because it doesn't really matter if the barrier exit stub comes before or after them.
; CHECK-DAG: getelementptr inbounds %barrier_test_live_mem_info, ptr %2, i32 0, i32 {{[0-5]}}
; CHECK-DAG: getelementptr inbounds %barrier_test_live_mem_info, ptr %2, i32 0, i32 {{[0-5]}}
; CHECK-DAG: getelementptr inbounds %barrier_test_live_mem_info, ptr %2, i32 0, i32 {{[0-5]}}
; CHECK-DAG: getelementptr inbounds %barrier_test_live_mem_info, ptr %2, i32 0, i32 {{[0-5]}}
; CHECK-DAG: getelementptr inbounds %barrier_test_live_mem_info, ptr %2, i32 0, i32 {{[0-5]}}
;
; Assert the DI entry is attached to the correct function
; CHECK: void @barrier_test.mux-barrier-wrapper(ptr addrspace(1) %input, ptr addrspace(1) %output) {{.*}} !dbg [[DI_SUBPROGRAM:![0-9]+]]
;
; Array of live_variables structs, one for each work-item in a work-group
; CHECK: %live_variables{{.*}} = alloca %barrier_test_live_mem_info, {{(i64|i32)}} {{%.*}}, align {{(8|4)}}
; CHECK: [[LIVE_VAR_GEP:%[0-9]+]] = getelementptr inbounds %barrier_test_live_mem_info, ptr %live_variables
;
; Check we have a debug intrinsic with the location of each source variable.
;
; Validate location expressions, we deference the pointer to 'LIVE_VAR_GEP' to
; get the start of the live variable struct, then add an offset into the struct
; for that particular variable. Because our live var struct type contains
; pointers, the offsets will be architecture dependant for 32 & 64 bit devices.
; As a result in this check we only generically assert the offset is a decimal
; number rather than the specific byte offset.
;
; The pass non-determinism also affects the placement of padding elements which
; further complicates the issue since this too impacts the offsets. See CA-119.
; CHECK-GE19: #dbg_value(ptr undef
; CHECK-LT19: call void @llvm.dbg.value(metadata ptr undef
; CHECK-SAME: !DIExpression(DW_OP_deref, DW_OP_plus_uconst, {{0|[1-9][0-9]*}}
;
; CHECK-GE19: #dbg_value(ptr undef
; CHECK-LT19: call void @llvm.dbg.value(metadata ptr undef
; CHECK-SAME: !DIExpression(DW_OP_deref, DW_OP_plus_uconst, {{0|[1-9][0-9]*}}
;
; CHECK-GE19: #dbg_value(ptr undef
; CHECK-LT19: call void @llvm.dbg.value(metadata ptr undef
; CHECK-SAME: !DIExpression(DW_OP_deref, DW_OP_plus_uconst, {{0|[1-9][0-9]*}}
;
; CHECK-GE19: #dbg_value(ptr undef
; CHECK-LT19: call void @llvm.dbg.value(metadata ptr undef
; CHECK-SAME: !DIExpression(DW_OP_deref, DW_OP_plus_uconst, {{0|[1-9][0-9]*}}
;
; CHECK-GE19: #dbg_value(ptr undef
; CHECK-LT19: call void @llvm.dbg.value(metadata ptr undef
; CHECK-SAME: !DIExpression(DW_OP_deref, DW_OP_plus_uconst, {{0|[1-9][0-9]*}}
;
; CHECK-GE19: #dbg_value(ptr undef
; CHECK-LT19: call void @llvm.dbg.value(metadata ptr undef
; CHECK-SAME: !DIExpression(DW_OP_deref, DW_OP_plus_uconst, {{0|[1-9][0-9]*}}
;
; Debug info for first kernel scope
; CHECK-DAG: [[DI_FUNC_ENTRY]] = distinct !DISubprogram(name: "__barrier_entry", linkageName: "__barrier_entry",{{.*}}
; CHECK-DAG: [[DI_FUNC_EXIT]] = distinct !DISubprogram(name: "__barrier_exit", linkageName: "__barrier_exit",{{.*}}

; This looks like a bug - should we really be referencing the old DISubprogram?
; See CA-4531.
; CHECK-DAG: [[OLD_DI_SUBPROGRAM:![0-9]+]] = distinct !DISubprogram({{.*}},
;
; Check the list of variables for the subprogram
; CHECK-DAG: [[DI_SUBPROGRAM]] = distinct !DISubprogram({{.*}},
; CHECK-DAG: !DILocalVariable(name: "input", {{(arg: 1, )?}}scope: [[DI_SUBPROGRAM]], file: !{{.*}}, line: 11
; CHECK-DAG: !DILocalVariable(name: "output", {{(arg: 2, )?}}scope: [[DI_SUBPROGRAM]], file: !{{.*}}, line: 11,
; CHECK-DAG: !DILocalVariable(name: "global_id", scope: [[DI_SUBPROGRAM]], file: !{{.*}}, line: 12
; CHECK-DAG: !DILocalVariable(name: "local_id", scope: [[DI_SUBPROGRAM]], file: !{{.*}}, line: 13
; CHECK-DAG: !DILocalVariable(name: "multiplied", scope: [[DI_SUBPROGRAM]], file: !{{.*}}, line: 17
; CHECK-DAG: !DILocalVariable(name: "result", scope: [[DI_SUBPROGRAM]], file: !{{.*}}, line: 21
;
; CHECK-DAG: [[IF_SCOPE:![0-9]+]] = distinct !DILexicalBlock(scope: [[OLD_DI_SUBPROGRAM]], file: !{{.*}}, line: 15)
; CHECK-DAG: [[NESTED_SCOPE:![0-9]+]] = distinct !DILexicalBlock(scope: [[IF_SCOPE:![0-9]+]], file: !{{.*}}, line: 16)
;
; Check the locations for the variables
; CHECK-DAG: !DILocation(line: 12, scope: [[OLD_DI_SUBPROGRAM]])
; CHECK-DAG: !DILocation(line: 13, scope: [[OLD_DI_SUBPROGRAM]])
; CHECK-DAG: !DILocation(line: 17, scope: [[NESTED_SCOPE_2:![0-9]+]])
; CHECK-DAG: !DILocation(line: 19, scope: [[NESTED_SCOPE_2]])
; CHECK-DAG: !DILocation(line: 21, scope: [[NESTED_SCOPE_2]])
; CHECK-DAG: !DILocation(line: 22, scope: [[NESTED_SCOPE_2]])

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: convergent nounwind
declare void @__mux_work_group_barrier(i32, i32, i32) #3

; Function Attrs: convergent mustprogress nofree nounwind readonly willreturn
declare i64 @__mux_get_global_id(i32 ) #4

; Function Attrs: convergent mustprogress nofree nounwind readonly willreturn
declare i64 @__mux_get_global_size(i32 ) #4

; Function Attrs: convergent mustprogress nofree nounwind readonly willreturn
declare i64 @__mux_get_local_id(i32 ) #4

declare i64 @__mux_get_local_size(i32)

declare i64 @__mux_get_num_groups(i32)

declare i64 @__mux_get_group_id(i32)

declare i64 @__mux_get_global_offset(i32)

declare void @__mux_set_local_id(i32, i64)

attributes #0 = { norecurse nounwind "frame-pointer"="none" "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="0" "stackrealign" "uniform-work-group-size"="true" "mux-kernel"="entry-point" }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { convergent mustprogress nofree norecurse nounwind readonly willreturn "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "stackrealign" }
attributes #3 = { convergent nounwind "frame-pointer"="none" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="0" "stackrealign" }
attributes #4 = { convergent mustprogress nofree nounwind readonly willreturn "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "stackrealign" }
attributes #5 = { nobuiltin nounwind readonly willreturn "no-builtins" }
attributes #6 = { convergent nounwind readonly willreturn }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3}
!opencl.ocl.version = !{!4}
!opencl.spir.version = !{!4}
!opencl.kernels = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "<stdin>", directory: "test")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = !{i32 1, !"wchar_size", i32 4}
!4 = !{i32 1, i32 2}
!5 = !{void (i32 addrspace(1)*, i32 addrspace(1)*)* @barrier_test, !6, !7, !8, !9, !10, !11}
!6 = !{!"kernel_arg_addr_space", i32 1, i32 1}
!7 = !{!"kernel_arg_access_qual", !"none", !"none"}
!8 = !{!"kernel_arg_type", !"int*", !"int*"}
!9 = !{!"kernel_arg_base_type", !"int*", !"int*"}
!10 = !{!"kernel_arg_type_qual", !"const", !""}
!11 = !{!"kernel_arg_name", !"input", !"output"}
!12 = !{!""}
!13 = distinct !DISubprogram(name: "barrier_test", scope: !14, file: !14, line: 11, type: !15, scopeLine: 11, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !21)
!14 = !DIFile(filename: "kernel.opencl", directory: "test")
!15 = !DISubroutineType(cc: DW_CC_LLVM_OpenCLKernel, types: !16)
!16 = !{null, !17, !20}
!17 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !18, size: 64, dwarfAddressSpace: 1)
!18 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !19)
!19 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!20 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !19, size: 64, dwarfAddressSpace: 1)
!21 = !{}
!24 = !DILocalVariable(name: "input", arg: 1, scope: !13, file: !14, line: 11, type: !17)
!25 = !DILocation(line: 11, scope: !13)
!26 = !DILocalVariable(name: "output", arg: 2, scope: !13, file: !14, line: 11, type: !20)
!27 = !DILocalVariable(name: "global_id", scope: !13, file: !14, line: 12, type: !28)
!28 = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !29, line: 51, baseType: !30)
!29 = !DIFile(filename: "builtins.h", directory: "test")
!30 = !DIDerivedType(tag: DW_TAG_typedef, name: "ulong", file: !29, line: 49, baseType: !31)
!31 = !DIBasicType(name: "unsigned long", size: 64, encoding: DW_ATE_unsigned)
!32 = !DILocation(line: 12, scope: !13)
!33 = !{i64 0, i64 1024}
!34 = !DILocalVariable(name: "local_id", scope: !13, file: !14, line: 13, type: !28)
!35 = !DILocation(line: 13, scope: !13)
!36 = !DILocation(line: 15, scope: !37)
!37 = distinct !DILexicalBlock(scope: !13, file: !14, line: 15)
!38 = !{i64 1, i64 1025}
!39 = !DILocation(line: 15, scope: !13)
!40 = !DILocalVariable(name: "multiplied", scope: !41, file: !14, line: 17, type: !28)
!41 = distinct !DILexicalBlock(scope: !37, file: !14, line: 16)
!42 = !DILocation(line: 17, scope: !41)
!43 = !DILocation(line: 19, scope: !41)
!44 = !DILocalVariable(name: "result", scope: !41, file: !14, line: 21, type: !19)
!45 = !DILocation(line: 21, scope: !41)
!46 = !DILocation(line: 22, scope: !41)
!47 = !DILocation(line: 23, scope: !41)
!48 = !DILocation(line: 24, scope: !13)
