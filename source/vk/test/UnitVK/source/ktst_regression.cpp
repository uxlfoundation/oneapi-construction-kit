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

#include "ktst_clspv_common.h"

using namespace kts::uvk;

TEST_F(Execution, Regression_06_Cross_Elem4_Zero) {
  if (clspvSupported_) {
    auto refIn1 = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_Float);
    auto refIn2 = kts::BuildVec4Reference1D<cl_float4>(kts::Ref_Float);

    kts::Reference1D<cl_float4> refOut = [](size_t) {
      // cross(x, x) == 0
      cl_float4 v;
      v.data[0] = 0;
      v.data[1] = 0;
      v.data[2] = 0;
      v.data[3] = 0;
      return v;
    };

    AddInputBuffer(kts::N, refIn1);
    AddInputBuffer(kts::N, refIn2);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Regression_10_Dont_Mask_Workitem_Builtins) {
  // Tests for Redmine #8883

  if (clspvSupported_) {
    kts::Reference1D<cl_int> refIn = [](size_t x) {
      return kts::Ref_Identity(x + 2) * 3;
    };
    kts::Reference1D<cl_int> refOut = [](size_t x) {
      const size_t local_id = x % kts::localN;
      if (local_id > 0) {
        return (kts::Ref_Identity(x) + 2) * 3;
      } else {
        return 42;
      }
    };

    AddInputBuffer(kts::N, refIn);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N, kts::localN);
  }
}

