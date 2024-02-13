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

// TODO(oneapi-src/unified-runtime#42): Workaround header issue where bool is
// not defined for C.
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ur_api.h"

#define IS_UR_SUCCESS(X)                                                      \
  {                                                                           \
    const ur_result_t ret_val = X;                                            \
    if (UR_RESULT_SUCCESS != ret_val) {                                       \
      (void)fprintf(stderr,                                                   \
                    "Unified Runtime error occurred: %s returned 0x%x\n", #X, \
                    ret_val);                                                 \
      exit(1);                                                                \
    }                                                                         \
  }

/// @brief Print help message on executable usage
///
/// @param arg0 Name of executable
void printUsage(const char *arg0) {
  printf("usage: %s [-h] [--platform <name>] [--device <name>]\n", arg0);
}

/// @brief Parse executable arguments for platform and device name
///
/// Found platform and device names are returned as C-string output
/// parameters. If --help / -h is passed as an argument the help
/// message is printed and the application exits with success.
///
/// @param[in] argc Number of arguments propagated from main()
/// @param[in] argv Command-line arguments propagated from main()
/// @param[out] platform_name Platform name found from --platform argument
/// @param[out] device_name Device name found from --device argument
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
        (void)fprintf(stderr, "expected platform name\n");
        exit(1);
      }
      *platform_name = argv[argi];
    } else if (0 == strcmp("--device", argv[argi])) {
      argi++;
      if (argi == argc) {
        printUsage(argv[0]);
        (void)fprintf(stderr, "error: expected device name\n");
        exit(1);
      }
      *device_name = argv[argi];
    } else {
      printUsage(argv[0]);
      (void)fprintf(stderr, "error: invalid argument: %s\n", argv[argi]);
      exit(1);
    }
  }
}

/// @brief Select the Unified Runtime platform
///
/// If a platform name string is passed on the command-line this is used to
/// select the platform, otherwise if only one platform exists this is chosen.
/// If neither of these cases apply the user is asked which platform to use.
///
/// @param platform_name_arg String of platform name passed on command-line
///
/// @return Unified Runtime platform selected
ur_platform_handle_t selectPlatform(const char *platform_name_arg) {
  uint32_t num_platforms = 0;
  IS_UR_SUCCESS(urPlatformGet(0, NULL, &num_platforms));

  if (0 == num_platforms) {
    (void)fprintf(stderr, "No Unified Runtime platforms found, exiting\n");
    exit(1);
  }

  ur_platform_handle_t *platforms = (ur_platform_handle_t *)malloc(
      sizeof(ur_platform_handle_t) * num_platforms);
  if (NULL == platforms) {
    (void)fprintf(stderr, "\nCould not allocate memory for platform ids\n");
    exit(1);
  }
  IS_UR_SUCCESS(urPlatformGet(num_platforms, platforms, NULL));

  printf("Available platforms are:\n");

  unsigned selected_platform = 0;
  for (uint32_t i = 0; i < num_platforms; ++i) {
    size_t platform_name_size;
    IS_UR_SUCCESS(urPlatformGetInfo(platforms[i], UR_PLATFORM_INFO_NAME, 0,
                                    NULL, &platform_name_size));

    if (0 == platform_name_size) {
      printf("  %u. Nameless platform\n", i + 1);
    } else {
      char *platform_name = (char *)malloc(platform_name_size);
      if (NULL == platform_name) {
        (void)fprintf(stderr,
                      "\nCould not allocate memory for platform name\n");
        exit(1);
      }
      IS_UR_SUCCESS(urPlatformGetInfo(platforms[i], UR_PLATFORM_INFO_NAME,
                                      platform_name_size, platform_name, NULL));
      printf("  %u. %s\n", i + 1, platform_name);
      if (platform_name_arg && 0 == strcmp(platform_name, platform_name_arg)) {
        selected_platform = i + 1;
      }
      free(platform_name);
    }
  }

  if (platform_name_arg != NULL && selected_platform == 0) {
    (void)fprintf(stderr, "Platform name matching '--platform %s' not found\n",
                  platform_name_arg);
    exit(1);
  }

  if (1 == num_platforms) {
    printf("\nSelected platform 1\n");
    selected_platform = 1;
  } else if (0 != selected_platform) {
    printf("\nSelected platform %u by '--platform %s'\n", selected_platform,
           platform_name_arg);
  } else {
    printf("\nPlease select a platform: ");
    if (1 != scanf("%u", &selected_platform)) {
      (void)fprintf(stderr, "\nCould not parse provided input, exiting\n");
      exit(1);
    }
  }

  selected_platform -= 1;

  if (num_platforms <= selected_platform) {
    (void)fprintf(stderr, "\nSelected unknown platform, exiting\n");
    exit(1);
  } else {
    printf("\nRunning example on platform %u\n", selected_platform + 1);
  }

  ur_platform_handle_t selected_platform_id = platforms[selected_platform];
  free(platforms);
  return selected_platform_id;
}

