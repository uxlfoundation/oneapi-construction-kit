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

add_ca_executable(veczc ${CMAKE_CURRENT_SOURCE_DIR}/source/veczc.cpp)
target_compile_options(veczc PRIVATE ${VECZ_COMPILE_OPTIONS})
target_compile_definitions(veczc PRIVATE ${VECZ_COMPILE_DEFINITIONS})
target_include_directories(veczc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

llvm_map_components_to_libnames(llvm_libs all ${LLVM_TARGETS_TO_BUILD})
# LLVM 8 adds these invalid libraries to the list, remove them to avoid
# attempting to link against LTO-NOTFOUND and OptRemarks-NOTFOUND.
list(REMOVE_ITEM llvm_libs LTO OptRemarks)
target_link_libraries(veczc PUBLIC vecz multi_llvm compiler-pipeline ${llvm_libs})
install(TARGETS veczc RUNTIME DESTINATION bin COMPONENT VECZ)
