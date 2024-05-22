/**
 * SYCL FOR CUDA : Vector Addition Example
 *
 * Copyright 2020 Codeplay Software Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 *
 * @File: vector_addition.cpp
 */

#include <sycl/sycl.hpp>

#include <algorithm>
#include <iostream>
#include <vector>

void vecAdd(const float *a, const float *b, float *c, size_t id) {
  c[id] = a[id] + b[id];
}

class vec_add;
int main(int argc, char *argv[]) {
  constexpr const size_t N = 100000;
  const sycl::range<1> VecSize{N};

  sycl::buffer<float> bufA{VecSize};
  sycl::buffer<float> bufB{VecSize};
  sycl::buffer<float> bufC{VecSize};

  // Initialize input data
  {
    sycl::host_accessor h_a{bufA, sycl::write_only};
    sycl::host_accessor h_b{bufB, sycl::write_only};

    for (int i = 0; i < N; i++) {
      h_a[i] = sycl::sin((float)i) * sycl::sin((float)i);
      h_b[i] = sycl::cos((float)i) * sycl::cos((float)i);
    }
  }

  sycl::queue myQueue;

  // Command Group creation
  auto cg = [&](sycl::handler &h) {
    sycl::accessor a{bufA, h, sycl::read_only};
    sycl::accessor b{bufB, h, sycl::read_only};
    sycl::accessor c{bufC, h, sycl::write_only};

    h.parallel_for<vec_add>(
        VecSize, [=](sycl::id<1> i) { vecAdd(&a[0], &b[0], &c[0], i[0]); });
  };

  myQueue.submit(cg);

  {
    sycl::host_accessor h_c{bufC, sycl::read_only};

    float sum = 0.0f;
    for (int i = 0; i < N; i++) {
      sum += h_c[i];
    }
    std::cout << "final result: " << sum << std::endl;
  }

  return 0;
}
