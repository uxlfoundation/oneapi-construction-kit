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
#include <cargo/string_algorithm.h>
#include <cargo/string_view.h>
#include <cl/binary/binary.h>
#include <cl/config.h>
#include <cl/device.h>
#include <cl/limits.h>
#include <cl/macros.h>
#include <cl/platform.h>
#include <cl/validate.h>
#include <compiler/context.h>
#include <compiler/limits.h>
#include <tracer/tracer.h>
#include <utils/system.h>

#include <algorithm>
#include <climits>
#include <cstring>
#include <type_traits>

namespace {
// Clamp a value into a range specified by [lo..hi]
template <typename T, typename U>
T clamp(const T x, const U lo, const U hi) {
  return std::min(static_cast<T>(hi), std::max(x, static_cast<T>(lo)));
}

cl_device_fp_config setOpenCLFromMux(uint32_t capabilities) {
  cl_device_fp_config config = 0;
  if (cl::validate::IsInBitSet(capabilities,
                               mux_floating_point_capabilities_denorm)) {
    config |= CL_FP_DENORM;
  }
  if (cl::validate::IsInBitSet(capabilities,
                               mux_floating_point_capabilities_inf_nan)) {
    config |= CL_FP_INF_NAN;
  }
  if (cl::validate::IsInBitSet(capabilities,
                               mux_floating_point_capabilities_rte)) {
    config |= CL_FP_ROUND_TO_NEAREST;
  }
  if (cl::validate::IsInBitSet(capabilities,
                               mux_floating_point_capabilities_rtz)) {
    config |= CL_FP_ROUND_TO_ZERO;
  }
  if (cl::validate::IsInBitSet(capabilities,
                               mux_floating_point_capabilities_rtp) &&
      cl::validate::IsInBitSet(capabilities,
                               mux_floating_point_capabilities_rtn)) {
    config |= CL_FP_ROUND_TO_INF;
  }
  if (cl::validate::IsInBitSet(capabilities,
                               mux_floating_point_capabilities_fma)) {
    config |= CL_FP_FMA;
  }
  if (cl::validate::IsInBitSet(capabilities,
                               mux_floating_point_capabilities_soft)) {
    config |= CL_FP_SOFT_FLOAT;
  }
  return config;
}

}  // namespace

_cl_device_id::_cl_device_id(cl_platform_id platform,
                             mux_allocator_info_t mux_allocator,
                             mux_device_t mux_device)
    : base<_cl_device_id>(cl::ref_count_type::INTERNAL),
      platform(platform),
      mux_allocator(mux_allocator),
      mux_device(mux_device),
      address_bits(),
      available(CL_TRUE),
      compiler_available(CL_FALSE),
      double_fp_config(setOpenCLFromMux(mux_device->info->double_capabilities)),
      endian_little(mux_device->info->endianness == mux_endianness_little
                        ? CL_TRUE
                        : CL_FALSE),
      error_correction_support(CL_FALSE),
      execution_capabilities(CL_EXEC_KERNEL),
      global_mem_cache_size(mux_device->info->cache_size),
      global_mem_cache_type(),
      global_mem_cacheline_size(
          static_cast<cl_uint>(mux_device->info->cacheline_size)),
      global_mem_size(mux_device->info->memory_size),
      half_fp_config(setOpenCLFromMux(mux_device->info->half_capabilities)),
      host_unified_memory(CL_TRUE),
      image_support(mux_device->info->image_support),
      image3d_writes(mux_device->info->image3d_writes),
      image2d_max_height(mux_device->info->max_image_dimension_2d),
      image2d_max_width(mux_device->info->max_image_dimension_2d),
      image3d_max_depth(mux_device->info->max_image_dimension_3d),
      image3d_max_height(mux_device->info->max_image_dimension_3d),
      image3d_max_width(mux_device->info->max_image_dimension_3d),
      image_max_buffer_size(mux_device->info->max_image_dimension_1d),
      image_max_array_size(mux_device->info->max_image_array_layers),
      linker_available(CL_FALSE),
      local_mem_size(mux_device->info->shared_local_memory_size),
      local_mem_type((mux_shared_local_memory_physical ==
                      mux_device->info->shared_local_memory_type)
                         ? CL_LOCAL
                         : CL_GLOBAL),
      max_clock_frequency(mux_device->info->clock_frequency),
      max_compute_units(mux_device->info->compute_units),
      max_constant_args(8),                 // 8 is spec mandated minimum
      max_constant_buffer_size(64 * 1024),  // 64k is spec mandated minimum
      max_mem_alloc_size(mux_device->info->allocation_size),
      max_parameter_size(1024),  // 1024 is spec mandated minimum
      max_read_image_args(mux_device->info->max_sampled_images),
      max_samplers(mux_device->info->max_samplers),
      max_work_group_size(mux_device->info->max_concurrent_work_items),
      max_work_item_dimensions(cl::max::WORK_ITEM_DIM),
      max_work_item_sizes{mux_device->info->max_work_group_size_x,
                          mux_device->info->max_work_group_size_y,
                          mux_device->info->max_work_group_size_z},
      max_write_image_args(mux_device->info->max_storage_images),
      // TODO: mem_base_addr_align min max requirement is less for embedded
      // profile, we might need to expose this in Mux.
      mem_base_addr_align(
          std::max<cl_uint>(CHAR_BIT * sizeof(cl_long16),
                            CHAR_BIT * mux_device->info->buffer_alignment)),
      min_data_type_align_size(mux_device->info->buffer_alignment),
      native_vector_width_char(clamp(
          mux_device->info->native_vector_width / sizeof(cl_char), 1, 16)),
      native_vector_width_short(clamp(
          mux_device->info->native_vector_width / sizeof(cl_short), 1, 16)),
      native_vector_width_int(
          clamp(mux_device->info->native_vector_width / sizeof(cl_int), 1, 16)),
      native_vector_width_long(clamp(
          mux_device->info->native_vector_width / sizeof(cl_long), 1, 16)),
      native_vector_width_float(clamp(
          mux_device->info->native_vector_width / sizeof(cl_float), 1, 16)),
      native_vector_width_double(
          mux_device->info->double_capabilities
              ? clamp(mux_device->info->native_vector_width / sizeof(cl_double),
                      1, 16)
              : 0),
      native_vector_width_half(
          mux_device->info->half_capabilities
              ? clamp(mux_device->info->native_vector_width / sizeof(cl_half),
                      1, 16)
              : 0),
      parent_device(0),
      partition_max_sub_devices(0),
      partition_properties(0),
      partition_affinity_domain(0),
      partition_type(0),
      preferred_vector_width_char(clamp(
          mux_device->info->preferred_vector_width / sizeof(cl_char), 1, 16)),
      preferred_vector_width_short(clamp(
          mux_device->info->preferred_vector_width / sizeof(cl_short), 1, 16)),
      preferred_vector_width_int(clamp(
          mux_device->info->preferred_vector_width / sizeof(cl_int), 1, 16)),
      preferred_vector_width_long(clamp(
          mux_device->info->preferred_vector_width / sizeof(cl_long), 1, 16)),
      preferred_vector_width_float(clamp(
          mux_device->info->preferred_vector_width / sizeof(cl_float), 1, 16)),
      preferred_vector_width_double(
          mux_device->info->double_capabilities
              ? clamp(mux_device->info->preferred_vector_width /
                          sizeof(cl_double),
                      1, 16)
              : 0),
      preferred_vector_width_half(
          mux_device->info->half_capabilities
              ? clamp(
                    mux_device->info->preferred_vector_width / sizeof(cl_half),
                    1, 16)
              : 0),
      printf_buffer_size(compiler::PRINTF_BUFFER_SIZE),
      preferred_interop_user_sync(CL_TRUE),
      profile(),
      profiling_timer_resolution(5),                // Get from Mux?
      queue_properties(CL_QUEUE_PROFILING_ENABLE),  // Get from Mux?
      reference_count(1),  // All devices are root devices.
      single_fp_config(setOpenCLFromMux(mux_device->info->float_capabilities)),
      type(),
      vendor_id(mux_device->info->khronos_vendor_id)
