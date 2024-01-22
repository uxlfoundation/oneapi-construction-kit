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

// System headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
// In-house headers
#include "cargo/string_algorithm.h"
#include "oclc.h"

namespace {
template <typename T>
struct TypeConverter;

template <>
struct TypeConverter<cl_float> {
  using type = cl_int;
};

template <>
struct TypeConverter<cl_double> {
  using type = cl_long;
};

enum class SourceFileType { Spirv, OpenCL_C };
}  // namespace

int main(int argc, char **argv) {
  // Parse the command-line arguments.
  oclc::Driver driver;
  if (driver.ParseArguments(argc, argv) != oclc::success) {
    return 1;
  }

  // Initialize the OpenCL context.
  if (driver.InitCL() != oclc::success) {
    return 1;
  }

  // Build the kernel and saved the compiled output.
  if (driver.BuildProgram() != oclc::success) {
    return 1;
  }

  for (driver.execution_count_ = 0;
       driver.execution_count_ < driver.execution_limit_;
       ++driver.execution_count_) {
    // Enqueue the kernel
    if (driver.EnqueueKernel() != oclc::success) {
      return 1;
    }
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

oclc::Driver::Driver()
    : execution_limit_(1),
      execution_count_(0),
      platform_(nullptr),
      device_(nullptr),
      context_(nullptr),
      program_(nullptr),
      input_file_(),
      output_file_(),
      cl_device_name_(),
      target_cpu_(),
      target_features_(),
      source_(),
      binary_(),
      enqueue_kernel_(),
      kernel_arg_map_(),
      printed_argument_map_(),
      // global_work_size_.size() == execution_limit_
      // global_work_size_[0].size() == work_dim_
      global_work_size_{{64, 4}},
      // default to initialising engine_ with it's default seed
      engine_(),
      argument_queue_(),
      ulp_tolerance_(0),
      work_dim_(2),
      char_tolerance_(0),
      verbose_(false),
      execute_(false) {}

oclc::Driver::~Driver() {
  if (program_) {
    clReleaseProgram(program_);
    program_ = nullptr;
  }
  if (context_) {
    clReleaseContext(context_);
    context_ = nullptr;
  }
}

void oclc::Driver::PrintUsage(int argc, char **argv) {
  (void)argc;

  fprintf(stderr, "usage: %s [options] <CL kernel file>\n", argv[0]);

  fprintf(stderr, "\noptions:\n");
  fprintf(stderr,
          "-o <output_file>                                        Set the "
          "output file to write the binary to.\n");
  fprintf(stderr,
          "-v                                                      Run oclc in "
          "verbose mode.\n");
  fprintf(stderr,
          "-format <output_format>                                 Set the "
          "output file format.\n");
  fprintf(stderr,
          "                                                        Matches the "
          "first occurrence of stage as a substring\n");
  fprintf(stderr,
          "                                                        against "
          "options from '-list'.\n");
  fprintf(stderr,
          "-cl-options 'options...'                                OpenCL "
          "options to use when compiling the kernel.\n");
  fprintf(stderr,
          "-cl-device '<device name>'                              OpenCL "
          "device to use when compiling the kernel.\n");
  fprintf(stderr,
          "-enqueue <kernel name>                                  Enqueues a "
          "kernel to enqueue on work-group\n");
  fprintf(stderr,
          "                                                        size "
          "specific transformations.\n");
  fprintf(stderr,
          "-execute                                                Executes "
          "the enqueued kernel.\n");
  fprintf(stderr,
          "-seed <value>                                           Set the "
          "seed of the random number engine used in rand() calls.\n");
  fprintf(stderr,
          "                                                        The seed is "
          "set to a default value if this is not set.\n");
  fprintf(stderr,
          "-arg <name>[,<width>[,<height>]],<list>                 Assigns a "
          "list value (as described below) to the\n");
  fprintf(stderr,
          "                                                        named "
          "argument when the kernel is executed.\n");
  fprintf(stderr,
          "                                                        If the "
          "argument is a 2D image, a width in pixels must be provided.\n");
  fprintf(stderr,
          "                                                        if the "
          "argument is a 3D image, a height in pixels must also be "
          "provided.\n");
  fprintf(stderr,
          "                                                        If the "
          "argument is an image, 4 values must be provided per pixel,\n");
  fprintf(stderr,
          "                                                        as images "
          "are treated as unsigned 8 bit RGBA arrays by default.\n");
  fprintf(stderr,
          "                                                        If the "
          "argument is declared with the __local qualifier, the\n");
  fprintf(stderr,
          "                                                        first "
          "integer specified will be used to denote the size of the\n");
  fprintf(stderr,
          "                                                        local "
          "argument in bytes, and subsequent values will be ignored.\n");
  fprintf(stderr,
          "-arg <name>[,<width>[,<height>]],<list>:<filename>      Assigns a "
          "list value (as described below), held in a\n");
  fprintf(stderr,
          "                                                        file, to "
          "the named argument when the kernel is executed.\n");
  fprintf(stderr,
          "-print <name>[,<offset>],<size>                         Prints a "
          "given number of elements from the given\n");
  fprintf(stderr,
          "                                                        named "
          "argument after execution to stdout, possibly\n");
  fprintf(stderr,
          "                                                        starting "
          "from some offset.\n");
  fprintf(stderr,
          "-print <name>[,<offset>],<size>:<filename>              Prints a "
          "given number of elements from the given\n");
  fprintf(stderr,
          "                                                        named "
          "argument after execution to a file, possibly\n");
  fprintf(stderr,
          "                                                        starting "
          "from some offset.\n");
  fprintf(stderr,
          "-show <name>,<width>,[,<height>[,<depth>]][:<filename>] Prints the "
          "named image argument of the specified size to stdout,\n");
  fprintf(stderr,
          "                                                        or a file, "
          "if one is provided.\n");
  fprintf(stderr,
          "-compare <name>,<expected>                              Compares "
          "the named buffer to an expected list.\n");
  fprintf(stderr,
          "-compare <name>:<filename>                              Compares "
          "the named buffer to an expected list, held in a file.\n");
  fprintf(stderr,
          "-global <g1>,<g2>,...                                   Sets the "
          "global work size to the given array of values.\n");
  fprintf(stderr,
          "-local <l1>,<l2>,...                                    Sets the "
          "local work size to the given array of values.\n");
  fprintf(stderr,
          "-ulp-error <tolerance>                                  Sets the "
          "maximum ULP error between the actual and target values accepted.\n");
  fprintf(stderr,
          "                                                        as a "
          "'match' when -compare is applied to float or double values. "
          "Defaults to 0.\n");
  fprintf(stderr,
          "-char-error <tolerance>                                 Sets the "
          "maximum difference between the actual and target values accepted\n");
  fprintf(stderr,
          "                                                        as a "
          "'match' when -compare is applied to char or uchar values. Defaults "
          "to 0.\n");
  fprintf(stderr,
          "-repeat-execution <N>                                   Executes "
          "the kernel N times. -global, -local, and -arg\n");
  fprintf(stderr,
          "                                                        arguments "
          "may be set to {<list>},{<list>},... to take on\n");
  fprintf(stderr,
          "                                                        different "
          "values on each execution.\n");

  fprintf(stderr, "\nAvailable output formats:\n");
  fprintf(stderr,
          "  text                                                  "
          "  textual format such as LLVM IR or assembly\n");
  fprintf(stderr,
          "  binary                                                "
          "  binary format such as LLVM BC or ELF\n");

  fprintf(stderr, "\nPossible kernel argument values:\n");
  fprintf(stderr, "  <list>   ::= <el>\n");
  fprintf(stderr, "            |  <el> \",\" <list>\n");
  fprintf(stderr,
          "            |  <cl_bool> \",\" <cl_addressing_mode> \",\" "
          "<cl_filter_mode>\" (for specifying sampler_t only)\n\n");
  fprintf(stderr, "  <el>     ::= <integer or decimal>\n");
  fprintf(stderr,
          "            |  \"repeat(\" <unsigned integer> \",\" <list> \")\"\n");
  fprintf(stderr, "            |  \"rand(\" <decimal> \",\" <decimal> \")\"\n");
  fprintf(stderr,
          "            |  \"randint(\" <integer> \",\" <integer> \")\"\n");
  fprintf(stderr,
          "            |  \"range(\" <integer or decimal> \",\" <integer or "
          "decimal> \")\"\n");
  fprintf(stderr,
          "            |  \"range(\" <integer or decimal> \",\" <integer or "
          "decimal> \",\" <integer or decimal> \")\"\n\n");

  fprintf(stderr, "  <cl_bool>            ::= \"CL_TRUE\" | \"CL_FALSE\"\n");
  fprintf(stderr,
          "  <cl_addressing_mode> ::= \"CL_ADDRESS_NONE\" | "
          "\"CL_ADDRESS_CLAMP_TO_EDGE\" | \"CL_ADDRESS_CLAMP\"\n");
  fprintf(stderr,
          "                        |  \"CL_ADDRESS_REPEAT\" | "
          "\"CL_ADDRESS_MIRRORED_REPEAT\"\n");
  fprintf(stderr,
          "  <cl_filter_mode>     ::= \"CL_FILTER_NEAREST\" | "
          "\"CL_FILTER_LINEAR\"\n");

  fprintf(stderr, "\nSpecial kernel argument values:\n");
  fprintf(stderr,
          "  repeat(N,list)                              creates a list "
          "containing `list` repeated `N` times\n");
  fprintf(stderr,
          "                                              repeat(3,2,4) => "
          "2,4,2,4,2,4\n");
  fprintf(stderr,
          "  rand(min,max)                               creates a random "
          "floating point number in [min,max]\n");
  fprintf(stderr,
          "                                              rand(1.2,4) => "
          "3.195201 (potentially)\n");
  fprintf(stderr,
          "  randint(min,max)                            creates a random "
          "integer number in [min,max]\n");
  fprintf(stderr,
          "                                              randint(1,4) => 3 "
          "(potentially)\n");
  fprintf(stderr,
          "  range(a,b,stride)                           produces a list "
          "beginning at `a`, moving in the direction of `b`\n"
          "                                              by `stride` units. if "
          "`stride` is not stated, it defaults to 1.\n");
  fprintf(stderr,
          "                                              range(-4,21,5) => "
          "-4,1,6,11,16,21\n\n");
}

bool oclc::Driver::ParseArguments(int argc, char **argv) {
  Arguments args(argc, argv);
  std::vector<const char *> positional_args;
  while (args.HasMore()) {
    const char *arg_str = nullptr;
    bool failed = false;
    if ((arg_str = args.TakePositional(failed))) {
      positional_args.push_back(arg_str);
    } else if ((arg_str = args.TakeKeyValue("-o", failed))) {
      output_file_ = arg_str;
    } else if (args.TakeKey("-v", failed)) {
      verbose_ = true;
    } else if ((arg_str = args.TakeKeyValue("-cl-options", failed))) {
      cl_options_ = arg_str;
    } else if ((arg_str = args.TakeKeyValue("-cl-device", failed))) {
      cl_device_name_ = arg_str;
    } else if ((arg_str = args.TakeKeyValue("-enqueue", failed))) {
      enqueue_kernel_ = arg_str;
    } else if (args.TakeKey("-execute", failed)) {
      execute_ = true;
    } else if ((arg_str = args.TakeKeyValue("-arg", failed))) {
      argument_queue_.push_back(std::string(arg_str));
    } else if ((arg_str = args.TakeKeyValue("-print", failed))) {
      failed = (ParseArgumentPrintInfo(arg_str) == oclc::failure);
    } else if ((arg_str = args.TakeKeyValue("-show", failed))) {
      failed = (ParseArgumentImageShowInfo(arg_str) == oclc::failure);
    } else if ((arg_str = args.TakeKeyValue("-global", failed))) {
      std::vector<std::string> globalList;
      SplitAndExpandList(arg_str, '\0', globalList);
      failed = (ParseSizeInfo("global", globalList) == oclc::failure);
    } else if ((arg_str = args.TakeKeyValue("-local", failed))) {
      std::vector<std::string> localList;
      SplitAndExpandList(arg_str, '\0', localList);
      failed = (ParseSizeInfo("local", localList) == oclc::failure);
    } else if ((arg_str = args.TakeKeyValue("-seed", failed))) {
      const unsigned long seed =
          static_cast<unsigned long>(strtoull(arg_str, nullptr, 10));
      OCLC_CHECK_FMT(seed == 0 && (strcmp(arg_str, "0") != 0),
                     "error: seed '%s' is an invalid value.\n", arg_str);
      engine_.seed(seed);
    } else if ((arg_str = args.TakeKeyValue("-ulp-error", failed))) {
      const cl_ulong ulpVal =
          static_cast<cl_ulong>(strtol(arg_str, nullptr, 10));
      OCLC_CHECK_FMT(ulpVal == 0 && (strcmp(arg_str, "0") != 0),
                     "error: ulp tolerance '%s' is an invalid value.\n",
                     arg_str);
      ulp_tolerance_ = ulpVal;
    } else if ((arg_str = args.TakeKeyValue("-char-error", failed))) {
      const cl_uchar charErrVal =
          static_cast<cl_uchar>(strtol(arg_str, nullptr, 10));
      OCLC_CHECK_FMT(charErrVal == 0 && (strcmp(arg_str, "0") != 0),
                     "error: char error tolerance '%s' is an invalid value.\n",
                     arg_str);
      char_tolerance_ = charErrVal;
    } else if ((arg_str = args.TakeKeyValue("-compare", failed))) {
      failed = ParseArgumentCompareInfo(arg_str) == oclc::failure;
    } else if ((arg_str = args.TakeKeyValue("-repeat-execution", failed))) {
      const size_t limit = static_cast<size_t>(strtoull(arg_str, nullptr, 10));
      OCLC_CHECK_FMT(limit == 0, "error: seed '%s' is an invalid value.\n",
                     arg_str);
      execution_limit_ = limit;
    } else if (args.TakeKey("-", failed)) {
      // Input file is stdin.
      positional_args.push_back("-");
    } else if (args.TakeKey("-help", failed) ||
               args.TakeKey("--help", failed) || args.TakeKey("-h", failed)) {
      PrintUsage(argc, argv);
      // Return failure to exit directly after argument parsing, see redmine
      // #8154.
      return oclc::failure;
    } else {
      const char *unknown = args.Peek();
      OCLC_CHECK(!unknown, "Expected another argument but got a nullptr");
      (void)fprintf(stderr, "error: unknown option '%s'.\n", unknown);
      return oclc::failure;
    }

    // Handle parsing failures for the current argument before moving on.
    if (failed) {
      return oclc::failure;
    }
  }

  if (positional_args.size() != 1) {
    PrintUsage(argc, argv);
    if (positional_args.size() > 1) {
      (void)fprintf(stderr, "\nerror: too many positional arguments.\n");
    }
    return oclc::failure;
  }

  for (const std::string &s : argument_queue_) {
    if (ParseKernelArgument(s.c_str()) == oclc::failure) {
      return oclc::failure;
    }
  }

  FillSizeInfo(global_work_size_);
  FillSizeInfo(local_work_size_);

  input_file_ = positional_args[0];
  return oclc::success;
}

bool oclc::Driver::VerifyGreaterThanZero(const std::vector<std::string> &vec) {
  for (const std::string &s : vec) {
    if (strtoull(s.c_str(), nullptr, 10) == 0) {
      return false;
    }
  }
  return true;
}

bool oclc::Driver::VerifySignedInt(const std::vector<std::string> &vec) {
  for (std::string s : vec) {
    if (s[0] != '-' && !std::isdigit(s[0])) {
      return false;
    }
    for (size_t i = 1; i < s.length(); ++i) {
      if (!std::isdigit(s[i])) {
        return false;
      }
    }
  }
  return true;
}

bool oclc::Driver::VerifyDoubleVec(const std::vector<std::string> &vec) {
  for (const std::string &s : vec) {
    char *doubleEnd = VerifyDouble(s);
    if (doubleEnd != nullptr && *doubleEnd == '\0') {
      return false;
    }
  }
  return true;
}

char *oclc::Driver::VerifyDouble(const std::string &str) {
  char *end = nullptr;
  errno = 0;
  strtod(str.c_str(), &end);

  return (end != str.c_str() && errno == 0) ? end : nullptr;
}

vector2d<std::string> oclc::Driver::GetRepeatExecutionValues(
    const std::vector<std::string> &vec) {
  vector2d<std::string> repeatVec;
  bool multipleValues = true;
  for (auto &s : vec) {
    if (s.back() == '}') {
      std::vector<std::string> splitvals;
      SplitAndExpandList(s, '}', splitvals);
      repeatVec.push_back(splitvals);
    } else {
      multipleValues = false;
    }
  }

  if (!multipleValues) {
    repeatVec.clear();
    repeatVec.push_back(vec);
  }
  return repeatVec;
}

void oclc::Driver::FillSizeInfo(vector2d<size_t> &workSize) {
  for (auto &el : workSize) {
    const size_t currentSize = el.size();
    if (currentSize < work_dim_) {
      el.resize(work_dim_);
      for (size_t dimIndex = currentSize; dimIndex < work_dim_; ++dimIndex) {
        el[dimIndex] = 1;
      }
    }
  }
}

bool oclc::Driver::ParseSizeInfo(const std::string &argName,
                                 const std::vector<std::string> &vec) {
  vector2d<std::string> repeatVec = GetRepeatExecutionValues(vec);
  const size_t repeatCount = repeatVec.size();
  for (size_t count = 0; count < repeatCount; ++count) {
    OCLC_CHECK_FMT(!VerifyGreaterThanZero(repeatVec[count]),
                   "error: work size '%s' was not described as a list of "
                   "unsigned integers greater than 0\n",
                   argName.c_str());

    const cl_uint current_work_dim = (cl_uint)repeatVec[count].size();
    if (current_work_dim > work_dim_) {
      work_dim_ = current_work_dim;
    }

    if (argName == "global") {
      global_work_size_.resize(repeatCount);
      global_work_size_[count].resize(current_work_dim);
      for (size_t i = 0; i < current_work_dim; ++i) {
        global_work_size_[count][i] =
            (size_t)atoll(repeatVec[count][i].c_str());
      }
    } else if (argName == "local") {
      local_work_size_.resize(repeatCount);
      local_work_size_[count].resize(current_work_dim);
      for (size_t i = 0; i < current_work_dim; ++i) {
        local_work_size_[count][i] = (size_t)atoll(repeatVec[count][i].c_str());
      }
    }
  }
  return oclc::success;
}

size_t oclc::Driver::ParseListElement(
    const char *elementEnd, std::string &rawArg, char expectedEnd,
    std::vector<std::string> &splitVals, size_t &listSize,
    const std::vector<std::string> &elementVals) {
  const size_t elementSize = elementEnd - rawArg.c_str();
  splitVals.insert(splitVals.end(), elementVals.begin(), elementVals.end());
  listSize += elementSize;
  if (*elementEnd == expectedEnd) {
    return listSize;
  }
  if (*elementEnd == ',') {
    rawArg = rawArg.substr(elementSize + 1);
    listSize++;
    return -2;
  }
  splitVals.clear();
  return -1;
}

size_t oclc::Driver::SplitAndExpandList(std::string rawArg, char expectedEnd,
                                        std::vector<std::string> &splitVals) {
  size_t listSize = 0;
  bool valid = true;
  do {
    std::vector<std::string> elementVals;

    // first check if this element is a double
    const char *elementEnd = VerifyDouble(rawArg);
    if (elementEnd) {
      elementVals.push_back(rawArg.substr(0, elementEnd - rawArg.c_str()));
      const size_t elementSize = ParseListElement(
          elementEnd, rawArg, expectedEnd, splitVals, listSize, elementVals);
      if (elementSize == -1u || elementSize == listSize) {
        return elementSize;
      }
      continue;
    }

    // next check if it is a rand() function
    elementEnd = VerifyRand(rawArg.c_str());
    if (elementEnd) {
      elementVals.push_back(rawArg.substr(0, elementEnd - rawArg.c_str()));
      const size_t elementSize = ParseListElement(
          elementEnd, rawArg, expectedEnd, splitVals, listSize, elementVals);
      if (elementSize == -1u || elementSize == listSize) {
        return elementSize;
      }
      continue;
    }

    // next check if it is a randint() function
    elementEnd = VerifyRandInt(rawArg.c_str());
    if (elementEnd) {
      elementVals.push_back(rawArg.substr(0, elementEnd - rawArg.c_str()));
      const size_t elementSize = ParseListElement(
          elementEnd, rawArg, expectedEnd, splitVals, listSize, elementVals);
      if (elementSize == -1u || elementSize == listSize) {
        return elementSize;
      }
      continue;
    }

    // next check if it is a list enclosed by curly braces
    elementEnd = VerifyRepeatExec(rawArg.c_str());
    if (elementEnd) {
      elementVals.push_back(rawArg.substr(1, elementEnd - rawArg.c_str() - 1));
      const size_t elementSize = ParseListElement(
          elementEnd, rawArg, expectedEnd, splitVals, listSize, elementVals);
      if (elementSize == -1u || elementSize == listSize) {
        return elementSize;
      }
      continue;
    }

    // next check if it is a repeat() function
    elementEnd = VerifyRepeat(rawArg.c_str(), elementVals);
    if (elementEnd) {
      const size_t elementSize = ParseListElement(
          elementEnd, rawArg, expectedEnd, splitVals, listSize, elementVals);
      if (elementSize == -1u || elementSize == listSize) {
        return elementSize;
      }
      continue;
    }

    // next check if it is a sampler_t function
    elementEnd = VerifySampler(rawArg.c_str(), elementVals);
    if (elementEnd) {
      const size_t elementSize = ParseListElement(
          elementEnd, rawArg, expectedEnd, splitVals, listSize, elementVals);
      if (elementSize == -1u || elementSize == listSize) {
        return elementSize;
      }
      continue;
    }

    // next check if it is a range() function
    elementEnd = VerifyRange(rawArg.c_str(), elementVals);
    if (elementEnd) {
      const size_t elementSize = ParseListElement(
          elementEnd, rawArg, expectedEnd, splitVals, listSize, elementVals);
      if (elementSize == -1u || elementSize == listSize) {
        return elementSize;
      }
      continue;
    }

    valid = false;
  } while (valid);

  splitVals.clear();
  return listSize;
}

const char *oclc::Driver::VerifyRepeatExec(const char *arg) {
  if (*arg != '{') {
    return nullptr;
  }
  while (*(++arg) != '\0') {
    if (*arg == '}') {
      return arg + 1;
    }
  }
  return nullptr;
}

const char *oclc::Driver::VerifySampler(const char *arg,
                                        std::vector<std::string> &vec) {
  std::vector<std::string> outVec;
  // parse "normalized_coords"
  if (std::string(arg, sizeof("CL_TRUE,") - 1) == "CL_TRUE,") {
    outVec.push_back("CL_TRUE");
    arg += sizeof("CL_TRUE,") - 1;
  } else if (std::string(arg, sizeof("CL_FALSE,") - 1) == "CL_FALSE,") {
    outVec.push_back("CL_FALSE");
    arg += sizeof("CL_FALSE,") - 1;
  } else {
    return nullptr;
  }
  // parse "addressing_mode"
  if (std::string(arg, sizeof("CL_ADDRESS_MIRRORED_REPEAT,") - 1) ==
      "CL_ADDRESS_MIRRORED_REPEAT,") {
    outVec.push_back("CL_ADDRESS_MIRRORED_REPEAT");
    arg += sizeof("CL_ADDRESS_MIRRORED_REPEAT,") - 1;
  } else if (std::string(arg, sizeof("CL_ADDRESS_REPEAT,") - 1) ==
             "CL_ADDRESS_REPEAT,") {
    outVec.push_back("CL_ADDRESS_REPEAT");
    arg += sizeof("CL_ADDRESS_REPEAT,") - 1;
  } else if (std::string(arg, sizeof("CL_ADDRESS_CLAMP_TO_EDGE,") - 1) ==
             "CL_ADDRESS_CLAMP_TO_EDGE,") {
    outVec.push_back("CL_ADDRESS_CLAMP_TO_EDGE");
    arg += sizeof("CL_ADDRESS_CLAMP_TO_EDGE,") - 1;
  } else if (std::string(arg, sizeof("CL_ADDRESS_CLAMP,") - 1) ==
             "CL_ADDRESS_CLAMP,") {
    outVec.push_back("CL_ADDRESS_CLAMP");
    arg += sizeof("CL_ADDRESS_CLAMP,") - 1;
  } else if (std::string(arg, sizeof("CL_ADDRESS_NONE,") - 1) ==
             "CL_ADDRESS_NONE,") {
    outVec.push_back("CL_ADDRESS_NONE");
    arg += sizeof("CL_ADDRESS_NONE,") - 1;
  } else {
    return nullptr;
  }
  // parse "filter_mode"
  if (std::string(arg, sizeof("CL_FILTER_NEAREST") - 1) ==
      "CL_FILTER_NEAREST") {
    outVec.push_back("CL_FILTER_NEAREST");
    arg += sizeof("CL_FILTER_NEAREST") - 1;
  } else if (std::string(arg, sizeof("CL_FILTER_LINEAR") - 1) ==
             "CL_FILTER_LINEAR") {
    outVec.push_back("CL_FILTER_LINEAR");
    arg += sizeof("CL_FILTER_LINEAR") - 1;
  } else {
    return nullptr;
  }
  vec = outVec;
  return arg;
}

char *oclc::Driver::VerifyRand(const char *arg) {
  if (std::string(arg, sizeof("rand(") - 1) != "rand(") {
    return nullptr;
  }
  arg += sizeof("rand(") - 1;

  errno = 0;
  char *cEnd = nullptr;
  strtod(arg, &cEnd);

  if (cEnd == arg || *cEnd != ',' || errno != 0) {
    return nullptr;
  }

  arg = cEnd + 1;  // move past comma

  cEnd = nullptr;
  strtod(arg, &cEnd);

  if (cEnd == arg || *cEnd != ')' || errno != 0) {
    return nullptr;
  }
  return cEnd + 1;  // +1 for the closing ')'
}

char *oclc::Driver::VerifyRandInt(const char *arg) {
  if (std::string(arg, sizeof("randint(") - 1) != "randint(") {
    return nullptr;
  }
  arg += sizeof("randint(") - 1;

  errno = 0;
  char *cEnd = nullptr;
  strtoll(arg, &cEnd, 10);

  if (cEnd == arg || *cEnd != ',' || errno != 0) {
    return nullptr;
  }

  arg = cEnd + 1;  // move past comma

  cEnd = nullptr;
  strtoll(arg, &cEnd, 10);

  if (cEnd == arg || *cEnd != ')' || errno != 0) {
    return nullptr;
  }
  return cEnd + 1;  // +1 for the closing ')'
}

template <typename T>
T oclc::Driver::NextUniform(T min, T max) {
  const double dScale =
      (1 + max - min) / (double)(engine_.max() - engine_.min());
  return (engine_() - engine_.min()) * dScale + min;
}

bool oclc::Driver::ExpandRandVec(std::vector<std::string> &vec) {
  for (std::string &str : vec) {
    if (str.substr(0, sizeof("rand(") - 1) == "rand(") {
      char *valueTracker =
          const_cast<char *>(str.c_str()) + sizeof("rand(") - 1;
      const double min = strtod(valueTracker, &valueTracker);
      const double max = strtod(valueTracker + 1, nullptr);
      if (min > max) {
        return false;
      }
      const double randVal = NextUniform<double>(min, max);
      str = ToStringPrecise(randVal);
    }
  }
  return true;
}

bool oclc::Driver::ExpandRandIntVec(std::vector<std::string> &vec) {
  for (std::string &str : vec) {
    if (str.substr(0, sizeof("randint(") - 1) == "randint(") {
      char *valueTracker =
          const_cast<char *>(str.c_str()) + sizeof("randint(") - 1;
      const long long int min = strtoll(valueTracker, &valueTracker, 10);
      const long long int max = strtoll(valueTracker + 1, nullptr, 10);
      if (min > max) {
        return false;
      }
      const long long int randVal = NextUniform<long long int>(min, max);
      str = std::to_string(randVal);
    }
  }
  return true;
}

template <typename T>
std::vector<std::string> oclc::Driver::CreateRange(T a, T b, T stride) {
  std::vector<std::string> vec;
  // if T is type 'double', equality comparisons in the for loop condition are
  // not reliable. The loops therefore run until (`i` - half of the stride),
  // reaches `b`, as this value will be well past `b`, and the small floating
  // point inaccuracies won't matter.
  T error = std::is_same<T, double>::value ? 0.5 * stride : 0;
  if (b > a && stride > 0.0) {
    for (T i = a; i - error <= b; i += stride) {
      vec.push_back(std::to_string(i));
    }
  } else if (b < a && stride < 0.0) {
    for (T i = a; i - error >= b; i += stride) {
      vec.push_back(std::to_string(i));
    }
  } else if (stride == 0.0) {
    (void)fprintf(
        stderr,
        "error: stride value of 0 for range() function not acceptable\n");
  } else {
    (void)fprintf(stderr,
                  "error: the sign of (b - a) must match the sign of stride in "
                  "range() function\n");
  }
  return vec;
}

const char *oclc::Driver::VerifyRange(const char *arg,
                                      std::vector<std::string> &vec) {
  if (std::string(arg, sizeof("range(") - 1) != "range(") {
    return nullptr;
  }
  arg += sizeof("range(") - 1;

  bool possibleLongLong = true;

  char *cEndD = nullptr;
  const double aD = strtod(arg, &cEndD);
  char *cEndLL = nullptr;
  const long long int aLL = strtoll(arg, &cEndLL, 10);

  if ((cEndD == arg || *cEndD != ',') && (cEndLL == arg || *cEndD != ',')) {
    return nullptr;
  }

  // if strtod parses more characters than strtoll, then there is a decimal
  // point, and the input should be treated as a double. otherwise, they
  // parse the same number of chars, as strtoll won't parse more
  if (cEndD > cEndLL) {
    possibleLongLong = false;
  }

  arg = cEndD + 1;  // move past comma

  cEndD = nullptr;
  cEndLL = nullptr;
  const double bD = strtod(arg, &cEndD);
  const long long int bLL = strtoll(arg, &cEndLL, 10);
  if ((cEndD == arg || !(*cEndD == ')' || *cEndD == ',')) &&
      (cEndD == arg || !(*cEndD == ')' || *cEndD == ','))) {
    return nullptr;
  }

  if (cEndD > cEndLL) {
    possibleLongLong = false;
  }

  arg = cEndD + 1;  // move past comma or closing parenthesis

  if (*cEndD == ')') {
    if (possibleLongLong) {
      vec = CreateRange<int64_t>(aLL, bLL, 1);
    } else {
      vec = CreateRange<double>(aD, bD, 1.0);
    }
    return vec.empty() ? nullptr : arg;
  }

  cEndD = nullptr;
  cEndLL = nullptr;
  const double strideD = strtod(arg, &cEndD);
  const long long int strideLL = strtoll(arg, &cEndLL, 10);

  if (cEndD == arg || *cEndD != ')' || errno != 0) {
    return nullptr;
  }

  if (possibleLongLong) {
    vec = CreateRange<int64_t>(aLL, bLL, strideLL);
  } else {
    vec = CreateRange<double>(aD, bD, strideD);
  }
  return vec.empty() ? nullptr : cEndD + 1;  // +1 for the closing ')'
}

const char *oclc::Driver::VerifyRepeat(const char *arg,
                                       std::vector<std::string> &vec) {
  if (std::string(arg, sizeof("repeat(") - 1) != "repeat(") {
    return nullptr;
  }
  arg += sizeof("repeat(") - 1;

  char *cEnd = nullptr;
  const size_t count = static_cast<size_t>(strtoll(arg, &cEnd, 10));

  if (cEnd == arg || *cEnd != ',') {
    return nullptr;
  }

  arg = cEnd + 1;  // move past comma

  std::vector<std::string> subList;
  arg += SplitAndExpandList(arg, ')', subList);
  if (subList.empty()) {
    return nullptr;
  }

  const size_t stride = subList.size();
  vec.resize(count * stride);

  for (size_t itr = 0; itr < count; ++itr) {
    for (size_t sublist_itr = 0; sublist_itr < stride; ++sublist_itr) {
      vec[itr * stride + sublist_itr] = subList[sublist_itr];
    }
  }

  return arg + 1;  // +1 for the closing ')'
}

bool oclc::Driver::ParseKernelArgument(const char *rawArg) {
  std::vector<std::string> argVal;
  std::string argName;
  const bool listFound = ReadListOrFile(rawArg, argName, argVal);
  if (!listFound) {
    return oclc::failure;
  }

  vector2d<std::string> repeatVec = GetRepeatExecutionValues(argVal);
  for (auto &vec : repeatVec) {
    bool acceptableRange = ExpandRandVec(vec);
    acceptableRange = acceptableRange && ExpandRandIntVec(vec);
    OCLC_CHECK_FMT(
        !acceptableRange,
        "error: minimum value greater than maximum in rand() function in "
        "argument '%s'.\n",
        argName.c_str());
  }
  kernel_arg_map_.insert(
      std::pair<std::string, vector2d<std::string>>(argName, repeatVec));

  return oclc::success;
}

template <typename T>
std::string oclc::Driver::ToStringPrecise(T floating) {
  std::stringstream stream;  // NOLINT(misc-const-correctness)
  stream << std::setprecision(std::numeric_limits<T>::digits10 + 1);
  stream << floating;
  return stream.str();
}

bool oclc::Driver::ReadListOrFile(const char *rawArg, std::string &argName,
                                  std::vector<std::string> &splitVals) {
  // a colon after the argument name denotes a file, whereas a comma denotes a
  // list on the command line
  const char *commaPos = strchr(rawArg, ',');
  const char *colonPos = strchr(rawArg, ':');
  OCLC_CHECK_FMT(
      !commaPos && !colonPos,
      "error: command line argument '%s' is incorrectly formatted. It "
      "may be missing a commma or colon.\n",
      rawArg);

  if (colonPos) {
    argName = std::string(rawArg, colonPos);
    const std::string fileName(colonPos + 1);
    std::string argNameAndValue;
    const bool found = GetArgumentFromFile(fileName, argName, argNameAndValue);
    OCLC_CHECK_FMT(!found,
                   "error: command line argument '%s' could not be parsed into "
                   "the name of a file containing a list of numbers.\n",
                   rawArg);
    SplitAndExpandList(argNameAndValue.substr((colonPos - rawArg) + 1), '\0',
                       splitVals);
  } else if (commaPos) {
    argName = std::string(rawArg, commaPos);
    SplitAndExpandList(commaPos + 1, '\0', splitVals);
  }
  OCLC_CHECK_FMT(splitVals.empty(),
                 "error: kernel argument '%s' to be compared could not be "
                 "parsed into a list of numbers.\n",
                 argName.c_str());

  return oclc::success;
}

bool oclc::Driver::ParseArgumentCompareInfo(const char *rawArg) {
  std::vector<std::string> splitVals;
  std::string argName;
  const bool listFound = ReadListOrFile(rawArg, argName, splitVals);
  if (!listFound) {
    return oclc::failure;
  }

  std::string expectedValue = "";
  for (const std::string &s : splitVals) {
    expectedValue += s + ",";
  }
  expectedValue.resize(expectedValue.size() - 1);
  compared_argument_map_.insert(
      std::pair<std::string, std::string>(argName, expectedValue));

  return oclc::success;
}

bool oclc::Driver::GetArgumentFromFile(const std::string &fileName,
                                       const std::string &argName,
                                       std::string &argNameAndValue) {
  std::ifstream fin(fileName, std::fstream::in);
  OCLC_CHECK_FMT(!fin.good(), "error: file '%s' could not be opened.\n",
                 fileName.c_str());

  // find the size of the file in bytes, so we can allocate a large enough read
  // buffer
  const std::streampos fileBegin = fin.tellg();
  fin.seekg(0, std::ios_base::end);
  const std::streampos fileEnd = fin.tellg();
  fin.seekg(0, std::ios_base::beg);

  const size_t fileSize = static_cast<size_t>(fileEnd - fileBegin);
  char *kernelArgBuffer = new char[fileSize];
  bool found = false;
  while (!fin.eof() && !found) {
    fin.getline(kernelArgBuffer, fileSize);

    char *fileCommaPos = strchr(kernelArgBuffer, ',');
    if (fileCommaPos) {
      if (argName == std::string(kernelArgBuffer, fileCommaPos)) {
        argNameAndValue = kernelArgBuffer;
        found = true;
      }
    }
  }
  delete[] kernelArgBuffer;
  fin.close();
  return found;
}

bool oclc::Driver::ParseArgumentImageShowInfo(const char *rawArg) {
  std::string arg(rawArg);
  const uint8_t dimensions =
      static_cast<uint8_t>(std::count(arg.begin(), arg.end(), ','));
  OCLC_CHECK_FMT(dimensions < 1 || dimensions > 3,
                 "error: command line argument '%s' is incorrectly formatted. "
                 "It has an incorrect number of commas.\n",
                 rawArg);
  std::array<const char *, 3> commas;
  commas[0] = strchr(rawArg, ',');
  commas[1] = strchr(commas[0] + 1, ',');
  commas[2] = commas[1] ? strchr(commas[1] + 1, ',') : nullptr;
  std::array<size_t, 3> size;
  for (size_t i = 0; i < size.size(); ++i) {
    size[i] = nullptr == commas[i]
                  ? 0
                  : static_cast<size_t>(strtoull(commas[i] + 1, nullptr, 10));
  }
  const std::string imageName(rawArg, commas[0]);
  const char *colonPos = strchr(rawArg, ':');
  // If the user has not explicitly declared a destination file, print to stdout
  const std::string destinationFileName =
      colonPos ? std::string(colonPos + 1) : "-";

  auto destinationFileMap = shown_image_map_.find(destinationFileName);
  if (destinationFileMap == shown_image_map_.end()) {
    shown_image_map_[destinationFileName] =
        std::map<std::string, std::array<size_t, 3>>();
    shown_image_map_[destinationFileName][imageName] = size;
  } else {
    destinationFileMap->second[imageName] = size;
  }
  return oclc::success;
}

bool oclc::Driver::ParseArgumentPrintInfo(const char *rawArg) {
  const char *firstCommaPos = strchr(rawArg, ',');
  OCLC_CHECK_FMT((!firstCommaPos),
                 "error: command line argument '%s' is incorrectly formatted. "
                 "It may be missing a commma.\n",
                 rawArg);
  const std::string printValName(rawArg, firstCommaPos);
  // if there is a second comma, the user has specified a print offset
  const char *secondCommaPos = strchr(firstCommaPos + 1, ',');

  size_t printOffset, printSize;
  if (secondCommaPos) {
    printOffset = static_cast<size_t>(strtoull(firstCommaPos + 1, nullptr, 10));
    printSize = static_cast<size_t>(strtoull(secondCommaPos + 1, nullptr, 10));
    OCLC_CHECK_FMT(
        printSize == 0,
        "error: command line argument '%s' is incorrectly formatted. "
        "third parameter, print size, could not be parsed as an "
        "integer greater than 0.\n",
        rawArg);
    OCLC_CHECK_FMT(
        printOffset == 0 && std::string(secondCommaPos + 1, 2) != "0,",
        "error: command line argument '%s' is incorrectly formatted. "
        "second parameter, buffer offset, could not be parsed as an "
        "integer.\n",
        rawArg);
  } else {
    printSize = static_cast<size_t>(strtoull(firstCommaPos + 1, nullptr, 10));
    OCLC_CHECK_FMT(
        printSize == 0,
        "error: command line argument '%s' is incorrectly formatted. "
        "third parameter, print size, could not be parsed as an "
        "integer greater than 0.\n",
        rawArg);
    printOffset = 0;
  }
  auto offsetSizePair = std::pair<size_t, size_t>(printOffset, printSize);

  const char *colonPos = strchr(rawArg, ':');
  // If the user has not explicitly declared a destination file, print to stdout
  const std::string destinationFileName =
      colonPos ? std::string(colonPos + 1) : "-";

  auto destinationFileMap = printed_argument_map_.find(destinationFileName);
  if (destinationFileMap == printed_argument_map_.end()) {
    printed_argument_map_[destinationFileName] =
        std::map<std::string, std::pair<size_t, size_t>>();
    printed_argument_map_[destinationFileName][printValName] = offsetSizePair;
  } else {
    destinationFileMap->second[printValName] = offsetSizePair;
  }
  return oclc::success;
}

bool oclc::Driver::InitCL() {
  cl_int err = CL_SUCCESS;

  // Choose a platform.
  cl_uint num_platforms = 0;
  std::vector<cl_platform_id> platforms;
  err = clGetPlatformIDs(0, nullptr, &num_platforms);
  OCLC_CHECK_CL(err, "clGetPlatformIDs failed");
  platforms.resize(num_platforms);
  err = clGetPlatformIDs(num_platforms, platforms.data(), nullptr);
  OCLC_CHECK_CL(err, "clGetPlatformIDs failed");
  OCLC_CHECK(platforms.size() == 0, "No OpenCL platform found");
  platform_ = platforms[0];
  if (verbose_) {
    size_t name_size = 0;
    std::string platform_name;
    err =
        clGetPlatformInfo(platform_, CL_PLATFORM_NAME, 0, nullptr, &name_size);
    OCLC_CHECK_CL(err, "Getting the platform name size failed");
    platform_name.resize(name_size);
    err = clGetPlatformInfo(platform_, CL_PLATFORM_NAME, name_size,
                            platform_name.data(), nullptr);
    OCLC_CHECK_CL(err, "Getting the platform name failed");
    fprintf(stderr, "Platform: %s\n", platform_name.c_str());
  }

  // Choose a device.
  const cl_device_type device_type = CL_DEVICE_TYPE_ALL;
  cl_uint num_devices = 0;
  std::vector<cl_device_id> devices;
  err = clGetDeviceIDs(platform_, device_type, 0, nullptr, &num_devices);
  OCLC_CHECK_CL(err, "clGetDeviceIDs failed");
  devices.resize(num_devices);
  err = clGetDeviceIDs(platform_, device_type, num_devices, devices.data(),
                       nullptr);
  OCLC_CHECK_CL(err, "clGetDeviceIDs failed");
  OCLC_CHECK(devices.size() == 0,
             "No OpenCL device found on the default platform");

  // pick a device
  std::string picked_device_name;
  for (unsigned i = 0; i < devices.size(); ++i) {
    cl_device_id device = devices[i];
    size_t name_size = 0;
    std::string device_name;

    err = clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr, &name_size);
    OCLC_CHECK_CL(err, "Getting the device name size failed");
    device_name.resize(name_size);
    err = clGetDeviceInfo(device, CL_DEVICE_NAME, name_size, device_name.data(),
                          nullptr);
    OCLC_CHECK_CL(err, "Getting the device name failed");

    // if -cl-device wasn't specified pick the first device
    if ((i == 0) && cl_device_name_.empty()) {
      device_ = device;
      picked_device_name = device_name;
    }

    if (verbose_) {
      if (i == 0) {
        fprintf(stderr, "Device list:\n");
      }
      fprintf(stderr, "\tDevice: %s\n", device_name.c_str());
    }

    // if -cl-device was specified pick that device
    if (!cl_device_name_.empty() &&
        (0 == strcmp(device_name.c_str(), cl_device_name_.c_str()))) {
      device_ = device;
      picked_device_name = device_name;
    }
  }
  OCLC_CHECK_FMT(!cl_device_name_.empty() && device_ == nullptr,
                 "Device \'%s\' not found.", cl_device_name_.c_str());
  if (verbose_ && !cl_device_name_.empty()) {
    fprintf(stderr, "Using device: %s\n", picked_device_name.c_str());
  }

  // Create a context.
  context_ = clCreateContext(
      nullptr, 1, &device_,
      [](const char *errinfo, const void *, size_t, void *) {
        std::fprintf(stderr, "%s\n", errinfo);
      },
      nullptr, &err);
  OCLC_CHECK_CL(err, "Could not create an OpenCL context (%d).\n");

  // Extension to create a program with IL.
  create_program_with_il_ =
      (clCreateProgramWithILKHR_fn)clGetExtensionFunctionAddressForPlatform(
          platform_, "clCreateProgramWithILKHR");

  return oclc::success;
}

void oclc::Driver::AddBuildOptions() {
  // Always add -cl-kernel-arg-info so we can analyse the parameters
  if (cl_options_.size() > 0) {
    cl_options_.append(" ");
  }
  cl_options_.append("-cl-kernel-arg-info");
}

bool oclc::Driver::BuildProgram() {
  // Load the kernel source.
  const char *mode = "rb";
  FILE *fin = nullptr;
  if (input_file_ == "-") {
    // Read the source from the standard input.
    char buffer[256];
    fin = stdin;
    while (true) {
      const size_t bytes_read = fread(buffer, 1, sizeof(buffer), fin);
      if (bytes_read == 0) break;
      source_.append(buffer, buffer + bytes_read);
    }
  } else {
    fin = fopen(input_file_.c_str(), mode);
    OCLC_CHECK(!fin, "Could not open input file");
    fseek(fin, 0, SEEK_END);
    source_.resize(ftell(fin));
    rewind(fin);
    if (source_.size() != fread(source_.data(), 1, source_.size(), fin)) {
      fclose(fin);
      OCLC_CHECK(true, "Could not read input file");
    }
  }
  fclose(fin);

  // Detect the source file type.
  SourceFileType source_file_type = SourceFileType::OpenCL_C;
  const static char spir_magic[] = {'B', 'C', (char)0xC0, (char)0xDE};
  const static char spirv_magic[] = {(char)0x03, (char)0x02, (char)0x23,
                                     (char)0x07};
  if (source_.size() > 4) {
    if (0 == memcmp(source_.data(), spir_magic, sizeof(spir_magic))) {
      OCLC_CHECK(true, "No support for SPIR 1.2");
    } else if (0 == memcmp(source_.data(), spirv_magic, sizeof(spirv_magic))) {
      source_file_type = SourceFileType::Spirv;
    }
  }

  // Build the program.
  cl_int err = CL_SUCCESS;
  if (source_file_type == SourceFileType::Spirv) {
    const unsigned char *source_data = (const unsigned char *)source_.data();
    const size_t source_size = source_.size();

    if (create_program_with_il_ == nullptr) {
      OCLC_CHECK(true,
                 "Tried to create OpenCL program from IL, but "
                 "clGetExtensionFunctionAddressForPlatform failed to load the "
                 "clCreateProgramWithILKHR function");
    }

    program_ =
        create_program_with_il_(context_, source_data, source_size, &err);
  } else {
    const char *source_data = source_.data();
    program_ =
        clCreateProgramWithSource(context_, 1, &source_data, nullptr, &err);
  }
  OCLC_CHECK_CL(err, "Could not create OpenCL program");

  // Build the program.
  AddBuildOptions();
  err = clBuildProgram(program_, 1, &device_, cl_options_.c_str(), nullptr,
                       nullptr);

  if (verbose_ || (CL_SUCCESS != err)) {
    cl_int err_log;
    size_t build_log_size = 0;
    std::string build_log;
    err_log = clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, 0,
                                    nullptr, &build_log_size);
    OCLC_CHECK_CL(err_log, "Requesting the build log size failed");
    build_log.resize(build_log_size);
    err_log = clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG,
                                    build_log_size, (void *)build_log.data(),
                                    nullptr);
    OCLC_CHECK_CL(err_log, "Requesting the build log failed");

    fprintf(stderr, "Build log:\n\n");
    fprintf(stderr, "%s", build_log.c_str());

    if (CL_SUCCESS != err) {
      (void)fprintf(stderr, "Build program failed with error: %s (%d)\n",
                    oclc::cl_error_code_to_name_map[err].c_str(), err);
      return oclc::failure;
    }
  }

  // If we're running the kernel, skip ahead.
  if (!enqueue_kernel_.empty()) {
    return oclc::success;
  }

  // Retrieve the compiled binary.
  OCLC_CHECK(GetProgramBinary() != oclc::success,
             "Could not retrieve the binary using clGetProgramInfo");

  return WriteToFile(binary_.data(), binary_.size(), /*binary*/ true);
}

