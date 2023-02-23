# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# If the riscv target is disabled, then the compiler should be too.
if(NOT TARGET riscv)
  return()
endif()

if(CA_RUNTIME_COMPILER_ENABLED)
  set(RISCV_SOURCES
      ${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/get_info.h
  )

  add_ca_library(compiler-riscv STATIC ${RISCV_SOURCES})

  ca_option(CA_RISCV_DEMO_MODE BOOL "Enable ComputeAorta RISC-V demo mode" ON)
  if(CA_RISCV_DEMO_MODE)
    target_compile_definitions(compiler-riscv PRIVATE CA_RISCV_DEMO_MODE=$<BOOL:${CA_RISCV_DEMO_MODE}>)
  endif()

  target_include_directories(compiler-riscv PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  target_link_libraries(compiler-riscv PUBLIC
    compiler-riscv-utils
    compiler-linker-utils
    LLVMCoverage LLVMDebugInfoCodeView LLVMExecutionEngine
    LLVMMCJIT LLVMVectorize LLVMipo multi_llvm)

  add_mux_compiler_target(compiler-riscv
     COMPILER_INFO riscv::getInfos
     HEADER_FILE riscv/get_info.h)

  if(CA_ENABLE_TESTS)
    # Include the test subdirectory for any additional riscv tests
    add_subdirectory(test)
  endif()
endif()
