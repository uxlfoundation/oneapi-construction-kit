// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#ifndef UNITCL_USM_H_INCLUDED
#define UNITCL_USM_H_INCLUDED

#include <CL/cl_ext.h>

#include "Common.h"

// Base class for checking if the USM extension is enabled. If so Setup queries
// for function pointers to new extension entry points that test fixtures
// can use.
struct cl_intel_unified_shared_memory_Test : public virtual ucl::ContextTest {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!isDeviceExtensionSupported("cl_intel_unified_shared_memory")) {
      GTEST_SKIP();
    }

#define CL_GET_EXTENSION_ADDRESS(FUNC)                            \
  FUNC = reinterpret_cast<FUNC##_fn>(                             \
      clGetExtensionFunctionAddressForPlatform(platform, #FUNC)); \
  ASSERT_NE(nullptr, FUNC);

    CL_GET_EXTENSION_ADDRESS(clHostMemAllocINTEL);
    CL_GET_EXTENSION_ADDRESS(clDeviceMemAllocINTEL);
    CL_GET_EXTENSION_ADDRESS(clSharedMemAllocINTEL);
    CL_GET_EXTENSION_ADDRESS(clMemFreeINTEL);
    CL_GET_EXTENSION_ADDRESS(clMemBlockingFreeINTEL);
    CL_GET_EXTENSION_ADDRESS(clGetMemAllocInfoINTEL);
    CL_GET_EXTENSION_ADDRESS(clSetKernelArgMemPointerINTEL);
    CL_GET_EXTENSION_ADDRESS(clEnqueueMemFillINTEL);
    CL_GET_EXTENSION_ADDRESS(clEnqueueMemcpyINTEL);
    CL_GET_EXTENSION_ADDRESS(clEnqueueMigrateMemINTEL);
    CL_GET_EXTENSION_ADDRESS(clEnqueueMemAdviseINTEL);
    CL_GET_EXTENSION_ADDRESS(clEnqueueMemsetINTEL);

#undef GET_EXTENSION_ADDRESS
  }

  static void* getPointerOffset(void* ptr, size_t offset) {
    cl_char* byte_ptr = reinterpret_cast<cl_char*>(ptr) + offset;
    return reinterpret_cast<void*>(byte_ptr);
  }

  clHostMemAllocINTEL_fn clHostMemAllocINTEL = nullptr;
  clDeviceMemAllocINTEL_fn clDeviceMemAllocINTEL = nullptr;
  clSharedMemAllocINTEL_fn clSharedMemAllocINTEL = nullptr;
  clMemFreeINTEL_fn clMemFreeINTEL = nullptr;
  clMemBlockingFreeINTEL_fn clMemBlockingFreeINTEL = nullptr;
  clGetMemAllocInfoINTEL_fn clGetMemAllocInfoINTEL = nullptr;
  clSetKernelArgMemPointerINTEL_fn clSetKernelArgMemPointerINTEL = nullptr;
  clEnqueueMemFillINTEL_fn clEnqueueMemFillINTEL = nullptr;
  clEnqueueMemcpyINTEL_fn clEnqueueMemcpyINTEL = nullptr;
  clEnqueueMigrateMemINTEL_fn clEnqueueMigrateMemINTEL = nullptr;
  clEnqueueMemAdviseINTEL_fn clEnqueueMemAdviseINTEL = nullptr;
  clEnqueueMemsetINTEL_fn clEnqueueMemsetINTEL = nullptr;
};

template <typename T>
class USMWithParam : public cl_intel_unified_shared_memory_Test,
                     public ::testing::WithParamInterface<T> {};

#endif  // UNITCL_USM_H_INCLUDED
