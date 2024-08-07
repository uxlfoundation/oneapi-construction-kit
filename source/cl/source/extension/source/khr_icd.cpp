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

#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <cl/buffer.h>
#include <cl/command_queue.h>
#include <cl/config.h>
#include <cl/context.h>
#include <cl/device.h>
#include <cl/event.h>
#include <cl/image.h>
#include <cl/kernel.h>
#include <cl/macros.h>
#if defined(CL_VERSION_3_0)
#include <cl/opencl-3.0.h>
#endif
#include <cl/platform.h>
#include <cl/program.h>
#include <cl/sampler.h>
#include <extension/khr_icd.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

extension::khr_icd::khr_icd()
    : extension("cl_khr_icd",
#ifdef OCL_EXTENSION_cl_khr_icd
                usage_category::PLATFORM
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(1, 0, 0)) {
}

cl_int extension::khr_icd::GetPlatformInfo(cl_platform_id platform,
                                           cl_platform_info param_name,
                                           size_t param_value_size,
                                           void *param_value,
                                           size_t *param_value_size_ret) const {
  std::size_t value_size;
  const void *value_pointer = nullptr;

  switch (param_name) {
    case CL_PLATFORM_ICD_SUFFIX_KHR: {
      // See The OpenCL Extension Specification, version: 1.2, document
      // revision: 19, last revision date: 1/16/14, section 9.18.10
      // Additions to Chapter 9 of the OpenCL 1.2 Extension Specification
      static const char icd_suffix[] = "CODEPLAY";
      value_size = sizeof(icd_suffix);
      value_pointer = icd_suffix;
      break;
    }
    default: {
      // Call default implementation that uses the constructor set name.
      return extension::GetPlatformInfo(platform, param_name, param_value_size,
                                        param_value, param_value_size_ret);
    }
  }

  OCL_CHECK(nullptr != param_value && param_value_size < value_size,
            return CL_INVALID_VALUE);

  if (nullptr != param_value) {
    std::memcpy(param_value, value_pointer, value_size);
  }

  OCL_SET_IF_NOT_NULL(param_value_size_ret, value_size);

  return CL_SUCCESS;
}

