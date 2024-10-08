// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <common.h>

static const char *kernel_source =
    "__kernel void vector_addition(__global int *src1, __global int *src2,\n"
    "                              __global int *dst) {\n"
    "  size_t gid = get_global_id(0);\n"
    "  dst[gid] = src1[gid] + src2[gid];\n"
    "}\n";

#define NUM_WORK_ITEMS 64

int main(const int argc, const char **argv) {
  const char *platform_name = NULL;
  const char *device_name = NULL;
  parseArguments(argc, argv, &platform_name, &device_name);

  cl_platform_id selected_platform = selectPlatform(platform_name);
  cl_device_id selected_device = selectDevice(selected_platform, device_name);

  /* Create context */
  cl_int errcode;
  cl_context context = clCreateContext(NULL, /* num_devices */ 1,
                                       &selected_device, NULL, NULL, &errcode);
  IS_CL_SUCCESS(errcode);
  printf(" * Created context\n");

  /* Build program */
  cl_program program = clCreateProgramWithSource(
      context, /* count */ 1, &kernel_source, NULL, &errcode);
  IS_CL_SUCCESS(errcode);

  IS_CL_SUCCESS(clBuildProgram(program, 0, NULL, "", NULL, NULL));
  printf(" * Built program\n");

  /* Create buffers */
  cl_mem src1_buffer =
      clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_int) * NUM_WORK_ITEMS,
                     NULL, &errcode);
  IS_CL_SUCCESS(errcode);
  cl_mem src2_buffer =
      clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_int) * NUM_WORK_ITEMS,
                     NULL, &errcode);
  IS_CL_SUCCESS(errcode);
  cl_mem dst_buffer =
      clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                     sizeof(cl_int) * NUM_WORK_ITEMS, NULL, &errcode);
  IS_CL_SUCCESS(errcode);
  printf(" * Created buffers\n");

  /* Create kernel and set arguments */
  cl_kernel kernel = clCreateKernel(program, "vector_addition", &errcode);
  IS_CL_SUCCESS(errcode);
  IS_CL_SUCCESS(
      clSetKernelArg(kernel, 0, sizeof(src1_buffer), (void *)&src1_buffer));
  IS_CL_SUCCESS(
      clSetKernelArg(kernel, 1, sizeof(src2_buffer), (void *)&src2_buffer));
  IS_CL_SUCCESS(
      clSetKernelArg(kernel, 2, sizeof(dst_buffer), (void *)&dst_buffer));
  printf(" * Created kernel and set arguments\n");

  /* Create command queue */
  cl_command_queue queue =
      clCreateCommandQueue(context, selected_device, 0, &errcode);
  IS_CL_SUCCESS(errcode);
  printf(" * Created command queue\n");

  /* Enqueue source buffer writes */
  cl_int src1[NUM_WORK_ITEMS];
  cl_int src2[NUM_WORK_ITEMS];

  for (size_t i = 0; i < NUM_WORK_ITEMS; ++i) {
    src1[i] = i;
    src2[i] = i + 1;
  }

  IS_CL_SUCCESS(clEnqueueWriteBuffer(queue, src1_buffer, CL_FALSE,
                                     /* offset */ 0, sizeof(src1), src1, 0,
                                     NULL, NULL));
  IS_CL_SUCCESS(clEnqueueWriteBuffer(queue, src2_buffer, CL_FALSE,
                                     /* offset */ 0, sizeof(src2), src2, 0,
                                     NULL, NULL));
  printf(" * Enqueued writes to source buffers\n");

  /* Enqueue kernel */
  size_t global_work_size = NUM_WORK_ITEMS;
  IS_CL_SUCCESS(clEnqueueNDRangeKernel(queue, kernel, /* work_dim */ 1,
                                       /* global_work_offset */ NULL,
                                       &global_work_size, NULL, 0, NULL, NULL));
  printf(" * Enqueued NDRange kernel\n");

  /* Enqueue destination buffer read */
  cl_int dst[NUM_WORK_ITEMS];
  IS_CL_SUCCESS(clEnqueueReadBuffer(queue, dst_buffer, CL_TRUE,
                                    /* offset */ 0, sizeof(dst), dst, 0, NULL,
                                    NULL));
  printf(" * Enqueued read from destination buffer\n");

  /* Check the result */
  // KLOCWORK "UNINIT.STACK.ARRAY.MUST" possible false positive
  // dst is initialized by OpenCL above
  for (size_t i = 0; i < NUM_WORK_ITEMS; ++i) {
    if (dst[i] != src1[i] + src2[i]) {
      printf("Result mismatch for index %zu\n", i);
      printf("Got %d, but expected %d\n", dst[i], src1[i] + src2[i]);
      exit(1);
    }
  }
  printf(" * Result verified\n");

  /* Cleanup */
  IS_CL_SUCCESS(clReleaseCommandQueue(queue));
  IS_CL_SUCCESS(clReleaseKernel(kernel));
  IS_CL_SUCCESS(clReleaseMemObject(src1_buffer));
  IS_CL_SUCCESS(clReleaseMemObject(src2_buffer));
  IS_CL_SUCCESS(clReleaseMemObject(dst_buffer));
  IS_CL_SUCCESS(clReleaseProgram(program));
  IS_CL_SUCCESS(clReleaseContext(context));
  printf(" * Released all created OpenCL objects\n");

  printf("\nExample ran successfully, exiting\n");

  return 0;
}
