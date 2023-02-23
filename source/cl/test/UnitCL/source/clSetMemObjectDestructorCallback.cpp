// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
  ASSERT_SUCCESS(
      clSetMemObjectDestructorCallback(buffer, Callback::callback, &hit));
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
