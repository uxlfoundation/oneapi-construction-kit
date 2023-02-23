// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/environment.h"

int main(int argc, char **argv) {
  uur::Environment *environment = new uur::Environment{argc, argv};
  testing::InitGoogleTest(&argc, argv);
  testing::AddGlobalTestEnvironment(environment);
  return RUN_ALL_TESTS();
}
