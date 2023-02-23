# Copyright (C) Codeplay Software Limited. All Rights Reserved.

function(add_baked_data TARGET NAME HEADER SRC)
  set(DATA_SRC ${SRC})
  if (NOT IS_ABSOLUTE ${DATA_SRC})
    set(DATA_SRC ${CMAKE_CURRENT_SOURCE_DIR}/${DATA_SRC})
  endif()
  add_bin2h_target(${NAME} ${DATA_SRC} ${CMAKE_CURRENT_BINARY_DIR}/${HEADER})

  add_dependencies(${TARGET} ${NAME})

  target_include_directories(${TARGET} PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR}
  )
endfunction()