#if defined(CL_VERSION_3_0)
      ,
      svm_capabilities(0),
      atomic_memory_capabilities(CL_DEVICE_ATOMIC_ORDER_RELAXED |
                                 CL_DEVICE_ATOMIC_SCOPE_WORK_GROUP),
      atomic_fence_capabilities(CL_DEVICE_ATOMIC_ORDER_RELAXED |
                                CL_DEVICE_ATOMIC_ORDER_ACQ_REL |
                                CL_DEVICE_ATOMIC_SCOPE_WORK_GROUP),
      device_enqueue_capabilities(0),
      queue_on_device_properties(0),
      queue_on_device_prefered_size(0),
      queue_on_device_max_size(0),
      max_on_device_queues(0),
      max_on_device_events(0),
      pipe_support(CL_FALSE),
      max_pipe_args(0),
      pipe_max_active_reservations(0),
      pipe_max_packet_size(0),
      max_global_variable_size(0),
      global_variable_prefered_total_size(0),
      non_uniform_work_group_support(0),
      max_read_write_image_args(0),
      image_pitch_alignment(0),
      image_base_address_alignment(0),
      il_version(""),
      max_num_sub_groups(mux_device->info->max_sub_group_count),
      sub_group_independent_forward_progress(
          mux_device->info->sub_groups_support_ifp ? CL_TRUE : CL_FALSE),
      work_group_collective_functions_support(
          mux_device->info->supports_work_group_collectives),
      generic_address_space_support(
          mux_device->info->supports_generic_address_space),
      preferred_platform_atomic_alignment(0),
      preferred_global_atomic_alignment(0),
      preferred_local_atomic_alignment(0),
      preferred_work_group_size_multiple(1)
