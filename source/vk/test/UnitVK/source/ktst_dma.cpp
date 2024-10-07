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

#include "kts/vecz_tasks_common.h"
#include "ktst_clspv_common.h"

using namespace kts::uvk;

const size_t local_wg_size = 16;

// Vector addition: C[i] = A[i] + B[i];
static kts::Reference1D<cl_int> vaddInA = [](size_t x) {
  return (kts::Ref_Identity(x) * 3) + 27;
};

static kts::Reference1D<cl_int> vaddInB = [](size_t x) {
  return (kts::Ref_Identity(x) * 7) + 41;
};

static kts::Reference1D<cl_int> vaddOutC = [](size_t x) {
  return vaddInA(x) + vaddInB(x);
};

TEST_F(Execution, Dma_01_Direct) {
  if (clspvSupported_) {
    AddInputBuffer(kts::N, vaddInA);
    AddInputBuffer(kts::N, vaddInB);
    AddOutputBuffer(kts::N, vaddOutC);
    RunGeneric1D(kts::N, local_wg_size);
  }
}

const size_t GLOBAL_ITEMS_1D = 4;
const size_t GLOBAL_ITEMS_2D = 4;
const size_t LOCAL_ITEMS_1D = 2;
const size_t LOCAL_ITEMS_2D = 2;
const size_t GLOBAL_ITEMS_TOTAL = GLOBAL_ITEMS_1D * GLOBAL_ITEMS_2D;

class DmaAutoConvolutionExecute : public Execution {
 public:
  void DmaAutoConvolution(bool includeMiddle, cl_uint totalStart,
                          uint32_t maskLoop1, uint32_t maskLoop2) {
    const size_t global_range[] = {GLOBAL_ITEMS_1D, GLOBAL_ITEMS_2D};
    const size_t local_range[] = {LOCAL_ITEMS_1D, LOCAL_ITEMS_2D};

    const size_t srcWidth = GLOBAL_ITEMS_1D + 16;
    const size_t srcHeight = GLOBAL_ITEMS_2D + 8;
    kts::Reference1D<cl_uint> inA = [](size_t x) {
      return kts::Ref_Identity(x);
    };
    kts::Reference1D<cl_uint> RefOutput = [&](size_t x) {
      // First of all work out gidY and gidX
      const cl_uint gidX = kts::Ref_Identity(x) % GLOBAL_ITEMS_1D;
      const cl_uint gidY = kts::Ref_Identity(x) / GLOBAL_ITEMS_1D;
      const cl_uint gsizeX = GLOBAL_ITEMS_1D;
      cl_uint total = totalStart;
      const cl_uint dstYStride = gsizeX;
      const cl_uint srcYStride = dstYStride + 16;
      cl_uint srcIndex = (gidY * srcYStride) + gidX + 8;
      srcIndex += srcYStride;
      for (uint32_t yy = 0; yy < 3; yy++) {
        for (uint32_t xx = 0; xx < 3; xx++) {
          if (!includeMiddle && xx == 1 && yy == 1) {
            continue;
          }
          if (((1 << xx) & maskLoop1) && ((1 << xx) & maskLoop2)) {
            const cl_uint srcIndexLoop = (yy * srcYStride) + srcIndex + xx - 1;
            total = total + inA(srcIndexLoop);
          }
        }
      }
      total = total / (8 + (includeMiddle ? 1 : 0));

      return total;
    };
    AddInputBuffer(srcWidth * srcHeight, inA);
    AddOutputBuffer(GLOBAL_ITEMS_TOTAL, RefOutput);
    AddPrimitive(10);
    RunGenericND(2, global_range, local_range);
  }
};

TEST_F(DmaAutoConvolutionExecute, Dma_06_auto_dma_convolution) {
  if (clspvSupported_) {
    DmaAutoConvolution(false, 8, 7, 7);
  }
}

TEST_F(DmaAutoConvolutionExecute, Dma_07_auto_dma_loop_convolution) {
  if (clspvSupported_) {
    DmaAutoConvolution(true, 9, 7, 7);
  }
}

// See CA-1410
TEST_F(DmaAutoConvolutionExecute,
       Dma_08_auto_dma_loop_convolution_cond_round_inner_loop) {
  if (clspvSupported_) {
    DmaAutoConvolution(true, 9, 7, 2);
  }
}

TEST_F(DmaAutoConvolutionExecute,
       Dma_09_auto_dma_loop_convolution_cond_not_global_id) {
  if (clspvSupported_) {
    DmaAutoConvolution(true, 19, 7, 7);
  }
}
