# Copyright (C) Codeplay Software Limited. All Rights Reserved.

set(SCRIPT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.py)
add_custom_target(check
    COMMAND ${Python3_EXECUTABLE} ${SCRIPT_PATH} -L lib -b bin --strict --timeout 00:01:00
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    USES_TERMINAL)

add_dependencies(check ClikExamples)
if(NOT CLIK_HAL_NAME STREQUAL "")
  add_dependencies(check hal_${CLIK_HAL_NAME})
endif()
