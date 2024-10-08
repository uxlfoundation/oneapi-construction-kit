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

#ifndef ICD_H_INCLUDED
#define ICD_H_INCLUDED

#include <cstdint>

template <typename T>
struct icd_t {
  uintptr_t magic = 0x01CDC0DE;

  friend T;

 private:
  icd_t() = default;
  icd_t(const icd_t &) = default;
  icd_t(icd_t &&) = default;
};

#endif
