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

#include <cstdio>

#if defined(__MINGW32__) || defined(__MINGW64__)
// For getpid
#include <sys/types.h>
#include <unistd.h>
// The MinGW implementation does some custom error processing which needs more
// imported C functionality.
#include <cerrno>
#include <cstdlib>
#include <cstring>
#endif

#include "Common.h"
#include "kts/printf.h"
#include "kts/reference_functions.h"

using namespace kts::ucl;

UCL_EXECUTION_TEST_SUITE(PrintfExecution, testing::ValuesIn(getSourceTypes()))
UCL_EXECUTION_TEST_SUITE(PrintfExecutionSPIRV,
                         testing::Values(SPIRV, OFFLINESPIRV))

BasePrintfExecution::BasePrintfExecution() : BaseExecution() {}

void BasePrintfExecution::SetPrintfReference(size_t size,
                                             ReferencePrintfString ref) {
  reader.reset(new PrintfStringReference(size, ref));
}

void BasePrintfExecution::SetPrintfReference(size_t size,
                                             ReferencePrintfRegex ref) {
  reader.reset(new PrintfRegexReference(size, ref));
}

void BasePrintfExecution::RunPrintfND(cl_uint numDims, size_t *globalDims,
                                      size_t *localDims) {
  if ((OPENCL_C == source_type || SPIRV == source_type) &&
      !UCL::hasCompilerSupport(device)) {
    GTEST_SKIP();
  }
  stdout_capture.CaptureStdout();
  RunGenericND(numDims, globalDims, localDims);
  stdout_capture.RestoreStdout();
  std::string buf = stdout_capture.ReadBuffer();
  // Don't run result check if RunGenericND decided to skip the test, or if no
  // reader was set to use as a reference.
  if (IsSkipped() || !reader) {
    return;
  }

  // If there are multiple work-items, i.e. 'reader.size > 1', then this only
  // works if the order of printing is guaranteed (e.g., only work-item 0 ever
  // prints).  That, however, is the only sensible way to test printf as the
  // specification allows interleaving printf output.
  for (size_t i = 0; i < reader->size; i++) {
    reader->Verify(i, buf);
  }
  EXPECT_EQ(0, buf.size());
}

void BasePrintfExecution::RunPrintfNDConcurrent(cl_uint numDims,
                                                size_t *globalDims,
                                                size_t *localDims,
                                                size_t expectedTotalPrintSize) {
  if ((OPENCL_C == source_type || SPIRV == source_type) &&
      !UCL::hasCompilerSupport(device)) {
    GTEST_SKIP();
  }
  stdout_capture.CaptureStdout();
  RunGenericND(numDims, globalDims, localDims);
  stdout_capture.RestoreStdout();
  const std::string buf = stdout_capture.ReadBuffer();
  // Don't run check the result if RunGenericND decided to skip the test
  if (IsSkipped()) {
    return;
  }
  EXPECT_EQ(buf.size(), expectedTotalPrintSize) << "Output was: " << buf;
}

void BasePrintfExecution::RunPrintf1D(size_t globalX, size_t localX) {
  size_t globalDims[1] = {globalX};
  size_t localDims[1] = {localX};
  RunPrintfND(1, globalDims, localX ? localDims : nullptr);
}

void BasePrintfExecution::RunPrintf1DConcurrent(size_t globalX, size_t localX,
                                                size_t expectedTotalPrintSize) {
  size_t globalDims[1] = {globalX};
  size_t localDims[1] = {localX};
  RunPrintfNDConcurrent(1, globalDims, localX ? localDims : nullptr,
                        expectedTotalPrintSize);
}

TEST_P(PrintfExecution, Printf_01_Hello) {
  fail_if_not_vectorized_ = false;
  ReferencePrintfString ref = [](size_t) { return "Hello world!\n"; };

  this->SetPrintfReference(1, ref);
  this->RunPrintf1D(1);
}

using PrintfExecutionWorkItems = PrintfExecutionWithParam<size_t>;
UCL_EXECUTION_TEST_SUITE_P(PrintfExecutionWorkItems, testing::Values(OPENCL_C),
                           testing::Values(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                           kts::N))

// Run N threads of a kernel N times, except that each time only a
// single (different) thread will print anything.
TEST_P(PrintfExecutionWorkItems, Printf_02_Order) {
  fail_if_not_vectorized_ = false;
  const size_t work_items = getParam();
  for (size_t i = 0; i < work_items; i++) {
    ReferencePrintfString ref = [i](size_t x) {
      if (x != i) {
        return std::string("");
      }

      return "Execution " + std::to_string(x) + "\n";
    };

    AddPrimitive((cl_int)i);
    SetPrintfReference(work_items, ref);
    RunPrintf1D(work_items);
  }
}

TEST_P(PrintfExecution, Printf_03_String) {
  fail_if_not_vectorized_ = false;
  ReferencePrintfString ref = [](size_t) { return "Hello World!\n"; };

  this->SetPrintfReference(1, ref);
  this->RunPrintf1D(1);
}

TEST_P(PrintfExecution, Printf_04_Multiple_Printf) {
  fail_if_not_vectorized_ = false;
  ReferencePrintfString ref = [](size_t) { return "1,2,3,4,5,6"; };

  this->SetPrintfReference(1, ref);
  this->RunPrintf1D(1);
}

TEST_P(PrintfExecution, Printf_05_Side_Effects) {
  fail_if_not_vectorized_ = false;
  ReferencePrintfString ref = [](size_t) { return "1"; };
  kts::Reference1D<cl_int> side_effect = [](size_t) { return 2; };

  AddOutputBuffer(1, side_effect);
  this->SetPrintfReference(1, ref);
  this->RunPrintf1D(1);
}

TEST_P(PrintfExecution, Printf_06_Signed_Unsigned) {
  fail_if_not_vectorized_ = false;
  ReferencePrintfString ref = [](size_t) { return "-1, 1, -1, 1"; };

  this->SetPrintfReference(1, ref);
  this->RunPrintf1D(1);
}

TEST_P(PrintfExecution, Printf_07_Multiple_Functions) {
  fail_if_not_vectorized_ = false;
  ReferencePrintfString ref = [](size_t) { return "1,2,3"; };

  this->SetPrintfReference(1, ref);
  this->RunPrintf1D(1);
}

TEST_P(PrintfExecution, Printf_08_Multiple_Strings) {
  fail_if_not_vectorized_ = false;
  ReferencePrintfString ref = [](size_t) { return "test string"; };

  this->SetPrintfReference(1, ref);
  this->RunPrintf1D(1);
}

TEST_P(PrintfExecution, Printf_09_Percent) {
  fail_if_not_vectorized_ = false;
  ReferencePrintfString ref = [](size_t) { return "% 1 % 2 % %"; };

  this->SetPrintfReference(1, ref);
  this->RunPrintf1D(1);
}

