; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test -vecz-scalable -vecz-simd-width=8 -S < %s | %filecheck %t

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @test(i32 addrspace(1)* %in) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %load = load i32, i32 addrspace(1)* %in
  %slot = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %idx
  store i32 %load, i32 addrspace(1)* %slot
  ret void
}

; CHECK-GE15: define spir_kernel void @test(ptr addrspace(1) %in) !codeplay_ca_vecz.base !0
; CHECK-LT15: define spir_kernel void @test(i32 addrspace(1)* %in) !codeplay_ca_vecz.base !0
; CHECK-GE15: define spir_kernel void @__vecz_nxv8_test(ptr addrspace(1) %in) #0 !codeplay_ca_vecz.derived !2
; CHECK-LT15: define spir_kernel void @__vecz_nxv8_test(i32 addrspace(1)* %in) #0 !codeplay_ca_vecz.derived !2

; CHECK: attributes #0 = { "mux-base-fn-name"="__vecz_nxv8_test" }

; CHECK-GE15: !0 = !{!1, ptr @__vecz_nxv8_test}
; CHECK-LT15: !0 = !{!1, void (i32 addrspace(1)*)* @__vecz_nxv8_test}

; CHECK: !1 = !{i32 8, i32 1, i32 0, i32 0}
; CHECK-GE15: !2 = !{!1, ptr @test}
; CHECK-LT15: !2 = !{!1, void (i32 addrspace(1)*)* @test}