#endif
{
  cl::retainInternal(platform);

  version = CA_CL_DEVICE_VERSION;
  compiler_info = compiler::getCompilerForDevice(platform->getCompilerLibrary(),
                                                 mux_device->info);
  if (compiler_info) {
    compiler_available = linker_available = CL_TRUE;

#if defined(CL_VERSION_3_0)
    il_version = "SPIR-V_1.0";
#endif

    const char *llvm_version =
        compiler::llvmVersion(platform->getCompilerLibrary());
    version += " LLVM ";
    version += llvm_version ? llvm_version : "Unknown";
  } else {
    version += " Offline-only";
  }
  profile =
      cl::binary::detectMuxDeviceProfile(compiler_available, mux_device->info);

  // We initialize max_work_item_dimensions above with cl::max::WORK_ITEM_DIM,
  // and the size of the max_work_item_sizes[] array is
  // cl::max::WORK_ITEM_DIM, but we then initialize the contents of the
  // max_work_item_sizes[] array with {x, y, z} dimensions from Mux.  Thus if
  // cl::max::WORK_ITEM_DIM is ever not 3 things will start to go wrong.
  static_assert(3 == cl::max::WORK_ITEM_DIM,
                "Mux API is hard-coded to 3 dimensions");

  if (cl::validate::IsInBitSet(mux_device->info->address_capabilities,
                               mux_address_capabilities_bits32)) {
    address_bits = 32;
  } else if (cl::validate::IsInBitSet(mux_device->info->address_capabilities,
                                      mux_address_capabilities_bits64)) {
    address_bits = 64;
  } else {
    OCL_ABORT("Unsupported mux_address_capabilities!");
  }

  const bool readCache = cl::validate::IsInBitSet(
      mux_device->info->cache_capabilities, mux_cache_capabilities_read);
  const bool writeCache = cl::validate::IsInBitSet(
      mux_device->info->cache_capabilities, mux_cache_capabilities_write);
  if (readCache && writeCache) {
    global_mem_cache_type = CL_READ_WRITE_CACHE;
  } else if (readCache) {
    global_mem_cache_type = CL_READ_ONLY_CACHE;
  } else {
    global_mem_cache_type = CL_NONE;
  }

  switch (mux_device->info->device_type) {
    case mux_device_type_cpu:
      type = CL_DEVICE_TYPE_CPU;
      break;
    case mux_device_type_gpu_integrated:
    case mux_device_type_gpu_discrete:
    case mux_device_type_gpu_virtual:
      type = CL_DEVICE_TYPE_GPU;
      break;
    case mux_device_type_accelerator:
      type = CL_DEVICE_TYPE_ACCELERATOR;
      break;
    case mux_device_type_custom:
      type = CL_DEVICE_TYPE_CUSTOM;
      break;
  }

  auto builtins =
      cargo::split(mux_device->info->builtin_kernel_declarations, ";");

  for (const auto &builtin : builtins) {
    const size_t pos = builtin.find("(");

    if (pos != cargo::string_view::npos) {
      if (!builtin_kernel_names.empty()) {
        builtin_kernel_names.append(";");
      }

      // Declarations can have leading/trailing whitespace. Trim it.
      const auto trimmed = cargo::trim(cargo::string_view(builtin.data(), pos));

      builtin_kernel_names.append(trimmed.cbegin(), trimmed.cend());
    }
  }
#if defined(CL_VERSION_3_0)
  for (const cl_version_khr version :
       {CL_MAKE_VERSION_KHR(1, 2, 0), CL_MAKE_VERSION_KHR(1, 1, 0),
        CL_MAKE_VERSION_KHR(1, 0, 0), CL_MAKE_VERSION_KHR(3, 0, 0)}) {
    cl_name_version_khr name_version{};
    std::strcpy(name_version.name, "OpenCL C");
    name_version.version = version;
    auto error = opencl_c_all_versions.push_back(name_version);
    (void)error;
    OCL_ASSERT(error == cargo::success, "Out of memory");
  }
#endif
}

_cl_device_id::~_cl_device_id() {
  muxDestroyDevice(mux_device, mux_allocator);
  cl::releaseInternal(platform);
}

/// @brief  OpenCL has reserved bit fields for half, but doesn't define them so
/// we have to.
#if !defined(CL_DEVICE_HALF_FP_CONFIG)
enum { CL_DEVICE_HALF_FP_CONFIG = 0x1033 };
#endif  // !defined(CL_DEVICE_HALF_FP_CONFIG)

