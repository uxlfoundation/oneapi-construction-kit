; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes "require<device-info>,check-doubles" -S %s 2>&1
; RUN: %not %muxc --passes check-doubles -S %s 2>&1 | %filecheck %s --check-prefix ERROR

; ERROR: error: A double precision floating point number was generated, but cl_khr_fp64 is not supported on this target.

define void @foo() {
  %a = fadd double 0.0, 1.0
  ret void

}
