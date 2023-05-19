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

#include <cstdint>
#include <iostream>

extern uint32_t os_cpu_frequency();
extern uint32_t os_num_cpus();
extern uint64_t os_cache_size();
extern uint64_t os_cacheline_size();
extern uint64_t os_memory_total_size();
extern uint64_t os_memory_bounded_size();

int main() {
  std::cout << "CPU Frequency: " << os_cpu_frequency() << " MHz\n";
  std::cout << "CPU Processor Count: " << os_num_cpus() << "\n";
  std::cout << "CPU Cache Size: " << os_cache_size() << " bytes\n";
  std::cout << "CPU Cache-Line Size: " << os_cacheline_size() << " bytes\n";
  std::cout << "System Memory: " << os_memory_total_size() << " bytes\n";
  std::cout << "System Memory (Bounded): " << os_memory_bounded_size()
            << " bytes\n";
  return 0;
}
