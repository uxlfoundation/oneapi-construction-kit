# Hal server

## Introduction

This supports a remote CPU based HAL which uses a socket connection to
communicate with a remote client. This can be cross-compiled as needed. It is a
small program which can be modified to use different ways of
transmission or different underlying HAL interfaces.

It should be paired with a built `oneapi-construction-kit` OpenCL interface using
the HAL socket client, which resides under `examples/hal_cpu_client`. Details of
how to build this target will be given below.

## Security

We recommend not running this program as root to reduce any likelihood of a
malicious actor, as this gives a protocol for running programs. The customer
should consider a specific user account just for running the server that has
limited access or using dockers to limit access.

We also recommend using ssh port forwarding. Note that we do not add any
additional encryption of the executables that are sent.

## Building the server

This can be done as follows:

```
  cmake -GNinja <path_to_ock>/examples/hal_cpu_remote_server -Bbuild_server
  ninja
```

If cross-compilation (for example RISC-V) is needed the following additional arguments are needed:

```
   -DCMAKE_TOOLCHAIN_FILE=<path_to_ock>/platform/riscv64-linux/riscv64-gcc-toolchain.cmake
```

Note this requires the RISC-V toolchain to be installed as follows on Ubuntu:

```
  sudo apt-get install --yes gcc-9-riscv64-linux-gnu g++-9-riscv64-linux-gnu
```

If you wish to run on a non RISC-V machine, then `qemu` and the RISC-V toolchain will need to be installed.

## Building the client

The client can built in-tree (for risc-v) as follows:

```
  cmake -Bbuild_client -GNinja \
    -DCA_MUX_TARGETS_TO_ENABLE="riscv" \
    -DCA_RISCV_ENABLED=ON \
    -DCA_ENABLE_API=cl \
    -DCA_LLVM_INSTALL_DIR=<path_to_llvm_install> \
    -DCA_CL_ENABLE_ICD_LOADER=ON  \
    -DCA_HAL_NAME=cpu_client
   ninja -Cbuild_client
```

It is also possible to build out of tree using `create_target.py` and
`scripts/new_target_templates/cpu_client_riscv.json`. This could also be modified to work
for different architectures. For example:

```
  python <path_to_ock>/scripts/create_target.py <path_to_ock> \
    <path_to_ock>/scripts/new_target_templates/cpu_client_riscv.json \
    --external-dir <new_target_dir>

  cd <new_target_dir>

  cmake -Bbuild_client -GNinja \
    -DCA_MUX_TARGETS_TO_ENABLE="cpu_client" \
    -DCA_CPU_CLIENT_ENABLED=ON \
    -DCA_ENABLE_API=cl \
    -DCA_LLVM_INSTALL_DIR=<path_to_llvm_install> \
    -DCA_CL_ENABLE_ICD_LOADER=ON  \
    -DCA_EXTERNAL_ONEAPI_CON_KIT_DIR=<path_to_ock> 
    -DCA_EXTERNAL_CPU_CLIENT_HAL_DIR=<path_to_ock>/examples/hals/hal_cpu_client
   ninja -Cbuild_client
```

The llvm install must have LLVM_ENABLE_PROJECT="lld;clang" and
LLVM_TARGETS_TO_BUILD shoul include `RISCV` for a `RISC-V` target.

This assumes building the ICD, but it is possible to build without this. Note
also the script requires cookiecutter to be installed with `pip install
cookiecutter`.

Note if we just wish a quick test then we could target `UnitCL` with the ninja
line. Also if the server is running on a different architecture the LLVM build
will need to enable that architecture.

# Running the server

The server requires a port which will be listened on. This can be any free user
port. It also will only accept connections from specified nodes. This can be an
ip address or host name e.g. "127.0.0.1" or "localhost". This defaults to
"127.0.0.1", assuming that ports will be forwarded if the client is not run on
the same machine.

The server can be run as follows:

```
   cd build_server
   ./hal_cpu_server_bin <port>
```
The node can also set with `-n`. It supports `-h` for info.

The server will accept one connection, which when completed will end the
program. This is to avoid programs putting the cpu into a bad state.

The server can easily be run in a loop though or repeatedly started through a service manager e.g.

```
 while qemu-riscv64 -L/usr/riscv64-linux-gnu ./hal_cpu_server_bin <port>; do :; done
```

The environment variable `HAL_DEBUG_SERVER` can be set to 1 to give more debug output.

We recommend this is never run as root as it runs executables sent over tcp/ip.

# Running the client

The client currently supports a socket connection only to the current node and
it expects the server to be running on the same machine. However, if the
customer desires to be able to run the server on a different machine (for
example where we have a RISC-V device running Ubuntu), then the customer may
consider using ssh port forwarding as suggested above. The recommended way of
using port forwarding would be as follows:

```
ssh -L <local_port>:127.0.0.1:<remote_port> user@destmachine
```

Running the client is done similar to any normal `OCK` OpenCL target (or via
SYCL OpenCL plugin). The environment variable `HAL_REMOTE_PORT` should be set to
the local port number. You will also need to set LD_LIBRARY_PATH to
the `lib` build directory if you do not have an OpenCL ICD on your machine.

As a simple test try running a simple UnitCL test (using the in-tree version):

```
  cd build_client
  HAL_REMOTE_PORT=<port_num> OCL_ICD_FILENAMES=$PWDlib/libCL.so.4.0 \
    ./bin/UnitCL --gtest_filter=Execution/Execution.Task_01_02_Add/OpenCLC
```
This should show as `PASSED`.

# Running the client with a SYCL example

The `OneAPI Construction Kit` has some simple `SYCL` examples. To compile a
simple vector add test you will need a `clang++` which has been built for
`SYCL`. This can be downloaded from https://github.com/intel/llvm/releases,
built from that repo or from the base toolkit (see
https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/README.md
for more details). First of all start the server as above.

Build and run simple-vector-add as follows from the build_client directory:-

```
export LD_LIBRARY_PATH=<path_to_dpcpp_compiler_base>/lib:$PWD/lib:$LD_LIBRARY_PATH
export OCL_ICD_FILENAMES=$PWD/lib/libCL.so
export ONEAPI_DEVICE_SELECTOR=*:fpga
<path_to_dpcpp_compiler_base>/bin/clang++ -fsycl <path_to_ock>/examples/applications/simple-vector-add.cpp -o simple-vector-add
HAL_REMOTE_PORT=<port_num> OCL_ICD_FILENAMES=$PWD/lib/libCL.so.4.0 ./simple-vector-add
```

This should show "The results are correct!".
