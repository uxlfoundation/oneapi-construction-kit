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

#include "ur/program.h"

#include "ur/context.h"
#include "ur/platform.h"

ur_program_handle_t_::~ur_program_handle_t_() {
  for (const auto &device_program_pair : device_program_map) {
    const auto mux_executable = device_program_pair.second.mux_executable;
    const auto mux_device = device_program_pair.first->mux_device;
    muxDestroyExecutable(mux_device, mux_executable,
                         context->platform->mux_allocator_info);
  }
}

void ur_program_handle_t_::initDevicePrograms() {
  for (auto &device : context->devices) {
    device_program_map.emplace(device, device);
  }
}

cargo::expected<ur_program_handle_t, ur_result_t> ur_program_handle_t_::create(
    ur_context_handle_t context, const void *il, uint32_t length) {
  cargo::dynamic_array<uint32_t> source_copy;
  if (cargo::success != source_copy.alloc(length / sizeof(uint32_t))) {
    return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
  }
  std::copy_n(static_cast<const uint32_t *>(il), source_copy.size(),
              std::begin(source_copy));
  auto program =
      std::make_unique<ur_program_handle_t_>(context, std::move(source_copy));

  return program.release();
}

cargo::expected<ur_program_handle_t, ur_result_t> ur_program_handle_t_::create(
    ur_context_handle_t context) {
  auto program = std::make_unique<ur_program_handle_t_>(context);

  return program.release();
}

ur_result_t ur_program_handle_t_::setOptions(cargo::string_view in_options,
                                             compiler::Options::Mode mode) {
  if (cargo::success != options.alloc(in_options.length())) {
    return UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
  }

  std::copy(std::begin(in_options), std::end(in_options), std::begin(options));

  for (auto &device : context->devices) {
    if (device_program_map[device].module->parseOptions(options, mode) !=
        compiler::Result::SUCCESS) {
      if (mode == compiler::Options::Mode::LINK) {
        return UR_RESULT_ERROR_PROGRAM_LINK_FAILURE;
      } else {
        return UR_RESULT_ERROR_PROGRAM_BUILD_FAILURE;
      }
    }
  }

  return UR_RESULT_SUCCESS;
}

ur_result_t ur_program_handle_t_::compile() {
  for (auto &device_program_iter : device_program_map) {
    // TODO: Support specialization constants.
    if (!device_program_iter.second.module->compileSPIRV(
            source, device_program_iter.first->spv_device_info, {})) {
      return UR_RESULT_ERROR_PROGRAM_BUILD_FAILURE;
    }
  }
  return UR_RESULT_SUCCESS;
}

ur_result_t ur_program_handle_t_::finalize() {
  for (auto &device_program_pair : device_program_map) {
    std::vector<builtins::printf::descriptor> printf_calls;
    compiler::ProgramInfo program_info;
    if (compiler::Result::SUCCESS !=
        device_program_pair.second.module->finalize(&program_info,
                                                    printf_calls)) {
      return UR_RESULT_ERROR_PROGRAM_BUILD_FAILURE;
    }
    device_program_pair.second.program_info = std::move(program_info);

    // Assume for the time being we don't support deferred compilation.
    cargo::array_view<uint8_t> executable;
    if (compiler::Result::SUCCESS !=
        device_program_pair.second.module->createBinary(executable)) {
      return UR_RESULT_ERROR_PROGRAM_BUILD_FAILURE;
    }

    if (mux_success !=
        muxCreateExecutable(device_program_pair.first->mux_device,
                            executable.data(), executable.size(),
                            context->platform->mux_allocator_info,
                            &device_program_pair.second.mux_executable)) {
      return UR_RESULT_ERROR_PROGRAM_BUILD_FAILURE;
    }
  }
  return UR_RESULT_SUCCESS;
}

