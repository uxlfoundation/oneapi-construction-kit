// Copyright (C) Codeplay Software Limited. All Rights Reserved.

struct __attribute__((packed)) PerItemKernelInfo {
  uint4 global_id;
  uint4 group_id;
};

void kernel work_item(global struct PerItemKernelInfo* info) {
  size_t xId = get_global_id(0);
  size_t yId = get_global_id(1);
  size_t zId = get_global_id(2);
  size_t id = xId + (get_global_size(0) * yId) +
              (get_global_size(0) * get_global_size(1) * zId);
  info[id].global_id =
      (uint4)(get_global_id(0), get_global_id(1), get_global_id(2), 0);
  info[id].group_id =
      (uint4)(get_group_id(0), get_group_id(1), get_group_id(2), 0);
}
