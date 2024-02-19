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

#[=======================================================================[.rst:
.. cmake:command:: add_ca_cl_example

  The ``add_ca_example()`` function creates an executable using
  :cmake:command:`add_ca_cl_executable`, adds a check target, and
  places the target in the install directory.

  Arguments:
    * ``ARGV0`` - Target name for example
    * ``ARGN`` - C/C++ source files

#]=======================================================================]
function(add_ca_cl_example)
  add_ca_cl_executable(${ARGV})
  target_include_directories(${ARGV0} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

  if (CA_ENABLE_TESTS)
    # Create a specific check target for each known device and add those to a
    # group created for that example:
    # * check-ock (and check-ock-cl)
    #   * check-ock-example
    #     * check-ock-example-device0
    #     * check-ock-example-device1
    # This way the example runs on each device when building the global check
    # target, or the example's check target, but each device can be run
    # individually if desired.
    add_ca_check_group(${ARGV0})
    get_ock_check_name(example_group_name ${ARGV0})

    foreach(target ${MUX_TARGET_LIBRARIES})
      foreach(device_name ${${target}_DEVICE_NAMES})
        # Device names may have spaces and other awkward characters in them -
        # convert to a name containing underscores.
        string(REGEX REPLACE "[^A-Za-z0-9]" "_" underscore_device_name "${device_name}")
        # Do not automatically add this check to the global check target
        # (NOGLOBAL) nor to the combined CL check target (NOGLOBALCL)
        get_ock_check_name(device_example_name "${ARGV0}-${underscore_device_name}")
        add_ca_cl_check("${ARGV0}-${underscore_device_name}"
          NOGLOBAL NOGLOBALCL
          COMMAND ${ARGV0} --platform ComputeAorta --device "${device_name}"
          DEPENDS ${ARGV0})

        # Add this device-specific example to the more general group of this
        # example on all devices.
        add_dependencies(${example_group_name} ${device_example_name})
      endforeach()
    endforeach()

    # Add this example's group to the CL-specific group
    # Note that the CL-specific isn't a dependency of the global check target,
    # but the example group is.
    get_ock_check_name(check_cl_name cl)
    add_dependencies(${check_cl_name} ${example_group_name})

  endif()

  install(TARGETS ${ARGV0}
    RUNTIME DESTINATION bin COMPONENT OCLExamples)
endfunction()

add_ca_cl_example(clVectorAddition
    ${CMAKE_CURRENT_SOURCE_DIR}/VectorAddition/source/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/include/common.h)

if(${OCL_EXTENSION_cl_khr_command_buffer})
  add_ca_cl_example(clCommandBufferKHR
    ${CMAKE_CURRENT_SOURCE_DIR}/CommandBufferKHR/source/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/common.h)
endif()

if(${OCL_EXTENSION_cl_khr_command_buffer_mutable_dispatch})
  add_ca_cl_example(clMutableDispatchKHR
    ${CMAKE_CURRENT_SOURCE_DIR}/MutableDispatchKHR/source/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/common.h)
endif()

add_ca_cl_example(clSubGroups
  ${CMAKE_CURRENT_SOURCE_DIR}/SubGroups/source/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/common.h)
