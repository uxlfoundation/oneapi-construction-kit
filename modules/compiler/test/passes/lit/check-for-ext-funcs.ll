; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %not %muxc --passes check-ext-funcs -S %s 2>&1 | %filecheck %s

; CHECK: error: Could not find a definition for external function 'foo'
; CHECK-NOT: error: Could not find a definition for external function 'bar'

declare i32 @foo()
declare i32 @_Z3bar()
