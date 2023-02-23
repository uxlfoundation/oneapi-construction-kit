; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %muxc --passes "barriers-pass<debug>,verify" -S %s | %filecheck %t

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; Function Attrs: norecurse nounwind
define void @test_args(i32 addrspace(1)* %in1, <2 x float> addrspace(1)* %in2, i32 %in3, float addrspace(3)* %in4, i32 addrspace(2)* %in5, i32 addrspace(1)* %in6, <2 x i32> addrspace(1)* %in7, <3 x i32> addrspace(1)* %in8, <4 x i32> addrspace(1)* %in9, i32 addrspace(1)* %out) #0 !dbg !13 {
entry:
  %in1.addr = alloca i32 addrspace(1)*, align 8
  %in2.addr = alloca <2 x float> addrspace(1)*, align 8
  %in3.addr = alloca i32, align 4
  %in4.addr = alloca float addrspace(3)*, align 8
  %in5.addr = alloca i32 addrspace(2)*, align 8
  %in6.addr = alloca i32 addrspace(1)*, align 8
  %in7.addr = alloca <2 x i32> addrspace(1)*, align 8
  %in8.addr = alloca <3 x i32> addrspace(1)*, align 8
  %in9.addr = alloca <4 x i32> addrspace(1)*, align 8
  %out.addr = alloca i32 addrspace(1)*, align 8
  store i32 addrspace(1)* %in1, i32 addrspace(1)** %in1.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %in1.addr, metadata !45, metadata !DIExpression(DW_OP_constu, 0, DW_OP_swap, DW_OP_xderef)), !dbg !46
  store <2 x float> addrspace(1)* %in2, <2 x float> addrspace(1)** %in2.addr, align 8
  call void @llvm.dbg.declare(metadata <2 x float> addrspace(1)** %in2.addr, metadata !47, metadata !DIExpression(DW_OP_constu, 0, DW_OP_swap, DW_OP_xderef)), !dbg !48
  store i32 %in3, i32* %in3.addr, align 4
  call void @llvm.dbg.declare(metadata i32* %in3.addr, metadata !49, metadata !DIExpression(DW_OP_constu, 0, DW_OP_swap, DW_OP_xderef)), !dbg !50
  store float addrspace(3)* %in4, float addrspace(3)** %in4.addr, align 8
  call void @llvm.dbg.declare(metadata float addrspace(3)** %in4.addr, metadata !51, metadata !DIExpression(DW_OP_constu, 0, DW_OP_swap, DW_OP_xderef)), !dbg !52
  store i32 addrspace(2)* %in5, i32 addrspace(2)** %in5.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(2)** %in5.addr, metadata !53, metadata !DIExpression(DW_OP_constu, 0, DW_OP_swap, DW_OP_xderef)), !dbg !54
  store i32 addrspace(1)* %in6, i32 addrspace(1)** %in6.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %in6.addr, metadata !55, metadata !DIExpression(DW_OP_constu, 0, DW_OP_swap, DW_OP_xderef)), !dbg !56
  store <2 x i32> addrspace(1)* %in7, <2 x i32> addrspace(1)** %in7.addr, align 8
  call void @llvm.dbg.declare(metadata <2 x i32> addrspace(1)** %in7.addr, metadata !57, metadata !DIExpression(DW_OP_constu, 0, DW_OP_swap, DW_OP_xderef)), !dbg !58
  store <3 x i32> addrspace(1)* %in8, <3 x i32> addrspace(1)** %in8.addr, align 8
  call void @llvm.dbg.declare(metadata <3 x i32> addrspace(1)** %in8.addr, metadata !59, metadata !DIExpression(DW_OP_constu, 0, DW_OP_swap, DW_OP_xderef)), !dbg !60
  store <4 x i32> addrspace(1)* %in9, <4 x i32> addrspace(1)** %in9.addr, align 8
  call void @llvm.dbg.declare(metadata <4 x i32> addrspace(1)** %in9.addr, metadata !61, metadata !DIExpression(DW_OP_constu, 0, DW_OP_swap, DW_OP_xderef)), !dbg !62
  store i32 addrspace(1)* %out, i32 addrspace(1)** %out.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %out.addr, metadata !63, metadata !DIExpression(DW_OP_constu, 0, DW_OP_swap, DW_OP_xderef)), !dbg !64
  %0 = load i32, i32* %in3.addr, align 4, !dbg !65
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %out.addr, align 8, !dbg !65
  %call = call i64 @_Z13get_global_idj(i32 0) #4, !dbg !65, !range !66
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %1, i64 %call, !dbg !65
  store i32 %0, i32 addrspace(1)* %arrayidx, align 4, !dbg !65
  ret void, !dbg !67
}

