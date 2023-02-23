// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "ur/program.h"

#include "ur/context.h"
#include "ur/device.h"
#include "ur/kernel.h"
#include "ur/module.h"
#include "ur/platform.h"

namespace ur {
kernel_data_t *program_info_t::getKernel(size_t kernel_index) {
  if (kernel_index >= kernel_descriptions.size()) {
    return nullptr;
  }
  return &kernel_descriptions[kernel_index];
}

kernel_data_t *program_info_t::getKernel(cargo::string_view name) {
  // Linear search :(
  for (auto &i : kernel_descriptions) {
    if (i.name == name) {
      return &i;
    }
  }
  return nullptr;
}
}  // namespace ur

auto getKernelInfoCallBack(ur::program_info_t &program_info) {
  return [&program_info](const compiler::KernelInfo &compiler_kernel_info) {
    auto result = program_info.kernel_descriptions.emplace_back();
    (void)result;
    auto &kernel_info = *(program_info.kernel_descriptions.end() - 1);

    kernel_info.name = compiler_kernel_info.name;
    kernel_info.attributes = compiler_kernel_info.attributes;
    kernel_info.num_arguments = compiler_kernel_info.argument_types.size();

    (void)kernel_info.argument_types.alloc(
        compiler_kernel_info.argument_types.size());
    for (size_t i = 0; i < kernel_info.argument_types.size(); ++i) {
      const auto &compiler_arg_type = compiler_kernel_info.argument_types[i];
      auto &arg_type = kernel_info.argument_types[i];
      arg_type.kind = compiler_arg_type.kind;
    }

    if (compiler_kernel_info.argument_info) {
      kernel_info.argument_info.emplace();
      (void)kernel_info.argument_info->resize(
          compiler_kernel_info.argument_info->size());
      for (size_t i = 0; i < kernel_info.argument_info->size(); ++i) {
        const auto &compiler_arg_info =
            compiler_kernel_info.argument_info.value()[i];
        auto &arg_info = kernel_info.argument_info.value()[i];
        arg_info.type_name = compiler_arg_info.type_name;
        arg_info.name = compiler_arg_info.name;
      }
    }
  };
}

ur_program_handle_t_::~ur_program_handle_t_() {
  for (const auto &device_program_pair : device_program_map) {
    const auto mux_executable = device_program_pair.second.mux_executable;
    const auto mux_device = device_program_pair.first->mux_device;
    muxDestroyExecutable(mux_device, mux_executable,
                         context->platform->mux_allocator_info);
  }
}

cargo::expected<ur_program_handle_t, ur_result_t> ur_program_handle_t_::create(
    ur_context_handle_t context,
    cargo::array_view<const ur_module_handle_t> modules,
    cargo::string_view linker_options) {
  // TODO: Handle the linker_options, this will only be relevant when we can
  // handle more than one module (see below).
  (void)linker_options;

  ur_module_handle_t ur_module = modules[0];

  auto program = std::make_unique<ur_program_handle_t_>(context);
  for (const auto &device : ur_module->context->devices) {
    ur_program_handle_t_::device_program_t device_program;

    if (!program) {
      return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
    }
    const auto target = device->target.get();
    const auto module =
        target->createModule(device_program.num_errors, device_program.log);
    if (!module) {
      return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
    }

    // TODO: Support specialization constants.
    const auto result =
        module->compileSPIRV(ur_module->source, device->spv_device_info, {});
    if (!result) {
      // TODO: It's currently unclear if it is valid to compile here and there
      // is no appropriate error code if you do and it fails.
      return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
    }

    // Link other module's device program
    std::vector<std::unique_ptr<compiler::Module>> link_modules;
    link_modules.reserve(modules.size());
    for (size_t i = 1; i < modules.size(); i++) {
      ur_module_handle_t ur_other_module = modules[i];

      auto &devices = ur_other_module->context->devices;
      auto m_device = std::find(devices.begin(), devices.end(), device);
      assert(m_device != devices.end() &&
             "All modules should have same device lists");

      ur_program_handle_t_::device_program_t device_program;

      auto target = (*m_device)->target.get();
      auto other_module =
          target->createModule(device_program.num_errors, device_program.log);
      if (!other_module) {
        return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
      }

      // TODO: Support specialization constants.
      const auto result = other_module->compileSPIRV(
          ur_other_module->source, (*m_device)->spv_device_info, {});
      if (!result) {
        // TODO: It's currently unclear if it is valid to compile here and
        // there is no appropriate error code if you do and it fails.
        return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
      }

      link_modules.emplace_back(std::move(other_module));
    }

    std::vector<compiler::Module *> v_modules;
    v_modules.reserve(link_modules.size());

    // TODO: is there a std::algo which does this? unique_ptr doesnt play well
    // with some of them
    for (size_t i = 0; i < link_modules.size(); i++) {
      v_modules.push_back(link_modules[i].get());
    }
    cargo::array_view<compiler::Module *> l_modules(v_modules);
    module->link(l_modules);

    // TODO: Support printf.
    std::vector<builtins::printf::descriptor> printf_calls;
    // TODO: Support a callback.
    // compiler::KernelInfoCallback callback;
    ur::program_info_t program_info_populate;
    if (compiler::Result::SUCCESS !=
        module->finalize(getKernelInfoCallBack(program_info_populate),
                         printf_calls)) {
      // TODO: Figure out the appropriate error code to return here.
      return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
    }
    program->program_info = std::move(program_info_populate);

    // Assume for the time being we don't support deferred compilation.
    cargo::array_view<uint8_t> executable;
    if (compiler::Result::SUCCESS != module->createBinary(executable)) {
      // TODO: Figure out the appropriate error code to return here.
      return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
    }

    if (mux_success !=
        muxCreateExecutable(device->mux_device, executable.data(),
                            executable.size(),
                            context->platform->mux_allocator_info,
                            &device_program.mux_executable)) {
      // TODO: Figure out the appropriate error code to return here.
      return cargo::make_unexpected(UR_RESULT_ERROR_OUT_OF_HOST_MEMORY);
    }

    program->device_program_map[device] = device_program;
  }

  return program.release();
}

UR_APIEXPORT ur_result_t UR_APICALL
urProgramCreate(ur_context_handle_t hContext, uint32_t count,
                const ur_module_handle_t *phModules, const char *pOptions,
                ur_program_handle_t *phProgram) {
  if (!hContext) {
    return UR_RESULT_ERROR_INVALID_NULL_HANDLE;
  }
  if (!phModules || !phProgram) {
    return UR_RESULT_ERROR_INVALID_NULL_POINTER;
  }
  auto program =
      ur_program_handle_t_::create(hContext, {phModules, count}, pOptions);
  if (!program) {
    return program.error();
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