ur_result_t ur_program_handle_t_::link(
    cargo::array_view<const ur_program_handle_t> input_programs) {
  for (auto &device : context->devices) {
    cargo::small_vector<compiler::Module *, 8> input_modules;
    for (auto &input_program : input_programs) {
      if (!input_program) {
        return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
      }

      auto input_module =
          input_program->device_program_map[device].module.get();

      if (!input_module ||
          input_module->getState() != compiler::ModuleState::COMPILED_OBJECT) {
        return UR_RESULT_ERROR_PROGRAM_LINK_FAILURE;
      }

      if (input_modules.push_back(input_module) != cargo::success) {
        return UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
      }
    }

    auto error = device_program_map[device].module->link(input_modules);
    if (error != compiler::Result::SUCCESS) {
      return UR_RESULT_ERROR_PROGRAM_LINK_FAILURE;
    }
  }
  return UR_RESULT_SUCCESS;
}
cargo::expected<const compiler::KernelInfo &, ur_result_t>
ur_program_handle_t_::getKernelData(const cargo::string_view name) {
  for (const auto &program_info_pair : device_program_map) {
    for (const auto &kernel_info : program_info_pair.second.program_info) {
      if (kernel_info.name == name) {
        return kernel_info;
      }
    }
  }
  return cargo::make_unexpected(UR_RESULT_ERROR_INVALID_KERNEL_NAME);
}

ur_result_t ur_program_handle_t_::build() {
  auto result = compile();
  if (result != UR_RESULT_SUCCESS) {
    return result;
  }

  result = finalize();
  if (result != UR_RESULT_SUCCESS) {
    return result;
  }
  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL
urProgramCreateWithIL(ur_context_handle_t hContext, const void *pIL,
                      size_t length, const ur_program_properties_t *pProperties,
                      ur_program_handle_t *phProgram) {
  (void)pProperties;
  if (!hContext) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  if (!pIL || !phProgram) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }
  if (!length) {
    return UR_RESULT_ERROR_INVALID_SIZE;
  }

  auto program = ur_program_handle_t_::create(hContext, pIL, length);
  if (!program) {
    return program.error();
  }
  *phProgram = *program;
  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urProgramBuild(ur_context_handle_t hContext,
                                                   ur_program_handle_t hProgram,
                                                   const char *pOptions) {
  if (!hContext || !hProgram) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (pOptions) {
    hProgram->setOptions(pOptions, compiler::Options::Mode::BUILD);
  }

  return hProgram->build();
}

UR_APIEXPORT ur_result_t UR_APICALL
urProgramCompile(ur_context_handle_t hContext, ur_program_handle_t hProgram,
                 const char *pOptions) {
  if (!hContext || !hProgram) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (pOptions) {
    hProgram->setOptions(pOptions, compiler::Options::Mode::COMPILE);
  }

  return hProgram->compile();
}

UR_APIEXPORT ur_result_t UR_APICALL
urProgramLink(ur_context_handle_t hContext, uint32_t count,
              const ur_program_handle_t *phPrograms, const char *pOptions,
              ur_program_handle_t *phProgram) {
  if (!hContext) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }

  if (!count) {
    return UR_RESULT_ERROR_INVALID_VALUE;
  }

  if (!phPrograms || !phProgram) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }

  auto program = ur_program_handle_t_::create(hContext);
  if (!program) {
    return program.error();
  }

  if (pOptions) {
    program.value()->setOptions(pOptions, compiler::Options::Mode::LINK);
  }

  ur_result_t error = program.value()->link(
      cargo::array_view<const ur_program_handle_t>(phPrograms, count));

  if (error != UR_RESULT_SUCCESS) {
    ur::release(*program);
    return error;
  }

  error = program.value()->finalize();

  if (error != UR_RESULT_SUCCESS) {
    ur::release(*program);
    return error;
  }

  *phProgram = *program;

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL
urProgramRetain(ur_program_handle_t hProgram) {
  if (!hProgram) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  return ur::retain(hProgram);
}

UR_APIEXPORT ur_result_t UR_APICALL
urProgramRelease(ur_program_handle_t hProgram) {
  if (!hProgram) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  return ur::release(hProgram);
}
