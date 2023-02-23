// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <CL/cl_ext_codeplay.h>
#include <cl/config.h>
#include <cl/context.h>
#include <cl/device.h>
#include <cl/macros.h>
#include <cl/program.h>
#include <compiler/module.h>
#include <extension/codeplay_program_snapshot.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

extension::codeplay_program_snapshot::codeplay_program_snapshot()
    : extension("cl_codeplay_program_snapshot",
#ifdef OCL_EXTENSION_cl_codeplay_program_snapshot
                usage_category::DEVICE
#else
                usage_category::DISABLED
#endif
                    CA_CL_EXT_VERSION(0, 2, 0)) {
}

extension::codeplay_program_snapshot::~codeplay_program_snapshot() {}

void* extension::codeplay_program_snapshot::
    GetExtensionFunctionAddressForPlatform(cl_platform_id platform,
                                           const char* func_name) const {
  OCL_UNUSED(platform);

#ifndef OCL_EXTENSION_cl_codeplay_program_snapshot
  OCL_UNUSED(platform);
  OCL_UNUSED(func_name);
  return nullptr;
#else
  if (func_name &&
      0 == strcmp("clRequestProgramSnapshotListCODEPLAY", func_name)) {
    return (void*)&clRequestProgramSnapshotListCODEPLAY;
  }

  if (func_name && 0 == strcmp("clRequestProgramSnapshotCODEPLAY", func_name)) {
    return (void*)&clRequestProgramSnapshotCODEPLAY;
  }

  return nullptr;
#endif
}

cl_int CL_API_CALL clRequestProgramSnapshotListCODEPLAY(cl_program program,
                                                        cl_device_id device,
                                                        const char** stages,
                                                        cl_uint* num_stages) {
  // Validate the program.
  OCL_CHECK(!program, return CL_INVALID_PROGRAM);

  // Validate the device.
  OCL_CHECK(!device, return CL_INVALID_DEVICE);

  // Validate input pointers.
  OCL_CHECK(!stages && !num_stages, return CL_INVALID_ARG_VALUE);

  // Validate the device.
  cl_context context = program->context;
  OCL_CHECK(!context->hasDevice(device), return CL_INVALID_DEVICE);

  // Get compiler target.
  compiler::Target *target = context->getCompilerTarget(device);
  OCL_CHECK(!target, return CL_INVALID_DEVICE);

  if (stages) {
    uint32_t total_snapshot_stages;
    if (num_stages) {
      total_snapshot_stages = *num_stages;
    } else if (compiler::Result::SUCCESS !=
               target->listSnapshotStages(0, nullptr, &total_snapshot_stages)) {
      return CL_INVALID_VALUE;
    }

    if (compiler::Result::SUCCESS !=
        target->listSnapshotStages(total_snapshot_stages, stages, nullptr)) {
      return CL_INVALID_VALUE;
    }
  } else {
    if (compiler::Result::SUCCESS !=
        target->listSnapshotStages(0, nullptr, num_stages)) {
      return CL_INVALID_VALUE;
    }
  }

  return CL_SUCCESS;
}

cl_int CL_API_CALL clRequestProgramSnapshotCODEPLAY(
    cl_program program, cl_device_id device, const char* stage,
    cl_codeplay_program_binary_format format,
    cl_codeplay_snapshot_callback_t callback, void* user_data) {
  // Validate the program.
  OCL_CHECK(!program, return CL_INVALID_PROGRAM);

  // Validate the device.
  OCL_CHECK(!device, return CL_INVALID_DEVICE);
  cl_context context = program->context;
  OCL_CHECK(!context->hasDevice(device), return CL_INVALID_DEVICE);

  // Validate the snapshot stage.
  OCL_CHECK(!stage, return CL_INVALID_ARG_VALUE);

  // Validate format.
  OCL_CHECK(format != CL_PROGRAM_BINARY_FORMAT_DEFAULT_CODEPLAY &&
                format != CL_PROGRAM_BINARY_FORMAT_TEXT_CODEPLAY &&
                format != CL_PROGRAM_BINARY_FORMAT_BINARY_CODEPLAY,
            return CL_INVALID_ARG_VALUE);

  // Validate the callback.
  OCL_CHECK(!callback, return CL_INVALID_ARG_VALUE);

  // Get compiler target.
  compiler::Target *target = context->getCompilerTarget(device);
  OCL_CHECK(!target, return CL_INVALID_DEVICE);

  // Check snapshot stage exists
  std::vector<const char*> stages;
  uint32_t total_snapshot_stages;
  OCL_CHECK(compiler::Result::SUCCESS !=
                target->listSnapshotStages(0, nullptr, &total_snapshot_stages),
            return CL_INVALID_VALUE);

  stages.resize(total_snapshot_stages);
  OCL_CHECK(compiler::Result::SUCCESS !=
                target->listSnapshotStages(total_snapshot_stages, stages.data(),
                                           nullptr),
            return CL_INVALID_VALUE);

  if (std::find(stages.begin(), stages.end(), std::string(stage)) ==
      stages.end()) {
    return CL_INVALID_ARG_VALUE;
  }

  // Validate that the program is compiled.
  OCL_CHECK(program->programs[device].type !=
                cl::device_program_type::COMPILER_MODULE,
            return CL_INVALID_PROGRAM);

  // Make sure that the program was not compiled already.
  OCL_CHECK(!(program->programs[device].compiler_module.module->getState() ==
                  compiler::ModuleState::NONE ||
              program->programs[device].compiler_module.module->getState() ==
                  compiler::ModuleState::INTERMEDIATE),
            return CL_INVALID_PROGRAM_EXECUTABLE);

  // Mark the binary as needing a snapshot taken.
  compiler::SnapshotFormat snapshot_format;
  switch (format) {
    case CL_PROGRAM_BINARY_FORMAT_DEFAULT_CODEPLAY:
      snapshot_format = compiler::SnapshotFormat::DEFAULT;
      break;
    case CL_PROGRAM_BINARY_FORMAT_TEXT_CODEPLAY:
      snapshot_format = compiler::SnapshotFormat::TEXT;
      break;
    case CL_PROGRAM_BINARY_FORMAT_BINARY_CODEPLAY:
      snapshot_format = compiler::SnapshotFormat::BINARY;
      break;
    default:
      // Format is already checked above but compilers warn about missing
      // default cases.
      return CL_INVALID_ARG_VALUE;
      break;
  }
  program->programs[device].compiler_module.module->setSnapshotCallback(
      stage, callback, user_data, snapshot_format);

  return CL_SUCCESS;
}
