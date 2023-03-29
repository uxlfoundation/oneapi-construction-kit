# Copyright (C) Codeplay Software Limited. All Rights Reserved.

find_package(Lit)
if(NOT Lit_FOUND)
  message(WARNING "refsi-g1-wi-lit tests will not be runnable: lit not found "
    "Please install https://pypi.python.org/pypi/lit")
  return()
endif()


# Find required LLVM tools.
find_package(LLVMTool COMPONENTS
  llc llvm-as llvm-dis llvm-profdata llvm-cov llvm-objdump FileCheck not)
if(NOT LLVMTool_FOUND)
  message(WARNING "refsi-g1-wi-lit disabled, not found: "
    "llc llvm-as llvm-dis llvm-profdata llvm-cov llvm-objdump FileCheck not")
  return()
endif()

add_ca_configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  MAIN_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg"
  DEFINED
  muxc_EXECUTABLE=$<TARGET_FILE:muxc>  
  RISCV_DEVICE_NAME=${CA_RISCV_DEVICE}
  )

add_ca_lit_testsuite(refsi-g1-wi-compiler
  ${CMAKE_CURRENT_BINARY_DIR}
  TARGET refsi-g1-wi
  DEPENDS ca-common-lit muxc ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DESTINATION "share/refsi-g1-wi" COMPONENT OCLTest)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DESTINATION "share/refsi-g1-wi" COMPONENT OCLTest)
