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

#include <CL/cl_ext.h>
#include <cargo/small_vector.h>
#include <cargo/string_algorithm.h>
#include <cl/config.h>
#include <cl/context.h>
#include <cl/device.h>
#include <cl/macros.h>
#include <cl/mux.h>
#include <cl/program.h>
#include <cl/validate.h>
#include <tracer/tracer.h>

#include <algorithm>
#include <memory>
#include <mutex>
#include <unordered_set>
namespace {
cl_int convertModuleStateToCL(compiler::ModuleState state) {
  switch (state) {
    case compiler::ModuleState::NONE:
      return CL_PROGRAM_BINARY_TYPE_NONE;
    case compiler::ModuleState::INTERMEDIATE:
      return CL_PROGRAM_BINARY_TYPE_INTERMEDIATE;
    case compiler::ModuleState::COMPILED_OBJECT:
      return CL_PROGRAM_BINARY_TYPE_COMPILED_OBJECT;
    case compiler::ModuleState::LIBRARY:
      return CL_PROGRAM_BINARY_TYPE_LIBRARY;
    case compiler::ModuleState::EXECUTABLE:
      return CL_PROGRAM_BINARY_TYPE_EXECUTABLE;
    default:
      return CL_PROGRAM_BINARY_TYPE_NONE;
  }
}
}  // namespace

cl::mux_kernel_cache::mux_kernel_cache()
    : use_builtin_kernels(true),
      mux_executable(nullptr, {nullptr, {nullptr, nullptr, nullptr}}) {}

cl::mux_kernel_cache::mux_kernel_cache(
    mux::unique_ptr<mux_executable_t> executable)
    : use_builtin_kernels(false), mux_executable(std::move(executable)) {}

cargo::expected<mux_kernel_t, mux_result_t>
cl::mux_kernel_cache::getOrCreateKernel(cl_device_id device,
                                        const std::string &name) {
  {
    const std::lock_guard<std::mutex> guard{kernel_map_mutex};
    auto kernel_it = kernel_map.find(name);
    if (kernel_it != kernel_map.end()) {
      return kernel_it->second.get();
    }
  }

  mux_kernel_t kernel;
  mux_result_t result = mux_success;
  if (use_builtin_kernels) {
    result =
        muxCreateBuiltInKernel(device->mux_device, name.c_str(), name.size(),
                               device->mux_allocator, &kernel);
  } else {
    result =
        muxCreateKernel(device->mux_device, mux_executable.get(), name.c_str(),
                        name.size(), device->mux_allocator, &kernel);
  }
  if (mux_success != result) {
    return cargo::make_unexpected(result);
  }

  {
    const std::lock_guard<std::mutex> guard{kernel_map_mutex};
    kernel_map.emplace(
        name, mux::unique_ptr<mux_kernel_t>(
                  kernel, {device->mux_device, device->mux_allocator}));
  }
  return kernel;
}

cl::device_program::device_program()
    : num_errors(0), type(cl::device_program_type::NONE) {}

cl::device_program::~device_program() { clear(); }

void cl::device_program::initializeAsBinary(
    mux::unique_ptr<mux_executable_t> executable,
    cargo::dynamic_array<uint8_t> binary_buffer) {
  clear();
  type = cl::device_program_type::BINARY;
  // As 'binary' is defined in a union, it must be explicitly constructed.
  new (&binary) Binary(std::move(executable), std::move(binary_buffer));
}

void cl::device_program::initializeAsBuiltin() {
  clear();
  type = cl::device_program_type::BUILTIN;
  // As 'builtin_kernels' is defined in a union, it must be explicitly
  // constructed.
  new (&builtin) Builtin();
}

void cl::device_program::initializeAsCompilerModule(compiler::Target *target) {
  clear();
  type = cl::device_program_type::COMPILER_MODULE;
  // As 'compiler_module' is defined in a union, it must be explicitly
  // constructed.
  new (&compiler_module) CompilerModule();
  compiler_module.module = target->createModule(num_errors, compiler_log);
}

void cl::device_program::reportError(cargo::string_view error) {
  num_errors++;
  compiler_log.append(error.data(), error.size());
  compiler_log += "\n";
}

bool cl::device_program::isExecutable() const {
  switch (type) {
    case cl::device_program_type::BINARY:
      return true;
    case cl::device_program_type::BUILTIN:
      return true;
    case cl::device_program_type::COMPILER_MODULE:
      return compiler_module.module->getState() ==
             compiler::ModuleState::EXECUTABLE;
    default:
      return false;
  }
}

bool cl::device_program::finalize(cl_device_id device) {
  if (type == cl::device_program_type::BUILTIN) {
    // Extract type metadata from declarations.
    program_info =
        cl::binary::kernelDeclsToProgramInfo(builtin.kernel_decls, true);
    if (!program_info) {
      return false;
    }
  } else if (type == cl::device_program_type::COMPILER_MODULE) {
    // Finalise the module.
    compiler::ProgramInfo program_info_to_populate;
    if (compiler::Result::SUCCESS !=
        compiler_module.module->finalize(&program_info_to_populate,
                                         printf_calls)) {
      return false;
    }
    if (num_errors != 0) {
      return false;
    }
    program_info = std::move(program_info_to_populate);

    // If the compiler does not support deferred compilation, we get the final
    // binary from the module and initialize a mux_kernel_cache.
    if (!device->compiler_info->supports_deferred_compilation) {
      // Get the Mux binary.
      auto binary_result = compiler_module.getOrCreateMuxBinary();
      if (!binary_result) {
        return false;
      }

      // Create the Mux executable and set up the mux_kernel_cache.
      cargo::array_view<const uint8_t> binary = *binary_result;
      mux_executable_t mux_executable = nullptr;
      if (mux_success !=
          muxCreateExecutable(device->mux_device, binary.data(), binary.size(),
                              device->mux_allocator, &mux_executable)) {
        reportError("Failed to create Mux executable.");
        return false;
      }
      compiler_module.kernels.emplace(mux::unique_ptr<mux_executable_t>{
          mux_executable, {device->mux_device, device->mux_allocator}});
    }
  }
  return true;
}

cargo::expected<std::unique_ptr<MuxKernelWrapper>, cl_int>
cl::device_program::createKernel(cl_device_id device,
                                 const std::string &kernel_name) {
  std::unique_ptr<MuxKernelWrapper> kernel_wrapper;
  switch (type) {
    case cl::device_program_type::BINARY: {
      auto mux_kernel_result =
          binary.kernels.getOrCreateKernel(device, kernel_name);
      if (!mux_kernel_result) {
        return cargo::make_unexpected(
            cl::getErrorFrom(mux_kernel_result.error()));
      }
      kernel_wrapper.reset(
          new MuxKernelWrapper(device, mux_kernel_result.value()));
      if (!kernel_wrapper) {
        return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
      }
    } break;

    case cl::device_program_type::BUILTIN: {
      auto mux_kernel_result =
          builtin.kernels.getOrCreateKernel(device, kernel_name);
      if (!mux_kernel_result) {
        return cargo::make_unexpected(
            cl::getErrorFrom(mux_kernel_result.error()));
      }
      kernel_wrapper.reset(
          new MuxKernelWrapper(device, mux_kernel_result.value()));
      if (!kernel_wrapper) {
        return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
      }
    } break;

    case cl::device_program_type::COMPILER_MODULE: {
      if (device->compiler_info->supports_deferred_compilation) {
        compiler::Kernel *deferred_kernel =
            compiler_module.module->getKernel(kernel_name);
        if (!deferred_kernel) {
          return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
        }
        kernel_wrapper.reset(new MuxKernelWrapper(device, deferred_kernel));
        if (!kernel_wrapper) {
          return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
        }
      } else {
        auto mux_kernel_result =
            compiler_module.kernels->getOrCreateKernel(device, kernel_name);
        if (!mux_kernel_result) {
          return cargo::make_unexpected(
              cl::getErrorFrom(mux_kernel_result.error()));
        }
        kernel_wrapper.reset(
            new MuxKernelWrapper(device, mux_kernel_result.value()));
        if (!kernel_wrapper) {
          return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
        }
      }
    } break;

    default:
      return cargo::make_unexpected(CL_INVALID_PROGRAM);
  }

  return {std::move(kernel_wrapper)};
}

