# OpenCL

```{toctree}
:maxdepth: 3

cl/cmake
cl/computemux_mapping
cl/extension
cl/external-extensions
cl/icd-loader
cl/intercept
cl/tools
cl/test
```

The ComputeAorta implementation of the [OpenCL spec][opencl-1.2] provides
definitions for its entry points which _must not_ be changed. In addition the
[OpenCL][opencl-1.2] also defines the OpenCL object types, such as
`_cl_platform_id`, which hold the state required to implement those objects.

Each header and source file is named after the OpenCL object it implements. For
example, the `_cl_context` object can be found in `include/cl/context.h` and
`source/context.cpp`. This pattern is used to group associated functionality in
the same place. All entry points that take a `cl_context` as the first parameter
or return a `cl_context`, such as `clCreateContext`, can be found in these
source files. This pattern is repeated for all of the OpenCL API objects.

> There are two special cases of note which relate to the `cl_mem` object. To
> Separate the implementation of buffer and image objects two new types
> inheriting From `_cl_mem` have been added; `_cl_mem_buffer` representing
> `cl_mem` objects Created with `clCreateBuffer`; and `_cl_mem_image` for
> `cl_mem`'s representing Images. These definitions and their associated API
> entry points can be found in The buffer and image header and source files.

All objects, in OpenCL these specified to be reference counted. The
implementation of reference counting is shared between all API objects and can
be found in the `include/cl/base.h` header file. Every API object inherits from
the `cl::base<T>` class template, which makes use of the Curiously Recurring
Template Pattern (CRTP) to avoid the introduction of a virtual function table.
This is an important point because the OpenCL ICD requires the first
`sizeof(void*)` bytes of each API object to contain the ICD dispatch table, not
a C++ virtual function table.

## Supported Features and Implementation Details

