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

; Run the verifier first, as working with scalable vectors in literal structs
; can be fraught with illegality (as discovered when writing this test).
; RUN: muxc --passes verify,work-item-loops,verify -S %s  | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: %foo_live_mem_info = type { i8, [7 x i8], { <4 x i8>, i32, <8 x i1> }, [12 x i8], [0 x i8] }

; Check that we can successfully save/reload fixed and scalable struct literals
; across barriers. Scalable struct literals must be decomposed as it's invalid
; to store them whole - they're "unsized". This might change in future versions
; of LLVM.
; CHECK: @foo.mux-barrier-region(ptr [[D:%.*]], ptr [[A:%.*]], ptr [[MEM:%.*]])
; CHECK:  [[FIXED_ADDR:%.*]] = getelementptr inbounds %foo_live_mem_info, ptr [[MEM]], i32 0, i32 2
; CHECK:  [[VSCALE:%.*]] = call i64 @llvm.vscale.i64()
; CHECK:  [[NXV1I16_OFFS:%.*]] = mul i64 [[VSCALE]], 32
; CHECK:  [[NXV1I16_ADDR:%.*]] = getelementptr inbounds %foo_live_mem_info, ptr [[MEM]], i32 0, i32 4, i64 [[NXV1I16_OFFS]]
; CHECK:  [[NXV8I32_ADDR:%.*]] = getelementptr inbounds %foo_live_mem_info, ptr [[MEM]], i32 0, i32 4, i32 0
; CHECK:  [[I8_ADDR:%.*]] = getelementptr inbounds %foo_live_mem_info, ptr [[MEM]], i32 0, i32 0
; CHECK:  [[IDX:%.*]] = tail call i64 @__mux_get_global_id(i32 0)
; We can store the fixed struct whole
; CHECK:  [[FIXED_STRUCT:%.*]] = call { <4 x i8>, i32, <8 x i1> } @ext_fixed_vec()
; CHECK:  store { <4 x i8>, i32, <8 x i1> } [[FIXED_STRUCT]], ptr [[FIXED_ADDR]], align 4
; We must break down the scalable struct into pieces
; CHECK:  [[SCALABLE_STRUCT:%.*]] = call { <vscale x 1 x i16>, <vscale x 8 x i32>, i8 } @ext_scalable_vec()
; CHECK:  [[EXT0:%.*]] = extractvalue { <vscale x 1 x i16>, <vscale x 8 x i32>, i8 } [[SCALABLE_STRUCT]], 0
; CHECK:  store <vscale x 1 x i16> [[EXT0]], ptr [[NXV1I16_ADDR:%.*]], align 2
; CHECK:  [[EXT1:%.*]] = extractvalue { <vscale x 1 x i16>, <vscale x 8 x i32>, i8 } [[SCALABLE_STRUCT]], 1
; CHECK:  store <vscale x 8 x i32> [[EXT1]], ptr [[NXV8I32_ADDR]], align 32
; CHECK:  [[EXT2:%.*]] = extractvalue { <vscale x 1 x i16>, <vscale x 8 x i32>, i8 } [[SCALABLE_STRUCT]], 2
; CHECK:  store i8 [[EXT2]], ptr [[I8_ADDR]], align 1
; CHECK:  ret i32 2


; CHECK: @foo.mux-barrier-region.1(ptr [[D:%.*]], ptr [[A:%.*]], ptr [[MEM:%.*]])
; CHECK:  [[FIXED_ADDR:%.*]] = getelementptr inbounds %foo_live_mem_info, ptr [[MEM]], i32 0, i32 2
; CHECK:  [[VSCALE:%.*]] = call i64 @llvm.vscale.i64()
; CHECK:  [[NXV1I16_OFFS:%.*]] = mul i64 [[VSCALE]], 32
; CHECK:  [[NXV1I16_ADDR:%.*]] = getelementptr inbounds %foo_live_mem_info, ptr [[MEM]], i32 0, i32 4, i64 [[NXV1I16_OFFS]]
; CHECK:  [[NXV8I32_ADDR:%.*]] = getelementptr inbounds %foo_live_mem_info, ptr [[MEM]], i32 0, i32 4, i32 0
; CHECK:  [[I8_ADDR:%.*]] = getelementptr inbounds %foo_live_mem_info, ptr [[MEM]], i32 0, i32 0
; CHECK: [[IDX:%.*]] = {{(tail )?}}call i64 @__mux_get_global_id(i32 0)
; We can reload the fixed struct directly
; CHECK:  [[FIXED_LD:%.*]] = load { <4 x i8>, i32, <8 x i1> }, ptr [[FIXED_ADDR]], align 4
; Load and insert the first scalable element
; CHECK:  [[NXV1I16_LD:%.*]] = load <vscale x 1 x i16>, ptr [[NXV1I16_ADDR]], align 2
; CHECK:  [[INS0:%.*]] = insertvalue { <vscale x 1 x i16>, <vscale x 8 x i32>, i8 } poison, <vscale x 1 x i16> [[NXV1I16_LD]], 0
; Load and insert the second scalable element
; CHECK:  [[NXV8I32_LD:%.*]] = load <vscale x 8 x i32>, ptr [[NXV8I32_ADDR]], align 32
; CHECK:  [[INS1:%.*]] = insertvalue { <vscale x 1 x i16>, <vscale x 8 x i32>, i8 } [[INS0]], <vscale x 8 x i32> [[NXV8I32_LD]], 1
; Load and insert the third and last scalable element
; CHECK:  [[I8_LD:%.*]] = load i8, ptr [[I8_ADDR:%.*]], align 1
; CHECK:  [[SCALABLE_LD:%.*]] = insertvalue { <vscale x 1 x i16>, <vscale x 8 x i32>, i8 } [[INS1]], i8 [[I8_LD]], 2

