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

#include <vector>

#include "Common.h"

class clGetSupportedImageFormatsTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
  }
};

static void printImageFormats(cl_mem_object_type type,
                              UCL::vector<cl_image_format> &formats) {
#define CASE(TYPE)           \
  case TYPE:                 \
    (void)printf(#TYPE " "); \
    break;
  switch (type) {
    CASE(CL_MEM_OBJECT_IMAGE1D)
    CASE(CL_MEM_OBJECT_IMAGE1D_BUFFER)
    CASE(CL_MEM_OBJECT_IMAGE2D)
    CASE(CL_MEM_OBJECT_IMAGE3D)
    CASE(CL_MEM_OBJECT_IMAGE1D_ARRAY)
    CASE(CL_MEM_OBJECT_IMAGE2D_ARRAY)
    default:
      (void)fprintf(stderr, "unknown image type!\n");
      abort();
  }
#undef CASE
  if (!formats.size()) {
    (void)printf("has no image formats supported.\n");
    return;
  }
  for (auto &format : formats) {
#define CASE(DATA_TYPE)                 \
  case DATA_TYPE:                       \
    (void)printf("  %20s", #DATA_TYPE); \
    break;
    switch (format.image_channel_data_type) {
      CASE(CL_SNORM_INT8)
      CASE(CL_SNORM_INT16)
      CASE(CL_UNORM_INT8)
      CASE(CL_UNORM_INT16)
      CASE(CL_UNORM_SHORT_565)
      CASE(CL_UNORM_SHORT_555)
      CASE(CL_UNORM_INT_101010)
      CASE(CL_SIGNED_INT8)
      CASE(CL_SIGNED_INT16)
      CASE(CL_SIGNED_INT32)
      CASE(CL_UNSIGNED_INT8)
      CASE(CL_UNSIGNED_INT16)
      CASE(CL_UNSIGNED_INT32)
      CASE(CL_HALF_FLOAT)
      CASE(CL_FLOAT)
      default:
        (void)fprintf(stderr, "unknown image channel data type!\n");
        abort();
    }
#undef CASE
#define CASE(ORDER)                \
  case ORDER:                      \
    (void)printf(" %s\n", #ORDER); \
    break;
    switch (format.image_channel_order) {
      CASE(CL_R)
      CASE(CL_Rx)
      CASE(CL_A)
      CASE(CL_INTENSITY)
      CASE(CL_LUMINANCE)
      CASE(CL_RG)
      CASE(CL_RGx)
      CASE(CL_RA)
      CASE(CL_RGB)
      CASE(CL_RGBx)
      CASE(CL_RGBA)
      CASE(CL_ARGB)
      CASE(CL_BGRA)
      default:
        (void)fprintf(stderr, "unknown image channel order!\n");
        abort();
    }
#undef CASE
  }
}

TEST_F(clGetSupportedImageFormatsTest, Default1D) {
  UCL::vector<cl_image_format> formats;
  cl_uint numEntries = 0;
  ASSERT_SUCCESS(clGetSupportedImageFormats(context, CL_MEM_READ_WRITE,
                                            CL_MEM_OBJECT_IMAGE1D, 0, nullptr,
                                            &numEntries));
  formats.resize(numEntries);
  ASSERT_SUCCESS(clGetSupportedImageFormats(context, CL_MEM_READ_WRITE,
                                            CL_MEM_OBJECT_IMAGE1D, numEntries,
                                            formats.data(), nullptr));
  printImageFormats(CL_MEM_OBJECT_IMAGE1D, formats);
}

TEST_F(clGetSupportedImageFormatsTest, Default1DBuffer) {
  UCL::vector<cl_image_format> formats;
  cl_uint numEntries = 0;
  ASSERT_SUCCESS(clGetSupportedImageFormats(context, CL_MEM_READ_WRITE,
                                            CL_MEM_OBJECT_IMAGE1D_BUFFER, 0,
                                            nullptr, &numEntries));
  formats.resize(numEntries);
  ASSERT_SUCCESS(clGetSupportedImageFormats(
      context, CL_MEM_READ_WRITE, CL_MEM_OBJECT_IMAGE1D_BUFFER, numEntries,
      formats.data(), nullptr));
  printImageFormats(CL_MEM_OBJECT_IMAGE1D_BUFFER, formats);
}

TEST_F(clGetSupportedImageFormatsTest, Default1DArray) {
  UCL::vector<cl_image_format> formats;
  cl_uint numEntries = 0;
  ASSERT_SUCCESS(clGetSupportedImageFormats(context, CL_MEM_READ_WRITE,
                                            CL_MEM_OBJECT_IMAGE1D_ARRAY, 0,
                                            nullptr, &numEntries));
  formats.resize(numEntries);
  ASSERT_SUCCESS(clGetSupportedImageFormats(
      context, CL_MEM_READ_WRITE, CL_MEM_OBJECT_IMAGE1D_ARRAY, numEntries,
      formats.data(), nullptr));
  printImageFormats(CL_MEM_OBJECT_IMAGE1D_ARRAY, formats);
}

TEST_F(clGetSupportedImageFormatsTest, Default2D) {
  UCL::vector<cl_image_format> formats;
  cl_uint numEntries = 0;
  ASSERT_SUCCESS(clGetSupportedImageFormats(context, CL_MEM_READ_WRITE,
                                            CL_MEM_OBJECT_IMAGE2D, 0, nullptr,
                                            &numEntries));
  formats.resize(numEntries);
  ASSERT_SUCCESS(clGetSupportedImageFormats(context, CL_MEM_READ_WRITE,
                                            CL_MEM_OBJECT_IMAGE2D, numEntries,
                                            formats.data(), nullptr));
  printImageFormats(CL_MEM_OBJECT_IMAGE2D, formats);
}

TEST_F(clGetSupportedImageFormatsTest, Default2DArray) {
  UCL::vector<cl_image_format> formats;
  cl_uint numEntries = 0;
  ASSERT_SUCCESS(clGetSupportedImageFormats(context, CL_MEM_READ_WRITE,
                                            CL_MEM_OBJECT_IMAGE2D_ARRAY, 0,
                                            nullptr, &numEntries));
  formats.resize(numEntries);
  ASSERT_SUCCESS(clGetSupportedImageFormats(
      context, CL_MEM_READ_WRITE, CL_MEM_OBJECT_IMAGE2D_ARRAY, numEntries,
      formats.data(), nullptr));
  printImageFormats(CL_MEM_OBJECT_IMAGE2D_ARRAY, formats);
}

TEST_F(clGetSupportedImageFormatsTest, Default3D) {
  UCL::vector<cl_image_format> formats;
  cl_uint numEntries = 0;
  ASSERT_SUCCESS(clGetSupportedImageFormats(context, CL_MEM_READ_WRITE,
                                            CL_MEM_OBJECT_IMAGE3D, 0, nullptr,
                                            &numEntries));
  formats.resize(numEntries);
  ASSERT_SUCCESS(clGetSupportedImageFormats(context, CL_MEM_READ_WRITE,
                                            CL_MEM_OBJECT_IMAGE3D, numEntries,
                                            formats.data(), nullptr));
  printImageFormats(CL_MEM_OBJECT_IMAGE3D, formats);
}

TEST_F(clGetSupportedImageFormatsTest, InvalidContext) {
  cl_uint numEntries = 0;
  ASSERT_EQ_ERRCODE(CL_INVALID_CONTEXT,
                    clGetSupportedImageFormats(nullptr, CL_MEM_READ_WRITE,
                                               CL_MEM_OBJECT_IMAGE2D, 0,
                                               nullptr, &numEntries));
  ASSERT_EQ(0u, numEntries);
}

struct clGetSupportedImageFormatsFlagsTest
    : ucl::ContextTest,
      testing::WithParamInterface<cl_mem_flags> {
  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    if (!getDeviceImageSupport()) {
      GTEST_SKIP();
    }
  }
};

TEST_P(clGetSupportedImageFormatsFlagsTest, InvalidValue) {
  cl_uint numEntries = 0;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetSupportedImageFormats(context, GetParam(), CL_MEM_OBJECT_IMAGE1D, 0,
                                 nullptr, &numEntries));
  ASSERT_EQ(0u, numEntries);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetSupportedImageFormats(context, GetParam(),
                                               CL_MEM_OBJECT_IMAGE1D_BUFFER, 0,
                                               nullptr, &numEntries));
  ASSERT_EQ(0u, numEntries);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetSupportedImageFormats(context, GetParam(), CL_MEM_OBJECT_IMAGE2D, 0,
                                 nullptr, &numEntries));
  ASSERT_EQ(0u, numEntries);
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetSupportedImageFormats(context, GetParam(), CL_MEM_OBJECT_IMAGE3D, 0,
                                 nullptr, &numEntries));
  ASSERT_EQ(0u, numEntries);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetSupportedImageFormats(context, GetParam(),
                                               CL_MEM_OBJECT_IMAGE1D_ARRAY, 0,
                                               nullptr, &numEntries));
  ASSERT_EQ(0u, numEntries);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetSupportedImageFormats(context, GetParam(),
                                               CL_MEM_OBJECT_IMAGE2D_ARRAY, 0,
                                               nullptr, &numEntries));
  ASSERT_EQ(0u, numEntries);
}

INSTANTIATE_TEST_CASE_P(InvalidFlags, clGetSupportedImageFormatsFlagsTest,
                        ::testing::Values<cl_mem_flags>(
                            CL_MEM_READ_WRITE | CL_MEM_READ_ONLY,
                            CL_MEM_READ_WRITE | CL_MEM_WRITE_ONLY,
                            CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR,
                            CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR,
                            CL_MEM_HOST_WRITE_ONLY | CL_MEM_HOST_READ_ONLY,
                            CL_MEM_HOST_NO_ACCESS | CL_MEM_HOST_WRITE_ONLY,
                            CL_MEM_HOST_NO_ACCESS | CL_MEM_HOST_READ_ONLY));

// TODO: TEST_F(clGetSupportedImageFormatsTest, OutOfResources) {}
// TODO: TEST_F(clGetSupportedImageFormatsTest, OutOfHostMemory) {}
