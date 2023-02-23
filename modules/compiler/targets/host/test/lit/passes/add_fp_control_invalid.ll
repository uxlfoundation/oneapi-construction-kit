; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %not %muxc --device "%default_device" --passes "add-fp-control<invalid>" -S %s
