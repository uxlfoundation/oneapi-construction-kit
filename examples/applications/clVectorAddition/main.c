// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IS_CL_SUCCESS(X)                                                       \
  {                                                                            \
    cl_int ret_val = X;                                                        \
    if (CL_SUCCESS != ret_val) {                                               \
      fprintf(stderr, "OpenCL error occurred: %s returned %d\n", #X, ret_val); \
      exit(1);                                                                 \
    }                                                                          \
  }

static const char *kernel_source =
    "__kernel void vector_addition(__global int *src1, __global int *src2,\n"
    "                              __global int *dst) {\n"
    "  size_t gid = get_global_id(0);\n"
    "  dst[gid] = src1[gid] + src2[gid];\n"
    "}\n";

#define NUM_WORK_ITEMS 64

void printUsage(const char *arg0) {
  printf("usage: %s [-h] [--platform <name>] [--device <name>]\n", arg0);
}

void parseArguments(const int argc, const char **argv,
                    const char **platform_name, const char **device_name) {
  for (int argi = 1; argi < argc; argi++) {
    if (0 == strcmp("-h", argv[argi]) || 0 == strcmp("--help", argv[argi])) {
      printUsage(argv[0]);
      exit(0);
    } else if (0 == strcmp("--platform", argv[argi])) {
      argi++;
      if (argi == argc) {
        printUsage(argv[0]);
        fprintf(stderr, "expected platform name\n");
        exit(1);
      }
      *platform_name = argv[argi];
    } else if (0 == strcmp("--device", argv[argi])) {
      argi++;
      if (argi == argc) {
        printUsage(argv[0]);
        fprintf(stderr, "error: expected device name\n");
        exit(1);
      }
      *device_name = argv[argi];
    } else {
      printUsage(argv[0]);
      fprintf(stderr, "error: invalid argument: %s\n", argv[argi]);
      exit(1);
    }
  }
}

cl_platform_id selectPlatform(const char *platform_name_arg) {
  cl_uint num_platforms;
  IS_CL_SUCCESS(clGetPlatformIDs(0, NULL, &num_platforms));

  if (0 == num_platforms) {
    fprintf(stderr, "No OpenCL platforms found, exiting\n");
    exit(1);
  }

  cl_platform_id *platforms = malloc(sizeof(cl_platform_id) * num_platforms);
  if (NULL == platforms) {
    fprintf(stderr, "\nCould not allocate memory for platform ids\n");
    exit(1);
  }
  IS_CL_SUCCESS(clGetPlatformIDs(num_platforms, platforms, NULL));

  printf("Available platforms are:\n");

  unsigned selected_platform = 0;
  for (cl_uint i = 0; i < num_platforms; ++i) {
    size_t platform_name_size;
    IS_CL_SUCCESS(clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 0, NULL,
                                    &platform_name_size));

    if (0 == platform_name_size) {
      printf("  %u. Nameless platform\n", i + 1);
    } else {
      char *platform_name = malloc(platform_name_size);
      if (NULL == platform_name) {
        fprintf(stderr, "\nCould not allocate memory for platform name\n");
        exit(1);
      }
      IS_CL_SUCCESS(clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME,
                                      platform_name_size, platform_name, NULL));
      printf("  %u. %s\n", i + 1, platform_name);
      if (platform_name_arg && 0 == strcmp(platform_name, platform_name_arg)) {
        selected_platform = i + 1;
      }
      free(platform_name);
    }
  }

  if (platform_name_arg != NULL && selected_platform == 0) {
    fprintf(stderr, "Platform name matching '--platform %s' not found\n",
            platform_name_arg);
    exit(1);
  }

  if (1 == num_platforms) {
    printf("\nSelected platform 1\n");
    selected_platform = 1;
  } else if (0 != selected_platform) {
    printf("\nSelected platform %d by '--platform %s'\n", selected_platform,
           platform_name_arg);
  } else {
    printf("\nPlease select a platform: ");
    if (1 != scanf("%u", &selected_platform)) {
      fprintf(stderr, "\nCould not parse provided input, exiting\n");
      exit(1);
    }
  }

  selected_platform -= 1;

  if (num_platforms <= selected_platform) {
    fprintf(stderr, "\nSelected unknown platform, exiting\n");
    exit(1);
  } else {
    printf("\nRunning example on platform %u\n", selected_platform + 1);
  }

  cl_platform_id selected_platform_id = platforms[selected_platform];
  free(platforms);
  return selected_platform_id;
}