; the regexes in the following line match extra attributes that clang-level optimizations might add
; CHECK-GE15: void @test_args(ptr addrspace(1) %in1, ptr addrspace(1) %in2, i32 %in3, ptr addrspace(3) %in4, ptr addrspace(2) %in5, ptr addrspace(1) %in6, ptr addrspace(1) %in7, ptr addrspace(1) %in8, ptr addrspace(1) %in9, ptr addrspace(1) %out)
; CHECK-LT15: void @test_args(i32 addrspace(1)* %in1, <2 x float> addrspace(1)* %in2, i32 %in3, float addrspace(3)* %in4, i32 addrspace(2)* %in5, i32 addrspace(1)* %in6, <2 x i32> addrspace(1)* %in7, <3 x i32> addrspace(1)* %in8, <4 x i32> addrspace(1)* %in9, i32 addrspace(1)* %out)

; CHECK: loopIR:
; CHECK: br label %loopIR2

; CHECK: loopIR2:
; CHECK: br label %loopIR3

; CHECK: loopIR3:

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: convergent mustprogress nofree norecurse nounwind readonly willreturn
define internal i64 @_Z13get_global_idj(i32 %x) #2 {
entry:
  %call = tail call i64 @__mux_get_global_id(i32 %x) #5
  ret i64 %call
}

; Function Attrs: convergent mustprogress nofree nounwind readonly willreturn
declare i64 @__mux_get_global_id(i32 ) #3

declare i64 @__mux_get_local_size(i32)

declare i64 @__mux_get_group_id(i32)

declare i64 @__mux_get_local_id(i32)

declare i64 @__mux_get_global_offset(i32)

declare void @__mux_set_local_id(i32, i64)

