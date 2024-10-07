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

/// @file
///
/// @brief Common code shared across examples

#ifndef CL_COMMON_EXAMPLES_H_INCLUDED
#define CL_COMMON_EXAMPLES_H_INCLUDED

#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IS_CL_SUCCESS(X)                                                   \
  {                                                                        \
    const cl_int ret_val = X;                                              \
    if (CL_SUCCESS != ret_val) {                                           \
      (void)fprintf(stderr, "OpenCL error occurred: %s returned %d\n", #X, \
                    ret_val);                                              \
      exit(1);                                                             \
    }                                                                      \
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

/// @brief Select the OpenCL platform
///
/// If a platform name string is passed on the command-line this is used to
/// select the platform, otherwise if only one platform exists this is chosen.
/// If neither of these cases apply the user is asked which platform to use.
///
/// @param platform_name_arg String of platform name passed on command-line
///
/// @return OpenCL platform selected
cl_platform_id selectPlatform(const char *platform_name_arg) {
  cl_uint num_platforms;
  IS_CL_SUCCESS(clGetPlatformIDs(0, NULL, &num_platforms));

  if (0 == num_platforms) {
    (void)fprintf(stderr, "No OpenCL platforms found, exiting\n");
    exit(1);
  }

  cl_platform_id *platforms =
      (cl_platform_id *)malloc(sizeof(cl_platform_id) * num_platforms);
  if (NULL == platforms) {
    (void)fprintf(stderr, "\nCould not allocate memory for platform ids\n");
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
      char *platform_name = (char *)malloc(platform_name_size);
      if (NULL == platform_name) {
        (void)fprintf(stderr,
                      "\nCould not allocate memory for platform name\n");
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

  cl_platform_id selected_platform_id = platforms[selected_platform];
  free((void *)platforms);
  return selected_platform_id;
}

/// @brief Select the OpenCL device
///
/// If a device name string is passed on the command-line this is used to
/// select the device in the platform, otherwise if only one device exists in
/// the platform this is chosen. If neither of these cases apply the user is
/// asked which device to use from the platform.
///
/// @param selected_platform OpenCL platform to use
/// @param device_name_arg String of device name passed on command-line
///
/// @return OpenCL device selected
cl_device_id selectDevice(cl_platform_id selected_platform,
                          const char *device_name_arg) {
  cl_uint num_devices;

  IS_CL_SUCCESS(clGetDeviceIDs(selected_platform, CL_DEVICE_TYPE_ALL, 0, NULL,
                               &num_devices));

  if (0 == num_devices) {
    (void)fprintf(stderr, "No OpenCL devices found, exiting\n");
    exit(1);
  }

  cl_device_id *devices =
      (cl_device_id *)malloc(sizeof(cl_device_id) * num_devices);
  if (NULL == devices) {
    (void)fprintf(stderr, "\nCould not allocate memory for device ids\n");
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
      char *device_name = (char *)malloc(device_name_size);
      if (NULL == device_name) {
        (void)fprintf(stderr, "\nCould not allocate memory for device name\n");
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

  cl_device_id selected_device_id = devices[selected_device];

  cl_bool device_compiler_available;
  IS_CL_SUCCESS(clGetDeviceInfo(selected_device_id,
                                CL_DEVICE_COMPILER_AVAILABLE, sizeof(cl_bool),
                                &device_compiler_available, NULL));
  if (!device_compiler_available) {
    printf("compiler not available for selected device, skipping example.\n");
    exit(0);
  }

  free((void *)devices);
  return selected_device_id;
}
#endif  // CL_COMMON_EXAMPLES_H_INCLUDED
