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

#ifndef CL_TOOLS_OCLC_H_INCLUDED
#define CL_TOOLS_OCLC_H_INCLUDED

#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <CL/cl_ext_codeplay.h>
#include <stdarg.h>
#include <stdio.h>

#include <array>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

/// @brief Print an error message and return failure.
#define OCLC_CHECK(cond, msg)                    \
  if ((cond)) {                                  \
    (void)fprintf(stderr, "error: %s\n", (msg)); \
    return oclc::failure;                        \
  }                                              \
  (void)0

/// @brief Print a formatted error message and return failure.
#define OCLC_CHECK_FMT(cond, fmt, ...)       \
  if (cond) {                                \
    (void)fprintf(stderr, fmt, __VA_ARGS__); \
    return oclc::failure;                    \
  }                                          \
  (void)0

/// @brief Print an error message and return failure.
#define OCLC_CHECK_CL(ret, msg)                                           \
  if ((ret) != CL_SUCCESS) {                                              \
    (void)fprintf(stderr, "error: %s (%s, %d)\n", (msg),                  \
                  (oclc::cl_error_code_to_name_map[ret].c_str()), (ret)); \
    return oclc::failure;                                                 \
  }                                                                       \
  (void)0

template <typename T>
using vector2d = std::vector<std::vector<T>>;

namespace oclc {

// Function return status
const bool success = true;
const bool failure = false;

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

/// @brief Drives the compilation and execution of OpenCL kernels
class Driver {
 public:
  /// @brief Create a new instance of Driver.
  Driver();
  ~Driver();

  /// @brief Initialize this class with command-line arguments.
  /// @return oclc::success or oclc::failure.
  bool ParseArguments(int argc, char **argv);
  /// @brief Print usage help to the console.
  void PrintUsage(int argc, char **argv);
  /// @brief Create an OpenCL context for compilation.
  /// @return oclc::success or oclc::failure.
  bool InitCL();
  /// @brief Build the user's kernel.
  /// @return oclc::success or oclc::failure.
  bool BuildProgram();
  /// @brief Save the program to a file.
  /// @return oclc::success or oclc::failure.
  bool WriteToFile(const char *data, const size_t length, bool binary = false);
  /// @brief Try to enqueue a kernel
  /// @return oclc::success or oclc::failure.
  bool EnqueueKernel();
  /// @brief Number of times the kernel should be executed.
  size_t execution_limit_;
  /// @brief The current iteration of the kernel execution.
  size_t execution_count_;

