// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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
}
