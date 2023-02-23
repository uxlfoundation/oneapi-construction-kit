// Copyright (C) Codeplay Software Limited. All Rights Reserved.

struct __attribute__ ((packed)) PerItemKernelInfo {
  ulong4 global_size;
  uint work_dim;
};

void kernel stride_misaligned(global struct PerItemKernelInfo * info) {
  size_t xId = get_global_id(0);
  size_t yId = get_global_id(1);
  size_t zId = get_global_id(2);
  size_t id = xId + (get_global_size(0) * yId) +
               (get_global_size(0) * get_global_size(1) * zId);
  info[id].global_size = (ulong4)(1, 2, 3, 4);
  info[id].work_dim = 5;
}
