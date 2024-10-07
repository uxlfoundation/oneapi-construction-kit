[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-1.4-4baaaa.svg)](CODE_OF_CONDUCT.md)
[![OpenSSF Scorecard](https://api.scorecard.dev/projects/github.com/codeplaysoftware/oneapi-construction-kit/badge)](https://scorecard.dev/viewer/?uri=github.com/codeplaysoftware/oneapi-construction-kit)

# oneAPI Construction Kit

The oneAPI Construction Kit is a framework to provide implementations of open standards, such as OpenCL and Vulkan, for a wide range of devices. The oneAPI Construction Kit can be used to build with the oneAPI Toolkit. The oneAPI Toolkit includes support for various open standards, such as OpenMP, SYCL, and DPC++. DPC++ is based on the SYCL programming model, which allows to write single-source C++ code that can target both CPUs and GPUs. To get more information on oneAPI, please visit https://www.intel.com/content/www/us/en/developer/tools/oneapi/overview.html.

>**_Note:_**
 It is not intended to be used as a standalone OpenCL or Vulkan implementation. It does not support the oneAPI Level Zero API.

For more information about building, implementing and maintaining the oneAPI Construction Kit please take the time to read the [developer guide](doc/developer-guide.md) and the other documentation in the
`doc` directory.

See [LICENSE.txt](LICENSE.txt) for details about the license for the code base,
and external components.

>**_Note:_**
   oneAPI Construction Kit was previously referred to as ComputeAorta and referred to as acronym `CA`. As a result, references to ComputeAorta or CA may be present in some oneAPI Construction Kit's documentation and code.

## Get started with the oneAPI Construction Kit
This section provides the minimum system requirements for building the oneAPI Construction Kit on Ubuntu 20.04. For Windows platform dependencies and build instructions please refer to the [developer guide](doc/developer-guide.md). There is a [blog post](https://codeplay.com/portal/blogs/2023/06/05/introducing-the-oneapi-construction-kit) demonstrating how to build the kit for a simulated RISC-V target. You can also find the documentation on [Codeplay's developer website](https://developer.codeplay.com/products/oneapi/construction-kit/home/).

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

To compile oneAPI Construction Kit for the host, please refer to the [developer guide](doc/developer-guide.md#compiling-oneapi-construction-kit).

#### Compiling oneAPI Construction Kit for RISC-V
This target aims to provide a flexible way of communicating with various customer RISC-V targets with different configurations. It supports multiple variants using an abstract class and can configure targets and execute commands. However, the current version has only been tested on an x86_64 host CPU.

The available targets in the current implementation are based on Codeplay's reference architecture, called RefSi, with two variations: `G` and `M1`. The `riscv` target is designed to support the `G` variant, while the `M1` variant has additional features like DMA. More information on `riscv` can be found [here](doc/modules/riscv.rst). To build in-tree, run the following:

```sh
cmake -GNinja \
   -Bbuild-riscv \
   -DCA_RISCV_ENABLED=ON \
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
