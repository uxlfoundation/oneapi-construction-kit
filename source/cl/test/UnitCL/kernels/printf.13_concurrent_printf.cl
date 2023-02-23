// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void concurrent_printf(void) {
  size_t i = get_global_id(0);
  if (0 == (i % 2)) {
    printf("%d", i);
  } else {
    printf("%d", i + 1);
  }
}
