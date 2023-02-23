# Copyright (C) Codeplay Software Limited. All Rights Reserved.

add_ca_executable(host-system-info system-info.cpp)
target_link_libraries(host-system-info PRIVATE host)
add_ca_check(host-system-info COMMAND host-system-info)
