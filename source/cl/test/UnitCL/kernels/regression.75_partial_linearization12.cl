// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void partial_linearization12(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;

  while (1) {
    if (n > 0) {
      ret++;
      if (ret <= 10) {
      goto f;
      }
    } else {
      ret++;
    }
    ret++;
    while (1) {
      if (n <= 2) {
        ret -= n * ret;
        goto j;
      } else {
        if (ret + id >= n) {
          ret /= n * n + ret;
          if (ret < n) {
            ret -= n;
            goto m;
          } else {
            ret += n;
            goto n;
          }
        } else {
          if (ret > n) {
            ret += n;
            goto m;
          } else {
            ret -= n;
            goto n;
          }
        }
      }
      m:
        if (n & ret) {
          ret *= n;
          goto q;
        } else {
          goto p;
        }

      n:
        ret *= ret;
      p:
        if (ret > n) {
          goto r;
        }
        ret++;
    }

    r:
      ret *= 4;
      ret++;

    if ((ret + n) & 1) {
      goto t;
    }
    ret++;
  }

f:
  ret /= n;
  goto j;

j:
  if (ret <= n) {
    goto q;
  } else {
    goto u;
  }

t:
  ret++;
  goto u;

q:
  ret++;
  goto v;

u:
  ret++;

v:
  out[id] = ret;
}