size_t cl::device_program::binarySize() {
  if (type == cl::device_program_type::NONE ||
      type == cl::device_program_type::BUILTIN) {
    return 0;
  }
  if (type == cl::device_program_type::BINARY) {
    return binary.binary.size();
  }

  if (compiler_module.cached_binary.has_value()) {
    return compiler_module.cached_binary->size();
  }

  if (compiler_module.module->getState() == compiler::ModuleState::NONE) {
    return 0;
  }

  return binarySerialize().size();
}

cargo::array_view<uint8_t> cl::device_program::binarySerialize() {
  if (type == cl::device_program_type::NONE ||
      type == cl::device_program_type::BUILTIN) {
    return {};
  }
  if (type == cl::device_program_type::BINARY) {
    return binary.binary;
  }

  if (compiler_module.module->getState() == compiler::ModuleState::NONE) {
    return {};
  }

  if (!compiler_module.cached_binary.has_value()) {
    auto &cached_binary = compiler_module.cached_binary.emplace();
    const bool is_executable =
        compiler_module.module->getState() == compiler::ModuleState::EXECUTABLE;
    if (is_executable) {
      auto mux_binary_result = compiler_module.getOrCreateMuxBinary();
      if (!mux_binary_result) {
        reportError("Failed to create binary from the compiler module");
        return {};
      }
      if (!cl::binary::serializeBinary(
              cached_binary, mux_binary_result.value(), printf_calls,
              *program_info,
              compiler_module.module->getOptions().kernel_arg_info, nullptr)) {
        reportError("Failed to serialize binary");
        return {};
      }
    } else {
      if (!cl::binary::serializeBinary(cached_binary, {}, printf_calls,
                                       *program_info, false,
                                       compiler_module.module.get())) {
        reportError("Failed to serialize binary");
        return {};
      }
    }
  }
  return *compiler_module.cached_binary;
}

bool cl::device_program::binaryDeserialize(
    cl_device_id device, compiler::Target *compiler_target,
    cargo::array_view<const uint8_t> buffer) {
  cargo::dynamic_array<uint8_t> executable;
  bool is_executable = false;
  program_info.emplace();
  if (!binary::deserializeBinary(buffer, printf_calls, *program_info,
                                 executable, is_executable)) {
    reportError("Failed to deserialize binary");
    return false;
  }
  if (is_executable) {
    mux_executable_t mux_executable = nullptr;
    if (mux_success != muxCreateExecutable(device->mux_device,
                                           executable.data(), executable.size(),
                                           device->mux_allocator,
                                           &mux_executable)) {
      reportError("Failed to create Mux executable.");
      return false;
    };

    // cache the binary
    cargo::dynamic_array<uint8_t> binary;
    if (binary.alloc(buffer.size())) {
      return false;
    }
    std::memcpy(binary.data(), buffer.data(), buffer.size());

    // Initialize as binary.
    initializeAsBinary(
        mux::unique_ptr<mux_executable_t>{
            mux_executable, {device->mux_device, device->mux_allocator}},
        std::move(binary));
  } else {
    // Note that as we are handling binary deserialization above, this case is
    // for handling the case where an OpenCL binary has been serialized after
    // compilation, but before finalization (i.e. the internal LLVM module is in
    // an intermediate state of compilation).
    if (device->compiler_available) {
      initializeAsCompilerModule(compiler_target);
      if (!compiler_module.module->deserialize(
              {executable.data(), executable.size()})) {
        // Error message is already reported by deserialize if there's a
        // failure.
        return false;
      }
    } else {
      reportError(
          "Cannot deserialize an intermediate CL binary without a compiler.");
      return false;
    }
  }

  return true;
}

void cl::device_program::clear() {
  switch (type) {
    case cl::device_program_type::NONE:
      break;
    case cl::device_program_type::BINARY:
      binary.~Binary();
      break;
    case cl::device_program_type::BUILTIN:
      builtin.~Builtin();
      break;
    case cl::device_program_type::COMPILER_MODULE:
      compiler_module.~CompilerModule();
      break;
  }
  type = cl::device_program_type::NONE;
  num_errors = 0;
  compiler_log = "";
}

void cl::device_program::CompilerModule::clear() {
  module->clear();
  cached_binary = cargo::optional<cargo::dynamic_array<uint8_t>>();
  cached_mux_binary = cargo::optional<cargo::dynamic_array<uint8_t>>();
}

cargo::expected<cargo::array_view<const uint8_t>, compiler::Result>
cl::device_program::CompilerModule::getOrCreateMuxBinary() {
  if (cached_mux_binary.has_value()) {
    return {*cached_mux_binary};
  }

  cargo::array_view<uint8_t> executable;
  const compiler::Result result = module->createBinary(executable);
  if (compiler::Result::SUCCESS != result) {
    return cargo::make_unexpected(result);
  }
  auto &executable_dst = cached_mux_binary.emplace();
  if (cargo::success != executable_dst.alloc(executable.size())) {
    return cargo::make_unexpected(compiler::Result::OUT_OF_MEMORY);
  }
  std::memcpy(executable_dst.data(), executable.data(), executable.size());
  return {*cached_mux_binary};
}

cl_program_binary_type cl::device_program::getCLProgramBinaryType() const {
  switch (type) {
    case cl::device_program_type::BINARY:
      return CL_PROGRAM_BINARY_TYPE_EXECUTABLE;
    case cl::device_program_type::COMPILER_MODULE:
      return convertModuleStateToCL(compiler_module.module->getState());
    default:
      return CL_PROGRAM_BINARY_TYPE_NONE;
  }
}

#if defined(CL_VERSION_3_0)
cl_int _cl_program::SPIRV::setSpecConstant(cl_uint spec_id, size_t spec_size,
                                           const void *spec_value) {
  if (specializable.count(spec_id) == 0) {
    return CL_INVALID_SPEC_ID;
  }
  auto &specDesc = specializable[spec_id];
  if (specDesc.constant_type == compiler::spirv::SpecializationType::BOOL) {
    if (spec_size != sizeof(cl_uchar)) {
      return CL_INVALID_VALUE;
    }
  } else if (spec_size * 8 != specDesc.size_in_bits) {
    // Assume a byte is 8 bits in line with the cl_platform.h header.
    return CL_INVALID_VALUE;
  }
  auto data = static_cast<const uint8_t *>(spec_value);
  const uint32_t offset = specData.size();
  if (!specData.insert(specData.end(), data, data + spec_size)) {
    return CL_OUT_OF_HOST_MEMORY;
  }
  specInfo.entries[spec_id] = {offset, spec_size};
  return CL_SUCCESS;
}
#endif

