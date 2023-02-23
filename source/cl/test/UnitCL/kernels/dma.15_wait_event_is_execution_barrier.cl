// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void wait_event_is_execution_barrier(
    __local int *tmpA, __local int *tmpB, __local int *tmpADash,
    __local int *tmpBDash, __global const int *A, __global const int *B,
    __global int *C, __global int *D) {
  size_t gid = get_global_id(0);
  size_t lid = get_local_id(0);
  size_t group = get_group_id(0);
  size_t size = get_local_size(0);
  event_t event;

  // Just do some meaningless copy from A to tmpA, we don't care much about it,
  // mostly it is just to give an an event_t to wait on.
  event = async_work_group_copy(tmpA, &A[group * size], size, 0);

  // But do an equivalent copy from B to tmpB using a direct memory access.
  tmpB[lid] = B[gid];

  // Now we wait on the event, which means that tmpA is populated, but more
  // importantly for this test it should mean that tmpB is populated for the
  // entire work-group as this is an execution barrier.  Add the memfence as
  // the spec says nothing about wait_group_events acts as a fence in the same
  // way as a barrier does.
  wait_group_events(1, &event);
  mem_fence(CLK_LOCAL_MEM_FENCE);

  // Do a rotate of tmpA/tmpB.  This will only work if all work-items have done
  // their copying, i.e. wait_group_events is a barrier.  We need to do a
  // barrier afterwards to ensure that every work item has done this.
  tmpADash[lid] = tmpA[(lid + 1) % size];
  tmpBDash[lid] = tmpB[(lid + 1) % size];
  barrier(CLK_LOCAL_MEM_FENCE);

  // Copy out the rotated results and wait on the DMA to complete.  The barrier
  // property of this wait is not interesting, as the kernel scope ending is a
  // barrier itself.
  event = async_work_group_copy(&C[group * size], tmpADash, size, 0);
  D[gid] = tmpBDash[lid];
  wait_group_events(1, &event);
}
