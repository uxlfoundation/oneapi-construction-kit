; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %not %muxc --passes wibble -S %s 2>&1 | %filecheck %s

; CHECK: Parse of wibble failed : unknown pass name 'wibble'
