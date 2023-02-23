# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Check if riscv is enabled in the llvm install.
# If so we can build this library
find_library(LLVMRISCVCODEGEN LLVMRISCVCodeGen PATHS
            "${LLVM_INSTALL_PREFIX}/lib" NO_DEFAULT_PATH)

if (LLVMRISCVCODEGEN)
  if(CA_RUNTIME_COMPILER_ENABLED)
    add_subdirectory(runtimelib)

    set(RISCV_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/source/passes/IRToBuiltinsPass.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/source/info.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/source/module.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/source/target.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/source/riscv_pass_machinery.cpp  
        ${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/bakery.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/info.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/module.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/target.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/riscv/riscv_pass_machinery.h
    )

    add_ca_library(compiler-riscv-utils STATIC ${RISCV_SOURCES})

    ca_option(CA_RISCV_DEMO_MODE BOOL "Enable ComputeAorta RISC-V demo mode" ON)
    if(CA_RISCV_DEMO_MODE)
      target_compile_definitions(compiler-riscv-utils PRIVATE CA_RISCV_DEMO_MODE=$<BOOL:${CA_RISCV_DEMO_MODE}>)
    endif()


    target_include_directories(compiler-riscv-utils PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(compiler-riscv-utils PUBLIC
      compiler-base
      hal_common      
      compiler-linker-utils
      LLVMCoverage LLVMDebugInfoCodeView LLVMExecutionEngine
      LLVMMCJIT LLVMVectorize LLVMipo multi_llvm)

    target_link_libraries(compiler-riscv-utils PUBLIC riscvrt)
  endif()
endif()
