// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// This is reduced from barrier.13_barrier_shift_loop_reduced.cl

// DESCRIPTION OF HOW THIS TEST FAILED
//
// This splits into two kernels. The second kernel is repeatedly called and the
// first is only called once. All of the looping is done by repeatedly calling
// the second kernel until 0 is returned. From now on we'll discuss only the
// second kernel.
//
// The x and y values are saved in %barrier_shift_loop_live_mem_info.1 at
// locations 0(x) and 2(y). These are loaded at the start of the second kernel.
//
// On a normal iteration, it increments x and checks if it is < 2. If so it
// stores incremented x and the y that is loaded.
//
// If not it increments y and checks for < 2. Although it does store y at this
// point it is to another part of the live variables which is not loaded next
// time through the kernel and is only used further down this function.
//
// If it is < 2 we set x to 0 and go around the loop where x is checked again
// and then it branches off to the end. In this case it stores the original
// value of y before the increment. Therefore we get an infinite loop. In
// summary the incremented value of y never gets stored to a place that gets
// reloaded in.

__kernel void barrier_shift_loop_reduced(__global uchar *dst) {
  for (int y = 0; y < 2; ++y) {
    for (int x = 0; x < 2; ++x) {
      // This barrier does not actually have any affect, but its presence is
      // key to triggering the bug.
      barrier(CLK_LOCAL_MEM_FENCE);

      *dst = (uchar)23;
    }
  }
}
