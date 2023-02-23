; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --print-passes 2>&1 | %filecheck %s

; CHECK: Module passes:
; CHECK: add-sched-params
; CHECK: Module passes with params:
; CHECK: replace-mux-math-decls<fast>
