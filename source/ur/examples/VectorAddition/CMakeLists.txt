# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_executable(urVectorAddition
  ${CMAKE_CURRENT_SOURCE_DIR}/source/main.c)
target_link_libraries(urVectorAddition PRIVATE UR)
# TODO: Remove this temporary workaround once issue #49 is fixed.
target_compile_options(urVectorAddition PRIVATE
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wno-typedef-redefinition>)
install(TARGETS urVectorAddition
  RUNTIME DESTINATION bin COMPONENT URExamples)