bool oclc::Driver::WriteToFile(const char *data, const size_t length,
                               bool binary) {
  bool ownsFount = false;
  FILE *fout = nullptr;
  if (output_file_.empty()) {
    output_file_ = !binary ? "-" : input_file_ + ".bin";
  }
  if (output_file_ == "-") {
    fout = stdout;
  } else {
    // KLOCWORK "NNTS.MUST" possible false positive
    // Klocwork doesn't realize c_str() returns a null-terminated string
    fout = fopen(output_file_.c_str(), "wb");
    OCLC_CHECK(!fout, "Could not open output file");
    ownsFount = true;
  }
  fwrite(data, sizeof(char), length, fout);
  fflush(fout);
  if (ownsFount) fclose(fout);

  return oclc::success;
}

bool oclc::Driver::GetProgramBinary() {
  cl_int err = CL_SUCCESS;
  size_t num_binaries = 0;
  std::vector<size_t> binary_sizes;
  err = clGetProgramInfo(program_, CL_PROGRAM_BINARY_SIZES, 0, nullptr,
                         &num_binaries);
  OCLC_CHECK_CL(err, "Getting the number of binary sizes failed");
  binary_sizes.resize(num_binaries / sizeof(size_t));
  err = clGetProgramInfo(program_, CL_PROGRAM_BINARY_SIZES, num_binaries,
                         binary_sizes.data(), nullptr);
  OCLC_CHECK_CL(err, "Getting the binary sizes failed");
  std::vector<std::string> binaries(binary_sizes.size());
  std::vector<char *> binary_refs;
  for (unsigned i = 0; i < binary_sizes.size(); i++) {
    const size_t binary_size = binary_sizes[i];
    std::string &binary = binaries[i];
    binary.resize(binary_size);
    binary_refs.push_back((char *)binary.data());
  }
  err = clGetProgramInfo(program_, CL_PROGRAM_BINARIES, 0, nullptr,
                         &num_binaries);
  OCLC_CHECK_CL(err, "Getting the number of binaries failed");
  binaries.resize(num_binaries / sizeof(const char *));
  err = clGetProgramInfo(program_, CL_PROGRAM_BINARIES, num_binaries,
                         binary_refs.data(), nullptr);
  OCLC_CHECK_CL(err, "Getting the binaries failed");
  binary_ = binaries[0];
  return oclc::success;
}

