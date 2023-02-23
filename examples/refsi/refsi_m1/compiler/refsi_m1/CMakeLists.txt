# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# If the riscv target is disabled, then the compiler should be too.
if(NOT TARGET riscv)
  return()
endif()

if(NOT "${CA_RISCV_DEVICE}" MATCHES "RefSi M1.*")
  return()
endif()

if(CA_RUNTIME_COMPILER_ENABLED)
  set(REFSI_M1_SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/source/refsi_replace_mux_dma_pass.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/source/refsi_wrapper_pass.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/source/refsi_pass_machinery.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/source/info.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/source/module.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/include/refsi_m1/refsi_wrapper_pass.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/refsi_m1/refsi_pass_machinery.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/refsi_m1/info.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/refsi_m1/module.h
  )

  add_ca_library(compiler-refsi-m1 STATIC ${REFSI_M1_SOURCES})

  ca_option(CA_REFSI_M1_DEMO_MODE BOOL "Enable ComputeAorta REFSI M1 demo mode" ON)
  if(CA_REFSI_M1_DEMO_MODE)
    target_compile_definitions(compiler-refsi-m1 PRIVATE CA_REFSI_M1_DEMO_MODE=$<BOOL:${CA_REFSI_M1_DEMO_MODE}>)
  endif()

  target_include_directories(compiler-refsi-m1 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  target_link_libraries(compiler-refsi-m1 PUBLIC
    compiler-riscv-utils
    compiler-utils  
    compiler-linker-utils      
    refsidrv
    compiler-base
    LLVMCoverage LLVMDebugInfoCodeView LLVMExecutionEngine
    LLVMMCJIT LLVMVectorize LLVMipo multi_llvm)

   add_mux_compiler_target(compiler-refsi-m1
     COMPILER_INFO refsi_m1::RefSiM1Info::get
     HEADER_FILE refsi_m1/info.h)


  if(CA_ENABLE_TESTS)
    # Include the test subdirectory for any additional refsi tests
    add_subdirectory(test)
  endif()
endif()
