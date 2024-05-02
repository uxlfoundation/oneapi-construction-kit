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

add_ca_library(compiler-pipeline STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/StructTypeRemapper.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/add_kernel_wrapper_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/add_scheduling_parameters_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/address_spaces.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/align_module_structs_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/attributes.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/barrier_regions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/builtin_info.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/cl_builtin_info.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/compute_local_memory_usage_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/define_mux_builtins_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/define_mux_dma_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/degenerate_sub_group_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/device_info.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/dma.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/encode_builtin_range_metadata_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/encode_kernel_metadata_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/fixup_calling_convention_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/gdb_registration_listener.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/group_collective_helpers.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/link_builtins_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/llvm_global_mutex.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/lower_to_mux_builtins_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/make_function_name_unique_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/manual_type_legalization_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/mangling.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/memory_buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/metadata.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/optimal_builtin_replacement_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/pass_functions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/pass_machinery.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/pipeline_parse_helpers.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/prepare_barriers_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/reduce_to_function_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/remove_exceptions_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/remove_lifetime_intrinsics_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/rename_builtins_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/replace_address_space_qualifier_functions_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/replace_atomic_funcs_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/replace_c11_atomic_funcs_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/replace_local_module_scope_variables_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/replace_mem_intrinsics_pass.h  
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/replace_mux_math_decls_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/replace_target_ext_tys_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/replace_wgc_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/scheduling.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/simple_callback_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/sub_group_analysis.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/sub_group_usage_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/target_extension_types.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/unique_opaque_structs_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/vectorization_factor.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/verify_reqd_sub_group_size_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compiler/utils/work_item_loops_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/add_kernel_wrapper_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/add_scheduling_parameters_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/align_module_structs_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/attributes.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/barrier_regions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/builtin_info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/cl_builtin_info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/compute_local_memory_usage_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/define_mux_builtins_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/define_mux_dma_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/degenerate_sub_group_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/dma.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/encode_builtin_range_metadata_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/encode_kernel_metadata_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/fixup_calling_convention_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/gdb_registration_listener.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/group_collective_helpers.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/link_builtins_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/llvm_global_mutex.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/lower_to_mux_builtins_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/make_function_name_unique_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/mangling.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/manual_type_legalization_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/metadata.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/mux_builtin_info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/optimal_builtin_replacement_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/pass_functions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/pass_machinery.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/prepare_barriers_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/reduce_to_function_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/remove_exceptions_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/remove_lifetime_intrinsics_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/rename_builtins_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/replace_address_space_qualifier_functions_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/replace_atomic_funcs_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/replace_c11_atomic_funcs_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/replace_local_module_scope_variables_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/replace_mem_intrinsics_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/replace_mux_math_decls_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/replace_target_ext_tys_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/replace_wgc_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/scheduling.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/sub_group_analysis.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/target_extension_types.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/unique_opaque_structs_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/verify_reqd_sub_group_size_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/work_item_loops_pass.cpp)

target_include_directories(compiler-pipeline PUBLIC
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_compile_definitions(compiler-pipeline PRIVATE
  $<$<BOOL:${CA_PLATFORM_LINUX}>:CA_PLATFORM_LINUX>
  $<$<BOOL:${CA_PLATFORM_WINDOWS}>:CA_PLATFORM_WINDOWS>
  $<$<BOOL:${CA_PLATFORM_MAC}>:CA_PLATFORM_MAC>
  $<$<BOOL:${CA_PLATFORM_ANDROID}>:CA_PLATFORM_ANDROID>
  $<$<BOOL:${CA_PLATFORM_QNX}>:CA_PLATFORM_QNX>)

target_link_libraries(compiler-pipeline PUBLIC
 multi_llvm LLVMPasses LLVMTransformUtils)
if(TARGET LLVMCore)
  target_link_libraries(compiler-pipeline PUBLIC LLVMCore)
endif()
