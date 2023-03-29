# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# If the riscv target is disabled, then the compiler should be too.
if(NOT TARGET riscv)
  return()
endif()

if(NOT "${CA_RISCV_DEVICE}" MATCHES "RefSi G1.*")
  return()
endif()

if(CA_RUNTIME_COMPILER_ENABLED)
  set(REFSI_G1_SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/source/refsi_wg_loop_pass.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/source/refsi_mux_builtin_info.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/source/refsi_pass_machinery.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/source/info.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/source/module.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/include/refsi_g1_wi/refsi_wg_loop_pass.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/refsi_g1_wi/refsi_mux_builtin_info.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/refsi_g1_wi/refsi_pass_machinery.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/refsi_g1_wi/info.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/refsi_g1_wi/module.h
  )

  add_ca_library(compiler-refsi-g1-wi STATIC ${REFSI_G1_SOURCES})

  ca_option(CA_REFSI_WI_G1_DEMO_MODE BOOL "Enable ComputeAorta REFSI G1 demo mode" ON)
  if(CA_REFSI_WI_G1_DEMO_MODE)
    target_compile_definitions(compiler-refsi-g1-wi PRIVATE CA_REFSI_WI_G1_DEMO_MODE=$<BOOL:${CA_REFSI_WI_G1_DEMO_MODE}>)
  endif()

  target_include_directories(compiler-refsi-g1-wi PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  target_link_libraries(compiler-refsi-g1-wi PUBLIC
    compiler-riscv-utils
    compiler-utils
    compiler-linker-utils
    refsidrv
    compiler-base
    LLVMCoverage LLVMDebugInfoCodeView LLVMExecutionEngine
    LLVMMCJIT LLVMVectorize LLVMipo multi_llvm)

   add_mux_compiler_target(compiler-refsi-g1-wi
     COMPILER_INFO refsi_g1_wi::RefSiG1Info::get
     HEADER_FILE refsi_g1_wi/info.h)

  if(CA_ENABLE_TESTS)
    # Include the test subdirectory for any additional refsi tests
    add_subdirectory(test)
  endif()
endif()