TEST_P(PrintfExecution, Printf_10_print_nan) {
  fail_if_not_vectorized_ = false;
  // Use a regex so that we can be more flexible in accepting sign bit printing
  // of NaNs. There are no IEEE-754 grantees about preservation of the sign bit
  // for NaN so we shouldn't rely on it being printed.
  // Note that some of the printfs have a fixed width using format specifiers.
  // This can mean that either a negative or a space is needed to get the same
  // width. We can match these using regular expressions using [ab] and
  // (reg1|reg2).
  ReferencePrintfRegex ref = [](size_t) {
    std::stringstream ss;
    ss << "f and F specifiers:\n"
       << "nan"
       << "-?nan"
       << "NAN"
       << "-?NAN"
       << "\ne and E specifiers:\n"
       << "nan"
       << "-?nan"
       << "NAN"
       << "-?NAN"
       << "\ng and G specifiers:\n"
       << "nan"
       << "-?nan"
       << "NAN"
       << "-?NAN"
       << "\na and A specifiers:\n"
       << "nan"
       << "-?nan"
       << "NAN"
       << "-?NAN"
       << "\ncomplex specifiers:\n"
       << "nan"
       << "    [- ]nan"
       << "nan     "
       << "(-nan|nan )                "
       << "nan"
       << " NAN"
       << "[- ]NAN"
       // Tests with `+` flag character in printf format string means that a
       // '+' or '-' sign character is always printed. For IEEE-754 NaNs we
       // can't guarantee that the sign is preserved, so accept either + or -
       << "(\\+|-)nan"
       << "(\\+|-)nan"
       << "(\\+|-)NAN"
       << "\nas part of a longer format:\n"
       << "lorem ipsum nan dolor sit amet";
    return std::regex(ss.str());
  };

  this->SetPrintfReference(1, ref);
  this->RunPrintf1D(1);
}

TEST_P(PrintfExecution, Printf_11_print_inf) {
  fail_if_not_vectorized_ = false;
  // TODO: This test should also accept Infinity, see #8550.
  ReferencePrintfString ref = [](size_t) {
    std::stringstream ss;
    ss << "f and F specifiers:\n"
       << "inf"
       << "-inf"
       << "INF"
       << "-INF"
       << "\ne and E specifiers:\n"
       << "inf"
       << "-inf"
       << "INF"
       << "-INF"
       << "\ng and G specifiers:\n"
       << "inf"
       << "-inf"
       << "INF"
       << "-INF"
       << "\na and A specifiers:\n"
       << "inf"
       << "-inf"
       << "INF"
       << "-INF"
       << "\ncomplex specifiers:\n"
       << "inf"
       << "    -inf"
       << "inf     "
       << "-inf                "
       << "inf"
       << " INF"
       << "-INF"
       << "+inf"
       << "-inf"
       << "+INF";
    return ss.str();
  };

  this->SetPrintfReference(1, ref);
  this->RunPrintf1D(1);
}

TEST_P(PrintfExecution, Printf_12_multiple_workgroups) {
  fail_if_not_vectorized_ = false;
  const std::string stringPrinted =
      "(0, 0, 0)(1, 0, 0)(0, 1, 0)(1, 1, 0)(0, 0, "
      "1)(1, 0, 1)(0, 1, 1)(1, 1, 1)";

  size_t globalDims[3] = {2, 2, 2};
  size_t localDims[3] = {1, 1, 1};

  // run on three dimensions with 8 work groups of 1 work item
  this->RunPrintfNDConcurrent(3, globalDims, localDims, stringPrinted.size());
}

TEST_P(PrintfExecution, Printf_13_concurrent_printf) {
  fail_if_not_vectorized_ = false;
  std::stringstream stringPrinted;
  for (size_t x = 0; x < kts::N; ++x) {
    if (0 == (x % 2)) {
      stringPrinted << x;
    } else {
      stringPrinted << x + 1;
    }
  };
  this->RunPrintf1DConcurrent(kts::N, 0, stringPrinted.str().size());
}

TEST_P(PrintfExecution, Printf_14_print_vector) {
  fail_if_not_vectorized_ = false;
  ReferencePrintfString strRef = [](size_t) {
    return std::string("0: 0013,0017,0019,0023-%-0xc,0x10,0x12,0x16\n");
  };

  kts::Reference1D<cl_int4> refIn = [](size_t) {
    return cl_int4{{13, 17, 19, 23}};
  };

  AddInputBuffer(1, refIn);
  AddPrimitive(1);
  this->SetPrintfReference(1, strRef);
  this->RunPrintf1D(kts::N);
}

