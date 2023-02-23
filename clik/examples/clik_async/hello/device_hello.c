// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "device_hello.h"

__kernel void hello_async(exec_state_t *ctx) {
  print(ctx, "Hello from clik_async! tid=%d, lid=%d, gid=%d\n",
        get_global_id(0, ctx), get_local_id(0, ctx), get_group_id(0, ctx));
}
