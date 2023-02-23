# Copyright (C) Codeplay Software Limited. All Rights Reserved.

set(abacus_integer_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/abs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/abs_diff.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/add_sat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/clamp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/clz.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hadd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mad24.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mad_hi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mad_sat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/max.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/min.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mul24.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mul_hi.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/popcount.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/rhadd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/rotate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sub_sat.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/upsample.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ctz.cpp
  PARENT_SCOPE)
