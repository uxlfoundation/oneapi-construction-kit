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

#include <CL/sycl.hpp>

#include <algorithm>
#include <iostream>
#include <vector>

using namespace cl;

void loadTiles(const float *a, const float *b, float *tile1, float *tile2,
               size_t id, size_t tile_i) {
  tile1[tile_i] = a[id];
  tile2[tile_i] = b[id];
}

void vecAdd(float *tile1, float *tile2, size_t id, size_t tile_i) {
  tile1[tile_i] += tile2[tile_i];
}

void storeTile(float *c, float *tile1, size_t id, size_t tile_i) {
  c[id] = tile1[tile_i];
}

class tiled_vec_add;
int main(int argc, char *argv[]) {
  constexpr const size_t N = 128000;  // this is the total vector size
  constexpr const size_t T = 32;      // this is the tile size
  const sycl::range<1> VecSize{N};
  const sycl::range<1> tile_size(T);

  sycl::buffer<float> bufA{VecSize};
  sycl::buffer<float> bufB{VecSize};
  sycl::buffer<float> bufC{VecSize};

  // Initialize input data
  {
    const auto dwrite_t = sycl::access::mode::discard_write;

    auto h_a = bufA.get_access<dwrite_t>();
    auto h_b = bufB.get_access<dwrite_t>();
    for (int i = 0; i < N; i++) {
      h_a[i] = sin(i) * sin(i);
      h_b[i] = cos(i) * cos(i);
    }
  }

  sycl::queue myQueue;

  // Command Group creation
  auto cg = [&](sycl::handler &h) {
    const auto read_t = sycl::access::mode::read;
    const auto write_t = sycl::access::mode::write;

    auto a = bufA.get_access<read_t>(h);
    auto b = bufB.get_access<read_t>(h);
    auto c = bufC.get_access<write_t>(h);
    sycl::accessor<float, 1, sycl::access::mode::read_write,
                   sycl::access::target::local>
        tile1(tile_size, h);
    sycl::accessor<float, 1, sycl::access::mode::read_write,
                   sycl::access::target::local>
        tile2(tile_size, h);

    h.parallel_for<tiled_vec_add>(
        sycl::nd_range<1>(VecSize, tile_size), [=](sycl::nd_item<1> i) {
          loadTiles(&a[0], &b[0], &tile1[0], &tile2[0], i.get_global_id(0),
                    i.get_local_id(0));
          i.barrier();
          vecAdd(&tile1[0], &tile2[0], i.get_global_id(0), i.get_local_id(0));
          i.barrier();
          storeTile(&c[0], &tile1[0], i.get_global_id(0), i.get_local_id(0));
        });
  };

  myQueue.submit(cg);

  {
    const auto read_t = sycl::access::mode::read;
    auto h_c = bufC.get_access<read_t>();
    float sum = 0.0f;
    for (int i = 0; i < N; i++) {
      sum += h_c[i];
    }
    std::cout << "final result: " << sum << std::endl;
  }

  return 0;
}
