; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes add-kernel-wrapper,verify -S %s  \
; RUN:   | %filecheck %s --check-prefixes PACKED,LOCALS-BY-SIZE
; RUN: %muxc --passes "add-kernel-wrapper<packed>,verify" -S %s  \
; RUN:   | %filecheck %s --check-prefixes PACKED,LOCALS-BY-SIZE
; RUN: %muxc --passes "add-kernel-wrapper<unpacked>,verify" -S %s  \
; RUN:   | %filecheck %s --check-prefixes UNPACKED,LOCALS-BY-SIZE

; RUN: %muxc --passes "add-kernel-wrapper<local-buffers-by-size>,verify" -S %s  \
; RUN:   | %filecheck %s --check-prefix LOCALS-BY-SIZE
; RUN: %muxc --passes "add-kernel-wrapper<local-buffers-by-ptr>,verify" -S %s  \
; RUN:   | %filecheck %s --check-prefix LOCALS-BY-PTR

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; PACKED:   %MuxPackedArgs.foo = type <{ ptr addrspace(1), i1, ptr addrspace(1) }>
; UNPACKED: %MuxPackedArgs.foo = type { ptr addrspace(1), i1, [7 x i8], ptr addrspace(1) }

; LOCALS-BY-SIZE: %MuxPackedArgs.bar = type {{<?}}{ ptr addrspace(1), i64 }{{>?}}
; LOCALS-BY-PTR:  %MuxPackedArgs.bar = type {{<?}}{ ptr addrspace(1), ptr addrspace(3) }{{>?}}

define spir_kernel void @foo(ptr addrspace(1) %x, i1 %b, ptr addrspace(1) %y) #0 {
  %z = addrspacecast ptr addrspace(1) %x to ptr
  %w = addrspacecast ptr addrspace(1) %y to ptr
  ret void
}

define spir_kernel void @bar(ptr addrspace(1) %global, ptr addrspace(3) %local) #0 {
  %x = addrspacecast ptr addrspace(1) %global to ptr
  %y = addrspacecast ptr addrspace(3) %local to ptr
  ret void
}

attributes #0 = { "mux-kernel"="entry-point" }
