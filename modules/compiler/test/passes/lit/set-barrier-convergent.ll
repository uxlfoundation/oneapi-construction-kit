; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes set-barrier-convergent -S %s | %filecheck %s

target datalayout = "e-p:32:32:32-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir-unknown-unknown"

; CHECK: declare spir_func void @_Z7barrierj(i32) [[ATTRS:#[0-9]+]]
declare spir_func void @_Z7barrierj(i32)

; CHECK: attributes [[ATTRS]] = { convergent }
