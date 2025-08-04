# OCK CI Overview

## Workflows: listing & types

### Listing

`CodeQL`: codeql.yml
- description: runs the CodeQL tool

`create llvm artefacts`:
create_llvm_artefacts.yml
- description: creates llvm artefacts

`Build and Package`: create_publish_artifacts.yml
- description: builds and packages publish artefacts

`Build documentation`: docs.yml
- description: builds docs for PR testing

`Run planned testing`: planned_testing_caller.yml
- description: runs planned_testing-style tests, called from an llvm version caller

`run planned tests for llvm 19`: planned_testing_caller_19.yml
- description: runs planned_tests for llvm 19

`run planned tests for llvm 20`: planned_testing_caller_20.yml
- description: runs planned_tests for llvm 20

`run planned tests for llvm 21`: planned_testing_caller_21.yml
- description: runs planned_tests for llvm 21

`run full planned tests for experimental llvm main`: planned_testing_caller_main.yml
- description: runs planned_tests for experimental llvm main

`run limited planned tests for experimental llvm main`: planned_testing_caller_mini_main.yml
- description: runs limited planned_tests for experimental llvm main

`Seed the cache for ock builds`: pr_tests_cache.yml
- description: seeds the cache for OCK builds

`publish docker images`: publish_docker_images.yml 
- description: builds and publishes docker images

`Run external tests`: run_ock_external_tests.yml
- description: runs external OCK tests

`Run ock internal tests`: run_ock_internal_tests.yml
- description: runs internal OCK tests

`Run ock tests for PR style testing`: run_pr_tests_caller.yml
- description: runs PR-style tests

`Scorecard supply-chain security`: scorecard.yml
- description: runs scorecard analysis and reporting

`Create a cache OpenCL-CTS artefact`: create_opencl_cts_artefact.yml
- description: Workflow for creating and caching OpenCL-CTS artefact

### `schedule:` workflows

`CodeQL`: codeql.yml

`run planned tests for llvm 19`: planned_testing_caller_19.yml

`run planned tests for llvm 20`: planned_testing_caller_20.yml

`run planned tests for llvm 21`: planned_testing_caller_21.yml

`run full planned tests for experimental llvm main`: planned_testing_caller_main.yml

`run limited planned tests for experimental llvm main`: planned_testing_caller_mini_main.yml

`Scorecard supply-chain security`: scorecard.yml

### `workflow_dispatch:` workflows (available in forks)

`run planned tests for llvm 19`: planned_testing_caller_19.yml

`run planned tests for llvm 20`: planned_testing_caller_20.yml

`run planned tests for llvm 21`: planned_testing_caller_21.yml

`Seed the cache for ock builds`: pr_tests_cache.yml

`Build and Package`: create_publish_artifacts.yml

`Build documentation`: docs.yml

`Run ock internal tests`: run_ock_internal_tests.yml

`Create a cache OpenCL-CTS artefact`: create_opencl_cts_artefact.yml

### PR workflow

`Run ock internal tests`: run_ock_internal_tests.yml

