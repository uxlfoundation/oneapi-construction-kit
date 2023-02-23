# Copyright (C) Codeplay Software Limited. All Rights Reserved.

find_package(Lit)
if(NOT Lit_FOUND)
  message(WARNING "muxc passes-lit tests will not be runnable: lit not found "
    "Please install https://pypi.python.org/pypi/lit")
  return()
endif()

# Find required LLVM tools.
find_package(LLVMTool COMPONENTS FileCheck not)
if(NOT LLVMTool_FOUND)
  message(WARNING "muxc passes-lit disabled, not found: "
    "llc llvm-as llvm-dis llvm-profdata llvm-cov llvm-objdump FileCheck not")
  return()
endif()

add_ca_configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  MAIN_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg"
  DEFINED
  muxc_EXECUTABLE=$<TARGET_FILE:muxc>)

add_ca_lit_testsuite(passes
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ca-common-lit muxc ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DESTINATION "share/passes" COMPONENT OCLTest)