std::string oclc::Driver::BufferToString(const unsigned char *buffer, size_t n,
                                         const std::string &dataType) {
  std::string stringVal = "";
  if (dataType == "float") {
    const float *floatBuffer = reinterpret_cast<const float *>(buffer);
    for (size_t i = 0; i < n; ++i) {
      stringVal += (ToStringPrecise(floatBuffer[i]) + ",");
    }
  } else if (dataType == "double") {
    const double *doubleBuffer = reinterpret_cast<const double *>(buffer);
    for (size_t i = 0; i < n; ++i) {
      stringVal += (ToStringPrecise(doubleBuffer[i]) + ",");
    }
  } else if (dataType == "char") {
    const char *charBuffer = reinterpret_cast<const char *>(buffer);
    for (size_t i = 0; i < n; ++i) {
      stringVal += (std::to_string(static_cast<short>(charBuffer[i])) + ",");
    }
  } else if (dataType == "uchar") {
    const unsigned char *ucharBuffer =
        reinterpret_cast<const unsigned char *>(buffer);
    for (size_t i = 0; i < n; ++i) {
      stringVal +=
          (std::to_string(static_cast<unsigned short>(ucharBuffer[i])) + ",");
    }
  } else if (dataType == "short") {
    const short *shortBuffer = reinterpret_cast<const short *>(buffer);
    for (size_t i = 0; i < n; ++i) {
      stringVal += (std::to_string(shortBuffer[i]) + ",");
    }
  } else if (dataType == "ushort") {
    const unsigned short *ushortBuffer =
        reinterpret_cast<const unsigned short *>(buffer);
    for (size_t i = 0; i < n; ++i) {
      stringVal += (std::to_string(ushortBuffer[i]) + ",");
    }
  } else if (dataType == "int") {
    const int *intBuffer = reinterpret_cast<const int *>(buffer);
    for (size_t i = 0; i < n; ++i) {
      stringVal += (std::to_string(intBuffer[i]) + ",");
    }
  } else if (dataType == "uint") {
    const unsigned int *uintBuffer =
        reinterpret_cast<const unsigned int *>(buffer);
    for (size_t i = 0; i < n; ++i) {
      stringVal += (std::to_string(uintBuffer[i]) + ",");
    }
  } else if (dataType == "long") {
    const int64_t *longBuffer = reinterpret_cast<const int64_t *>(buffer);
    for (size_t i = 0; i < n; ++i) {
      stringVal += (std::to_string(longBuffer[i]) + ",");
    }
  } else if (dataType == "ulong") {
    const uint64_t *ulongBuffer = reinterpret_cast<const uint64_t *>(buffer);
    for (size_t i = 0; i < n; ++i) {
      stringVal += (std::to_string(ulongBuffer[i]) + ",");
    }
  } else if (dataType == "half") {
    const cl_half *halfBuffer = reinterpret_cast<const cl_half *>(buffer);
    for (size_t i = 0; i < n; ++i) {
      stringVal += (std::to_string(halfBuffer[i]) + ",");
    }
  } else {
    (void)fprintf(stderr, "error printing buffer: unsupported data type (%s)\n",
                  dataType.c_str());
    return "";
  }

  stringVal.resize(stringVal.length() - 1);
  return stringVal;
}