cl_device_id selectDevice(cl_platform_id selected_platform,
                          const char *device_name_arg) {
  cl_uint num_devices;

  IS_CL_SUCCESS(clGetDeviceIDs(selected_platform, CL_DEVICE_TYPE_ALL, 0, NULL,
                               &num_devices));

  if (0 == num_devices) {
    fprintf(stderr, "No OpenCL devices found, exiting\n");
    exit(1);
  }

  cl_device_id *devices = malloc(sizeof(cl_device_id) * num_devices);
  if (NULL == devices) {
    fprintf(stderr, "\nCould not allocate memory for device ids\n");
    exit(1);
  }
  IS_CL_SUCCESS(clGetDeviceIDs(selected_platform, CL_DEVICE_TYPE_ALL,
                               num_devices, devices, NULL));

  printf("Available devices are:\n");

  unsigned selected_device = 0;
  for (cl_uint i = 0; i < num_devices; ++i) {
    size_t device_name_size;
    IS_CL_SUCCESS(clGetDeviceInfo(devices[i], CL_DEVICE_NAME, 0, NULL,
                                  &device_name_size));

    if (0 == device_name_size) {
      printf("  %u. Nameless device\n", i + 1);
    } else {
      char *device_name = malloc(device_name_size);
      if (NULL == device_name) {
        fprintf(stderr, "\nCould not allocate memory for device name\n");
        exit(1);
      }
      IS_CL_SUCCESS(clGetDeviceInfo(devices[i], CL_DEVICE_NAME,
                                    device_name_size, device_name, NULL));
      printf("  %u. %s\n", i + 1, device_name);
      if (device_name_arg && 0 == strcmp(device_name, device_name_arg)) {
        selected_device = i + 1;
      }
      free(device_name);
    }
  }

  if (device_name_arg != NULL && selected_device == 0) {
    fprintf(stderr, "Device name matching '--device %s' not found\n",
            device_name_arg);
    exit(1);
  }

  if (1 == num_devices) {
    printf("\nSelected device 1\n");
    selected_device = 1;
  } else if (0 != selected_device) {
    printf("\nSelected device %d by '--device %s'\n", selected_device,
           device_name_arg);
  } else {
    printf("\nPlease select a device: ");
    if (1 != scanf("%u", &selected_device)) {
      fprintf(stderr, "\nCould not parse provided input, exiting\n");
      exit(1);
    }
  }

  selected_device -= 1;

  if (num_devices <= selected_device) {
    fprintf(stderr, "\nSelected unknown device, exiting\n");
    exit(1);
  } else {
    printf("\nRunning example on device %u\n", selected_device + 1);
  }

  cl_device_id selected_device_id = devices[selected_device];

  cl_bool device_compiler_available;
  IS_CL_SUCCESS(clGetDeviceInfo(selected_device_id,
                                CL_DEVICE_COMPILER_AVAILABLE, sizeof(cl_bool),
                                &device_compiler_available, NULL));
  if (!device_compiler_available) {
    printf("compiler not available for selected device, skipping example.\n");
    exit(0);
  }

  free(devices);
  return selected_device_id;
}

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
  IS_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(src1_buffer), &src1_buffer));
  IS_CL_SUCCESS(clSetKernelArg(kernel, 1, sizeof(src2_buffer), &src2_buffer));
  IS_CL_SUCCESS(clSetKernelArg(kernel, 2, sizeof(dst_buffer), &dst_buffer));
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
