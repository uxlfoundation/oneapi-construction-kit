// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/checks.h"

TEST(urInitTest, Success) {
  ur_device_init_flags_t device_flags = 0;
  ASSERT_SUCCESS(urInit(device_flags));
}
TEST(urInitTest, ErrorInvalidEnumuerationDeviceFlags) {
  const ur_device_init_flags_t device_flags = UR_DEVICE_INIT_FLAG_FORCE_UINT32;
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_ENUMERATION, urInit(device_flags));
}