  // Methods:
 private:
  /// @brief Retrieve the program's binary from the built program.
  /// @return oclc::success or oclc::failure.
  bool GetProgramBinary();
  /// @brief Determine whether the output file will be a machine code file.
  bool IsOutputFileMC();
  /// @brief Add any required build options.
  void AddBuildOptions();
  /// @brief Parses an argument to be supplied to an enqueued kernel.
  bool ParseKernelArgument(const char *rawArg);
  /// @brief Splits a list of comma-seperated values into a vector of
  /// substrings, until a given end point, recursively expanding any substrings
  /// that aren't literal numbers, such as randint(). Returns the final,
  /// non-expanded size of the list string.
  size_t SplitAndExpandList(std::string rawArg, char expectedEnd,
                            std::vector<std::string> &splitVals);
  /// @brief Consumes an element of a list, and determines what should be done
  /// next, depending on the character following the consumed element.
  size_t ParseListElement(const char *elementEnd, std::string &rawArg,
                          char expectedEnd, std::vector<std::string> &splitVals,
                          size_t &listSize,
                          const std::vector<std::string> &elementVals);
  /// @brief If the start of a string can be parsed as a function of the form
  /// rand(<min val>,<max val>), returns a pointer to the end of the function,
  /// or returns nullptr otherwise.
  char *VerifyRand(const char *arg);
  /// @brief If the start of a string can be parsed as a function of the form
  /// randint(<min val>,<max val>), where both values are integers, returns a
  /// pointer to the end of the function, or returns nullptr otherwise.
  char *VerifyRandInt(const char *arg);
  /// @brief Expands all rand() functions in a vector to a literal number.
  bool ExpandRandVec(std::vector<std::string> &vec);
  /// @brief Expands all randint() functions in a vector to a literal number.
  bool ExpandRandIntVec(std::vector<std::string> &vec);
  /// @brief If the start of a string can be parsed as a function of the form
  /// repeat(<count>,<list>), returns a pointer to the end of the function, and
  /// populates `vec` with the expanded list, or returns nullptr otherwise.
  const char *VerifyRepeat(const char *arg, std::vector<std::string> &vec);
  /// @brief If the start of a string can be parsed as a function of the form
  /// range(<val a>,<val b>[,<stride>]), returns a pointer to the end of
  /// the function, and populates `vec` with the expanded list, or returns
  /// nullptr otherwise.
  const char *VerifyRange(const char *arg, std::vector<std::string> &vec);
  /// @brief Creates a vector of strings of every `stride`th element between low
  /// and high, inclusive.
  template <typename T>
  std::vector<std::string> CreateRange(T low, T high, T stride);
  /// @brief If the start of a string can be parsed as a list of the form
  /// `<cl_bool>,<cl_addressing_mode>,<cl_filter_mode>` returns a pointer to the
  /// end of the list, and populates `vec` with the three values as strings, or
  /// returns nullptr otherwise.
  const char *VerifySampler(const char *arg, std::vector<std::string> &vec);
  /// @brief True if every string in vec can be parsed as a double value.
  bool VerifyDoubleVec(const std::vector<std::string> &vec);
  /// @brief True if every string in vec can be parsed as an unsigned integer
  /// value greater than 0.
  bool VerifyGreaterThanZero(const std::vector<std::string> &vec);
  /// @brief True if every string in vec can be parsed as an signed integer
  /// value.
  bool VerifySignedInt(const std::vector<std::string> &vec);
  /// @brief If the start of a string can be parsed as a double, return a
  /// pointer to the end of the double, or nullptr otherwise.
  char *VerifyDouble(const std::string &str);
  /// @brief Parses an argument <name[,offset],size[:destination]> tuple to be
  /// printed after the kernel has executed. If no destination file is
  /// specified, the data is printed to stdout.
  bool ParseArgumentPrintInfo(const char *rawArg);
  /// @brief Parses an image argument
  /// <name,width[,height[,depth]][:destination]> tuple to be after the kernel
  /// has executed. If no destination file is specified, the data is printed to
  /// stdout.
  bool ParseArgumentImageShowInfo(const char *rawArg);
  /// @brief Parses a buffer <name,expected value> pair to be checked after the
  /// kernel has executed.
  bool ParseArgumentCompareInfo(const char *rawArg);
  /// @brief Parses and stores a list of unsigned integers representing a global
  /// or local work size.
  bool ParseSizeInfo(const std::string &argName,
                     const std::vector<std::string> &vec);
  /// @brief Casts every string in source to the appropriate integer type and
  /// stores the result.
  template <typename T>
  T *CastToTypeInteger(const std::vector<std::string> &source,
                       vector2d<T> &casted_buffers, size_t &size);
  /// @brief Casts every string in source to the appropriate floating point type
  /// and stores the result.
  template <typename T>
  T *CastToTypeFloat(const std::vector<std::string> &source,
                     vector2d<T> &casted_buffers, size_t &size);
  /// @brief Verifies that the seed in rawArg is a 64 bit integer, and sets the
  /// seed of engine_ to that value if it is.
  bool ApplySeed(const char *rawArg);
  /// @brief Generates a platform-independent uniform value in the given range.
  template <typename T>
  T NextUniform(T min, T max);
  /// @brief If the start of a string can be parsed as {...}, returns a pointer
  /// to the 1 pass the closing bracket, or returns nullptr otherwise.
  const char *VerifyRepeatExec(const char *arg);
  /// @brief Turns a vector of the form "{a,b}","{c,d}" to a vector of vectors,
  /// each set of curly braces filling a sub-vector.
  vector2d<std::string> GetRepeatExecutionValues(
      const std::vector<std::string> &vec);
  /// @brief If any work size is lower-dimensional than the largest work size in
  /// the program, all unspecified dimensions are filled with 1s.
  void FillSizeInfo(vector2d<size_t> &workSize);
  /// @brief Tries to look in the named file for the named argument <name,value>
  /// pair, and return it in `argNameAndValue`. Returns false if the file
  /// doesn't exist, or the argument isn't in the file.
  bool GetArgumentFromFile(const std::string &fileName,
                           const std::string &argName,
                           std::string &argNameAndValue);
  /// @brief Takes a kernel argument <name,value> or <name,filename> pair, and
  /// places the split, expanded vector of strings in splitVals, and the name of
  /// the argument in argName.
  bool ReadListOrFile(const char *rawArg, std::string &argName,
                      std::vector<std::string> &splitVals);
  /// @brief Creates an RGBA OpenCL image of uint8s
  bool CreateImage(
      const std::vector<std::string> &imageData, const std::string &name,
      vector2d<cl_uchar> &bufferHolder,
      std::map<std::string, std::pair<cl_mem, std::string>> &buffer_map,
      cl_mem &image, uint8_t dimensions);
  /// @brief Finds every non-struct / union typedef statement in a kernel.
  std::map<std::string, std::string> FindTypedefs();
  /// @brief Returns true if the ULP error of every element in `expectedVec` and
  /// the corresponding `compareBuffer` element is less than or equal to
  /// `ulp_tolerance_`.
  template <typename T>
  bool CompareEqualFloat(const std::vector<std::string> &expectedVec,
                         const std::vector<unsigned char> &compareBuffer);
  /// @brief Returns true if the difference between every element in
  /// `expectedVec` and the corresponding `compareBuffer` element is less than
  /// or equal to `char_tolerance_`. This is needed as floats casted to chars
  /// occasionally produce small differences on different architectures.
  template <typename T>
  bool CompareEqualChar(const std::vector<std::string> &expectedVec,
                        const std::vector<unsigned char> &compareBuffer);
  /// @brief Calculates the ULP error of a result value with respect to
  /// a reference value.
  ///
  /// @tparam T Single or Double precision floating point type.
  template <typename T>
  cl_ulong CalculateULP(const T &expected, const T &actual);
  /// @brief Returns a decimal string representation of a floating point number
  /// to the highest possible number of decimal places, as defined by
  /// std::numeric_limits<T>::digits10.
  template <typename T>
  std::string ToStringPrecise(T floating);
  /// @brief Converts an OpenCL buffer to a comma seperated string list.
  std::string BufferToString(const unsigned char *buffer, size_t n,
                             const std::string &dataType);