cargo::optional<const compiler::spirv::SpecializationInfo &>
_cl_program::SPIRV::getSpecInfo() {
#if defined(CL_VERSION_3_0)
  if (specInfo.entries.size() == 0) {
    return cargo::nullopt;
  }
  specInfo.data = specData.data();
  return specInfo;
#else
  return cargo::nullopt;
#endif
}

_cl_program::_cl_program(cl_context context)
    : base<_cl_program>(cl::ref_count_type::EXTERNAL),
      context(context),
      num_external_kernels(0),
      type(cl::program_type::NONE) {
  cl::retainInternal(context);
}

_cl_program::~_cl_program() {
  // This guard needs a variable name, otherwise it is destroyed just after
  // being created. Resource acquisition is required here, as clearing the
  // binaries changes the context, while the context may be accessed at the
  // same time by other _cl_program destructors and Compile and Link calls.
  // The scoping ensures that the resource (context) is released before its
  // possible destruction by the ReleaseInternal call.
  {
    std::unique_lock<std::mutex> guard(context->mutex, std::defer_lock);
    std::unique_lock<compiler::Context> context_guard;
    if (context->getCompilerContext()) {
      context_guard = std::unique_lock<compiler::Context>{
          *context->getCompilerContext(), std::defer_lock};

      // We need to use std::lock here to avoid a deadlock scenario when using
      // two mutexes.
      std::lock(guard, context_guard);
    } else {
      guard.lock();
    }

    // Clear the programs first because they use our cl_context
    programs.clear();
  }

  // Explicitly destruct union members where appropriate.
  switch (type) {
    case cl::program_type::OPENCLC:
      openclc.~OpenCLC();
      break;
    case cl::program_type::BUILTIN:
      builtInKernel.~BuiltInKernel();
      break;
    case cl::program_type::SPIRV:
      spirv.~SPIRV();
      break;
    default:
      break;
  }
  cl::releaseInternal(context);
}

// Used by clCreateProgramWithSource.
cargo::expected<std::unique_ptr<_cl_program>, cl_int> _cl_program::create(
    cl_context context, cl_uint count, const char *const *strings,
    const size_t *lengths) {
  std::unique_ptr<_cl_program> program{new _cl_program{context}};
  if (!program) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }
  // As openclc is defined in a union so must be explicitly constructed.
  new (&program->openclc) OpenCLC();
  // We also must set the program type to take the appropriate destructor path.
  program->type = cl::program_type::OPENCLC;
  for (cl_uint i = 0; i < count; i++) {
    if (lengths && lengths[i] > 0) {
      program->openclc.source.append(strings[i], lengths[i]);
    } else {
      program->openclc.source.append(strings[i]);
    }
  }
  // Create compiler modules.
  for (auto device : context->devices) {
    if (device->compiler_available) {
      program->programs[device].initializeAsCompilerModule(
          context->getCompilerTarget(device));
    }
  }
  return program;
}

// Used by clCreateProgramWithIL, clCreateProgramWithILKHR.
cargo::expected<std::unique_ptr<_cl_program>, cl_int> _cl_program::create(
    cl_context context, const void *il, size_t length) {
  std::unique_ptr<_cl_program> program{new _cl_program{context}};
  if (!program) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }
  // As spirv is defined in a union so must be explicitly constructed.
  new (&program->spirv) SPIRV();
  // We also must set the program type to take the appropriate destructor path.
  program->type = cl::program_type::SPIRV;
  // Convert length in bytes to length in words.
  length /= sizeof(uint32_t);
  if (cargo::success != program->spirv.code.alloc(length)) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }
  std::copy_n(static_cast<const uint32_t *>(il), length,
              program->spirv.code.begin());
#if defined(CL_VERSION_3_0)
  auto specializable = context->getCompilerContext()->getSpecializableConstants(
      {program->spirv.code.data(), program->spirv.code.size()});
  if (!specializable) {
    return cargo::make_unexpected(CL_INVALID_VALUE);
  }
  program->spirv.specializable = std::move(*specializable);
#endif
  for (auto device : context->devices) {
    if (device->compiler_available) {
      program->programs[device].initializeAsCompilerModule(
          context->getCompilerTarget(device));
    }
  }
  return program;
}

// Used by clCreateProgramWithBinary.
cargo::expected<std::unique_ptr<_cl_program>, cl_int> _cl_program::create(
    cl_context context, cl_uint num_devices, const cl_device_id *device_list,
    const size_t *lengths, const unsigned char *const *binaries,
    cl_int *binary_status) {
  std::unique_ptr<_cl_program> program{new _cl_program{context}};
  if (!program) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }
  cl_int error = CL_SUCCESS;
  for (cl_uint i = 0; i < num_devices; ++i) {
    auto setStatus = [&](cl_int error_code) {
      error = error_code;
      if (binary_status) {
        binary_status[i] = error_code;
      }
    };

    if (!lengths[i] || !binaries[i]) {
      setStatus(!lengths[i] ? CL_INVALID_VALUE : CL_INVALID_BINARY);
      program->programs[device_list[i]].reportError(
          !lengths[i] ? "Missing binary length." : "Missing binary.");
      continue;
    }
    {
      const std::lock_guard<std::mutex> guard(context->mutex);

      _cl_device_id *device = device_list[i];
      cl::device_program &device_program = program->programs[device];

      cargo::array_view<const uint8_t> buffer(
          reinterpret_cast<const uint8_t *>(binaries[i]), lengths[i]);

      compiler::Target *compiler_target =
          context->getCompilerTarget(device_list[i]);

      // Check if we have a SPIR 1.2 binary.
      uint8_t spir_magic[4] = {'B', 'C', 0xc0, 0xde};
      if (buffer.size() >= 4 && memcmp(buffer.data(), spir_magic, 4) == 0) {
        setStatus(CL_INVALID_BINARY);
        device_program.reportError("SPIR 1.2 binaries not supported.");
        continue;
      }

      // If not, we deserialize the binary.
      if (!device_program.binaryDeserialize(device, compiler_target, buffer)) {
        // Error message is already reported by binaryDeserialize if there's a
        // failure.
        setStatus(CL_INVALID_BINARY);
        continue;
      }
    }
    setStatus(CL_SUCCESS);
  }
  if (error) {
    return cargo::make_unexpected(error);
  }
  program->type = cl::program_type::BINARY;
  return program;
}

