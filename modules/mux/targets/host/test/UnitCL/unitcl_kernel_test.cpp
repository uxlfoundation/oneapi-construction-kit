// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cargo/string_view.h>
// In-house headers
#include "Common.h"
#include "Device.h"
#include "kts/vecz_tasks_common.h"

// This is needed for 'TEST_F'.
using namespace kts::ucl;

TEST_P(Execution, Host_01_Example) {
  // Since these are host specific test we want to skip it if we aren't
  // running on host.
  if (!UCL::isDevice_host(device)) {
    GTEST_SKIP();
  }

  AddInputBuffer(kts::N, kts::Ref_A);
  AddOutputBuffer(kts::N, kts::Ref_A);
  RunGeneric1D(kts::N);
}
