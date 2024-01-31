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
#include <cargo/argument_parser.h>
#include <cargo/small_vector.h>
#include <cargo/string_view.h>
#include <cl/binary/binary.h>
#include <cl/binary/program_info.h>
#include <cl/macros.h>
#include <compiler/limits.h>
#include <compiler/loader.h>
#include <metadata/detail/md_ctx.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <unordered_set>

#include "mux/utils/helpers.h"

namespace cl {
namespace binary {

namespace {

auto md_init_unique(md_hooks *hooks, void *userdata) {
  auto deleter = [](md_ctx ctx) {
    if (ctx) {
      md_release_ctx(ctx);
    }
  };
  auto ctx = md_init(hooks, userdata);
  return std::unique_ptr<md_ctx_, decltype(deleter)>(ctx, std::move(deleter));
}

bool serializeExecutable(OpenCLWriteUserdata *cl_userdata, md_ctx ctx) {
  md_stack stack = md_create_block(ctx, OCL_MD_EXECUTABLE_BLOCK);
  if (!stack) {
    return false;
  }

  if (cl_userdata->is_executable) {
    const int err = md_push_bytes(stack, cl_userdata->mux_executable.data(),
                                  cl_userdata->mux_executable.size());
    if (MD_CHECK_ERR(err)) {
      return false;
    }
  } else {
    cargo::dynamic_array<uint8_t> buf;
    auto *compiler_module = cl_userdata->compiler_module;
    if (buf.alloc(compiler_module->size())) {
      return false;
    }
    compiler_module->serialize(buf.data());
    const int err = md_push_bytes(stack, buf.data(), buf.size());
    if (MD_CHECK_ERR(err)) {
      return false;
    }
  }

  const int err = md_finalize_block(stack);
  if (MD_CHECK_ERR(err)) {
    return false;
  }
  return true;
}

bool serializeIsExecutable(md_ctx ctx, bool is_executable) {
  md_stack stack = md_create_block(ctx, OCL_MD_IS_EXECUTABLE_BLOCK);
  if (!stack) {
    return false;
  }
  int err = md_set_out_fmt(stack, md_fmt::MD_FMT_MSGPACK);
  if (MD_CHECK_ERR(err)) {
    return false;
  }

  err = md_push_uint(stack, is_executable ? 1 : 0);
  if (MD_CHECK_ERR(err)) {
    return false;
  }
  return true;
}

bool serializePrintfInfo(
    md_ctx ctx, const std::vector<builtins::printf::descriptor> &printf_calls) {
  md_stack stack = md_create_block(ctx, OCL_MD_PRINTF_INFO_BLOCK);
  if (!stack) {
    return false;
  }
  int err = md_set_out_fmt(stack, md_fmt::MD_FMT_MSGPACK);
  if (MD_CHECK_ERR(err)) {
    return false;
  }

  // Layout: [fmt_str, [types], [strings], ...]
  const int printf_arr_idx = md_push_array(stack, printf_calls.size());
  if (MD_CHECK_ERR(printf_arr_idx)) {
    return false;
  }

  for (const auto &printf_call : printf_calls) {
    const int fmt_str_idx =
        md_push_zstr(stack, printf_call.format_string.c_str());
    if (MD_CHECK_ERR(fmt_str_idx)) {
      return false;
    }
    err = md_array_append(stack, printf_arr_idx, fmt_str_idx);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    md_pop(stack);

    // Types
    const int type_arr_idx = md_push_array(stack, printf_call.types.size());
    if (MD_CHECK_ERR(type_arr_idx)) {
      return false;
    }
    for (const auto &type : printf_call.types) {
      const int type_idx = md_push_uint(stack, static_cast<uint64_t>(type));
      if (MD_CHECK_ERR(type_idx)) {
        return false;
      }
      err = md_array_append(stack, type_arr_idx, type_idx);
      if (MD_CHECK_ERR(err)) {
        return false;
      }
      md_pop(stack);
    }
    err = md_array_append(stack, printf_arr_idx, type_arr_idx);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    md_pop(stack);

    // strings
    const int string_arr_idx = md_push_array(stack, printf_call.strings.size());
    if (MD_CHECK_ERR(string_arr_idx)) {
      return false;
    }
    for (const auto &str : printf_call.strings) {
      const int str_idx = md_push_zstr(stack, str.c_str());
      if (MD_CHECK_ERR(str_idx)) {
        return false;
      }
      err = md_array_append(stack, string_arr_idx, str_idx);
      if (MD_CHECK_ERR(err)) {
        return false;
      }
      md_pop(stack);
    }
    err = md_array_append(stack, printf_arr_idx, string_arr_idx);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    md_pop(stack);
  }

  err = md_finalize_block(stack);
  if (MD_CHECK_ERR(err)) {
    return false;
  }
  return true;
}

bool serializeProgramInfo(md_ctx ctx, const compiler::ProgramInfo &program_info,
                          bool has_kernel_arg_info) {
  md_stack stack = md_create_block(ctx, OCL_MD_PROGRAM_INFO_BLOCK);
  if (!stack) {
    return false;
  }
  int err = md_set_out_fmt(stack, md_fmt::MD_FMT_MSGPACK);
  if (MD_CHECK_ERR(err)) {
    return false;
  }

  // Layout
  // [[kernel], [kernel], ...]
  // kernel -> [ n_args, has_full_md, [arg_info], [work_widths (x,y,z)],
  // kernel_name ]

  const int kernels_arr_idx =
      md_push_array(stack, program_info.getNumKernels());
  if (MD_CHECK_ERR(kernels_arr_idx)) {
    return false;
  }
  for (const auto &kernel : program_info) {
    const int indv_kernel_idx = md_push_array(stack, 5);
    if (MD_CHECK_ERR(indv_kernel_idx)) {
      return false;
    }

    // number of arguments
    const int n_args_idx = md_push_uint(stack, kernel.getNumArguments());
    if (MD_CHECK_ERR(n_args_idx)) {
      return false;
    }
    err = md_array_append(stack, indv_kernel_idx, n_args_idx);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    md_pop(stack);

    // Full metadata
    const bool store_arg_metadata = has_kernel_arg_info && kernel.argument_info;
    const int full_metadata_idx =
        md_push_uint(stack, store_arg_metadata ? 1 : 0);
    if (MD_CHECK_ERR(full_metadata_idx)) {
      return false;
    }
    err = md_array_append(stack, indv_kernel_idx, full_metadata_idx);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    md_pop(stack);

    // Arg Info
    const int kernel_arg_arr_idx =
        md_push_array(stack, kernel.getNumArguments());
    if (MD_CHECK_ERR(kernel_arg_arr_idx)) {
      return false;
    }
    for (size_t arg_idx = 0, e = kernel.getNumArguments(); arg_idx < e;
         ++arg_idx) {
      // argument kind
      const int arg_kind_idx = md_push_uint(
          stack, static_cast<uint64_t>(kernel.argument_types[arg_idx].kind));
      if (MD_CHECK_ERR(arg_kind_idx)) {
        return false;
      }
      err = md_array_append(stack, kernel_arg_arr_idx, arg_kind_idx);
      if (MD_CHECK_ERR(err)) {
        return false;
      }
      md_pop(stack);

      // address space
      const int arg_addr_space_idx = md_push_uint(
          stack,
          static_cast<uint64_t>(kernel.argument_types[arg_idx].address_space));
      if (MD_CHECK_ERR(arg_addr_space_idx)) {
        return false;
      }
      err = md_array_append(stack, kernel_arg_arr_idx, arg_addr_space_idx);
      if (MD_CHECK_ERR(err)) {
        return false;
      }
      md_pop(stack);

      if (store_arg_metadata) {
        const auto &arg_info = kernel.argument_info.value()[arg_idx];

        // address qualifier
        const int addr_qual_idx = md_push_uint(stack, arg_info.address_qual);
        if (MD_CHECK_ERR(addr_qual_idx)) {
          return false;
        }
        err = md_array_append(stack, kernel_arg_arr_idx, addr_qual_idx);
        if (MD_CHECK_ERR(err)) {
          return err;
        }
        md_pop(stack);

        // access qualifier
        const int access_qualifier =
            md_push_uint(stack, static_cast<uint64_t>(arg_info.access_qual));
        if (MD_CHECK_ERR(access_qualifier)) {
          return false;
        }
        err = md_array_append(stack, kernel_arg_arr_idx, access_qualifier);
        if (MD_CHECK_ERR(err)) {
          return err;
        }
        md_pop(stack);

        // arg type qualifier
        const int arg_type_qual_idx = md_push_uint(stack, arg_info.type_qual);
        if (MD_CHECK_ERR(arg_type_qual_idx)) {
          return false;
        }
        err = md_array_append(stack, kernel_arg_arr_idx, arg_type_qual_idx);
        if (MD_CHECK_ERR(err)) {
          return err;
        }
        md_pop(stack);

        // type name
        const int type_name_idx =
            md_push_zstr(stack, arg_info.type_name.c_str());
        if (MD_CHECK_ERR(type_name_idx)) {
          return false;
        }
        err = md_array_append(stack, kernel_arg_arr_idx, type_name_idx);
        if (MD_CHECK_ERR(err)) {
          return err;
        }
        md_pop(stack);

        // arg name
        const int arg_name_idx = md_push_zstr(stack, arg_info.name.c_str());
        if (MD_CHECK_ERR(arg_name_idx)) {
          return false;
        }
        err = md_array_append(stack, kernel_arg_arr_idx, arg_name_idx);
        if (MD_CHECK_ERR(err)) {
          return err;
        }
        md_pop(stack);
      }
    }

    err = md_array_append(stack, indv_kernel_idx, kernel_arg_arr_idx);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    md_pop(stack);

    // work group sizes
    const int work_size_arr_idx = md_push_array(stack, 3);
    if (MD_CHECK_ERR(work_size_arr_idx)) {
      return false;
    }
    for (size_t i = 0; i < 3; ++i) {
      const int work_size_idx =
          md_push_uint(stack, kernel.getReqdWGSizeOrZero()[i]);
      if (MD_CHECK_ERR(work_size_idx)) {
        return false;
      }
      err = md_array_append(stack, work_size_arr_idx, work_size_idx);
      if (MD_CHECK_ERR(err)) {
        return false;
      }
      md_pop(stack);
    }
    err = md_array_append(stack, indv_kernel_idx, work_size_arr_idx);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    md_pop(stack);

    // required sub-group size
    const int reqd_sub_group_size_idx =
        md_push_uint(stack, kernel.reqd_sub_group_size.value_or(0));
    if (MD_CHECK_ERR(reqd_sub_group_size_idx)) {
      return false;
    }
    err = md_array_append(stack, indv_kernel_idx, reqd_sub_group_size_idx);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    md_pop(stack);

    // kernel name
    const int kernel_name_idx = md_push_zstr(stack, kernel.name.c_str());
    if (MD_CHECK_ERR(kernel_name_idx)) {
      return false;
    }
    err = md_array_append(stack, indv_kernel_idx, kernel_name_idx);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    md_pop(stack);

    // add the kernel to the array
    err = md_array_append(stack, kernels_arr_idx, indv_kernel_idx);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    md_pop(stack);
  }
  err = md_finalize_block(stack);
  if (MD_CHECK_ERR(err)) {
    return false;
  }
  return true;
}

bool deserializeExecutable(md_ctx ctx,
                           cargo::dynamic_array<uint8_t> &executable) {
  md_stack executable_stack = md_get_block(ctx, OCL_MD_EXECUTABLE_BLOCK);
  if (!executable_stack) {
    return false;
  }
  size_t exec_len;
  char *exec_bytes;
  const int err = md_loadf(executable_stack, "s", &exec_len, &exec_bytes);
  if (MD_CHECK_ERR(err)) {
    return false;
  }
  if (executable.alloc(exec_len)) {
    return false;
  }
  std::memcpy(executable.data(), exec_bytes, exec_len);
  std::free(exec_bytes);
  return true;
}

cargo::expected<bool, int> deserializeIsExecutable(md_ctx ctx) {
  md_stack stack = md_get_block(ctx, OCL_MD_IS_EXECUTABLE_BLOCK);
  if (!stack) {
    return cargo::make_unexpected(md_err::MD_E_STACK_CORRUPT);
  }
  uint64_t is_executable_as_int;
  const int err = md_loadf(stack, "u", &is_executable_as_int);
  if (MD_CHECK_ERR(err)) {
    return cargo::make_unexpected(md_err::MD_E_STACK_CORRUPT);
  }
  return is_executable_as_int == 1;
}

bool deserializeOpenCLPrintfCalls(
    md_ctx ctx, std::vector<builtins::printf::descriptor> &printf_calls) {
  md_stack stack = md_get_block(ctx, OCL_MD_PRINTF_INFO_BLOCK);
  if (!stack) {
    return false;
  }

  md_value printf_arr = md_get_value(stack, 0);
  const int printf_size = md_get_array_size(printf_arr);
  if (MD_CHECK_ERR(printf_size)) {
    return false;
  }
  size_t printf_arr_idx = 0;
  while (printf_arr_idx < static_cast<size_t>(printf_size)) {
    builtins::printf::descriptor descriptor;

    /// --- FORMAT STRING --- ///
    md_value fmt_str_v;
    int err = md_get_array_idx(printf_arr, printf_arr_idx, &fmt_str_v);
    if (MD_CHECK_ERR(err)) {
      return false;
    }

    char *fmt_str;
    size_t fmt_str_len;
    err = md_get_zstr(fmt_str_v, &fmt_str, &fmt_str_len);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    descriptor.format_string = std::string(fmt_str, fmt_str_len);
    std::free(fmt_str);
    ++printf_arr_idx;

    /// --- TYPES --- ///
    md_value types_arr;
    err = md_get_array_idx(printf_arr, printf_arr_idx, &types_arr);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    const int types_arr_len = md_get_array_size(types_arr);
    if (MD_CHECK_ERR(types_arr_len)) {
      return false;
    }
    for (size_t j = 0; j < static_cast<size_t>(types_arr_len); ++j) {
      md_value type_v;
      err = md_get_array_idx(types_arr, j, &type_v);
      if (MD_CHECK_ERR(err)) {
        return false;
      }
      uint64_t type;
      err = md_get_uint(type_v, &type);
      if (MD_CHECK_ERR(err)) {
        return false;
      }
      descriptor.types.push_back(static_cast<builtins::printf::type>(type));
    }
    ++printf_arr_idx;

    /// --- STRINGS --- ///
    md_value strings_arr;
    err = md_get_array_idx(printf_arr, printf_arr_idx, &strings_arr);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    const int strings_arr_len = md_get_array_size(strings_arr);
    if (MD_CHECK_ERR(strings_arr_len)) {
      return false;
    }
    for (size_t strings_idx = 0;
         strings_idx < static_cast<size_t>(strings_arr_len); ++strings_idx) {
      md_value str_v;
      err = md_get_array_idx(strings_arr, strings_idx, &str_v);
      if (MD_CHECK_ERR(err)) {
        return false;
      }
      char *str;
      size_t str_len;
      err = md_get_zstr(str_v, &str, &str_len);
      if (MD_CHECK_ERR(err)) {
        return false;
      }
      descriptor.strings.emplace_back(std::string(str, str_len));
      std::free(str);
    }
    ++printf_arr_idx;

    printf_calls.emplace_back(std::move(descriptor));
  }
  return true;
}

bool deserializeOpenCLProgramInfo(md_ctx ctx,
                                  compiler::ProgramInfo &program_info) {
  md_stack stack = md_get_block(ctx, OCL_MD_PROGRAM_INFO_BLOCK);
  if (!stack) {
    return false;
  }

  md_value kernels_v = md_get_value(stack, 0);
  if (!kernels_v) {
    return false;
  }

  const int n_kernels = md_get_array_size(kernels_v);
  if (MD_CHECK_ERR(n_kernels)) {
    return false;
  }

  if (!program_info.resizeFromNumKernels(static_cast<uint32_t>(n_kernels))) {
    return false;
  }

  for (size_t kernel_idx = 0; kernel_idx < static_cast<size_t>(n_kernels);
       ++kernel_idx) {
    auto *kernelInfo = program_info.getKernel(kernel_idx);

    // get the kernel info value
    md_value kernel_info_v;
    int err = md_get_array_idx(kernels_v, kernel_idx, &kernel_info_v);
    if (MD_CHECK_ERR(err)) {
      return false;
    }

    // number of arguments
    md_value n_args_v;
    err = md_get_array_idx(kernel_info_v, 0, &n_args_v);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    uint64_t numArguments;
    err = md_get_uint(n_args_v, &numArguments);
    if (MD_CHECK_ERR(err)) {
      return false;
    }

    // has full metadata?
    md_value full_metadata_v;
    err = md_get_array_idx(kernel_info_v, 1, &full_metadata_v);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    uint64_t full_metadata_int;
    err = md_get_uint(full_metadata_v, &full_metadata_int);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    const bool hasArgMetadata = full_metadata_int == 1;
    if (hasArgMetadata) {
      kernelInfo->argument_info.emplace();
      if (kernelInfo->argument_info->resize(numArguments)) {
        return false;
      }
    }

    if (kernelInfo->argument_types.alloc(numArguments)) {
      return false;
    }

    // arg info array
    md_value arg_info_arr_v;
    err = md_get_array_idx(kernel_info_v, 2, &arg_info_arr_v);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    const int arg_info_array_len = md_get_array_size(arg_info_arr_v);
    if (MD_CHECK_ERR(arg_info_array_len)) {
      return false;
    }

    size_t cur_arg_array_idx = 0;
    size_t kernel_arg_idx = 0;
    while (cur_arg_array_idx < static_cast<size_t>(arg_info_array_len)) {
      // argument kind
      md_value arg_kind_v;
      err = md_get_array_idx(arg_info_arr_v, cur_arg_array_idx, &arg_kind_v);
      if (MD_CHECK_ERR(err)) {
        return false;
      }
      uint64_t arg_kind;
      err = md_get_uint(arg_kind_v, &arg_kind);
      if (MD_CHECK_ERR(err)) {
        return false;
      }
      kernelInfo->argument_types[kernel_arg_idx].kind =
          static_cast<compiler::ArgumentKind>(arg_kind);
      ++cur_arg_array_idx;

      // arg address space
      md_value arg_addr_space_v;
      err = md_get_array_idx(arg_info_arr_v, cur_arg_array_idx,
                             &arg_addr_space_v);
      if (MD_CHECK_ERR(err)) {
        return false;
      }
      uint64_t arg_addr_space;
      err = md_get_uint(arg_addr_space_v, &arg_addr_space);
      if (MD_CHECK_ERR(err)) {
        return false;
      }
      kernelInfo->argument_types[kernel_arg_idx].address_space =
          static_cast<compiler::AddressSpace>(arg_addr_space);
      ++cur_arg_array_idx;

      if (hasArgMetadata) {
        auto &arg_info = kernelInfo->argument_info.value()[kernel_arg_idx];

        // arg address qualifier
        md_value arg_address_qual_v;
        err = md_get_array_idx(arg_info_arr_v, cur_arg_array_idx,
                               &arg_address_qual_v);
        if (MD_CHECK_ERR(err)) {
          return false;
        }
        uint64_t arg_address_qual;
        err = md_get_uint(arg_address_qual_v, &arg_address_qual);
        if (MD_CHECK_ERR(err)) {
          return false;
        }
        arg_info.address_qual =
            static_cast<compiler::AddressSpace>(arg_address_qual);
        ++cur_arg_array_idx;

        // arg access qualifier
        md_value arg_access_qual_v;
        err = md_get_array_idx(arg_info_arr_v, cur_arg_array_idx,
                               &arg_access_qual_v);
        if (MD_CHECK_ERR(err)) {
          return false;
        }
        uint64_t arg_access_qual;
        err = md_get_uint(arg_access_qual_v, &arg_access_qual);
        if (MD_CHECK_ERR(err)) {
          return false;
        }
        arg_info.access_qual =
            static_cast<compiler::KernelArgAccess>(arg_access_qual);
        ++cur_arg_array_idx;

        // arg type qualifier
        md_value arg_type_qual_v;
        err = md_get_array_idx(arg_info_arr_v, cur_arg_array_idx,
                               &arg_type_qual_v);
        if (MD_CHECK_ERR(err)) {
          return false;
        }
        uint64_t arg_type_qual;
        err = md_get_uint(arg_type_qual_v, &arg_type_qual);
        if (MD_CHECK_ERR(err)) {
          return false;
        }
        arg_info.type_qual =
            static_cast<cl_kernel_arg_type_qualifier>(arg_type_qual);
        ++cur_arg_array_idx;

        // arg type name
        md_value arg_type_name_v;
        err = md_get_array_idx(arg_info_arr_v, cur_arg_array_idx,
                               &arg_type_name_v);
        if (MD_CHECK_ERR(err)) {
          return false;
        }
        char *type_name;
        size_t type_name_len;
        err = md_get_zstr(arg_type_name_v, &type_name, &type_name_len);
        if (MD_CHECK_ERR(err)) {
          return false;
        }
        arg_info.type_name = std::string(type_name, type_name_len);
        std::free(type_name);
        ++cur_arg_array_idx;

        // arg name
        md_value arg_name_v;
        err = md_get_array_idx(arg_info_arr_v, cur_arg_array_idx, &arg_name_v);
        if (MD_CHECK_ERR(err)) {
          return false;
        }
        char *arg_name;
        size_t arg_name_len;
        err = md_get_zstr(arg_name_v, &arg_name, &arg_name_len);
        if (MD_CHECK_ERR(err)) {
          return false;
        }
        arg_info.name = std::string(arg_name, arg_name_len);
        std::free(arg_name);
        ++cur_arg_array_idx;
      }
      ++kernel_arg_idx;
    }

    // work group sizes
    md_value work_size_arr_v;
    err = md_get_array_idx(kernel_info_v, 3, &work_size_arr_v);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    std::array<size_t, 3> reqd_wg_size;
    for (size_t j = 0; j < 3; ++j) {
      md_value work_size_v;
      err = md_get_array_idx(work_size_arr_v, j, &work_size_v);
      if (MD_CHECK_ERR(err)) {
        return false;
      }
      uint64_t work_size;
      err = md_get_uint(work_size_v, &work_size);
      if (MD_CHECK_ERR(err)) {
        return false;
      }
      reqd_wg_size[j] = work_size;
    }
    if (!std::all_of(reqd_wg_size.begin(), reqd_wg_size.end(),
                     [](size_t v) { return v == 0; })) {
      kernelInfo->reqd_work_group_size = reqd_wg_size;
    }

    // required sub-group size
    md_value work_size_v;
    err = md_get_array_idx(kernel_info_v, 4, &work_size_v);
    if (MD_CHECK_ERR(err)) {
      return false;
    }

    uint64_t reqd_sub_group_size;
    err = md_get_uint(work_size_v, &reqd_sub_group_size);
    if (MD_CHECK_ERR(err)) {
      return false;
    }

    if (reqd_sub_group_size) {
      kernelInfo->reqd_sub_group_size = reqd_sub_group_size;
    }

    // kernel name
    md_value kernel_name_v;
    err = md_get_array_idx(kernel_info_v, 5, &kernel_name_v);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    char *kernel_name;
    size_t kernel_name_len;
    err = md_get_zstr(kernel_name_v, &kernel_name, &kernel_name_len);
    if (MD_CHECK_ERR(err)) {
      return false;
    }
    kernelInfo->name = std::string(kernel_name, kernel_name_len);
    std::free(kernel_name);
  }
  return true;
}
}  // namespace

cargo::string_view detectMuxDeviceProfile(cl_bool compiler_available,
                                          mux_device_info_t device) {
  if (compiler_available == CL_FALSE) {
    return "EMBEDDED_PROFILE";
  }
  return mux::detectOpenCLProfile(device);
}

uint32_t detectBuiltinCapabilities(mux_device_info_t device_info) {
  uint32_t caps = 0;

  // Capabilities for doubles required for compliance
  // TODO: CA-882 Resolve how capabilities are checked
  const auto reqd_caps_fp64 = mux_floating_point_capabilities_denorm |
                              mux_floating_point_capabilities_inf_nan |
                              mux_floating_point_capabilities_rte |
                              mux_floating_point_capabilities_rtz |
                              mux_floating_point_capabilities_inf_nan |
                              mux_floating_point_capabilities_fma;

  // Capabilities for halfs required for compliance
  // TODO: CA-882 Resolve how capabilities are checked
  const auto reqd_caps_fp16_a = mux_floating_point_capabilities_rtz;
  const auto reqd_caps_fp16_b = mux_floating_point_capabilities_rte |
                                mux_floating_point_capabilities_inf_nan;

  // Bit width
  if ((device_info->address_capabilities & mux_address_capabilities_bits32) >
      0) {
    caps |= compiler::CAPS_32BIT;
  }

  // Doubles
  if ((device_info->double_capabilities & reqd_caps_fp64) == reqd_caps_fp64) {
    caps |= compiler::CAPS_FP64;
  }

  // Halfs
  if ((device_info->half_capabilities & reqd_caps_fp16_a) == reqd_caps_fp16_a ||
      (device_info->half_capabilities & reqd_caps_fp16_b) == reqd_caps_fp16_b) {
    caps |= compiler::CAPS_FP16;
  }

  return caps;
}

bool serializeBinary(
    cargo::dynamic_array<uint8_t> &binary,
    const cargo::array_view<const uint8_t> mux_binary,
    const std::vector<builtins::printf::descriptor> &printf_calls,
    const compiler::ProgramInfo &program_info, bool kernel_arg_info,
    compiler::Module *compiler_module) {
  const bool is_executable = compiler_module == nullptr;
  OpenCLWriteUserdata cl_userdata{&binary, mux_binary, is_executable,
                                  compiler_module};
  md_hooks cl_hooks = getOpenCLMetadataWriteHooks();

  auto ctx = md_init_unique(&cl_hooks, &cl_userdata);
  if (!ctx.get()) {
    return false;
  }

  if (!serializeExecutable(&cl_userdata, ctx.get())) {
    return false;
  }

  if (!serializeIsExecutable(ctx.get(), is_executable)) {
    return false;
  }

  if (is_executable) {
    if (!serializePrintfInfo(ctx.get(), printf_calls)) {
      return false;
    }

    if (!serializeProgramInfo(ctx.get(), program_info, kernel_arg_info)) {
      return false;
    }
  }

  md_finalize_ctx(ctx.get());
  return true;
}

bool deserializeBinary(cargo::array_view<const uint8_t> binary,
                       std::vector<builtins::printf::descriptor> &printf_calls,
                       compiler::ProgramInfo &program_info,
                       cargo::dynamic_array<uint8_t> &executable,
                       bool &is_executable) {
  OpenCLReadUserdata cl_userdata{binary};
  md_hooks cl_hooks = getOpenCLMetadataReadHooks();

  auto ctx = md_init_unique(&cl_hooks, &cl_userdata);
  if (!ctx.get()) {
    return false;
  }

  if (!deserializeExecutable(ctx.get(), executable)) {
    return false;
  }

  auto is_executable_result = deserializeIsExecutable(ctx.get());
  if (!is_executable_result.has_value()) {
    return false;
  }
  is_executable = is_executable_result.value();

  if (is_executable_result.value()) {
    if (!deserializeOpenCLPrintfCalls(ctx.get(), printf_calls)) {
      return false;
    }

    if (!deserializeOpenCLProgramInfo(ctx.get(), program_info)) {
      return false;
    }
  }

  return true;
}

md_hooks getOpenCLMetadataWriteHooks() {
  md_hooks cl_hooks{};
  cl_hooks.write = [](void *userdata, const void *data, size_t n) -> md_err {
    auto *cl_userdata = static_cast<OpenCLWriteUserdata *>(userdata);
    auto *binary = cl_userdata->binary_buffer;
    if (binary->alloc(n)) {
      return md_err::MD_E_OOM;
    }
    std::memcpy(binary->data(), data, n);
    return md_err::MD_SUCCESS;
  };
  cl_hooks.finalize = [](void *) {};
  return cl_hooks;
}

md_hooks getOpenCLMetadataReadHooks() {
  md_hooks cl_hooks{};
  cl_hooks.map = [](void *userdata, size_t *n) -> void * {
    auto *cl_userdata = static_cast<OpenCLReadUserdata *>(userdata);
    *n = cl_userdata->binary.size();
    return const_cast<uint8_t *>(cl_userdata->binary.data());
  };
  return cl_hooks;
}

}  // namespace binary
}  // namespace cl
