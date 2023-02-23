# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Some features of the compiler library are conditionally enabled or disabled at
# compile time depending on the CL version. As CA_CL_STANDARD is added as an
# option in source/cl/CMakelists.txt, it may not actually be defined yet,
# especially in a "no CL" build configuration. Therefore, we default it to 1.2.
set(CA_CL_STANDARD_INTERNAL 1.2)
if(CA_CL_STANDARD)
    set(CA_CL_STANDARD_INTERNAL ${CA_CL_STANDARD})
endif()
string(REPLACE "." "" CA_CL_STANDARD_INTERNAL ${CA_CL_STANDARD_INTERNAL})
string(APPEND CA_CL_STANDARD_INTERNAL "0")

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
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/check_for_doubles_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/check_for_ext_funcs_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/combine_fpext_fptrunc_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/fast_math_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/image_argument_substitution_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/mem_to_reg_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/pass_pipelines.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/printf_replacement_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/set_barrier_convergent_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/software_division_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/spir_fixup_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/program_metadata.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/base/target.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/base_module_pass_machinery.cpp  
  ${CMAKE_CURRENT_SOURCE_DIR}/source/context.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/module.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/bit_shift_fixup_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/builtin_simplification_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/check_for_doubles_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/check_for_ext_funcs_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/combine_fpext_fptrunc_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/fast_math_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/image_argument_substitution_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/mem_to_reg_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/pass_pipelines.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/printf_replacement_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/set_barrier_convergent_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/software_division_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/spir_fixup_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/program_metadata.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/target.cpp)

add_ca_library(compiler-base STATIC ${COMPILER_BASE_SOURCES})
target_include_directories(compiler-base PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/modules/compiler/include)
target_include_directories(compiler-base SYSTEM PUBLIC
  ${PROJECT_SOURCE_DIR}/modules/spirv-ll/external/SPIRV-Headers/include)

# Enable OpenCL 3.0 compiler features if 3.0 is specified in the CMake command
# line.
if(CA_CL_STANDARD VERSION_EQUAL "3.0")
  target_compile_definitions(compiler-base PUBLIC CA_COMPILER_ENABLE_CL_VERSION_3_0)
endif()

target_link_libraries(compiler-base PUBLIC
  builtins cargo mux spirv-ll compiler-utils vecz
  clangCodeGen LLVMCodeGen LLVMCoroutines LLVMCoverage LLVMLTO)

target_compile_definitions(compiler-base PRIVATE
  $<$<VERSION_EQUAL:${CA_CL_STANDARD},3.0
  >:ABACUS_ENABLE_OPENCL_3_0_BUILTINS>
  # CL_TARGET_OPENCL_VERSION is only used to change the behaviour of the
  # compiler implementation, so therefore is kept PRIVATE.
  CL_TARGET_OPENCL_VERSION=${CA_CL_STANDARD_INTERNAL}
  # Abacus builtins are only included as part of the .cpp files, so these
  # defines can also be kept PRIVATE.
  ABACUS_ENABLE_OPENCL_1_2_BUILTINS
  $<$<EQUAL:${CA_CL_STANDARD_INTERNAL},300>:ABACUS_ENABLE_OPENCL_3_0_BUILTINS>)

set(CA_COMPILER_LLVM_VERSION "${LLVM_PACKAGE_VERSION}")
if(NOT ${BUILD_TYPE_UPPER} MATCHES "RELEASE")
  string(APPEND CA_COMPILER_LLVM_VERSION " (${LLVM_BUILD_TYPE})")
endif()
target_compile_definitions(compiler-base PUBLIC
  CA_COMPILER_LLVM_VERSION=\"${CA_COMPILER_LLVM_VERSION}\"
  # This modifies the interface by advertising OpenCL C 3 support in the header,
  # therefore it's a PUBLIC definition.
  $<$<EQUAL:${CA_CL_STANDARD_INTERNAL},300>:CA_COMPILER_ENABLE_CL_VERSION_3_0>)

# Compiler passes in this module include abacus headers, so ensure they are
# generated before this module starts to build.
add_dependencies(compiler-base abacus_generate)
