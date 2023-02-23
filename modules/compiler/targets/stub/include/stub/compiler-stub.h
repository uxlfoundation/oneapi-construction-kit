// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <compiler/info.h>
#include <compiler/target.h>

namespace stub {
struct StubInfo : compiler::Info {
  StubInfo() { device_info = nullptr; }

  std::unique_ptr<compiler::Target> createTarget(
      compiler::Context *, compiler::NotifyCallbackFn) const override {
    return nullptr;
  }

  static void get(compiler::AddCompilerFn add_compiler) {
    static StubInfo info;
    add_compiler(&info);
  }
};
}  // namespace stub
