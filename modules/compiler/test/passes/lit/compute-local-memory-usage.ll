; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes compute-local-memory-usage,verify -S %s | %filecheck %s

@a = internal addrspace(3) global i16 undef, align 2
@b = internal addrspace(3) global [4 x float] undef, align 4

declare spir_func void @ext_fn()

declare spir_func void @leaf_fn()

define spir_func void @helper_fn() {
  %ld = load i16, i16 addrspace(3)* @a, align 2
  ret void
}

define spir_func void @other_helper_fn() {
  call spir_func void @leaf_fn()
  ret void
}

; CHECK: define spir_kernel void @kernel_bar() [[ATTRS_BAR:#[0-9]+]] {
define spir_kernel void @kernel_bar() #0 {
  call spir_func void @ext_fn()
  call spir_func void @other_helper_fn()
  ret void
}

; CHECK: define spir_kernel void @kernel_foo() [[ATTRS_FOO:#[0-9]+]] {
define spir_kernel void @kernel_foo() #0 {
  call spir_func void @ext_fn()
  call spir_func void @helper_fn()
  call spir_kernel void @kernel_bar()
  %ld = load [4 x float], [4 x float] addrspace(3)* @b
  ret void
}

; Check that we overwrite the existing attribute
; CHECK-DAG: attributes [[ATTRS_BAR]] = { "mux-kernel"="entry-point" "mux-local-mem-usage"="0" }
; CHECK-DAG: attributes [[ATTRS_FOO]] = { "mux-kernel"="entry-point" "mux-local-mem-usage"="18" }

attributes #0 = { "mux-kernel"="entry-point" "mux-local-mem-usage"="4" }
