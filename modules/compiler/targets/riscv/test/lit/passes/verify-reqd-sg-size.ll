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

; Try and forcibly vectorize this no-vecz kernel by 8 and check that the
; verification pass correctly picks up that we haven't satisfied the kernel's
; required sub-group size.
; RUN: env CA_RISCV_VF=8 not muxc --device "%riscv_device" \
; RUN:   --passes "run-vecz,verify-reqd-sub-group-satisfied" %s 2>&1 \
; RUN: | FileCheck %s

; CHECK: kernel.cl:10:0: kernel 'foo_sg8' has required sub-group size 8 but the compiler was unable to sastify this constraint
define void @foo_sg8() #0 !dbg !5 !intel_reqd_sub_group_size !2 {
  ret void
}

attributes #0 = { "mux-kernel"="entry-point" "vecz-mode"="never" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!1}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !4, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !{i32 2, !"Debug Info Version", i32 3}

!2 = !{i32 8}

!4 = !DIFile(filename: "kernel.cl", directory: "/oneAPI")
!5 = distinct !DISubprogram(name: "foo_sg7", scope: !4, file: !4, line: 10, scopeLine: 10, flags: DIFlagArtificial | DIFlagPrototyped, unit: !0)
