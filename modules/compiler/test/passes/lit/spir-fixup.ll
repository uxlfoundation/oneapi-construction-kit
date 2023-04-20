; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %muxc --passes spir-fixup,verify -S %s | %filecheck %t

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; Check that we change 'readnone' to 'readonly' on work-item functions
; CHECK: declare i64 @_Z13get_global_idj(i32) [[ATTRS:#[0-9]+]]
declare i64 @_Z13get_global_idj(i32) #0

define void @foo() #1 {
  ; CHECK: %x = call i64 @_Z13get_global_idj(i32 0) [[ATTRS]]
  %x = call i64 @_Z13get_global_idj(i32 0) #0
  ret void
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind }

; CHECK-GE16: attributes [[ATTRS]] = { nounwind memory(read) }
; CHECK-LT16: attributes [[ATTRS]] = { nounwind readonly }
