# Copyright (C) Codeplay Software Limited. All Rights Reserved.

find_package(Lit)
if(NOT Lit_FOUND)
  message(WARNING "vecz lit tests disabled: "
    "'lit' not found. Please install https://pypi.python.org/pypi/lit")
  return()
endif()

find_package(LLVMTool COMPONENTS
  llc llvm-profdata llvm-cov llvm-as llvm-dis FileCheck not)
if(NOT LLVMTool_FOUND)
  message(WARNING "vecz lit tests disabled, not found: "
    "llc llvm-profdata llvm-cov llvm-as llvm-dis FileCheck not")
  return()
endif()

add_ca_configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  MAIN_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py"
  DEFINED
  veczc_EXECUTABLE=$<TARGET_FILE:veczc>)

add_ca_lit_testsuite(vecz
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ca-common-lit veczc ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DESTINATION "share/vecz" COMPONENT OCLTest)
