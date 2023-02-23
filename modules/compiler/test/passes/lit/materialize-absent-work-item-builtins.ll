; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes missing-builtins,verify -S %s | %filecheck %s

declare spir_func i64 @_Z19get_local_linear_idv()
; CHECK: define spir_func i64 @_Z19get_local_linear_idv() {
; CHECK:   [[RET:%.*]] = call spir_func i64 @__mux_get_local_linear_id()
; CHECK:   ret i64 [[RET]]
; CHECK:  }

declare spir_func i64 @_Z20get_global_linear_idv()
; CHECK: define spir_func i64 @_Z20get_global_linear_idv() {
; CHECK:   [[RET:%.*]] = call spir_func i64 @__mux_get_global_linear_id()
; CHECK:   ret i64 [[RET]]
; CHECK:  }

declare spir_func i64 @_Z23get_enqueued_local_sizej(i32)
; CHECK: define spir_func i64 @_Z23get_enqueued_local_sizej(i32 [[P:%.*]]) {
; CHECK:   [[RET:%.*]] = call spir_func i64 @__mux_get_enqueued_local_size(i32 [[P]])
; CHECK:   ret i64 [[RET]]
; CHECK:  }
