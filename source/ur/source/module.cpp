// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "ur/module.h"

#include "ur/context.h"
#include "ur/platform.h"

cargo::expected<ur_module_handle_t, ur_result_t> ur_module_handle_t_::create(
    ur_context_handle_t context, const void *il, uint32_t length,
    cargo::string_view compilation_options) {
  // Make a copy of the source so the module can own its own copy converting the
  // length in bytes to words.
  cargo::dynamic_array<uint32_t> source_copy;
  if (cargo::success != source_copy.alloc(length / sizeof(uint32_t))) {
    return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
  }
  std::copy_n(static_cast<const uint32_t *>(il), source_copy.size(),
              std::begin(source_copy));
  // Make a copy of the compilation options so the module can own its own copy.
  cargo::dynamic_array<char> compilation_options_copy;
  if (cargo::success !=
      compilation_options_copy.alloc(compilation_options.length())) {
    return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
  }
  std::copy(std::begin(compilation_options), std::end(compilation_options),
            std::begin(compilation_options_copy));
  auto module = std::make_unique<ur_module_handle_t_>(
      context, std::move(source_copy), std::move(compilation_options_copy));
  if (!module) {
    return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
  }
  return module.release();
}

UR_APIEXPORT ur_result_t UR_APICALL
urModuleCreate(ur_context_handle_t hContext, const void *pIL, size_t length,
               const char *pOptions, ur_modulecreate_callback_t pfnNotify,
               void *pUserData, ur_module_handle_t *phModule) {
  if (!hContext) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  if (hContext->platform != ur_platform_handle_t_::instance) {
    return UR_RESULT_ERROR_UNINITIALIZED;
  }
  if (!pIL || !pOptions || !phModule) {
    // TODO: pOptions seems dubious to me, if the user doesn't want to pass any
    // options to the compiler why can't they just set this parameter to null
    // to express that? Anyway, it's in the spec like this, so leave it for the
    // time being.
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }
  // TODO: Figure out what to do with pfnNotify and pUserData.
  // These parameters are a bit confusing.
  //
  // * Firstly ur_module_handle_t doesn't
  //   do any compilation, so why these parameters are passed here and not
  //   further down the pipeline is a mystery.
  //
  // * Secondly, the type of pfnNotify is void**, which is **not** a function
  //   pointer. It is potentially a typo and should be void (*pfnNotify) or
  //   void * (*pfnNotify) although the latter is unlikely since callbacks don't
  //   often return values, so the second `*` may also be a typo.
  (void)pfnNotify;
  (void)pUserData;
  auto module = ur_module_handle_t_::create(hContext, pIL, length, pOptions);
  if (!module) {
    return module.error();
  }
  *phModule = *module;
  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urModuleRetain(ur_module_handle_t hModule) {
  if (!hModule) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  return ur::retain(hModule);
}

UR_APIEXPORT ur_result_t UR_APICALL
urModuleRelease(ur_module_handle_t hModule) {
  if (!hModule) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  return ur::release(hModule);
}
