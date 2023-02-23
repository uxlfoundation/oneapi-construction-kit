# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_cl_executable(clVectorAddition
  ${CMAKE_CURRENT_SOURCE_DIR}/source/main.c)
add_ca_cl_check(clVectorAddition
  COMMAND clVectorAddition --platform ComputeAorta
  DEPENDS clVectorAddition)
install(TARGETS clVectorAddition
  RUNTIME DESTINATION bin COMPONENT OCLExamples)
