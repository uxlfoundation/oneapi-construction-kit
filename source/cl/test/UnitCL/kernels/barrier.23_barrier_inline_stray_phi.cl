// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// The InlineFunctionsWithBarriers() function of HandleBarriersPass.cpp can in
// certain circumstances leave a dangling PHI node with no uses. This used to
// cause a crash in the DemoteRegToMem() function. [CA-2389]
//
// Removed inline from this function due to the clang SPIR generator dropping
// the function body when its present, this is a bug in the generator, this is
// a work around and retains the function body.
int WeirdPhi(const bool copy) {
  int result = 0;
  for (int k = 0; k < 2; ++k) {
    if (copy) {
      result = k;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
  }
  return result;
}

__kernel void barrier_inline_stray_phi(__global int* out) {
  out[get_global_id(0)] = WeirdPhi(true);
}
