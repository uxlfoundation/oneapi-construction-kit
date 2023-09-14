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

; RUN: muxc --passes "print<sub-groups>" < %s 2>&1 | FileCheck %s

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

; CHECK: Function 'kernel1' uses 2 sub-group builtins: {{[0-9]+,[0-9]+$}}
define spir_kernel void @kernel1(i32 %x) {
entry:
  %lid = call i32 @__mux_get_sub_group_local_id()
  %call = call i32 @__mux_sub_group_shuffle_i32(i32 %x, i32 %lid)
  ret void
}

; CHECK: Function 'kernel2' uses 2 sub-group builtins: {{[0-9]+,[0-9]+$}}
define spir_kernel void @kernel2() {
entry:
  %lid = call i32 @__mux_get_sub_group_local_id()
  br label %exit
exit:
  %call = call i32 @__mux_get_max_sub_group_size()
  ret void
}

; CHECK: Function 'function1' uses 1 sub-group builtin: {{[0-9]+$}}
define spir_func i32 @function1() {
  %call = call i32 @__mux_get_max_sub_group_size()
  ret i32 %call
}

; CHECK: Function 'function2' uses no sub-group builtins
define spir_func void @function2() {
  ret void
}

; CHECK: Function 'function3' uses 2 sub-group builtins: {{[0-9]+,[0-9]+$}}
define spir_func i32 @function3() {
  %call = call i32 @function1()
  %call2 = call i32 @__mux_get_sub_group_id()
  ret i32 %call
}

; CHECK: Function 'kernel3' uses 3 sub-group builtins: {{[0-9]+,[0-9]+,[0-9]+$}}
define spir_kernel void @kernel3() {
entry:
  %lid = call i32 @__mux_get_sub_group_local_id()
  br label %exit
exit:
  %call = call i32 @function3()
  ; Call this function twice - it shouldn't matter
  %call2 = call i32 @function3()
  ret void
}

; We don't consider any of the internal mux 'setters' as uses of sub-group
; builtins, as they are purely there to support the framework and aren't
; apparent to the user.
; CHECK: Function 'function4' uses no sub-group builtins
define spir_func void @function4() {
  call void @__mux_set_sub_group_id(i32 0)
  call void @__mux_set_num_sub_groups(i32 1)
  call void @__mux_set_max_sub_group_size(i32 2)
  ret void
}

declare i32 @__mux_get_sub_group_id()
declare i32 @__mux_get_sub_group_local_id()
declare i32 @__mux_sub_group_shuffle_i32(i32, i32)
declare i32 @__mux_get_max_sub_group_size()

declare void @__mux_set_sub_group_id(i32)
declare void @__mux_set_num_sub_groups(i32)
declare void @__mux_set_max_sub_group_size(i32)