// CA-2479: Some printf options are broken on MinGW
#if defined(__MINGW32__) || defined(__MINGW64__)
TEST_P(PrintfExecution, DISABLED_Printf_15_Floats) {
#else
TEST_P(PrintfExecution, Printf_15_Floats) {
#endif
  fail_if_not_vectorized_ = false;
  const float inputs[] = {-9999999933815811600000000000000000000.000000f,
                          -9999999933815811600000000000000000000.000000f,
                          -9999999933815811600000000000000000000.000000f,
                          -9999999933815811600000000000000000000.000000f,
                          -9999999933815811600000000000000000000.000000f,
                          -9999999933815811600000000000000000000.000000f,
                          -9999999933815811600000000000000000000.000000f,
                          -9999999933815811600000000000000000000.000000f,
                          -9999999933815811600000000000000000000.000000f,
                          -9999999933815811600000000000000000000.000000f,
                          -6.000000f,
                          -5.000000f,
                          -4.000000f,
                          -3.000000f,
                          -2.000000f,
                          -1.000000f,
                          0.000000f,
                          1.000000f,
                          2.000000f,
                          3.000000f,
                          4.000000f,
                          5.000000f,
                          9999999933815811600000000000000000000.000000f,
                          9999999933815811600000000000000000000.000000f,
                          9999999933815811600000000000000000000.000000f,
                          9999999933815811600000000000000000000.000000f,
                          9999999933815811600000000000000000000.000000f,
                          9999999933815811600000000000000000000.000000f,
                          9999999933815811600000000000000000000.000000f,
                          9999999933815811600000000000000000000.000000f,
                          9999999933815811600000000000000000000.000000f,
                          9999999933815811600000000000000000000.000000f};

  const unsigned num_inputs = sizeof(inputs) / sizeof(inputs[0]);
  kts::Reference1D<cl_float> Inputs = [=, &inputs](size_t x) {
    return inputs[x % num_inputs];
  };
  kts::Reference1D<cl_float> Outputs = [&Inputs](size_t x) {
    return Inputs(x) * Inputs(x);
  };

  ReferencePrintfString strRef = [](size_t) {
    return std::string(
        "             INF\n             INF\n"
        "             INF\n             INF\n"
        "             INF\n             INF\n"
        "             INF\n             INF\n"
        "             INF\n             INF\n"
        "        0X1.2P+5\n        0X1.9P+4\n"
        "        0X1.0P+4\n        0X1.2P+3\n"
        "        0X1.0P+2\n        0X1.0P+0\n"
        "        0X0.0P+0\n        0X1.0P+0\n"
        "        0X1.0P+2\n        0X1.2P+3\n"
        "        0X1.0P+4\n        0X1.9P+4\n"
        "             INF\n             INF\n"
        "             INF\n             INF\n"
        "             INF\n             INF\n"
        "             INF\n             INF\n"
        "             INF\n             INF\n");
  };

  AddMacro("NUM_INPUTS", num_inputs);
  AddInputBuffer(kts::N, Inputs);
  AddOutputBuffer(kts::N, Outputs);
  this->SetPrintfReference(1, strRef);
  this->RunPrintf1D(kts::N);
}

// CA-2479: Some printf options are broken on MinGW
#if defined(__MINGW32__) || defined(__MINGW64__)
TEST_P(PrintfExecution, DISABLED_Printf_16_Floats_Vectors) {
#else
TEST_P(PrintfExecution, Printf_16_Floats_Vectors) {
#endif
  fail_if_not_vectorized_ = false;
  const float inputs[] = {-9999999933815811600000000000000000000.000000f,
                          -9999999933815811600000000000000000000.000000f,
                          -9999999933815811600000000000000000000.000000f,
                          -9999999933815811600000000000000000000.000000f,
                          -9999999933815811600000000000000000000.000000f,
                          -9999999933815811600000000000000000000.000000f,
                          -9999999933815811600000000000000000000.000000f,
                          -9999999933815811600000000000000000000.000000f,
                          -9999999933815811600000000000000000000.000000f,
                          -9999999933815811600000000000000000000.000000f,
                          -6.000000f,
                          -5.000000f,
                          -4.000000f,
                          -3.000000f,
                          -2.000000f,
                          -1.000000f,
                          0.000000f,
                          1.000000f,
                          2.000000f,
                          3.000000f,
                          4.000000f,
                          5.000000f,
                          9999999933815811600000000000000000000.000000f,
                          9999999933815811600000000000000000000.000000f,
                          9999999933815811600000000000000000000.000000f,
                          9999999933815811600000000000000000000.000000f,
                          9999999933815811600000000000000000000.000000f,
                          9999999933815811600000000000000000000.000000f,
                          9999999933815811600000000000000000000.000000f,
                          9999999933815811600000000000000000000.000000f,
                          9999999933815811600000000000000000000.000000f,
                          9999999933815811600000000000000000000.000000f};

  const unsigned num_inputs = sizeof(inputs) / sizeof(inputs[0]);
  kts::Reference1D<cl_float> Inputs = [=, &inputs](size_t x) {
    return inputs[x % num_inputs];
  };
  kts::Reference1D<cl_float> Outputs = [&Inputs](size_t x) {
    return Inputs(x) * Inputs(x);
  };

  kts::Reference1D<cl_float2> Inputs2 = [&Inputs](size_t x) {
    float s0 = Inputs((x * 2) + 0);
    float s1 = Inputs((x * 2) + 1);
    return cl_float2{{s0, s1}};
  };

  kts::Reference1D<cl_float2> Outputs2 = [&Outputs](size_t x) {
    float s0 = Outputs((x * 2) + 0);
    float s1 = Outputs((x * 2) + 1);
    return cl_float2{{s0, s1}};
  };

  ReferencePrintfString strRef = [](size_t) {
    return std::string(
        "             INF,             INF\n"
        "             INF,             INF\n"
        "             INF,             INF\n"
        "             INF,             INF\n"
        "             INF,             INF\n"
        "        0X1.2P+5,        0X1.9P+4\n"
        "        0X1.0P+4,        0X1.2P+3\n"
        "        0X1.0P+2,        0X1.0P+0\n"
        "        0X0.0P+0,        0X1.0P+0\n"
        "        0X1.0P+2,        0X1.2P+3\n"
        "        0X1.0P+4,        0X1.9P+4\n"
        "             INF,             INF\n"
        "             INF,             INF\n"
        "             INF,             INF\n"
        "             INF,             INF\n"
        "             INF,             INF\n");
  };

  AddMacro("NUM_INPUTS", num_inputs / 2);
  AddInputBuffer(kts::N, Inputs2);
  AddOutputBuffer(kts::N, Outputs2);
  this->SetPrintfReference(1, strRef);
  this->RunPrintf1D(kts::N);
}

namespace Printf_17_Float_Formatting {
const cl_int num_inputs = 10;

static kts::Reference1D<cl_float> Ref = [](size_t x) {
  switch (x % num_inputs) {
    default:
    case 0:
      return 0.0f;
    case 1:
      return -0.0f;
    case 2:
      return 0.1f;
    case 3:
      return -0.1f;
    case 4:
      return 1.0f / 3.0f;
    case 5:
      return -1.0f / 3.0f;
    case 6:
      return 100.0f;
    case 7:
      return -100.0f;
    case 8:
      return FLT_MAX;
    case 9:
      return -FLT_MAX;
  }
};
}  // namespace Printf_17_Float_Formatting

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
// MSVC CRT printf differs from the OpenCL specification, so until ComputeAorta
// implements a complete '%a' and '%A' replacement these tests fail on Windows
// configurations.  See CA-1174.
TEST_P(PrintfExecution, DISABLED_Printf_17_Float_Formatting_aA) {
#else
TEST_P(PrintfExecution, Printf_17_Float_Formatting_aA) {
#endif
  fail_if_not_vectorized_ = false;

  ReferencePrintfString strRef = [](size_t) {
    return std::string(
        "*** SPACER ***\n"
        "0x0p+0\n"
        "0X0P+0\n"
        "0x0p+0, 0x0p+0\n"
        "0X0P+0, 0X0P+0\n"
        "0x0p+0 hello world\n"
        "0X0P+0 hello world\n"
        "0x0.p+0\n"
        "0X0.P+0\n"
        "    0x0p+0\n"
        "    0X0P+0\n"
        "   0x0.p+0\n"
        "   0X0.P+0\n"
        "0x0p+0\n"
        "0X0P+0\n"
        "0x0.p+0\n"
        "0X0.P+0\n"
        "0x0.000p+0\n"
        "0X0.000P+0\n"
        "0x0.000p+0\n"
        "0X0.000P+0\n"
        "0x0.000p+0\n"
        "0X0.000P+0\n"
        "0x0p+0     hello world\n"
        "0X0P+0     hello world\n"
        "+0x0.000p+0\n"
        "+0X0.000P+0\n"
        "0x0.000p+0\n"
        "0X0.000P+0\n"
        "  0x0.000p+0\n"
        "  0X0.000P+0\n"
        "  0x0.000p+0\n"
        "  0X0.000P+0\n"
        "*** SPACER ***\n"
        "-0x0p+0\n"
        "-0X0P+0\n"
        "-0x0p+0, -0x0p+0\n"
        "-0X0P+0, -0X0P+0\n"
        "-0x0p+0 hello world\n"
        "-0X0P+0 hello world\n"
        "-0x0.p+0\n"
        "-0X0.P+0\n"
        "   -0x0p+0\n"
        "   -0X0P+0\n"
        "  -0x0.p+0\n"
        "  -0X0.P+0\n"
        "-0x0p+0\n"
        "-0X0P+0\n"
        "-0x0.p+0\n"
        "-0X0.P+0\n"
        "-0x0.000p+0\n"
        "-0X0.000P+0\n"
        "-0x0.000p+0\n"
        "-0X0.000P+0\n"
        "-0x0.000p+0\n"
        "-0X0.000P+0\n"
        "-0x0p+0    hello world\n"
        "-0X0P+0    hello world\n"
        "-0x0.000p+0\n"
        "-0X0.000P+0\n"
        "-0x0.000p+0\n"
        "-0X0.000P+0\n"
        " -0x0.000p+0\n"
        " -0X0.000P+0\n"
        " -0x0.000p+0\n"
        " -0X0.000P+0\n"
        "*** SPACER ***\n"
        "0x1.99999ap-4\n"
        "0X1.99999AP-4\n"
        "0x1.99999ap-4, 0x1.99999ap-4\n"
        "0X1.99999AP-4, 0X1.99999AP-4\n"
        "0x1.99999ap-4 hello world\n"
        "0X1.99999AP-4 hello world\n"
        "0x1.99999ap-4\n"
        "0X1.99999AP-4\n"
        "0x1.99999ap-4\n"
        "0X1.99999AP-4\n"
        "0x1.99999ap-4\n"
        "0X1.99999AP-4\n"
        "0x2p-4\n"
        "0X2P-4\n"
        "0x2.p-4\n"
        "0X2.P-4\n"
        "0x1.99ap-4\n"
        "0X1.99AP-4\n"
        "0x1.99ap-4\n"
        "0X1.99AP-4\n"
        "0x1.99ap-4\n"
        "0X1.99AP-4\n"
        "0x1.99999ap-4 hello world\n"
        "0X1.99999AP-4 hello world\n"
        "+0x1.99ap-4\n"
        "+0X1.99AP-4\n"
        "0x1.99ap-4\n"
        "0X1.99AP-4\n"
        "  0x1.99ap-4\n"
        "  0X1.99AP-4\n"
        "  0x1.99ap-4\n"
        "  0X1.99AP-4\n"
        "*** SPACER ***\n"
        "-0x1.99999ap-4\n"
        "-0X1.99999AP-4\n"
        "-0x1.99999ap-4, -0x1.99999ap-4\n"
        "-0X1.99999AP-4, -0X1.99999AP-4\n"
        "-0x1.99999ap-4 hello world\n"
        "-0X1.99999AP-4 hello world\n"
        "-0x1.99999ap-4\n"
        "-0X1.99999AP-4\n"
        "-0x1.99999ap-4\n"
        "-0X1.99999AP-4\n"
        "-0x1.99999ap-4\n"
        "-0X1.99999AP-4\n"
        "-0x2p-4\n"
        "-0X2P-4\n"
        "-0x2.p-4\n"
        "-0X2.P-4\n"
        "-0x1.99ap-4\n"
        "-0X1.99AP-4\n"
        "-0x1.99ap-4\n"
        "-0X1.99AP-4\n"
        "-0x1.99ap-4\n"
        "-0X1.99AP-4\n"
        "-0x1.99999ap-4 hello world\n"
        "-0X1.99999AP-4 hello world\n"
        "-0x1.99ap-4\n"
        "-0X1.99AP-4\n"
        "-0x1.99ap-4\n"
        "-0X1.99AP-4\n"
        " -0x1.99ap-4\n"
        " -0X1.99AP-4\n"
        " -0x1.99ap-4\n"
        " -0X1.99AP-4\n"
        "*** SPACER ***\n"
        "0x1.555556p-2\n"
        "0X1.555556P-2\n"
        "0x1.555556p-2, 0x1.555556p-2\n"
        "0X1.555556P-2, 0X1.555556P-2\n"
        "0x1.555556p-2 hello world\n"
        "0X1.555556P-2 hello world\n"
        "0x1.555556p-2\n"
        "0X1.555556P-2\n"
        "0x1.555556p-2\n"
        "0X1.555556P-2\n"
        "0x1.555556p-2\n"
        "0X1.555556P-2\n"
        "0x1p-2\n"
        "0X1P-2\n"
        "0x1.p-2\n"
        "0X1.P-2\n"
        "0x1.555p-2\n"
        "0X1.555P-2\n"
        "0x1.555p-2\n"
        "0X1.555P-2\n"
        "0x1.555p-2\n"
        "0X1.555P-2\n"
        "0x1.555556p-2 hello world\n"
        "0X1.555556P-2 hello world\n"
        "+0x1.555p-2\n"
        "+0X1.555P-2\n"
        "0x1.555p-2\n"
        "0X1.555P-2\n"
        "  0x1.555p-2\n"
        "  0X1.555P-2\n"
        "  0x1.555p-2\n"
        "  0X1.555P-2\n"
        "*** SPACER ***\n"
        "-0x1.555556p-2\n"
        "-0X1.555556P-2\n"
        "-0x1.555556p-2, -0x1.555556p-2\n"
        "-0X1.555556P-2, -0X1.555556P-2\n"
        "-0x1.555556p-2 hello world\n"
        "-0X1.555556P-2 hello world\n"
        "-0x1.555556p-2\n"
        "-0X1.555556P-2\n"
        "-0x1.555556p-2\n"
        "-0X1.555556P-2\n"
        "-0x1.555556p-2\n"
        "-0X1.555556P-2\n"
        "-0x1p-2\n"
        "-0X1P-2\n"
        "-0x1.p-2\n"
        "-0X1.P-2\n"
        "-0x1.555p-2\n"
        "-0X1.555P-2\n"
        "-0x1.555p-2\n"
        "-0X1.555P-2\n"
        "-0x1.555p-2\n"
        "-0X1.555P-2\n"
        "-0x1.555556p-2 hello world\n"
        "-0X1.555556P-2 hello world\n"
        "-0x1.555p-2\n"
        "-0X1.555P-2\n"
        "-0x1.555p-2\n"
        "-0X1.555P-2\n"
        " -0x1.555p-2\n"
        " -0X1.555P-2\n"
        " -0x1.555p-2\n"
        " -0X1.555P-2\n"
        "*** SPACER ***\n"
        "0x1.9p+6\n"
        "0X1.9P+6\n"
        "0x1.9p+6, 0x1.9p+6\n"
        "0X1.9P+6, 0X1.9P+6\n"
        "0x1.9p+6 hello world\n"
        "0X1.9P+6 hello world\n"
        "0x1.9p+6\n"
        "0X1.9P+6\n"
        "  0x1.9p+6\n"
        "  0X1.9P+6\n"
        "  0x1.9p+6\n"
        "  0X1.9P+6\n"
        "0x2p+6\n"
        "0X2P+6\n"
        "0x2.p+6\n"
        "0X2.P+6\n"
        "0x1.900p+6\n"
        "0X1.900P+6\n"
        "0x1.900p+6\n"
        "0X1.900P+6\n"
        "0x1.900p+6\n"
        "0X1.900P+6\n"
        "0x1.9p+6   hello world\n"
        "0X1.9P+6   hello world\n"
        "+0x1.900p+6\n"
        "+0X1.900P+6\n"
        "0x1.900p+6\n"
        "0X1.900P+6\n"
        "  0x1.900p+6\n"
        "  0X1.900P+6\n"
        "  0x1.900p+6\n"
        "  0X1.900P+6\n"
        "*** SPACER ***\n"
        "-0x1.9p+6\n"
        "-0X1.9P+6\n"
        "-0x1.9p+6, -0x1.9p+6\n"
        "-0X1.9P+6, -0X1.9P+6\n"
        "-0x1.9p+6 hello world\n"
        "-0X1.9P+6 hello world\n"
        "-0x1.9p+6\n"
        "-0X1.9P+6\n"
        " -0x1.9p+6\n"
        " -0X1.9P+6\n"
        " -0x1.9p+6\n"
        " -0X1.9P+6\n"
        "-0x2p+6\n"
        "-0X2P+6\n"
        "-0x2.p+6\n"
        "-0X2.P+6\n"
        "-0x1.900p+6\n"
        "-0X1.900P+6\n"
        "-0x1.900p+6\n"
        "-0X1.900P+6\n"
        "-0x1.900p+6\n"
        "-0X1.900P+6\n"
        "-0x1.9p+6  hello world\n"
        "-0X1.9P+6  hello world\n"
        "-0x1.900p+6\n"
        "-0X1.900P+6\n"
        "-0x1.900p+6\n"
        "-0X1.900P+6\n"
        " -0x1.900p+6\n"
        " -0X1.900P+6\n"
        " -0x1.900p+6\n"
        " -0X1.900P+6\n"
        "*** SPACER ***\n"
        "0x1.fffffep+127\n"
        "0X1.FFFFFEP+127\n"
        "0x1.fffffep+127, 0x1.fffffep+127\n"
        "0X1.FFFFFEP+127, 0X1.FFFFFEP+127\n"
        "0x1.fffffep+127 hello world\n"
        "0X1.FFFFFEP+127 hello world\n"
        "0x1.fffffep+127\n"
        "0X1.FFFFFEP+127\n"
        "0x1.fffffep+127\n"
        "0X1.FFFFFEP+127\n"
        "0x1.fffffep+127\n"
        "0X1.FFFFFEP+127\n"
        "0x2p+127\n"
        "0X2P+127\n"
        "0x2.p+127\n"
        "0X2.P+127\n"
        "0x2.000p+127\n"
        "0X2.000P+127\n"
        "0x2.000p+127\n"
        "0X2.000P+127\n"
        "0x2.000p+127\n"
        "0X2.000P+127\n"
        "0x1.fffffep+127 hello world\n"
        "0X1.FFFFFEP+127 hello world\n"
        "+0x2.000p+127\n"
        "+0X2.000P+127\n"
        "0x2.000p+127\n"
        "0X2.000P+127\n"
        "0x2.000p+127\n"
        "0X2.000P+127\n"
        "0x2.000p+127\n"
        "0X2.000P+127\n"
        "*** SPACER ***\n"
        "-0x1.fffffep+127\n"
        "-0X1.FFFFFEP+127\n"
        "-0x1.fffffep+127, -0x1.fffffep+127\n"
        "-0X1.FFFFFEP+127, -0X1.FFFFFEP+127\n"
        "-0x1.fffffep+127 hello world\n"
        "-0X1.FFFFFEP+127 hello world\n"
        "-0x1.fffffep+127\n"
        "-0X1.FFFFFEP+127\n"
        "-0x1.fffffep+127\n"
        "-0X1.FFFFFEP+127\n"
        "-0x1.fffffep+127\n"
        "-0X1.FFFFFEP+127\n"
        "-0x2p+127\n"
        "-0X2P+127\n"
        "-0x2.p+127\n"
        "-0X2.P+127\n"
        "-0x2.000p+127\n"
        "-0X2.000P+127\n"
        "-0x2.000p+127\n"
        "-0X2.000P+127\n"
        "-0x2.000p+127\n"
        "-0X2.000P+127\n"
        "-0x1.fffffep+127 hello world\n"
        "-0X1.FFFFFEP+127 hello world\n"
        "-0x2.000p+127\n"
        "-0X2.000P+127\n"
        "-0x2.000p+127\n"
        "-0X2.000P+127\n"
        "-0x2.000p+127\n"
        "-0X2.000P+127\n"
        "-0x2.000p+127\n"
        "-0X2.000P+127\n");
  };

  AddPrimitive(Printf_17_Float_Formatting::num_inputs);
  AddInputBuffer(Printf_17_Float_Formatting::num_inputs,
                 Printf_17_Float_Formatting::Ref);
  this->SetPrintfReference(1, strRef);
  this->RunPrintf1D(1);
}

TEST_P(PrintfExecution, Printf_17_Float_Formatting_eE) {
  fail_if_not_vectorized_ = false;

  ReferencePrintfString strRef = [](size_t) {
    return std::string(
        "*** SPACER ***\n"
        "0.000000e+00\n"
        "0.000000E+00\n"
        "0.000000e+00, 0.000000e+00\n"
        "0.000000E+00, 0.000000E+00\n"
        "0.000000e+00 hello world\n"
        "0.000000E+00 hello world\n"
        "0e+00\n"
        "0E+00\n"
        "0.000e+00\n"
        "0.000E+00\n"
        "  0.000e+00\n"
        "  0.000E+00\n"
        "000.000e+00\n"
        "000.000E+00\n"
        "0.000e+00   hello world\n"
        "0.000E+00   hello world\n"
        " +0.000e+00\n"
        " +0.000E+00\n"
        "*** SPACER ***\n"
        "-0.000000e+00\n"
        "-0.000000E+00\n"
        "-0.000000e+00, -0.000000e+00\n"
        "-0.000000E+00, -0.000000E+00\n"
        "-0.000000e+00 hello world\n"
        "-0.000000E+00 hello world\n"
        "-0e+00\n"
        "-0E+00\n"
        "-0.000e+00\n"
        "-0.000E+00\n"
        " -0.000e+00\n"
        " -0.000E+00\n"
        "-00.000e+00\n"
        "-00.000E+00\n"
        "-0.000e+00  hello world\n"
        "-0.000E+00  hello world\n"
        " -0.000e+00\n"
        " -0.000E+00\n"
        "*** SPACER ***\n"
        "1.000000e-01\n"
        "1.000000E-01\n"
        "1.000000e-01, 1.000000e-01\n"
        "1.000000E-01, 1.000000E-01\n"
        "1.000000e-01 hello world\n"
        "1.000000E-01 hello world\n"
        "1e-01\n"
        "1E-01\n"
        "1.000e-01\n"
        "1.000E-01\n"
        "  1.000e-01\n"
        "  1.000E-01\n"
        "001.000e-01\n"
        "001.000E-01\n"
        "1.000e-01   hello world\n"
        "1.000E-01   hello world\n"
        " +1.000e-01\n"
        " +1.000E-01\n"
        "*** SPACER ***\n"
        "-1.000000e-01\n"
        "-1.000000E-01\n"
        "-1.000000e-01, -1.000000e-01\n"
        "-1.000000E-01, -1.000000E-01\n"
        "-1.000000e-01 hello world\n"
        "-1.000000E-01 hello world\n"
        "-1e-01\n"
        "-1E-01\n"
        "-1.000e-01\n"
        "-1.000E-01\n"
        " -1.000e-01\n"
        " -1.000E-01\n"
        "-01.000e-01\n"
        "-01.000E-01\n"
        "-1.000e-01  hello world\n"
        "-1.000E-01  hello world\n"
        " -1.000e-01\n"
        " -1.000E-01\n"
        "*** SPACER ***\n"
        "3.333333e-01\n"
        "3.333333E-01\n"
        "3.333333e-01, 3.333333e-01\n"
        "3.333333E-01, 3.333333E-01\n"
        "3.333333e-01 hello world\n"
        "3.333333E-01 hello world\n"
        "3e-01\n"
        "3E-01\n"
        "3.333e-01\n"
        "3.333E-01\n"
        "  3.333e-01\n"
        "  3.333E-01\n"
        "003.333e-01\n"
        "003.333E-01\n"
        "3.333e-01   hello world\n"
        "3.333E-01   hello world\n"
        " +3.333e-01\n"
        " +3.333E-01\n"
        "*** SPACER ***\n"
        "-3.333333e-01\n"
        "-3.333333E-01\n"
        "-3.333333e-01, -3.333333e-01\n"
        "-3.333333E-01, -3.333333E-01\n"
        "-3.333333e-01 hello world\n"
        "-3.333333E-01 hello world\n"
        "-3e-01\n"
        "-3E-01\n"
        "-3.333e-01\n"
        "-3.333E-01\n"
        " -3.333e-01\n"
        " -3.333E-01\n"
        "-03.333e-01\n"
        "-03.333E-01\n"
        "-3.333e-01  hello world\n"
        "-3.333E-01  hello world\n"
        " -3.333e-01\n"
        " -3.333E-01\n"
        "*** SPACER ***\n"
        "1.000000e+02\n"
        "1.000000E+02\n"
        "1.000000e+02, 1.000000e+02\n"
        "1.000000E+02, 1.000000E+02\n"
        "1.000000e+02 hello world\n"
        "1.000000E+02 hello world\n"
        "1e+02\n"
        "1E+02\n"
        "1.000e+02\n"
        "1.000E+02\n"
        "  1.000e+02\n"
        "  1.000E+02\n"
        "001.000e+02\n"
        "001.000E+02\n"
        "1.000e+02   hello world\n"
        "1.000E+02   hello world\n"
        " +1.000e+02\n"
        " +1.000E+02\n"
        "*** SPACER ***\n"
        "-1.000000e+02\n"
        "-1.000000E+02\n"
        "-1.000000e+02, -1.000000e+02\n"
        "-1.000000E+02, -1.000000E+02\n"
        "-1.000000e+02 hello world\n"
        "-1.000000E+02 hello world\n"
        "-1e+02\n"
        "-1E+02\n"
        "-1.000e+02\n"
        "-1.000E+02\n"
        " -1.000e+02\n"
        " -1.000E+02\n"
        "-01.000e+02\n"
        "-01.000E+02\n"
        "-1.000e+02  hello world\n"
        "-1.000E+02  hello world\n"
        " -1.000e+02\n"
        " -1.000E+02\n"
        "*** SPACER ***\n"
        "3.402823e+38\n"
        "3.402823E+38\n"
        "3.402823e+38, 3.402823e+38\n"
        "3.402823E+38, 3.402823E+38\n"
        "3.402823e+38 hello world\n"
        "3.402823E+38 hello world\n"
        "3e+38\n"
        "3E+38\n"
        "3.403e+38\n"
        "3.403E+38\n"
        "  3.403e+38\n"
        "  3.403E+38\n"
        "003.403e+38\n"
        "003.403E+38\n"
        "3.403e+38   hello world\n"
        "3.403E+38   hello world\n"
        " +3.403e+38\n"
        " +3.403E+38\n"
        "*** SPACER ***\n"
        "-3.402823e+38\n"
        "-3.402823E+38\n"
        "-3.402823e+38, -3.402823e+38\n"
        "-3.402823E+38, -3.402823E+38\n"
        "-3.402823e+38 hello world\n"
        "-3.402823E+38 hello world\n"
        "-3e+38\n"
        "-3E+38\n"
        "-3.403e+38\n"
        "-3.403E+38\n"
        " -3.403e+38\n"
        " -3.403E+38\n"
        "-03.403e+38\n"
        "-03.403E+38\n"
        "-3.403e+38  hello world\n"
        "-3.403E+38  hello world\n"
        " -3.403e+38\n"
        " -3.403E+38\n");
  };

  AddPrimitive(Printf_17_Float_Formatting::num_inputs);
  AddInputBuffer(Printf_17_Float_Formatting::num_inputs,
                 Printf_17_Float_Formatting::Ref);
  this->SetPrintfReference(1, strRef);
  this->RunPrintf1D(1);
}

TEST_P(PrintfExecution, Printf_17_Float_Formatting_fF) {
  fail_if_not_vectorized_ = false;

  ReferencePrintfString strRef = [](size_t) {
    return std::string(
        "*** SPACER ***\n"
        "0.000000\n"
        "0.000000\n"
        "0.000000, 0.000000\n"
        "0.000000, 0.000000\n"
        "0.000000 hello world\n"
        "0.000000 hello world\n"
        "0.000000 letter a\n"
        "0.000000 letter A\n"
        "0.000000 %a percent-a\n"
        "0.000000 %A percent-A\n"
        "0\n"
        "0.0\n"
        "0.00\n"
        "0\n"
        "0.0\n"
        "0.00\n"
        "    0\n"
        "  0.0\n"
        " 0.00\n"
        "00000\n"
        "000.0\n"
        "00.00\n"
        "0    \n"
        "0.0  \n"
        "0.00 \n"
        "   +0\n"
        " +0.0\n"
        "+0.00\n"
        "*** SPACER ***\n"
        "-0.000000\n"
        "-0.000000\n"
        "-0.000000, -0.000000\n"
        "-0.000000, -0.000000\n"
        "-0.000000 hello world\n"
        "-0.000000 hello world\n"
        "-0.000000 letter a\n"
        "-0.000000 letter A\n"
        "-0.000000 %a percent-a\n"
        "-0.000000 %A percent-A\n"
        "-0\n"
        "-0.0\n"
        "-0.00\n"
        "-0\n"
        "-0.0\n"
        "-0.00\n"
        "   -0\n"
        " -0.0\n"
        "-0.00\n"
        "-0000\n"
        "-00.0\n"
        "-0.00\n"
        "-0   \n"
        "-0.0 \n"
        "-0.00\n"
        "   -0\n"
        " -0.0\n"
        "-0.00\n"
        "*** SPACER ***\n"
        "0.100000\n"
        "0.100000\n"
        "0.100000, 0.100000\n"
        "0.100000, 0.100000\n"
        "0.100000 hello world\n"
        "0.100000 hello world\n"
        "0.100000 letter a\n"
        "0.100000 letter A\n"
        "0.100000 %a percent-a\n"
        "0.100000 %A percent-A\n"
        "0\n"
        "0.1\n"
        "0.10\n"
        "0\n"
        "0.1\n"
        "0.10\n"
        "    0\n"
        "  0.1\n"
        " 0.10\n"
        "00000\n"
        "000.1\n"
        "00.10\n"
        "0    \n"
        "0.1  \n"
        "0.10 \n"
        "   +0\n"
        " +0.1\n"
        "+0.10\n"
        "*** SPACER ***\n"
        "-0.100000\n"
        "-0.100000\n"
        "-0.100000, -0.100000\n"
        "-0.100000, -0.100000\n"
        "-0.100000 hello world\n"
        "-0.100000 hello world\n"
        "-0.100000 letter a\n"
        "-0.100000 letter A\n"
        "-0.100000 %a percent-a\n"
        "-0.100000 %A percent-A\n"
        "-0\n"
        "-0.1\n"
        "-0.10\n"
        "-0\n"
        "-0.1\n"
        "-0.10\n"
        "   -0\n"
        " -0.1\n"
        "-0.10\n"
        "-0000\n"
        "-00.1\n"
        "-0.10\n"
        "-0   \n"
        "-0.1 \n"
        "-0.10\n"
        "   -0\n"
        " -0.1\n"
        "-0.10\n"
        "*** SPACER ***\n"
        "0.333333\n"
        "0.333333\n"
        "0.333333, 0.333333\n"
        "0.333333, 0.333333\n"
        "0.333333 hello world\n"
        "0.333333 hello world\n"
        "0.333333 letter a\n"
        "0.333333 letter A\n"
        "0.333333 %a percent-a\n"
        "0.333333 %A percent-A\n"
        "0\n"
        "0.3\n"
        "0.33\n"
        "0\n"
        "0.3\n"
        "0.33\n"
        "    0\n"
        "  0.3\n"
        " 0.33\n"
        "00000\n"
        "000.3\n"
        "00.33\n"
        "0    \n"
        "0.3  \n"
        "0.33 \n"
        "   +0\n"
        " +0.3\n"
        "+0.33\n"
        "*** SPACER ***\n"
        "-0.333333\n"
        "-0.333333\n"
        "-0.333333, -0.333333\n"
        "-0.333333, -0.333333\n"
        "-0.333333 hello world\n"
        "-0.333333 hello world\n"
        "-0.333333 letter a\n"
        "-0.333333 letter A\n"
        "-0.333333 %a percent-a\n"
        "-0.333333 %A percent-A\n"
        "-0\n"
        "-0.3\n"
        "-0.33\n"
        "-0\n"
        "-0.3\n"
        "-0.33\n"
        "   -0\n"
        " -0.3\n"
        "-0.33\n"
        "-0000\n"
        "-00.3\n"
        "-0.33\n"
        "-0   \n"
        "-0.3 \n"
        "-0.33\n"
        "   -0\n"
        " -0.3\n"
        "-0.33\n"
        "*** SPACER ***\n"
        "100.000000\n"
        "100.000000\n"
        "100.000000, 100.000000\n"
        "100.000000, 100.000000\n"
        "100.000000 hello world\n"
        "100.000000 hello world\n"
        "100.000000 letter a\n"
        "100.000000 letter A\n"
        "100.000000 %a percent-a\n"
        "100.000000 %A percent-A\n"
        "100\n"
        "100.0\n"
        "100.00\n"
        "100\n"
        "100.0\n"
        "100.00\n"
        "  100\n"
        "100.0\n"
        "100.00\n"
        "00100\n"
        "100.0\n"
        "100.00\n"
        "100  \n"
        "100.0\n"
        "100.00\n"
        " +100\n"
        "+100.0\n"
        "+100.00\n"
        "*** SPACER ***\n"
        "-100.000000\n"
        "-100.000000\n"
        "-100.000000, -100.000000\n"
        "-100.000000, -100.000000\n"
        "-100.000000 hello world\n"
        "-100.000000 hello world\n"
        "-100.000000 letter a\n"
        "-100.000000 letter A\n"
        "-100.000000 %a percent-a\n"
        "-100.000000 %A percent-A\n"
        "-100\n"
        "-100.0\n"
        "-100.00\n"
        "-100\n"
        "-100.0\n"
        "-100.00\n"
        " -100\n"
        "-100.0\n"
        "-100.00\n"
        "-0100\n"
        "-100.0\n"
        "-100.00\n"
        "-100 \n"
        "-100.0\n"
        "-100.00\n"
        " -100\n"
        "-100.0\n"
        "-100.00\n"
        "*** SPACER ***\n"
        "340282346638528859811704183484516925440.000000\n"
        "340282346638528859811704183484516925440.000000\n"
        "340282346638528859811704183484516925440.000000, "
        "340282346638528859811704183484516925440.000000\n"
        "340282346638528859811704183484516925440.000000, "
        "340282346638528859811704183484516925440.000000\n"
        "340282346638528859811704183484516925440.000000 hello world\n"
        "340282346638528859811704183484516925440.000000 hello world\n"
        "340282346638528859811704183484516925440.000000 letter a\n"
        "340282346638528859811704183484516925440.000000 letter A\n"
        "340282346638528859811704183484516925440.000000 %a percent-a\n"
        "340282346638528859811704183484516925440.000000 %A percent-A\n"
        "340282346638528859811704183484516925440\n"
        "340282346638528859811704183484516925440.0\n"
        "340282346638528859811704183484516925440.00\n"
        "340282346638528859811704183484516925440\n"
        "340282346638528859811704183484516925440.0\n"
        "340282346638528859811704183484516925440.00\n"
        "340282346638528859811704183484516925440\n"
        "340282346638528859811704183484516925440.0\n"
        "340282346638528859811704183484516925440.00\n"
        "340282346638528859811704183484516925440\n"
        "340282346638528859811704183484516925440.0\n"
        "340282346638528859811704183484516925440.00\n"
        "340282346638528859811704183484516925440\n"
        "340282346638528859811704183484516925440.0\n"
        "340282346638528859811704183484516925440.00\n"
        "+340282346638528859811704183484516925440\n"
        "+340282346638528859811704183484516925440.0\n"
        "+340282346638528859811704183484516925440.00\n"
        "*** SPACER ***\n"
        "-340282346638528859811704183484516925440.000000\n"
        "-340282346638528859811704183484516925440.000000\n"
        "-340282346638528859811704183484516925440.000000, "
        "-340282346638528859811704183484516925440.000000\n"
        "-340282346638528859811704183484516925440.000000, "
        "-340282346638528859811704183484516925440.000000\n"
        "-340282346638528859811704183484516925440.000000 hello world\n"
        "-340282346638528859811704183484516925440.000000 hello world\n"
        "-340282346638528859811704183484516925440.000000 letter a\n"
        "-340282346638528859811704183484516925440.000000 letter A\n"
        "-340282346638528859811704183484516925440.000000 %a percent-a\n"
        "-340282346638528859811704183484516925440.000000 %A percent-A\n"
        "-340282346638528859811704183484516925440\n"
        "-340282346638528859811704183484516925440.0\n"
        "-340282346638528859811704183484516925440.00\n"
        "-340282346638528859811704183484516925440\n"
        "-340282346638528859811704183484516925440.0\n"
        "-340282346638528859811704183484516925440.00\n"
        "-340282346638528859811704183484516925440\n"
        "-340282346638528859811704183484516925440.0\n"
        "-340282346638528859811704183484516925440.00\n"
        "-340282346638528859811704183484516925440\n"
        "-340282346638528859811704183484516925440.0\n"
        "-340282346638528859811704183484516925440.00\n"
        "-340282346638528859811704183484516925440\n"
        "-340282346638528859811704183484516925440.0\n"
        "-340282346638528859811704183484516925440.00\n"
        "-340282346638528859811704183484516925440\n"
        "-340282346638528859811704183484516925440.0\n"
        "-340282346638528859811704183484516925440.00\n");
  };

  AddPrimitive(Printf_17_Float_Formatting::num_inputs);
  AddInputBuffer(Printf_17_Float_Formatting::num_inputs,
                 Printf_17_Float_Formatting::Ref);
  this->SetPrintfReference(1, strRef);
  this->RunPrintf1D(1);
}

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
// MSVC CRT printf differs from the OpenCL specification, so until ComputeAorta
// implements a complete '%g' and '%G' replacement these tests fail on Windows
// configurations.  See CA-1174.
TEST_P(PrintfExecution, DISABLED_Printf_17_Float_Formatting_gG) {
#else
TEST_P(PrintfExecution, Printf_17_Float_Formatting_gG) {
#endif
  fail_if_not_vectorized_ = false;

  ReferencePrintfString strRef = [](size_t) {
    return std::string(
        "*** SPACER ***\n"
        "0\n"
        "0\n"
        "0, 0\n"
        "0, 0\n"
        "0 hello world\n"
        "0 hello world\n"
        "0.00000\n"
        "0.00000\n"
        "0\n"
        "0\n"
        "0.\n"
        "0.\n"
        "0\n"
        "0\n"
        "0.00\n"
        "0.00\n"
        "         0\n"
        "         0\n"
        "0000000000\n"
        "0000000000\n"
        "0         \n"
        "0         \n"
        "        +0\n"
        "        +0\n"
        "      0.00\n"
        "      0.00\n"
        "*** SPACER ***\n"
        "-0\n"
        "-0\n"
        "-0, -0\n"
        "-0, -0\n"
        "-0 hello world\n"
        "-0 hello world\n"
        "-0.00000\n"
        "-0.00000\n"
        "-0\n"
        "-0\n"
        "-0.\n"
        "-0.\n"
        "-0\n"
        "-0\n"
        "-0.00\n"
        "-0.00\n"
        "        -0\n"
        "        -0\n"
        "-000000000\n"
        "-000000000\n"
        "-0        \n"
        "-0        \n"
        "        -0\n"
        "        -0\n"
        "     -0.00\n"
        "     -0.00\n"
        "*** SPACER ***\n"
        "0.1\n"
        "0.1\n"
        "0.1, 0.1\n"
        "0.1, 0.1\n"
        "0.1 hello world\n"
        "0.1 hello world\n"
        "0.100000\n"
        "0.100000\n"
        "0.1\n"
        "0.1\n"
        "0.1\n"
        "0.1\n"
        "0.1\n"
        "0.1\n"
        "0.100\n"
        "0.100\n"
        "       0.1\n"
        "       0.1\n"
        "00000000.1\n"
        "00000000.1\n"
        "0.1       \n"
        "0.1       \n"
        "      +0.1\n"
        "      +0.1\n"
        "     0.100\n"
        "     0.100\n"
        "*** SPACER ***\n"
        "-0.1\n"
        "-0.1\n"
        "-0.1, -0.1\n"
        "-0.1, -0.1\n"
        "-0.1 hello world\n"
        "-0.1 hello world\n"
        "-0.100000\n"
        "-0.100000\n"
        "-0.1\n"
        "-0.1\n"
        "-0.1\n"
        "-0.1\n"
        "-0.1\n"
        "-0.1\n"
        "-0.100\n"
        "-0.100\n"
        "      -0.1\n"
        "      -0.1\n"
        "-0000000.1\n"
        "-0000000.1\n"
        "-0.1      \n"
        "-0.1      \n"
        "      -0.1\n"
        "      -0.1\n"
        "    -0.100\n"
        "    -0.100\n"
        "*** SPACER ***\n"
        "0.333333\n"
        "0.333333\n"
        "0.333333, 0.333333\n"
        "0.333333, 0.333333\n"
        "0.333333 hello world\n"
        "0.333333 hello world\n"
        "0.333333\n"
        "0.333333\n"
        "0.3\n"
        "0.3\n"
        "0.3\n"
        "0.3\n"
        "0.333\n"
        "0.333\n"
        "0.333\n"
        "0.333\n"
        "     0.333\n"
        "     0.333\n"
        "000000.333\n"
        "000000.333\n"
        "0.333     \n"
        "0.333     \n"
        "    +0.333\n"
        "    +0.333\n"
        "     0.333\n"
        "     0.333\n"
        "*** SPACER ***\n"
        "-0.333333\n"
        "-0.333333\n"
        "-0.333333, -0.333333\n"
        "-0.333333, -0.333333\n"
        "-0.333333 hello world\n"
        "-0.333333 hello world\n"
        "-0.333333\n"
        "-0.333333\n"
        "-0.3\n"
        "-0.3\n"
        "-0.3\n"
        "-0.3\n"
        "-0.333\n"
        "-0.333\n"
        "-0.333\n"
        "-0.333\n"
        "    -0.333\n"
        "    -0.333\n"
        "-00000.333\n"
        "-00000.333\n"
        "-0.333    \n"
        "-0.333    \n"
        "    -0.333\n"
        "    -0.333\n"
        "    -0.333\n"
        "    -0.333\n"
        "*** SPACER ***\n"
        "100\n"
        "100\n"
        "100, 100\n"
        "100, 100\n"
        "100 hello world\n"
        "100 hello world\n"
        "100.000\n"
        "100.000\n"
        "1e+02\n"
        "1E+02\n"
        "1.e+02\n"
        "1.E+02\n"
        "100\n"
        "100\n"
        "100.\n"
        "100.\n"
        "       100\n"
        "       100\n"
        "0000000100\n"
        "0000000100\n"
        "100       \n"
        "100       \n"
        "      +100\n"
        "      +100\n"
        "      100.\n"
        "      100.\n"
        "*** SPACER ***\n"
        "-100\n"
        "-100\n"
        "-100, -100\n"
        "-100, -100\n"
        "-100 hello world\n"
        "-100 hello world\n"
        "-100.000\n"
        "-100.000\n"
        "-1e+02\n"
        "-1E+02\n"
        "-1.e+02\n"
        "-1.E+02\n"
        "-100\n"
        "-100\n"
        "-100.\n"
        "-100.\n"
        "      -100\n"
        "      -100\n"
        "-000000100\n"
        "-000000100\n"
        "-100      \n"
        "-100      \n"
        "      -100\n"
        "      -100\n"
        "     -100.\n"
        "     -100.\n"
        "*** SPACER ***\n"
        "3.40282e+38\n"
        "3.40282E+38\n"
        "3.40282e+38, 3.40282e+38\n"
        "3.40282E+38, 3.40282E+38\n"
        "3.40282e+38 hello world\n"
        "3.40282E+38 hello world\n"
        "3.40282e+38\n"
        "3.40282E+38\n"
        "3e+38\n"
        "3E+38\n"
        "3.e+38\n"
        "3.E+38\n"
        "3.4e+38\n"
        "3.4E+38\n"
        "3.40e+38\n"
        "3.40E+38\n"
        "   3.4e+38\n"
        "   3.4E+38\n"
        "0003.4e+38\n"
        "0003.4E+38\n"
        "3.4e+38   \n"
        "3.4E+38   \n"
        "  +3.4e+38\n"
        "  +3.4E+38\n"
        "  3.40e+38\n"
        "  3.40E+38\n"
        "*** SPACER ***\n"
        "-3.40282e+38\n"
        "-3.40282E+38\n"
        "-3.40282e+38, -3.40282e+38\n"
        "-3.40282E+38, -3.40282E+38\n"
        "-3.40282e+38 hello world\n"
        "-3.40282E+38 hello world\n"
        "-3.40282e+38\n"
        "-3.40282E+38\n"
        "-3e+38\n"
        "-3E+38\n"
        "-3.e+38\n"
        "-3.E+38\n"
        "-3.4e+38\n"
        "-3.4E+38\n"
        "-3.40e+38\n"
        "-3.40E+38\n"
        "  -3.4e+38\n"
        "  -3.4E+38\n"
        "-003.4e+38\n"
        "-003.4E+38\n"
        "-3.4e+38  \n"
        "-3.4E+38  \n"
        "  -3.4e+38\n"
        "  -3.4E+38\n"
        " -3.40e+38\n"
        " -3.40E+38\n");
  };

  AddPrimitive(Printf_17_Float_Formatting::num_inputs);
  AddInputBuffer(Printf_17_Float_Formatting::num_inputs,
                 Printf_17_Float_Formatting::Ref);
  this->SetPrintfReference(1, strRef);
  this->RunPrintf1D(1);
}

TEST_P(PrintfExecution, Printf_18_concurrent_printf) {
  fail_if_not_vectorized_ = false;
  const std::string stringPrinted = "Hello world!\n";
  this->RunPrintf1DConcurrent(kts::N, kts::localN,
                              (size_t)stringPrinted.size() * kts::N);
}

TEST_P(PrintfExecution, Printf_19_Print_Halfs) {
  if (!UCL::hasHalfSupport(this->device)) {
    GTEST_SKIP();
  }

  fail_if_not_vectorized_ = false;
  kts::Reference1D<cl_half> in_a = [](size_t) { return 0x5b9a; };
  kts::Reference1D<cl_half> in_b = [](size_t) { return 0xc6ce; };

  ReferencePrintfString ref = [](size_t) {
    return "input: (0x1.e68p+7, -0x1.b38p+2)\n";
  };

  AddInputBuffer(1, in_a);
  AddInputBuffer(1, in_b);
  this->SetPrintfReference(1, ref);
  this->RunPrintf1D(1);
}

TEST_P(PrintfExecution, Printf_20_Multiple_Kernels) {
  fail_if_not_vectorized_ = false;
  ReferencePrintfString ref = [](size_t) {
    return "Hello multiple_kernels Foo 10\n";
  };

  this->SetPrintfReference(1, ref);
  this->RunPrintf1D(1);
}

TEST_P(PrintfExecution, Printf_21_Float_With_Double_Conversion) {
  if (!UCL::hasDoubleSupport(this->device)) {
    GTEST_SKIP();
  }

  fail_if_not_vectorized_ = false;
  ReferencePrintfString ref = [](size_t) { return "4.000000\n"; };

  this->SetPrintfReference(1, ref);
  this->RunPrintf1D(1);
}

TEST_P(PrintfExecution, Printf_22_Half_With_Double_Conversion) {
  if (!UCL::hasHalfSupport(this->device) ||
      !UCL::hasDoubleSupport(this->device)) {
    GTEST_SKIP();
  }

  fail_if_not_vectorized_ = false;

  kts::Reference1D<cl_half> input = [](size_t) { return 0x5b9a; };
  ReferencePrintfString ref = [](size_t) { return "0x1.e68p+7\n"; };

  AddInputBuffer(1, input);
  this->SetPrintfReference(1, ref);

  this->RunPrintf1D(1);
}

TEST_P(PrintfExecutionSPIRV, Printf_23_String_DPCPP) {
  fail_if_not_vectorized_ = false;
  ReferencePrintfString ref = [](size_t) { return "Hello World!\n"; };

  this->SetPrintfReference(1, ref);
  this->RunPrintf1D(1);
}

TEST_P(PrintfExecution, Printf_24_Empty_String_Param) {
  fail_if_not_vectorized_ = false;
  ReferencePrintfString ref = [](size_t) { return "\n"; };

  this->SetPrintfReference(1, ref);
  this->RunPrintf1D(1);
}
