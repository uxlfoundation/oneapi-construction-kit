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

configure_file(include/ur/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/ur/config.h)

add_ca_subdirectory(external)

add_ca_library(UR SHARED
  # Utils
  ${CMAKE_CURRENT_BINARY_DIR}/include/ur/config.h
  include/ur/base.h
  include/ur/info.h
  include/ur/mux.h
  # API
  include/ur/context.h source/context.cpp
  include/ur/device.h source/device.cpp
  include/ur/event.h source/event.cpp
  include/ur/kernel.h source/kernel.cpp
  include/ur/memory.h source/memory.cpp
  include/ur/platform.h source/platform.cpp
  include/ur/program.h source/program.cpp
  include/ur/queue.h source/queue.cpp
  source/ddi.cpp)

target_include_directories(UR PRIVATE include
  ${CMAKE_CURRENT_BINARY_DIR}/include)
target_link_libraries(UR PUBLIC unified-runtime-headers)
target_link_libraries(UR PRIVATE cargo mux compiler-loader)
target_resources(UR NAMESPACES ${BUILTINS_NAMESPACES})

if(CA_ENABLE_TESTS)
  add_ca_subdirectory(test)
endif()

add_ca_subdirectory(examples)