template <typename T>
T *oclc::Driver::CastToTypeInteger(const std::vector<std::string> &source,
                                   vector2d<T> &casted_buffers, size_t &size) {
  if (VerifySignedInt(source)) {
    std::vector<T> data(source.size());
    for (size_t i = 0; i < source.size(); ++i) {
      data[i] = static_cast<T>(atoll(source[i].c_str()));
    }
    casted_buffers.push_back(data);
    size = casted_buffers.back().size() * sizeof(T);
    return casted_buffers.back().data();
  } else {
    (void)fprintf(
        stderr,
        "error: floating point value passed into integer kernel argument\n");
    return nullptr;
  }
}

template <typename T>
T *oclc::Driver::CastToTypeFloat(const std::vector<std::string> &source,
                                 vector2d<T> &casted_buffers, size_t &size) {
  std::vector<T> data(source.size());
  for (size_t i = 0; i < source.size(); ++i) {
    data[i] = static_cast<T>(atof(source[i].c_str()));
  }
  casted_buffers.push_back(data);
  size = casted_buffers.back().size() * sizeof(T);
  return casted_buffers.back().data();
}

bool oclc::Driver::CreateImage(
    const std::vector<std::string> &imageData, const std::string &name,
    vector2d<cl_uchar> &bufferHolder,
    std::map<std::string, std::pair<cl_mem, std::string>> &buffer_map,
    cl_mem &image, uint8_t dimensions) {
  cl_int err;

  // number of channels per pixel for CL_RGBA
  const size_t channel_count = 4;

  size_t width;
  size_t height;
  size_t depth;
  std::string typeName;
  cl_mem_object_type imageType;
  if (dimensions == 3) {
    // in a 3D image, the first value is the width, and second should be height
    width = static_cast<size_t>(strtoull(imageData[0].c_str(), nullptr, 10));
    height = static_cast<size_t>(strtoull(imageData[1].c_str(), nullptr, 10));
    OCLC_CHECK_FMT(
        (imageData.size() - 2) % (height * width * channel_count) != 0,
        "error: width and height given for 3D image '%s' (%zu) does not match "
        "the image data given (%zu elements)\n",
        name.c_str(), width * height * channel_count, (imageData.size() - 2));
    depth = (imageData.size() - 2) / (width * height * channel_count);
    typeName = "image3d_t";
    imageType = CL_MEM_OBJECT_IMAGE3D;
  } else if (dimensions == 2) {
    // in a 2D image, the first value is width
    width = static_cast<size_t>(strtoull(imageData[0].c_str(), nullptr, 10));
    height = (imageData.size() - 1) / (width * channel_count);
    depth = 0;
    typeName = "image2d_t";
    imageType = CL_MEM_OBJECT_IMAGE2D;
  } else if (dimensions == 1) {
    // the width of an OpenCL image is specified in pixels, which in our default
    // case is 4 uints. imageData.size() is specified in uints, so we need to
    // divide by the number of channels. This applies to 2D/3D as well
    width = imageData.size() / channel_count;
    height = 0;
    depth = 0;
    typeName = "image1d_t";
    imageType = CL_MEM_OBJECT_IMAGE1D;
  } else {
    (void)fprintf(stderr, "error: %d-dimensional image '%s' not supported.\n",
                  dimensions, name.c_str());
    return oclc::failure;
  }
  cl_image_desc desc;
  desc.image_type = imageType;
  desc.image_width = width;
  desc.image_height = height;
  desc.image_depth = depth;
  desc.image_array_size = 0;
  desc.image_row_pitch = 0;
  desc.image_slice_pitch = 0;
  desc.num_mip_levels = 0;
  desc.num_samples = 0;
  desc.buffer = nullptr;
  const cl_image_format format = {CL_RGBA, CL_UNSIGNED_INT8};

  size_t size = 0;
  void *data = CastToTypeInteger<cl_uchar>(
      std::vector<std::string>(imageData.begin() + dimensions - 1,
                               imageData.end()),
      bufferHolder, size);
  image = clCreateImage(context_, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                        &format, &desc, data, &err);
  OCLC_CHECK_CL(err, "Creating image failed");

  buffer_map.insert(std::pair<std::string, std::pair<cl_mem, std::string>>(
      name, std::pair<cl_mem, std::string>(image, typeName)));
  return oclc::success;
}