// Used by clCreateProgramWithBuiltInKernels.
cargo::expected<std::unique_ptr<_cl_program>, cl_int> _cl_program::create(
    cl_context context, cl_uint num_devices, const cl_device_id *device_list,
    const char *kernel_names) {
  std::unique_ptr<_cl_program> program(new _cl_program(context));
  if (!program) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }
  // As builtInKernel is defined in a union so must be explicitly constructed.
  new (&program->builtInKernel) BuiltInKernel();
  // We also must set the program type to take the appropriate destructor path.
  program->type = cl::program_type::BUILTIN;
  auto req_kernels_v = cargo::split_all(kernel_names, ";");
  // Remove any padding whitespace
  for (auto &req_kernel : req_kernels_v) {
    req_kernel = cargo::trim(req_kernel, " ");
  }
  // We need the kernel names in an unordered set so that we can quickly remove
  // them in a random order.
  std::unordered_set<cargo::string_view> requested_kernels(
      std::make_move_iterator(req_kernels_v.begin()),
      std::make_move_iterator(req_kernels_v.end()));
  req_kernels_v.clear();
  // If kernel_names is blank or just two blank names this is invalid.
  if (requested_kernels.empty()) {
    return cargo::make_unexpected(CL_INVALID_VALUE);
  }

  // The OpenCL spec says the following:
  // "`CL_INVALID_VALUE` if `kernel_names` is NULL or `kernel_names` contains a
  // kernel name that is not supported by any of the devices in `device_list`."
  // It's a little bit ambiguous, but we have taken the interpretation that if
  // the kernel is supported by none of the devices, then we will return
  // CL_INVALID_VALUE. Each of the requested kernels must be supported by at
  // least one device (though not necessarily the same device) for the call to
  // succeed.
  for (cl_uint i = 0; i < num_devices; ++i) {
    auto available_kernel_decls = cargo::split_all(
        device_list[i]->mux_device->info->builtin_kernel_declarations, ";");
    auto &device_program = program->programs[device_list[i]];

    device_program.initializeAsBuiltin();
    for (auto requested_kernel = requested_kernels.begin();
         requested_kernel != requested_kernels.end();) {
      // If the kernel names is empty then return CL_INVALID_VALUE
      if (requested_kernel->empty()) {
        return cargo::make_unexpected(CL_INVALID_VALUE);
      }
      bool found = false;

      // Find if the requested kernel is available on this device.
      for (const auto &avail_kern_decl : available_kernel_decls) {
        // Need to make sure that avail_kern_decl's name is exactly
        // requested_kernel. Otherwise, the user might request `foo` and get
        // `foobar`. Also, need to make sure spaces are ignored.
        auto avail_decl_trim = cargo::trim(avail_kern_decl, " ");
        if (avail_decl_trim.starts_with(*requested_kernel) &&
            ('(' == *(avail_decl_trim.cbegin() + requested_kernel->size()) ||
             ' ' == *(avail_decl_trim.cbegin() + requested_kernel->size()))) {
          // Add the kernel name and it's declaration string to the binary
          if (device_program.builtin.kernel_names.push_back(
                  cargo::as<std::string>(*requested_kernel))) {
            return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
          }
          if (device_program.builtin.kernel_decls.push_back(
                  cargo::as<std::string>(avail_kern_decl))) {
            return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
          }

          found = true;
          break;
        }
      }

      // If the requested kernel was found, we can remove it from the set of
      // requested kernels. Otherwise we just iterate past it.
      if (found) {
        requested_kernel = requested_kernels.erase(requested_kernel);
      } else {
        ++requested_kernel;
      }
    }
  }

  // Any kernels left in the set weren't found on any device.
  if (!requested_kernels.empty()) {
    return cargo::make_unexpected(CL_INVALID_VALUE);
  }

  program->builtInKernel.names = kernel_names;
  if (!program->finalize({device_list, num_devices})) {
    return cargo::make_unexpected(CL_INVALID_VALUE);
  }
  return program;
}

// Used by clLinkProgram.
cargo::expected<std::unique_ptr<_cl_program>, cl_int> _cl_program::create(
    cl_context context, cargo::array_view<const cl_device_id> devices,
    cargo::string_view options,
    cargo::array_view<const cl_program> input_programs) {
  std::unique_ptr<_cl_program> program(new _cl_program(context));
  if (!program) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }
  // Initialise device programs as compiler modules.
  for (auto device : context->devices) {
    program->programs[device].initializeAsCompilerModule(
        context->getCompilerTarget(device));
  }
  // We are creating a linked program, so change the type.
  program->type = cl::program_type::LINK;
  if (auto error = program->setOptions(devices, options,
                                       compiler::Options::Mode::LINK)) {
    return cargo::make_unexpected(error);
  }
  if (auto error = program->link(devices, input_programs)) {
    return cargo::make_unexpected(error);
  }
  if (!program->finalize(devices)) {
    return cargo::make_unexpected(CL_LINK_PROGRAM_FAILURE);
  }
  return program;
}

cl_int _cl_program::compile(
    cargo::array_view<const cl_device_id> devices,
    cargo::array_view<compiler::InputHeader> input_headers) {
  for (auto device : devices) {
    auto &device_program = programs[device];

    // We should have already checked in `clBuildProgram` or `clCompileProgram`
    // that the program should contain a valid compiler module.
    OCL_ASSERT(device_program.type == cl::device_program_type::COMPILER_MODULE,
               "Attempting to compile a binary.");
    auto &module = *device_program.compiler_module.module;

    compiler::Result error = compiler::Result::SUCCESS;
    switch (type) {
#if defined(OCL_EXTENSION_cl_khr_il_program) || defined(CL_VERSION_3_0)
      case cl::program_type::SPIRV: {
        auto spirv_device_info =
            context->getSPIRVDeviceInfo(device->mux_device->info);
        if (!spirv_device_info) {
          return CL_COMPILE_PROGRAM_FAILURE;
        }
        // We need to clear the device program as we're starting a new
        // compilation.
        device_program.compiler_module.clear();
        auto result = module.compileSPIRV(spirv.code, *spirv_device_info,
                                          spirv.getSpecInfo());
        if (!result) {
          error = result.error();
        }
      } break;
#endif
      case cl::program_type::OPENCLC:
        // We need to clear the device program as we're starting a new
        // compilation.
        device_program.compiler_module.clear();
        error = module.compileOpenCLC(device->profile, openclc.source,
                                      input_headers);
        break;
      default:
        return CL_INVALID_OPERATION;
    }
    if (error != compiler::Result::SUCCESS) {
      return cl::getErrorFrom(error);
    }
  }
  return CL_SUCCESS;
}

cl_int _cl_program::link(cargo::array_view<const cl_device_id> devices,
                         cargo::array_view<const cl_program> input_programs) {
  for (auto device : devices) {
    // This guard needs a variable name, otherwise it is destroyed just after
    // being created.
    const std::lock_guard<std::mutex> guard(context->mutex);
    cargo::small_vector<compiler::Module *, 8> input_modules;
    for (auto input_program : input_programs) {
      // Note: cl::LinkProgram already checks that the input program is a
      // compiler module.
      if (input_modules.push_back(
              input_program->programs[device].compiler_module.module.get()) !=
          cargo::success) {
        return CL_OUT_OF_HOST_MEMORY;
      }
    }
    // Note: _cl_program::create already initializes this device program as a
    // compiler module.
    auto error = programs[device].compiler_module.module->link(input_modules);
    if (error != compiler::Result::SUCCESS) {
      return cl::getErrorFrom(error);
    }
  }
  return CL_SUCCESS;
}

bool _cl_program::finalize(cargo::array_view<const cl_device_id> devices) {
  const std::lock_guard<std::mutex> lock(context->mutex);
  for (auto device : devices) {
    if (hasOption(device, "-create-library")) {
      continue;  // don't finalize a library, it's only used for linking.
    }
    if (!programs[device].finalize(device)) {
      return false;
    }
  }
  return true;
}

