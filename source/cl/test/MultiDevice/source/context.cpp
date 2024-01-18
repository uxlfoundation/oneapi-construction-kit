#include <common.h>

TEST_F(MultiDeviceContext, CreateCommandQueues) {
  cl_int error;
  std::vector<cl_command_queue> command_queues;
  for (auto device : devices) {
    command_queues.push_back(clCreateCommandQueue(context, device, 0, &error));
    EXPECT_EQ(CL_SUCCESS, error);
  }
  for (auto command_queue : command_queues) {
    EXPECT_EQ(CL_SUCCESS, clReleaseCommandQueue(command_queue));
  }
}

TEST_F(MultiDeviceContext, CreateBuffer) {
  cl_int error;
  auto buffer =
      clCreateBuffer(context, CL_MEM_READ_WRITE, 256, nullptr, &error);
  ASSERT_EQ(CL_SUCCESS, error);
  cl_context bufferContext;
  EXPECT_EQ(CL_SUCCESS,
            clGetMemObjectInfo(buffer, CL_MEM_CONTEXT, sizeof(cl_context),
                               &bufferContext, nullptr));
  EXPECT_EQ(context, bufferContext);
  ASSERT_EQ(CL_SUCCESS, clReleaseMemObject(buffer));
}

TEST_F(MultiDeviceContext, CreateImage) {
  if (hasImageSupport()) {
    cl_int error;
    const cl_image_format image_format = {CL_RGBA, CL_UNORM_INT8};
    const cl_image_desc image_desc = []() {
      cl_image_desc image_desc;
      image_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
      image_desc.image_width = 128;
      image_desc.image_height = 128;
      image_desc.image_depth = 0;
      image_desc.image_array_size = 0;
      image_desc.image_row_pitch = 0;
      image_desc.image_slice_pitch = 0;
      image_desc.num_mip_levels = 0;
      image_desc.num_samples = 0;
      image_desc.buffer = nullptr;
      return image_desc;
    }();
    auto image = clCreateImage(context, CL_MEM_READ_WRITE, &image_format,
                               &image_desc, nullptr, &error);
    ASSERT_EQ(CL_SUCCESS, error);
    cl_context imageContext;
    EXPECT_EQ(CL_SUCCESS,
              clGetMemObjectInfo(image, CL_MEM_CONTEXT, sizeof(cl_context),
                                 &imageContext, nullptr));
    EXPECT_EQ(context, imageContext);
    ASSERT_EQ(CL_SUCCESS, clReleaseMemObject(image));
  }
}

TEST_F(MultiDeviceContext, CreateProgram) {
  const char *source = "kernel void foo() {}";
  const size_t length = strlen(source);
  cl_int error;
  auto program =
      clCreateProgramWithSource(context, 1, &source, &length, &error);
  ASSERT_EQ(CL_SUCCESS, error);
  ASSERT_EQ(CL_SUCCESS, clReleaseProgram(program));
}

TEST_F(MultiDeviceContext, CreateKernel) {
  if (!hasCompilerSupport()) {
    GTEST_SKIP();
  }
  const char *source = "kernel void foo() {}";
  const size_t length = strlen(source);
  cl_int error;
  auto program =
      clCreateProgramWithSource(context, 1, &source, &length, &error);
  ASSERT_EQ(CL_SUCCESS, error);
  ASSERT_EQ(CL_SUCCESS, clBuildProgram(program, devices.size(), devices.data(),
                                       nullptr, nullptr, nullptr));
  auto kernel = clCreateKernel(program, "foo", &error);
  EXPECT_EQ(CL_SUCCESS, error);
  EXPECT_EQ(CL_SUCCESS, clReleaseKernel(kernel));
  ASSERT_EQ(CL_SUCCESS, clReleaseProgram(program));
}
