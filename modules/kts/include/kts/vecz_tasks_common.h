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
#ifndef KTS_VECZ_TASKS_COMMON_H_INCLUDED
#define KTS_VECZ_TASKS_COMMON_H_INCLUDED

// System headers
#include <algorithm>
#include <cmath>

// In-house headers
#include "kts/arguments_shared.h"
#include "kts/reference_functions.h"

/// @brief Populate and verify the contents of buffers used in atomic tests.
template <typename T>
class AtomicStreamer : public kts::BufferStreamer {
 public:
  AtomicStreamer(T init_value, T count)
      : init_value_(init_value), count_(count) {}

  virtual void PopulateBuffer(kts::ArgumentBase &arg,
                              const kts::BufferDesc &desc) override {
    kts::MemoryAccessor<T> accessor;
    arg.SetBufferStorageSize(desc.size_ * sizeof(T));
    if (arg.GetIndex() == 0) {
      // Initialize the global counter.
      accessor.StoreToBuffer(init_value_, arg.GetBufferStoragePtr(), 0);
    } else if (arg.GetIndex() == 1) {
      // Initialize the intermediate result buffer.
      for (size_t i = 0; i < desc.size_; i++) {
        accessor.StoreToBuffer(0, arg.GetBufferStoragePtr(), (unsigned int)i);
      }
    }
  }

  virtual bool ValidateBuffer(kts::ArgumentBase &arg,
                              const kts::BufferDesc &desc,
                              std::vector<std::string> *errors) override {
    kts::MemoryAccessor<T> accessor;
    if (arg.GetIndex() == 0) {
      // Validate the global counter, which should be equal to 'init + count'
      T result = accessor.LoadFromBuffer(arg.GetBufferStoragePtr(), 0);
      T expected = init_value_ + count_;
      if (expected != result) {
        std::stringstream ss;
        ss << "Result mismatch (expected: " << expected
           << ", actual: " << result << ")";
        errors->push_back(ss.str());
        return false;
      }
      return true;
    } else if (arg.GetIndex() == 1) {
      // Validate the intermediate result buffer, which should hold one copy of
      // all values from 'min_expected' to 'max_expected'.
      T min_expected = init_value_;
      T max_expected = init_value_ + count_ - 1;
      // Count the number of times each value appears in the buffer.
      std::vector<T> histogram;
      histogram.resize(count_, 0);
      for (size_t i = 0; i < desc.size_; i++) {
        T result =
            accessor.LoadFromBuffer(arg.GetBufferStoragePtr(), (unsigned int)i);
        if ((result < min_expected) || (result > max_expected)) {
          std::stringstream ss;
          ss << "Unexpected value " << result << " (valid range: ["
             << min_expected << ";" << max_expected << "])";
          errors->push_back(ss.str());
          return false;
        }
        histogram[result - min_expected]++;
      }
      // Verify that each value appears once.
      for (size_t i = 0; i < desc.size_; i++) {
        if (histogram[i] == 0) {
          std::stringstream ss;
          ss << "Did not find value " << (min_expected + i);
          errors->push_back(ss.str());
          return false;
        } else if (histogram[i] > 1) {
          std::stringstream ss;
          ss << "Found " << histogram[i] << " copies of value "
             << (min_expected + i);
          errors->push_back(ss.str());
          return false;
        }
      }
      return true;
    }
    return false;
  }

  virtual size_t GetElementSize() override { return sizeof(T); }

 private:
  T init_value_;
  T count_;
};

#endif  // KTS_VECZ_TASKS_COMMON_H_INCLUDED
