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

#include <cl/binary/program_info.h>

namespace cl {
namespace binary {

ProgramInfo::ProgramInfo() : kernel_descriptions() {}

bool ProgramInfo::addNewKernel() {
  if (cargo::success != kernel_descriptions.emplace_back()) {
    return false;
  }
  return true;
}

bool ProgramInfo::resizeFromNumKernels(int32_t numKernels) {
  if (cargo::success != kernel_descriptions.resize(numKernels)) {
    return false;
  }
  return true;
}

KernelInfo *ProgramInfo::getKernel(size_t kernel_index) {
  if (kernel_index >= kernel_descriptions.size()) {
    return nullptr;
  }
  return &kernel_descriptions[kernel_index];
}

const KernelInfo *ProgramInfo::getKernel(size_t kernel_index) const {
  if (kernel_index >= kernel_descriptions.size()) {
    return nullptr;
  }
  return &kernel_descriptions[kernel_index];
}

KernelInfo *ProgramInfo::getKernelByName(cargo::string_view kernel_name) {
  for (auto &desc : kernel_descriptions) {
    if (kernel_name == desc.name) {
      return &desc;
    }
  }
  return nullptr;
}

const KernelInfo *ProgramInfo::getKernelByName(
    cargo::string_view kernel_name) const {
  for (auto &desc : kernel_descriptions) {
    if (kernel_name == desc.name) {
      return &desc;
    }
  }
  return nullptr;
}

KernelInfo *ProgramInfo::begin() { return kernel_descriptions.begin(); }

const KernelInfo *ProgramInfo::begin() const {
  return kernel_descriptions.begin();
}

KernelInfo *ProgramInfo::end() { return kernel_descriptions.end(); }

const KernelInfo *ProgramInfo::end() const { return kernel_descriptions.end(); }

cargo::optional<ProgramInfo> kernelDeclsToProgramInfo(
    const cargo::small_vector<std::string, 8> &decls, bool store_arg_metadata) {
  ProgramInfo program_info;
  size_t num_kernels = decls.size();
  if (!decls.empty()) {
    if (!program_info.resizeFromNumKernels(num_kernels)) {
      return cargo::nullopt;
    }
    for (uint32_t i = 0; i < num_kernels; ++i) {
      if (!kernelDeclStrToKernelInfo(*program_info.getKernel(i),
                                     cargo::string_view(decls[i]),
                                     store_arg_metadata)) {
        return cargo::nullopt;
      }
    }
  }
  return {std::move(program_info)};
}

}  // namespace binary
}  // namespace cl