## Docker: images, dockerfiles and workflow
### Container images
OCK CI container images can be found under the [uxlfoundation repo packages tab](https://github.com/orgs/uxlfoundation/packages):
```
       ock_ubuntu_22.04-x86-64:latest
       ock_ubuntu_22.04-aarch64:latest
       ock_ubuntu_24.04-x86-64:latest
```

### Dockerfiles and build workflow
Corresponding dockerfiles used to build the above container images can be found in the repo [dockerfile folder](https://github.com/uxlfoundation/oneapi-construction-kit/tree/main/.github/dockerfiles):
```
       Dockerfile_22.04-x86-64
       Dockerfile_22.04-aarch64
       Dockerfile_24.04-x86-64
```
The `publish docker images` workflow is configured to rebuild the containers when any dockerfile update is pushed to main.

## LLVM management
(tbd)

## Adding Self-Hosted Runners
OCK CI runs on standard Github runners. References to these runners can be replaced by references to self-hosted runners by updating individual `runs-on:` settings in the CI config to use an appropriate self-hosted runner string.

Further information on deploying self-hosted runners can be found in the [github docs](https://docs.github.com/en/actions/concepts/runners/self-hosted-runners).
Further information on `runs-on:` can also be found in the [github docs](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax#jobsjob_idruns-on).

## Setting up PR testing pre-requisites
A number of individual PR test jobs in the `Run ock internal tests` (PR testing) workflow include a testing phase which involves calling the `run_cities.py` script to execute a portion of the tests. This phase also requires the pre-built `opencl_cts_host_x86_64_linux` opencl_cts artifact which must be available in repo cache prior to running these tests. If this artifact is not provided in the cache the PR tests workflow will fail.

The opencl_cts artifact concerned can be built and cached by calling the `Create a cache OpenCL-CTS artifact` workflow from the web interface (i.e. via a `workflow_dispatch:` manual event trigger) in advance of running the PR tests. This workflow can be called in forks.
There are a number of inputs to this workflow which relate to Git checkout references in OpenCL repos. The default values for these at time of writing are:
```
      header_ref:
        description: 'Git checkout ref for OpenCL Headers repo'
        default: 'v2025.06.13'
      icd_loader_ref:
        description: 'Git checkout ref for OpenCL ICD Loader repo'
        default: 'v2024.10.24'
      opencl_cts_ref:
        description: 'Git checkout ref for OpenCL-CTS repo'
        default: 'v2025-04-21-00'
```
These default values can also be updated interactively on a per-run basis when called from the web interface. 

At the point at which an update to the opencl_cts cache artifact is required (e.g. when new Git checkout references are available and the workflow inputs default values shown above have been updated accordingly) the existing artifact should be manually deleted prior to re-running the artefact creation workflow. The update workflow will fail if an existing cached artifact is found. Consideration should be given to avoid impacting any in-progress PRs referencing the previous opencl_cts cache artefact version.

## Running planned_testing workflows
### Planned_testing workflows in forks
Planned_testing workflows are configured to run via `workflow_dispatch:` (manual event trigger) in forks. Examples can be found [in this fork](https://github.com/AERO-Project-EU/oneapi-construction-kit/actions?query=event%3Aworkflow_dispatch).

### Tailoring planned_testing workflows
The following planned_testing workflows call `Run planned testing` (planned_testing_caller.yml) as a sub-workflow:
```
      run planned tests for llvm 19: planned_testing_caller_19.yml
      run planned tests for llvm 20: planned_testing_caller_20.yml
      run planned tests for llvm 21: planned_testing_caller_21.yml
```
They can be tailored to run specific llvm versions (e.g. 19), target lists (e.g. host_x86_64_linux) and test options (e.g. test_sanitizers), etc., by setting the `inputs:` values to `Run planned testing` accordingly. See the planned_testing workflow .yml files for examples of current default values and tailoring options. Note that tailored values should be set directly in the workflow config and cannot currently be updated interactively on a per-run basis when called from the web interface. 

<!---
Docs: add readme in .github
====
* workflows
- scheduled
- pr
* dockers
- ours
* link to self-hosted runners
* llvm
- cached
- installed
-
* callable workflows
* opencl-cts artifact for run_cities.

Uwe's suggestions
===
* where to find the CI runs (where the latest)
* how to schedule a pipeline and select test jobs (SYCL-CTS, e2e, OpenCL CTS, etc ) on the three platforms (x86, risc_v, Aarch64) where applicable 
* perhaps how to cancel a running pipeline (if that’s possible)
* what the limitations are if any (if there are restrictions on users, maximum number of launches to prevent swamping CI.)
* This documentation should ideally also work for the forks (especially AERO/SYCLOPS)

# oneAPI Construction Kit

The oneAPI Construction Kit is a framework to provide implementations of open standards, such as OpenCL, for a wide range of devices. The oneAPI Construction Kit can be used to build with the oneAPI Toolkit. The oneAPI Toolkit includes support for various open standards, such as OpenMP, SYCL, and DPC++. DPC++ is based on the SYCL programming model, which allows to write single-source C++ code that can target both CPUs and GPUs. To get more information on oneAPI, please visit https://www.intel.com/content/www/us/en/developer/tools/oneapi/overview.html.

The oneAPI Construction Kit is part of the [UXL Foundation].

[UXL Foundation]: http://www.uxlfoundation.org

>**_Note:_**
 It is not intended to be used as a standalone OpenCL implementation. It does not support the oneAPI Level Zero API.

For more information about building, implementing and maintaining the oneAPI Construction Kit please take the time to read the [developer guide](doc/developer-guide.md) and the other documentation in the
`doc` directory.

See [LICENSE.txt](LICENSE.txt) for details about the license for the code base,
and external components.

>**_Note:_**
   oneAPI Construction Kit was previously referred to as ComputeAorta and referred to as acronym `CA`. As a result, references to ComputeAorta or CA may be present in some oneAPI Construction Kit's documentation and code.

## Get started with the oneAPI Construction Kit
This section provides the minimum system requirements for building the oneAPI Construction Kit on Ubuntu 22.04. For Windows platform dependencies and build instructions please refer to the [developer guide](doc/developer-guide.md). There is a [blog post](https://codeplay.com/portal/blogs/2023/06/05/introducing-the-oneapi-construction-kit) demonstrating how to build the kit for a simulated RISC-V target. You can also find the documentation on [Codeplay's developer website](https://developer.codeplay.com/products/oneapi/construction-kit/home/).

### Platform Dependencies
* [GCC](https://gcc.gnu.org/)
* [Git](https://git-scm.com/)
* [CMake](https://cmake.org/) 3.16+
* [Python](https://www.python.org/) 3.6.9+
* [Visual Studio](https://www.visualstudio.com/) 2017 or 2019 (for Windows)

To install the dependencies on Ubuntu, open the terminal and run:
```sh
   $ sudo apt update
   $ sudo apt install -y build-essential git cmake libtinfo-dev python3
```

### Recommended packages
* [Ninja](https://ninja-build.org/)
* [clang-format](https://clang.llvm.org/docs/ClangFormat.html) 16
* [lit](https://llvm.org/docs/CommandGuide/lit.html)

To install the recommended packages, run:
```sh
   $ sudo apt install -y ninja-build doxygen python3-pip
   $ sudo pip3 install lit virtualenv cmakelint clang-format==19.1.0
```

### Compiling oneAPI Construction Kit
To compile the oneAPI Construction Kit, LLVM needs to be installed and linked against. The build process requires the use of tools from LLVM when the runtime compiler is enabled. The user can either follow the [LLVM guide](doc/developer-guide.md#compiling-llvm) to build a suitable install or follow the [without LLVM guide](doc/developer-guide.md#compiling-the-oneapi-construction-kit-without-llvm) to compile the oneAPI Construction Kit with the runtime compiler disabled.

Examples are provided to get started, but for more control over the compilation process, the user can consult the list of CMake options.

The oneAPI Construction Kit can be compiled for two reference targets; `host` and `refsi` (`riscv`). In SYCL programming, the host target refers to the system where the SYCL program is compiled and executed, while the refsi (Reference System Implementation) target refers to the target platform for which the program is being developed. The refsi target is a hardware-specific implementation of the SYCL specification, enabling the program to run on a specific target platform. SYCL implementations such as DPC++ provide various refsi targets for CPUs, GPUs, FPGAs, and accelerators, which can be selected during compilation using specific flags and code.

#### Compiling oneAPI Construction Kit for host

To compile oneAPI Construction Kit for the host, please refer to the [developer guide](doc/developer-guide.md#compiling-oneapi-construction-kit).

#### Compiling oneAPI Construction Kit for simulated RISC-V

This target aims to provide a flexible way of communicating with various customer RISC-V targets with different configurations. It supports multiple variants using an abstract class and can configure targets and execute commands. However, the current version has only been tested on an x86_64 host CPU.

This target is not intended for running oneAPI Construction Kit directly on RISC-V hardware. For that, the host target should be used.

The available targets in the current implementation are based on Codeplay's reference architecture, called RefSi, with two variations: `G` and `M1`. The `riscv` target is designed to support the `G` variant, while the `M1` variant has additional features like DMA. More information on `riscv` can be found [here](doc/modules/riscv.rst). To build in-tree, run the following:

```sh
cmake -GNinja \
   -Bbuild-riscv \
   -DCA_MUX_TARGETS_TO_ENABLE="riscv" \
   -DCA_LLVM_INSTALL_DIR=$LLVMInstall \
   -DCA_ENABLE_HOST_IMAGE_SUPPORT=OFF \
   -DCA_CL_ENABLE_ICD_LOADER=ON
ninja -C build-riscv install
```

> **_Note:_**
  The installed LLVM must have RISCV as an enabled target and build ``lld`` with
  ``-DLLVM_ENABLE_PROJECTS='clang;lld'``.

### Cross-compiling oneAPI Construction Kit
When cross-compiling with CMake, you need to set the `CMAKE_TOOLCHAIN_FILE` variable to tell CMake how to compile for the target architecture. This file sets up various CMake variables, such as the locations of the C and C++ compilers, the assembler, the linker, and the target file system root. By setting these variables correctly, CMake can generate the appropriate build system files for the target platform. More information regarding cross compiling of the oneAPI Construction Kit, can be found [here](doc/developer-guide.md#cross-platform-building-llvm-and-oneapi-construction-kit-for-linux)

### Compiling oneAPI-samples vector-add using official Intel oneAPI Base Toolkit
The official Intel OneAPI Base Toolkit can be obtained by visiting the following link: [Intel OneAPI Base Toolkit Download](https://www.intel.com/content/www/us/en/developer/tools/oneapi/base-toolkit-download.html). On this page, specify the operating system, the type of installer needed, and the desired version to access the download options. The initial download is for the installer application files only. The installer will acquire all the tools during the installation process. From the console, locate the downloaded install file.

```sh
# To launch the GUI installer as the root
sudo sh ./<installer>.sh
```
Or
```sh
# To launch the GUI installer as the current user.
sh ./<installer>.sh
```

Follow the instructions in the installer. And explore the Get Started Guide to get more information.

For example, for Linux, online installer and version 2023.2.0, follow the instructions below:

```sh
wget https://registrationcenter-download.intel.com/akdlm/IRC_NAS/992857b9-624c-45de-9701-f6445d845359/l_BaseKit_p_2023.2.0.49397.sh

sudo sh ./l_BaseKit_p_2023.2.0.49397.sh
```

To acquire the oneAPI-samples, clone it as follows.

```sh
git clone https://github.com/oneapi-src/oneAPI-samples.git
```

Now to compile the vector add from oneAPI samples, set the environment variables and follow the steps given below:

```sh
export OCL_ICD_FILENAMES=$ONEAPI_CON_KIT_INSTALL_DIR/lib/libCL.so
export LD_LIBRARY_PATH=/path/to/intel/oneapi/compiler/2023.2.0/linux/lib:/path/to/intel/oneapi/compiler/2023.2.0/linux/compiler/lib/intel64_lin:/path/to/intel/oneapi/compiler/2023.2.0/linux/compiler/lib/:$LD_LIBRARY_PATH
export ONEAPI_DEVICE_SELECTOR="*:fpga"

/path/to/intel/oneapi/compiler/2023.2.0/linux/bin-llvm/clang++ -fsycl /path/to/oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/vector-add/src/vector-add-buffers.cpp -o vector-add-buffers
CA_HAL_DEBUG=1 SYCL_CONFIG_FILE_NAME=  ./vector-add-buffers
```

>**_Note_:**
   As the release has a whitelist of devices, it filters out RefSi. To override it, as a temporary solution we can point `SYCL_CONFIG_FILE_NAME` to empty space. This way it doesn't set the default `sycl.conf`.

The generated output should be somthing like the following:
```sh
Running on device: RefSi G1 RV64
Vector size: 10000
refsi_hal_device::mem_alloc(size=40000, align=128) -> 0x98006380
refsi_hal_device::mem_write(dst=0x98006380, size=40000)
refsi_hal_device::mem_alloc(size=40000, align=128) -> 0x97ffc700
refsi_hal_device::mem_write(dst=0x97ffc700, size=40000)
refsi_hal_device::mem_alloc(size=40000, align=128) -> 0x97ff2a80
refsi_hal_device::mem_write(dst=0x97ff2a80, size=40000)
refsi_hal_device::program_find_kernel(name='_ZTSZZ9VectorAddRN4sycl3_V15queueERKSt6vectorIiSaIiEES7_RS5_ENKUlRNS0_7handlerEE_clESA_EUlT_E_.mux-kernel-wrapper') -> 0x00010570
refsi_hal_device::kernel_exec(kernel=0x00010570, num_args=6, global=<10000:1:1>, local=<16:1:1>)
refsi_hal_device::pack_arg(offset=0, align=8, value=0x0000000097ff2a80)
refsi_hal_device::pack_arg(offset=8, align=8, value=0x0000000000000000)
refsi_hal_device::pack_arg(offset=16, align=8, value=0x0000000098006380)
refsi_hal_device::pack_arg(offset=24, align=8, value=0x0000000000000000)
refsi_hal_device::pack_arg(offset=32, align=8, value=0x0000000097ffc700)
refsi_hal_device::pack_arg(offset=40, align=8, value=0x0000000000000000)
refsi_hal_device::kernel_exec finished in 0.003 s
refsi_hal_device::mem_read(src=0x97ff2a80, size=40000)
refsi_hal_device::mem_free(address=0x97ff2a80)
refsi_hal_device::mem_free(address=0x97ffc700)
refsi_hal_device::mem_free(address=0x98006380)
[0]: 0 + 0 = 0
[1]: 1 + 1 = 2
[2]: 2 + 2 = 4
...
[9999]: 9999 + 9999 = 19998
Vector add successfully completed on device.
```

### Compiling oneAPI-samples vector-add using DPC++ pre-released compiler
To obtain the pre-released DPC++ compiler, please visit the following link: https://github.com/intel/llvm/releases. It is worth noting that these releases are regularly updated on daily basis.

For illustrative purposes, we suggest installing and conducting tests using the DPC++ daily version that was made available on October 3, 2023. You can find this specific release at the following URL: https://github.com/intel/llvm/releases/tag/nightly-2023-10-03.

Set the environment variables:
```sh
export OCL_ICD_FILENAMES=$ONEAPI_CON_KIT_INSTALL_DIR/lib/libCL.so
export LD_LIBRARY_PATH=/path/to/dpcpp_compiler/lib:$ONEAPI_CONKIT_INSTALL_DIR/lib:$LD_LIBRARY_PATH
export ONEAPI_DEVICE_SELECTOR="*:fpga"
```

Now to compile the vector add using the downloaded dpc++, follow the steps below:

```sh
cd oneAPI-samples/DirectProgramming/C++SYCL/DenseLinearAlgebra/vector-add

/path/to/dpcpp_compiler/bin/clang++ -fsycl src/vector-add-buffers.cpp -o vector-add-buffers
 CA_HAL_DEBUG=1 ./vector-add-buffers
```

The generated output should look something like the following:
```sh
Running on device: RefSi G1 RV64
Vector size: 10000
refsi_hal_device::mem_alloc(size=40000, align=128) -> 0x98006380
refsi_hal_device::mem_write(dst=0x98006380, size=40000)
refsi_hal_device::mem_alloc(size=40000, align=128) -> 0x97ffc700
refsi_hal_device::mem_write(dst=0x97ffc700, size=40000)
refsi_hal_device::mem_alloc(size=40000, align=128) -> 0x97ff2a80
refsi_hal_device::mem_write(dst=0x97ff2a80, size=40000)
refsi_hal_device::program_find_kernel(name='_ZTSZZ9VectorAddRN4sycl3_V15queueERKSt6vectorIiSaIiEES7_RS5_ENKUlRNS0_7handlerEE_clESA_EUlT_E_.mux-kernel-wrapper') -> 0x000104a2
refsi_hal_device::kernel_exec(kernel=0x000104a2, num_args=6, global=<10000:1:1>, local=<16:1:1>)
refsi_hal_device::pack_arg(offset=0, align=8, value=0x0000000097ff2a80)
refsi_hal_device::pack_arg(offset=8, align=8, value=0x0000000000000000)
refsi_hal_device::pack_arg(offset=16, align=8, value=0x0000000098006380)
refsi_hal_device::pack_arg(offset=24, align=8, value=0x0000000000000000)
refsi_hal_device::pack_arg(offset=32, align=8, value=0x0000000097ffc700)
refsi_hal_device::pack_arg(offset=40, align=8, value=0x0000000000000000)
refsi_hal_device::kernel_exec finished in 0.007 s
refsi_hal_device::mem_read(src=0x97ff2a80, size=40000)
refsi_hal_device::mem_free(address=0x97ff2a80)
refsi_hal_device::mem_free(address=0x97ffc700)
refsi_hal_device::mem_free(address=0x98006380)
[0]: 0 + 0 = 0
[1]: 1 + 1 = 2
[2]: 2 + 2 = 4
...
[9999]: 9999 + 9999 = 19998
Vector add successfully completed on device.
```

### Compiling oneAPI DPC++ compiler from Intel LLVM-base projects
To build oneAPI DPC++ follow the commands below:

```sh
git clone https://github.com/intel/llvm intel-llvm -b sycl
```

Naming the directory "intel-llvm" is optional, but it helps distinguish between intel's llvm and any other llvm repo you may have. You can build anywhere, but an "in-tree" build is recommended

```sh
cd intel-llvm
```

The `-o build` is the default, but this allows you to name the build directory. Since this creates the build directory relative to the current working directory, so being inside intel-llvm at this point creates an in-tree build.

The easiest way to get started is to use the buildbot configure and compile scripts.

```sh
python buildbot/configure.py \
    -o build \
    --cmake-opt="-DSYCL_BE=opencl" \
    --cmake-opt="-DSYCL_TARGET_DEVICES=acc" \
    --cmake-opt="-DSYCL_TEST_E2E_TARGETS=opencl:acc" \
    --cmake-opt="-DTEST_SUITE_COLLECT_CODE_SIZE=OFF"

# Build. Again, "-o build" is default but this makes it more
# explicit. Relative to current working directory.
python buildbot/compile.py -o build
```

> **_Note:_**
  The instructions differ for the host and riscv. The instructions mentioned above are for `riscv` target.


#### Compiling a simple SYCL example with oneAPI DPC++ compiler
The following simple-vector-add sample code serves as an introductory example, similar to a "Hello, World!" program, for data parallel programming using SYCL. It showcases fundamental features of SYCL and demonstrates how to perform vector addition on arrays of integers and float. Furthermore, building and running the simple-vector-add sample code can be used as a verification step to ensure that your development environment is properly configured for oneAPI Toolkit and oneAPI Construction kit.

```c++
#include <CL/sycl.hpp>

#include <array>
#include <iostream>


constexpr sycl::access::mode sycl_read = sycl::access::mode::read;
constexpr sycl::access::mode sycl_write = sycl::access::mode::write;

/* This is the class used to name the kernel for the runtime.
 * This must be done when the kernel is expressed as a lambda. */
template <typename T>
class SimpleVadd;

template <typename T, size_t N>
void simple_vadd(const std::array<T, N> &VA, const std::array<T, N> &VB,
                 std::array<T, N> &VC) {
  cl::sycl::queue deviceQueue;
  cl::sycl::range<1> numOfItems{N};
  cl::sycl::buffer<T, 1> bufferA(VA.data(), numOfItems);
  cl::sycl::buffer<T, 1> bufferB(VB.data(), numOfItems);
  cl::sycl::buffer<T, 1> bufferC(VC.data(), numOfItems);

  deviceQueue.submit([&](sycl::handler &cgh) {
    auto accessorA = bufferA.template get_access<sycl_read>(cgh);
    auto accessorB = bufferB.template get_access<sycl_read>(cgh);
    auto accessorC = bufferC.template get_access<sycl_write>(cgh);

    auto kern = [=](sycl::id<1> wiID) {
      accessorC[wiID] = accessorA[wiID] + accessorB[wiID];
    };
    cgh.parallel_for<class SimpleVadd<T>>(numOfItems, kern);
  });
}

int main() {
  const size_t array_size = 4;
  std::array<sycl::opencl::cl_int, array_size> A = {{1, 2, 3, 4}},
                                               B = {{1, 2, 3, 4}}, C;
  std::array<sycl::opencl::cl_float, array_size> D = {{1.f, 2.f, 3.f, 4.f}},
                                                 E = {{1.f, 2.f, 3.f, 4.f}}, F;
  simple_vadd(A, B, C);
  simple_vadd(D, E, F);
  for (unsigned int i = 0; i < array_size; i++) {
    if (C[i] != A[i] + B[i]) {
      std::cout << "The results are incorrect (element " << i << " is " << C[i]
                << "!\n";
      return 1;
    }
    if (F[i] != D[i] + E[i]) {
      std::cout << "The results are incorrect (element " << i << " is " << F[i]
                << "!\n";
      return 1;
    }
  }
  std::cout << "The results are correct!\n";
  return 0;
}
```

Set the environment variables before compiling the code:

```sh
export OCL_ICD_FILENAMES=$ONEAPI_CON_KIT_INSTALL_DIR/lib/libCL.so
export LD_LIBRARY_PATH=$ONEAPI_TOOLKIT_BUILD_DIR/lib:$LD_LIBRARY_PATH
export ONEAPI_DEVICE_SELECTOR="*:fpga"

```
Compile the code using the `clang++` from the oneAPI toolkit.

```sh
./build/bin/clang++ -fsycl simple-vector-add.cpp -o simple-vector-add
```

Once the code is compiled, execute the generated binary for the `riscv` target.

```
CA_HAL_DEBUG=1 ./simple-vector-add
```

The expected output should be similar to:
```sh
refsi_hal_device::mem_alloc(size=16, align=128) -> 0x9ff0ff80
refsi_hal_device::mem_write(dst=0x9ff0ff80, size=16)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0x9ff0ff00
refsi_hal_device::mem_write(dst=0x9ff0ff00, size=16)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0x9ff0fe80
refsi_hal_device::mem_write(dst=0x9ff0fe80, size=16)
refsi_hal_device::program_find_kernel(name='_ZTS10SimpleVaddIiE.mux-kernel-wrapper') -> 0x00010558
refsi_hal_device::kernel_exec(kernel=0x00010558, num_args=6, global=<4:1:1>, local=<4:1:1>)
refsi_hal_device::pack_arg(offset=0, align=8, value=0x000000009ff0fe80)
refsi_hal_device::pack_arg(offset=8, align=8, value=0x0000000000000000)
refsi_hal_device::pack_arg(offset=16, align=8, value=0x000000009ff0ff80)
refsi_hal_device::pack_arg(offset=24, align=8, value=0x0000000000000000)
refsi_hal_device::pack_arg(offset=32, align=8, value=0x000000009ff0ff00)
refsi_hal_device::pack_arg(offset=40, align=8, value=0x0000000000000000)
refsi_hal_device::kernel_exec finished in 0.002 s
refsi_hal_device::mem_read(src=0x9ff0fe80, size=16)
refsi_hal_device::mem_free(address=0x9ff0fe80)
refsi_hal_device::mem_free(address=0x9ff0ff00)
refsi_hal_device::mem_free(address=0x9ff0ff80)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0x9ff0ff80
refsi_hal_device::mem_write(dst=0x9ff0ff80, size=16)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0x9ff0ff00
refsi_hal_device::mem_write(dst=0x9ff0ff00, size=16)
refsi_hal_device::mem_alloc(size=16, align=128) -> 0x9ff0fe80
refsi_hal_device::mem_write(dst=0x9ff0fe80, size=16)
refsi_hal_device::program_find_kernel(name='_ZTS10SimpleVaddIfE.mux-kernel-wrapper') -> 0x000109e2
refsi_hal_device::kernel_exec(kernel=0x000109e2, num_args=6, global=<4:1:1>, local=<4:1:1>)
refsi_hal_device::pack_arg(offset=0, align=8, value=0x000000009ff0fe80)
refsi_hal_device::pack_arg(offset=8, align=8, value=0x0000000000000000)
refsi_hal_device::pack_arg(offset=16, align=8, value=0x000000009ff0ff80)
refsi_hal_device::pack_arg(offset=24, align=8, value=0x0000000000000000)
refsi_hal_device::pack_arg(offset=32, align=8, value=0x000000009ff0ff00)
refsi_hal_device::pack_arg(offset=40, align=8, value=0x0000000000000000)
refsi_hal_device::kernel_exec finished in 0.002 s
refsi_hal_device::mem_read(src=0x9ff0fe80, size=16)
refsi_hal_device::mem_free(address=0x9ff0fe80)
refsi_hal_device::mem_free(address=0x9ff0ff00)
refsi_hal_device::mem_free(address=0x9ff0ff80)
The results are correct!
```

# Support

Questions can be submitted on [GitHub Discussions Q&A] or on [GitHub Issues].

Before submitting a question, please make sure to read through the relevant
[documentation] and any existing discussions or issues.

If you find that your question has been previously asked but you are in need of
further clarification, feel free to write your question on the existing
discussion or issue.

If you would like to open a new question, we ask that you follow these steps:

* [Open a new question](https://github.com/uxlfoundation/oneapi-construction-kit/discussions/new?category=q-a)
* Alternatively, [open a new issue](https://github.com/codeplaysoftware/oneapi-construction-kit/issues/new/choose).
  * Select the **Question** issue template and fill in the requested details.
* Provide as much context as you can about the problem or question you have.
* Provide project version and any relevant system details (e.g. operating
  system).

Once your question has been opened, we will take a look and try to help you as
soon as possible.

[Documentation]: https://developer.codeplay.com/products/oneapi/construction-kit/guides
[GitHub Discussions Q&A]: https://github.com/uxlfoundation/oneapi-construction-kit/discussions/categories/q-a
[GitHub Issues]: https://github.com/uxlfoundation/oneapi-construction-kit/issues

# Governance
The oneAPI Construction Kit project is governed by the [UXL Foundation] and you can get involved in
this project in the following ways:
* Contribute to the oneAPI Construction Kit project. Read [CONTRIBUTING](./CONTRIBUTING.md) for more information.
* Join the [Open Source and Specification Working Group](https://github.com/uxlfoundation/foundation/tree/main?tab=readme-ov-file#working-groups) meetings.
* Join the mailing lists for the [UXL Foundation](https://lists.uxlfoundation.org/g/main/subgroups) to receive meetings schedule and latest updates.
-->
