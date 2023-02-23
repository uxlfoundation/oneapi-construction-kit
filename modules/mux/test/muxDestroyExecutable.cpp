// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common.h"

struct muxDestroyExecutableTest : public DeviceCompilerTest {};

INSTANTIATE_DEVICE_TEST_SUITE_P(muxDestroyExecutableTest);

TEST_P(muxDestroyExecutableTest, Default) {
  // Kernel source.
  const char *nop_source = R"(void kernel nop() {})";

  mux_executable_t executable = nullptr;
  ASSERT_SUCCESS(createMuxExecutable(nop_source, &executable));
  muxDestroyExecutable(device, executable, allocator);
}
