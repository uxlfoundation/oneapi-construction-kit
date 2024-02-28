# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

if(NOT CA_CL_ENABLED)
  return()
endif()

# Include our OpenCL CMake machinery.
include(${ComputeAorta_SOURCE_DIR}/source/cl/cmake/AddCACL.cmake)

add_ca_cl_executable(cl_ext_codeplay_example
  ${CMAKE_CURRENT_SOURCE_DIR}/main.c)

if(CA_ENABLE_TESTS)
  add_ca_check_group(cl_ext_codeplay_example)
  get_ock_check_name(example_group_name cl_ext_codeplay_example)

  # For each host device, create a check target running this example on that
  # device.
  foreach(device_name ${host_DEVICE_NAMES})
    # Device names may have spaces and other awkward characters in them -
    # convert to a name containing underscores.
    string(REGEX REPLACE "[^A-Za-z0-9]" "_" underscore_device_name "${device_name}")
    get_ock_check_name(device_example_name "cl_ext_codeplay_example-${underscore_device_name}")
    # Do not automatically add this check to the global check target
    # (NOGLOBAL) nor to the combined CL check target (NOGLOBALCL)
    add_ca_cl_check(cl_ext_codeplay_example-${underscore_device_name}
      NOGLOBAL NOGLOBALCL
      COMMAND cl_ext_codeplay_example --platform ComputeAorta --device "${device_name}"
      DEPENDS cl_ext_codeplay_example)
    # Add this device-specific example to the more general group of this
    # example on all devices.
    add_dependencies(${example_group_name} ${device_example_name})
  endforeach()

  # Add this example's group to the CL-specific group
  # Note that the CL-specific isn't a dependency of the global check target,
  # but the example group is.
  get_ock_check_name(check_cl_name cl)
  add_dependencies(${check_cl_name} ${example_group_name})
endif()

install(TARGETS cl_ext_codeplay_example RUNTIME DESTINATION bin)
