// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void uniform_atomics(global int* flag) { atomic_inc(flag); }
