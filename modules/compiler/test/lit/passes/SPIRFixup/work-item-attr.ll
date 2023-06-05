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
;
; Bitcode files generated using Khronos's modified version of Clang:
;
; clang -cc1 -emit-llvm-bc -triple spir-unknown-unknown
;       -include path/to/opencl_spir.h -O0
;       -o work-item-attr.bc32
;          work-item-attr.cl
;
; clang -cc1 -emit-llvm-bc -triple spir64-unknown-unknown
;       -include path/to/opencl_spir.h -O0
;       -o work-item-attr.bc64
;          work-item-attr.cl

; kernel void k(__global size_t* out) {
;   out[0] = get_global_size(0);
;   out[1] = get_global_id(0);
;   out[2] = get_global_offset(0);
;   out[3] = get_local_size(0);
;   out[4] = get_local_id(0);
;   out[5] = get_num_groups(0);
;   out[6] = get_group_id(0);
; }

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: muxc %p/Inputs/work-item-attr.bc32 -passes spir-fixup -S < %s | FileCheck %t
; RUN: muxc %p/Inputs/work-item-attr.bc64 -passes spir-fixup -S < %s | FileCheck %t
;
; Each of the get_*() functions should have the readonly attribute
; (or LLVM 16+ equivalent) and not the readnone attribute.
;
; CHECK: declare spir_func {{.*}} @_Z15get_global_sizej(i32) #[[ATTR:[0-9]+]]
;
; CHECK: declare spir_func {{.*}} @_Z13get_global_idj(i32) #[[ATTR]]
;
; CHECK: declare spir_func {{.*}} @_Z17get_global_offsetj(i32) #[[ATTR]]
;
; CHECK-NOT: readnone
; CHECK: declare spir_func {{.*}} @_Z14get_local_sizej(i32) #[[ATTR]]
;
; CHECK-NOT: readnone
; CHECK: declare spir_func {{.*}} @_Z12get_local_idj(i32) #[[ATTR]]
;
; CHECK-NOT: readnone
; CHECK: declare spir_func {{.*}} @_Z14get_num_groupsj(i32) #[[ATTR]]
;
; CHECK-NOT: readnone
; CHECK: declare spir_func {{.*}} @_Z12get_group_idj(i32) #[[ATTR]]
;
; CHECK-NOT: attributes #[[ATTR]] = { {{([^ ]+ )*}}readnone{{( [^ ]+)*}} }
; CHECK-LT16: attributes #[[ATTR]] = { {{([^ ]+ )*}}readonly{{( [^ ]+)*}} }
; CHECK-GE16: attributes #[[ATTR]] = { {{([^ ]+ )*}}memory(read){{( [^ ]+)*}} }
