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

; RUN: not muxc --device-sg-sizes 6,7,8,9 --passes=verify-reqd-sub-group-legal %s 2>&1 | FileCheck %s

; CHECK: kernel.cl:10:0: kernel 'foo_sg5' has required sub-group size 5 which is not supported by this device
define void @foo_sg5() !dbg !5 !intel_reqd_sub_group_size !2 {
  ret void
}

; CHECK-NOT: kernel 'foo_sg6' has required sub-group size 6 which is not supported by this device
define void @foo_sg6() !dbg !6 !intel_reqd_sub_group_size !3 {
  ret void
}

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!1}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !4, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !{i32 2, !"Debug Info Version", i32 3}

!2 = !{i32 5}
!3 = !{i32 6}

!4 = !DIFile(filename: "kernel.cl", directory: "/oneAPI")
!5 = distinct !DISubprogram(name: "foo_sg5", scope: !4, file: !4, line: 10, scopeLine: 10, flags: DIFlagArtificial | DIFlagPrototyped, unit: !0)
!6 = distinct !DISubprogram(name: "foo_sg6", scope: !4, file: !4, line: 12, scopeLine: 12, flags: DIFlagArtificial | DIFlagPrototyped, unit: !0)
