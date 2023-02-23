; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; We can't do any meaningful vectorization with muxc right now, but we can test
; that pass runs and the required analyses can be registered.
; RUN: %muxc --passes "require<vecz-pass-opts>,require<vecz-target-info>,run-vecz" -S %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @foo(i32 addrspace(1)* %a, i32 addrspace(1)* %z) #0 {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %call
  %x = load i32, i32 addrspace(1)* %arrayidx, align 4
  %add = add nsw i32 %x, 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %z, i64 %call
  store i32 %add, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

attributes #0 = { "mux-kernel"="entry-point" }
