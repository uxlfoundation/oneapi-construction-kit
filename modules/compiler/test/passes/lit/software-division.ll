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

; RUN: muxc --passes software-div,verify -S %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: define spir_kernel void @zero_udiv(
; CHECK: [[CMP:%.*]] = icmp eq i64 %gid, 0
; CHECK: [[SAFE:%.*]] = select i1 [[CMP]], i64 1, i64 %gid
; CHECK: udiv i64 %x.conv, [[SAFE]]
define spir_kernel void @zero_udiv(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
  %gid = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %gid
  %x = load i32, i32 addrspace(1)* %arrayidx, align 4
  %x.conv = sext i32 %x to i64
  %div = udiv i64 %x.conv, %gid
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %gid
  %div.trunc = trunc i64 %div to i32
  store i32 %div.trunc, i32 addrspace(1)* %arrayidx2
  ret void
}

; CHECK: define spir_kernel void @zero_sdiv(
; CHECK: [[CMP1:%.*]] = icmp eq i64 %x.conv, -9223372036854775808
; CHECK: [[CMP2:%.*]] = icmp eq i64 %gid, -1
; CHECK: [[CMP3:%.*]] = and i1 [[CMP1]], [[CMP2]]
; CHECK: [[CMP4:%.*]] = icmp eq i64 %gid, 0
; CHECK: [[CMP5:%.*]] = or i1 [[CMP4]], [[CMP3]]
; CHECK: [[SAFE:%.*]] = select i1 [[CMP5]], i64 1, i64 %gid
; CHECK: sdiv i64 %x.conv, [[SAFE]]
define spir_kernel void @zero_sdiv(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
  %gid = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %gid
  %x = load i32, i32 addrspace(1)* %arrayidx, align 4
  %x.conv = sext i32 %x to i64
  %div = sdiv i64 %x.conv, %gid
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %gid
  %div.trunc = trunc i64 %div to i32
  store i32 %div.trunc, i32 addrspace(1)* %arrayidx2
  ret void
}

; Check that optnone functions are also fixed up
; CHECK: define spir_kernel void @zero_udiv_optnone(
; CHECK: [[CMP:%.*]] = icmp eq i64 %gid, 0
; CHECK: [[SAFE:%.*]] = select i1 [[CMP]], i64 1, i64 %gid
; CHECK: udiv i64 %x.conv, [[SAFE]]
define spir_kernel void @zero_udiv_optnone(i32 addrspace(1)* %in, i32 addrspace(1)* %out) optnone noinline {
  %gid = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %gid
  %x = load i32, i32 addrspace(1)* %arrayidx, align 4
  %x.conv = sext i32 %x to i64
  %div = udiv i64 %x.conv, %gid
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %gid
  %div.trunc = trunc i64 %div to i32
  store i32 %div.trunc, i32 addrspace(1)* %arrayidx2
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)
