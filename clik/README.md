# clik

![clik logo](doc/clik_logo.svg)

clik is a simple compute library that demonstrates how offloading code to an accelerator works. Multiple devices can be supported through 'HALs', which are libraries handling tasks such as memory allocation, data transfers and kernel execution on the device. A reference HAL that targets the system's CPU is included with clik. New HALs targeting different devices can easily be created and used through the same framework.

clik can be used as an introduction to how offloading to an accelerator works, but also as scaffolding when adding support for a new device in ComputeAorta. It has been designed to be as simple as possible, so that it is easy to understand and troubleshoot. Several basic examples are included, covering essential functions such as creating buffers, data transfers, executing kernels, work-group barriers and local memory. When these examples all run successfully, clik's role as scaffolding is complete. However, it is also possible to add new examples as needed.

clik kernels are written in C and can be compiled using a stand-alone compiler such as GCC. This means that compared to more advanced compute libraries a compiler back-end does not need to be integrated into the compute library. As a result, bringing up a new target can be faster due to being able to develop the runtime part of the device code before needing to integrate a compiler.

The concept of HAL, a library that abstracts the interface to a device, is central to clik. This framework has been designed with the idea that multiple devices can be targeted using the same simple API and examples. In addition, a HAL that has been developed using clik can then be integrated in ComputeAorta. The source code for the HAL does not need to be rewritten for ComputeAorta, instead it can seamlessly be loaded by a generic ComputeMux target.

## Requirements

Common 'build' packages are required to be installed on the system. On Ubuntu, they can be installed with the following commands:

    $ sudo apt-get install python3 build-essential cmake ninja-build

clik needs the base HAL to build. This is not necessarily contained within a
clik. This is defaulted to look in clik/external, then the directory above
and then look using the cmake variable CLIK_HAL_DIR. By default as part of the
DDK it will be in the directory above clik.

## Build instructions

Once this is done, the clik runtime, reference HAL and examples can be built:

    $ cd path/to/clik
    $ mkdir -p build/clik-debug
    $ cd build/clik-debug
    $ cmake -G Ninja \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_INSTALL_PREFIX=install \
        ../..
    $ ninja

ComputeMux examples can optionally be built by passing additional options to CMake:

    -DCLIK_BUILD_MUX_EXAMPLES=TRUE \
    -DCLIK_MUX_INCLUDE_DIR=path/to/ComputeAorta/modules/core/include \
    -DCLIK_MUX_LIBRARY_PATH=path/to/ComputeAorta/build/debug/lib/libcore.so

## Running tests

Examples included with clik can be used as a test suite for basic validation:

    $ cd path/to/clik/build/clik-debug
    $ ninja check
    [100 %] [0:0:11/11] PASS matrix_multiply_tiled

    Passed:           11 (100.0 %)
    Failed:            0 (  0.0 %)
    Timeouts:          0 (  0.0 %)

## Running the examples

After this is done, examples can simply be executed from the build directory:

    $ cd path/to/clik/build/clik-debug
    $ bin/hello
    Using device 'CPU'
    Running hello example (Global size: 8, local size: 1)
    Hello from clik_sync! tid=0, lid=0, gid=0
    Hello from clik_sync! tid=1, lid=0, gid=1
    Hello from clik_sync! tid=2, lid=0, gid=2
    Hello from clik_sync! tid=3, lid=0, gid=3
    Hello from clik_sync! tid=4, lid=0, gid=4
    Hello from clik_sync! tid=5, lid=0, gid=5
    Hello from clik_sync! tid=6, lid=0, gid=6
    Hello from clik_sync! tid=7, lid=0, gid=7
    $ bin/vector_add
    Using device 'CPU'
    Running vector_add example (Global size: 1024, local size: 16)
    Results validated successfully.

## Adding a new example

1. Create a new `my_example` subdirectory in `examples/clik_sync`
2. Add the following to the `CMakeLists.txt` file in `examples/clik_sync`:

```
add_subdirectory(my_example)
```

3. Create a `CMakeLists.txt` file in the `my_example` subdirectory:

```
add_example(my_example
  my_example.cpp
)

target_link_libraries(my_example PRIVATE clik_runtime_sync)

add_baked_kernel(my_example my_example_kernel kernel_binary.h device_my_example.c)
```