Documentation on the OpenCL API supported by ComputeAorta and implementation
details for effective use. The [API](../api-reference.html#cl-module)
includes all standard, non-optional OpenCL 1.2 APIs and language features.
ComputeAorta is also compatible with some [deprecated](#deprecated-entry-points)
OpenCL version 1.0 and 1.1 APIs, as well as those APIs defined in implemented
[extensions](cl/extension.md).

* [Data Types](#data-types)
* [Platform Info](#platform-info)
* [Device Info](#device-info)
* [Program Compilation](#program-compilation)
* [Image Support](#image-support)
* [Deprecated Entry Points](#deprecated-entry-points)

## Data Types

OpenCL headers provide datatypes which are guaranteed to be a consistent size.
This is important since the size of C/C++ datatypes is implementation defined,
which leads to non-portable code and discrepancies between host and program
data.

Note that the use of half precision floating point scalar and vector types is
optional, enabled using the [`cl_khr_fp16`][cl_khr_fp16] extension.
ComputeAorta's doesn't currently support this as half is not implemented in our
maths library. However the ComputeAorta CPU target does support
[`cl_khr_fp64`][cl_khr_fp64], which is the double precision floating point
extension. Use [`clGetDeviceInfo`][clGetDeviceInfo] for information about what
floating point extensions are supported for your target device.

### [Scalar Data Types][scalarDataTypes]

| API type    | Size                                  |
|-------------|---------------------------------------|
| `cl_char`   | 8-bit signed integer                  |
| `cl_uchar`  | 8-bit unsigned integer                |
| `cl_short`  | 16-bit signed integer                 |
| `cl_ushort` | 16-bit unsigned integer               |
| `cl_int`    | 32-bit signed integer                 |
| `cl_uint`   | 32-bit unsigned integer               |
| `cl_long`   | 64-bit signed integer                 |
| `cl_ulong`  | 64-bit unsigned integer               |
| `cl_half`   | 16-bit IEEE 754 floating point number |
| `cl_float`  | 32-bit IEEE 754 floating point number |
| `cl_double` | 64-bit IEEE 754 floating point number |

`cl_bool` is also available but unlike the other `cl_` types is not guaranteed
to be the same size as the `bool` in kernels.

### [Vector Data Types][vectorDataTypes]

| API type       | Size                                                 |
|----------------|------------------------------------------------------|
| `cl_char`_n_   | vector of _n_ 8-bit signed integers                  |
| `cl_uchar`_n_  | vector of _n_ 8-bit unsigned integers                |
| `cl_short`_n_  | vector of _n_ 16-bit signed integers                 |
| `cl_ushort`_n_ | vector of _n_ 16-bit unsigned integers               |
| `cl_int`_n_    | vector of _n_ 32-bit signed integers                 |
| `cl_uint`_n_   | vector of _n_ 32-bit unsigned integers               |
| `cl_long`_n_   | vector of _n_ 64-bit signed integers                 |
| `cl_ulong`_n_  | vector of _n_ 64-bit unsigned integers               |
| `cl_half`_n_   | vector of _n_ 16-bit IEEE 754 floating point numbers |
| `cl_float`_n_  | vector of _n_ 32-bit IEEE 754 floating point numbers |
| `cl_double`_n_ | vector of _n_ 64-bit IEEE 754 floating point numbers |

Built-in vector data types are supported by ComputeAorta even if the underlying
compute device does not support any or all of the vector data types. Vector
widths defined by the standard are 2, 3, 4, 8, and 16.

## Platform Info

The OpenCL platform layer implements platform-specific features that allow
applications to find OpenCL devices, device configuration information, and to
create OpenCL contexts using one or more devices.

Use [clGetPlatformInfo][clGetPlatformInfo] to query the platform for information
such as available extensions, and the name & version of the implementation.
ComputeAorta typically will have platform name `ComputeAorta` and vendor name
`Codeplay Software Ltd.` The version string will also contain both the version
of ComputeAorta and the LLVM version built against.

## Device Info

A device is a collection of compute units typically correspond to a GPU, a
multi-core CPU, and other processors such as DSPs.

To find the available devices on a platform use
[clGetDeviceIDs][clGetDeviceIDs]. ComputeAorta should contain a host CPU device
of type `CL_DEVICE_TYPE_CPU`, as well as optionally other accelerators. DSPs
fall under devices type `CL_DEVICE_TYPE_ACCELERATOR`.

All the information about a device can be queried with
[clGetDeviceInfo][clGetDeviceInfo]. Including device specific extensions and
information regarding memory size and work group limits. The `CL_DEVICE_NAME` of
ComputeAorta's CPU device target will be `ComputeAorta ARCH`, where `ARCH` is
replaced with the platform architecture, e.g., `x86_64`.

### Profiles

OpenCL devices can report supporting `"FULL_PROFILE"` or `"EMBEDDED_PROFILE"` by
passing `CL_DEVICE_PROFILE` as the `param_name` to
[clGetDeviceInfo][clGetDeviceInfo]. ComputeAorta detects which profile a device
supports using the following table, as specified in [OpenCL 1.2][#opencl-1.2].
If any of these limits fall below the `"FULL_PROFILE"` value then the device
will report support for `"EMBEDDED_PROFILE"`.

| OpenCL Device Property               |   FULL | EMBEDDED |
|--------------------------------------|-------:|---------:|
| `CL_DEVICE_MAX_MEM_ALLOC_SIZE`       | 128 MB |     1 KB |
| `CL_DEVICE_MAX_PARAMETER_SIZE`       |   1024 |      256 |
| `CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE` |  64 KB |     1 KB |
| `CL_DEVICE_MAX_CONSTANT_ARGS`        |      8 |        4 |
| `CL_DEVICE_LOCAL_MEM_SIZE`           |  32 KB |     1 KB |
| `CL_DEVICE_PRINTF_BUFFER_SIZE`       |   1 MB |     1 KB |
| `CL_DEVICE_MAX_READ_IMAGE_ARGS`      |    128 |        8 |
| `CL_DEVICE_MAX_WRITE_IMAGE_ARGS`     |      8 |        1 |
| `CL_DEVICE_MAX_SAMPLERS`             |     16 |        8 |
| `CL_DEVICE_IMAGE2D_MAX_WIDTH`        |   8192 |     2048 |
| `CL_DEVICE_IMAGE2D_MAX_HEIGHT`       |   8192 |     2048 |
| `CL_DEVICE_IMAGE3D_MAX_WIDTH`        |   2048 |        0 |
| `CL_DEVICE_IMAGE3D_MAX_HEIGHT`       |   2048 |        0 |
| `CL_DEVICE_IMAGE3D_MAX_DEPTH`        |   2048 |        0 |
| `CL_DEVICE_IMAGE_MAX_BUFFER_SIZE`    |  65536 |     2048 |
| `CL_DEVICE_IMAGE_MAX_ARRAY_SIZE`     |   2048 |      256 |

As `cl_device_id`'s are created from a `mux_device_t`, which expose a different
set of properties, an implementation of [Mux][../modules/mux/spec] can
control which OpenCL profile is reported using the following property mappings.

| OpenCL Device Property                       | Mux Device Property        |
|----------------------------------------------|----------------------------|
| `CL_DEVICE_MAX_MEM_ALLOC_SIZE`               | `allocation_size`          |
| `CL_DEVICE_LOCAL_MEM_SIZE`                   | `shared_local_memory_size` |
| `CL_DEVICE_MAX_READ_IMAGE_ARGS`              | `max_sampled_images`       |
| `CL_DEVICE_MAX_WRITE_IMAGE_ARGS`             | `max_storage_images`       |
| `CL_DEVICE_MAX_SAMPLERS`                     | `max_samplers`             |
| `CL_DEVICE_IMAGE_MAX_BUFFER_SIZE`            | `max_image_dimension_1d`   |
| `CL_DEVICE_IMAGE2D_MAX_{WIDTH,HEIGHT}`       | `max_image_dimension_2d`   |
| `CL_DEVICE_IMAGE3D_MAX_{WIDTH,HEIGHT,DEPTH}` | `max_image_dimension_3d`   |
| `CL_DEVICE_IMAGE_MAX_ARRAY_SIZE`             | `max_image_array_layers`   |

## Program Compilation

An OpenCL program consists of a set of kernels that are identified as functions
declared with the `__kernel` qualifier in the program source. Each program
object can represent program source or binary.

ComputeAorta tends to delay final compilation until kernel run time, e.g.
[clEnqueueNDRangeKernel][clEnqueueNDRange] , where all the scheduling
information will be available. Therefore if a user runs the same kernel kernel
twice with identical work loads and scheduling, the first run may suffer some
initial latency from compilation.

### [clCreateProgramWithSource][clCreateProgramWithSource]

```c
cl_program clCreateProgramWithSource(cl_context context,
                                     cl_uint count,
                                     const char **strings,
                                     const size_t *lengths,
                                     cl_int *errcode_ret);
```

Creates an OpenCL program object from OpenCL C source, ComputeAorta does no
compilation at this stage.

### [clCreateProgramWithBinary][clCreateProgramWithBinary]

```c
cl_program clCreateProgramWithBinary(cl_context context,
                                     cl_uint num_devices,
                                     const cl_device_id *device_list,
                                     const size_t *lengths,
                                     const unsigned char **binaries,
                                     cl_int *binary_status,
                                     cl_int *errcode_ret);
```

Create an OpenCL program object from a binary. Because ComputeAorta supports the
[`cl_khr_spir`][cl_khr_spir] extension, this can also be a SPIR binary. If the
binary is a SPIR binary, then the program **must** be compiled (with either
`clBuildProgram()`, or `clCompileProgram()` and `clLinkProgram()`). If the
binary is a pre-compiled binary, then it **may** be compiled again, but this
will have no effect.

### [clCreateProgramWithBuiltInKernels][clCreateProgramWithBuiltInKernels]

```c
cl_program clCreateProgramWithBuiltInKernels(cl_context context,
                                             cl_uint num_devices,
                                             const cl_device_id *device_list,
                                             const char *kernel_names,
                                             cl_int *errcode_ret);
```

Create an OpenCL program object with built-in kernels. This is useful if you
have a configurable, but not programmable, accelerator with predefined
computations it is optimized for. ComputeAorta currently doesn't support any
built-in kernels but this will change in the future.

### [clCompileProgram][clCompileProgram]

```c
cl_int clCompileProgram(cl_program program,
                        cl_uint num_devices,
                        const cl_device_id *device_list,
                        const char *options,
                        cl_uint num_input_headers,
                        const cl_program *input_headers,
                        const char **header_include_names,
                        void (CL_CALLBACK *pfn_notify) (
                          cl_program program,
                          void *user_data
                        ),
                        void *user_data);
```

Compile the program object. If the program was created from source ComputeAorta
runs the clang compiler frontend at this point.

### [clLinkProgram][clLinkProgram]

```c
cl_program clLinkProgram(cl_context context,
                         cl_uint num_devices,
                         const cl_device_id *device_list,
                         const char *options,
                         cl_uint num_input_programs,
                         const cl_program *input_programs,
                         void (CL_CALLBACK *pfn_notify) (
                           cl_program program,
                           void *user_data
                         ),
                         void *user_data,
                         cl_int *errcode_ret);
```

Link one of more program objects. In the spec wording it mentions that this may
create an executable, however in ComputeAorta we delay the creation of an
executable until [`clEnqueueNDRangeKernel`][clEnqueueNDRange] for performance
reasons. We do however link separate LLVM Modules together here.

### [clBuildProgram][clBuildProgram]

```c
cl_int clBuildProgram(cl_program program,
                      cl_uint num_devices,
                      const cl_device_id *device_list,
                      const char *options,
                      void (CL_CALLBACK *pfn_notify) (
                        cl_program program,
                        void *user_data
                      ),
                      void *user_data);
```

Build, or compile and link, the program object. This is equivalent to a call to
[`clCompileProgram`](#clcompileprogram) followed by [`clLinkProgram`](#cllinkprogram).
The `options` string parameter can be used to pass compiler flags, including
some only available as Codeplay vendor extensions.

### [clGetProgramInfo][clGetProgramInfo]

```c
cl_int clGetProgramInfo(cl_program program,
                        cl_program_info param_name,
                        size_t param_value_size,
                        void *param_value,
                        size_t *param_value_size_ret);
```

Query the program object for information. One use of this is to retrieve
compiled binaries which can we used in future to pass to
[`clCreateProgramFromBinary`](#clcreateprogramwithbinary) to save compilation
time. In this case ComputeAorta will return LLVM IR for the binary format.

### [clGetProgramBuildInfo][clGetProgramBuildInfo]

```c
cl_int clGetProgramBuildInfo(cl_program program,
                             cl_device_id device,
                             cl_program_build_info param_name,
                             size_t param_value_size,
                             void *param_value,
                             size_t *param_value_size_ret);
```

Query the program for the latest build information. If compilation failed this
function can be used to retrieve the error log, which ComputeAorta will output
this as a LLVM diagnostic.

## Image Support

An image object is used to store a 1, 2, or 3 dimensional texture,
frame-buffer or image. The elements of an image object are selected from a
list of predefined image formats. Samplers can then be used to read from
multi-dimensional images.

Not all OpenCL devices will support images, check the `CL_DEVICE_IMAGE_SUPPORT`
property from [`clGetDeviceInfo`][clGetDeviceInfo] to find out for a specific
device. `CL_INVALID_OPERATION` will be returned as an error code from some of
the image API functions if there are no devices in the context that support
images. Types such as `image1d_t` and `sampler_t` will also be unavailable.

The ComputeAorta CPU device target does support images, but these are emulated in
software rather than hardware accelerated, so not performant. Other
ComputeAorta platform devices may or may not support images.

## Debugging OpenCL kernels

Although a developer can easily create a debug build of an OpenCL application,
this will only allow debugging of the host side code. Successful debugging of
kernels themselves is more involved. The sections below document the steps
needed for smooth debugging of kernels.

### Build flags

To best debug an OpenCL kernel the build flags `-g`, `-S`, and `-cl-opt-disable`
should be set in the options to `clBuildProgram()`, where `-g` and `-S` are
part of our `cl_codeplay_extra_build_options` platform extension.

* `-g` enables emission of debug info. Without this flag the debugger user will
  not be able to set a breakpoint on the kernel name.
* `-S <path/to/source/file>` points the generated debug info to a source file on
  disk so that the debugger can display kernel source code. If this file does
  not exist already the OCL runtime will create a file from the string passed to
  `clCreateProgramWithSource()`, if this API call occurred.
* `-cl-opt-disable` disables performance optimizations. Omitting this flag will
  result in an inability to reliably inspect frame variables and source step.
  Analogous to debugging C/C++ code that hasn't been built with `-O0`.

### Viewing kernel source in the debugger

> Note: This section was created before the `-S` build option was added.
> Mapping the source in the debugger can be done as a more invasive
> fall-back but the suggested technique is to utilize `-S`.

It is problematic for a debugger to show source code for cl programs since they
are created from an API call to `clCreateProgramWithSource()`. Where the
source string passed in could be programmatically generated, not just read
directly from a .cl file on disk.

As a placeholder the OCL runtime sets the source filename to `kernel.opencl`,
and directory to wherever `libOpenCl.so` was dynamically loaded into.
This path then gets propagated into the debug info and picked up by the
debugger. So if a debugger user wants to set a breakpoint on a file & line,
then the file will have to be `kernel.opencl`. This could be an issue if
multiple kernels from different cl programs are being run.

More significant for the user experience is that the debugger can't show the
source code line for the current pc address of a stopped process. A workaround
to this problem in the case where the .cl file does exist on disk is to rename
the file to `kernel.opencl`. The directories may still not match however, so an
example fix would be to copy the source .cl file to `/tmp/kernel.opencl`, then
remap the directory in the debugger to point to `/tmp`. `lldb` uses a path
substitution, while in `gdb` you can provide a catch all directory to search for
files in. Note `lldb` doesn't substitute environmental variables here so instead
of using `$HOME` or `$USER` set the absolute path.

```
(lldb) settings set target.source-map /home/foo/Aorta/build /tmp
(gdb) directory /tmp
```

### LLDB sample session

Example debugging session of running the convolution UnitCL test and debugging
its kernel. Note UnitCL needs to be passed the `-g`, `-S`, and `-cl-opt-disable`
options via the `--unitcl_build_options` command line argument.

We start by setting a function breakpoint on the kernel name with
'b convolution', which won't be resolved immediately since `libOpenCL.so`
hasn't been dynamically loaded yet.

```
$ lldb -- build/bin/UnitCL \
          --gtest_filter=Execution.Task_07_04_Convolution \
          --unitcl_build_options="-g -cl-opt-disable -S OCL/test/UnitCL/kernels/task_07.04_convolution.cl"
(lldb) b convolution
Breakpoint 1: no locations (pending)
WARNING:  Unable to resolve breakpoint to any actual locations.
(lldb) r
```

`lldb` shows all the active threads which are currently stopped which makes the
output cluttered, but it's important to note that the thread IDs displayed have
no relation to OpenCL work item IDs. To see all the threads in `lldb` run
'thread list', while to select an individual thread 'thread select' can be used.
Depending on the OCL host implementation scheduling each process thread could
execute several work items, and so may hit a kernel breakpoint several times.

Next in our debug session we  step over the line `int x = get_global_id(0);`
with the 'thread step-over' command, also aliased to 'next'. Then by printing
the frame variables we can see that the global id of the current work item is
130.

```
(lldb) thread select 7
(lldb) thread step-over
(lldb) frame var
(float *) src = 0x0000000000d28e80
(float *) dst = 0x00000000007fd800
(int) x = 130
(int) width = 0
```

We can also set a breakpoint on a file line, where the file must be set as
`task_07.04_convolution.cl`. Additionally we can narrow this breakpoint scope
down to a single work-item with the condition 'x==4', since x holds the global
id. When the breakpoint is hit `lldb` can list multiple threads as stopped, and
the debugger user may have to cycle through the threads to find the specific one
where x==4.

```
(lldb) break set -f task_07.04_convolution.cl -l 20 -c "x == 4"
(lldb) break del 1
(lldb) continue
convolution(src=0x0000000000d28e80, dst=0x00000000007fd800)
   17  	            sum += weight * src[x + i];
   18  	        }
   19  	        sum /= totalWeight;
-> 20  	        dst[x] = sum;
   21  	    }
   22  	    else
   23  	    {

(lldb) print x
(int) $0 = 4
(lldb) print dst[x]
(float) $1 = 0
(lldb) next
(lldb) print dst[x]
(float) $2 = 95.9932632
```

### GDB sample session

Here we run through the same debugging scenario as for `lldb`, but using the
equivalent `gdb` command syntax.

```
$ gdb --args build/bin/UnitCL \
             --gtest_filter=Execution.Task_07_04_Convolution \
             --unitcl_build_options="-g -cl-opt-disable -S OCL/test/UnitCL/kernels/task_07.04_convolution.cl"
(gdb) b convolution
(gdb) run
```

Unlike `lldb`, `gdb` only displays a single thread when the process is stopped.
But you can see them all with command 'info threads', and select the individual
thread with `thread $tid`.

```
(gdb) thread 6
[Switching to thread 6 (Thread 0x7fffeb8f2700 (LWP 4090))]
#0  convolution (src=0xd28e80, dst=0x7fd800) at task_07.04_convolution.cl:7
7	    int x = get_global_id(0);
(gdb) next
8	    int width = get_global_size(0);
(gdb) info locals
x = 130
width = 0
(gdb) info args
src = 0xd28e80
dst = 0x7fd800
```

We can set a conditional breakpoint on a file line only for work-item 4 using
the below `gdb` command syntax.

```
(gdb) break task_07.04_convolution.cl:20 if x == 4
(gdb) delete 1
(gdb) continue
Continuing.
[Switching to Thread 0x7fffeb8f2700 (LWP 4106)]

Breakpoint 2, convolution (src=0xd28e80, dst=0x7fd800) at task_07.04_convolution.cl:20
20	        dst[x] = sum;
(gdb) info locals
sum = 0.833333313
totalWeight = 12
x = 4
width = 256
(gdb) print  dst[x]
$1 = 0
(gdb) next
7	    int x = get_global_id(0);
(gdb) print  dst[x]
$2 = 0.833333313
```

## Deprecated Entry Points

Deprecated OpenCL 1.0 and 1.1 functions which are implemented by ComputeAorta
for conformance and backwards compatibility.

### [clGetExtensionFunctionAddress][clGetExtensionFunctionAddress]

```c
void* clGetExtensionFunctionAddress(const char *func_name);
```

Query the platform for address of extension function, deprecated in OpenCL 1.2.
Replaced by
[`clGetExtensionFunctionAddressForPlatform`][clGetExtensionFunctionAddressForPlatform].

### [clEnqueueWaitForEvents][clEnqueueWaitForEvents]

```c
cl_int clEnqueueWaitForEvents(cl_command_queue queue,
                              cl_uint num_events,
                              const cl_event *event_list);
```

Enqueue an event wait list, deprecated in OpenCL 1.2. Replaced by
[`clWaitForEvents`][clWaitForEvents].

### [clEnqueueBarrier][clEnqueueBarrier]

```c
cl_int clEnqueueBarrier(cl_command_queue queue);
```

Enqueue a barrier on the command queue, deprecated in OpenCL 1.2. Replaced by
[`clEnqueueBarrierWithWaitList`][clEnqueueBarrierWithWaitList]

### [clEnqueueMarker][clEnqueueMarker]

```c
cl_int clEnqueueMarker(cl_command_queue queue,
                       cl_event *event);
```

Enqueue a marker on the command queue, deprecated in OpenCL 1.2. Replaced by
[`clEnqueueMarkerWithWaitList`][clEnqueueMarkerWithWaitList]

### [clUnloadCompiler][clUnloadCompiler]

```c
cl_int clUnloadCompiler();
```

Unload the compiler, deprecated in OpenCL 1.2. This is a hint from the
application and does not guarantee that the compiler will actually be unloaded
by the implementation. As this is just a hint ComputeAorta ignores it.

Replaced by [`clUnloadPlatformCompiler`][clUnloadPlatformCompiler].

### [clCreateImage2D][clCreateImage2D]

```c
cl_mem clCreateImage2D(cl_context context,
                       cl_mem_flags flags,
                       const cl_image_format *image_format,
                       size_t image_width,
                       size_t image_height,
                       size_t image_row_pitch,
                       void *host_ptr,
                       cl_int *errcode_ret);
```

Create a 2D image memory object, deprecated in OpenCL 1.2. Replaced by
[`clCreateImage`][clCreateImage].

### [clCreateImage3D][clCreateImage3D]

```c
cl_mem clCreateImage3D(cl_context context,
                       cl_mem_flags flags,
                       const cl_image_format *image_format,
                       size_t image_width,
                       size_t image_height,
                       size_t image_depth,
                       size_t image_row_pitch,
                       size_t image_slice_pitch,
                       void *host_ptr,
                       cl_int *errcode_ret);
```

Create a 3D image memory object, deprecated in OpenCL 1.2. Replaced by
[`clCreateImage`][clCreateImage].

[cl_khr_fp16]: https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/cl_khr_fp16.html
[cl_khr_fp64]: https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/cl_khr_fp64.html
[scalarDataTypes]: https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/scalarDataTypes.html
[vectorDataTypes]: https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/vectorDataTypes.html
[clGetPlatformInfo]: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetPlatformInfo.html
[clGetDeviceIDs]: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetDeviceIDs.html
[clGetDeviceInfo]: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetDeviceInfo.html
[opencl-1.2]: https://www.khronos.org/registry/OpenCL/specs/opencl-1.2.pdf
[clEnqueueNDRangeKernel]: https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clEnqueueNDRangeKernel.html
[clCreateProgramWithSource]: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCreateProgramWithSource.html
[clCreateProgramWithBinary]: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCreateProgramWithBinary.html
[cl_khr_spir]: https://www.khronos.org/registry/OpenCL/sdk/2.0/docs/man/xhtml/cl_khr_spir.html
[clCreateProgramWithBuiltInKernels]: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCreateProgramWithBuiltInKernels.html
[clCompileProgram]: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCompileProgram.html
[clLinkProgram]: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clLinkProgram.html
[clBuildProgram]: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clBuildProgram.html
[clGetProgramInfo]: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetProgramInfo.html
[clGetProgramBuildInfo]: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetProgramBuildInfo.html
[clGetExtensionFunctionAddress]: http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clGetExtensionFunctionAddress.html
[clGetExtensionFunctionAddressForPlatform]: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetExtensionFunctionAddressForPlatform.html
[clEnqueueWaitForEvents]: http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clEnqueueWaitForEvents.html
[clWaitForEvents]: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clWaitForEvents.html
[clEnqueueBarrier]: http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clEnqueueBarrier.html
[clEnqueueBarrierWithWaitList]: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueBarrierWithWaitList.html
[clEnqueueMarker]: http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clEnqueueMarker.html
[clEnqueueMarkerWithWaitList]: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueMarkerWithWaitList.html
[clUnloadCompiler]: http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clUnloadCompiler.html
[clUnloadPlatformCompiler]: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clUnloadPlatformCompiler.html
[clCreateImage2D]: http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clCreateImage2D.html
[clCreateImage]: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clCreateImage.html
[clCreateImage3D]: http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clCreateImage3D.html