cargo::optional<const compiler::KernelInfo *> _cl_program::getKernelInfo(
    cargo::string_view name) const {
  for (auto device : context->devices) {
    auto &device_program = programs.at(device);

    // If we have loaded a binary, built-in kernel, or we have a fully compiled
    // module, then we should have a program info to read from.
    if (device_program.type == cl::device_program_type::BINARY ||
        device_program.type == cl::device_program_type::BUILTIN ||
        (device_program.type == cl::device_program_type::COMPILER_MODULE &&
         device_program.compiler_module.module->getState() ==
             compiler::ModuleState::EXECUTABLE)) {
      OCL_ASSERT(device_program.program_info, "Program info was null!");
      auto &program_info = *device_program.program_info;

      auto isRequestedKernel = [&](const compiler::KernelInfo &kernel_info) {
        if (device_program.type == cl::device_program_type::BUILTIN) {
          return name.find(kernel_info.name) != std::string::npos;
        }
        return kernel_info.name == name;
      };

      auto found = std::find_if(program_info.begin(), program_info.end(),
                                isRequestedKernel);
      if (found != program_info.end()) {
        return found;
      }
    }
  }

  return cargo::nullopt;
}

size_t _cl_program::getNumKernels() const {
  size_t numKernels = 0;

  for (auto device : context->devices) {
    auto &device_program = programs.at(device);

    OCL_ASSERT(device_program.isExecutable(),
               "OpenCL _cl_program. Error: not all kernels have been built "
               "into executables");

    const size_t deviceNumKernels =
        device_program.program_info->getNumKernels();

    OCL_ASSERT((0 == numKernels) || (deviceNumKernels == numKernels),
               "Devices that program is built for do not agree on the number "
               "of kernels in the binary");

    numKernels = deviceNumKernels;
  }

  OCL_ASSERT(0 != numKernels,
             "OpenCL _cl_program. Error zero kernels in the program");

  return numKernels;
}

const char *_cl_program::getKernelNameByOffset(
    const size_t kernel_index) const {
  const char *result = nullptr;

  for (auto device : context->devices) {
    auto &device_program = programs.at(device);

    OCL_ASSERT(device_program.isExecutable(),
               "OpenCL _cl_program. Error: not all kernels have been built "
               "into executables");

    const compiler::ProgramInfo &program_info =
        device_program.program_info.value();

    OCL_ASSERT(kernel_index < program_info.getNumKernels(),
               "OpenCL _cl_program. Error kernel index greater than the total "
               "number of kernels in the program");

    auto kernelInfo = program_info.getKernel(kernel_index);

    if (!kernelInfo) {
      return nullptr;
    }

    const char *kernelName = kernelInfo->name.c_str();

    if (nullptr == result) {
      result = kernelName;
    } else if (0 != strcmp(result, kernelName)) {
      return nullptr;
    }
  }

  OCL_ASSERT(nullptr != result,
             "Could not find a kernel within a program using an index");

  return result;
}

bool _cl_program::hasDevice(const cl_device_id device_id) const {
  return context->hasDevice(device_id);
}

bool _cl_program::hasOption(cl_device_id device, const char *option) {
  return programs[device].options.find(option) != std::string::npos;
}