4. Write the host-side source code (`my_example.cpp`) using the clik_sync API, for example by using `examples/clik_sync/vector_add/vector_add.cpp` as a starting point.
5. Write the device-side (kernel) source code (`device_my_example.c`). In order to run the example, each HAL will also need to provide a kernel entry point file (`device_my_example_entry.c`) under their example directory, which should mirror the structure of `clik/examples`. For example the entry file will be located at `${HAL_FOO_EXAMPLE_DIR}/clik_sync/device_my_example_entry.c`) where `${HAL_FOO_EXAMPLE_DIR}` is the example directory exported by the Foo HAL in CMake.
6. Build by running `ninja`
7. Run the new example:

```
$ bin/my_example
Hello from my_example!
```

## Using a different HAL

HALs other than the reference HAL can be built alongside clik using `ninja`. To do so, they can either be copied to the `external` directory (e.g. `external/hal_refsi_tutorial`) or the `cmake` variable `CLIK_EXTERNAL_HAL_DIR` should be set to an external HAL directory.

 A CMake script at the top level of the target `HAL`(`hal_refsi_tutorial/CMakeLists.txt`) will automatically be included. This script should define a function called `hal_refsi_tutorial_compile_kernel` that compiles a kernel source file into an executable that can be loaded by the HAL. Here we wish to build for a target HAL at `$HOME/hals/hal_refsi_tutorial`:

    $ cmake -G Ninja \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_INSTALL_PREFIX=install \
        -DCLIK_HAL_NAME=refsi_tutorial \
        -DCLIK_EXTERNAL_HAL_DIR=$HOME/hals/hal_refsi_tutorial \
        ../..
    -- Found HAL: cpu
    -- Found HAL: refsi_tutorial
    -- Configuring done
    -- Generating done
    -- Build files have been written to: path/to/clik/build/clik-debug

In order to switch HALs, an option such as `-DCLIK_HAL_NAME=refsi_tutorial` can be passed to CMake. When `ninja` is executed again, all the example kernels will be recompiled for that particular HAL. Running an example shows that a different HAL is used to execute kernels:

    $ bin/hello
    Using device 'RefSi M1 Tutorial'
    Running hello example (Global size: 8, local size: 1)
    Hello from clik_sync! tid=0, lid=0, gid=0
    Hello from clik_sync! tid=1, lid=0, gid=1
    Hello from clik_sync! tid=2, lid=0, gid=2
    Hello from clik_sync! tid=3, lid=0, gid=3
    Hello from clik_sync! tid=4, lid=0, gid=4
    Hello from clik_sync! tid=5, lid=0, gid=5
    Hello from clik_sync! tid=6, lid=0, gid=6
    Hello from clik_sync! tid=7, lid=0, gid=7

The name of the HAL to load can also be specified at run time using the `CA_HAL_DEVICE` environment variable. However, bear in mind that example kernels will not be recompiled for that HAL device, resulting in errors when running examples that execute kernels:

    $ CA_HAL_DEVICE=cpu bin/hello
    Using device 'CPU'
    Unable to create a program from the kernel binary.

## Debugging the CPU HAL

The CPU HAL has two purposes, being a reference implementation (e.g. to give an idea how a HAL might be implemented) as well as being a teaching aid (e.g. to demonstrate how offloading can work). Kernels are compiled for the host CPU and can therefore be debugged in the same way as host code.

In order to debug kernels, a debug build must be used by passing `-DCMAKE_BUILD_TYPE=Debug` to CMake. Setting the global size to one by passing `-S1` to most examples will also make stepping through easier:

    $ gdb bin/hello
    (gdb) b hello
    Function "hello" not defined.
    Make breakpoint pending on future shared library load? (y or [n]) y
    Breakpoint 1 (hello) pending.

    (gdb) run -S1
    Starting program: path/to/clik/build/clik-debug/bin/hello -S1

    Using device 'CPU'
    Running hello example (Global size: 1, local size: 1)

    Breakpoint 1, hello (item=0x555555577e90) at path/to/clik/examples/clik_sync/hello/device_hello.c:5
    5       __kernel void hello(exec_state_t *item) {
    (gdb) n
    6         item->printf("Hello from clik_sync! tid=%d, lid=%d, gid=%d\n",

    (gdb) n
    Hello from clik_sync! tid=0, lid=0, gid=0
    9       }

    (gdb)