CL_API_ENTRY cl_int CL_API_CALL cl::GetDeviceIDs(cl_platform_id platform,
                                                 cl_device_type device_type,
                                                 cl_uint num_entries,
                                                 cl_device_id *devices,
                                                 cl_uint *num_devices) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetDeviceIDs");
  OCL_CHECK(!platform, return CL_INVALID_PLATFORM);
  OCL_CHECK(!device_type, return CL_INVALID_DEVICE_TYPE);
  const cl_device_type validDeviceMask =
      CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR |
      CL_DEVICE_TYPE_CUSTOM | CL_DEVICE_TYPE_DEFAULT;
  OCL_CHECK(
      (CL_DEVICE_TYPE_ALL != device_type) && (~validDeviceMask & device_type),
      return CL_INVALID_DEVICE_TYPE);
  OCL_CHECK(devices && (0 == num_entries), return CL_INVALID_VALUE);
  OCL_CHECK(!devices && (0 < num_entries), return CL_INVALID_VALUE);
  OCL_CHECK(!devices && !num_devices, return CL_INVALID_VALUE);

  cl_uint numDevices = 0;
  for (auto device : platform->devices) {
    if (device->type & device_type) {
      numDevices++;
    }
  }
  // When checking for the number of devices, if there is at least one device on
  // the system and we have been asked to report back at least the DEFAULT type
  // but we haven't found any yet we want to report that there is at least 1
  // available device (the default one)
  if ((platform->devices.size() > 0) &&
      (device_type & CL_DEVICE_TYPE_DEFAULT) && (numDevices < 1)) {
    numDevices = 1;
  }

  OCL_CHECK(0 == numDevices, return CL_DEVICE_NOT_FOUND);

  if (devices) {
    const cl_uint numEntries = std::min(num_entries, numDevices);

    if (numEntries) {
      cl_uint out_device_index = 0;
      int current_priority = 0;

      for (auto device : platform->devices) {
        // Do a normal device check. If we have a device that matches the type
        // add it.
        if (device->type & device_type) {
          devices[out_device_index] = device;
          out_device_index++;
          if (out_device_index == numEntries) {
            break;
          }
        }
      }
      // If we haven't found enough devices. Start looking for a default device.
      if (out_device_index < numEntries) {
        for (auto device : platform->devices) {
          if ((device_type & CL_DEVICE_TYPE_DEFAULT) &&
              (device->mux_device->info->device_priority >= current_priority)) {
            current_priority = device->mux_device->info->device_priority;
            devices[out_device_index] = device;
          }
        }
      }
    }

    for (cl_uint index = 0; index < numEntries; ++index) {
      OCL_CHECK(!(devices[index]), return CL_OUT_OF_HOST_MEMORY);
    }
  }

  OCL_SET_IF_NOT_NULL(num_devices, numDevices);

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::RetainDevice(cl_device_id device) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clRetainDevice");
  // The OpenCL spec says that this function does nothing for root level
  // devices (because such devices are not created, they are retrieved via
  // clGetDeviceIDs). We don't support sub devices yet, so there is no actual
  // work to do here.  We do, however, need to check if it's a pointer to an
  // actual device.
  //
  // From section 4.3 of the OpenCL 1.2 spec (page 53):
  // "The function clRetainDevice increments the device reference count if
  // 'device' is a valid sub-device created by a call to clCreateSubDevices. If
  // 'device' is a root level device i.e. a cl_device_id returned by
  // clGetDeviceIDs, the 'device' reference count remains unchanged."
  auto platform = _cl_platform_id::getInstance();
  if (!platform) {
    return CL_INVALID_DEVICE;
  }
  auto &devices = platform.value()->devices;
  if (std::find(devices.begin(), devices.end(), device) == devices.end()) {
    return CL_INVALID_DEVICE;
  }
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::ReleaseDevice(cl_device_id device) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clReleaseDevice");
  // This function does not release anything, see cl::RetainDevice.
  auto platform = _cl_platform_id::getInstance();
  if (!platform) {
    return CL_INVALID_DEVICE;
  }
  auto &devices = platform.value()->devices;
  if (std::find(devices.begin(), devices.end(), device) == devices.end()) {
    return CL_INVALID_DEVICE;
  }
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::GetDeviceInfo(
    cl_device_id device, cl_device_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetDeviceInfo");
  if (!device) {
    return CL_INVALID_DEVICE;
  }

  // driver_version has to be Major.Minor - CA_VERSION has a.b.c, so
  // we need to strip off the last digit.
  const cargo::string_view ocl_version(CA_CL_DRIVER_VERSION);
  const auto firstFullstop = ocl_version.find_first_of(".");
  const auto secondFullstop = ocl_version.find_first_of(".", firstFullstop + 1);
  const auto errorOrVersion = ocl_version.substr(0, secondFullstop);

  if (!errorOrVersion) {
    return CL_INVALID_DEVICE;
  }

  const cargo::string_view driver_version = *errorOrVersion;

#define DEVICE_INFO_CASE_SPECIAL_STRING(ENUM, TYPE)                           \
  case ENUM: {                                                                \
    const size_t typeSize = (TYPE).size() + 1;                                \
    OCL_CHECK(param_value && (param_value_size < typeSize),                   \
              return CL_INVALID_VALUE);                                       \
    if (param_value) {                                                        \
      /* Although we're copying string data we may be deliberating copying a  \
       * truncated view, so use memcpy to signal to the compiler that we know \
       * exactly how many bytes to copy. */                                   \
      std::memcpy((char *)param_value, (TYPE).data(), (TYPE).size());         \
      ((char *)param_value)[(TYPE).size()] = '\0';                            \
    }                                                                         \
    OCL_SET_IF_NOT_NULL(param_value_size_ret, typeSize);                      \
  } break

#define DEVICE_INFO_CASE_SPECIAL_VECTOR(ENUM, TYPE)              \
  case ENUM: {                                                   \
    const size_t typeSize =                                      \
        (TYPE).size() * sizeof(decltype(TYPE)::value_type);      \
    OCL_CHECK(param_value && (param_value_size < typeSize),      \
              return CL_INVALID_VALUE);                          \
    if (param_value) {                                           \
      std::memcpy((char *)param_value, (TYPE).data(), typeSize); \
    }                                                            \
    OCL_SET_IF_NOT_NULL(param_value_size_ret, typeSize);         \
  } break

#define DEVICE_INFO_CASE(ENUM, TYPE)                                           \
  case ENUM: {                                                                 \
    const size_t typeSize = sizeof(TYPE);                                      \
    OCL_CHECK(param_value && (param_value_size < typeSize),                    \
              return CL_INVALID_VALUE);                                        \
    if (param_value) {                                                         \
      *static_cast<std::remove_const_t<decltype(TYPE)> *>(param_value) = TYPE; \
    }                                                                          \
    OCL_SET_IF_NOT_NULL(param_value_size_ret, typeSize);                       \
  } break

  switch (param_name) {
    DEVICE_INFO_CASE_SPECIAL_STRING(
        CL_DEVICE_NAME,
        cargo::string_view(device->mux_device->info->device_name));
    DEVICE_INFO_CASE_SPECIAL_STRING(CL_DEVICE_VENDOR,
                                    cargo::string_view(CA_CL_DEVICE_VENDOR));
    DEVICE_INFO_CASE_SPECIAL_STRING(CL_DEVICE_VERSION,
                                    cargo::string_view(device->version));
    DEVICE_INFO_CASE_SPECIAL_STRING(
        CL_DEVICE_OPENCL_C_VERSION,
        cargo::string_view(CA_CL_DEVICE_OPENCL_C_VERSION));
    DEVICE_INFO_CASE(CL_DEVICE_SINGLE_FP_CONFIG, device->single_fp_config);
    DEVICE_INFO_CASE(CL_DEVICE_ADDRESS_BITS, device->address_bits);
    DEVICE_INFO_CASE(CL_DEVICE_AVAILABLE, device->available);
    DEVICE_INFO_CASE_SPECIAL_STRING(
        CL_DEVICE_BUILT_IN_KERNELS,
        cargo::string_view(device->builtin_kernel_names));
    DEVICE_INFO_CASE(CL_DEVICE_COMPILER_AVAILABLE, device->compiler_available);
    DEVICE_INFO_CASE(CL_DEVICE_DOUBLE_FP_CONFIG, device->double_fp_config);
    DEVICE_INFO_CASE(CL_DEVICE_ENDIAN_LITTLE, device->endian_little);
    DEVICE_INFO_CASE(CL_DEVICE_ERROR_CORRECTION_SUPPORT,
                     device->error_correction_support);
    DEVICE_INFO_CASE(CL_DEVICE_EXECUTION_CAPABILITIES,
                     device->execution_capabilities);
    case CL_DEVICE_EXTENSIONS: {
      return extension::GetDeviceInfo(device, param_name, param_value_size,
                                      param_value, param_value_size_ret);
    }
      DEVICE_INFO_CASE(CL_DEVICE_GLOBAL_MEM_CACHE_SIZE,
                       device->global_mem_cache_size);
      DEVICE_INFO_CASE(CL_DEVICE_GLOBAL_MEM_CACHE_TYPE,
                       device->global_mem_cache_type);
      DEVICE_INFO_CASE(CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE,
                       device->global_mem_cacheline_size);
      DEVICE_INFO_CASE(CL_DEVICE_GLOBAL_MEM_SIZE, device->global_mem_size);
      DEVICE_INFO_CASE(CL_DEVICE_HOST_UNIFIED_MEMORY,
                       device->host_unified_memory);
      DEVICE_INFO_CASE(CL_DEVICE_IMAGE_SUPPORT, device->image_support);
      DEVICE_INFO_CASE(CL_DEVICE_IMAGE2D_MAX_HEIGHT,
                       device->image2d_max_height);
      DEVICE_INFO_CASE(CL_DEVICE_IMAGE2D_MAX_WIDTH, device->image2d_max_width);
      DEVICE_INFO_CASE(CL_DEVICE_IMAGE3D_MAX_DEPTH, device->image3d_max_depth);
      DEVICE_INFO_CASE(CL_DEVICE_IMAGE3D_MAX_HEIGHT,
                       device->image3d_max_height);
      DEVICE_INFO_CASE(CL_DEVICE_IMAGE3D_MAX_WIDTH, device->image3d_max_width);
      DEVICE_INFO_CASE(CL_DEVICE_IMAGE_MAX_BUFFER_SIZE,
                       device->image_max_buffer_size);
      DEVICE_INFO_CASE(CL_DEVICE_IMAGE_MAX_ARRAY_SIZE,
                       device->image_max_array_size);
      DEVICE_INFO_CASE(CL_DEVICE_LINKER_AVAILABLE, device->linker_available);
      DEVICE_INFO_CASE(CL_DEVICE_LOCAL_MEM_SIZE, device->local_mem_size);
      DEVICE_INFO_CASE(CL_DEVICE_LOCAL_MEM_TYPE, device->local_mem_type);
      DEVICE_INFO_CASE(CL_DEVICE_MAX_CLOCK_FREQUENCY,
                       device->max_clock_frequency);
      DEVICE_INFO_CASE(CL_DEVICE_MAX_COMPUTE_UNITS, device->max_compute_units);
      DEVICE_INFO_CASE(CL_DEVICE_MAX_CONSTANT_ARGS, device->max_constant_args);
      DEVICE_INFO_CASE(CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE,
                       device->max_constant_buffer_size);
      DEVICE_INFO_CASE(CL_DEVICE_MAX_MEM_ALLOC_SIZE,
                       device->max_mem_alloc_size);
      DEVICE_INFO_CASE(CL_DEVICE_MAX_PARAMETER_SIZE,
                       device->max_parameter_size);
      DEVICE_INFO_CASE(CL_DEVICE_MAX_READ_IMAGE_ARGS,
                       device->max_read_image_args);
      DEVICE_INFO_CASE(CL_DEVICE_MAX_SAMPLERS, device->max_samplers);
      DEVICE_INFO_CASE(CL_DEVICE_MAX_WORK_GROUP_SIZE,
                       device->max_work_group_size);
      DEVICE_INFO_CASE(CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
                       device->max_work_item_dimensions);
    case CL_DEVICE_MAX_WORK_ITEM_SIZES: {
      const size_t typeSize = device->max_work_item_dimensions * sizeof(size_t);
      OCL_CHECK(param_value && (param_value_size < typeSize),
                return CL_INVALID_VALUE);
      if (param_value) {
        std::uninitialized_copy_n(device->max_work_item_sizes,
                                  device->max_work_item_dimensions,
                                  static_cast<size_t *>(param_value));
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, typeSize);
    } break;
      DEVICE_INFO_CASE(CL_DEVICE_MAX_WRITE_IMAGE_ARGS,
                       device->max_write_image_args);
      DEVICE_INFO_CASE(CL_DEVICE_MEM_BASE_ADDR_ALIGN,
                       device->mem_base_addr_align);
      DEVICE_INFO_CASE(CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE,
                       device->min_data_type_align_size);
      DEVICE_INFO_CASE(CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR,
                       device->native_vector_width_char);
      DEVICE_INFO_CASE(CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT,
                       device->native_vector_width_short);
      DEVICE_INFO_CASE(CL_DEVICE_NATIVE_VECTOR_WIDTH_INT,
                       device->native_vector_width_int);
      DEVICE_INFO_CASE(CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG,
                       device->native_vector_width_long);
      DEVICE_INFO_CASE(CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT,
                       device->native_vector_width_float);
      DEVICE_INFO_CASE(CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE,
                       device->native_vector_width_double);
      DEVICE_INFO_CASE(CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF,
                       device->native_vector_width_half);
      DEVICE_INFO_CASE(CL_DEVICE_PARENT_DEVICE, device->parent_device);
      DEVICE_INFO_CASE(CL_DEVICE_PARTITION_MAX_SUB_DEVICES,
                       device->partition_max_sub_devices);
      DEVICE_INFO_CASE(CL_DEVICE_PARTITION_PROPERTIES,
                       device->partition_properties);
      DEVICE_INFO_CASE(CL_DEVICE_PARTITION_AFFINITY_DOMAIN,
                       device->partition_affinity_domain);
      DEVICE_INFO_CASE(CL_DEVICE_PARTITION_TYPE, device->partition_type);
      DEVICE_INFO_CASE(CL_DEVICE_PLATFORM, device->platform);
      DEVICE_INFO_CASE(CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR,
                       device->preferred_vector_width_char);
      DEVICE_INFO_CASE(CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT,
                       device->preferred_vector_width_short);
      DEVICE_INFO_CASE(CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT,
                       device->preferred_vector_width_int);
      DEVICE_INFO_CASE(CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG,
                       device->preferred_vector_width_long);
      DEVICE_INFO_CASE(CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT,
                       device->preferred_vector_width_float);
      DEVICE_INFO_CASE(CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE,
                       device->preferred_vector_width_double);
      DEVICE_INFO_CASE(CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF,
                       device->preferred_vector_width_half);
      DEVICE_INFO_CASE(CL_DEVICE_PRINTF_BUFFER_SIZE,
                       device->printf_buffer_size);
      DEVICE_INFO_CASE(CL_DEVICE_PREFERRED_INTEROP_USER_SYNC,
                       device->preferred_interop_user_sync);
      DEVICE_INFO_CASE_SPECIAL_STRING(CL_DEVICE_PROFILE, device->profile);
      DEVICE_INFO_CASE(CL_DEVICE_PROFILING_TIMER_RESOLUTION,
                       device->profiling_timer_resolution);
      DEVICE_INFO_CASE(CL_DEVICE_QUEUE_PROPERTIES, device->queue_properties);
      DEVICE_INFO_CASE(CL_DEVICE_REFERENCE_COUNT, device->reference_count);
      DEVICE_INFO_CASE(CL_DEVICE_TYPE, device->type);
      DEVICE_INFO_CASE(CL_DEVICE_VENDOR_ID, device->vendor_id);
      DEVICE_INFO_CASE_SPECIAL_STRING(CL_DRIVER_VERSION, driver_version);
#if defined(CL_VERSION_3_0)
      DEVICE_INFO_CASE(CL_DEVICE_SVM_CAPABILITIES, device->svm_capabilities);
      DEVICE_INFO_CASE(CL_DEVICE_ATOMIC_MEMORY_CAPABILITIES,
                       device->atomic_memory_capabilities);
      DEVICE_INFO_CASE(CL_DEVICE_ATOMIC_FENCE_CAPABILITIES,
                       device->atomic_fence_capabilities);
      DEVICE_INFO_CASE(CL_DEVICE_DEVICE_ENQUEUE_CAPABILITIES,
                       device->device_enqueue_capabilities);
      DEVICE_INFO_CASE(CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES,
                       device->queue_on_device_properties);
      DEVICE_INFO_CASE(CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE,
                       device->queue_on_device_prefered_size);
      DEVICE_INFO_CASE(CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE,
                       device->queue_on_device_max_size);
      DEVICE_INFO_CASE(CL_DEVICE_MAX_ON_DEVICE_QUEUES,
                       device->max_on_device_queues);
      DEVICE_INFO_CASE(CL_DEVICE_MAX_ON_DEVICE_EVENTS,
                       device->max_on_device_events);
      DEVICE_INFO_CASE(CL_DEVICE_PIPE_SUPPORT, device->pipe_support);
      DEVICE_INFO_CASE(CL_DEVICE_MAX_PIPE_ARGS, device->max_pipe_args);
      DEVICE_INFO_CASE(CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS,
                       device->pipe_max_active_reservations);
      DEVICE_INFO_CASE(CL_DEVICE_PIPE_MAX_PACKET_SIZE,
                       device->pipe_max_packet_size);
      DEVICE_INFO_CASE(CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE,
                       device->max_global_variable_size);
      DEVICE_INFO_CASE(CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE,
                       device->global_variable_prefered_total_size);
      DEVICE_INFO_CASE(CL_DEVICE_NON_UNIFORM_WORK_GROUP_SUPPORT,
                       device->non_uniform_work_group_support);
      DEVICE_INFO_CASE(CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS,
                       device->max_read_write_image_args);
      DEVICE_INFO_CASE(CL_DEVICE_IMAGE_PITCH_ALIGNMENT,
                       device->image_pitch_alignment);
      DEVICE_INFO_CASE(CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT,
                       device->image_base_address_alignment);
      DEVICE_INFO_CASE_SPECIAL_STRING(CL_DEVICE_IL_VERSION, device->il_version);
    case CL_DEVICE_ILS_WITH_VERSION: {
      auto split_il_version = cargo::split(device->il_version, ";");
      const size_t size_in_bytes =
          split_il_version.size() * sizeof(cl_name_version_khr);
      OCL_CHECK(param_value && (param_value_size < size_in_bytes),
                return CL_INVALID_VALUE);
      if (param_value) {
        cargo::dynamic_array<cl_name_version_khr> device_ils_with_version{};
        auto error = device_ils_with_version.alloc(split_il_version.size());
        if (cargo::success != error) {
          return CL_OUT_OF_HOST_MEMORY;
        }
        for (size_t i = 0; i < split_il_version.size(); ++i) {
          auto il_version_pair = cargo::split(split_il_version[i], "_");
          auto il_prefix = il_version_pair[0];
          auto major_minor_version_pair = cargo::split(il_version_pair[1], ".");
          auto major_version = major_minor_version_pair[0];
          auto minor_version = major_minor_version_pair[1];
          cl_name_version_khr ils_with_version{};
          std::memcpy(ils_with_version.name, il_prefix.data(),
                      il_prefix.size());
          ils_with_version.name[il_prefix.size()] = '\0';
          ils_with_version.version = CL_MAKE_VERSION_KHR(
              *major_version.data() - '0', *minor_version.data() - '0', 0);
          device_ils_with_version[i] = ils_with_version;
        }
        std::memcpy(param_value, device_ils_with_version.data(),
                    device_ils_with_version.size() *
                        sizeof(decltype(device_ils_with_version)::value_type));
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, size_in_bytes);
    } break;
      DEVICE_INFO_CASE(CL_DEVICE_MAX_NUM_SUB_GROUPS,
                       device->max_num_sub_groups);
      DEVICE_INFO_CASE(CL_DEVICE_SUB_GROUP_INDEPENDENT_FORWARD_PROGRESS,
                       device->sub_group_independent_forward_progress);
      DEVICE_INFO_CASE(CL_DEVICE_WORK_GROUP_COLLECTIVE_FUNCTIONS_SUPPORT,
                       device->work_group_collective_functions_support);
      DEVICE_INFO_CASE(CL_DEVICE_GENERIC_ADDRESS_SPACE_SUPPORT,
                       device->generic_address_space_support);
    case CL_DEVICE_NUMERIC_VERSION: {
      OCL_CHECK(param_value && (param_value_size < sizeof(cl_version)),
                return CL_INVALID_VALUE);
      if (param_value) {
        const cargo::string_view version_string{device->version};
        *static_cast<cl_version_khr *>(param_value) = CL_MAKE_VERSION_KHR(
            CA_CL_PLATFORM_VERSION_MAJOR, CA_CL_PLATFORM_VERSION_MINOR, 0);
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(cl_version));
    } break;
    case CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION: {
      auto split_built_in_kernel_names =
          cargo::split(device->builtin_kernel_names, ";");
      const size_t size_in_bytes =
          split_built_in_kernel_names.size() * sizeof(cl_name_version_khr);
      OCL_CHECK(param_value && (param_value_size < size_in_bytes),
                return CL_INVALID_VALUE);
      if (param_value) {
        cargo::dynamic_array<cl_name_version_khr>
            built_in_kernels_with_version{};
        auto error = built_in_kernels_with_version.alloc(
            split_built_in_kernel_names.size());
        if (cargo::success != error) {
          return CL_OUT_OF_HOST_MEMORY;
        }
        for (size_t i = 0; i < built_in_kernels_with_version.size(); ++i) {
          cl_name_version_khr built_in_kernel_with_version{};
          OCL_ASSERT(
              split_built_in_kernel_names[i].size() <
                  CL_NAME_VERSION_MAX_NAME_SIZE_KHR,
              "Built in kernel name exceeds buffer in cl_name_version object");
          std::memcpy(
              built_in_kernel_with_version.name,
              split_built_in_kernel_names[i].data(),
              std::min(split_built_in_kernel_names[i].size(),
                       static_cast<size_t>(CL_NAME_VERSION_MAX_NAME_SIZE_KHR)));
          built_in_kernel_with_version
              .name[split_built_in_kernel_names[i].size()] = '\0';
          built_in_kernel_with_version.version = CL_MAKE_VERSION_KHR(1, 0, 0);
          built_in_kernels_with_version[i] = built_in_kernel_with_version;
        }
        std::memcpy(
            param_value, built_in_kernels_with_version.data(),
            built_in_kernels_with_version.size() *
                sizeof(decltype(built_in_kernels_with_version)::value_type));
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, size_in_bytes);
    } break;
      DEVICE_INFO_CASE_SPECIAL_VECTOR(CL_DEVICE_OPENCL_C_ALL_VERSIONS,
                                      device->opencl_c_all_versions);
    case CL_DEVICE_OPENCL_C_FEATURES: {
      // First pass just calculate how many cl_name_version_khr we need.
      size_t name_version_count{0};
      if (device->image3d_writes) {
        ++name_version_count;
      }
      if (device->atomic_memory_capabilities & CL_DEVICE_ATOMIC_ORDER_ACQ_REL) {
        ++name_version_count;
      }
      if (device->atomic_memory_capabilities & CL_DEVICE_ATOMIC_ORDER_SEQ_CST) {
        ++name_version_count;
      }
      if (device->atomic_memory_capabilities & CL_DEVICE_ATOMIC_SCOPE_DEVICE) {
        ++name_version_count;
      }
      if (device->atomic_memory_capabilities &
          CL_DEVICE_ATOMIC_SCOPE_ALL_DEVICES) {
        ++name_version_count;
      }
      if (device->device_enqueue_capabilities) {
        ++name_version_count;
      }
      if (device->generic_address_space_support) {
        ++name_version_count;
      }
      if (device->pipe_support) {
        ++name_version_count;
      }
      if (device->max_global_variable_size != 0) {
        ++name_version_count;
      }
      if (device->max_read_image_args != 0) {
        ++name_version_count;
      }
      if (device->max_num_sub_groups != 0) {
        ++name_version_count;
      }
      if (device->work_group_collective_functions_support) {
        ++name_version_count;
      }
      if (device->mux_device->info->integer_capabilities &
          mux_integer_capabilities_64bit) {
        ++name_version_count;
      }
      if (device->mux_device->info->double_capabilities) {
        ++name_version_count;
      }
      if (device->mux_device->info->image_support) {
        ++name_version_count;
      }
      const size_t required_size_in_bytes =
          name_version_count * sizeof(cl_name_version_khr);
      OCL_CHECK(param_value && (param_value_size < required_size_in_bytes),
                return CL_INVALID_VALUE);
      if (param_value) {
        // Second pass we know we have enough memory so just copy in the data.
        auto name_version_array =
            static_cast<cl_name_version_khr *>(param_value);
        auto copy_name_version = [](const char *name,
                                    cl_name_version_khr *name_version) {
          constexpr cl_version version_30 = CL_MAKE_VERSION_KHR(3, 0, 0);
          name_version->version = version_30;
          std::memcpy(name_version->name, name, std::strlen(name) + 1);
        };

        if (device->image3d_writes) {
          copy_name_version("__opencl_c_3d_image_writes", name_version_array++);
        }
        if (device->atomic_memory_capabilities &
            CL_DEVICE_ATOMIC_ORDER_ACQ_REL) {
          copy_name_version("__opencl_c_atomic_order_acq_rel",
                            name_version_array++);
        }
        if (device->atomic_memory_capabilities &
            CL_DEVICE_ATOMIC_ORDER_SEQ_CST) {
          copy_name_version("__opencl_c_atomic_order_seq_cst",
                            name_version_array++);
        }
        if (device->atomic_memory_capabilities &
            CL_DEVICE_ATOMIC_SCOPE_DEVICE) {
          copy_name_version("__opencl_c_atomic_scope_device",
                            name_version_array++);
        }
        if (device->atomic_memory_capabilities &
            CL_DEVICE_ATOMIC_SCOPE_ALL_DEVICES) {
          copy_name_version("__opencl_c_atomic_scope_all_devices",
                            name_version_array++);
        }
        if (device->device_enqueue_capabilities) {
          copy_name_version("__opencl_c_device_enqueue", name_version_array++);
        }
        if (device->generic_address_space_support) {
          copy_name_version("__opencl_c_generic_address_space",
                            name_version_array++);
        }
        if (device->pipe_support) {
          copy_name_version("__opencl_c_pipes", name_version_array++);
        }
        if (device->max_global_variable_size != 0) {
          copy_name_version("__opencl_c_program_scope_global_variables",
                            name_version_array++);
        }
        if (device->max_read_image_args != 0) {
          copy_name_version("__opencl_read_write_images", name_version_array++);
        }
        if (device->max_num_sub_groups != 0) {
          copy_name_version("__opencl_c_subgroups", name_version_array++);
        }
        if (device->work_group_collective_functions_support) {
          copy_name_version("__opencl_c_work_group_collective_functions",
                            name_version_array++);
        }
        if (device->mux_device->info->integer_capabilities &
            mux_integer_capabilities_64bit) {
          copy_name_version("__opencl_c_int64", name_version_array++);
        }
        if (device->mux_device->info->double_capabilities) {
          copy_name_version("__opencl_c_fp64", name_version_array++);
        }
        if (device->mux_device->info->image_support) {
          copy_name_version("__opencl_c_images", name_version_array++);
        }
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, required_size_in_bytes);
    } break;
      DEVICE_INFO_CASE(CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT,
                       device->preferred_platform_atomic_alignment);
      DEVICE_INFO_CASE(CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT,
                       device->preferred_global_atomic_alignment);
      DEVICE_INFO_CASE(CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT,
                       device->preferred_local_atomic_alignment);
      DEVICE_INFO_CASE(CL_DEVICE_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                       device->preferred_work_group_size_multiple);
      // This returns the date of the git commit of the most recent CTS version
      // tested in CA-OpenCL-CTS: 5bb4d089dd13d7f33225c77d95e9547dff3057df.
      // TODO: Update this commit when we pass the CTS (see CA-2974).
      // TODO: This should probably also eventually be a Mux property since it
      // will be per device, however since it is 3.0 only and we do not
      // currently ship 3.0 this is enough for the MVP (see CA-2975).
      DEVICE_INFO_CASE_SPECIAL_STRING(
          CL_DEVICE_LATEST_CONFORMANCE_VERSION_PASSED,
          cargo::string_view{"v2020-10-18-08"});
#endif
    default: {
      return extension::GetDeviceInfo(device, param_name, param_value_size,
                                      param_value, param_value_size_ret);
    }
  }
#undef DEVICE_INFO_CASE
#undef DEVICE_INFO_CASE_SPECIAL_STRING
#undef DEVICE_INFO_CASE_SPECIAL_VECTOR

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::CreateSubDevices(
    cl_device_id in_device, const cl_device_partition_property *properties,
    cl_uint num_devices, cl_device_id *out_devices, cl_uint *num_devices_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCreateSubDevices");
  OCL_CHECK(!in_device, return CL_INVALID_DEVICE);

  OCL_UNUSED(out_devices);

  if (properties) {
    switch (properties[0]) {
      case CL_DEVICE_PARTITION_EQUALLY: {
        const cl_uint num_sub_devices = static_cast<cl_uint>(properties[1]);
        OCL_CHECK(num_sub_devices > num_devices, return CL_INVALID_VALUE);
        return CL_INVALID_VALUE;  // Sub devices not supported
      }
      case CL_DEVICE_PARTITION_BY_COUNTS: {
        OCL_CHECK(static_cast<cl_uint>(properties[1]) >
                      in_device->partition_max_sub_devices,
                  return CL_INVALID_DEVICE_PARTITION_COUNT);
        cl_uint idx = 0, device_count = 0, cu_count = 0;
        while (properties[++idx] != CL_DEVICE_PARTITION_BY_COUNTS_LIST_END) {
          if (properties[idx] != 0) {
            device_count++;
            cu_count += static_cast<cl_uint>(properties[idx]);
          }
        }
        OCL_CHECK(device_count > in_device->max_compute_units,
                  return CL_INVALID_DEVICE_PARTITION_COUNT);
        OCL_CHECK(cu_count > in_device->partition_max_sub_devices,
                  return CL_INVALID_DEVICE_PARTITION_COUNT);

        return CL_INVALID_VALUE;  // Sub devices not supported
      }
      case CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN:
        return CL_INVALID_VALUE;  // Sub devices not supported
      default:
        return CL_INVALID_VALUE;
    }
  }

  OCL_SET_IF_NOT_NULL(num_devices_ret, in_device->partition_max_sub_devices);

  return CL_SUCCESS;
}