// Copied from UnitCL kts_precision.h
template <typename T>
cl_ulong oclc::Driver::CalculateULP(const T &expected, const T &actual) {
  typedef typename TypeConverter<T>::type IntTy;
  const IntTy e = cargo::bit_cast<IntTy>(expected);
  const IntTy a = cargo::bit_cast<IntTy>(actual);

  if (std::isnan(expected) && std::isnan(actual)) {
    return 0;
  } else if ((e < 0) ^ (a < 0)) {
    // Sign mismatch means infinite ULP error unless both values are zero.
    // The CTS accepts zeros of differing sign since -0.0 == 0.0 evaluates as
    // true.
    return (actual == expected) ? 0 : CL_ULONG_MAX;
  } else if (std::isfinite(expected) ^ std::isfinite(actual)) {
    // one of our values was INF or NaN, and the other was not
    return CL_ULONG_MAX;
  } else {
    const IntTy diff = e - a;
    const IntTy abs = std::abs(diff);
    return static_cast<cl_ulong>(abs);
  }
}

template <typename T>
bool oclc::Driver::CompareEqualFloat(
    const std::vector<std::string> &expectedVec,
    const std::vector<unsigned char> &compareBuffer) {
  const size_t bufferSize = expectedVec.size();
  std::vector<T> referenceVec(bufferSize);
  for (size_t i = 0; i < bufferSize; ++i) {
    referenceVec[i] = static_cast<T>(strtod(expectedVec[i].c_str(), nullptr));
  }

  const T *data = reinterpret_cast<const T *>(compareBuffer.data());
  for (size_t i = 0; i < bufferSize; ++i) {
    if (CalculateULP(referenceVec[i], data[i]) > ulp_tolerance_) {
      return false;
    }
  }
  return true;
}

