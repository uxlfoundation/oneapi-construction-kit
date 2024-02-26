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

#include <cargo/string_algorithm.h>
#include <host/builtin_kernel.h>

#include <array>
#include <initializer_list>
#include <mutex>
#include <random>
#include <string>

namespace {
#ifdef CA_HOST_ENABLE_BUILTIN_KERNEL
void nop_builtin_kernel(void *, host::schedule_info_s *) {}
#endif
}  // namespace

host::builtin_kernel_map host::getBuiltinKernels(
    mux_device_info_t device_info) {
  host::builtin_kernel_map builtin_kernels;
#ifndef CA_HOST_ENABLE_BUILTIN_KERNEL
  (void)device_info;
#else
  builtin_kernels = {
      // A set of test kernels used for testing builtin kernel functionality.
      {"copy_buffer(global const int* in, global int* out)",
       [](void *packed_args, host::schedule_info_s *schedule) {
         auto buffers = static_cast<int32_t **>(packed_args);
         auto slice_size = schedule->global_size[0] * schedule->global_size[1] *
                           schedule->global_size[2] / schedule->total_slices;

         auto lower_bound = slice_size * (schedule->slice);
         auto upper_bound = slice_size * (schedule->slice + 1);

         auto remainder = (schedule->global_size[0] * schedule->global_size[1] *
                           schedule->global_size[2] % schedule->total_slices);

         // If the work items doesn't evenly divide over the separate CPUs,
         // tack the remaining block onto the end of the final slice
         if (remainder != 0 && schedule->total_slices - 1 == schedule->slice) {
           upper_bound += remainder;
         }

         // buffers[0]: in, buffers[1]: out
         std::copy(buffers[0] + lower_bound, buffers[0] + upper_bound,
                   buffers[1] + lower_bound);
       }},

      // This kernel is here to test the presence of multiple kernels
      {"print_message()",
       [](void *, host::schedule_info_s *schedule) {
         printf("Executing printf via a builtin kernel from slice %zu\n",
                schedule->slice);
       }},
  };

  // To test kernel function argument declaration the following generates a
  // builtin kernel for each scalar and vector type, each has argument
  // uniquely applies type specifiers for value, address space qualifiers for
  // pointer types, and access qualifiers for image types. Builtin kernels
  // using features which are optional (half, double, images) and not
  // supported by the device are not generated.
  std::vector<std::string> scalar_types = {{"char", "uchar", "short", "ushort",
                                            "int", "uint", "long", "ulong",
                                            "float"}};
  if (device_info->double_capabilities) {
    scalar_types.emplace_back("double");
  }
  if (device_info->half_capabilities) {
    scalar_types.emplace_back("half");
  }
  const std::array<std::string, 6> vector_sizes = {
      {"", "2", "3", "4", "8", "16"}};
  const std::array<std::string, 3> address_space_qualifiers = {
      {"local", "global", "constant"}};

  // Add some random spaces to parameter strings to stress the parser. We don't
  // need cryptographically secure randomness, just something vaguely random.
  // The seed is for repeatability. In some places, 0 spaces is invalid.
  auto get_spaces = [](bool inc_zero = true) {
    static std::minstd_rand0 rand(1337);
    static std::uniform_int_distribution<int> dist0(0, 3);
    static std::uniform_int_distribution<int> dist1(1, 3);
    const size_t num = inc_zero ? dist0(rand) : dist1(rand);
    return std::string(num, ' ');
  };

  auto add_argument = [&get_spaces](std::string &kernel_decl,
                                    std::initializer_list<std::string> parts) {
    if (kernel_decl.back() == '(') {
      kernel_decl += get_spaces();
    } else {
      kernel_decl += get_spaces() + ',' + get_spaces();
    }
    for (const auto &part : parts) {
      // No space is allowed only if `*` is the trailing character
      kernel_decl += part + get_spaces(part.back() == '*');
    }
  };

  // Loop over all type combinations.
  for (auto &scalar_type : scalar_types) {
    for (auto &vector_size : vector_sizes) {
      const std::string type(scalar_type + vector_size);
      const std::string typeptr(type + get_spaces() + '*');
      std::string kernel_decl(get_spaces() + "args_" + type + get_spaces() +
                              '(');

      // Create value arguments with type qualifiers.
      add_argument(kernel_decl, {type, "t"});
      add_argument(kernel_decl, {"const", type, "ct"});
      add_argument(kernel_decl, {"volatile", type, "vt"});
      add_argument(kernel_decl, {"const volatile", type, "cvt"});

      // Create pointer arguments with address space qualifiers.
      for (auto &address_space_qualifier : address_space_qualifiers) {
        const std::string addrspace(address_space_qualifier.data(), 2);
        add_argument(kernel_decl,
                     {address_space_qualifier, typeptr, addrspace + "p"});
        add_argument(kernel_decl, {address_space_qualifier, "const", typeptr,
                                   addrspace + "cp"});
        add_argument(kernel_decl, {address_space_qualifier, "volatile", typeptr,
                                   addrspace + "vp"});
        add_argument(kernel_decl, {address_space_qualifier, typeptr, "restrict",
                                   addrspace + "rp"});
        add_argument(kernel_decl, {address_space_qualifier, "const volatile",
                                   typeptr, addrspace + "cvp"});
        add_argument(kernel_decl, {address_space_qualifier, "const", typeptr,
                                   "restrict", addrspace + "crp"});
        add_argument(kernel_decl, {address_space_qualifier, "volatile", typeptr,
                                   "restrict", addrspace + "vrp"});
        add_argument(kernel_decl, {address_space_qualifier, "const volatile",
                                   typeptr, "restrict", addrspace + "cvrp"});
      }
      kernel_decl += get_spaces() + ')' + get_spaces();
      builtin_kernels[kernel_decl] = nop_builtin_kernel;
    }
  }

  {
    // `void*` types are special: there are no vector or value versions
    const std::string typeptr("void" + get_spaces() + '*');
    std::string kernel_decl(get_spaces() + "args_void" + get_spaces() + '(');

    // Create pointer arguments with address space qualifiers.
    for (auto &address_space_qualifier : address_space_qualifiers) {
      const std::string addrspace(address_space_qualifier.data(), 2);
      add_argument(kernel_decl,
                   {address_space_qualifier, typeptr, addrspace + "p"});
      add_argument(kernel_decl, {address_space_qualifier, "const", typeptr,
                                 addrspace + "cp"});
      add_argument(kernel_decl, {address_space_qualifier, "volatile", typeptr,
                                 addrspace + "vp"});
      add_argument(kernel_decl, {address_space_qualifier, typeptr, "restrict",
                                 addrspace + "rp"});
      add_argument(kernel_decl, {address_space_qualifier, "const volatile",
                                 typeptr, addrspace + "cvp"});
      add_argument(kernel_decl, {address_space_qualifier, "const", typeptr,
                                 "restrict", addrspace + "crp"});
      add_argument(kernel_decl, {address_space_qualifier, "volatile", typeptr,
                                 "restrict", addrspace + "vrp"});
      add_argument(kernel_decl, {address_space_qualifier, "const volatile",
                                 typeptr, "restrict", addrspace + "cvrp"});
    }
    kernel_decl += get_spaces() + ')' + get_spaces();
    builtin_kernels[kernel_decl] = nop_builtin_kernel;
  }

  std::string kernel_decl(get_spaces() + "args_address_qualifiers" +
                          get_spaces() + '(');
  add_argument(kernel_decl, {"__local", "int", "*", "loi"});
  add_argument(kernel_decl, {"__global", "int", "*", "gli"});
  add_argument(kernel_decl, {"__constant", "int",
                             "*"
                             "coi"});
  kernel_decl += get_spaces() + ')' + get_spaces();
  builtin_kernels[kernel_decl] = nop_builtin_kernel;

  if (device_info->image_support) {
    const std::array<std::string, 6> image_types = {
        {"image1d_t", "image1d_array_t", "image1d_buffer_t", "image2d_t",
         "image2d_array_t", "image3d_t"}};

    // Create image arguments with access qualifiers.
    for (auto &image_type : image_types) {
      std::string kernel_decl(get_spaces() + "args_" + image_type +
                              get_spaces() + '(');
      add_argument(kernel_decl, {image_type, "i"});
      add_argument(kernel_decl, {"read_only", image_type, "roi"});
      if ((image_type != "image3d_t" || image_type != "image2d_array_t") ||
          (image_type == "image3d_t" && device_info->image3d_writes) ||
          (image_type == "image2d_array_t" &&
           device_info->image2d_array_writes)) {
        add_argument(kernel_decl, {"write_only", image_type, "woi"});
      }
      kernel_decl += get_spaces() + ')' + get_spaces();
      builtin_kernels[kernel_decl] = nop_builtin_kernel;
    }

    std::string kernel_decl(get_spaces() + "args_sampler_t" + get_spaces() +
                            '(');
    add_argument(kernel_decl, {"sampler_t", "s"});
    kernel_decl += get_spaces() + ')' + get_spaces();
    builtin_kernels[kernel_decl] = nop_builtin_kernel;
  }
#endif
  return builtin_kernels;
}