  // Attributes:

  /// @brief Selected OpenCL platform.
  cl_platform_id platform_;
  /// @brief Selected OpenCL device.
  cl_device_id device_;
  /// @brief OpenCL context used to compile programs.
  cl_context context_;
  /// @brief OpenCL program created from the user's kernel.
  cl_program program_;
  /// @brief Extension function that allows OpenCL to compile kernels from IL
  /// (SPIR-V in practice).
  clCreateProgramWithILKHR_fn create_program_with_il_;

  /// @brief Path to the user's kernel file.
  std::string input_file_;
  /// @brief Path to the output file to create.
  std::string output_file_;
  /// @brief OpenCL options to use when building an OpenCL program.
  std::string cl_options_;
  /// @brief OpenCL device to use when building an OpenCL program.
  std::string cl_device_name_;
  /// @brief LLVM target triple to use to compile the program.
  std::string target_triple_;
  /// @brief LLVM target CPU to use to compile the program.
  std::string target_cpu_;
  /// @brief LLVM target features to use to compile the program.
  std::string target_features_;

  /// @brief Source of the user's kernel file.
  std::string source_;
  /// @brief Binary taken from the OpenCL program.
  std::string binary_;
  /// @brief Name of kernel to attempt to run.
  std::string enqueue_kernel_;
  /// @brief Map of arguments to the enqueued kernel, if it should be run.
  std::map<std::string, vector2d<std::string>> kernel_arg_map_;
  /// @brief Map of destination file names, to a map of arguments to be printed
  /// after kernel execution, and their buffer offsets & sizes.
  std::map<std::string, std::map<std::string, std::pair<size_t, size_t>>>
      printed_argument_map_;
  /// @brief Map of arguments to be checked after kernel execution, and their
  /// expected values.
  std::map<std::string, std::string> compared_argument_map_;
  /// @brief Vector containing the global work size of the executed kernel.
  vector2d<size_t> global_work_size_;
  /// @brief Vector containing the local work size of the executed kernel.
  vector2d<size_t> local_work_size_;
  /// @brief Mersenne Twister engine for generating platform-independent 64 bit
  /// integers.
  std::mt19937_64 engine_;
  /// @brief Vector of kernel arguments to be processed after all oclc
  /// parameters are read, so that processing is not affected by parameter
  /// order.
  std::vector<std::string> argument_queue_;
  /// @brief Map of destination file names, to a map of image arguments to be
  /// shown after kernel execution, and their width, height, and depth.
  std::map<std::string, std::map<std::string, std::array<size_t, 3>>>
      shown_image_map_;

