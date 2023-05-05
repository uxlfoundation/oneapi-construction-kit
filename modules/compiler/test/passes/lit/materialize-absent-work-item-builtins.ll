; Copyright (C) Codeplay Software Limited
;
; Licensed under the Apache License, Version 2.0 (the "License") with LLVM
; Exceptions; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