; All the original code from after the barrier
; CHECK: %arrayidx1 = getelementptr inbounds i8, ptr %0, i64 [[IDX]]
; CHECK: store { <4 x i8>, i32, <8 x i1> } [[FIXED_LD]], ptr %arrayidx1, align 4
; CHECK: %elt0 = extractvalue { <vscale x 1 x i16>, <vscale x 8 x i32>, i8 } [[SCALABLE_LD]], 0
; CHECK: %elt1 = extractvalue { <vscale x 1 x i16>, <vscale x 8 x i32>, i8 } [[SCALABLE_LD]], 1
; CHECK: %elt2 = extractvalue { <vscale x 1 x i16>, <vscale x 8 x i32>, i8 } [[SCALABLE_LD]], 2
; CHECK: %arrayidx2 = getelementptr inbounds i8, ptr [[A]], i64 [[IDX]]
; CHECK: store <vscale x 1 x i16> %elt0, ptr %arrayidx2, align 1
; CHECK: store <vscale x 8 x i32> %elt1, ptr %arrayidx2, align 4
; CHECK: store i8 %elt2, ptr %arrayidx2, align 1

; CHECK: define void @foo.mux-barrier-wrapper(ptr %d, ptr %a)
; CHECK:  [[VSCALE:%.*]] = call i64 @llvm.vscale.i64()
; CHECK:  [[SCALABLE_SIZE:%.*]] = mul i64 [[VSCALE:%.*]], 64
; CHECK:  [[PER_WI_SIZE:%.*]] = add i64 [[SCALABLE_SIZE]], 32
; CHECK:  [[TOTAL_WG_SIZE:%.*]] = mul i64 [[PER_WI_SIZE]], {{%.*}}
; CHECK:  %live_variables = alloca i8, i64 [[TOTAL_WG_SIZE]], align 32
define internal void @foo(ptr %d, ptr %a) #0 {
entry:
  %idx = tail call i64 @__mux_get_global_id(i32 0)
  %fixed.struct.literal = call { <4 x i8>, i32, <8 x i1> } @ext_fixed_vec()
  %scalable.struct.literal = call { <vscale x 1 x i16>, <vscale x 8 x i32>, i8 } @ext_scalable_vec()

  tail call void @__mux_work_group_barrier(i32 0, i32 1, i32 272)

  ; Do something with the value on the other side of the barrier.
  %arrayidx1 = getelementptr inbounds i8, ptr %d, i64 %idx
  store { <4 x i8>, i32, <8 x i1> } %fixed.struct.literal, ptr %arrayidx1, align 4

  ; We can't store "unsized types", so manually extract values and store those.
  %elt0 = extractvalue { <vscale x 1 x i16>, <vscale x 8 x i32>, i8 } %scalable.struct.literal, 0
  %elt1 = extractvalue { <vscale x 1 x i16>, <vscale x 8 x i32>, i8 } %scalable.struct.literal, 1
  %elt2 = extractvalue { <vscale x 1 x i16>, <vscale x 8 x i32>, i8 } %scalable.struct.literal, 2

  %arrayidx2 = getelementptr inbounds i8, ptr %a, i64 %idx
  store <vscale x 1 x i16> %elt0, ptr %arrayidx2, align 1
  store <vscale x 8 x i32> %elt1, ptr %arrayidx2, align 4
  store i8 %elt2, ptr %arrayidx2, align 1

  ret void
}

declare i64 @__mux_get_global_id(i32)
declare void @__mux_work_group_barrier(i32, i32, i32)
declare { <4 x i8>, i32, <8 x i1> } @ext_fixed_vec()
declare { <vscale x 1 x i16>, <vscale x 8 x i32>, i8 } @ext_scalable_vec()

attributes #0 = { "mux-kernel"="entry-point" }
