// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <cl/base.h>
#include <cl/buffer.h>
#include <cl/image.h>

namespace {
void destroyMemObject(cl_mem object) {
  switch (object->type) {
    case CL_MEM_OBJECT_BUFFER: {
      delete static_cast<_cl_mem_buffer*>(object);
      break;
    }
    case CL_MEM_OBJECT_IMAGE2D:
    case CL_MEM_OBJECT_IMAGE3D:
    case CL_MEM_OBJECT_IMAGE2D_ARRAY:
    case CL_MEM_OBJECT_IMAGE1D:
    case CL_MEM_OBJECT_IMAGE1D_ARRAY:
    case CL_MEM_OBJECT_IMAGE1D_BUFFER: {
      delete static_cast<_cl_mem_image*>(object);
      break;
    }
    default:
      OCL_ABORT("Unknown cl_mem type");
  }
}
}  // anonymous

namespace cl {
template <>
cl_int invalid<cl_platform_id>() {
  return CL_INVALID_PLATFORM;
}

template <>
cl_int invalid<cl_device_id>() {
  return CL_INVALID_DEVICE;
}

template <>
cl_int invalid<cl_context>() {
  return CL_INVALID_CONTEXT;
}

template <>
cl_int invalid<cl_command_queue>() {
  return CL_INVALID_COMMAND_QUEUE;
}

template <>
cl_int invalid<cl_mem>() {
  return CL_INVALID_MEM_OBJECT;
}

template <>
cl_int invalid<_cl_mem_buffer*>() {
  return CL_INVALID_MEM_OBJECT;
}

template <>
cl_int invalid<_cl_mem_image*>() {
  return CL_INVALID_MEM_OBJECT;
}

template <>
cl_int invalid<cl_sampler>() {
  return CL_INVALID_SAMPLER;
}

template <>
cl_int invalid<cl_program>() {
  return CL_INVALID_PROGRAM;
}

template <>
cl_int invalid<cl_kernel>() {
  return CL_INVALID_KERNEL;
}

template <>
cl_int invalid<cl_event>() {
  return CL_INVALID_EVENT;
}

template <>
cl_int releaseExternal<cl_mem>(cl_mem object) {
  if (!object) {
    return invalid<cl_mem>();
  }
  bool should_destroy = false;
  const cl_int error = object->releaseExternal(should_destroy);
  if (error) {
    return error;
  }
  if (should_destroy) {
    destroyMemObject(object);
  }
  return CL_SUCCESS;
}

template <>
void releaseInternal<cl_mem>(cl_mem object) {
  if (!object) {
    return;
  }
  bool should_destroy = false;
  object->releaseInternal(should_destroy);
  if (should_destroy) {
    destroyMemObject(object);
  }
}
}  // cl
