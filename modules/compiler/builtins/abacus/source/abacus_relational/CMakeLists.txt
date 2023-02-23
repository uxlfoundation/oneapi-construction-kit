# Copyright (C) Codeplay Software Limited. All Rights Reserved.

set(abacus_relational_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/all.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/any.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bitselect.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isequal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isfinite.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isgreater.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isgreaterequal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isinf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isless.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/islessequal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/islessgreater.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isnan.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isnormal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isnotequal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isordered.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/isunordered.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/select.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/signbit.cpp
  PARENT_SCOPE)
