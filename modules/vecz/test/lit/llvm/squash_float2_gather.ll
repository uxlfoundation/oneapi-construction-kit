; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k squash -vecz-passes="squash-small-vecs,packetizer" -S < %s | %filecheck %t

; ModuleID = 'kernel.opencl'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

; Function Attrs: nounwind
define spir_kernel void @squash(i64 addrspace(1)* %idx, <2 x float> addrspace(1)* %data, <2 x float> addrspace(1)* %output) #0 {
entry:
  %gid = call spir_func i64 @_Z13get_global_idj(i64 0) #2
  %idx.ptr = getelementptr inbounds i64, i64 addrspace(1)* %idx, i64 %gid
  %idx.ld = load i64, i64 addrspace(1)* %idx.ptr, align 8
  %data.ptr = getelementptr inbounds <2 x float>, <2 x float> addrspace(1)* %data, i64 %idx.ld
  %data.ld = load <2 x float>, <2 x float> addrspace(1)* %data.ptr, align 8
  %output.ptr = getelementptr inbounds <2 x float>, <2 x float> addrspace(1)* %output, i64 %gid
  store <2 x float> %data.ld, <2 x float> addrspace(1)* %output.ptr, align 8
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i64) #1

attributes #0 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="0" "stackrealign" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="0" "stackrealign" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nobuiltin nounwind }

; It checks that the <2 x float> is converted into a i64 for the purpose of the
; gather load
;
; CHECK: void @__vecz_v4_squash
; CHECK:  %[[GID:.+]] = call spir_func i64 @_Z13get_global_idj(i64 0) #[[ATTRS:[0-9]+]]
; CHECK-GE15:  %[[IDX_PTR:.+]] = getelementptr inbounds i64, ptr addrspace(1) %idx, i64 %[[GID]]
; CHECK-LT15:  %[[IDX_PTR:.+]] = getelementptr inbounds i64, i64 addrspace(1)* %idx, i64 %[[GID]]
; CHECK-GE15:  %[[WIDE_LOAD:.+]] = load <4 x i64>, ptr addrspace(1) %[[IDX_PTR]], align 8
; CHECK-LT15:  %[[WIDEN:.+]] = bitcast i64 addrspace(1)* %[[IDX_PTR]] to <4 x i64> addrspace(1)*
; CHECK-LT15:  %[[WIDE_LOAD:.+]] = load <4 x i64>, <4 x i64> addrspace(1)* %[[WIDEN]], align 8
; CHECK-GE15:  %[[DATA_PTR:.+]] = getelementptr inbounds <2 x float>, ptr addrspace(1) %data, <4 x i64> %[[WIDE_LOAD]]
; CHECK-LT15:  %[[DATA_PTR:.+]] = getelementptr inbounds <2 x float>, <2 x float> addrspace(1)* %data, <4 x i64> %[[WIDE_LOAD]]
; CHECK-GE15:  %[[GATHER:.+]] = call <4 x i64> @__vecz_b_gather_load8_Dv4_mDv4_u3ptrU3AS1(<4 x ptr addrspace(1)> %[[DATA_PTR]])
; CHECK-LT15:  %[[SQUASH:.+]] = bitcast <4 x <2 x float> addrspace(1)*> %[[DATA_PTR]] to <4 x i64 addrspace(1)*>
; CHECK-LT15:  %[[GATHER:.+]] = call <4 x i64> @__vecz_b_gather_load8_Dv4_mDv4_PU3AS1m(<4 x i64 addrspace(1)*> %[[SQUASH]])
; CHECK:  %[[UNSQUASH:.+]] = bitcast <4 x i64> %[[GATHER]] to <8 x float>
; CHECK-GE15:  %[[OUTPUT_PTR:.+]] = getelementptr inbounds <2 x float>, ptr addrspace(1) %output, i64 %[[GID]]
; CHECK-LT15:  %[[OUTPUT_PTR:.+]] = getelementptr inbounds <2 x float>, <2 x float> addrspace(1)* %output, i64 %[[GID]]
; CHECK-GE15:  store <8 x float> %[[UNSQUASH]], ptr addrspace(1) %[[OUTPUT_PTR]], align 8
; CHECK-LT15:  %[[WIDEN2:.+]] = bitcast <2 x float> addrspace(1)* %[[OUTPUT_PTR]] to <8 x float> addrspace(1)*
; CHECK-LT15:  store <8 x float> %[[UNSQUASH]], <8 x float> addrspace(1)* %[[WIDEN2]], align 8
; CHECK:  ret void

; CHECK: attributes #[[ATTRS]] = { nobuiltin nounwind }