template <typename T>
bool oclc::Driver::CompareEqualChar(
    const std::vector<std::string> &expectedVec,
    const std::vector<unsigned char> &compareBuffer) {
  const size_t bufferSize = expectedVec.size();
  std::vector<T> referenceVec(bufferSize);
  for (size_t i = 0; i < bufferSize; ++i) {
    referenceVec[i] =
        static_cast<T>(strtol(expectedVec[i].c_str(), nullptr, 10));
  }
  const T *data = reinterpret_cast<const T *>(compareBuffer.data());
  for (size_t i = 0; i < bufferSize; ++i) {
    if (std::abs(referenceVec[i] - data[i]) > char_tolerance_) {
      return false;
    }
  }
  return true;
}

std::map<std::string, std::string> oclc::Driver::FindTypedefs() {
  std::map<std::string, std::string> typedefs;

  size_t typedefIdx = source_.find("typedef", 0);
  while (typedefIdx != source_.npos) {
    std::string unsupportedKeywords[] = {"struct", "union"};
    const size_t scIdx = source_.find(';', typedefIdx);
    bool supportedTypedef = true;
    for (auto &keyword : unsupportedKeywords) {
      const size_t wordIdx = source_.find(keyword, typedefIdx);
      // if an unsupported keyword is closer than the closest semicolon then
      // it's part of the typedef
      if (scIdx == source_.npos ||
          (wordIdx != source_.npos && wordIdx < scIdx)) {
        supportedTypedef = false;
        break;
      }
    }

    if (supportedTypedef) {
      const std::string typedefString =
          source_.substr(typedefIdx, scIdx - typedefIdx);

      std::vector<cargo::string_view> components =
          cargo::split_of(typedefString);
      const std::string alias(components.back().data(),
                              components.back().size());
      std::string trueType = "";
      for (size_t i = 1; i < components.size() - 1; ++i) {
        trueType += std::string(components[i].data(), components[i].size());
        if (i < components.size() - 2) {
          trueType += " ";
        }
      }
      typedefs[alias] = trueType;
    }
    typedefIdx = source_.find("typedef", typedefIdx + 1);
  }
  return typedefs;
}

