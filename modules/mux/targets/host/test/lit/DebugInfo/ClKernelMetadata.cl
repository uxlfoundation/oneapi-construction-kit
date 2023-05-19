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

// Test Brief:
// Check that cl.kernel metadata is preserved until the end of compilation.

// RUN: %oclc %s -stage cl_snapshot_host_barrier -enqueue test_args > %t
// RUN: %filecheck < %t %s

kernel void test_args(global int *in1,
                      global float2 *in2,
                      const int in3,
                      local float *in4,
                      constant int *in5,
                      global int *in6,
                      global int2 *in7,
                      global int3 *in8,
                      global int4 *in9,
                      global int *out)
{
  out[get_global_id(0)] = in3;
}

//;; the regexes in the following line match extra attributes that clang-level optimizations might add
// CHECK: void @test_args(i32 addrspace(1)* {{[^,]*}}%in1, <2 x float> addrspace(1)* {{[^,]*}}%in2, i32 {{[^,]*}}%in3, float addrspace(3)* {{[^,]*}}%in4, i32 addrspace(2)* {{[^,]*}}%in5, i32 addrspace(1)* {{[^,]*}}%in6, <2 x i32> addrspace(1)* {{[^,]*}}%in7, <3 x i32> addrspace(1)* {{[^,]*}}%in8, <4 x i32> addrspace(1)* {{[^,]*}}%in9, i32 addrspace(1)* {{[^,]*}}%out)

// CHECK: !opencl.kernels = !{![[MD_INDEX:[0-9]+]]}

// CHECK: ![[MD_INDEX]] = !{void (i32 addrspace(1)*, <2 x float> addrspace(1)*, i32, float addrspace(3)*, i32 addrspace(2)*, i32 addrspace(1)*, <2 x i32> addrspace(1)*, <3 x i32> addrspace(1)*, <4 x i32> addrspace(1)*, i32 addrspace(1)*)* @test_args, ![[ADDR_SPACE_INDEX:[0-9]+]], ![[QUAL_INDEX:[0-9]+]], ![[ARG_TYPE:[0-9]+]], ![[ARG_BASE:[0-9]+]], ![[TYPE_QUAL:[0-9]+]], ![[ARG_NAME:[0-9]+]]
// CHECK: ![[ADDR_SPACE_INDEX]] = !{!"kernel_arg_addr_space", i32 1, i32 1, i32 0, i32 3, i32 2, i32 1, i32 1, i32 1, i32 1, i32 1}
// CHECK: ![[QUAL_INDEX]] = !{!"kernel_arg_access_qual", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none"}
// CHECK: ![[ARG_TYPE]] = !{!"kernel_arg_type", !"int*", !"float2*", !"int", !"float*", !"int*", !"int*", !"int2*", !"int3*", !"int4*", !"int*"}
// CHECK: ![[ARG_BASE]] = !{!"kernel_arg_base_type", !"int*", !"float __attribute__((ext_vector_type(2)))*", !"int", !"float*", !"int*", !"int*", !"int __attribute__((ext_vector_type(2)))*", !"int __attribute__((ext_vector_type(3)))*", !"int __attribute__((ext_vector_type(4)))*", !"int*"}
// CHECK: ![[TYPE_QUAL]] = !{!"kernel_arg_type_qual", !"", !"", !"", !"", !"const", !"", !"", !"", !"", !""}
// CHECK: ![[ARG_NAME]] = !{!"kernel_arg_name", !"in1", !"in2", !"in3", !"in4", !"in5", !"in6", !"in7", !"in8", !"in9", !"out"}
