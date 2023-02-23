// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void partial_linearization17(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;
  int i = 0;

  while (1) {
    if (n > 10) {
      goto c;
    } else if (n > 5) {
      goto f;
    }
    if (id + i++ % 2 == 0) {
      break;
    }
  }

  for (int i = 0; i < n + 10; i++) ret++;
  goto m;

f:
  ret += n * 2;
  for (int i = 0; i < n * 2; i++) ret += i;
  goto m;

c:
  for (int i = 0; i < n + 5; i++) ret += 2;
  if (id % 2 == 0) {
    goto h;
  } else {
    goto m;
  }

m:
  ret <<= 2;
  goto o;

h:
  for (int i = 0; i < n * 2; i++) {
    if (n > 5) {
      goto l;
    }
  }
  ret += id << 3;
  goto p;

l:
  ret += id << 3;

o:
  for (int i = 0; i < n * 2; i++) ret += i;

p:
  out[id] = ret;
}
