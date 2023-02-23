# Copyright (C) Codeplay Software Limited. All Rights Reserved.

find_package(Lit)
if(NOT Lit_FOUND)
  message(WARNING "refsi-m1-lit tests will not be runnable: lit not found "
    "Please install https://pypi.python.org/pypi/lit")
  return()
endif()


# Find required LLVM tools.
find_package(LLVMTool COMPONENTS
  llc llvm-as llvm-dis llvm-profdata llvm-cov llvm-objdump FileCheck not)
if(NOT LLVMTool_FOUND)
  message(WARNING "refsi-m1-lit disabled, not found: "
    "llc llvm-as llvm-dis llvm-profdata llvm-cov llvm-objdump FileCheck not")
  return()
endif()

add_ca_configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  MAIN_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg"
  DEFINED
  clc_EXECUTABLE=$<TARGET_FILE:clc>
  muxc_EXECUTABLE=$<TARGET_FILE:muxc>  
  llvm-objdump_EXECUTABLE=$<TARGET_FILE:LLVM::llvm-objdump>
  RISCV_DEVICE_NAME=${CA_RISCV_DEVICE}
  )

add_ca_lit_testsuite(refsi-m1-compiler
  ${CMAKE_CURRENT_BINARY_DIR}
  TARGET refsi-m1 
  DEPENDS ca-common-lit muxc ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DESTINATION "share/refsi-m1" COMPONENT OCLTest)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DESTINATION "share/refsi-m1" COMPONENT OCLTest)
