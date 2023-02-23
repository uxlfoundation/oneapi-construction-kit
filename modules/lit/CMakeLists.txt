# Copyright (C) Codeplay Software Limited. All Rights Reserved.

get_property(CA_LIT_CONFIG_FILES GLOBAL PROPERTY CA_LIT_CONFIG_FILES)
list(LENGTH CA_LIT_CONFIG_FILES file_len)
math(EXPR file_last "${file_len} - 1")

set(CA_LIT_CONFIG_MAP "")

if (${file_last} GREATER -1)
  foreach(i RANGE 0 ${file_last} 2)
    list(GET CA_LIT_CONFIG_FILES ${i} main_config)
    math(EXPR i1 "${i} + 1")
    list(GET CA_LIT_CONFIG_FILES ${i1} site_out)
    set(map "map_config(path(r'${main_config}'), path(r'${site_out}'))")
    set(CA_LIT_CONFIG_MAP "${CA_LIT_CONFIG_MAP}\n${map}")
  endforeach()
endif()

# Note: not using add_ca_configure_file as it doesn't parse CA_LIT_CONFIG_MAP
# correctly
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/ca-lit.in
  ${CMAKE_CURRENT_BINARY_DIR}/ca-lit)

# Provide common LIT utilities for test suites
# FIXME: Only works for compiler targets since we require paths to LLVM tools.
# Could we make this more flexible with optional LLVM tools?
if(CA_RUNTIME_COMPILER_ENABLED)
  add_ca_configure_lit_site_cfg(
    ${CMAKE_CURRENT_SOURCE_DIR}/lit.common.cfg.in
    ${CMAKE_CURRENT_BINARY_DIR}/lit.common.cfg
    MAIN_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/lit.common.cfg")

  add_custom_target(ca-common-lit ALL DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/lit.common.cfg")
endif()
