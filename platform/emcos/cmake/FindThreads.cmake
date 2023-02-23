# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# The eMCOS toolchain always statically links with pthreads so don't need to do
# anything here, just add the library below so find_package(Threads) succeeds.
if(NOT TARGET Threads::Threads)
  add_library(Threads::Threads INTERFACE IMPORTED)
endif()
