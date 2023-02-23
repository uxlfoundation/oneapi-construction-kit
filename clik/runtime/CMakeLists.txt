# Copyright (C) Codeplay Software Limited. All Rights Reserved.

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -fvisibility=hidden)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fvisibility=hidden)

install(FILES ${PROJECT_SOURCE_DIR}/include/clik_common.h DESTINATION include)

add_subdirectory(sync)
add_subdirectory(async)
