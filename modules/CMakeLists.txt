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

# Create an internally cached variable to store a list of include directories
# for modules which do not export a target, such as header only libraries.
# These modules must append to this string so that the include directories can
# be propagated throughout the project.
set(MODULES_INCLUDE_DIRS # No default value
  CACHE INTERNAL "List of module include directories.")

# Create an internally cached variable to store all module libraries. Modules
# must append to this list so that the libraries can be propagated throughout
# the project.
set(MODULES_LIBRARIES # No default value
  CACHE INTERNAL "List of module libraries.")


# Fetch the official SPIR-V headers. These are required by some up our public
# compiler headers, which are also included by mux, so this must take place
# before including both mux and compiler modules.
include(FetchContent)
FetchContent_Declare(
  SPIRV-Headers
  GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers.git
  GIT_TAG        sdk-1.3.239.0
)
FetchContent_MakeAvailable(SPIRV-Headers)

if(CA_NATIVE_CPU)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cargo)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/compiler)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/metadata)
else()
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cargo)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/loader)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tracer)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/debug)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/utils)

if(NOT spirv-headers_POPULATED)
  message(FATAL_ERROR "No SPIRV-Headers found.")
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mux)
# compiler/builtins requires capabilities from ComputeMux targets.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/compiler)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/metadata)
if(CA_ENABLE_TESTS)
  # mux tests depend on builtins.
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/mux/test)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/kts)
endif()
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lit)

foreach(LIBRARY ${MODULES_LIBRARIES})
  target_include_directories(${LIBRARY} PUBLIC ${MODULES_INCLUDE_DIRS})
endforeach()
endif() #CA_NATIVE_CPU