attributes #0 = { norecurse nounwind "frame-pointer"="none" "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="0" "stackrealign" "uniform-work-group-size"="true" "mux-kernel"="entry-point" }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { convergent mustprogress nofree norecurse nounwind readonly willreturn "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "stackrealign" }
attributes #3 = { convergent mustprogress nofree nounwind readonly willreturn "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "stackrealign" }
attributes #4 = { nobuiltin nounwind readonly willreturn "no-builtins" }
attributes #5 = { convergent nounwind readonly willreturn }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3}
!opencl.ocl.version = !{!4}
!opencl.spir.version = !{!4}
!opencl.kernels = !{!5}
!host.build_options = !{!12}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "<stdin>", directory: "test")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = !{i32 1, !"wchar_size", i32 4}
!4 = !{i32 1, i32 2}
!5 = !{void (i32 addrspace(1)*, <2 x float> addrspace(1)*, i32, float addrspace(3)*, i32 addrspace(2)*, i32 addrspace(1)*, <2 x i32> addrspace(1)*, <3 x i32> addrspace(1)*, <4 x i32> addrspace(1)*, i32 addrspace(1)*)* @test_args, !6, !7, !8, !9, !10, !11}
!6 = !{!"kernel_arg_addr_space", i32 1, i32 1, i32 0, i32 3, i32 2, i32 1, i32 1, i32 1, i32 1, i32 1}
!7 = !{!"kernel_arg_access_qual", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none"}
!8 = !{!"kernel_arg_type", !"int*", !"float2*", !"int", !"float*", !"int*", !"int*", !"int2*", !"int3*", !"int4*", !"int*"}
!9 = !{!"kernel_arg_base_type", !"int*", !"float __attribute__((ext_vector_type(2)))*", !"int", !"float*", !"int*", !"int*", !"int __attribute__((ext_vector_type(2)))*", !"int __attribute__((ext_vector_type(3)))*", !"int __attribute__((ext_vector_type(4)))*", !"int*"}
!10 = !{!"kernel_arg_type_qual", !"", !"", !"", !"", !"const", !"", !"", !"", !"", !""}
!11 = !{!"kernel_arg_name", !"in1", !"in2", !"in3", !"in4", !"in5", !"in6", !"in7", !"in8", !"in9", !"out"}
!12 = !{!""}
!13 = distinct !DISubprogram(name: "test_args", scope: !14, file: !14, line: 9, type: !15, scopeLine: 19, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !42)
!14 = !DIFile(filename: "kernel.opencl", directory: "test")
!15 = !DISubroutineType(cc: DW_CC_LLVM_OpenCLKernel, types: !16)
!16 = !{null, !17, !19, !26, !27, !28, !17, !29, !32, !37, !17}
!17 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !18, size: 64, dwarfAddressSpace: 1)
!18 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!19 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !20, size: 64, dwarfAddressSpace: 1)
!20 = !DIDerivedType(tag: DW_TAG_typedef, name: "float2", file: !21, line: 98, baseType: !22)
!21 = !DIFile(filename: "builtins.h", directory: "test")
!22 = !DICompositeType(tag: DW_TAG_array_type, baseType: !23, size: 64, flags: DIFlagVector, elements: !24)
!23 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!24 = !{!25}
!25 = !DISubrange(count: 2)
!26 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !18)
!27 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !23, size: 64, dwarfAddressSpace: 3)
!28 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !18, size: 64, dwarfAddressSpace: 2)
!29 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !30, size: 64, dwarfAddressSpace: 1)
!30 = !DIDerivedType(tag: DW_TAG_typedef, name: "int2", file: !21, line: 81, baseType: !31)
!31 = !DICompositeType(tag: DW_TAG_array_type, baseType: !18, size: 64, flags: DIFlagVector, elements: !24)
!32 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !33, size: 64, dwarfAddressSpace: 1)
!33 = !DIDerivedType(tag: DW_TAG_typedef, name: "int3", file: !21, line: 82, baseType: !34)
!34 = !DICompositeType(tag: DW_TAG_array_type, baseType: !18, size: 128, flags: DIFlagVector, elements: !35)
!35 = !{!36}
!36 = !DISubrange(count: 3)
!37 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !38, size: 64, dwarfAddressSpace: 1)
!38 = !DIDerivedType(tag: DW_TAG_typedef, name: "int4", file: !21, line: 83, baseType: !39)
!39 = !DICompositeType(tag: DW_TAG_array_type, baseType: !18, size: 128, flags: DIFlagVector, elements: !40)
!40 = !{!41}
!41 = !DISubrange(count: 4)
!42 = !{}
!45 = !DILocalVariable(name: "in1", arg: 1, scope: !13, file: !14, line: 9, type: !17)
!46 = !DILocation(line: 9, scope: !13)
!47 = !DILocalVariable(name: "in2", arg: 2, scope: !13, file: !14, line: 10, type: !19)
!48 = !DILocation(line: 10, scope: !13)
!49 = !DILocalVariable(name: "in3", arg: 3, scope: !13, file: !14, line: 11, type: !26)
!50 = !DILocation(line: 11, scope: !13)
!51 = !DILocalVariable(name: "in4", arg: 4, scope: !13, file: !14, line: 12, type: !27)
!52 = !DILocation(line: 12, scope: !13)
!53 = !DILocalVariable(name: "in5", arg: 5, scope: !13, file: !14, line: 13, type: !28)
!54 = !DILocation(line: 13, scope: !13)
!55 = !DILocalVariable(name: "in6", arg: 6, scope: !13, file: !14, line: 14, type: !17)
!56 = !DILocation(line: 14, scope: !13)
!57 = !DILocalVariable(name: "in7", arg: 7, scope: !13, file: !14, line: 15, type: !29)
!58 = !DILocation(line: 15, scope: !13)
!59 = !DILocalVariable(name: "in8", arg: 8, scope: !13, file: !14, line: 16, type: !32)
!60 = !DILocation(line: 16, scope: !13)
!61 = !DILocalVariable(name: "in9", arg: 9, scope: !13, file: !14, line: 17, type: !37)
!62 = !DILocation(line: 17, scope: !13)
!63 = !DILocalVariable(name: "out", arg: 10, scope: !13, file: !14, line: 18, type: !17)
!64 = !DILocation(line: 18, scope: !13)
!65 = !DILocation(line: 20, scope: !13)
!66 = !{i64 0, i64 1024}
!67 = !DILocation(line: 21, scope: !13)
