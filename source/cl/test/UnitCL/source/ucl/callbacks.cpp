// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "ucl/callbacks.h"

#include <iostream>

#include "cargo/string_algorithm.h"
#include "ucl/environment.h"

void CL_CALLBACK ucl::contextCallback(const char *errinfo, const void *, size_t,
                                      void *) {
  std::cerr << errinfo << "\n";
}

void CL_CALLBACK ucl::buildLogCallback(cl_program program, void *) {
  auto device = ucl::Environment::instance->GetDevice();
  size_t logSize;
  if (auto error = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                                         0, nullptr, &logSize)) {
    std::cerr << "error: program build log returned: " << error << "\n";
    return;
  }
  if (logSize) {
    std::vector<char> log(logSize);
    if (auto error =
            clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                                  logSize, log.data(), nullptr)) {
      std::cerr << "error: program build log returned: " << error << "\n";
      return;
    }
    auto logTrimmed = cargo::trim(log.data());
    if (!logTrimmed.empty()) {
      std::cerr << logTrimmed << "\n";
      return;
    }
  }
};
