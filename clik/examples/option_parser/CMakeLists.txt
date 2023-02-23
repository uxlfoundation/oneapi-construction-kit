# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_library(option_parser STATIC
    option_parser.cc
)

target_include_directories(option_parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

