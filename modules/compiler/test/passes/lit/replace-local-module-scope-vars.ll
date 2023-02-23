; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %muxc --passes replace-module-scope-vars,verify -S %s | %filecheck %t

; Check that wrapped kernels are given 'alwaysinline' attributes, unless they
; have 'noinline' attributes.

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK-LT15: %localVarTypes = type { i16, [6 x i8], i32 addrspace(3)*, i32 }
; CHECK-GE15: %localVarTypes = type { i16, [6 x i8], ptr addrspace(3), i32 }

@a = internal addrspace(3) global i16 undef, align 2
@b = internal addrspace(3) global [4 x float] undef, align 4
@c = internal addrspace(3) global i32 addrspace(3)* undef
@d = internal addrspace(3) global i32 undef

; CHECK-GE15: define internal spir_kernel void @add(ptr addrspace(1) %in, ptr addrspace(1) %out, ptr [[STRUCTPTR:%.*]]) #[[ATTRS:[0-9]+]]
; CHECK-GE15: [[GEP:%.*]] = getelementptr inbounds %localVarTypes, ptr [[STRUCTPTR]], i32 0, i32 0
; CHECK-GE15: [[ADDR:%.*]] = addrspacecast ptr [[GEP]] to ptr addrspace(3)
; CHECK-GE15: %ld = load i16, ptr addrspace(3) [[ADDR]], align 2
; CHECK-GE15: [[GEPC:%.*]] = getelementptr inbounds %localVarTypes, ptr [[STRUCTPTR]], i32 0, i32 2
; CHECK-GE15: [[ADDRC:%.*]] = addrspacecast ptr [[GEPC]] to ptr addrspace(3)
; CHECK-GE15: [[GEPD:%.*]] = getelementptr inbounds %localVarTypes, ptr [[STRUCTPTR]], i32 0, i32 3
; CHECK-GE15: [[ADDRD:%.*]] = addrspacecast ptr [[GEPD]] to ptr addrspace(3)
; CHECK-GE15: %val = cmpxchg ptr addrspace(3) [[ADDRC]], ptr addrspace(3) [[ADDRD]], ptr addrspace(3) [[ADDRD]] acq_rel monotonic

; CHECK-LT15: define internal spir_kernel void @add(i32 addrspace(1)* %in, i32 addrspace(1)* %out, %localVarTypes* [[STRUCTPTR:%.*]]) #[[ATTRS:[0-9]+]]
; CHECK-LT15: [[GEP:%.*]] = getelementptr inbounds %localVarTypes, %localVarTypes* [[STRUCTPTR]], i32 0, i32 0
; CHECK-LT15: [[ADDR:%.*]] = addrspacecast i16* [[GEP]] to i16 addrspace(3)*
; CHECK-LT15: %ld = load i16, i16 addrspace(3)* [[ADDR]], align 2
; CHECK-LT15: [[GEPC:%.*]] = getelementptr inbounds %localVarTypes, %localVarTypes* [[STRUCTPTR]], i32 0, i32 2
; CHECK-LT15: [[ADDRC:%.*]] = addrspacecast i32 addrspace(3)** [[GEPC]] to i32 addrspace(3)* addrspace(3)*
; CHECK-LT15: [[GEPD:%.*]] = getelementptr inbounds %localVarTypes, %localVarTypes* [[STRUCTPTR]], i32 0, i32 3
; CHECK-LT15: [[ADDRD:%.*]] = addrspacecast i32* [[GEPD]] to i32 addrspace(3)*
; CHECK-LT15: %val = cmpxchg i32 addrspace(3)* addrspace(3)* [[ADDRC]], i32 addrspace(3)* [[ADDRD]], i32 addrspace(3)* [[ADDRD]] acq_rel monotonic
; CHECK: ret void
; CHECK: }


; CHECK-GE15: define spir_kernel void @foo.mux-local-var-wrapper(ptr addrspace(1) [[ARG0:%.*]], ptr addrspace(1) [[ARG1:%.*]]) #[[WRAPPER_ATTRS:[0-9]+]]
; CHECK-LT15: define spir_kernel void @foo.mux-local-var-wrapper(i32 addrspace(1)* [[ARG0:%.*]], i32 addrspace(1)* [[ARG1:%.*]]) #[[WRAPPER_ATTRS:[0-9]+]]
; The alignment of this alloca must be the maximum alignment of the new struct
; CHECK: [[ALLOCA:%.*]] = alloca %localVarTypes, align 8
; CHECK-GE15: call spir_kernel void @add(ptr addrspace(1) [[ARG0]], ptr addrspace(1) [[ARG1]], ptr [[ALLOCA]])
; CHECK-LT15: call spir_kernel void @add(i32 addrspace(1)* [[ARG0]], i32 addrspace(1)* [[ARG1]], %localVarTypes* [[ALLOCA]])
define spir_kernel void @add(i32 addrspace(1)* %in, i32 addrspace(1)* %out) #0 {
  %ld = load i16, i16 addrspace(3)* @a, align 2
  %val = cmpxchg i32 addrspace(3)* addrspace(3)* @c, i32 addrspace(3)* @d, i32 addrspace(3)* @d acq_rel monotonic
  ret void
}

; Check we haven't added alwaysinline, given that the kernel is marked
; noinline.
; Check also that we've preserved the original function name attribute
; CHECK: attributes #[[ATTRS]] = { noinline "mux-base-fn-name"="foo" }
; CHECK: attributes #[[WRAPPER_ATTRS]] = { noinline nounwind "mux-base-fn-name"="foo" "mux-kernel"="entry-point" }

attributes #0 = { noinline "mux-base-fn-name"="foo" "mux-kernel"="entry-point" }
