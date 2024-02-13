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

set(VK_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(VK_TEST_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/UnitVK)

get_ock_check_name(unitvk_check_name UnitVK)
get_ock_check_name(spirv_ll_check_name spirv-ll-lit)
add_ca_check_group(vk NOGLOBAL
  DEPENDS ${unitvk_check_name} ${spirv_ll_check_name})
