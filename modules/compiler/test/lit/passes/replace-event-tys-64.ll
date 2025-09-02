; Copyright (C) Codeplay Software Limited
;
; Licensed under the Apache License, Version 2.0 (the "License") with LLVM
; Exceptions; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

; RUN: muxc --passes replace-target-ext-tys,verify %s | FileCheck %s 

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; CHECK-DAG: [[NEW_STRUCTA:.*]] = type { i64, i64, i64 }
%structTyA = type { i64, target("spirv.Event"), i64 }
; CHECK-DAG: [[NEW_STRUCTB:.*]] = type <{ i8, [4 x i64], [2 x [[NEW_STRUCTA]]] }>
%structTyB = type <{ i8, [4 x target("spirv.Event")], [2 x %structTyA] }>
; Don't remap this type - expect the exact same name
; CHECK-DAG: %structTyC = type { i64, i64, i64 }
%structTyC = type { i64, i64, i64 }

; CHECK-LABEL: define spir_kernel void @my_func
; CHECK-SAME:    [2 x i64] %array_param,
; CHECK-SAME:    [[NEW_STRUCTA]] %named_s_a,
; CHECK-SAME:    { [[NEW_STRUCTA]], i8 } %literal_s,
; CHECK-SAME:    [[NEW_STRUCTB]] %named_s_b,
; CHECK-SAME:    %structTyC %named_s_c
define spir_kernel void @my_func([2 x target("spirv.Event")] %array_param,
                                 %structTyA %named_s_a, { %structTyA, i8 } %literal_s,
                                 %structTyB %named_s_b,
                                 %structTyC %named_s_c) #0 {
; CHECK: event_array = alloca [2 x i64]
  %event_array = alloca [2 x target("spirv.Event")]
; CHECK: %an_event = extractvalue [[NEW_STRUCTA]] %named_s_a, 1
  %an_event = extractvalue %structTyA %named_s_a, 1
; CHECK: %another_event = extractvalue { [[NEW_STRUCTA]], i8 } %literal_s, 0, 1
  %another_event = extractvalue { %structTyA, i8 } %literal_s, 0, 1
; CHECK: call void @some_function(i64 %an_event)
  call void @some_function(target("spirv.Event") %an_event)
; CHECK: call void @some_function(i64 %another_event)
  call void @some_function(target("spirv.Event") %another_event)
  %a_third_event = extractvalue %structTyB %named_s_b, 1, 3
; CHECK: call void @some_function(i64 %a_third_event)
  call void @some_function(target("spirv.Event") %a_third_event)
  ret void
}

declare void @some_function(target("spirv.Event"))

attributes #0 = { convergent nounwind }
