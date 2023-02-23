// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Note that this requires that 'source' is 32-byte aligned because it will be
// cast to a short16 pointer.  The spec does not mandate this, but this same
// assumption is present in the CTS test that this was reduced from:
//   * conformance_test_relationals shuffle_array_cast
//
// Note that this assumption would definitely break if get_global_id were used
// as the array index.
__kernel void shuffle_cast(__global const short *source,
                           __global short8 *dest) {
  short8 tmp = (short8)((short)0);
  tmp.s3 = ((__global const short16 *)source)[0].s0;
  dest[get_global_id(0)] = tmp;
}