TEST_F(Execution, Regression_14_Argument_Stride) {
  if (clspvSupported_) {
    static const cl_int Stride = 3;
    static const cl_int Max = 1 << 30;
    kts::Reference1D<cl_int> refIn = [](size_t x) {
      return kts::Ref_Identity(x) % Max;
    };
    kts::Reference1D<cl_int> refOut = [&refIn](size_t x) {
      return kts::Ref_Identity(x) % Stride == 0 ? refIn(x) : 1;
    };

    AddInputBuffer(kts::N * Stride, refIn);
    AddOutputBuffer(kts::N * Stride, refOut);
    AddPrimitive(Stride);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Regression_15_Negative_Stride) {
  if (clspvSupported_) {
    const cl_int MaxIndex = static_cast<cl_int>(kts::N) - 1;
    kts::Reference1D<cl_int> refIn = [](size_t x) {
      return static_cast<cl_int>(x * x);
    };
    kts::Reference1D<cl_int> refOut = [MaxIndex, refIn](size_t x) {
      return refIn(MaxIndex - x) + refIn(x);
    };

    AddInputBuffer(kts::N, refIn);
    AddOutputBuffer(kts::N, refOut);
    AddPrimitive(MaxIndex);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Regression_16_Negative_Argument_Stride) {
  if (clspvSupported_) {
    const cl_int MaxIndex = static_cast<cl_int>(kts::N) - 1;
    kts::Reference1D<cl_int> refIn = [](size_t x) {
      return static_cast<cl_int>(x * x);
    };
    kts::Reference1D<cl_int> refOut = [MaxIndex, refIn](size_t x) {
      return refIn(MaxIndex - x) + refIn(x);
    };

    AddInputBuffer(kts::N, refIn);
    AddOutputBuffer(kts::N, refOut);
    AddPrimitive(-1);
    AddPrimitive(MaxIndex);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Regression_17_Scalar_Select_Transform) {
  if (clspvSupported_) {
    // Inputs are not important, since this bug caused a compilation failure
    // because a function was called with the wrong arguments.
    kts::Reference1D<cl_int4> refA = [](size_t x) -> cl_int4 {
      const cl_int A = kts::Ref_A(x);
      return cl_int4{{A, A, A, A}};
    };
    kts::Reference1D<cl_int4> refB = [](size_t x) -> cl_int4 {
      const cl_int B = kts::Ref_B(x);
      return cl_int4{{B, B, B, B}};
    };
    kts::Reference1D<cl_int4> refOut = [&refA, &refB](size_t x) {
      return x % 2 == 0 ? refA(x) : refB(x);
    };

    AddInputBuffer(kts::N, refA);
    AddInputBuffer(kts::N, refB);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Regression_18_Uniform_Alloca) {
  if (clspvSupported_) {
    kts::Reference1D<cl_int> refOut = [](size_t x) -> cl_int {
      if (x == 0 || x == 1) {
        return kts::Ref_A(x);
      } else if (x % 2 == 0) {
        return 11;
      } else {
        return 13;
      }
    };

    AddInputBuffer(2, kts::Ref_A);
    AddOutputBuffer(kts::N * 2, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Regression_19_Memcpy_Optimization) {
  if (clspvSupported_) {
    // This tests assumes that clang will optimize the struct copying into a
    // memcpy.
    kts::Reference1D<cl_int4> refIn = [](size_t x) {
      const cl_int v = kts::Ref_Identity(x);
      return cl_int4{{v, v + 11, v + 12, v + 13}};
    };

    AddInputBuffer(kts::N, refIn);
    AddOutputBuffer(kts::N, refIn);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Regression_28_Uniform_Atomics) {
  if (clspvSupported_) {
    // The output buffers are default-initialized, so integers they are
    // zeroed-out.
    kts::Reference1D<cl_int> refOut = [](size_t) {
      return static_cast<cl_int>(kts::localN);
    };
    AddOutputBuffer(1, refOut);
    RunGeneric1D(kts::localN, kts::localN);
  }
}

TEST_F(Execution, Regression_29_Divergent_Memfence) {
  if (clspvSupported_) {
    // Inputs/outputs are unimportant
    AddPrimitive(1);
    AddOutputBuffer(kts::N, kts::Ref_Identity);
    RunGeneric1D(kts::N);
  }
}

// This test was added to trigger assertions and crashes in the X86 LLVM
// backend when we try to vectorize by the entire x-dimension (i.e. potentially
// very wide vectors).
TEST_F(Execution, Regression_34_Codegen_1) {
  if (clspvSupported_) {
    // This test particularly needs a local workgroup size of 512, so make sure
    // that the global size can accommodate that.
    const int items = std::max<int>(static_cast<int>(kts::N), 1024);
    const cl_int reps = 4;  // How many entries each work item should process.
    const int size = items * reps;

    kts::Reference1D<cl_int> refSize = [&size](size_t) { return size; };

    auto refIn = kts::Ref_Identity;
    kts::Reference1D<cl_int> refOut = [=, &refIn](size_t x) {
      cl_int sum = 0;
      for (size_t i = x * reps; i < (x + 1) * reps; i++) {
        sum += refIn(i);
      }
      return sum * 3;  // Three for three input arrays.
    };

    AddInputBuffer(refSize(0), refIn);
    AddInputBuffer(refSize(1), refIn);
    AddInputBuffer(refSize(2), refIn);
    AddOutputBuffer(items, refOut);
    AddInputBuffer(3, refSize);
    AddPrimitive(reps);
    RunGeneric1D(items, 512);
  }
}

// This test was added to trigger assertions and crashes in the X86 LLVM
// backend when we try to vectorize by the entire x-dimension (i.e. potentially
// very wide vectors).  Note that this test triggered a different crash than
// Regression_34_Codegen_1.
TEST_F(Execution, Regression_34_Codegen_2) {
  if (clspvSupported_) {
    // This test particularly needs a local workgroup size of 256, so make sure
    // that the global size can accommodate that.
    const int items = std::max<int>(static_cast<int>(kts::N), 512);
    const cl_int reps = 4;  // How many entries each work item should process.
    const cl_int size = items * reps;

    auto refIn = kts::Ref_Identity;
    kts::Reference1D<cl_int> refOut = [=, &refIn](size_t x) {
      cl_int sum = 0;
      for (size_t i = x * reps; i < (x + 1) * reps; i++) {
        sum += refIn(i);
      }
      return sum;
    };

    AddInputBuffer(size, refIn);
    AddOutputBuffer(items, refOut);
    AddPrimitive(size);
    AddPrimitive(reps);
    RunGeneric1D(items, 256);
  }
}

// At the moment this test crashes clspv
TEST_F(Execution, Regression_37_CFC) {
  if (clspvSupported_) {
    const cl_int limit = static_cast<cl_int>(kts::N / 2);
    kts::Reference1D<cl_int> refOut = [limit](size_t x) {
      const cl_int ix = kts::Ref_Identity(x);
      return ix < limit ? ix : kts::Ref_A(ix % 32);
    };
    AddInputBuffer(limit, kts::Ref_A);
    AddOutputBuffer(kts::N, refOut);
    AddPrimitive(limit);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Regression_43_Scatter_Gather) {
  if (clspvSupported_) {
    kts::Reference1D<cl_int> refOut = [](size_t x) { return (cl_int)(x * 7); };
    kts::Reference1D<cl_int> refIn = [](size_t x) {
      return (cl_int)((x + 1) * 7);
    };
    AddPrimitive(64);
    AddInputBuffer(kts::N, refIn);
    AddOutputBuffer(kts::N + 1, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Regression_51_Local_phi) {
  if (clspvSupported_) {
    kts::Reference1D<cl_int> refOut = [](size_t x) {
      return static_cast<cl_int>(x);
    };

    AddOutputBuffer(kts::N / kts::localN, refOut);
    RunGeneric1D(kts::N, kts::localN);
  }
}

TEST_F(Execution, Regression_52_Nested_Loop_Using_Kernel_Arg) {
  if (clspvSupported_) {
    kts::Reference1D<cl_int> refIn = [](size_t) { return 42; };

    kts::Reference1D<cl_int> refOut = [](size_t) { return 42; };

    AddInputBuffer(kts::N, refIn);
    AddOutputBuffer(kts::N, refOut);
    RunGeneric1D(kts::N);
  }
}

TEST_F(Execution, Regression_54_Negative_Comparison) {
  if (clspvSupported_) {
    kts::Reference1D<cl_float> outRef = [](size_t x) -> cl_float {
      return 4.0f * x;
    };

    AddOutputBuffer(4, outRef);
    AddPrimitive(10);
    AddPrimitive(10);
    RunGeneric1D(4, 4);
  }
}

using ktst_regression_array_spec =
    kts::uvk::GenericKernelTest<uvk::Shader::kts_array_spec>;
TEST_F(ktst_regression_array_spec, RegressionTest) {
  glsl::uintTy size = 16;

  AddInputBuffer(size, kts::Ref_Float);
  AddOutputBuffer(size, kts::Ref_Float);

  setUpShaderModule();
  VkShaderModule shaderModule = getShaderModule().value();
  VkPipelineLayout pipelineLayout = getPipelineLayout().value();

  const VkSpecializationMapEntry entries[] = {{0, 0, sizeof(glsl::uintTy)}};

  const VkSpecializationInfo spec_info = {
      1,                         // mapEntryCount
      entries,                   // pMapEntries
      1 * sizeof(glsl::uintTy),  // dataSize
      &size                      // pData
  };

  VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
  shaderStageCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageCreateInfo.module = shaderModule;
  shaderStageCreateInfo.pName = "main";
  shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageCreateInfo.pSpecializationInfo = &spec_info;

  VkComputePipelineCreateInfo pipelineCreateInfo = {};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.layout = pipelineLayout;
  pipelineCreateInfo.stage = shaderStageCreateInfo;

  VkPipeline pipeline_;
  vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo,
                           nullptr, &pipeline_);
  providePipeline(pipeline_);

  uint32_t global[3] = {1, 1, 1};
  RunGeneric(global);
}

using ktst_regression_array_spec_op =
    kts::uvk::GenericKernelTest<uvk::Shader::kts_array_spec_op>;
TEST_F(ktst_regression_array_spec_op, RegressionTest) {
  glsl::uintTy sizeArr[2] = {4, 12};

  const size_t size = sizeArr[0] + sizeArr[1];
  AddInputBuffer(size, kts::Ref_Float);
  AddOutputBuffer(size, kts::Ref_Float);

  setUpShaderModule();
  VkShaderModule shaderModule = getShaderModule().value();
  VkPipelineLayout pipelineLayout = getPipelineLayout().value();

  const VkSpecializationMapEntry entries[] = {
      {0, 0, sizeof(glsl::uintTy)},
      {1, sizeof(glsl::uintTy), sizeof(glsl::uintTy)}};

  const VkSpecializationInfo spec_info = {
      2,                         // mapEntryCount
      entries,                   // pMapEntries
      2 * sizeof(glsl::uintTy),  // dataSize
      sizeArr                    // pData
  };

  VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
  shaderStageCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageCreateInfo.module = shaderModule;
  shaderStageCreateInfo.pName = "main";
  shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageCreateInfo.pSpecializationInfo = &spec_info;

  VkComputePipelineCreateInfo pipelineCreateInfo = {};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.layout = pipelineLayout;
  pipelineCreateInfo.stage = shaderStageCreateInfo;

  VkPipeline pipeline_;
  vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo,
                           nullptr, &pipeline_);
  providePipeline(pipeline_);

  uint32_t global[3] = {1, 1, 1};
  RunGeneric(global);
}

using ktst_regression_workgroup_spec =
    kts::uvk::GenericKernelTest<uvk::Shader::kts_workgroup_spec>;
TEST_F(ktst_regression_workgroup_spec, RegressionTest) {
  uint32_t local[3] = {4, 1, 1};
  uint32_t global[3] = {4, 1, 1};
  const glsl::uintTy size = global[0] * local[0];

  AddInputBuffer(size, kts::Ref_Float);
  AddOutputBuffer(size, kts::Ref_Float);

  RunGeneric(global, local);
}

using ktst_regression_workgroup_spec_mixed =
    kts::uvk::GenericKernelTest<uvk::Shader::kts_workgroup_spec_mixed>;
TEST_F(ktst_regression_workgroup_spec_mixed, RegressionTest) {
  const int localY = 2;
  glsl::uintTy specData[2] = {2, 2};
  uint32_t global[3] = {8, 1, 1};

  const size_t size = localY * specData[0] * specData[1] * global[0];
  AddInputBuffer(size, kts::Ref_Float);
  AddOutputBuffer(size, kts::Ref_Float);

  setUpShaderModule();
  VkShaderModule shaderModule = getShaderModule().value();
  VkPipelineLayout pipelineLayout = getPipelineLayout().value();

  const VkSpecializationMapEntry entries[] = {
      {0, 0, sizeof(glsl::uintTy)},
      {1, sizeof(glsl::uintTy), sizeof(glsl::uintTy)}};

  const VkSpecializationInfo spec_info = {
      2,                         // mapEntryCount
      entries,                   // pMapEntries
      2 * sizeof(glsl::uintTy),  // dataSize
      specData                   // pData
  };

  VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
  shaderStageCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageCreateInfo.module = shaderModule;
  shaderStageCreateInfo.pName = "main";
  shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageCreateInfo.pSpecializationInfo = &spec_info;

  VkComputePipelineCreateInfo pipelineCreateInfo = {};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.layout = pipelineLayout;
  pipelineCreateInfo.stage = shaderStageCreateInfo;

  VkPipeline pipeline_;
  vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo,
                           nullptr, &pipeline_);
  providePipeline(pipeline_);

  RunGeneric(global);
}

using ktst_regression_uniform_outside_main =
    kts::uvk::GenericKernelTest<uvk::Shader::kts_uniform_outside_main>;
TEST_F(ktst_regression_uniform_outside_main, RegressionTest) {
  const int32_t pushConstant = 42;

  kts::Reference1D<int32_t> inRef = [](size_t x) -> int32_t { return x; };
  kts::Reference1D<int32_t> outRef = [&](size_t x) -> int32_t {
    return x + pushConstant;
  };

  AddInputBuffer(kts::N, inRef);
  AddOutputBuffer(kts::N, outRef);

  setUpDescriptorSetLayout();
  VkDescriptorSetLayout descriptorSetLayout = getDescriptorSetLayout().value();

  VkPushConstantRange pushConstantRange = {};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(int32_t);

  VkPipelineLayoutCreateInfo pLayoutCreateInfo = {};
  pLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pLayoutCreateInfo.setLayoutCount = 1;
  pLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
  pLayoutCreateInfo.pushConstantRangeCount = 1;
  pLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

  VkPipelineLayout pipelineLayout;
  vkCreatePipelineLayout(device, &pLayoutCreateInfo, nullptr, &pipelineLayout);

  providePipelineLayout(pipelineLayout);

  // To get the push constant command into the command buffer it needs to be
  // recorded into a secondary command buffer which will get executed in
  // `RunGeneric1D` after we provide it to the fixture below. If it was recorded
  // directly into the primary it would be overwritten when the dispatch
  // commands are recorded.
  VkCommandBuffer secondaryCommandBuffer;

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandBufferCount = 1;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  allocInfo.commandPool = commandPool;

  vkAllocateCommandBuffers(device, &allocInfo, &secondaryCommandBuffer);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  vkBeginCommandBuffer(secondaryCommandBuffer, &beginInfo);
  vkCmdPushConstants(secondaryCommandBuffer, pipelineLayout,
                     VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int32_t),
                     &pushConstant);
  vkEndCommandBuffer(secondaryCommandBuffer);

  provideSecondaryCommandBuffer(secondaryCommandBuffer);

  RunGeneric1D(kts::N);
}
