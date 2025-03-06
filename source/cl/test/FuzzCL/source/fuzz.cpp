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

#include <cstring>
#include <random>
#include <unordered_map>

#include "FuzzCL/arguments.h"
#include "FuzzCL/context.h"

namespace {
const std::string usage = R"(
usage:
    FuzzCL -c | --corpus path [options]
    FuzzCL -f | --file path [options]
    FuzzCL -h | --help

Required:
    -c, --corpus        Fuzz from a corpus located at path.
    -f, --file          Fuzz from a file located at path.

Options:
    -d, --device        Select a specific OpenCL device.
    --enable-callbacks  Enable event callbacks in FuzzCL
    -h, --help          Show this screen.
    -o, --output        Generate cpp files from OpenCL runtime calls to output.
    -v, --verbose       Print each runtime call to stdout
)";
struct arguments_t {
  std::string file;
  std::string corpus;
  std::string output;
  std::string device;
  bool verbose;
  bool callbacks;
};
}  // namespace
/// @brief Parse command line arguments into a dictionnary
///
/// @param[in] argc Number of arguments
/// @param[in] argv Argument array
///
/// @return Returns the parsed dictionnary
static arguments_t parse_arguments(int argc, char *argv[]) {
  cargo::argument_parser<1> parser;

  cargo::string_view file;
  fuzzcl::add_argument(parser, file, "-f", "--file");

  cargo::string_view corpus;
  fuzzcl::add_argument(parser, corpus, "-c", "--corpus");

  cargo::string_view output;
  fuzzcl::add_argument(parser, output, "-o", "--output");

  cargo::string_view device;
  fuzzcl::add_argument(parser, device, "-d", "--device");

  bool verbose = false;
  fuzzcl::add_argument(parser, verbose, "-v", "--verbose");

  bool callbacks = false;
  fuzzcl::add_argument(parser, callbacks, "--enable-callbacks");

  if (auto error = parser.parse_args(argc, argv)) {
    std::cerr << "error: invalid arguments : " << error;
    switch (error) {
      case cargo::success:
        std::cerr << " not_found";
        break;
      case cargo::bad_argument:
        std::cerr << " bad_argument";
        break;
      case cargo::bad_alloc:
        std::cerr << " bad_alloc";
        break;
      default:
        break;
    }
    std::cerr << usage;
    exit(1);
  }

  // -f and -c incompatible
  if (!file.empty() && !corpus.empty()) {
    std::cerr << usage;
    exit(1);
  }
  // if -o when not -f
  if (file.empty() && !output.empty()) {
    std::cerr << usage;
    exit(1);
  }
  // if no arguments where passed
  if (file.empty() && corpus.empty()) {
    std::cerr << usage;
    exit(1);
  }

  return {cargo::as<std::string>(file),
          cargo::as<std::string>(corpus),
          cargo::as<std::string>(output),
          cargo::as<std::string>(device),
          verbose,
          callbacks};
}

int main(int argc, char *argv[]) {
  arguments_t arguments = parse_arguments(argc, argv);

  std::vector<std::string> kernels;
  fuzzcl::list_dir(KERNEL_SOURCE_DIR, kernels);

  // this array will delete the kernel binaries at the end of the scope
  std::vector<std::vector<unsigned char>> kernel_binaries;
  kernel_binaries.reserve(kernels.size());

  // this array only contains pointers to each kernel binaries. It is used to go
  // from std::vector<std::vector<unsigned char>> to const unsigned char**
  // needed by OpenCL
  std::vector<const unsigned char *> kernel_binary_pointers;
  std::vector<size_t> kernel_binary_sizes;
  for (size_t i = 0; i < kernels.size(); i++) {
    kernel_binaries.emplace_back(
        fuzzcl::read_file<unsigned char>(KERNEL_SOURCE_DIR + kernels[i]));
    kernel_binary_pointers.emplace_back(kernel_binaries.back().data());
    kernel_binary_sizes.emplace_back(kernel_binaries.back().size() *
                                     sizeof(unsigned char));
  }
  const unsigned char **binaries = kernel_binary_pointers.data();

  if (!arguments.corpus.empty()) {
    if (arguments.corpus.back() != '/') {
      arguments.corpus += "/";
    }

    std::vector<std::string> files;
    fuzzcl::list_dir(arguments.corpus, files);

    for (const std::string &file : files) {
      // a flush is specifically needed so a python wrapper can get an
      // unbuffered output
      std::cout << file << '\n' << std::flush;
      std::vector<uint8_t> data =
          fuzzcl::read_file<uint8_t>(arguments.corpus + file);

      fuzzcl::fuzz_from_input(data.data(), data.size(), binaries,
                              kernel_binary_sizes,
                              {arguments.callbacks, arguments.verbose,
                               arguments.device, arguments.output});
    }
  } else if (!arguments.file.empty()) {
    std::vector<uint8_t> data = fuzzcl::read_file<uint8_t>(arguments.file);
    fuzzcl::fuzz_from_input(data.data(), data.size(), binaries,
                            kernel_binary_sizes,
                            {arguments.callbacks, arguments.verbose,
                             arguments.device, arguments.output});
  }

  return 0;
}