namespace {
// For functions we implement, we obtain the type from the function prototype
// in other cases we use void function pointers. We do not want to include all
// the Khronos headers, as they include system headers like Direct3D headers
// which might not be available on all platforms.
#define ICD_FUNCTION(F) decltype(&cl::F) function_cl##F = cl::F;
#define OCL_ICD_NOT_IMPLEMENTED(T) void (*function_##T)(void) = nullptr;

// Dispatch table. The order must match that in the Khronos provided example
// icd loader icd_dispatch.h.
// TODO CA-2619: Use dispatch table from upstream headers for a more long term
// solution to maintaining the ordering of function pointers in this struct.
const struct icd_dispatch_table_t {
  ICD_FUNCTION(GetPlatformIDs)
  ICD_FUNCTION(GetPlatformInfo)
  ICD_FUNCTION(GetDeviceIDs)
  ICD_FUNCTION(GetDeviceInfo)
  ICD_FUNCTION(CreateContext)
  ICD_FUNCTION(CreateContextFromType)
  ICD_FUNCTION(RetainContext)
  ICD_FUNCTION(ReleaseContext)
  ICD_FUNCTION(GetContextInfo)
  ICD_FUNCTION(CreateCommandQueue)
  ICD_FUNCTION(RetainCommandQueue)
  ICD_FUNCTION(ReleaseCommandQueue)
  ICD_FUNCTION(GetCommandQueueInfo)
  ICD_FUNCTION(SetCommandQueueProperty)
  ICD_FUNCTION(CreateBuffer)
  ICD_FUNCTION(CreateImage2D)
  ICD_FUNCTION(CreateImage3D)
  ICD_FUNCTION(RetainMemObject)
  ICD_FUNCTION(ReleaseMemObject)
  ICD_FUNCTION(GetSupportedImageFormats)
  ICD_FUNCTION(GetMemObjectInfo)
  ICD_FUNCTION(GetImageInfo)
  ICD_FUNCTION(CreateSampler)
  ICD_FUNCTION(RetainSampler)
  ICD_FUNCTION(ReleaseSampler)
  ICD_FUNCTION(GetSamplerInfo)
  ICD_FUNCTION(CreateProgramWithSource)
  ICD_FUNCTION(CreateProgramWithBinary)
  ICD_FUNCTION(RetainProgram)
  ICD_FUNCTION(ReleaseProgram)
  ICD_FUNCTION(BuildProgram)
  ICD_FUNCTION(UnloadCompiler)
  ICD_FUNCTION(GetProgramInfo)
  ICD_FUNCTION(GetProgramBuildInfo)
  ICD_FUNCTION(CreateKernel)
  ICD_FUNCTION(CreateKernelsInProgram)
  ICD_FUNCTION(RetainKernel)
  ICD_FUNCTION(ReleaseKernel)
  ICD_FUNCTION(SetKernelArg)
  ICD_FUNCTION(GetKernelInfo)
  ICD_FUNCTION(GetKernelWorkGroupInfo)
  ICD_FUNCTION(WaitForEvents)
  ICD_FUNCTION(GetEventInfo)
  ICD_FUNCTION(RetainEvent)
  ICD_FUNCTION(ReleaseEvent)
  ICD_FUNCTION(GetEventProfilingInfo)
  ICD_FUNCTION(Flush)
  ICD_FUNCTION(Finish)
  ICD_FUNCTION(EnqueueReadBuffer)
  ICD_FUNCTION(EnqueueWriteBuffer)
  ICD_FUNCTION(EnqueueCopyBuffer)
  ICD_FUNCTION(EnqueueReadImage)
  ICD_FUNCTION(EnqueueWriteImage)
  ICD_FUNCTION(EnqueueCopyImage)
  ICD_FUNCTION(EnqueueCopyImageToBuffer)
  ICD_FUNCTION(EnqueueCopyBufferToImage)
  ICD_FUNCTION(EnqueueMapBuffer)
  ICD_FUNCTION(EnqueueMapImage)
  ICD_FUNCTION(EnqueueUnmapMemObject)
  ICD_FUNCTION(EnqueueNDRangeKernel)
  ICD_FUNCTION(EnqueueTask)
  ICD_FUNCTION(EnqueueNativeKernel)
  ICD_FUNCTION(EnqueueMarker)
  ICD_FUNCTION(EnqueueWaitForEvents)
  ICD_FUNCTION(EnqueueBarrier)
  ICD_FUNCTION(GetExtensionFunctionAddress)
  OCL_ICD_NOT_IMPLEMENTED(clCreateFromGLBuffer)
  OCL_ICD_NOT_IMPLEMENTED(clCreateFromGLTexture2D)
  OCL_ICD_NOT_IMPLEMENTED(clCreateFromGLTexture3D)
  OCL_ICD_NOT_IMPLEMENTED(clCreateFromGLRenderbuffer)
  OCL_ICD_NOT_IMPLEMENTED(clGetGLObjectInfo)
  OCL_ICD_NOT_IMPLEMENTED(clGetGLTextureInfo)
  OCL_ICD_NOT_IMPLEMENTED(clEnqueueAcquireGLObjects)
  OCL_ICD_NOT_IMPLEMENTED(clEnqueueReleaseGLObjects)
  OCL_ICD_NOT_IMPLEMENTED(clGetGLContextInfoKHR)

  OCL_ICD_NOT_IMPLEMENTED(clGetDeviceIDsFromD3D10KHR)
  OCL_ICD_NOT_IMPLEMENTED(clCreateFromD3D10BufferKHR)
  OCL_ICD_NOT_IMPLEMENTED(clCreateFromD3D10Texture2DKHR)
  OCL_ICD_NOT_IMPLEMENTED(clCreateFromD3D10Texture3DKHR)
  OCL_ICD_NOT_IMPLEMENTED(clEnqueueAcquireD3D10ObjectsKHR)
  OCL_ICD_NOT_IMPLEMENTED(clEnqueueReleaseD3D10ObjectsKHR)

  ICD_FUNCTION(SetEventCallback)
  ICD_FUNCTION(CreateSubBuffer)
  ICD_FUNCTION(SetMemObjectDestructorCallback)
  ICD_FUNCTION(CreateUserEvent)
  ICD_FUNCTION(SetUserEventStatus)
  ICD_FUNCTION(EnqueueReadBufferRect)
  ICD_FUNCTION(EnqueueWriteBufferRect)
  ICD_FUNCTION(EnqueueCopyBufferRect)

  OCL_ICD_NOT_IMPLEMENTED(clCreateSubDevicesEXT)
  OCL_ICD_NOT_IMPLEMENTED(clRetainDeviceEXT)
  OCL_ICD_NOT_IMPLEMENTED(clReleaseDeviceEXT)

  OCL_ICD_NOT_IMPLEMENTED(clCreateEventFromGLsyncKHR)

  ICD_FUNCTION(CreateSubDevices)
  ICD_FUNCTION(RetainDevice)
  ICD_FUNCTION(ReleaseDevice)
  ICD_FUNCTION(CreateImage)
  ICD_FUNCTION(CreateProgramWithBuiltInKernels)
  ICD_FUNCTION(CompileProgram)
  ICD_FUNCTION(LinkProgram)
  ICD_FUNCTION(UnloadPlatformCompiler)
  ICD_FUNCTION(GetKernelArgInfo)
  ICD_FUNCTION(EnqueueFillBuffer)
  ICD_FUNCTION(EnqueueFillImage)
  ICD_FUNCTION(EnqueueMigrateMemObjects)
  ICD_FUNCTION(EnqueueMarkerWithWaitList)
  ICD_FUNCTION(EnqueueBarrierWithWaitList)
  ICD_FUNCTION(GetExtensionFunctionAddressForPlatform)
  OCL_ICD_NOT_IMPLEMENTED(clCreateFromGLTexture)

  OCL_ICD_NOT_IMPLEMENTED(clGetDeviceIDsFromD3D11KHR)
  OCL_ICD_NOT_IMPLEMENTED(clCreateFromD3D11BufferKHR)
  OCL_ICD_NOT_IMPLEMENTED(clCreateFromD3D11Texture2DKHR)
  OCL_ICD_NOT_IMPLEMENTED(clCreateFromD3D11Texture3DKHR)
  OCL_ICD_NOT_IMPLEMENTED(clCreateFromDX9MediaSurfaceKHR)
  OCL_ICD_NOT_IMPLEMENTED(clEnqueueAcquireD3D11ObjectsKHR)
  OCL_ICD_NOT_IMPLEMENTED(clEnqueueReleaseD3D11ObjectsKHR)

  OCL_ICD_NOT_IMPLEMENTED(clGetDeviceIDsFromDX9MediaAdapterKHR)
  OCL_ICD_NOT_IMPLEMENTED(clEnqueueAcquireDX9MediaSurfacesKHR)
  OCL_ICD_NOT_IMPLEMENTED(clEnqueueReleaseDX9MediaSurfacesKHR)

  OCL_ICD_NOT_IMPLEMENTED(CreateFromEGLImageKHR)
  OCL_ICD_NOT_IMPLEMENTED(EnqueueAcquireEGLObjectsKHR)
  OCL_ICD_NOT_IMPLEMENTED(EnqueueReleaseEGLObjectsKHR)

  OCL_ICD_NOT_IMPLEMENTED(CreateEventFromEGLSyncKHR)

  // OpenCL-2.0.
  // Some of these APIs are optional in 3.0.
#if defined(OCL_EXTENSION_cl_khr_icd) && defined(CL_VERSION_3_0)
  ICD_FUNCTION(CreateCommandQueueWithProperties)
  ICD_FUNCTION(CreatePipe)
  ICD_FUNCTION(GetPipeInfo)
  ICD_FUNCTION(SVMAlloc)
  ICD_FUNCTION(SVMFree)
  ICD_FUNCTION(EnqueueSVMFree)
  ICD_FUNCTION(EnqueueSVMMemcpy)
  ICD_FUNCTION(EnqueueSVMMemFill)
  ICD_FUNCTION(EnqueueSVMMap)
  ICD_FUNCTION(EnqueueSVMUnmap)
  ICD_FUNCTION(CreateSamplerWithProperties)
  ICD_FUNCTION(SetKernelArgSVMPointer)
  ICD_FUNCTION(SetKernelExecInfo)
#else
  OCL_ICD_NOT_IMPLEMENTED(CreateCommandQueueWithProperties)
  OCL_ICD_NOT_IMPLEMENTED(CreatePipe)
  OCL_ICD_NOT_IMPLEMENTED(GetPipeInfo)
  OCL_ICD_NOT_IMPLEMENTED(SVMAlloc)
  OCL_ICD_NOT_IMPLEMENTED(SVMFree)
  OCL_ICD_NOT_IMPLEMENTED(EnqueueSVMFree)
  OCL_ICD_NOT_IMPLEMENTED(EnqueueSVMMemcpy)
  OCL_ICD_NOT_IMPLEMENTED(EnqueueSVMMemFill)
  OCL_ICD_NOT_IMPLEMENTED(EnqueueSVMMap)
  OCL_ICD_NOT_IMPLEMENTED(EnqueueSVMUnmap)
  OCL_ICD_NOT_IMPLEMENTED(CreateSamplerWithProperties)
  OCL_ICD_NOT_IMPLEMENTED(SetKernelArgSVMPointer)
  OCL_ICD_NOT_IMPLEMENTED(SetKernelExecInfo)
#endif

  OCL_ICD_NOT_IMPLEMENTED(GetKernelSubGroupInfoKHR)

  // OpenCL-2.1.
  // Some of these APIs are optional in 3.0.
#if defined(OCL_EXTENSION_cl_khr_icd) && defined(CL_VERSION_3_0)
  ICD_FUNCTION(CloneKernel)
  ICD_FUNCTION(CreateProgramWithIL)
  ICD_FUNCTION(EnqueueSVMMigrateMem)
  ICD_FUNCTION(GetDeviceAndHostTimer)
  ICD_FUNCTION(GetHostTimer)
  ICD_FUNCTION(GetKernelSubGroupInfo)
  ICD_FUNCTION(SetDefaultDeviceCommandQueue)
#else
  OCL_ICD_NOT_IMPLEMENTED(CloneKernel)
  OCL_ICD_NOT_IMPLEMENTED(CreateProgramWithIL)
  OCL_ICD_NOT_IMPLEMENTED(EnqueueSVMMigrateMem)
  OCL_ICD_NOT_IMPLEMENTED(GetDeviceAndHostTimer)
  OCL_ICD_NOT_IMPLEMENTED(GetHostTimer)
  OCL_ICD_NOT_IMPLEMENTED(GetKernelSubGroupInfo)
  OCL_ICD_NOT_IMPLEMENTED(SetDefaultDeviceCommandQueue)
#endif

  // OpenCL-2.2.
  // Some of these Apis are optional in 3.0.
#if defined(OCL_EXTENSION_cl_khr_icd) && defined(CL_VERSION_3_0)
  ICD_FUNCTION(SetProgramReleaseCallback)
  ICD_FUNCTION(SetProgramSpecializationConstant)
#else
  OCL_ICD_NOT_IMPLEMENTED(SetProgramReleaseCallback)
  OCL_ICD_NOT_IMPLEMENTED(SetProgramSpecializationConstant)
#endif

  // OpenCL-3.0.
#if defined(OCL_EXTENSION_cl_khr_icd) && defined(CL_VERSION_3_0)
  ICD_FUNCTION(CreateBufferWithProperties)
  ICD_FUNCTION(CreateImageWithProperties)
  ICD_FUNCTION(SetContextDestructorCallback)
#else
  OCL_ICD_NOT_IMPLEMENTED(CreateBufferWithProperties)
  OCL_ICD_NOT_IMPLEMENTED(CreateImageWithProperties)
  OCL_ICD_NOT_IMPLEMENTED(SetContextDestructorCallback)
#endif
} icd_dispatch_table{};

#undef ICD_FUNCTION
#undef OCL_ICD_NOT_IMPLEMENTED

cl_int CL_API_CALL IcdGetPlatformIDsKHR(cl_uint num_entries,
                                        cl_platform_id *platforms,
                                        cl_uint *num_platforms) {
  return cl::GetPlatformIDs(num_entries, platforms, num_platforms);
}
}  // namespace

CL_API_ENTRY cl_int CL_API_CALL clIcdGetPlatformIDsKHR(
    cl_uint num_entries, cl_platform_id *platforms, cl_uint *num_platforms) {
  return IcdGetPlatformIDsKHR(num_entries, platforms, num_platforms);
}

void *extension::khr_icd::GetExtensionFunctionAddressForPlatform(
    cl_platform_id platform, const char *func_name) const {
  OCL_UNUSED(platform);
#ifndef OCL_EXTENSION_cl_khr_icd
  OCL_UNUSED(func_name);
#else
  if (func_name && (std::strcmp("clIcdGetPlatformIDsKHR", func_name) == 0)) {
    return (void *)&IcdGetPlatformIDsKHR;
  }
#endif
  return nullptr;
}

const void *extension::khr_icd::GetIcdDispatchTable() {
  return reinterpret_cast<const void *>(&icd_dispatch_table);
}