bool oclc::Driver::EnqueueKernel() {
  if (enqueue_kernel_.empty()) {
    return oclc::success;
  }

  cl_int err;
  cl_command_queue queue = clCreateCommandQueue(context_, device_, 0, &err);
  OCLC_CHECK_CL(err, "Creating command queue failed");

  cl_kernel kernel = clCreateKernel(program_, enqueue_kernel_.c_str(), &err);
  OCLC_CHECK_CL(err, "Creating kernel failed");

  // Try to set kernel arguments
  cl_uint num_args = 0;
  err = clGetKernelInfo(kernel, CL_KERNEL_NUM_ARGS, sizeof(cl_uint), &num_args,
                        nullptr);
  OCLC_CHECK_CL(err, "Querying kernel arguments failed");

  const size_t local_sizes_1D = 16;

  const std::map<std::string, size_t> type_name_to_size_map = {
      {"char", sizeof(cl_char)},     {"uchar", sizeof(cl_uchar)},
      {"short", sizeof(cl_short)},   {"ushort", sizeof(cl_ushort)},
      {"int", sizeof(cl_int)},       {"uint", sizeof(cl_uint)},
      {"float", sizeof(cl_float)},   {"half", sizeof(cl_half)},
      {"double", sizeof(cl_double)}, {"long", sizeof(cl_long)},
      {"ulong", sizeof(cl_ulong)},
  };

  const size_t local_work_size_index =
      (local_work_size_.size() > execution_count_) ? execution_count_ : 0;
  const size_t global_work_size_index =
      (global_work_size_.size() > execution_count_) ? execution_count_ : 0;

  std::map<std::string, std::pair<cl_mem, std::string>> buffer_map;
  std::vector<cl_sampler> samplers;

  vector2d<cl_float> cl_float_buffers;
  vector2d<cl_double> cl_double_buffers;
  vector2d<cl_half> cl_half_buffers;
  vector2d<cl_char> cl_char_buffers;
  vector2d<cl_uchar> cl_uchar_buffers;
  // creating a vector of vectors of cl_short causes compilation errors when
  // used in templates replaced with int16_t, which is equivalent,
  // but not explicitly aligned to 2 bytes
  vector2d<int16_t> cl_short_buffers;
  vector2d<cl_ushort> cl_ushort_buffers;
  vector2d<cl_int> cl_int_buffers;
  vector2d<cl_uint> cl_uint_buffers;
  vector2d<cl_long> cl_long_buffers;
  vector2d<cl_ulong> cl_ulong_buffers;

  // arbitrary large space to cover any parameters for clSetKernelArg
  cl_char argSpace[1024];

  for (unsigned int i = 0; i < num_args; ++i) {
    const size_t max_param_size = 64;
    char param_type_name[max_param_size];
    size_t param_value_size_ret = 0;

    err = clGetKernelArgInfo(kernel, i, CL_KERNEL_ARG_TYPE_NAME, max_param_size,
                             param_type_name, &param_value_size_ret);
    OCLC_CHECK_CL(err, "clGetKernelArgInfo failed");

    cl_kernel_arg_address_qualifier addr_qual;
    err = clGetKernelArgInfo(kernel, i, CL_KERNEL_ARG_ADDRESS_QUALIFIER,
                             sizeof(cl_kernel_arg_address_qualifier),
                             &addr_qual, &param_value_size_ret);
    OCLC_CHECK_CL(err, "clGetKernelArgInfo failed");

    size_t arg_name_size = 0;
    err = clGetKernelArgInfo(kernel, i, CL_KERNEL_ARG_NAME, 0, nullptr,
                             &arg_name_size);
    OCLC_CHECK_CL(err, "clGetKernelInfo failed");
    std::string arg_name;
    arg_name.resize(arg_name_size - 1);
    err = clGetKernelArgInfo(kernel, i, CL_KERNEL_ARG_NAME, arg_name_size,
                             arg_name.data(), nullptr);
    OCLC_CHECK_CL(err, "clGetKernelInfo failed");

    auto input = kernel_arg_map_.find(arg_name);
    size_t kernelArgIndex = 0;
    if (input != kernel_arg_map_.end() &&
        input->second.size() > execution_count_) {
      kernelArgIndex = execution_count_;
    }

    std::string raw_type_name(param_type_name);
    // if the current argument has a typedefed type, swap it out
    char *is_buf = strchr(param_type_name, '*');
    if (is_buf) {
      raw_type_name = raw_type_name.substr(0, (is_buf - param_type_name));
    }
    const std::map<std::string, std::string> typedefs = FindTypedefs();
    for (const auto &pair : typedefs) {
      if (raw_type_name == pair.first) {
        raw_type_name = pair.second;
        break;
      }
    }
    if (is_buf) {
      raw_type_name += "*";
    }

    // If we have a star in it treat it as a buffer
    char *is_image1d = strstr(param_type_name, "image1d_t");
    char *is_image2d = strstr(param_type_name, "image2d_t");
    char *is_image3d = strstr(param_type_name, "image3d_t");
    char *is_sampler = strstr(param_type_name, "sampler_t");
    const bool is_scalar = type_name_to_size_map.count(raw_type_name) != 0;

    const size_t vecSizeIndex = raw_type_name.find_first_of("1248");
    const bool is_vector =
        vecSizeIndex != std::string::npos &&
        type_name_to_size_map.count(raw_type_name.substr(0, vecSizeIndex)) != 0;

    const size_t vec_length =
        is_vector
            ? static_cast<size_t>(strtoull(
                  raw_type_name.substr(vecSizeIndex).c_str(), nullptr, 10))
            : 1;
    void *data = nullptr;
    size_t data_size = 0;

    if (is_buf || is_scalar || is_vector) {
      std::string type_name;
      // Remove the '*' from buffer types, and size from vector types.
      if (is_vector) {
        type_name = raw_type_name.substr(0, vecSizeIndex);
      } else if (is_buf) {
        type_name = raw_type_name.substr(0, raw_type_name.find_first_of('*'));
      } else {
        type_name = raw_type_name;
      }
      if (input != kernel_arg_map_.end()) {
        const auto &source = input->second[kernelArgIndex];
        if (type_name == "float") {
          data = CastToTypeFloat<cl_float>(source, cl_float_buffers, data_size);
        } else if (type_name == "double") {
          data =
              CastToTypeFloat<cl_double>(source, cl_double_buffers, data_size);
        } else if (type_name == "half") {
          data = CastToTypeFloat<cl_half>(source, cl_half_buffers, data_size);
        } else if (type_name == "char") {
          data = CastToTypeInteger<cl_char>(source, cl_char_buffers, data_size);
        } else if (type_name == "unsigned char" || type_name == "uchar") {
          data =
              CastToTypeInteger<cl_uchar>(source, cl_uchar_buffers, data_size);
        } else if (type_name == "unsigned short" || type_name == "ushort") {
          data = CastToTypeInteger<cl_ushort>(source, cl_ushort_buffers,
                                              data_size);
        } else if (type_name == "short") {
          data =
              CastToTypeInteger<int16_t>(source, cl_short_buffers, data_size);
        } else if (type_name == "int") {
          data = CastToTypeInteger<cl_int>(source, cl_int_buffers, data_size);
        } else if (type_name == "unsigned int" || type_name == "uint") {
          data = CastToTypeInteger<cl_uint>(source, cl_uint_buffers, data_size);
        } else if (type_name == "long") {
          data = CastToTypeInteger<cl_long>(source, cl_long_buffers, data_size);
        } else if (type_name == "unsigned long" || type_name == "ulong") {
          data =
              CastToTypeInteger<cl_ulong>(source, cl_ulong_buffers, data_size);
        } else {
          OCLC_CHECK_FMT(
              true,
              "error: type '%s' of argument '%s' not currently supported\n",
              raw_type_name.c_str(), arg_name.c_str());
        }
      }
    }

    if (is_buf) {
      std::string type_name(
          raw_type_name, 0,
          raw_type_name.length() - 1);  // Trim off the following '*'
      if (is_vector) {
        type_name = type_name.substr(0, vecSizeIndex);
      }
      cl_mem buffer;

      if (addr_qual == CL_KERNEL_ARG_ADDRESS_LOCAL) {
        size_t local_memory_size = sizeof(cl_mem);
        if (input != kernel_arg_map_.end() && input->second.size() > 0 &&
            input->second[0].size() > 0) {
          // argument specified though -arg
          local_memory_size = std::stoi(input->second[0][0]);
        }
        err = clSetKernelArg(kernel, i, local_memory_size, nullptr);
      } else {
        // Unless otherwise specified, set the size of output buffers to the
        // product of each dimension of the global work size.
        size_t maxWriteIndex = 1;
        for (const size_t g : global_work_size_[global_work_size_index]) {
          maxWriteIndex *= g;
        }

        size_t kernelElements = 0;
        // search through every instance of -print <arg_name,size[,file_name]>
        // and get the maximum (size + offset) for arg_name
        for (auto &printMap : printed_argument_map_) {
          auto printInfo = printMap.second.find(arg_name);
          if (printInfo != printMap.second.end()) {
            kernelElements =
                std::max(kernelElements,
                         printInfo->second.first + printInfo->second.second);
          }
        }
        auto compareInfo = compared_argument_map_.find(arg_name);
        if (input != kernel_arg_map_.end()) {
          // argument specified though -arg
          buffer =
              clCreateBuffer(context_, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                             data_size, data, &err);
        } else if (kernelElements != 0) {
          // argument specified though -print
          kernelElements = std::max(kernelElements, maxWriteIndex);
          buffer = clCreateBuffer(
              context_, CL_MEM_READ_WRITE,
              kernelElements * type_name_to_size_map.find(type_name)->second,
              nullptr, &err);
        } else if (compareInfo != compared_argument_map_.end()) {
          // argument specified though -compare
          kernelElements = std::max(
              static_cast<size_t>(std::count(compareInfo->second.begin(),
                                             compareInfo->second.end(), ',') +
                                  1),
              maxWriteIndex);
          buffer = clCreateBuffer(
              context_, CL_MEM_READ_WRITE,
              kernelElements * type_name_to_size_map.find(type_name)->second,
              nullptr, &err);
        } else {
          buffer = clCreateBuffer(context_, CL_MEM_READ_WRITE,
                                  128 * local_sizes_1D, nullptr, &err);
        }
        OCLC_CHECK_CL(err, "Creating buffer failed");
        buffer_map.emplace(arg_name,
                           std::pair<cl_mem, std::string>(buffer, type_name));

        err = clSetKernelArg(kernel, i, sizeof(cl_mem), &buffer);
      }
    } else if (is_image1d || is_image2d || is_image3d) {
      cl_mem image;
      std::array<size_t, 3> showSize = {};
      for (auto &showMap : shown_image_map_) {
        auto showInfo = showMap.second.find(arg_name);
        if (showInfo != showMap.second.end()) {
          showSize = showInfo->second;
          break;
        }
      }

      uint8_t dimensions = {};
      std::string typeName;
      cl_mem_object_type imageType = {};
      if (is_image1d) {
        dimensions = 1;
        typeName = "image1d_t";
        imageType = CL_MEM_OBJECT_IMAGE1D;
      } else if (is_image2d) {
        dimensions = 2;
        typeName = "image2d_t";
        imageType = CL_MEM_OBJECT_IMAGE2D;
      } else if (is_image3d) {
        dimensions = 3;
        typeName = "image3d_t";
        imageType = CL_MEM_OBJECT_IMAGE3D;
      }

      if (input != kernel_arg_map_.end()) {
        // argument specified through -arg
        if (CreateImage(input->second[kernelArgIndex], input->first,
                        cl_uchar_buffers, buffer_map, image,
                        dimensions) == oclc::failure) {
          return oclc::failure;
        }
      } else {
        // argument specified through -show
        cl_image_desc desc;
        desc.image_type = imageType;
        desc.image_width = showSize[0];
        desc.image_height = showSize[1];
        desc.image_depth = showSize[2];
        desc.image_array_size = 0;
        desc.image_row_pitch = 0;
        desc.image_slice_pitch = 0;
        desc.num_mip_levels = 0;
        desc.num_samples = 0;
        desc.buffer = nullptr;
        const cl_image_format format = {CL_RGBA, CL_UNSIGNED_INT8};
        image = clCreateImage(context_, CL_MEM_READ_WRITE, &format, &desc,
                              nullptr, &err);
        OCLC_CHECK_CL(err, "Creating image failed");

        buffer_map.insert(
            std::pair<std::string, std::pair<cl_mem, std::string>>(
                arg_name, std::pair<cl_mem, std::string>(image, typeName)));
      }
      err = clSetKernelArg(kernel, i, sizeof(cl_mem), &image);
    } else if (is_sampler) {
      cl_sampler sampler;
      if (input != kernel_arg_map_.end()) {
        std::vector<std::string> samplerParams = input->second[kernelArgIndex];
        const cl_bool normalized_coords =
            samplerParams[0] == "CL_TRUE" ? CL_TRUE : CL_FALSE;
        const cl_filter_mode filter_mode =
            samplerParams[2] == "CL_FILTER_NEAREST" ? CL_FILTER_NEAREST
                                                    : CL_FILTER_LINEAR;
        cl_addressing_mode addressing_mode;
        if (samplerParams[1] == "CL_ADDRESS_MIRRORED_REPEAT") {
          addressing_mode = CL_ADDRESS_MIRRORED_REPEAT;
        } else if (samplerParams[1] == "CL_ADDRESS_REPEAT") {
          addressing_mode = CL_ADDRESS_REPEAT;
        } else if (samplerParams[1] == "CL_ADDRESS_CLAMP_TO_EDGE") {
          addressing_mode = CL_ADDRESS_CLAMP_TO_EDGE;
        } else if (samplerParams[1] == "CL_ADDRESS_CLAMP") {
          addressing_mode = CL_ADDRESS_CLAMP;
        } else {
          addressing_mode = CL_ADDRESS_NONE;
        }
        sampler = clCreateSampler(context_, normalized_coords, addressing_mode,
                                  filter_mode, &err);
      } else {
        sampler = clCreateSampler(context_, CL_TRUE, CL_ADDRESS_NONE,
                                  CL_FILTER_NEAREST, &err);
      }
      OCLC_CHECK_CL(err, "Creating sampler failed");
      err = clSetKernelArg(kernel, i, sizeof(cl_sampler), &sampler);
      if (CL_SUCCESS == err) {
        samplers.push_back(sampler);
      }
    } else if (is_scalar) {
      if (input != kernel_arg_map_.end()) {
        err = clSetKernelArg(
            kernel, i, type_name_to_size_map.find(raw_type_name)->second, data);
      } else {
        // Use dummy data if the user did not specify this scalar argument
        const size_t sizeType =
            type_name_to_size_map.find(raw_type_name)->second;
        const size_t vec_length = 1;
        err = clSetKernelArg(kernel, i, sizeType * vec_length, &argSpace);
      }
    } else if (is_vector) {
      if (input != kernel_arg_map_.end()) {
        err = clSetKernelArg(
            kernel, i,
            type_name_to_size_map.find(raw_type_name.substr(0, vecSizeIndex))
                    ->second *
                vec_length,
            data);
      } else {
        const size_t sizeType =
            type_name_to_size_map.find(raw_type_name.substr(0, vecSizeIndex))
                ->second;
        err = clSetKernelArg(kernel, i, sizeType * vec_length, &argSpace);
      }
    } else {
      // Come up with some default size in case it's something odd like a struct
      size_t sizeType = sizeof(cl_int);
      size_t vec_length = 1;
      // Split into string and optional number to catch vectors
      std::string param_type_as_string(param_type_name);
      const size_t len = param_type_as_string.length();
      for (size_t index = 0; index < len; index++) {
        if (isdigit(param_type_as_string[index])) {
          vec_length = std::stoi(param_type_as_string.substr(index));
          param_type_as_string = param_type_as_string.substr(0, index);
          break;
        }
      }

      // Look up the name to get its size.
      auto search = type_name_to_size_map.find(param_type_as_string);
      if (search != type_name_to_size_map.end()) {
        sizeType = search->second;
      }
      err = clSetKernelArg(kernel, i, sizeType * vec_length, &argSpace);
    }
    OCLC_CHECK_CL(err, "Setting kernel argument failed");
  }

  size_t *local_data = local_work_size_.empty()
                           ? nullptr
                           : local_work_size_[local_work_size_index].data();
  // Enqueue kernel
  if (execute_) {
    err =
        clEnqueueNDRangeKernel(queue, kernel, work_dim_, nullptr,
                               global_work_size_[global_work_size_index].data(),
                               local_data, 0, nullptr, nullptr);
  } else {
    // Create a never-triggered user event to stop the kernel from ever running
    // while still running the compiler.
    cl_event user_event = clCreateUserEvent(context_, &err);
    OCLC_CHECK_CL(err, "Error creating user event");

    err =
        clEnqueueNDRangeKernel(queue, kernel, work_dim_, nullptr,
                               global_work_size_[global_work_size_index].data(),
                               local_data, 1, &user_event, nullptr);

    err = clSetUserEventStatus(user_event, CL_INVALID_EVENT);
    OCLC_CHECK_CL(err, "Error invalidating user event");

    OCLC_CHECK_CL(clReleaseEvent(user_event), "Error releasing user event");
  }
  if (execute_) {
    // display the output from -compare flags
    for (auto &pair : compared_argument_map_) {
      const auto &name = pair.first;
      std::string kernelOutput = "";

      auto bufferPair = buffer_map.find(name);
      if (bufferPair != buffer_map.end()) {
        std::string expectedValue = pair.second;
        // only compare as many digits as are given in the expected value
        const size_t bufferSize =
            std::count(expectedValue.begin(), expectedValue.end(), ',') + 1;
        cl_mem compareBuff = bufferPair->second.first;
        const auto &rawType = bufferPair->second.second;
        const size_t dataSize = type_name_to_size_map.find(rawType)->second;

        std::vector<unsigned char> compareBuffer(bufferSize * dataSize);
        err = clEnqueueReadBuffer(queue, compareBuff, CL_TRUE, 0,
                                  bufferSize * dataSize, compareBuffer.data(),
                                  0, nullptr, nullptr);
        OCLC_CHECK_CL(err, "Enqueuing read buffer failed");
        const std::string bufferString =
            BufferToString(compareBuffer.data(), bufferSize, rawType);
        bool almostMatch = false;
        if (rawType == "float" || rawType == "double" || rawType == "char" ||
            rawType == "uchar") {
          std::vector<std::string> expectedVec;
          SplitAndExpandList(expectedValue, '\0', expectedVec);
          if (rawType == "float") {
            almostMatch =
                CompareEqualFloat<cl_float>(expectedVec, compareBuffer);
          } else if (rawType == "double") {
            almostMatch =
                CompareEqualFloat<cl_double>(expectedVec, compareBuffer);
          } else if (rawType == "char") {
            almostMatch = CompareEqualChar<cl_char>(expectedVec, compareBuffer);
          } else if (rawType == "uchar") {
            almostMatch =
                CompareEqualChar<cl_uchar>(expectedVec, compareBuffer);
          }
        }
        if (almostMatch || (expectedValue == bufferString)) {
          kernelOutput += name;
          kernelOutput += " - match\n";
        } else {
          kernelOutput += name;
          kernelOutput += " - no match:\nexpected: ";
          kernelOutput += expectedValue;
          kernelOutput += "\nactual:   ";
          kernelOutput += bufferString + "\n";
        }
      }
      WriteToFile(kernelOutput.c_str(), kernelOutput.size());
    }
    // display the output from -print flags
    for (auto &printMap : printed_argument_map_) {
      output_file_ = printMap.first;
      std::string kernelOutput = "";

      for (auto &pair : printMap.second) {
        const auto &name = pair.first;

        auto bufferPair = buffer_map.find(name);
        auto scalarPair = kernel_arg_map_.find(name);
        if (bufferPair != buffer_map.end()) {
          const size_t printOffset = pair.second.first;
          const size_t printSize = pair.second.second;
          cl_mem printBuff = bufferPair->second.first;
          const auto &rawType = bufferPair->second.second;
          const size_t dataSize = type_name_to_size_map.find(rawType)->second;

          std::vector<unsigned char> printBuffer(printSize * dataSize);
          err = clEnqueueReadBuffer(
              queue, printBuff, CL_TRUE, printOffset * dataSize,
              printSize * dataSize, printBuffer.data(), 0, nullptr, nullptr);
          OCLC_CHECK_CL(err, "Enqueuing read buffer failed");
          const std::string bufferString =
              BufferToString(printBuffer.data(), printSize, rawType);

          kernelOutput += name;
          kernelOutput += ",";
          kernelOutput += bufferString;
          kernelOutput += "\n";
        } else if (scalarPair != kernel_arg_map_.end()) {
          // we may want to print out the values of scalar inputs, i.e. if they
          // are a random number
          const size_t kernelArgIndex =
              (scalarPair->second.size() > execution_count_) ? execution_count_
                                                             : 0;
          kernelOutput +=
              name + "," + scalarPair->second[kernelArgIndex][0] + "\n";
        }
      }
      WriteToFile(kernelOutput.c_str(), kernelOutput.size());
    }
    for (auto &showMap : shown_image_map_) {
      output_file_ = showMap.first;
      std::string kernelOutput = "";

      for (auto &pair : showMap.second) {
        const std::string name = pair.first;
        auto bufferPair = buffer_map.find(name);
        if (bufferPair != buffer_map.end()) {
          std::array<size_t, 3> region = pair.second;
          for (size_t &n : region) {
            if (n == 0) {
              n = 1;
            }
          }
          cl_mem showBuff = bufferPair->second.first;
          // number of channels per pixel for CL_RGBA
          const size_t channel_count = 4;
          const size_t printSize =
              region[0] * region[1] * region[2] * channel_count;
          std::vector<unsigned char> showBuffer(printSize);

          size_t origin[3] = {0, 0, 0};
          err = clEnqueueReadImage(queue, showBuff, CL_TRUE, origin,
                                   region.data(), 0, 0, showBuffer.data(), 0,
                                   nullptr, nullptr);
          OCLC_CHECK_CL(err, "Enqueuing read buffer failed");
          const std::string bufferString =
              BufferToString(showBuffer.data(), printSize, "uchar");

          kernelOutput += name;
          kernelOutput += ",";
          kernelOutput += bufferString;
          kernelOutput += "\n";
        }
      }
      WriteToFile(kernelOutput.c_str(), kernelOutput.size());
    }
  }

  clFinish(queue);
  clReleaseCommandQueue(queue);
  for (auto &elem : buffer_map) {
    clReleaseMemObject(elem.second.first);
  }
  for (auto &sampler : samplers) {
    clReleaseSampler(sampler);
  }
  clReleaseKernel(kernel);
  return oclc::success;
}

