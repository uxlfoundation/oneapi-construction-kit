# Copyright (C) Codeplay Software Limited. All Rights Reserved.

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/riscv-isa-sim/CMakeLists.txt)
  add_subdirectory(riscv-isa-sim)
else()
  include(FetchContent)

  # Use upstream github for riscv-isa-sim, but with some local changes to support refsi
  FetchContent_Declare(
    riscv-isa-sim
    GIT_REPOSITORY https://github.com/riscv-software-src/riscv-isa-sim.git
    GIT_TAG        057139ab90b8e03b71825e8f2be7cab58fef0f90
    # Do a check to see if it has been applied first as this can sometimes be an
    # issue when cmake changes are made
    PATCH_COMMAND  git apply --check --reverse ${CMAKE_CURRENT_SOURCE_DIR}/codeplay_riscv_isa_sim.patch || git apply ${CMAKE_CURRENT_SOURCE_DIR}/codeplay_riscv_isa_sim.patch
  )

  FetchContent_MakeAvailable(riscv-isa-sim)
endif()
