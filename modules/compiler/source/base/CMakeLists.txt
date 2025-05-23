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

# If coverage is enabled we add the modules so that the XML file can be
# generated automatically.
if(CA_ENABLE_COVERAGE)
  add_coverage_modules(
    ${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/compiler-base.dir/source

    ${CMAKE_CURRENT_SOURCE_DIR}/include/base
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/compiler-base.dir/source
  )
endif()

set(COMPILER_BASE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/context.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/kernel.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/macros.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/module.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/bit_shift_fixup_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/builtin_simplification_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/check_for_ext_funcs_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/check_for_unsupported_types_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/combine_fpext_fptrunc_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/fast_math_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/image_argument_substitution_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/mem_to_reg_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/pass_pipelines.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/printf_replacement_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/set_convergent_attr_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/software_division_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/program_metadata.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/target.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/base_module_pass_machinery.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/source/context.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/module.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/bit_shift_fixup_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/builtin_simplification_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/check_for_ext_funcs_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/check_for_unsupported_types_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/combine_fpext_fptrunc_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/fast_math_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/image_argument_substitution_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/mem_to_reg_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/pass_pipelines.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/printf_replacement_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/set_convergent_attr_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/software_division_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/program_metadata.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/target.cpp)

add_ca_library(compiler-base STATIC ${COMPILER_BASE_SOURCES} DEPENDS abacus_generate)
target_include_directories(compiler-base PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/modules/compiler/include>)
target_include_directories(compiler-base SYSTEM PUBLIC
  ${spirv-headers_SOURCE_DIR}/include)

if(OCK_IN_LLVM_TREE)
  # When in-tree, we must explicitly specify the clang include directories, as
  # they are not in the same place as the generic LLVM includes, as in a
  # pre-installed LLVM.
  if(NOT "clang" IN_LIST LLVM_ENABLE_PROJECTS
      OR "${LLVM_EXTERNAL_CLANG_SOURCE_DIR}" STREQUAL "")
    message(FATAL_ERROR "The 'clang' project is not enabled")
  endif()
  target_include_directories(compiler-base PUBLIC
    ${LLVM_EXTERNAL_CLANG_SOURCE_DIR}/include)
  # Clang also generates some header files at configure time - we must also
  # include those. There's no direct way of accessing this, so assume some
  # knowledge of the LLVM source tree.
  target_include_directories(compiler-base PUBLIC
    ${LLVM_BINARY_DIR}/tools/clang/include)
endif()

# Explicitly link against the individual static versions of the clang
# libraries, as if clang was built with LLVM_LINK_LLVM_DYLIB=ON, then the
# 'clangCodeGen' library target brings in the 'LLVM' dynamic library. It is
# not safe for us to link with LLVM's dynamic library, due to LLVM's use of
# global/static state.
set(CLANG_LIBS "clangCodeGen" "clangFrontend" "clangDriver"
               "clangParse" "clangSerialization" "clangSema"
               "clangAnalysis" "clangAST" "clangEdit" "clangASTMatchers"
               "clangLex" "clangBasic" "clangSupport" "clangAPINotes"
               "clangBasic")
set(LLVM_LIBS "LLVMCodeGen" "LLVMCoroutines" "LLVMCoverage"
              "LLVMFrontendDriver" "LLVMFrontendHLSL" "LLVMLTO" "LLVMOption"
              "LLVMWindowsDriver")

# In-tree, we should have each of these library targets available.
# Otherwise, we have to provide the full path to them.
if(NOT OCK_IN_LLVM_TREE)
  list(TRANSFORM CLANG_LIBS PREPEND "${LLVM_LIBRARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}")
  list(TRANSFORM CLANG_LIBS APPEND "${CMAKE_STATIC_LIBRARY_SUFFIX}")
endif()

target_link_libraries(compiler-base PUBLIC
  builtins cargo mux spirv-ll compiler-pipeline compiler-binary-metadata vecz
  "${CLANG_LIBS}"
  # Link against version (for clang) on Windows.
  $<$<BOOL:${WIN32}>:version>
  "${LLVM_LIBS}"
)

target_compile_definitions(compiler-base PRIVATE
  # Abacus builtins are only included as part of the .cpp files, so these
  # defines can also be kept PRIVATE.
  ABACUS_ENABLE_OPENCL_1_2_BUILTINS
  ABACUS_ENABLE_OPENCL_3_0_BUILTINS)

set(CA_COMPILER_LLVM_VERSION "${LLVM_PACKAGE_VERSION}")
if(NOT ${BUILD_TYPE_UPPER} MATCHES "RELEASE")
  string(APPEND CA_COMPILER_LLVM_VERSION " (${LLVM_BUILD_TYPE})")
endif()
target_compile_definitions(compiler-base PUBLIC
  CA_COMPILER_LLVM_VERSION=\"${CA_COMPILER_LLVM_VERSION}\")