/// @brief Select the Unified Runtime device
///
/// If a device name string is passed on the command-line this is used to
/// select the device in the platform, otherwise if only one device exists in
/// the platform this is chosen. If neither of these cases apply the user is
/// asked which device to use from the platform.
///
/// @param selected_platform Unified Runtime platform to use
/// @param device_name_arg String of device name passed on command-line
///
/// @return Unified Runtime device selected
ur_device_handle_t selectDevice(ur_platform_handle_t selected_platform,
                                const char *device_name_arg) {
  uint32_t num_devices = 0;

  IS_UR_SUCCESS(urDeviceGet(selected_platform, UR_DEVICE_TYPE_ALL, 0, NULL,
                            &num_devices));

  if (0 == num_devices) {
    (void)fprintf(stderr, "No Unified Runtime devices found, exiting\n");
    exit(1);
  }

  ur_device_handle_t *devices =
      (ur_device_handle_t *)malloc(sizeof(ur_device_handle_t) * num_devices);
  if (NULL == devices) {
    (void)fprintf(stderr, "\nCould not allocate memory for device ids\n");
    exit(1);
  }
  IS_UR_SUCCESS(urDeviceGet(selected_platform, UR_DEVICE_TYPE_ALL, num_devices,
                            devices, NULL));

  printf("Available devices are:\n");

  unsigned selected_device = 0;
  for (uint32_t i = 0; i < num_devices; ++i) {
    size_t device_name_size;
    IS_UR_SUCCESS(urDeviceGetInfo(devices[i], UR_DEVICE_INFO_NAME, 0, NULL,
                                  &device_name_size));

    if (0 == device_name_size) {
      printf("  %u. Nameless device\n", i + 1);
    } else {
      char *device_name = (char *)malloc(device_name_size);
      if (NULL == device_name) {
        (void)fprintf(stderr, "\nCould not allocate memory for device name\n");
        exit(1);
      }
      IS_UR_SUCCESS(urDeviceGetInfo(devices[i], UR_DEVICE_INFO_NAME,
                                    device_name_size, device_name, NULL));
      printf("  %u. %s\n", i + 1, device_name);
      if (device_name_arg && 0 == strcmp(device_name, device_name_arg)) {
        selected_device = i + 1;
      }
      free(device_name);
    }
  }

  if (device_name_arg != NULL && selected_device == 0) {
    (void)fprintf(stderr, "Device name matching '--device %s' not found\n",
                  device_name_arg);
    exit(1);
  }

  if (1 == num_devices) {
    printf("\nSelected device 1\n");
    selected_device = 1;
  } else if (0 != selected_device) {
    printf("\nSelected device %u by '--device %s'\n", selected_device,
           device_name_arg);
  } else {
    printf("\nPlease select a device: ");
    if (1 != scanf("%u", &selected_device)) {
      (void)fprintf(stderr, "\nCould not parse provided input, exiting\n");
      exit(1);
    }
  }

  selected_device -= 1;

  if (num_devices <= selected_device) {
    (void)fprintf(stderr, "\nSelected unknown device, exiting\n");
    exit(1);
  } else {
    printf("\nRunning example on device %u\n", selected_device + 1);
  }

  ur_device_handle_t selected_device_id = devices[selected_device];

  bool device_compiler_available;
  size_t bool_size = sizeof(bool);
  IS_UR_SUCCESS(urDeviceGetInfo(selected_device_id,
                                UR_DEVICE_INFO_COMPILER_AVAILABLE, bool_size,
                                &device_compiler_available, NULL));
  if (!device_compiler_available) {
    printf("compiler not available for selected device, skipping example.\n");
    exit(0);
  }

  free(devices);
  return selected_device_id;
}

