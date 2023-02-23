// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