cl_int _cl_program::setOptions(cargo::array_view<const cl_device_id> devices,
                               cargo::string_view options_string,
                               const compiler::Options::Mode mode) {
  // Make sure we aren't doing string operations on nullptr.
  if (options_string.empty()) {
    options_string = "";
  }

  for (auto device : devices) {
    compiler::Options compiler_options;

    // Query for enabled extensions in order to add the required macro
    // definitions to the compile options.
    cargo::string_view extensions_view;
    cl_int err =
        extension::GetRuntimeExtensionsForDevice(device, extensions_view);
    if (CL_SUCCESS != err) {
      return err;
    }
    for (auto &ext_name : cargo::split(extensions_view.data(), " ")) {
      compiler_options.runtime_extensions.emplace_back(ext_name.data(),
                                                       ext_name.size());
    }

    err = extension::GetCompilerExtensionsForDevice(device, extensions_view);
    if (CL_SUCCESS != err) {
      return err;
    }
    for (auto &ext_name : cargo::split(extensions_view.data(), " ")) {
      compiler_options.compiler_extensions.emplace_back(ext_name.data(),
                                                        ext_name.size());
    }

    // Ensure we are initialized to the compiler state.
    if (programs[device].type != cl::device_program_type::COMPILER_MODULE) {
      programs[device].initializeAsCompilerModule(
          context->getCompilerTarget(device));
    }
    programs[device].compiler_module.module->getOptions() = compiler_options;

    // Append options set by environment variables, these are not stored in
    // options[device] so they don't pollute the value returned by
    // clGetProgramBuildInfo with CL_PROGRAM_BUILD_OPTIONS.
    auto options_to_parse = [](const compiler::Options::Mode mode,
                               cargo::string_view options) {
      auto join = [](std::initializer_list<cargo::string_view> options) {
        return cargo::as<std::string>(
            cargo::trim(cargo::join(options.begin(), options.end(), " ")));
      };
      const char *extraCompileOpts = "CA_EXTRA_COMPILE_OPTS";
      const char *extraLinkOpts = "CA_EXTRA_LINK_OPTS";
      switch (mode) {
        case compiler::Options::Mode::BUILD:
          return join({options, std::getenv(extraCompileOpts),
                       std::getenv(extraLinkOpts)});
        case compiler::Options::Mode::COMPILE:
          return join({options, std::getenv(extraCompileOpts)});
        case compiler::Options::Mode::LINK:
          return join({options, std::getenv(extraLinkOpts)});
      }
      return std::string();
    }(mode, options_string);

    // Check if we have already parsed these options.
    if (programs[device].options == options_string &&
        options_to_parse.empty()) {
      return CL_SUCCESS;
    }

    // Store the options for this device.
    programs[device].options = cargo::as<std::string>(options_string);

    auto error = programs[device].compiler_module.module->parseOptions(
        options_to_parse, mode);
    if (error != compiler::Result::SUCCESS) {
      programs[device].num_errors++;
      return cl::getErrorFrom(error);
    }
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_program CL_API_CALL cl::CreateProgramWithSource(
    cl_context context, cl_uint count, const char *const *strings,
    const size_t *lengths, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCreateProgramWithSource");
  OCL_CHECK(!context, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return nullptr);
  OCL_CHECK(0 == count, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK(!strings, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  for (cl_uint i = 0; i < count; i++) {
    OCL_CHECK(!strings[i], OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
              return nullptr);
  }

  auto program = _cl_program::create(context, count, strings, lengths);
  if (!program) {
    OCL_SET_IF_NOT_NULL(errcode_ret, program.error());
    return nullptr;
  }
  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return program->release();
}

CL_API_ENTRY cl_program CL_API_CALL cl::CreateProgramWithBinary(
    cl_context context, cl_uint num_devices, const cl_device_id *device_list,
    const size_t *lengths, const unsigned char *const *binaries,
    cl_int *binary_status, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCreateProgramWithBinary");
  OCL_CHECK(!context, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return nullptr);
  OCL_CHECK(!device_list, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK(num_devices == 0,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK(!lengths, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK(!binaries, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  for (cl_uint i = 0; i < num_devices; ++i) {
    OCL_CHECK(!context->hasDevice(device_list[i]),
              OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_DEVICE);
              return nullptr);
  }

  auto program = _cl_program::create(context, num_devices, device_list, lengths,
                                     binaries, binary_status);
  if (!program) {
    OCL_SET_IF_NOT_NULL(errcode_ret, program.error());
    return nullptr;
  }
  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return program->release();
}

CL_API_ENTRY cl_program CL_API_CALL cl::CreateProgramWithBuiltInKernels(
    cl_context context, cl_uint num_devices, const cl_device_id *device_list,
    const char *kernel_names, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard(
      "clCreateProgramWithBuiltInKernels");
  OCL_CHECK(!context, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return nullptr);
  OCL_CHECK(!device_list || num_devices == 0,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);

  for (cl_uint i = 0; i < num_devices; ++i) {
    bool in_context = false;
    for (auto ctx_device : context->devices) {
      OCL_CHECK(device_list[i] == ctx_device, in_context = true);
    }
    OCL_CHECK(!in_context, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_DEVICE);
              return nullptr);
  }
  OCL_CHECK(!kernel_names, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);

  auto program =
      _cl_program::create(context, num_devices, device_list, kernel_names);
  if (!program) {
    OCL_SET_IF_NOT_NULL(errcode_ret, program.error());
    return nullptr;
  }
  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return program->release();
}

CL_API_ENTRY cl_int CL_API_CALL cl::RetainProgram(cl_program program) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clRetainProgram");
  OCL_CHECK(!program, return CL_INVALID_PROGRAM);

  return cl::retainExternal(program);
}

CL_API_ENTRY cl_int CL_API_CALL cl::ReleaseProgram(cl_program program) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clReleaseProgram");
  OCL_CHECK(!program, return CL_INVALID_PROGRAM);

  return cl::releaseExternal(program);
}

CL_API_ENTRY cl_int CL_API_CALL cl::CompileProgram(
    cl_program program, cl_uint num_devices, const cl_device_id *device_list,
    const char *options, cl_uint num_input_headers,
    const cl_program *input_headers, const char *const *header_include_names,
    void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
    void *user_data) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCompileProgram");
  OCL_CHECK(!pfn_notify && user_data, return CL_INVALID_VALUE);
  const _cl_program::callback callback(program, pfn_notify, user_data);

  OCL_CHECK(!program, return CL_INVALID_PROGRAM);
  OCL_CHECK(program->num_external_kernels > 0, return CL_INVALID_OPERATION);
  OCL_CHECK(!device_list && (0 < num_devices), return CL_INVALID_VALUE);
  OCL_CHECK(device_list && (0 == num_devices), return CL_INVALID_VALUE);

  cargo::array_view<const cl_device_id> devices;
  if (device_list) {
    devices = {device_list, num_devices};
    for (auto device : devices) {
      OCL_CHECK(!program->hasDevice(device), return CL_INVALID_DEVICE);
    }
  } else {
    devices = program->context->devices;
  }
  for (auto device : devices) {
    OCL_CHECK(!device->compiler_available, return CL_COMPILER_NOT_AVAILABLE);
    OCL_CHECK(program->programs[device].type !=
                  cl::device_program_type::COMPILER_MODULE,
              return CL_INVALID_OPERATION);
  }

  OCL_CHECK((0 == num_input_headers) && (header_include_names || input_headers),
            return CL_INVALID_VALUE);
  OCL_CHECK(
      (0 != num_input_headers) && !(header_include_names && input_headers),
      return CL_INVALID_VALUE);

  cargo::small_vector<compiler::InputHeader, 8> inputHeaders;
  for (uint32_t i = 0; i < num_input_headers; i++) {
    // Note that this behavior is not mandated by the OpenCL 1.2 specification,
    // but if we don't check for this we segfault when given an invalid header.
    // The specification doesn't say what to do in this situation, and returning
    // CL_INVALID_PROGRAM is preferable to segfaulting.
    OCL_CHECK(!input_headers[i], return CL_INVALID_PROGRAM);
    // Extract the source from the input header program.
    compiler::InputHeader inputHeader;
    // Check the input header's type to ensure the openclc union member is a
    // valid object, not uninitialized memory, before accessing it.
    if (input_headers[i]->type == cl::program_type::OPENCLC) {
      inputHeader.source = input_headers[i]->openclc.source;
    }
    // And set the include name for the headers.
    inputHeader.name = header_include_names[i];
    if (inputHeaders.push_back(std::move(inputHeader))) {
      return CL_OUT_OF_HOST_MEMORY;
    }
  }

  if (auto error = program->setOptions(devices, options,
                                       compiler::Options::Mode::COMPILE)) {
    return error;
  }
  if (auto error = program->compile(devices, inputHeaders)) {
    return error;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_program CL_API_CALL cl::LinkProgram(
    cl_context context, cl_uint num_devices, const cl_device_id *device_list,
    const char *options, cl_uint num_input_programs,
    const cl_program *input_programs,
    void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
    void *user_data, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clLinkProgram");
  OCL_CHECK(!pfn_notify && user_data,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  _cl_program::callback callback(nullptr, pfn_notify, user_data);
  OCL_CHECK(!context, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return nullptr);
  OCL_CHECK(!device_list && (0 < num_devices),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK(device_list && (0 == num_devices),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK((0 == num_input_programs),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK((0 != num_input_programs) && !input_programs,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);

  if (device_list) {
    for (cl_uint k = 0; k < num_devices; k++) {
      OCL_CHECK(!context->hasDevice(device_list[k]),
                OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_DEVICE);
                return nullptr);
    }
  } else {
    device_list = context->devices.data();
    num_devices = context->devices.size();
  }

  for (cl_uint k = 0; k < num_devices; ++k) {
    OCL_CHECK(!device_list[k]->linker_available,
              OCL_SET_IF_NOT_NULL(errcode_ret, CL_LINKER_NOT_AVAILABLE);
              return nullptr);
  }

  for (cl_uint i = 0; i < num_input_programs; i++) {
    OCL_CHECK(!input_programs[i],
              OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_PROGRAM);
              return nullptr);

    for (cl_uint k = 0; k < num_devices; k++) {
      const auto &device_program = input_programs[i]->programs[device_list[k]];
      if (device_program.type != cl::device_program_type::COMPILER_MODULE) {
        OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_OPERATION);
        return nullptr;
      }
      switch (device_program.compiler_module.module->getState()) {
        case compiler::ModuleState::COMPILED_OBJECT:  // Fall through.
        case compiler::ModuleState::LIBRARY:
          break;
        default:
          OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_OPERATION);
          return nullptr;
      }
    }
  }

  auto program =
      _cl_program::create(context, {device_list, num_devices}, options,
                          {input_programs, num_input_programs});
  if (!program) {
    OCL_SET_IF_NOT_NULL(errcode_ret, program.error());
    return nullptr;
  }
  // The program must be set in the RAII callback only when we know that the
  // unique_ptr will be released.
  callback.program = program->get();
  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return program->release();
}

CL_API_ENTRY cl_int CL_API_CALL cl::BuildProgram(
    cl_program program, cl_uint num_devices, const cl_device_id *device_list,
    const char *options,
    void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
    void *user_data) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clBuildProgram");
  OCL_CHECK(!program, return CL_INVALID_PROGRAM);
  OCL_CHECK(!pfn_notify && user_data, return CL_INVALID_VALUE);
  const _cl_program::callback callback(program, pfn_notify, user_data);

  OCL_CHECK(program->num_external_kernels > 0, return CL_INVALID_OPERATION);
  OCL_CHECK(device_list && num_devices == 0, return CL_INVALID_VALUE);
  OCL_CHECK(!device_list && num_devices > 0, return CL_INVALID_VALUE);
  // A builtin program is not required to be built so return
  // `CL_INVALID_OPERATION`
  OCL_CHECK(program->type == cl::program_type::BUILTIN,
            return CL_INVALID_OPERATION);

  cargo::array_view<const cl_device_id> devices;
  if (device_list) {
    devices = {device_list, num_devices};
  } else {
    devices = program->context->devices;
  }

  for (auto device : devices) {
    OCL_CHECK(!program->hasDevice(device), return CL_INVALID_DEVICE);
    OCL_CHECK((program->type == cl::program_type::OPENCLC ||
               program->type == cl::program_type::SPIRV) &&
                  !device->compiler_available,
              return CL_COMPILER_NOT_AVAILABLE);
    OCL_CHECK(program->programs[device].type == cl::device_program_type::NONE &&
                  program->type == cl::program_type::BINARY,
              return CL_INVALID_BINARY);
  }

  // Programs created from binaries don't need to be compiled or finalized but
  // are allowed to be passed to clBuildProgram().
  if (program->type != cl::program_type::BINARY &&
      program->type != cl::program_type::BUILTIN) {
    if (auto error = program->setOptions(devices, options,
                                         compiler::Options::Mode::BUILD)) {
      return error;
    }
    if (auto error = program->compile(devices, {})) {
      return error == CL_COMPILE_PROGRAM_FAILURE ? CL_BUILD_PROGRAM_FAILURE
                                                 : error;
    }
    if (!program->finalize(devices)) {
      return CL_BUILD_PROGRAM_FAILURE;
    }
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::GetProgramInfo(
    cl_program program, cl_program_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetProgramInfo");
  OCL_CHECK(!program, return CL_INVALID_PROGRAM);
  OCL_CHECK(!param_value && !param_value_size_ret, return CL_INVALID_VALUE);

#define PROGRAM_INFO_CASE(ENUM, VALUE)                                    \
  case ENUM: {                                                            \
    const size_t typeSize = sizeof(VALUE);                                \
    OCL_CHECK(param_value && (param_value_size < typeSize),               \
              return CL_INVALID_VALUE);                                   \
    if (param_value) {                                                    \
      *static_cast<std::remove_const_t<decltype(VALUE)> *>(param_value) = \
          VALUE;                                                          \
    }                                                                     \
    OCL_SET_IF_NOT_NULL(param_value_size_ret, typeSize);                  \
  } break

#define PROGRAM_INFO_CASE_WITH_TYPE(ENUM, VALUE, TYPE)      \
  case ENUM: {                                              \
    const size_t typeSize = sizeof(TYPE);                   \
    OCL_CHECK(param_value && (param_value_size < typeSize), \
              return CL_INVALID_VALUE);                     \
    if (param_value) {                                      \
      *static_cast<TYPE *>(param_value) = VALUE;            \
    }                                                       \
    OCL_SET_IF_NOT_NULL(param_value_size_ret, typeSize);    \
  } break

  switch (param_name) {
#if defined(CL_VERSION_3_0)
    PROGRAM_INFO_CASE(CL_PROGRAM_SCOPE_GLOBAL_CTORS_PRESENT,
                      program->scope_global_ctors_present);
    PROGRAM_INFO_CASE(CL_PROGRAM_SCOPE_GLOBAL_DTORS_PRESENT,
                      program->scope_global_dtors_present);
#endif
    PROGRAM_INFO_CASE(CL_PROGRAM_REFERENCE_COUNT, program->refCountExternal());
    PROGRAM_INFO_CASE(CL_PROGRAM_CONTEXT, program->context);
    PROGRAM_INFO_CASE_WITH_TYPE(CL_PROGRAM_NUM_DEVICES,
                                program->context->devices.size(), cl_uint);
    case CL_PROGRAM_DEVICES: {
      const cl_uint numDevices = program->context->devices.size();
      const cl_uint totalSize = sizeof(cl_device_id) * numDevices;
      OCL_SET_IF_NOT_NULL(param_value_size_ret, totalSize);
      OCL_CHECK(param_value && param_value_size < totalSize,
                return CL_INVALID_VALUE);
      if (param_value) {
        cl_device_id *reinterpreted =
            reinterpret_cast<cl_device_id *>(param_value);

        std::copy(program->context->devices.begin(),
                  program->context->devices.end(), reinterpreted);
      }
    } break;
    case CL_PROGRAM_SOURCE:
      if (program->type == cl::program_type::OPENCLC &&
          program->openclc.source.size() > 0) {
        const size_t length = program->openclc.source.size() + 1;
        OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(char) * length);
        OCL_CHECK(param_value && param_value_size < sizeof(char) * length,
                  return CL_INVALID_VALUE);
        if (param_value) {
          strncpy(reinterpret_cast<char *>(param_value),
                  program->openclc.source.data(), length);
        }
      } else {
        OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(char));
        OCL_CHECK(param_value && (0 == param_value_size),
                  return CL_INVALID_VALUE);
        OCL_SET_IF_NOT_NULL(reinterpret_cast<char *>(param_value), '\0');
      }
      break;
    case CL_PROGRAM_BINARY_SIZES: {
      const size_t size = sizeof(size_t) * program->context->devices.size();
      OCL_SET_IF_NOT_NULL(param_value_size_ret, size);
      OCL_CHECK(param_value && (param_value_size < size),
                return CL_INVALID_VALUE);
      if (param_value) {
        for (cl_uint i = 0; i < program->context->devices.size(); i++) {
          cl_device_id device = program->context->devices[i];
          reinterpret_cast<size_t *>(param_value)[i] =
              program->programs[device].binarySize();
        }
      }
    } break;
    case CL_PROGRAM_BINARIES: {
      const size_t size = sizeof(char *) * program->context->devices.size();
      OCL_SET_IF_NOT_NULL(param_value_size_ret, size);
      OCL_CHECK(param_value && (param_value_size < size),
                return CL_INVALID_VALUE);
      if (param_value) {
        for (cl_uint i = 0; i < program->context->devices.size(); i++) {
          cl_device_id device = program->context->devices[i];
          auto &device_program = program->programs[device];
          if (device_program.type == cl::device_program_type::BINARY ||
              device_program.type == cl::device_program_type::COMPILER_MODULE) {
            auto buffer = device_program.binarySerialize();
            OCL_CHECK(0 < device_program.num_errors, return CL_INVALID_PROGRAM);
            if (!buffer.empty()) {
              std::uninitialized_copy_n(
                  buffer.data(), buffer.size(),
                  reinterpret_cast<unsigned char **>(param_value)[i]);
            }
          }
          // Do nothing for cl::device_program_type::BUILTIN as we report
          // CL_PROGRAM_BINARY_SIZES to be 0 bytes per device because there are
          // no binaries to return for built-in kernels.
        }
      }
    } break;
    case CL_PROGRAM_NUM_KERNELS: {
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(size_t));
      OCL_CHECK(param_value && param_value_size < sizeof(size_t),
                return CL_INVALID_VALUE);
      bool success = false;

      for (auto device : program->context->devices) {
        if (program->programs[device].isExecutable()) {
          if (param_value) {
            OCL_SET_IF_NOT_NULL(
                reinterpret_cast<size_t *>(param_value),
                program->programs[device].program_info->getNumKernels());
          }

          success = true;
          break;
        }
      }
      if (!success) {
        return CL_INVALID_PROGRAM_EXECUTABLE;
      }
      break;
    }
    case CL_PROGRAM_KERNEL_NAMES: {
      bool success = false;
      for (auto device : program->context->devices) {
        if (program->programs[device].isExecutable()) {
          success = true;
          std::string name;

          const compiler::ProgramInfo &program_info =
              *program->programs[device].program_info;
          for (size_t k = 0, e = program_info.getNumKernels(); k < e; k++) {
            OCL_ASSERT(nullptr != program_info.getKernel(k),
                       "Fewer than getNumKernels() kernels");
            name += program_info.getKernel(k)->name;
            if ((k + 1) < e) {
              name += ';';
            }
          }

          const size_t length = name.length() + 1;

          OCL_SET_IF_NOT_NULL(param_value_size_ret, length);
          OCL_CHECK(param_value && param_value_size < length,
                    return CL_INVALID_VALUE);
          if (param_value) {
            strncpy(reinterpret_cast<char *>(param_value), name.c_str(),
                    length);
          }
          break;
        }
      }
      if (!success) {
        return CL_INVALID_PROGRAM_EXECUTABLE;
      }
      break;
    }
#if defined(CL_VERSION_3_0)
    case CL_PROGRAM_IL: {
      if (program->type != cl::program_type::SPIRV) {
        OCL_SET_IF_NOT_NULL(param_value_size_ret, 0);
      } else {
        const size_t spirvSizeInBytes =
            program->spirv.code.size() * sizeof(uint32_t);
        OCL_CHECK(param_value && (param_value_size < spirvSizeInBytes),
                  return CL_INVALID_VALUE);
        if (param_value) {
          std::memcpy(param_value, program->spirv.code.data(),
                      spirvSizeInBytes);
        }
        OCL_SET_IF_NOT_NULL(param_value_size_ret, spirvSizeInBytes);
      }
    } break;
#endif
    default: {
      return extension::GetProgramInfo(program, param_name, param_value_size,
                                       param_value, param_value_size_ret);
    }
  }

  return CL_SUCCESS;

#undef PROGRAM_INFO_CASE
}

CL_API_ENTRY cl_int CL_API_CALL cl::GetProgramBuildInfo(
    cl_program program, cl_device_id device_id,
    cl_program_build_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetProgramBuildInfo");
  OCL_CHECK(!program, return CL_INVALID_PROGRAM);
  OCL_CHECK(!device_id, return CL_INVALID_DEVICE);
  OCL_CHECK(!program->context->hasDevice(device_id), return CL_INVALID_DEVICE);
  OCL_CHECK(!param_value && !param_value_size_ret, return CL_INVALID_VALUE);

  switch (param_name) {
    case CL_PROGRAM_BUILD_STATUS:
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(cl_build_status));
      if (param_value) {
        OCL_CHECK(param_value_size < sizeof(cl_build_status),
                  return CL_INVALID_VALUE);

        if (program->programs[device_id].num_errors > 0) {
          *reinterpret_cast<cl_build_status *>(param_value) = CL_BUILD_ERROR;
        } else {
          if (program->programs[device_id].type ==
                  cl::device_program_type::COMPILER_MODULE &&
              program->programs[device_id].compiler_module.module->getState() !=
                  compiler::ModuleState::NONE) {
            *reinterpret_cast<cl_build_status *>(param_value) =
                CL_BUILD_SUCCESS;
          } else if (program->programs[device_id].type ==
                     cl::device_program_type::BINARY) {
            *reinterpret_cast<cl_build_status *>(param_value) =
                CL_BUILD_SUCCESS;
          } else {
            *reinterpret_cast<cl_build_status *>(param_value) = CL_BUILD_NONE;
          }
        }
      }
      break;
    case CL_PROGRAM_BUILD_OPTIONS:
      if (0 == program->programs.size()) {
        OCL_SET_IF_NOT_NULL(param_value_size_ret, 1);
        if (param_value) {
          OCL_CHECK(param_value_size < 1, return CL_INVALID_VALUE);
          *static_cast<char *>(param_value) = '\0';
        }
      } else {
        const auto &build_options = program->programs[device_id].options;
        // need + 1 for the null terminator
        const size_t bytes_required_for_str = build_options.size() + 1;
        OCL_SET_IF_NOT_NULL(param_value_size_ret, bytes_required_for_str);
        if (param_value) {
          OCL_CHECK(param_value_size < bytes_required_for_str,
                    return CL_INVALID_VALUE);
          char *value = static_cast<char *>(param_value);
          std::strncpy(value, build_options.c_str(), param_value_size);
        }
      }
      break;
    case CL_PROGRAM_BUILD_LOG:
      OCL_SET_IF_NOT_NULL(param_value_size_ret,
                          program->programs[device_id].compiler_log.size() + 1);
      OCL_CHECK(param_value &&
                    param_value_size <
                        program->programs[device_id].compiler_log.size() + 1,
                return CL_INVALID_VALUE);
      if (param_value) {
        std::strncpy(reinterpret_cast<char *>(param_value),
                     program->programs[device_id].compiler_log.c_str(),
                     param_value_size);
      }
      break;
    case CL_PROGRAM_BINARY_TYPE:
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(cl_program_binary_type));
      if (param_value) {
        OCL_CHECK(param_value_size < sizeof(cl_program_binary_type),
                  return CL_INVALID_VALUE);
        *reinterpret_cast<cl_program_binary_type *>(param_value) =
            program->programs[device_id].getCLProgramBinaryType();
      }
      break;
#if defined(CL_VERSION_3_0)
    case CL_PROGRAM_BUILD_GLOBAL_VARIABLE_TOTAL_SIZE: {
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(size_t));
      if (param_value) {
        OCL_CHECK(param_value_size < sizeof(size_t), return CL_INVALID_VALUE);
        *static_cast<size_t *>(param_value) =
            program->global_variable_total_size;
      }
    } break;
#endif
    default: {
      return extension::GetProgramBuildInfo(program, device_id, param_name,
                                            param_value_size, param_value,
                                            param_value_size_ret);
    }
  }

  return CL_SUCCESS;
}

// This function is deprecated in OpenCL 1.2 (it does not take a cl_platform_id
// and thus it is not compatible with the ICD), it is replaced by
// clUnloadPlatformCompiler.  This function is just a hint, it is not a
// guarantee from the programmer that the compiler will not be used again.  So
// for now we just ignore the hint.
CL_API_ENTRY cl_int CL_API_CALL cl::UnloadCompiler() {
  const tracer::TraceGuard<tracer::OpenCL> guard("clUnloadCompiler");
  return CL_SUCCESS;
}

// This function is just a hint, it is not a guarantee from the programmer that
// the compiler will not be used again.  So for now we just ignore the hint.
CL_API_ENTRY cl_int CL_API_CALL
cl::UnloadPlatformCompiler(cl_platform_id platform) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clUnloadPlatformCompiler");
  OCL_CHECK(!platform, return CL_INVALID_PLATFORM);
  return CL_SUCCESS;
}