// Generated from the following OpenCL C:
//
// kernel void vector_addition(global int *src1, global int *src2,
//                             global int *dst) {
//   size_t gid = get_global_id(0);
//   dst[gid] = src1[gid] + src2[gid];
// }
static const uint8_t kernel_source[] = {
    0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x0e, 0x00, 0x06, 0x00,
    0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x11, 0x00, 0x02, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
    0x0b, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x4f, 0x70, 0x65, 0x6e, 0x43, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x00, 0x00,
    0x0e, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x0f, 0x00, 0x08, 0x00, 0x06, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
    0x76, 0x65, 0x63, 0x74, 0x6f, 0x72, 0x5f, 0x61, 0x64, 0x64, 0x69, 0x74,
    0x69, 0x6f, 0x6e, 0x00, 0x05, 0x00, 0x00, 0x00, 0x07, 0x00, 0x0e, 0x00,
    0x17, 0x00, 0x00, 0x00, 0x6b, 0x65, 0x72, 0x6e, 0x65, 0x6c, 0x5f, 0x61,
    0x72, 0x67, 0x5f, 0x74, 0x79, 0x70, 0x65, 0x2e, 0x76, 0x65, 0x63, 0x74,
    0x6f, 0x72, 0x5f, 0x61, 0x64, 0x64, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x2e,
    0x69, 0x6e, 0x74, 0x2a, 0x2c, 0x69, 0x6e, 0x74, 0x2a, 0x2c, 0x69, 0x6e,
    0x74, 0x2a, 0x2c, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00,
    0xa0, 0x86, 0x01, 0x00, 0x05, 0x00, 0x0b, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x5f, 0x5f, 0x73, 0x70, 0x69, 0x72, 0x76, 0x5f, 0x42, 0x75, 0x69, 0x6c,
    0x74, 0x49, 0x6e, 0x47, 0x6c, 0x6f, 0x62, 0x61, 0x6c, 0x49, 0x6e, 0x76,
    0x6f, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x49, 0x64, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x73, 0x72, 0x63, 0x31,
    0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00,
    0x73, 0x72, 0x63, 0x32, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x03, 0x00,
    0x0d, 0x00, 0x00, 0x00, 0x64, 0x73, 0x74, 0x00, 0x05, 0x00, 0x04, 0x00,
    0x0e, 0x00, 0x00, 0x00, 0x65, 0x6e, 0x74, 0x72, 0x79, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x63, 0x61, 0x6c, 0x6c,
    0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x11, 0x00, 0x00, 0x00,
    0x61, 0x72, 0x72, 0x61, 0x79, 0x69, 0x64, 0x78, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x05, 0x00, 0x13, 0x00, 0x00, 0x00, 0x61, 0x72, 0x72, 0x61,
    0x79, 0x69, 0x64, 0x78, 0x31, 0x00, 0x00, 0x00, 0x05, 0x00, 0x03, 0x00,
    0x15, 0x00, 0x00, 0x00, 0x61, 0x64, 0x64, 0x00, 0x05, 0x00, 0x05, 0x00,
    0x16, 0x00, 0x00, 0x00, 0x61, 0x72, 0x72, 0x61, 0x79, 0x69, 0x64, 0x78,
    0x32, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x0b, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x47, 0x00, 0x03, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
    0x0b, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x47, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00,
    0x26, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x47, 0x00, 0x0d, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x5f, 0x5f, 0x73, 0x70,
    0x69, 0x72, 0x76, 0x5f, 0x42, 0x75, 0x69, 0x6c, 0x74, 0x49, 0x6e, 0x47,
    0x6c, 0x6f, 0x62, 0x61, 0x6c, 0x49, 0x6e, 0x76, 0x6f, 0x63, 0x61, 0x74,
    0x69, 0x6f, 0x6e, 0x49, 0x64, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x15, 0x00, 0x04, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x21, 0x00, 0x06, 0x00, 0x09, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x09, 0x00, 0x00, 0x00, 0x37, 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x0b, 0x00, 0x00, 0x00, 0x37, 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x00, 0x00, 0x37, 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x0d, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x0e, 0x00, 0x00, 0x00,
    0x3d, 0x00, 0x04, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x46, 0x00, 0x05, 0x00, 0x08, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00,
    0x0b, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x06, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x46, 0x00, 0x05, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x06, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x14, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x15, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00,
    0x46, 0x00, 0x05, 0x00, 0x08, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00,
    0x0d, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x05, 0x00,
    0x16, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00};

const size_t kernel_source_length =
    sizeof(kernel_source) / sizeof(kernel_source[0]);

#define NUM_WORK_ITEMS 64

