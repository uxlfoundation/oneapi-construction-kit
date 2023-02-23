# Copyright (C) Codeplay Software Limited. All Rights Reserved.

find_package(Lit)
if(NOT Lit_FOUND)
  message(WARNING "{{cookiecutter.target_name}}-lit tests will not be runnable: lit not found "
    "Please install https://pypi.python.org/pypi/lit")
  return()
 endif()

add_ca_configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  MAIN_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg"
  DEFINED
  muxc_EXECUTABLE=$<TARGET_FILE:muxc>  
  )

add_ca_lit_testsuite({{cookiecutter.target_name}}
  ${CMAKE_CURRENT_BINARY_DIR} 
  DEPENDS ca-common-lit muxc ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg)
