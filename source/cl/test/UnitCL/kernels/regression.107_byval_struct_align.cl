// Copyright (C) Codeplay Software Limited. All Rights Reserved.


typedef struct _my_innermost_struct {
  char a;
} my_innermost_struct;

typedef struct _my_innermost_struct_holder {
  my_innermost_struct s;
} my_innermost_struct_holder;

typedef struct _my_innermost_struct_holder_holder {
  my_innermost_struct_holder s;
} my_innermost_struct_holder_holder;

typedef struct _my_struct_tuple {
  my_innermost_struct_holder_holder s;
  my_innermost_struct t;
} my_struct_tuple;

typedef struct _my_struct {
  my_innermost_struct_holder_holder s;
  my_struct_tuple t;
} my_struct;

__kernel void byval_struct_align(struct _my_struct s1, int v, __global int *outs1,
                                 struct _my_struct s2, int w, __global int *outs2) {

  const size_t idx = get_global_id(0);
  outs1[idx] = s1.t.t.a + v + w;
  outs2[idx] = s2.t.s.s.s.a + v + w;
}
