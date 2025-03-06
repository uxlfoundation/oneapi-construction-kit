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

#include "Common.h"

class clSetMemObjectDestructorCallbackTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    cl_int errorcode;
    buffer = clCreateBuffer(context, 0, 128, nullptr, &errorcode);
    ASSERT_TRUE(buffer);
    ASSERT_SUCCESS(errorcode);
  }

  cl_mem buffer = nullptr;
};

TEST_F(clSetMemObjectDestructorCallbackTest, Default) {
  struct Callback {
    static void CL_CALLBACK callback(cl_mem memobj, void *user_data) {
      *static_cast<cl_mem *>(user_data) = memobj;
    }
  };

  cl_mem hit = nullptr;
  ASSERT_SUCCESS(clSetMemObjectDestructorCallback(buffer, Callback::callback,
                                                  static_cast<void *>(&hit)));
  ASSERT_SUCCESS(clReleaseMemObject(buffer));
  ASSERT_EQ(hit, buffer);
}

TEST_F(clSetMemObjectDestructorCallbackTest, CallbackOrder) {
  struct Callback {
    unsigned id;
    unsigned *shared_id;

    static void CL_CALLBACK callback(cl_mem memobj, void *user_data) {
      (void)memobj;  // unused parameter
      auto *me = static_cast<Callback *>(user_data);

      me->id = *(me->shared_id);
      *(me->shared_id) += 1;
    }

    Callback(unsigned *shared_id) : id(0), shared_id(shared_id) {}
  };

  unsigned id = 0;
  Callback first(&id);
  Callback second(&id);
  ASSERT_SUCCESS(
      clSetMemObjectDestructorCallback(buffer, Callback::callback, &first));
  ASSERT_SUCCESS(
      clSetMemObjectDestructorCallback(buffer, Callback::callback, &second));
  ASSERT_SUCCESS(clReleaseMemObject(buffer));
  ASSERT_EQ(0u, second.id);
  ASSERT_EQ(1u, first.id);
}
