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

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  option(CA_VECZ_ONLY "Build Vecz only and not all of ComputeAorta" ON)
  cmake_minimum_required(VERSION 3.4.3)

  project(VECZ)
endif()

if(CA_VECZ_ONLY)
  # LLVM_CMAKE_DIR should have "LLVM-Config.cmake".
  if(NOT LLVM_CMAKE_DIR)
    message(FATAL_ERROR "Please set up llvm cmake directory")
  endif()

  if(NOT EXISTS "${LLVM_CMAKE_DIR}/LLVM-Config.cmake")
    message(FATAL_ERROR "LLVM-Config.cmake does not exist in ${LLVM_CMAKE_DIR}.")
  endif()

  list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

  include(LLVMConfig)
  include(AddLLVM)
  include(HandleLLVMOptions)

  add_definitions(${LLVM_DEFINITIONS})
  link_directories("${LLVM_LIBRARY_DIRS}")
endif()

set(VECZ_PUBLIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(VECZ_PRIVATE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(VECZ_PRIVATE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source/include)

# If the online coverage is enabled we add the modules so that the XML file
# can be generated automatically.
if(${CA_ENABLE_COVERAGE} AND ${CA_RUNTIME_COMPILER_ENABLED})
  add_coverage_modules(${VECZ_PRIVATE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/vecz.dir/source
    ${VECZ_PRIVATE_SOURCE_DIR}/analysis
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/vecz.dir/source/analysis
    ${VECZ_PRIVATE_SOURCE_DIR}/transform
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/vecz.dir/source/transform
    ${VECZ_PUBLIC_INCLUDE_DIR}/vecz
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/vecz.dir/source
    ${VECZ_PRIVATE_INCLUDE_DIR}/analysis
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/vecz.dir/source/analysis
    ${VECZ_PRIVATE_INCLUDE_DIR}/transform
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/vecz.dir/source/transform
    ${VECZ_PRIVATE_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/vecz.dir/source)
endif()

set(COMMON_SRCS
  ${VECZ_PUBLIC_INCLUDE_DIR}/vecz/pass.h
  ${VECZ_PUBLIC_INCLUDE_DIR}/vecz/vecz_choices.h
  ${VECZ_PUBLIC_INCLUDE_DIR}/vecz/vecz_target_info.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/analysis/control_flow_analysis.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/analysis/divergence_analysis.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/analysis/instantiation_analysis.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/analysis/liveness_analysis.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/analysis/packetization_analysis.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/analysis/simd_width_analysis.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/analysis/stride_analysis.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/analysis/uniform_value_analysis.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/analysis/vectorizable_function_analysis.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/analysis/vectorization_unit_analysis.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/transform/common_gep_elimination_pass.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/transform/control_flow_conversion_pass.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/transform/inline_post_vectorization_pass.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/transform/instantiation_pass.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/transform/interleaved_group_combine_pass.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/transform/packetization_helpers.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/transform/packetization_pass.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/transform/packetizer.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/transform/passes.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/transform/printf_scalarizer.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/transform/scalarization_pass.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/transform/scalarizer.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/transform/ternary_transform_pass.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/control_flow_boscc.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/control_flow_roscc.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/debugging.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/ir_cleanup.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/llvm_helpers.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/memory_operations.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/offset_info.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/reachability.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/simd_packet.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/vectorization_context.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/vectorization_helpers.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/vectorization_heuristics.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/vectorization_unit.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/vectorizer.h
  ${VECZ_PRIVATE_INCLUDE_DIR}/vecz_pass_builder.h
  ${VECZ_PRIVATE_SOURCE_DIR}/analysis/control_flow_analysis.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/analysis/divergence_analysis.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/analysis/instantiation_analysis.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/analysis/liveness_analysis.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/analysis/packetization_analysis.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/analysis/simd_width_analysis.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/analysis/stride_analysis.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/analysis/uniform_value_analysis.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/analysis/vectorizable_function_analysis.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/analysis/vectorization_unit_analysis.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/basic_mem2reg_pass.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/builtin_inlining_pass.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/common_gep_elimination_pass.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/control_flow_conversion_pass.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/inline_post_vectorization_pass.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/loop_rotate_custom_pass.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/instantiation_pass.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/interleaved_group_combine_pass.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/packetization_helpers.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/packetization_pass.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/packetizer.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/passes.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/pre_linearize_pass.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/printf_scalarizer.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/remove_intptr_pass.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/scalarization_pass.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/scalarizer.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/simplify_infinite_loop_pass.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/squash_small_vectors_pass.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/ternary_transform_pass.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/transform/uniform_reassociation_pass.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/control_flow_boscc.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/control_flow_roscc.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/debugging.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/ir_cleanup.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/llvm_helpers.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/memory_operations.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/offset_info.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/pass.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/reachability.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/simd_packet.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/vector_target_info.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/vector_target_info_arm.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/vector_target_info_riscv.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/vectorization_choices.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/vectorization_context.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/vectorization_helpers.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/vectorization_heuristics.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/vectorization_unit.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/vectorizer.cpp
  ${VECZ_PRIVATE_SOURCE_DIR}/vecz_pass_builder.cpp
)

if(MSVC)
  # Disable: unreferenced formal parameter.
  list(REMOVE_ITEM VECZ_COMPILE_OPTIONS -we4100)
  list(APPEND VECZ_COMPILE_OPTIONS -wd4100)
endif()

if(CA_VECZ_ONLY)
  if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_llvm_loadable_module(vecz ${COMMON_SRCS})
  else()
    add_llvm_library(vecz ${COMMON_SRCS})
    target_include_directories(vecz PUBLIC
      "${LLVM_INCLUDE_DIRS}")
  endif()
else()
  # We want a static library for linking with libOpenCL
  add_ca_library(vecz STATIC ${MODULES_LIBRARY_TYPE} ${COMMON_SRCS})
  target_include_directories(vecz PUBLIC
    "${LLVM_INCLUDE_DIRS}")
  target_include_directories(vecz SYSTEM PUBLIC
    ${CA_LLVM_INSTALL_DIR}/include)

  llvm_map_components_to_libnames(LLVM_LIBS
    support core analysis instcombine transformutils scalaropts ipo passes)
endif()

target_include_directories(vecz
  PUBLIC ${VECZ_PUBLIC_INCLUDE_DIR}
  PRIVATE ${VECZ_PRIVATE_INCLUDE_DIR})
target_compile_options(vecz PRIVATE ${VECZ_COMPILE_OPTIONS})
target_compile_definitions(vecz PRIVATE
  ${VECZ_COMPILE_DEFINITIONS})

target_link_libraries(vecz PRIVATE compiler-pipeline multi_llvm PUBLIC ${LLVM_LIBS})

# intrinsics_gen uses tablegen to generate Attributes.inc, which is
# (recursively) included by 'llvm/IR/Module.h', therefore this module depends
# on that target.
if(TARGET intrinsics_gen)
  add_dependencies(vecz intrinsics_gen)
endif()

if(NOT CA_VECZ_ONLY)
  add_subdirectory(tools)
  if(CA_ENABLE_TESTS)
    add_subdirectory(test)
  endif()
endif()

# Append to the list of module libraries, the cache MUST be updated.
list(APPEND MODULES_LIBRARIES vecz)
set(MODULES_LIBRARIES ${MODULES_LIBRARIES}
  CACHE INTERNAL "List of module libraries.")
