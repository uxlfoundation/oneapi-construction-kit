// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <ur_ddi.h>

#if defined(__cplusplus)
extern "C" {
#endif

UR_DLLEXPORT ur_result_t UR_APICALL urGetGlobalProcAddrTable(
    ur_api_version_t version, ur_global_dditable_t *pDdiTable) {
  if (UR_API_VERSION_CURRENT < version) {
    return UR_RESULT_ERROR_UNSUPPORTED_VERSION;
  }
  pDdiTable->pfnTearDown = urTearDown;
  pDdiTable->pfnGetLastResult = nullptr;
  pDdiTable->pfnInit = urInit;
  return UR_RESULT_SUCCESS;
}

UR_DLLEXPORT ur_result_t urGetContextProcAddrTable(
    ur_api_version_t version, ur_context_dditable_t *pDdiTable) {
  if (UR_API_VERSION_CURRENT < version) {
    return UR_RESULT_ERROR_UNSUPPORTED_VERSION;
  }
  pDdiTable->pfnCreate = urContextCreate;
  pDdiTable->pfnCreateWithNativeHandle = nullptr;
  pDdiTable->pfnGetInfo = nullptr;
  pDdiTable->pfnGetNativeHandle = nullptr;
  pDdiTable->pfnRelease = urContextRelease;
  pDdiTable->pfnRetain = urContextRetain;
  pDdiTable->pfnSetExtendedDeleter = nullptr;
  return UR_RESULT_SUCCESS;
}

UR_DLLEXPORT ur_result_t urGetEnqueueProcAddrTable(
    ur_api_version_t version, ur_enqueue_dditable_t *pDdiTable) {
  if (UR_API_VERSION_CURRENT < version) {
    return UR_RESULT_ERROR_UNSUPPORTED_VERSION;
  }
  pDdiTable->pfnDeviceGlobalVariableRead = nullptr;
  pDdiTable->pfnDeviceGlobalVariableWrite = nullptr;
  pDdiTable->pfnEventsWait = urEnqueueEventsWait;
  pDdiTable->pfnEventsWaitWithBarrier = urEnqueueEventsWaitWithBarrier;
  pDdiTable->pfnKernelLaunch = urEnqueueKernelLaunch;
  pDdiTable->pfnMemBufferCopy = urEnqueueMemBufferCopy;
  pDdiTable->pfnMemBufferCopyRect = urEnqueueMemBufferCopyRect;
  pDdiTable->pfnMemBufferFill = urEnqueueMemBufferFill;
  pDdiTable->pfnMemBufferMap = urEnqueueMemBufferMap;
  pDdiTable->pfnMemBufferRead = urEnqueueMemBufferRead;
  pDdiTable->pfnMemBufferReadRect = urEnqueueMemBufferReadRect;
  pDdiTable->pfnMemBufferWrite = urEnqueueMemBufferWrite;
  pDdiTable->pfnMemBufferWriteRect = urEnqueueMemBufferWriteRect;
  pDdiTable->pfnMemImageCopy = nullptr;
  pDdiTable->pfnMemImageRead = nullptr;
  pDdiTable->pfnMemImageWrite = nullptr;
  pDdiTable->pfnMemUnmap = urEnqueueMemUnmap;
  pDdiTable->pfnUSMFill = urEnqueueUSMFill;
  pDdiTable->pfnUSMFill2D = nullptr;
  pDdiTable->pfnUSMMemAdvise = nullptr;
  pDdiTable->pfnUSMMemcpy2D = nullptr;
  pDdiTable->pfnUSMMemcpy = urEnqueueUSMMemcpy;
  pDdiTable->pfnUSMPrefetch = nullptr;
  return UR_RESULT_SUCCESS;
}

UR_DLLEXPORT ur_result_t urGetEventProcAddrTable(
    ur_api_version_t version, ur_event_dditable_t *pDdiTable) {
  if (UR_API_VERSION_CURRENT < version) {
    return UR_RESULT_ERROR_UNSUPPORTED_VERSION;
  }

  pDdiTable->pfnCreateWithNativeHandle = nullptr;
  pDdiTable->pfnGetInfo = nullptr;
  pDdiTable->pfnGetNativeHandle = nullptr;
  pDdiTable->pfnGetProfilingInfo = nullptr;
  pDdiTable->pfnRelease = urEventRelease;
  pDdiTable->pfnRetain = nullptr;
  pDdiTable->pfnSetCallback = nullptr;
  pDdiTable->pfnWait = urEventWait;
  return UR_RESULT_SUCCESS;
}

UR_DLLEXPORT ur_result_t urGetKernelProcAddrTable(
    ur_api_version_t version, ur_kernel_dditable_t *pDdiTable) {
  if (UR_API_VERSION_CURRENT < version) {
    return UR_RESULT_ERROR_UNSUPPORTED_VERSION;
  }
  pDdiTable->pfnCreate = urKernelCreate;
  pDdiTable->pfnCreateWithNativeHandle = nullptr;
  pDdiTable->pfnGetGroupInfo = nullptr;
  pDdiTable->pfnGetInfo = nullptr;
  pDdiTable->pfnGetNativeHandle = nullptr;
  pDdiTable->pfnGetSubGroupInfo = nullptr;
  pDdiTable->pfnRelease = urKernelRelease;
  pDdiTable->pfnRetain = urKernelRetain;
  pDdiTable->pfnSetArgLocal = nullptr;
  pDdiTable->pfnSetArgMemObj = urKernelSetArgMemObj;
  pDdiTable->pfnSetArgPointer = nullptr;
  pDdiTable->pfnSetArgSampler = nullptr;
  pDdiTable->pfnSetArgValue = nullptr;
  pDdiTable->pfnSetExecInfo = nullptr;
  pDdiTable->pfnSetSpecializationConstants = nullptr;

  return UR_RESULT_SUCCESS;
}

UR_DLLEXPORT ur_result_t urGetMemProcAddrTable(ur_api_version_t version,
                                               ur_mem_dditable_t *pDdiTable) {
  if (UR_API_VERSION_CURRENT < version) {
    return UR_RESULT_ERROR_UNSUPPORTED_VERSION;
  }

  pDdiTable->pfnBufferCreate = urMemBufferCreate;
  pDdiTable->pfnBufferPartition = nullptr;
  pDdiTable->pfnCreateWithNativeHandle = nullptr;
  pDdiTable->pfnGetInfo = nullptr;
  pDdiTable->pfnGetNativeHandle = nullptr;
  pDdiTable->pfnImageCreate = nullptr;
  pDdiTable->pfnImageGetInfo = nullptr;
  pDdiTable->pfnRelease = urMemRelease;
  pDdiTable->pfnRetain = urMemRetain;

  return UR_RESULT_SUCCESS;
}

UR_DLLEXPORT ur_result_t urGetPlatformProcAddrTable(
    ur_api_version_t version, ur_platform_dditable_t *pDdiTable) {
  if (UR_API_VERSION_CURRENT < version) {
    return UR_RESULT_ERROR_UNSUPPORTED_VERSION;
  }
  pDdiTable->pfnCreateWithNativeHandle = nullptr;
  pDdiTable->pfnGet = urPlatformGet;
  pDdiTable->pfnGetApiVersion = nullptr;
  pDdiTable->pfnGetInfo = urPlatformGetInfo;
  pDdiTable->pfnGetNativeHandle = nullptr;
  return UR_RESULT_SUCCESS;
}

UR_DLLEXPORT ur_result_t urGetProgramProcAddrTable(
    ur_api_version_t version, ur_program_dditable_t *pDdiTable) {
  if (UR_API_VERSION_CURRENT < version) {
    return UR_RESULT_ERROR_UNSUPPORTED_VERSION;
  }
  pDdiTable->pfnCreateWithIL = urProgramCreateWithIL;
  pDdiTable->pfnBuild = urProgramBuild;
  pDdiTable->pfnCompile = urProgramCompile;
  pDdiTable->pfnLink = urProgramLink;
  pDdiTable->pfnCreateWithBinary = nullptr;
  pDdiTable->pfnCreateWithNativeHandle = nullptr;
  pDdiTable->pfnGetBuildInfo = nullptr;
  pDdiTable->pfnGetFunctionPointer = nullptr;
  pDdiTable->pfnGetInfo = nullptr;
  pDdiTable->pfnGetNativeHandle = nullptr;
  pDdiTable->pfnRelease = urProgramRelease;
  pDdiTable->pfnRetain = urProgramRetain;
  pDdiTable->pfnSetSpecializationConstants = nullptr;

  return UR_RESULT_SUCCESS;
}

UR_DLLEXPORT ur_result_t urGetQueueProcAddrTable(
    ur_api_version_t version, ur_queue_dditable_t *pDdiTable) {
  if (UR_API_VERSION_CURRENT < version) {
    return UR_RESULT_ERROR_UNSUPPORTED_VERSION;
  }
  pDdiTable->pfnCreate = urQueueCreate;
  pDdiTable->pfnCreateWithNativeHandle = nullptr;
  pDdiTable->pfnFinish = urQueueFinish;
  pDdiTable->pfnFlush = urQueueFlush;
  pDdiTable->pfnGetInfo = nullptr;
  pDdiTable->pfnGetNativeHandle = nullptr;
  pDdiTable->pfnRelease = urQueueRelease;
  pDdiTable->pfnRetain = urQueueRetain;
  return UR_RESULT_SUCCESS;
}

UR_DLLEXPORT ur_result_t urGetSamplerProcAddrTable(ur_api_version_t version,
                                                   ur_sampler_dditable_t *) {
  if (UR_API_VERSION_CURRENT < version) {
    return UR_RESULT_ERROR_UNSUPPORTED_VERSION;
  }
  return UR_RESULT_SUCCESS;
}

UR_DLLEXPORT ur_result_t urGetUSMProcAddrTable(ur_api_version_t version,
                                               ur_usm_dditable_t *pDdiTable) {
  if (UR_API_VERSION_CURRENT < version) {
    return UR_RESULT_ERROR_UNSUPPORTED_VERSION;
  }
  pDdiTable->pfnDeviceAlloc = urUSMDeviceAlloc;
  pDdiTable->pfnFree = urUSMFree;
  pDdiTable->pfnGetMemAllocInfo = nullptr;
  pDdiTable->pfnHostAlloc = urUSMHostAlloc;
  pDdiTable->pfnSharedAlloc = nullptr;

  return UR_RESULT_SUCCESS;
}

UR_DLLEXPORT ur_result_t urGetDeviceProcAddrTable(
    ur_api_version_t version, ur_device_dditable_t *pDdiTable) {
  if (UR_API_VERSION_CURRENT < version) {
    return UR_RESULT_ERROR_UNSUPPORTED_VERSION;
  }
  pDdiTable->pfnCreateWithNativeHandle = nullptr;
  pDdiTable->pfnGet = urDeviceGet;
  pDdiTable->pfnGetGlobalTimestamps = nullptr;
  pDdiTable->pfnGetInfo = urDeviceGetInfo;
  pDdiTable->pfnGetNativeHandle = nullptr;
  pDdiTable->pfnPartition = nullptr;
  pDdiTable->pfnRelease = nullptr;
  pDdiTable->pfnRetain = nullptr;
  pDdiTable->pfnSelectBinary = nullptr;
  return UR_RESULT_SUCCESS;
}

#if defined(__cplusplus)
}  // extern "C"
#endif
