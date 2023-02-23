// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel
void load16(__global uchar *out,
            __global uchar *in,
            int stride) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    uchar v1 = in[(x + y*stride) * 2];
    uchar v2 = in[(x + y*stride) * 2 + 1];

    out[x + y*stride] = v1 + v2;
}