////////////////////////////////////////////////////////////////////////////////

bool oclc::Arguments::HasMore() const { return HasMore(1); }
bool oclc::Arguments::HasMore(int count) const {
  return (pos_ + count - 1) < argc_;
}

const char *oclc::Arguments::Peek() {
  if (!HasMore(1)) return nullptr;
  return argv_[pos_];
}

const char *oclc::Arguments::Take() {
  const char *arg = Peek();
  if (!arg) return nullptr;
  pos_++;
  return arg;
}

const char *oclc::Arguments::TakePositional(bool &failed) {
  const char *arg = Peek();
  if (!arg) {
    (void)fprintf(stderr, "error: no argument left to parse.\n");
    failed |= true;
    return nullptr;
  } else if (arg[0] == '-') {
    return nullptr;
  }
  return Take();
}

bool oclc::Arguments::TakeKey(const char *key, bool &failed) {
  const char *arg = Peek();
  if (!arg) {
    (void)fprintf(stderr, "error: no argument left to parse.\n");
    failed |= true;
    return oclc::failure;
  } else if (0 != strcmp(key, arg)) {
    return oclc::failure;
  }
  Take();
  return oclc::success;
}

const char *oclc::Arguments::TakeKeyValue(const char *key, bool &failed) {
  const char *first_arg = Peek();
  if (!first_arg || (0 != strcmp(first_arg, key))) {
    return nullptr;
  } else if (!HasMore(2)) {
    (void)fprintf(stderr, "error: '%s' must be followed by another argument.\n",
                  key);
    failed |= true;
    return nullptr;
  }
  Take();
  return Take();
}