int main(const int argc, const char **argv) {
  /* Initialize the drivers */
  IS_UR_SUCCESS(urInit(0));
  printf(" * Uniform Runtime initialized\n");

  const char *platform_name = NULL;
  const char *device_name = NULL;
  parseArguments(argc, argv, &platform_name, &device_name);

  ur_platform_handle_t selected_platform = selectPlatform(platform_name);
  ur_device_handle_t selected_device =
      selectDevice(selected_platform, device_name);

  /* Create context */
  ur_context_handle_t context;
  IS_UR_SUCCESS(
      urContextCreate(/* num_devices */ 1, &selected_device, NULL, &context));
  printf(" * Created context\n");

  /* Create and build program */
  ur_program_handle_t program;
  IS_UR_SUCCESS(urProgramCreateWithIL(context, kernel_source,
                                      kernel_source_length, NULL, &program));
  printf(" * Created program\n");

  IS_UR_SUCCESS(urProgramBuild(context, program, NULL));
  printf(" * Built program\n");

  /* Create buffers */
  ur_mem_handle_t src1_buffer;
  IS_UR_SUCCESS(urMemBufferCreate(context, UR_MEM_FLAG_READ_ONLY,
                                  sizeof(int32_t) * NUM_WORK_ITEMS, NULL,
                                  &src1_buffer));
  ur_mem_handle_t src2_buffer;
  IS_UR_SUCCESS(urMemBufferCreate(context, UR_MEM_FLAG_READ_ONLY,
                                  sizeof(int32_t) * NUM_WORK_ITEMS, NULL,
                                  &src2_buffer));
  ur_mem_handle_t dst_buffer;
  IS_UR_SUCCESS(urMemBufferCreate(context, UR_MEM_FLAG_WRITE_ONLY,
                                  sizeof(int32_t) * NUM_WORK_ITEMS, NULL,
                                  &dst_buffer));
  printf(" * Created buffers\n");

  /* Create kernel and set arguments */
  ur_kernel_handle_t kernel;
  IS_UR_SUCCESS(urKernelCreate(program, "vector_addition", &kernel));
  IS_UR_SUCCESS(urKernelSetArgMemObj(kernel, 0, src1_buffer));
  IS_UR_SUCCESS(urKernelSetArgMemObj(kernel, 1, src2_buffer));
  IS_UR_SUCCESS(urKernelSetArgMemObj(kernel, 2, dst_buffer));
  printf(" * Created kernel and set arguments\n");

  /* Create command queue */
  ur_queue_handle_t queue;
  IS_UR_SUCCESS(urQueueCreate(context, selected_device, NULL, &queue));
  printf(" * Created command queue\n");

  /* Enqueue source buffer writes */
  int32_t src1[NUM_WORK_ITEMS];
  int32_t src2[NUM_WORK_ITEMS];

  for (size_t i = 0; i < NUM_WORK_ITEMS; ++i) {
    src1[i] = i;
    src2[i] = i + 1;
  }

  IS_UR_SUCCESS(urEnqueueMemBufferWrite(queue, src1_buffer, false,
                                        /* offset */ 0, sizeof(src1), src1, 0,
                                        NULL, NULL));
  IS_UR_SUCCESS(urEnqueueMemBufferWrite(queue, src2_buffer, false,
                                        /* offset */ 0, sizeof(src2), src2, 0,
                                        NULL, NULL));
  printf(" * Enqueued writes to source buffers\n");

  /* Enqueue kernel */
  size_t global_work_size = NUM_WORK_ITEMS;
  size_t local_work_size = NUM_WORK_ITEMS / 8;
  ur_event_handle_t event;
  const size_t offsets[] = {0, 0, 0};
  IS_UR_SUCCESS(urEnqueueKernelLaunch(queue, kernel, /* work_dim */ 1, offsets,
                                      &global_work_size, &local_work_size, 0,
                                      NULL, &event));
  printf(" * Enqueued NDRange kernel\n");

  /* Enqueue destination buffer read */
  int32_t dst[NUM_WORK_ITEMS];
  IS_UR_SUCCESS(urEnqueueMemBufferRead(queue, dst_buffer, true,
                                       /* offset */ 0, sizeof(dst), dst, 0,
                                       NULL, NULL));
  printf(" * Enqueued read from destination buffer\n");

  /* Check the result */
  for (size_t i = 0; i < NUM_WORK_ITEMS; ++i) {
    if (dst[i] != src1[i] + src2[i]) {
      printf("Result mismatch for index %zu\n", i);
      printf("Got %d, but expected %d\n", dst[i], src1[i] + src2[i]);
      exit(1);
    }
  }
  printf(" * Result verified\n");

  /* Cleanup */
  IS_UR_SUCCESS(urEventRelease(event));
  IS_UR_SUCCESS(urQueueRelease(queue));
  IS_UR_SUCCESS(urKernelRelease(kernel));
  IS_UR_SUCCESS(urMemRelease(src1_buffer));
  IS_UR_SUCCESS(urMemRelease(src2_buffer));
  IS_UR_SUCCESS(urMemRelease(dst_buffer));
  IS_UR_SUCCESS(urProgramRelease(program));
  IS_UR_SUCCESS(urContextRelease(context));
  printf(" * Released all created Unified Runtime objects\n");

  /* Tear down the drivers */
  ur_tear_down_params_t tear_down_params;
  IS_UR_SUCCESS(urTearDown(&tear_down_params));
  printf(" * Uniform Runtime tear down complete\n");

  printf("\nExample ran successfully, exiting\n");

  return 0;
}
