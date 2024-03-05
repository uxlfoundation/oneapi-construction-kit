# Getting Started

## Introduction

The release package is a self-contained pre-built project to demonstrate how to run OpenCL
and SYCL applications on the RISC-V spike simulator, including a deep neural
network demo that uses the VGG network.

We have put together a set of guides that serve as a walkthrough for a selection
of OpenCL and SYCL applications.

The release package contains a number of different directories:

- oneAPI Construction Kit(OCK) is Codeplay’s multi-target, multi-platform framework.
  - A few examples have been added to OCK that are useful for analyzing and demonstrating
    the performance of OCK and oneAPI on RISC-V.
- portDNN and portBLAS network sample project that executes VGG and Resnet50 networks

For dpc++ support, Intel oneAPI nightly releases can be downloaded from [here](https://github.com/intel/llvm/releases). A few sycl examples can be used for testing from [oneAPI-Samples](https://github.com/oneapi-src/oneAPI-samples).

You can run SYCL code using the `vector-add` from oneAPI-samples and samples from
OCK's `examples/applications`. Both directories implement SYCL code and therefore demonstrate
running SYCL code using the oneAPI.

You can also run an implementation of the VGG and Resnet50 neural networks
written using portDNN, SYCL and C++.

## Installation and Setup

OCK demo package contains the build and install directories of all components. All the repositories mentioned above are open-sourced and can be cloned as follows.

- [oneAPI Construction Kit](https://github.com/codeplaysoftware/oneapi-construction-kit.git)
- [portDNN](https://github.com/codeplaysoftware/portDNN.git)
- [portBLAS](https://github.com/codeplaysoftware/portBLAS.git)
- [oneAPI toolkit](https://github.com/intel/llvm/releases)

The above repositories can be built for dpc++ following their respective build instructions.
All of the following instructions have been tested on Ubuntu:20.04.

You'll need to extract the files from the archive file and and oneAPI nighlty release needs to be
downloaded from `intel/llvm`:

```sh
    mkdir Release
    cd Release

    # As the current documentation, the latest ock-demo artifacts available is dated 2024-02-29.
    wget https://github.com/codeplaysoftware/oneapi-construction-kit/releases/download/ock-demo-2024-02-29-f0588da/ock_demo_artifacts.tar.gz
    wget https://github.com/codeplaysoftware/oneapi-construction-kit/releases/download/ock-demo-2024-02-29-f0588da/ock_demo_components.tar.gz
    wget https://github.com/codeplaysoftware/oneapi-construction-kit/releases/download/ock-demo-2024-02-29-f0588da/network_artifacts.tar.gz

    tar -xf ock_demo_artifacts.tar.gz
    tar -xf ock_demo_components.tar.gz
    # ock_demo_components comprises of portBLAS.tar.gz and portDNN.tar.gz
    tar -xf portBLAS.tar.gz
    tar -xf portDNN.tar.gz
    tar -xf network_artifacts.tar.gz

    # As of the current documentation, the latest nightly release available is dated 2024-03-04.
    wget "https://github.com/intel/llvm/releases/download/nightly-2024-03-04/sycl_linux.tar.gz"
    mkdir linux_nightly_release
    tar -xzf sycl_linux.tar.gz -C linux_nightly_release
    rm sycl_linux.tar.gz ock_demo_artifacts.tar.gz ock_demo_components.tar.gz network_artifacts.tar.gz portBLAS_build.tar.gz portDNN_build.tar.gz

    export RELEASE_DIR=$PWD
```

Once you have extracted the files from the archive file, you should have a
hierarchy that looks like this:

```
- WORKDIR: $RELEASE_DIR
    - DIR: ock_install_dir/
    - DIR: portBLAS_build_dir/
    - DIR: portDNN_build_dir/
    - DIR: resnet_data
    - DIR: vgg_data
    - DIR: linux_nightly_release
    - FILE: Labrador_Retriever_Molly.jpg
    - FILE: Labrador_Retriever_Molly.jpg.bin
    - FILE: get-started.md
    - FILE: envvars
```

Then, you'll need to set the environment variables that will be used during this
walkthrough:

```sh
    source $RELEASE_DIR/envvars
```

You can now check that OpenCL is correctly set by running the following command:

```sh
    clinfo | grep "Platform "
```

Following is the expected output:
```sh
  Platform Name                                   ComputeAorta
  Platform Vendor                                 Codeplay Software Ltd.
  Platform Version                                OpenCL 3.0 ComputeAorta 4.0.0 Linux x86_64
  Platform Profile                                FULL_PROFILE
  Platform Extensions                             cl_codeplay_kernel_exec_info cl_codeplay_soft_math cl_khr_create_command_queue cl_khr_icd cl_codeplay_extra_build_options
  Platform Extensions with Version                cl_codeplay_kernel_exec_info                                       0x1000 (0.1.0)
  Platform Numeric Version                        0xc00000 (3.0.0)
  Platform Extensions function suffix             CODEPLAY
  Platform Host timer resolution                  0ns
  Platform Name                                   ComputeAorta
```

## Running the examples

Once you have followed either the local installation or untared prebuilt OCK demo, you are ready to run some OpenCL and SYCL code.

This section will be split in four parts:

- Running the samples from OCK.
- Running the VGG16 and ResNet50 neural networks.

### Running simple-vector-add

OCK demo contains more than one executable, but for the sake of our example, we will be only focusing on the ```simple-vector-add``` one. This sample does what it says, it implements adding two vectors together
using SYCL.

```sh
    cd $RELEASE_DIR/ock_install_dir/tests/
    ./simple-vector-add
```

This is the expected output you should have:

```
The results are correct!
```

Debug tracing can be enabled by setting the `CA_HAL_DEBUG` environment variable to
`1` prior to executing the program to troubleshoot.

```sh
    CA_HAL_DEBUG=1 ./simple-vector-add
```

The output should be:

```
refsi_hal_device::mem_alloc(size=16, align=128) -> 0xb800ff80
refsi_hal_device::mem_write(dst=0xb800ff80, size=16)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0xb800ff00
refsi_hal_device::mem_write(dst=0xb800ff00, size=16)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0xb800fe80
refsi_hal_device::mem_write(dst=0xb800fe80, size=16)
refsi_hal_device::program_find_kernel(name='_ZTS10SimpleVaddIiE.mux-kernel-wrapper') -> 0x000101b2
refsi_hal_device::kernel_exec(kernel=0x000101b2, num_args=6, global=<4:1:1>, local=<4:1:1>)
refsi_hal_device::mem_read(src=0xb800fe80, size=16)
refsi_hal_device::mem_free(address=0xb800fe80)
refsi_hal_device::mem_free(address=0xb800ff00)
refsi_hal_device::mem_free(address=0xb800ff80)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0xb800ff80
refsi_hal_device::mem_write(dst=0xb800ff80, size=16)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0xb800ff00
refsi_hal_device::mem_write(dst=0xb800ff00, size=16)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0xb800fe80
refsi_hal_device::mem_write(dst=0xb800fe80, size=16)
refsi_hal_device::program_find_kernel(name='_ZTS10SimpleVaddIfE.mux-kernel-wrapper') -> 0x00010544
refsi_hal_device::kernel_exec(kernel=0x00010544, num_args=6, global=<4:1:1>, local=<4:1:1>)
refsi_hal_device::mem_read(src=0xb800fe80, size=16)
refsi_hal_device::mem_free(address=0xb800fe80)
refsi_hal_device::mem_free(address=0xb800ff00)
refsi_hal_device::mem_free(address=0xb800ff80)
The results are correct!
```

### Emitting IR

An environment variable can be used to dump the LLVM intermediate representation
created at build time.
Setting the ```CA_RISCV_DUMP_IR``` will display the IR on stderr.

First, check you are in the right directory to run the sample:

```sh
    cd $RELEASE_DIR/ock_install_dir/tests
```

Here's how to dump the IR of the ```simple-vector-add``` kernel:


```sh
    CA_RISCV_DUMP_IR=1 CA_HAL_DEBUG=1 OCL_ICD_FILENAMES=$RELEASE_DIR/install/lib/libCL.so ONEAPI_DEVICE_SELECTOR=opencl:fpga SYCL_CONFIG_FILE_NAME="" ./simple-vector-add
```

This is the expected output you should have:

```
refsi_hal_device::mem_alloc(size=16, align=128) -> 0xb800ff80
refsi_hal_device::mem_write(dst=0xb800ff80, size=16)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0xb800ff00
refsi_hal_device::mem_write(dst=0xb800ff00, size=16)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0xb800fe80
refsi_hal_device::mem_write(dst=0xb800fe80, size=16)
; ModuleID = 'SPIR-V'
source_filename = "SPIR-V"
target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128"
target triple = "riscv64-unknown-unknown-elf"

%"class.sycl::_V1::range" = type { %"class.sycl::_V1::detail::array" }
%"class.sycl::_V1::detail::array" = type { [1 x i64] }

; Function Attrs: nofree norecurse nounwind memory(read, argmem: readwrite)
define spir_kernel void @_ZTSN4sycl3_V16detail19__pf_kernel_wrapperI10SimpleVaddIiEEE(ptr nocapture noundef readonly byval(%"class.sycl::_V1::range") %_arg_UserRange, ptr addrspace(1) nocapture noundef writeonly %_arg_accessorC, ptr nocapture noundef readonly byval(%"class.sycl::_V1::range") %_arg_accessorC3, ptr addrspace(1) nocapture noundef readonly %_arg_accessorA, ptr nocapture noundef readonly byval(%"class.sycl::_V1::range") %_arg_accessorA6, ptr addrspace(1) nocapture noundef readonly %_arg_accessorB, ptr nocapture noundef readonly byval(%"class.sycl::_V1::range") %_arg_accessorB9) local_unnamed_addr #0 {
entry:
  %0 = load i64, ptr %_arg_UserRange, align 8
  %1 = load i64, ptr %_arg_accessorC3, align 8
  %add.ptr.i = getelementptr inbounds i32, ptr addrspace(1) %_arg_accessorC, i64 %1
  %2 = load i64, ptr %_arg_accessorA6, align 8
  %add.ptr.i36 = getelementptr inbounds i32, ptr addrspace(1) %_arg_accessorA, i64 %2
  %3 = load i64, ptr %_arg_accessorB9, align 8
  %add.ptr.i45 = getelementptr inbounds i32, ptr addrspace(1) %_arg_accessorB, i64 %3
  %4 = tail call i64 @__mux_get_global_id(i32 0) #7
  %5 = tail call i64 @__mux_get_global_size(i32 0) #7
  %cmp6.not.i.not.i = icmp ult i64 %4, %0
  br i1 %cmp6.not.i.not.i, label %for.body.i, label %_ZNK4sycl3_V16detail18RoundedRangeKernelINS0_4itemILi1ELb1EEELi1EZZ11simple_vaddIiLm4EEvRKSt5arrayIT_XT0_EESA_RS8_ENKUlRNS0_7handlerEE_clESD_EUlNS0_2idILi1EEEE_EclES4_.exit

for.body.i:                                       ; preds = %entry, %for.body.i
  %Gen.sroa.0.0.i1 = phi i64 [ %add.i13.i, %for.body.i ], [ %4, %entry ]
  %arrayidx.i.i = getelementptr inbounds i32, ptr addrspace(1) %add.ptr.i36, i64 %Gen.sroa.0.0.i1
  %6 = load i32, ptr addrspace(1) %arrayidx.i.i, align 4
  %arrayidx.i7.i = getelementptr inbounds i32, ptr addrspace(1) %add.ptr.i45, i64 %Gen.sroa.0.0.i1
  %7 = load i32, ptr addrspace(1) %arrayidx.i7.i, align 4
  %add.i.i = add nsw i32 %7, %6
  %arrayidx.i11.i = getelementptr inbounds i32, ptr addrspace(1) %add.ptr.i, i64 %Gen.sroa.0.0.i1
  store i32 %add.i.i, ptr addrspace(1) %arrayidx.i11.i, align 4
  %add.i13.i = add i64 %Gen.sroa.0.0.i1, %5
  %cmp6.i.i = icmp ult i64 %add.i13.i, %0
  br i1 %cmp6.i.i, label %for.body.i, label %_ZNK4sycl3_V16detail18RoundedRangeKernelINS0_4itemILi1ELb1EEELi1EZZ11simple_vaddIiLm4EEvRKSt5arrayIT_XT0_EESA_RS8_ENKUlRNS0_7handlerEE_clESD_EUlNS0_2idILi1EEEE_EclES4_.exit

_ZNK4sycl3_V16detail18RoundedRangeKernelINS0_4itemILi1ELb1EEELi1EZZ11simple_vaddIiLm4EEvRKSt5arrayIT_XT0_EESA_RS8_ENKUlRNS0_7handlerEE_clESD_EUlNS0_2idILi1EEEE_EclES4_.exit: ; preds = %for.body.i, %entry
  ret void
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: readwrite)
declare void @llvm.assume(i1 noundef) #1

; Function Attrs: nofree nounwind memory(read, argmem: readwrite, inaccessiblemem: readwrite)
define spir_kernel void @_ZTS10SimpleVaddIiE(ptr addrspace(1) nocapture noundef writeonly %_arg_accessorC, ptr nocapture noundef readonly byval(%"class.sycl::_V1::range") %_arg_accessorC3, ptr addrspace(1) nocapture noundef readonly %_arg_accessorA, ptr nocapture noundef readonly byval(%"class.sycl::_V1::range") %_arg_accessorA6, ptr addrspace(1) nocapture noundef readonly %_arg_accessorB, ptr nocapture noundef readonly byval(%"class.sycl::_V1::range") %_arg_accessorB9) local_unnamed_addr #2 {
entry:
  %0 = load i64, ptr %_arg_accessorC3, align 8
  %add.ptr.i = getelementptr inbounds i32, ptr addrspace(1) %_arg_accessorC, i64 %0
  %1 = load i64, ptr %_arg_accessorA6, align 8
  %add.ptr.i34 = getelementptr inbounds i32, ptr addrspace(1) %_arg_accessorA, i64 %1
  %2 = load i64, ptr %_arg_accessorB9, align 8
  %add.ptr.i43 = getelementptr inbounds i32, ptr addrspace(1) %_arg_accessorB, i64 %2
  %3 = tail call i64 @__mux_get_global_id(i32 0) #7
  %cmp.i.i = icmp ult i64 %3, 2147483648
  tail call void @llvm.assume(i1 %cmp.i.i)
  %arrayidx.i = getelementptr inbounds i32, ptr addrspace(1) %add.ptr.i34, i64 %3
  %4 = load i32, ptr addrspace(1) %arrayidx.i, align 4
  %arrayidx.i47 = getelementptr inbounds i32, ptr addrspace(1) %add.ptr.i43, i64 %3
  %5 = load i32, ptr addrspace(1) %arrayidx.i47, align 4
  %add.i = add nsw i32 %5, %4
  %arrayidx.i51 = getelementptr inbounds i32, ptr addrspace(1) %add.ptr.i, i64 %3
  store i32 %add.i, ptr addrspace(1) %arrayidx.i51, align 4
  ret void
}

; Function Attrs: nofree norecurse nounwind memory(read, argmem: readwrite)
define spir_kernel void @_ZTSN4sycl3_V16detail19__pf_kernel_wrapperI10SimpleVaddIfEEE(ptr nocapture noundef readonly byval(%"class.sycl::_V1::range") %_arg_UserRange, ptr addrspace(1) nocapture noundef writeonly %_arg_accessorC, ptr nocapture noundef readonly byval(%"class.sycl::_V1::range") %_arg_accessorC3, ptr addrspace(1) nocapture noundef readonly %_arg_accessorA, ptr nocapture noundef readonly byval(%"class.sycl::_V1::range") %_arg_accessorA6, ptr addrspace(1) nocapture noundef readonly %_arg_accessorB, ptr nocapture noundef readonly byval(%"class.sycl::_V1::range") %_arg_accessorB9) local_unnamed_addr #3 {
entry:
  %0 = load i64, ptr %_arg_UserRange, align 8
  %1 = load i64, ptr %_arg_accessorC3, align 8
  %add.ptr.i = getelementptr inbounds float, ptr addrspace(1) %_arg_accessorC, i64 %1
  %2 = load i64, ptr %_arg_accessorA6, align 8
  %add.ptr.i36 = getelementptr inbounds float, ptr addrspace(1) %_arg_accessorA, i64 %2
  %3 = load i64, ptr %_arg_accessorB9, align 8
  %add.ptr.i45 = getelementptr inbounds float, ptr addrspace(1) %_arg_accessorB, i64 %3
  %4 = tail call i64 @__mux_get_global_id(i32 0) #7
  %5 = tail call i64 @__mux_get_global_size(i32 0) #7
  %cmp6.not.i.not.i = icmp ult i64 %4, %0
  br i1 %cmp6.not.i.not.i, label %for.body.i, label %_ZNK4sycl3_V16detail18RoundedRangeKernelINS0_4itemILi1ELb1EEELi1EZZ11simple_vaddIfLm4EEvRKSt5arrayIT_XT0_EESA_RS8_ENKUlRNS0_7handlerEE_clESD_EUlNS0_2idILi1EEEE_EclES4_.exit

for.body.i:                                       ; preds = %entry, %for.body.i
  %Gen.sroa.0.0.i1 = phi i64 [ %add.i13.i, %for.body.i ], [ %4, %entry ]
  %arrayidx.i.i = getelementptr inbounds float, ptr addrspace(1) %add.ptr.i36, i64 %Gen.sroa.0.0.i1
  %6 = load float, ptr addrspace(1) %arrayidx.i.i, align 4
  %arrayidx.i7.i = getelementptr inbounds float, ptr addrspace(1) %add.ptr.i45, i64 %Gen.sroa.0.0.i1
  %7 = load float, ptr addrspace(1) %arrayidx.i7.i, align 4
  %add.i.i = fadd float %6, %7
  %arrayidx.i11.i = getelementptr inbounds float, ptr addrspace(1) %add.ptr.i, i64 %Gen.sroa.0.0.i1
  store float %add.i.i, ptr addrspace(1) %arrayidx.i11.i, align 4
  %add.i13.i = add i64 %Gen.sroa.0.0.i1, %5
  %cmp6.i.i = icmp ult i64 %add.i13.i, %0
  br i1 %cmp6.i.i, label %for.body.i, label %_ZNK4sycl3_V16detail18RoundedRangeKernelINS0_4itemILi1ELb1EEELi1EZZ11simple_vaddIfLm4EEvRKSt5arrayIT_XT0_EESA_RS8_ENKUlRNS0_7handlerEE_clESD_EUlNS0_2idILi1EEEE_EclES4_.exit

_ZNK4sycl3_V16detail18RoundedRangeKernelINS0_4itemILi1ELb1EEELi1EZZ11simple_vaddIfLm4EEvRKSt5arrayIT_XT0_EESA_RS8_ENKUlRNS0_7handlerEE_clESD_EUlNS0_2idILi1EEEE_EclES4_.exit: ; preds = %for.body.i, %entry
  ret void
}

; Function Attrs: nofree nounwind memory(read, argmem: readwrite, inaccessiblemem: readwrite)
define spir_kernel void @_ZTS10SimpleVaddIfE(ptr addrspace(1) nocapture noundef writeonly %_arg_accessorC, ptr nocapture noundef readonly byval(%"class.sycl::_V1::range") %_arg_accessorC3, ptr addrspace(1) nocapture noundef readonly %_arg_accessorA, ptr nocapture noundef readonly byval(%"class.sycl::_V1::range") %_arg_accessorA6, ptr addrspace(1) nocapture noundef readonly %_arg_accessorB, ptr nocapture noundef readonly byval(%"class.sycl::_V1::range") %_arg_accessorB9) local_unnamed_addr #4 {
entry:
  %0 = load i64, ptr %_arg_accessorC3, align 8
  %add.ptr.i = getelementptr inbounds float, ptr addrspace(1) %_arg_accessorC, i64 %0
  %1 = load i64, ptr %_arg_accessorA6, align 8
  %add.ptr.i34 = getelementptr inbounds float, ptr addrspace(1) %_arg_accessorA, i64 %1
  %2 = load i64, ptr %_arg_accessorB9, align 8
  %add.ptr.i43 = getelementptr inbounds float, ptr addrspace(1) %_arg_accessorB, i64 %2
  %3 = tail call i64 @__mux_get_global_id(i32 0) #7
  %cmp.i.i = icmp ult i64 %3, 2147483648
  tail call void @llvm.assume(i1 %cmp.i.i)
  %arrayidx.i = getelementptr inbounds float, ptr addrspace(1) %add.ptr.i34, i64 %3
  %4 = load float, ptr addrspace(1) %arrayidx.i, align 4
  %arrayidx.i47 = getelementptr inbounds float, ptr addrspace(1) %add.ptr.i43, i64 %3
  %5 = load float, ptr addrspace(1) %arrayidx.i47, align 4
  %add.i = fadd float %4, %5
  %arrayidx.i51 = getelementptr inbounds float, ptr addrspace(1) %add.ptr.i, i64 %3
  store float %add.i, ptr addrspace(1) %arrayidx.i51, align 4
  ret void
}

; Function Attrs: noinline nounwind optnone
define spir_func void @__itt_offload_wi_start_stub(ptr addrspace(4) %group_id, i64 %wi_id, i32 %wg_size) local_unnamed_addr #5 {
entry:
  %group_id.addr = alloca ptr addrspace(4), align 8
  %wi_id.addr = alloca i64, align 8
  %wg_size.addr = alloca i32, align 4
  %group_id.addr.ascast = addrspacecast ptr %group_id.addr to ptr addrspace(4)
  %wi_id.addr.ascast = addrspacecast ptr %wi_id.addr to ptr addrspace(4)
  %wg_size.addr.ascast = addrspacecast ptr %wg_size.addr to ptr addrspace(4)
  store ptr addrspace(4) %group_id, ptr addrspace(4) %group_id.addr.ascast, align 8
  store i64 %wi_id, ptr addrspace(4) %wi_id.addr.ascast, align 8
  store i32 %wg_size, ptr addrspace(4) %wg_size.addr.ascast, align 4
  ret void
}

; Function Attrs: noinline nounwind optnone
define spir_func void @__itt_offload_wi_finish_stub(ptr addrspace(4) %group_id, i64 %wi_id) local_unnamed_addr #5 {
entry:
  %group_id.addr = alloca ptr addrspace(4), align 8
  %wi_id.addr = alloca i64, align 8
  %group_id.addr.ascast = addrspacecast ptr %group_id.addr to ptr addrspace(4)
  %wi_id.addr.ascast = addrspacecast ptr %wi_id.addr to ptr addrspace(4)
  store ptr addrspace(4) %group_id, ptr addrspace(4) %group_id.addr.ascast, align 8
  store i64 %wi_id, ptr addrspace(4) %wi_id.addr.ascast, align 8
  ret void
}

; Function Attrs: alwaysinline norecurse nounwind memory(read)
declare i64 @__mux_get_global_size(i32) #6

; Function Attrs: alwaysinline norecurse nounwind memory(read)
declare i64 @__mux_get_global_id(i32) #6

attributes #0 = { nofree norecurse nounwind memory(read, argmem: readwrite) "mux-kernel"="entry-point" "mux-orig-fn"="_ZTSN4sycl3_V16detail19__pf_kernel_wrapperI10SimpleVaddIiEEE" "vecz-mode"="auto" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: readwrite) "vecz-mode"="auto" }
attributes #2 = { nofree nounwind memory(read, argmem: readwrite, inaccessiblemem: readwrite) "mux-kernel"="entry-point" "mux-orig-fn"="_ZTS10SimpleVaddIiE" "vecz-mode"="auto" }
attributes #3 = { nofree norecurse nounwind memory(read, argmem: readwrite) "mux-kernel"="entry-point" "mux-orig-fn"="_ZTSN4sycl3_V16detail19__pf_kernel_wrapperI10SimpleVaddIfEEE" "vecz-mode"="auto" }
attributes #4 = { nofree nounwind memory(read, argmem: readwrite, inaccessiblemem: readwrite) "mux-kernel"="entry-point" "mux-orig-fn"="_ZTS10SimpleVaddIfE" "vecz-mode"="auto" }
attributes #5 = { noinline nounwind optnone "vecz-mode"="auto" }
attributes #6 = { alwaysinline norecurse nounwind memory(read) "vecz-mode"="auto" }
attributes #7 = { alwaysinline norecurse nounwind memory(read) }

!llvm.ident = !{!0}
!opencl.kernels = !{!1, !8, !15, !18}
!opencl.ocl.version = !{!21}

!0 = !{!"Source language: OpenCL C++, Version: 100000"}
!1 = !{ptr @_ZTSN4sycl3_V16detail19__pf_kernel_wrapperI10SimpleVaddIiEEE, !2, !3, !4, !5, !6, !7}
!2 = !{!"kernel_arg_addr_space", i32 0, i32 1, i32 0, i32 1, i32 0, i32 1, i32 0}
!3 = !{!"kernel_arg_access_qual", !"none", !"none", !"none", !"none", !"none", !"none", !"none"}
!4 = !{!"kernel_arg_type", !"class sycl::_V1::range*", !"uint*", !"class sycl::_V1::range*", !"uint*", !"class sycl::_V1::range*", !"uint*", !"class sycl::_V1::range*"}
!5 = !{!"kernel_arg_base_type", !"class sycl::_V1::range*", !"uint*", !"class sycl::_V1::range*", !"uint*", !"class sycl::_V1::range*", !"uint*", !"class sycl::_V1::range*"}
!6 = !{!"kernel_arg_type_qual", !"", !"", !"", !"", !"", !"", !""}
!7 = !{!"kernel_arg_name", !"_arg_UserRange", !"_arg_accessorC", !"_arg_accessorC3", !"_arg_accessorA", !"_arg_accessorA6", !"_arg_accessorB", !"_arg_accessorB9"}
!8 = !{ptr @_ZTS10SimpleVaddIiE, !9, !10, !11, !12, !13, !14}
!9 = !{!"kernel_arg_addr_space", i32 1, i32 0, i32 1, i32 0, i32 1, i32 0}
!10 = !{!"kernel_arg_access_qual", !"none", !"none", !"none", !"none", !"none", !"none"}
!11 = !{!"kernel_arg_type", !"uint*", !"class sycl::_V1::range*", !"uint*", !"class sycl::_V1::range*", !"uint*", !"class sycl::_V1::range*"}
!12 = !{!"kernel_arg_base_type", !"uint*", !"class sycl::_V1::range*", !"uint*", !"class sycl::_V1::range*", !"uint*", !"class sycl::_V1::range*"}
!13 = !{!"kernel_arg_type_qual", !"", !"", !"", !"", !"", !""}
!14 = !{!"kernel_arg_name", !"_arg_accessorC", !"_arg_accessorC3", !"_arg_accessorA", !"_arg_accessorA6", !"_arg_accessorB", !"_arg_accessorB9"}
!15 = !{ptr @_ZTSN4sycl3_V16detail19__pf_kernel_wrapperI10SimpleVaddIfEEE, !2, !3, !16, !17, !6, !7}
!16 = !{!"kernel_arg_type", !"class sycl::_V1::range*", !"float*", !"class sycl::_V1::range*", !"float*", !"class sycl::_V1::range*", !"float*", !"class sycl::_V1::range*"}
!17 = !{!"kernel_arg_base_type", !"class sycl::_V1::range*", !"float*", !"class sycl::_V1::range*", !"float*", !"class sycl::_V1::range*", !"float*", !"class sycl::_V1::range*"}
!18 = !{ptr @_ZTS10SimpleVaddIfE, !9, !10, !19, !20, !13, !14}
!19 = !{!"kernel_arg_type", !"float*", !"class sycl::_V1::range*", !"float*", !"class sycl::_V1::range*", !"float*", !"class sycl::_V1::range*"}
!20 = !{!"kernel_arg_base_type", !"float*", !"class sycl::_V1::range*", !"float*", !"class sycl::_V1::range*", !"float*", !"class sycl::_V1::range*"}
!21 = !{i32 3, i32 0}
refsi_hal_device::program_find_kernel(name='_ZTS10SimpleVaddIiE.mux-kernel-wrapper') -> 0x000101b2
refsi_hal_device::kernel_exec(kernel=0x000101b2, num_args=6, global=<4:1:1>, local=<4:1:1>)
refsi_hal_device::mem_read(src=0xb800fe80, size=16)
refsi_hal_device::mem_free(address=0xb800fe80)
refsi_hal_device::mem_free(address=0xb800ff00)
refsi_hal_device::mem_free(address=0xb800ff80)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0xb800ff80
refsi_hal_device::mem_write(dst=0xb800ff80, size=16)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0xb800ff00
refsi_hal_device::mem_write(dst=0xb800ff00, size=16)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0xb800fe80
refsi_hal_device::mem_write(dst=0xb800fe80, size=16)
refsi_hal_device::program_find_kernel(name='_ZTS10SimpleVaddIfE.mux-kernel-wrapper') -> 0x00010544
refsi_hal_device::kernel_exec(kernel=0x00010544, num_args=6, global=<4:1:1>, local=<4:1:1>)
refsi_hal_device::mem_read(src=0xb800fe80, size=16)
refsi_hal_device::mem_free(address=0xb800fe80)
refsi_hal_device::mem_free(address=0xb800ff00)
refsi_hal_device::mem_free(address=0xb800ff80)
The results are correct!
```

### RVV instructions

The RISC-V Vector extension (RVV) enables processor cores based on the RISC-V
instruction set architecture to process data arrays, alongside traditional
scalar operations to accelerate the computation of single instruction streams
on large data sets.

By setting the following environment variables, you will enable the vectorizer
and the RISC-V vector support:

- CA_RISCV_VF
- CA_RISCV_VLEN_BITS_MIN

Here is how to run the simple-vector-add sample with vectorization enabled by
setting the previously shown environment variables:

First, check you are in the right directory to run the sample:

```sh
    cd $RELEASE_DIR/ock_install_dir/tests
```

Then run the command:

```sh
    env CA_RISCV_VLEN_BITS_MIN=128 CA_RISCV_VF=4 ./simple-vector-add
```

The CA_RISCV_VLEN_BITS_MIN variable is used to set the numbers of bits in a
single vector register. This variable must be a power of two and must be no
greater than 4096. It defaults to 128.

The CA_RISCV_VF variable is the vectorization factor that set the number of
elements for the width of the vector.

To see the generated code, you can use the environment variables
to emit IR or assembly.

Furthermore, you can also use the interactive debug mode to see at runtime the
executed instructions which in this case will show you some RVV instructions.
```sh
refsi_hal_device::mem_alloc(size=16, align=128) -> 0xb800ff80
refsi_hal_device::mem_write(dst=0xb800ff80, size=16)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0xb800ff00
refsi_hal_device::mem_write(dst=0xb800ff00, size=16)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0xb800fe80
refsi_hal_device::mem_write(dst=0xb800fe80, size=16)
refsi_hal_device::program_find_kernel(name='__vecz_v4__ZTS10SimpleVaddIiE.mux-kernel-wrapper') -> 0x000102ac
refsi_hal_device::kernel_exec(kernel=0x000102ac, num_args=6, global=<4:1:1>, local=<4:1:1>)
refsi_hal_device::mem_read(src=0xb800fe80, size=16)
refsi_hal_device::mem_free(address=0xb800fe80)
refsi_hal_device::mem_free(address=0xb800ff00)
refsi_hal_device::mem_free(address=0xb800ff80)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0xb800ff80
refsi_hal_device::mem_write(dst=0xb800ff80, size=16)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0xb800ff00
refsi_hal_device::mem_write(dst=0xb800ff00, size=16)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0xb800fe80
refsi_hal_device::mem_write(dst=0xb800fe80, size=16)
refsi_hal_device::program_find_kernel(name='__vecz_v4__ZTS10SimpleVaddIfE.mux-kernel-wrapper') -> 0x000106fe
refsi_hal_device::kernel_exec(kernel=0x000106fe, num_args=6, global=<4:1:1>, local=<4:1:1>)
refsi_hal_device::mem_read(src=0xb800fe80, size=16)
refsi_hal_device::mem_free(address=0xb800fe80)
refsi_hal_device::mem_free(address=0xb800ff00)
refsi_hal_device::mem_free(address=0xb800ff80)
The results are correct!
```

### Running clVectorAddition

The OCK examples/applications directory contains examples and benchmarks that are useful
for analyzing and demonstrating the performance of OCK on RISC-V.

Like the previous subsections, this directory contains more than one executable.

We will focus on ```clVectorAddition``` in this subsection.

Here's how to run ```clVectorAddition```:

```sh
    cd $RELEASE_DIR/ock_install_dir/tests
    ./clVectorAddition
```

This is the expected output you should have:

```
Available platforms are:
  1. ComputeAorta

Selected platform 1

Running example on platform 1
Available devices are:
  1. RefSi G1 RV64

Selected device 1

Running example on device 1
 * Created context
 * Built program
 * Created buffers
 * Created kernel and set arguments
 * Created command queue
 * Enqueued writes to source buffers
 * Enqueued NDRange kernel
 * Enqueued read from destination buffer
 * Result verified
 * Released all created OpenCL objects

Example ran successfully, exiting
```

## Debugging, profiling and examining low level instructions

The following subsections describe how to debug, profile and examine the low
level instructions emitted during program execution using the samples and
benchmarks we previously built.

The following examples will be using the ```simple-vector-add``` kernel from
the `ock_install_dir/tests` directory.

### Profiler

First, check you are in the right directory to run the sample:

```sh
    cd $RELEASE_DIR/ock_install_dir/tests
```

The profiler is a built-in feature that enables you to profile or track
performance of a kernel. It will display statistics gathered during the
execution of that kernel.

There are several ways that you can enable the profiler and store those
information.

Here's the variables that will be used:
- ```CA_PROFILE_LEVEL```
- ```CA_PROFILE_CSV_PATH```

#### CA_PROFILE_LEVEL

The profiler has four distinct levels to perform tracking. Here's how to set
this variable when running the ```simple-vector-add``` kernel:

```sh
    env CA_PROFILE_LEVEL=<profiler's level> ./simple-vector-add
```

Level 0 means that no profiling will be done on the kernel.

Level 1 enables the profiler and will output to standard error the
number of host read and host write as well as the total number of retired
instructions and cycles.

Here's what it looks like when running the ```simple-vector-add``` kernel:

```sh
    env CA_PROFILE_LEVEL=1 ./simple-vector-add
```

```
The results are correct!

[+] total retired instructions: 156
[+] total elapsed cycles: 156
[+] total direct memory write access: 96B
[+] total direct memory read access: 96B
```

If this line is commented out a higher number of instructions is generated and
consequently the assembly is more complicated.

Level 2 enables everything from level 1. It will also write to a file
more information formatted as CSV. The file path by default is
```/tmp/riscv.csv```. We will see later how we can modify this file path.

Here's what the file ```/tmp/riscv.csv``` will contain after running the
```simple-vector-add``` kernel:

```sh
    env CA_PROFILE_LEVEL=2 ./simple-vector-add
```

```
kernel_name,hart,retired_inst,cycles,host_write,host_read,,,,,16,,
,,,,16,,
,,,,16,,
_ZTS10SimpleVaddIiE,0,317,317,,,
_ZTS10SimpleVaddIiE,1,121,121,,,
,,,,,16,
,,,,16,,
,,,,16,,
,,,,16,,
_ZTS10SimpleVaddIfE,0,313,313,,,
_ZTS10SimpleVaddIfE,1,121,121,,,
,,,,,16,  
```

Finally, level 3 enables everything from level 1 and level 2.
It will also write to the same file path formatted as CSV with more information
about the kernel execution.

Here's what the file ```/tmp/riscv.csv``` will contain after running the
```simple-vector-add``` kernel:

```sh
    env CA_PROFILE_LEVEL=3 ./simple-vector-add
```

```
kernel_name,hart,retired_inst,cycles,int_inst,float_inst,branches_inst,mem_read_inst,mem_read_bytes_inst,mem_read_short_inst,mem_read_word_inst,mem_read_double_inst,mem_read_quad_inst,mem_write_inst,mem_write_bytes_inst,mem_write_short_inst,mem_write_word_inst,mem_write_double_inst,mem_write_quad_inst,host_write,host_read,
,,,,,,,,,,,,,,,,,,,16,,
,,,,,,,,,,,,,,,,,,,16,,
,,,,,,,,,,,,,,,,,,,16,,
_ZTS10SimpleVaddIiE,0,317,317,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,,,
_ZTS10SimpleVaddIiE,1,121,121,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,,,
,,,,,,,,,,,,,,,,,,,,16,
,,,,,,,,,,,,,,,,,,,16,,
,,,,,,,,,,,,,,,,,,,16,,
,,,,,,,,,,,,,,,,,,,16,,
_ZTS10SimpleVaddIfE,0,313,313,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,,,
_ZTS10SimpleVaddIfE,1,121,121,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,,,
,,,,,,,,,,,,,,,,,,,,16,
```

#### CA_PROFILE_CSV_PATH

We've previously seen that with the profiler enabled in level 2 and level 3,
the performance analysis information will be written to a default file path.
This file path can be modified by setting the ```CA_PROFILE_CSV_PATH```

Here's how to run the profiler on level 2 and write the gathered information
to your given file path:

```sh
    env CA_PROFILE_LEVEL=3 CA_PROFILE_CSV_PATH=<filepath> ./simple-vector-add
```

### VGG16 and ResNet50

The `portDNN` directory contains two implementations of VGG16 and
ResNet50, one in C++ and one in Python. Example input image and its pre-processed
version are provided in `$RELEASE_DIR`. Also VGG16 and ResNet50 modles are pre-downloaded
in `$RELEASE_DIR/vgg_data` and `$RELEASE_DIR\resnet_data` respectively.

#### Running VGG16 & ResNet50
```
    # Testing on image for VGG16
    CA_HAL_DEBUG=1 CA_PROFILE_LEVEL=3 OCL_ICD_FILENAMES=$RELEASE_DIR/install/lib/libCL.so ONEAPI_DEVICE_SELECTOR=opencl:fpga SYCL_CONFIG_FILE_NAME=  $RELEASE_DIR/portDNN_build_dir/samples/networks/vgg/vgg vgg_data/ $RELEASE_DIR/Labrador_Retriever_Molly.jpg.bin

    # Testing on image for Resnet50
    CA_HAL_DEBUG=1 CA_PROFILE_LEVEL=3 OCL_ICD_FILENAMES=$RELEASE_DIR/install/lib/libCL.so ONEAPI_DEVICE_SELECTOR=opencl:fpga SYCL_CONFIG_FILE_NAME=  $RELEASE_DIR/portDNN_build_dir/samples/networks/resnet50/resnet50 resnet_data/ $(pwd)/Labrador_Retriever_Molly.jpg.bin
```

This may take a few minutes and the expected output should end like this:

```
refsi_hal_device::mem_free(address=0x72bc9a80)
refsi_hal_device::mem_free(address=0xb475ba80)
refsi_hal_device::mem_free(address=0x75209a80)
refsi_hal_device::mem_free(address=0xb475b280)
refsi_hal_device::mem_free(address=0x72b67a80)
refsi_hal_device::mem_free(address=0x72b05a80)
refsi_hal_device::mem_free(address=0x72aa3a80)
refsi_hal_device::mem_free(address=0x9bf5b280)
refsi_hal_device::mem_free(address=0x72a8b280)
refsi_hal_device::mem_free(address=0x9bf57280)
refsi_hal_device::mem_free(address=0x72a87280)
refsi_hal_device::mem_free(address=0x72a83280)
refsi_hal_device::mem_free(address=0x97f57280)
refsi_hal_device::mem_free(address=0x72a7f280)
refsi_hal_device::mem_free(address=0x97f53280)
refsi_hal_device::mem_free(address=0x72a7b280)
refsi_hal_device::mem_free(address=0x72a77280)
refsi_hal_device::mem_free(address=0x96fb3280)
refsi_hal_device::mem_free(address=0x72a73280)
refsi_hal_device::mem_free(address=0x96fb2280)
refsi_hal_device::mem_free(address=0x72a72280)
refsi_hal_device::mem_free(address=0x72a70200)
refsi_hal_device::mem_free(address=0x72a71200)
refsi_hal_device::mem_free(address=0x72a71280)
refsi_hal_device::mem_free(address=0xb7f7cf80)
1332923491164 ns
1333565282439 ns
1332118203098 ns
1331295811286 ns
1330008188846 ns
1332405637563 ns
1332031223909 ns
1332465310963 ns
[+] total retired instructions: 78097436433
[+] total elapsed cycles: 78097436433
[+] total direct memory write access: 579MB
[+] total direct memory read access: 3KB
```