  /// @brief Unit of least precision tolerance for floating point comparisons.
  cl_ulong ulp_tolerance_;
  /// @brief Number of dimensions of the global and local work sizes.
  cl_uint work_dim_;

  /// @brief Error tolerance for char comparisons.
  cl_uchar char_tolerance_;
  /// @brief Whether oclc should be verbose or not.
  bool verbose_;
  /// @brief True if executing enqueued kernel.
  bool execute_;
};

/// @brief Helps with consuming arguments from the command-line.
struct Arguments {
  /// @brief Create a new argument list.
  Arguments(int argc, char **argv) : argc_(argc), argv_(argv), pos_(1) {}

  /// @brief Determine whether we can take one argument from the list.
  bool HasMore() const;
  /// @brief Determine whether we can take count arguments from the list.
  bool HasMore(int count) const;

  /// @brief Return the current argument in the list or nullptr.
  const char *Peek();
  /// @brief Return the current argument in the list and move to the next one.
  const char *Take();

  /// @brief Take the current argument from the list if it is positional.
  ///        Positinal arguments do not start with '-'.
  /// @arg[inout] failed - If there is no more arguments left in the list,
  ///                      true is OR'd to this value.
  /// @return Positional argument string or nullptr.
  const char *TakePositional(bool &failed);

  /// @brief Take a key arguments from the list if the key matches.
  /// @arg[in] key - Option name string to match.
  /// @arg[inout] failed - If there are not enough arguments left in the list,
  ///                      true is OR'd to this value.
  /// @return oclc::success or oclc::failure.
  bool TakeKey(const char *key, bool &failed);

  /// @brief Take (key, value) arguments from the list if the key matches.
  /// @arg[in] key - Option name string to match.
  /// @arg[inout] failed - If there are not enough arguments left in the list,
  ///                      true is OR'd to this value.
  /// @return Value argument string or nullptr.
  const char *TakeKeyValue(const char *key, bool &failed);

  // Attributes:
 private:
  /// @brief Number of command-line arguments.
  int argc_;
  /// @brief Array of command-line arguments.
  char **argv_;
  /// @brief Index of the current command-line argument.
  int pos_;
};
}  // namespace oclc

#endif  // CL_TOOLS_OCLC_H_INCLUDED
