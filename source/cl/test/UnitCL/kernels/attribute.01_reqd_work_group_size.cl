// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel __attribute__((reqd_work_group_size(16, 8, 4)))
void reqd_work_group_size(global ulong *out) {
  out[0] = get_local_size(0);
  out[1] = get_local_size(1);
  out[2] = get_local_size(2);
}
