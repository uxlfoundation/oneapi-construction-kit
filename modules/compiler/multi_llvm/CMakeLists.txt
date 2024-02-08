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

add_ca_interface_library(multi_llvm)

target_include_directories(multi_llvm INTERFACE
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# If we are in-tree LLVM then we already have the include_directories available
# for LLVM_INCLUDE_DIR. Otherwise they are not set regardless of which LLVM
# targets we depend on, so we set it as part of the interface for multi_llvm
# which all compiler libraries that need llvm depend on.
# TODO: Review whether there is a better way to do this.
if (NOT OCK_IN_LLVM_TREE)
  target_include_directories(multi_llvm SYSTEM INTERFACE
    ${LLVM_INCLUDE_DIR})
endif()

# No need to append interface libraries to MODULES_LIBRARIES variable
