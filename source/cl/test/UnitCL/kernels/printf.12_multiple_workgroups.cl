// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void multiple_workgroups(void) {
  size_t i = get_local_id(0);
  if (i == 0) {
    printf("(%d, %d, %d)", get_group_id(0), get_group_id(1), get_group_id(2));
  }
}
