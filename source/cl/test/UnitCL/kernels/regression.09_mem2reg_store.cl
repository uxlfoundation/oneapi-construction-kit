// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// SPIR OPTIONS: "-w"
// SPIRV OPTIONS: "-w"

struct S2 {
  short g_34;
  int g_74[7];
  int g_86;
  char16 g_95;
  int g_124[4];
};

__kernel void mem2reg_store(__global ulong *result) {
  struct S2 c_640;
  struct S2 *p_639 = &c_640;
  struct S2 c_641 = {5, {}, (int)p_639, 0, {}};
  p_639->g_34 = 42;
  result[get_global_id(0)] = p_639->g_34;
}
