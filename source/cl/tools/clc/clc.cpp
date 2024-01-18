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

#include "clc.h"

#include <cl/binary/binary.h>
#include <cl/binary/spirv.h>
#include <compiler/context.h>
#include <compiler/info.h>
#include <compiler/module.h>
#include <compiler/target.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "cargo/argument_parser.h"
#include "cargo/small_vector.h"
#include "cargo/string_algorithm.h"

#define CLC_CHECK_CL(EXPRESSION, MESSAGE)                                \
  {                                                                      \
    cl_int result = EXPRESSION;                                          \
    if (result != CL_SUCCESS) {                                          \
      (void)std::fprintf(stderr, "error: %s (%s, %d)\n", MESSAGE,        \
                         clc::cl_error_code_to_name_map[result].c_str(), \
                         (result));                                      \
      return clc::result::failure;                                       \
    }                                                                    \
  }                                                                      \
  (void)0

int main(int argc, char **argv) {
  clc::driver driver;
  if (auto error = driver.parseArguments(argc, argv)) {
    return error;
  }
  if (auto error = driver.setupContext()) {
    return error;
  }
  if (auto error = driver.buildProgram()) {
    return error;
  }
  if (auto error = driver.saveBinary()) {
    return error;
  }
  return clc::success;
}

namespace clc {

void mux_message(const char *message, const void *, size_t) {
  (void)std::fprintf(stderr, "%s", message);
}

std::map<cl_int, std::string> cl_error_code_to_name_map = {
    {0, "CL_SUCCESS"},
    {-1, "CL_DEVICE_NOT_FOUND"},
    {-2, "CL_DEVICE_NOT_AVAILABLE"},
    {-3, "CL_COMPILER_NOT_AVAILABLE"},
    {-4, "CL_MEM_OBJECT_ALLOCATION_FAILURE"},
    {-5, "CL_OUT_OF_RESOURCES"},
    {-6, "CL_OUT_OF_HOST_MEMORY"},
    {-7, "CL_PROFILING_INFO_NOT_AVAILABLE"},
    {-8, "CL_MEM_COPY_OVERLAP"},
    {-9, "CL_IMAGE_FORMAT_MISMATCH"},
    {-10, "CL_IMAGE_FORMAT_NOT_SUPPORTED"},
    {-11, "CL_BUILD_PROGRAM_FAILURE"},
    {-12, "CL_MAP_FAILURE"},
    {-13, "CL_MISALIGNED_SUB_BUFFER_OFFSET"},
    {-14, "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST"},
    {-15, "CL_COMPILE_PROGRAM_FAILURE"},
    {-16, "CL_LINKER_NOT_AVAILABLE"},
    {-17, "CL_LINK_PROGRAM_FAILURE"},
    {-18, "CL_DEVICE_PARTITION_FAILED"},
    {-19, "CL_KERNEL_ARG_INFO_NOT_AVAILABLE"},
    {-30, "CL_INVALID_VALUE"},
    {-31, "CL_INVALID_DEVICE_TYPE"},
    {-32, "CL_INVALID_PLATFORM"},
    {-33, "CL_INVALID_DEVICE"},
    {-34, "CL_INVALID_CONTEXT"},
    {-35, "CL_INVALID_QUEUE_PROPERTIES"},
    {-36, "CL_INVALID_COMMAND_QUEUE"},
    {-37, "CL_INVALID_HOST_PTR"},
    {-38, "CL_INVALID_MEM_OBJECT"},
    {-39, "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR"},
    {-40, "CL_INVALID_IMAGE_SIZE"},
    {-41, "CL_INVALID_SAMPLER"},
    {-42, "CL_INVALID_BINARY"},
    {-43, "CL_INVALID_BUILD_OPTIONS"},
    {-44, "CL_INVALID_PROGRAM"},
    {-45, "CL_INVALID_PROGRAM_EXECUTABLE"},
    {-46, "CL_INVALID_KERNEL_NAME"},
    {-47, "CL_INVALID_KERNEL_DEFINITION"},
    {-48, "CL_INVALID_KERNEL"},
    {-49, "CL_INVALID_ARG_INDEX"},
    {-50, "CL_INVALID_ARG_VALUE"},
    {-51, "CL_INVALID_ARG_SIZE"},
    {-52, "CL_INVALID_KERNEL_ARGS"},
    {-53, "CL_INVALID_WORK_DIMENSION"},
    {-54, "CL_INVALID_WORK_GROUP_SIZE"},
    {-55, "CL_INVALID_WORK_ITEM_SIZE"},
    {-56, "CL_INVALID_GLOBAL_OFFSET"},
    {-57, "CL_INVALID_EVENT_WAIT_LIST"},
    {-58, "CL_INVALID_EVENT"},
    {-59, "CL_INVALID_OPERATION"},
    {-60, "CL_INVALID_GL_OBJECT"},
    {-61, "CL_INVALID_BUFFER_SIZE"},
    {-62, "CL_INVALID_MIP_LEVEL"},
    {-63, "CL_INVALID_GLOBAL_WORK_SIZE"},
    {-64, "CL_INVALID_PROPERTY"},
    {-65, "CL_INVALID_IMAGE_DESCRIPTOR"},
    {-66, "CL_INVALID_COMPILER_OPTIONS"},
    {-67, "CL_INVALID_LINKER_OPTIONS"},
    {-68, "CL_INVALID_DEVICE_PARTITION_COUNT"},
    {-69, "CL_INVALID_PIPE_SIZE"},
    {-70, "CL_INVALID_DEVICE_QUEUE"},
};

bool matchSubstring(cargo::string_view big_string, cargo::string_view filter) {
  return big_string.find(filter) != cargo::string_view::npos;
}

result printMuxCompilers(cargo::array_view<const compiler::Info *> compilers) {
  for (cl_uint i = 0; i < compilers.size(); i++) {
    std::fprintf(stderr, "device %u: %s\n", i + 1,
                 compilers[i]->device_info->device_name);
  }
  return result::success;
}

driver::driver()
    : verbose(false),
      dry_run(false),
      input_file(),
      output_file(""),
      device_name_substring(""),
      cl_build_args(),
      strip_binary_header(false),
      compiler_info(nullptr),
      context(nullptr),
      module(nullptr) {}

#define CL_STD_CHOICES "{CL1.1,CL1.2,CL3.0}\n                        "

const char *CLC_USAGE =
    R"(usage: %s [options] [--] [<input>]

An OpenCL C 1.2 and SPIR-V 1.0 compiler to generate machine code for the
specified OpenCL device, the resulting offline binaries can be passed to the
OpenCL driver to completely bypass online compilation stages at runtime.

positional arguments:
  <input>               the input file e.g. kernel.cl or spirv.spv
                        the default value "-" specifies input should be read
                        from standard input. Only one input file is accepted.

Any options not defined below will be passed directly to clBuildProgram.

optional arguments:
  -h, --help            show this message and exit
  --version             show program's version number and exit
  -v, --verbose         show more information during execution
  -n, --no-output       suppresses generation of the output file, but runs the
                        rest of the compilation process
  -o file, --output file
                        output file path, defaults to the name of the last
                        input file "<input>.bin" if present, or "-" otherwise
                        to write to standard output
  -d name, --device name
                        a substring of the device name to select, choose from:%s
  --list-devices        print the list of available devices and exit
  -X opt                passes an option directly to the OpenCL compiler
  --strip-binary-header strips the header containing argument and kernel count
                        information, leaving only the binary directly from the
                        target implementation. WARNING: The output binary cannot
                        be loaded by the ComputeAorta runtime again!

optional preprocessor arguments:
  -D name               predefine name as a macro, with definition 1
  -D name=definition    the contents of definition are tokenized and processed
                        as if they appeared during translation phase three in a
                        `#define' directive. In particular, the definition will
                        be truncated by embedded newline characters
  -I dir                adds a directory to the list to be searched for headers

optional math intrinsics arguments:
  -cl-single-precision-constant
                        treat double precision floating-point constants as
                        single precision constants
  -cl-denorms-are-zero  allows flushes of denormalized numbers to zero for
                        optimization

optional optimization arguments:
  -cl-opt-disable       this option disables all optimizations
  -cl-mad-enable        allow a * b + c to be replaced by a mad with reduced
                        accuracy
  -cl-no-signed-zeros   allow optimizations for floating-point arithmetic that
                        ignore the signedness of zero
  -cl-unsafe-math-optimizations
                        allow optimizations for floating-point arithmetic that
                        may violate IEEE 754
  -cl-finite-math-only  allow optimizations for floating-point arithmetic that
                        assume that arguments and results are not NaNs or
                        +/-inf
  -cl-fast-relaxed-math sets -cl-finite-math-only and
                        -cl-unsafe-math-optimizations

optional additional arguments:
  -w                    disables the OpenCL warnings
  -Werror               makes the OpenCL warnings into errors
  -cl-std=)" CL_STD_CHOICES R"(determine the OpenCL C language version to use
  -cl-kernel-arg-info   this option allows the compiler to store
                        information for clGetKernelArgInfo

optional ComputeAorta extended arguments:
  -codeplay-soft-math   inhibit use of LLVM intrinsics for mathematical builtins
  -g                    enables generation of debug information, for best
                        results use in combination with -S
  -S file               Point debug information to a source file on disk. If
                        this does not exist, the runtime creates the file with
                        cached source.
  -cl-llvm-stats        enable reporting LLVM statistics
  -cl-wfv={always,auto,never}
                        sets whole function vectorization mode
  -cl-vec={none|loop|slp|all}
                        enables kernel early vectorization passes
)";

result driver::parseArguments(int argc, char **argv) {
  using parse = cargo::argument::parse;
  cargo::argument_parser<24, 4, 12> parser(
      cargo::argument_parser_option::ACCEPT_POSITIONAL |
      cargo::argument_parser_option::KEEP_UNRECOGNIZED);

#define CHECK(RESULT)                                                       \
  if (RESULT) {                                                             \
    (void)std::fprintf(stderr,                                              \
                       "error: failed to parse command line arguments.\n"); \
    return result::failure;                                                 \
  }                                                                         \
  (void)0

  bool show_help = false;
  CHECK(parser.add_argument({"-h", show_help}));
  CHECK(parser.add_argument({"--help", show_help}));

  bool show_version = false;
  CHECK(parser.add_argument({"--version", show_version}));

  CHECK(parser.add_argument({"-v", verbose}));
  CHECK(parser.add_argument({"--verbose", verbose}));

  CHECK(parser.add_argument({"-n", dry_run}));
  CHECK(parser.add_argument({"--no-output", dry_run}));

  CHECK(parser.add_argument({"-o", output_file}));
  CHECK(parser.add_argument({"--output", output_file}));

  CHECK(parser.add_argument({"-d", device_name_substring}));
  CHECK(parser.add_argument({"--device", device_name_substring}));

  bool list_devices = false;
  CHECK(parser.add_argument({"--list-devices", list_devices}));

  CHECK(parser.add_argument({"--strip-binary-header", strip_binary_header}));

  // Custom handler for clBuildProgram arguments which appends the command line
  // value to the last clBuildProgram argument, always runs just after the
  // option itself has been appended.
  auto append_to_last_arg = [&](cargo::string_view val) {
    auto &arg = cl_build_args.back();
    arg.insert(arg.end(), val.begin(), val.end());
    return parse::COMPLETE;
  };

  CHECK(parser.add_argument({"-D",
                             [&](cargo::string_view) {
                               cl_build_args.push_back("-D");
                               return parse::INCOMPLETE;
                             },
                             append_to_last_arg}));

  CHECK(parser.add_argument({"-I",
                             [&](cargo::string_view) {
                               cl_build_args.push_back("-I");
                               return parse::INCOMPLETE;
                             },
                             append_to_last_arg}));
  CHECK(parser.add_argument({"-S",
                             [&](cargo::string_view) {
                               cl_build_args.push_back("-S");
                               return parse::INCOMPLETE;
                             },
                             append_to_last_arg}));
  CHECK(parser.add_argument({"-x",
                             [&](cargo::string_view) {
                               cl_build_args.push_back("-x");
                               return parse::INCOMPLETE;
                             },
                             append_to_last_arg}));

  CHECK(parser.add_argument({"-X",
                             [&](cargo::string_view) {
                               cl_build_args.push_back(" ");
                               return parse::INCOMPLETE;
                             },
                             append_to_last_arg}));

  struct custom_device_option final {
    bool takes_value;
    std::string name;
    std::string help;

    custom_device_option(cargo::string_view name_view,
                         cargo::string_view help_view, bool takes_value)
        : takes_value(takes_value),
          name(name_view.data(), name_view.size()),
          help(help_view.data(), help_view.size()){};
  };

  std::map<const compiler::Info *, std::vector<custom_device_option>>
      custom_option_map;

  auto compilers = compiler::compilers();
  if (!compilers.empty()) {
    const auto empty_value_parser = [](cargo::string_view) {
      return parse::NOT_FOUND;
    };

    const auto takes_value_parser = [this](cargo::string_view val) {
      if (val.starts_with("-")) {
        // Value for option shouldn't start with '-', suggests we've started
        // parsing the next argument.
        return cargo::argument::parse::INVALID;
      }
      auto &arg = cl_build_args.back();
      arg.insert(arg.end(), val.begin(), val.end());
      return parse::COMPLETE;
    };

    const auto arg_name_parser = [this](cargo::string_view argument,
                                        bool takes_value,
                                        cargo::string_view arg_name) {
      if (!takes_value && (argument != arg_name)) {
        // Flags must match exactly rather than being a substring
        return parse::INVALID;
      }
      cl_build_args.emplace_back(arg_name.data(), arg_name.size());
      return takes_value ? parse::INCOMPLETE : parse::COMPLETE;
    };

    for (auto compiler : compilers) {
      auto split_options = cargo::split(compiler->compilation_options, ";");

      std::vector<custom_device_option> device_opts;
      device_opts.reserve(split_options.size());

      for (auto option : split_options) {
        const auto tuple = cargo::split_all(option, ",");

        const cargo::string_view name = tuple[0];
        const bool takes_value = tuple[1][0] == '1';
        device_opts.emplace_back(
            custom_device_option(name, tuple[2], takes_value));

        cargo::argument::custom_handler_function name_parser = std::bind(
            arg_name_parser, std::placeholders::_1, takes_value, name);

        cargo::argument::custom_handler_function value_parser =
            takes_value
                ? cargo::argument::custom_handler_function(takes_value_parser)
                : cargo::argument::custom_handler_function(empty_value_parser);

        CHECK(parser.add_argument(
            {name, std::move(name_parser), std::move(value_parser)}));
      }
      custom_option_map.insert({compiler, std::move(device_opts)});
    }
  }

  CHECK(parser.parse_args(argc, argv));

#undef CHECK

  if (show_help) {
    std::string device_names;
    std::string device_options;
    for (const auto &pair : custom_option_map) {
      const auto name = pair.first->device_info->device_name;
      device_names += "\n                        \"";
      device_names += name;
      device_names += '"';

      device_options += name;
      device_options += " device specific options:\n";

      for (const auto &option : pair.second) {
        device_options += "  ";
        device_options += option.name;

        // Calculate length of printed option
        size_t name_len = option.name.length() + 2;
        if (option.takes_value) {
          device_options += " value";
          name_len += 6;  // strlen(" value")
        }

        // Max width of option message before line break needed for help
        const size_t help_indent = 24;
        if (name_len < help_indent) {
          device_options += std::string(help_indent - name_len, ' ');
        } else {
          device_options.push_back('\n');
          device_options += std::string(help_indent, ' ');
        }
        device_options += option.help;
        device_options.push_back('\n');
      }
      device_options.push_back('\n');
    }

    std::printf(CLC_USAGE, (argc > 0) ? argv[0] : "clc", device_names.c_str());
    if (!device_options.empty()) {
      std::printf("\n%s\n", device_options.c_str());
    }

    std::exit(0);
  }

  if (show_version) {
    std::printf("%s %s (LLVM %s)\n", (argc > 0) ? argv[0] : "clc", CLC_VERSION,
                CLC_LLVM_VERSION);
    std::exit(0);
  }

  if (list_devices) {
    for (auto compiler : compiler::compilers()) {
      std::printf("%s\n", compiler->device_info->device_name);
    }
    std::exit(0);
  }

  const auto &pos_args = parser.get_positional_args();
  if (pos_args.size() > 1) {
    (void)std::fprintf(stderr,
                       "error: more than one input file is not supported\n");
    return result::failure;
  } else if (pos_args.empty()) {
    input_file = "-";
  } else {
    const auto &file = pos_args.front();
    input_file = std::string(file.data(), file.length());
  }

  for (auto arg : parser.get_unrecognized_args()) {
    cl_build_args.push_back(std::string(arg.data(), arg.length()));
  }

  generated_output_file = std::string(output_file.data(), output_file.length());
  if (output_file.empty()) {
    if (input_file != "-") {
      output_file = input_file;
      const size_t lastdot = output_file.find_last_of('.');
      if (lastdot != cargo::string_view::npos) {
        generated_output_file =
            std::string(output_file.data(), lastdot) + ".bin";
        output_file = generated_output_file;
      }
    } else {
      output_file = "-";
    }
  }

  return result::success;
}

result driver::setupContext() {
  if (findDevice() == result::failure) {
    return result::failure;
  }

  context = compiler::createContext();
  if (!context) {
    (void)std::fprintf(stderr, "error: Could not create compiler context\n");
    return result::failure;
  }

  compiler_target = compiler_info->createTarget(context.get(), mux_message);
  if (!compiler_target ||
      compiler_target->init(cl::binary::detectBuiltinCapabilities(
          compiler_info->device_info)) != compiler::Result::SUCCESS) {
    (void)std::fprintf(stderr, "error: Could not create compiler target\n");
    return result::failure;
  }

  return result::success;
}

result ReadWholeFile(std::istream &fp, std::vector<char> &output) {
  fp.seekg(0, std::ios_base::end);

  if (!fp.fail()) {
    const long file_length = fp.tellg();
    if (file_length < 0) {
      (void)std::fprintf(stderr,
                         "error: Could not determine input file size\n");
      return result::failure;
    }
    output.reserve(output.size() + file_length);
  }
  fp.seekg(0, std::ios_base::beg);
  fp.setstate(std::ios_base::goodbit);

  std::copy(std::istreambuf_iterator<char>(fp),
            std::istreambuf_iterator<char>(), std::back_inserter(output));

  return result::success;
}

result driver::buildProgram() {
  module_log.clear();
  uint32_t module_num_errors = 0;
  module = compiler_target->createModule(module_num_errors, module_log);
  if (!module) {
    (void)std::fprintf(stderr, "error: Could not create compiler module\n");
    return result::failure;
  }

  std::vector<char> source_vec;
  {
    const bool use_stdin = (input_file == "-");
    std::ifstream file_instream;
    if (!use_stdin) {
      file_instream.open(input_file, std::ios_base::binary | std::ios_base::in);
      if (file_instream.fail()) {
        (void)std::fprintf(stderr, "error: Could not open input file %s\n",
                           input_file.c_str());
        return result::failure;
      }
      module->getOptions().source_file_in.assign(input_file.data(),
                                                 input_file.size());
    } else {
      module->getOptions().source_file_in = "[stdin]";
    }
    std::istream &fp = use_stdin ? std::cin : file_instream;

    if (ReadWholeFile(fp, source_vec) == result::failure) {
      return result::failure;
    }
  }

  // LLVM requires the buffer to be null-terminated.
  source_vec.push_back('\0');
  // But the null-terminator should not be the part of the passed string_view.
  auto source_as_string =
      cargo::string_view(source_vec.data(), source_vec.size() - 1);
  auto source_as_spirv = cargo::array_view<const uint32_t>(
      reinterpret_cast<const uint32_t *>(source_vec.data()),
      source_vec.size() / sizeof(uint32_t));
  input_type source_type = input_type::opencl_c;
  if (context->isValidSPIRV(source_as_spirv)) {
    source_type = input_type::spirv;
  }

  if (verbose) {
    const char *source_type_name;
    switch (source_type) {
      case input_type::spirv:
        source_type_name = "SPIR-V";
        break;
      case input_type::opencl_c:
        [[fallthrough]];
      default:
        source_type_name = "OpenCL C";
        break;
    }
    std::fprintf(stderr, "info: Input file detected to be in %s format\n",
                 source_type_name);
  }

  std::vector<char> cl_options;
  for (auto opt : cl_build_args) {
    cl_options.push_back('\'');
    cl_options.insert(cl_options.end(), opt.begin(), opt.end());
    cl_options.push_back('\'');
    cl_options.push_back(' ');
  }
  cl_options.push_back('\0');

  if (verbose) {
    std::fprintf(stderr, "info: Compilation options: %.*s\n",
                 static_cast<int>(cl_options.size() - 1), cl_options.data());
  }

  const cargo::string_view device_profile =
      cl::binary::detectMuxDeviceProfile(CL_TRUE, compiler_info->device_info);
  compiler::Result errcode = compiler::Result::SUCCESS;
  switch (source_type) {
    case input_type::spirv: {
      auto spv_device_info = cl::binary::getSPIRVDeviceInfo(
          compiler_info->device_info, device_profile);
      auto result = module->compileSPIRV(source_as_spirv, *spv_device_info,
                                         cargo::nullopt);
      if (!result) {
        errcode = result.error();
      }
    } break;
    case input_type::opencl_c:
      [[fallthrough]];
    default: {
      if (module->parseOptions({cl_options.data(), cl_options.size() - 1},
                               compiler::Options::Mode::BUILD) !=
          compiler::Result::SUCCESS) {
        (void)std::fprintf(
            stderr, "error: Could not parse compiler options:\n%.*s\n",
            static_cast<int>(cl_options.size() - 1), cl_options.data());
        return result::failure;
      }
      errcode = module->compileOpenCLC(device_profile, source_as_string, {});
    }
  }

  if (errcode == compiler::Result::BUILD_PROGRAM_FAILURE) {
    ++module_num_errors;  // ensure we fail
    errcode = compiler::Result::SUCCESS;
  }
  if (compiler::Result::SUCCESS != errcode) {
    if (!module_log.empty() && module_log.front() != '\0') {
      (void)std::fprintf(stderr, "%.*s", static_cast<int>(module_log.size()),
                         module_log.data());
      return result::failure;
    }
    (void)std::fprintf(
        stderr, "Failed to build the program, no build log available.\n");
    return result::failure;
  }

  // Linking not needed when a single program is built.
  if (compiler::Result::SUCCESS !=
      module->finalize(&program_info, printf_calls)) {
    if (!module_log.empty() && module_log.front() != '\0') {
      std::fprintf(stderr, "%.*s", static_cast<int>(module_log.size()),
                   module_log.data());
      return result::failure;
    }
    std::fprintf(stderr, "Unknown compilation error in 'finalize'.\n");
    return result::failure;
  }

  if (verbose) {
    std::fprintf(stderr, "info: Build successful\n");
  }

  return result::success;
}

result driver::saveBinary() {
  cargo::dynamic_array<uint8_t> binary_storage;
  cargo::array_view<uint8_t> binary;

  // Generate the binary without the OpenCL header.
  cargo::array_view<uint8_t> module_executable;
  if (compiler::Result::SUCCESS != module->createBinary(module_executable)) {
    if (!module_log.empty() && module_log.front() != '\0') {
      std::fprintf(stderr, "%.*s", static_cast<int>(module_log.size()),
                   module_log.data());
    } else {
      (void)std::fprintf(stderr,
                         "Unknown compilation error in 'createBinary'.\n");
    }
    return result::failure;
  }

  if (strip_binary_header) {
    binary = module_executable;
  } else {
    if (!cl::binary::serializeBinary(
            binary_storage, module_executable, printf_calls, program_info,
            module->getOptions().kernel_arg_info, nullptr)) {
      (void)std::fprintf(stderr, "Failed to serialize binary");
      return result::failure;
    }
    binary = binary_storage;
  }

  if (output_file == "-") {
    if (!dry_run) {
      if (fwrite(binary.data(), 1, binary.size(), stdout) != binary.size()) {
        perror("error: Could not write all of the binary to the output file");
        return result::failure;
      }
      fflush(stdout);
    }
  } else {
    if (verbose) {
      std::fprintf(stderr, "info: writing binary to %s\n",
                   generated_output_file.c_str());
    }
    if (!dry_run) {
      FILE *fp = fopen(generated_output_file.c_str(), "wb");
      if (fp == nullptr) {
        perror("error: Could not open output file");
        return result::failure;
      }
      if (fwrite(binary.data(), 1, binary.size(), fp) != binary.size()) {
        perror("error: Could not write all of the binary to the output file");
        fclose(fp);
        return result::failure;
      }
      fclose(fp);
    }
  }

  return result::success;
}

result driver::findDevice() {
  auto compilers = compiler::compilers();
  if (compilers.empty()) {
    (void)std::fprintf(stderr, "error: no compilers found\n");
    return result::failure;
  }

  if (compilers.size() > 1 && device_name_substring.empty()) {
    (void)std::fprintf(stderr,
                       "error: Multiple devices available, please choose one "
                       "(--device NAME):\n");
    printMuxCompilers(compilers);
    return result::failure;
  }

  bool found = false;
  for (auto compiler : compilers) {
    bool matches = true;
    if (!device_name_substring.empty()) {
      matches &= matchSubstring(compiler->device_info->device_name,
                                device_name_substring);
    }
    if (matches) {
      if (found) {
        (void)std::fprintf(
            stderr, "error: Device selection ambiguous, available devices:\n");
        printMuxCompilers(compilers);
        return result::failure;
      }
      found = true;
      compiler_info = compiler;
    }
  }
  if (!found) {
    (void)std::fprintf(
        stderr,
        "error: No device matched the given substring, available devices:\n");
    printMuxCompilers(compilers);
    return result::failure;
  }

  if (verbose) {
    std::fprintf(stderr, "info: Using device %s\n",
                 compiler_info->device_info->device_name);
  }

  return result::success;
}

}  // namespace clc
