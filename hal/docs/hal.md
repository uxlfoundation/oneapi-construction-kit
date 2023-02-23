# Hardware Abstraction Layer

A HAL sits below a MUX target, abstracting away the interface to one or more
compute devices. It is used to manage memory and load and execute binaries. By
abstracting at this level, there are a number of benefits:

- Multiple different HALs can be used by one MUX target without increased MUX
  complexity.
- New devices can be added quickly as all MUX development can be skipped.
- Tested and proven code can be shared between HALs for faster bring-up.

Currently the HAL is used exclusively as part of the RISC-V MUX target and in
the CLIK tool. The HAL specification makes no demands on the the executable used
except for its use in RISC-V target. Since the RISC-V target will be used as a
template for future targets this is also detailed
[here](#hal-kernel-abi-used-in-the-current-risc-v-mux).

The HAL specification defines a contract between a MUX target or CLIK and a HAL
implementation.


-----
## Interface

The HAL is implemented as a collection of interface classes, that will have a
concrete implementation for each target. It serves to hide target specific
interfacing details from ComputeAorta.

The goals of the HAL interface are as follows:
- Hide all target interfacing details from the main RISC-V ComputeAorta target
- Reduce the complexity involved in adding and supporting a new RISC-V
  ComputeAorta target
- Be adaptable to a wide range of different targets, such as Simulators and
  Hardware
- Have a base level which is very simple and over time add optional components.


----
## The hal_t class

At the top level there is a `hal_t` class which will give general information
about the target and allows access to a per device `hal_device_info_t` class
which gives information about the specific device. `hal_device_t` objects can
also be created and freed through this class. `hal_device_t` objects allow
the caller to perform actions on the device such as executing a kernel.

The `hal_t` interface is a pure virtual class, which must be derived by an
implementation. A creator function will be provided to instantiate and return a
specific HAL while also hiding the implementation details. An example HAL
instantiation can be seen in the example `hal_null` target.

Currently a HAL library exposes the `get_hal()` exported function, which will
return an instance of `hal_t`. Repeated calls will return the same instance.

The information methods are described in [Target Info](#target-info), the
remaining methods are for creation and deletion of `hal_device_t` objects:

* `hal_device_t *device_create(uint32_t device_index)`
  - This can be used to create a `hal_device_t`, which will be used for most
    of the actions from Mux.

* `hal_device_t *device_delete(uint32_t device_index)`
  - This can be used to delete a `hal_device_t`.

Note that `device_create` takes an index parameter which must be under the
`num_devices` value in the `hal_info_t` object. `device_create` is just a signal
to `hal_t` object that it must be created at this point; it is up to the
implementor of `hal_t` to make the decision on whether it actually needs
creating or not or whether this just passes a pointer back. Similarly,
`device_delete` just means that it is no longer needed.


-----
## Basic Types

All basic types used by the HAL are stored in the `hal_types.h` file.

The basic types used by the HAL are listed below and are intended to support
both 32 bit and 64 bit targets. For consistency all basic types are 64 bits in
length to support 32 bit and 64 bit targets equally. For 32 bit targets the
expectation is that the high 32 bits of `hal_addr_t` and `hal_size_t` will
always be zero.

```cpp
// intended to store a target side memory address
typedef uint64_t hal_addr_t;
// intended to store a target side size
typedef uint64_t hal_size_t;
// intended to store a target value
typedef uint64_t hal_word_t;
// a unique handle identifying a loaded program
typedef uint64_t hal_program_t;
// a unique handle identifying a kernel
typedef uint64_t hal_kernel_t;

enum {
  hal_nullptr = 0,
  hal_invalid_program = 0,
  hal_invalid_kernel = 0,
};
```


----
## The hal_device_t class

`hal_device_t` is another abstract class which must be filled in. It is needed
for any actions on the device, as follows:

* [Device Memory Access](#device-memory-access)
  - memory functions such as reading, writing and allocation of memory

* [Kernel Operations](#kernel-operations)
  - kernel related functions such as loading programs and executing kernels across
  a range of workgroups


-----
## Device Memory Access

The HAL must provide access to target memory for the purposes of support buffer
allocation and access operations. These operations are intended to provide
access to `global` memory in the OpenCL sense. The following functions are
exposed by the HAL:

```cpp
struct hal_device_t {
  ...

  // allocate a memory range on the target
  // will return `hal_nullptr` if the allocation fails
  virtual hal_addr_t mem_alloc(hal_size_t size,
                               hal_size_t alignment) = 0;

  // copy memory between target buffers
  virtual bool mem_copy(hal_addr_t dst,
                        hal_addr_t src,
                        hal_size_t size) = 0;

  // free a memory range on the target
  virtual bool mem_free(hal_addr_t addr) = 0;

  // fill memory with a pattern
  virtual bool mem_fill(hal_addr_t dst,
                        const void *pattern,
                        hal_size_t pattern_size,
                        hal_size_t size) = 0;

  // read memory from the target to the host
  virtual bool mem_read(void *dst,
                        hal_addr_t src,
                        hal_size_t size) = 0;

  // write host memory to the target
  virtual bool mem_write(hal_addr_t dst,
                         const void *src,
                         hal_size_t size) = 0;
```

:::{note}
All `size` parameters are in bytes.
:::

In terms of consistency, any write, fill or copy operation should become
immediately visible to any following read operation or kernel execution
initiated by the HAL.

As multiple HALs need to map the above memory access routines to a dedicated
physical region of memory, the HAL provides a library for this purpose; see
`allocator.h`.


-----
## Kernel Operations

The HAL must support loading of arbitrary program binary files produced by the
LLVM backend (typically ELF files). The HAL assumes nothing about whether the
final binary is linked, but `hal_device_info_t` provides a per device linker
script which would typically be used during the compilation phase such as in
the RISC-V target.

Execution of kernels uses the following struct:

```cpp
struct hal_ndrange_t {
  hal_size_t offset[3];
  hal_size_t global[3];
  hal_size_t local[3];
};
```

The `hal_ndrange_t` structure closely matches the `ndrange` requirements of OpenCL
and the meanings are the same. `offset` is the starting offset of the ndrange
within the work space. `global` defines the global size to be executed, and
`local` defines the local workgroup size.

The relevant functions for loading kernels and executing them are defined as
follows:

```cpp
struct hal_device_t {
  ...

  // load an ELF file
  // returns `hal_invalid_program` if the program could not be loaded
  virtual hal_program_t program_load(const void *data, hal_size_t size) = 0;

  // find a specific kernel function in a compiled program
  // returns `hal_invalid_kernel` if no symbol could be found
  virtual hal_kernel_t program_find_kernel(hal_program_t program,
                                           const char *name) = 0;

  // execute a kernel on the target
  virtual bool kernel_exec(hal_program_t program,
                           hal_kernel_t kernel,
                           const hal_ndrange_t *nd_range,
                           const hal_arg_t *args,
                           uint32_t num_args,
                           uint32_t work_dim /*1,2 or 3*/) = 0;

  // unload a program from the target
  virtual bool program_free(hal_program_t program) = 0;

  ...
};
```

To provision for multiple kernels being loaded at the same time, kernels are
identified following a handle scheme. When a kernel is loaded it returns a
handle to the loaded kernel, which can be used to specify the kernel in question
during following kernel calls.

It is important to clarify that loading a program in this context means giving
it to the HAL, but does not mandate that it should be uploaded to the target at
that time. It is left to the discretion of a HAL implementation when it
actually uploads a program to the device.

A binary provided to `program_load` may not be relocatable and it is valid for
two different programs to be located in overlapping memory ranges. It is the
responsibility of the HAL to manage the upload process to avoid conflict.

A simple scheme for this is to defer upload of a binary to the target until it
is required during a `kernel_exec` call. This way a target need only load on
demand and conflicts can't arise. A HAL is however free to manage uploads any
way it sees fit.

For example to load and execute a kernel using the HAL the following steps
might be performed:

```cpp
const hal_program_t program = hal->program_load(elf_data, elf_size);
if (program == hal_invalid_program) {
  // error
}
const hal_kernel_t kernel = hal->program_find_kernel(program, "my_kernel");
if (kernel == hal_invalid_kernel) {
  // error
}
if (!hal->kernel_exec(program, kernel, ...)) {
  // error
}
hal->program_free(program);
```

The kernel execution function `kernel_exec` must be provided with the entry
point of the kernel, the `ndrange` it will operate over and a list of kernel
argument descriptors.

:::{note}
It is valid for the kernel_exec function to block until execution has completed
or an error has been encountered.
:::

For further clarity, the following code is also a valid use of the HAL API when
using multiple programs:

```cpp
// be aware, error handling has been omitted for brevity.

const hal_program_t program1 = hal->program_load(elf_data1, elf_size1);
const hal_program_t program2 = hal->program_load(elf_data2, elf_size2);

const hal_kernel_t kernel1 = hal->program_find_kernel(program1, "kernel_1");
const hal_kernel_t kernel2 = hal->program_find_kernel(program2, "kernel_2");

hal->kernel_exec(program1, kernel1, ...);
hal->kernel_exec(program2, kernel2, ...);

hal->program_free(program1);
hal->program_free(program2);
```

A HAL writer can also expect that when `program_load` is called it becomes the
responsibility of the callee to call `program_free` to release any resources
used. All currently loaded programs will be freed by the callee prior to any
call being made to `hal_device_t::device_delete`. The intension of this rule is
to simplify HAL implementations so they dont have to track allocations.


-----
### Synchronization

The HAL currently mandates that kernels have finished executing by the time
the `kernel_exec` function returns.

In terms of consistency, any externally visible state must match the ordering of
HAL API calls as if they were executed in order and fully completed before the
next API call was processed.


### Argument Passing

When invoking a kernel with `kernel_exec` the kernel arguments are passed as a
list of argument descriptors to the HAL. These descriptors are intentionally
neutral regarding the details of how the data will be passed to the kernel. This
breaks the direct coupling between the ABI and the HAL API, leading to less
changes when we change the kernel ABI.

```cpp
enum hal_arg_kind_t {
  hal_arg_address,
  hal_arg_value,
  // ...
};

enum hal_addr_space_t {
  hal_space_local,
  hal_space_global,
};

struct hal_arg_t {
  hal_arg_kind_t kind;
  hal_addr_space_t space;
  hal_size_t size;
  union {
    hal_addr_t address;
    void *pod_data;
  };
};
```

Consider the following example kernel definition:

```cpp
__kernel void my_kernel(__global int *in, __global int *out);
```

This kernel would have two `hal_arg_t` entries, specified left to right, both of
which would be the global form shown below.

Global buffers will have the following form:

```cpp
hal_arg_t arg = {
  .kind    = hal_arg_address,
  .space   = hal_space_global,
  .size    = <global_buffer_size>,
  .address = <target_address>
};
```

Local buffers will have the following form:

```cpp
hal_arg_t arg = {
  .kind  = hal_arg_address,
  .space = hal_space_local,
  .size  = <local_buffer_size>,
};
```

POD data will have the following form:

```cpp
hal_arg_t arg = {
  .kind     = hal_arg_value,
  .size     = <POD data size>,
  .pod_data = <POD data on host>,
};
```
Some targets, such as the default RISC-V target will place local data on the
stack and the kernel ABI is expected to give it information to the function in
order to size it.


-----
## Target Info

ComputeAorta requires information about the target to guide the compilation and
execution process. For this purpose the HAL exposes a mechanism to query the
target for information.

A HAL implementation can be asked to fill out the `hal_info_t` and
`hal_device_info_t` structures with relevant information about the target in a
generic form using the base instance of the HAL.

For the definition of these types see `hal_types.h`.

```cpp
  // return generic platform information, including number of devices and 
  // platform name
  virtual const hal_info_t& get_info() = 0;

  // return generic target information
  // This can be upcast depending on the type information in the class
  // This include information such as word size and memory sizes
  virtual const hal_device_info_t *device_get_info(uint32_t device_index) = 0;
```

ISA specific `hal_device_info_t` subclasses are also specified which allow a HAL
to provide more target specific information. By querying the
`hal_device_info_t::type` member the `hal_device_info_t` instance can be
downcast correctly to the correct type, giving access to ISA specific
information.

The rational behind this is to avoid multiple ISAs turning the `hal_device_info_t`
struct into a "god" struct, while still allowing common information to be
shared. If a ComputeAorta target needs to execute a target specific code-path
the HAL can be downcast accordingly to access any target specific functionality.

Currently only the RISC-V target has a `hal_device_info_t` subclass which can be
found in the `hal_riscv.h` file.

-----
### Versioning

The base `hal_t` class has a constant member `api_version`.

```cpp
static constexpr uint32_t api_version = 3;
```

This should be updated when any interface is updated and should be matched by
any user of the HAL such as `ComputeMux`. A runtime failure will happen if they
do not match.


-----
### Utility classes

It is possible to share elements of code between multiple HAL implementations
which is desirable where possible. Currently the HAL API provides the following
reusable elements:

- A low-level memory allocator for managing bare metal memory regions.
- An ELF file parser for RISC-V binaries.
- RISC-V target string parsing.


----
## HAL Kernel ABI used in the current RISC-V MUX

The current RISC-V target specifies a specific ABI for calling the kernel. As this target will be used as a template for others, it is likely a similar ABI will be used there, so is defined here:

```cpp
void kernel(void *args, const hal_sched_info_??_t *sched);
```

Kernels compiled by the RISC-V MUX target will adhere to the following:

- `sched` will be `hal_sched_info_32_t` or `hal_sched_info_64_t` depending on
  the target word size.
- `args` is a pointer to a raw buffer of packed and aligned argument data. The
  kernel itself handles the unpacking and extraction of this data.

The RISC-V MUX target will compile a kernel according to this ABI and the HAL
will execute it accordingly forming a contract between the two code areas.


----
## Schedule Structure

The second kernel parameter (`hal_sched_info_??_t *sched`) will be one of the
structures below, selected to match the specific word size of the processor
(XLEN in RISC-V nomenclature).

```cpp
// for RV32
struct hal_sched_info_32_t {
  uint32_t group_id_start[3];       // Start group ID for each dimension
  uint32_t num_groups_total[3];     // Total number of groups in each dimension
                                    // (needed for get_global_size())
  uint32_t global_offset[3];        // Global offset for each dimension
                                    // (needed for get_global_offset())
  uint32_t local_size[3];           // Number of work-items in each dimension
  uint32_t num_dim;                 // ND-range rank (1, 2 or 3)
  uint32_t num_groups_per_call[3];  // Number of groups from top level call to
                                    // kernel if supported
  uint32_t hal_extra;               // Device pointer to additional information
};

// for RV64
struct hal_sched_info_64_t {
  uint64_t group_id_start[3];       // Start group ID for each dimension
  uint64_t num_groups_total[3];     // Total number of groups in each dimension
                                    // (needed for get_global_size())
  uint64_t global_offset[3];        // Global offset for each dimension
                                    // (needed for get_global_offset())
  uint32_t local_size[3];           // Number of work-items in each dimension
  uint32_t num_dim;                 // ND-range rank (1, 2 or 3)
  uint64_t num_groups_per_call[3];  // Number of groups from top level call to
                                    // kernel if supported
  uint64_t hal_extra;               // Device pointer to additional information
};
```

`num_groups_per_call` is used to allow more than one group per call. Individual
implementations could ignore or interpret this differently.

`hal_extra` provides a way that a HAL can pass additional information to a
kernel and is optional. Examples may include local ids or printf. Set to
`nullptr` if not used.

:::{note}
Currently these two fields are not used by kernels compiled from the `riscv`
target.
:::

----
## Packed Arguments

The arguments for kernels are "packed" into a flat memory buffer, the start
address of which is passed to the kernel when it is invoked. Each argument is
aligned to a power of 2 greater than or equal to its total size. A compiler pass
will have transformed the kernel so that all kernel argument accesses will index
their corresponding offsets in the argument pack instead. During kernel
execution, as an argument is required, it will be read from the argument pack.
We chose to do this to simplify the loading process for a small increase in
complexity of the compilation process, however by now this method has been tried
and tested.


----
## Local Variables

Local variables are currently managed largely by the compiled kernel and thus
their implementation is dependant on how the MUX target compiles a kernel.

Currently the following is adhered to:
- Locals defined within a kernel will be placed on the stack.
- Local buffer arguments are not passed as a pointer but rather their size in
  the parameter pack. When the kernel executes it will reserve the specified
  size itself on the stack. A more flexible approach may be considered in the
  future to cope with local memory being in a different place.


----
## Multiple kernel Variants

For performance reasons it can be useful to create multiple distinct variants of
kernels. While they are equivalent in operation, they vary in terms of
optimization approach. At runtime the best kernel can be selected for the
current situation to maximize kernel performance.

These kernels may be distinguished by augmenting their function name within the
program binary. As the program execution API operates in terms of a kernel name
support for kernel variation is possible.

The implementation of this feature is coordinated entirely within the MUX target
and does not require direct support from the HAL. Note the current RISC-V target
does not produce multiple variants.

----

## Profiling Counters

HAL implementations can expose profiling counters, allowing kernel execution and
memory operations to be profiled. Counters can have a single value, or have
multiple 'sub-values', for example a counter for cycles elapsed can have a value
for each hardware thread.

If a HAL does not support any counters then all an implementation needs to do is
set `num_counters` in the device info to `0`. 

### Defining counters

Counters are defined on a per-device basis. To define counters, the following
fields of the `hal_device_info_t` struct must be set:

```cpp
  uint32_t num_counters;
  hal_counter_description_t *counter_descriptions;
```

The value of `num_counters` should be the number of counters the device
supports, and `counter_descriptions` should point to an array of size
`num_counters`.

The layout of `hal_counter_description_t` is as follows.

```cpp
struct hal_counter_description_t {
  /// @brief A unique id for this counter.
  uint32_t counter_id;
  /// @brief Short-form name for this counter, e.g. "cycles"
  const char *name;
  /// @brief Descriptive name for this counter, e.g. "elapsed cycles"
  const char *description;
  /// @brief Used if contained_value > 1. Descriptive name for what each
  /// contained value represents, e.g. "core" for per-core contained values
  const char *sub_value_name;
  /// @brief The number of contained values within this counter. Must be at
  /// least 1.
  uint32_t contained_values;
  /// @brief The unit to display this counter's value with.
  hal_counter_unit_t unit;
  /// @brief Configuration for displaying this counter in profiling logs
  hal_counter_log_config_t log_cfg;
};
```

The `log_cfg` field contains configuration options for how to report the counter
values to the user. The layout of `hal_counter_log_config_t` is as follows.

```cpp
struct hal_counter_log_config_t {
  /// @brief A hint describing the minimum log/verbosity level at which to
  /// display the individual values of this counter. Set to 'none' to not
  /// display the counter in this way at any level.
  hal_counter_verbosity_t min_verbosity_per_value;
  /// @brief A hint describing the minimum log/verbosity level at which to
  /// display the overall total value of this counter. Set to 'none' to not
  /// display the counter in this way at any level.
  hal_counter_verbosity_t min_verbosity_total;
};
```

The `min_verbosity_*` fields are hints to users of the counter values. For
example, a counter can be defined with a per-value verbosity of medium and a 
total value verbosity of low. If a user runs with a profiling log level of
low, they will see the total value for that counter displayed but not the
individual values. If they run with a log level of medium or high, they will
see both.

### Setting counter values

HAL implementations must set counter values when events occur that should update
them. It is not necessary to update every counter after every event; for example
a retired instructions counter would not be updated by a mem_write event.

It is the responsibility of the user of the HAL to check for new counter values
where appropriate. In practice this means the user will check for new counter
values after calling one of the following device functions:

* kernel_exec
* mem_write
* mem_read
* mem_fill
* mem_copy

The HAL device should implement the following function to facilitate reading
new counter values.

```cpp
  virtual bool counter_read(uint32_t counter_id, uint64_t &out,
                            uint32_t index = 0) = 0;
```

It must write out the counter value if there is a new unread counter value for
that particular counter ID and value index. Once the function as been called for
that particular value it should be considered read. If there is no new value it
will return false.

The HAL provides the following utility class to help HAL implementations track
counter data.

```cpp
// See HAL/include/counters.h for the full implementation
struct hal_counter_value_t {
  hal_counter_value_t(uint32_t counter_id, uint32_t num_values);
  bool has_value(uint32_t value_index);
  void clear_value(uint32_t value_index);
  void set_value(uint32_t value_index, uint64_t value);
  uint64_t get_value(uint32_t value_index);
};
```

# Clik Specific interfaces and cmake utility functions

`clik` is a tool which can be used with the `HAL` to create and test a `HAL`. It
requires some specific interfaces to be added to the target `HAL`, which are
detailed under this section. `hal_cpu` will be used to demonstrate various
features and can be found under `external/hal_cpu` under the top level of the
`clik` source.

We will discuss some utility `cmake` functions that can be used by the target
`HAL`, as well as describe the following required interfaces:

* Required `cmake` properties
* Clik `cmake` functions required
* Clik header files
* Example test kernel entry points

## Required cmake properties

`clik` uses the property `KNOWN_HAL_DEVICES` to allow it to iterate through the
`HAL` targets. This requires the name of the `HAL` to be appended in the `HAL`
target `cmake` as follows:

```cmake
  set_property(GLOBAL APPEND PROPERTY KNOWN_HAL_DEVICES "<hal_name>")
```

## Cmake Utility functions

The following utility functions are provided by the base `HAL` and are used by `clik`:

* hal_add_bin2h_command
* hal_add_bin2h_target
* hal_add_baked_data

These are described in detail under `HAL/cmake` source directory.

In general these don't need to be known by the target `HAL` but are available for
use where it is necessary to turn data into a C style header file, often referred
to as baking data. This might include things like a linker script.

For example `hal_add_baked_data` is used in `hal_cpu` to bake the linker script
(which can then be included in the target `HAL`) as follows:

```cmake
hal_add_baked_data(hal_cpu
    hal_cpu_linker_script
    linker_script.h
    ${HAL_CPU_SOURCE_DIR}/include/device/program.lds)
```

This takes the input `program.lds` and turns it into a header file `linker_script.h`.

## Clik cmake functions required

The following cmake functions are required to be provided by the `HAL`.

* hal_<hal_name>_compile_kernel_source
* hal_<hal_name>_link_kernel

These would normally be added under a cmake directory in a file called
`CompileKernel.cmake` and included from the top level CMakeLists.txt as follows:

```cmake
set(HAL_<HAL_NAME_CAPITALS>_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND CMAKE_MODULE_PATH ${HAL_<HAL_NAME_CAPITALS>_SOURCE_DIR}/cmake)

include(CompileKernel)
```

### hal_<hal_name>_compile_kernel_source

This is required to compile a source file into an object file. It looks like:

```cmake
function(hal_<hal_name>_compile_kernel_source OBJECT SRC)
```

* `OBJECT` is the object file to generate.
* `SRC` is the c file to compile.
* `{ARGN}` will give a list of additional include directories, currently the
  original source directory where the test is defined

It may be advisable to set a property to the top level target HAL source
directory, as `${CMAKE_CURRENT_SOURCE_DIR}` will reflect where the function is
called from inside the function.

These can be used to generate include paths in order to compile the function.

Ultimately the function should call `add_custom_command` to compile the source
into `${OBJECT}`, with dependencies on `${SRC}`.

The below shows how `hal_cpu` does this:

```cmake
function(hal_cpu_compile_kernel_source OBJECT SRC)
  # These are include dirs passed to the function, will include the test directory
  set(INCLUDES ${ARGN})
  # get the property for the HAL CPU top level directory
  get_property(ROOT_DIR GLOBAL PROPERTY HAL_CPU_DIR)

  # Add include/device to the include list
  set(DEVICE_INCLUDE_DIR ${ROOT_DIR}/include/device)
  list(APPEND INCLUDES ${DEVICE_INCLUDE_DIR})
  # debug flags
  set(OPT -O2)
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(OPT -O0 -g)
  endif()
  # set up includes
  set(EXTRA_CFLAGS)
  foreach(INCLUDE ${INCLUDES})
    set(EXTRA_CFLAGS ${EXTRA_CFLAGS} -I${INCLUDE})
  endforeach()
  # create custom command
  add_custom_command(OUTPUT ${OBJECT}
                     COMMAND ${CMAKE_C_COMPILER} ${OPT} -c -fPIC -DBUILD_FOR_DEVICE ${EXTRA_CFLAGS} ${SRC} -o ${OBJECT}
                     DEPENDS ${SRC})
endfunction()
```

### hal_<hal_name>_link_kernel

This should link files together to generate a final binary.

It looks like:

```cmake
function(hal_<hal_name>_link_kernel BINARY)
```

Additionally `${ARGN}` will contain a list of object files. This should finally
call a custom command which depends on `${ARGN}` and calls a linker to output to
`${BINARY}`. The `hal_cpu` example is as follows:

```cmake
function(hal_cpu_link_kernel BINARY)
  set(OBJECTS ${ARGN})
  add_custom_command(OUTPUT ${BINARY}
                     COMMAND ${CMAKE_C_COMPILER} -shared ${OBJECTS} -o ${BINARY}
                     DEPENDS ${OBJECTS})
endfunction()
```

## Clik header files

`clik` requires an additional header file `kernel_if.h` which is included in all
which is included by all `clik` kernels. This provides interface information on
structures and functions which it requires to build the kernels. The include path
to this file (and any it might include) needs to be added in
`hal_<hal_name>_compile_kernel_source`.

There are two main things that need to be provided in this file:

* Some types need defined (possibly via a macro) such as __global
* Define `exec_state_t` which is passed down to the lowest level of the kernels,
  as well as any other required structures.
* Define a number of builtin functions which are used in the tests. These include
  builtins such as `get_global_id()` as well as functions such as `print`.
  Note these functions should be done as `static inline`.

### Types

There are a number of types used in `clik` kernels. These are:

* uint
* __kernel
* __global
* __constant
* __local
* __local_variable
* __private

`uint` is the same as `OpenCL` and represents a 32 bit unsigned integer.
`__kernel` is used to designate the bottom level kernel and typically would be
defined as nothing. `__global`, `__local`, `__constant` and `__private` are used
to refer to different address spaces and match the `OpenCL` concepts of memory
types. On a single memory space these would typically just be defined as nothing
(see `hal_cpu`). `__local_variable` is used to indicate a variable being defined
as being in `local` memory (ie., across the work-group). On `hal_cpu`, amongst
others, this is defined as:

```cpp
static __attribute__((section(".local")))
```

### Builtin functions

All of the builtin functions will be based off the `exec_state_t` structure.

The following functions should be defined in `kernel_if.h` or included from there
if the example tests which use them are defined: The `get_*` builtins use the
same names as `OpenCL` and follow the same logic.

```cpp
 uint32_t get_work_dim(exec_state_t *e)
 uint32_t get_global_id(uint32_t rank, exec_state_t *e)
 uint32_t get_local_id(uint32_t rank, exec_state_t *e)
 uint32_t get_group_id(uint32_t rank, exec_state_t *e)
 uint32_t get_global_offset(uint32_t rank, exec_state_t *e)
 uint32_t get_local_size(uint32_t rank, exec_state_t *e)
 uint32_t get_global_size(uint32_t rank, exec_state_t *e)
 int print(exec_state_t *e, const char *fmt, ...)
 uintptr_t start_dma(void *dst, const void *src, size_t size_in_bytes,
                         struct exec_state *e)
 void wait_dma(uintptr_t xfer_id, struct exec_state *e)
 void barrier(struct exec_state *e)
```

Most of the tests use either `get_global_id` and/or `get_local_id` and the
majority of tests can pass with these defined. `get_work_dim` and
`get_global_offset` are not currently used in any tests.

`hello` and `barrier_print` requires the `print` builtin. This function is the
same as `printf` and requires some effort on non cpu devices, but can be very
useful for debugging.

`device_concatenate_dma` requires the `start_dma` and `wait_dma` function. These
should be thought of as an asynchronous `memcpy`. Some targets such as `hal_cpu`
implement this as `memcpy` directly and write `wait_dma` as a no-op.

`barrier` is required for `barrier_sum`, `barrier_print` and
`matrix_multiply_tiled`. It acts to ensure that all work items in a work group
have completed at he point of the barrier. This tends to be used only for
interfaces where we work on a work item per thread and typically would be a call
to some sort of hardware fence.

### exec_state_t

All of the example tests take an `exec_state_t` as a parameter. This will be
passed down from the target entry point interfaces.

`exec_state_t` can contain any information and is not directly referenced in the
test other than as an opaque structure which it passes to builtin functions. It
needs to contain enough information for the various tests to work. This might
include for example a thread id to work out the `local id` or the `id` of the
current work group. It may also include function pointers for printing etc which
some tests require.

## example test kernel interfaces

Each test in `clik` is defined at the lowest level of acting on a single work
item, in a similar way to `OpenCL`. An example for this is `vector_add`:

```cpp
__kernel void vector_add(__global uint *src1, __global uint *src2,
                         __global uint *dst, exec_state_t *item) {
  uint tid = get_global_id(0, item);
  dst[tid] = src1[tid] + src2[tid];
}
```

Every hardware interface to kernels may be different so it is required that the
`HAL` provides interfaces to these examples to work in `clik`. This is done by
replicating the examples directory structure and providing the actual function
for that test (normally `kernel_main`). This should be done in a file called
`device_<test_name>_entry.c`. These can be done optionally, so for tests not yet
supported the directory can be omitted entirely. It is necessary to set a
property in the top level `CMakeLists.txt` to indicate where to look:

```cmake
set_property(GLOBAL PROPERTY HAL_CPU_EXAMPLE_DIR "${HAL_<hal_name_capitals>/<relative_directory_path>")
```

Each device entry has access to a header file associated with that test. This
header file includes the low level kernel function descriptor which must be
called from the entry point and a structure containing the packed arguments.

The actual interface to the kernel can be anything that works for the particular
architecture, but will require some way of transforming of the inputs into the
kernel arguments and an `exec_state_t`. `hal_cpu` shows one possible way:-

```cpp
#include "device_vector_add.h"

// Execute the kernel once for each work-group. This function is called on each
// hardware thread of the device. Together, all hardware threads on the device
// execute the same work-group. The N-D range can also be divided into slices in
// order to have more control over how work-groups are mapped to hardware
// threads.
void kernel_main(const vector_add_args *args, exec_state_t *ctx) {
  wg_info_t *wg = &ctx->wg;
  ctx->local_id[0] = ctx->thread_id;
  for (uint i = 0; i < wg->num_groups[0]; i++) {
    wg->group_id[0] = i;
    vector_add(args->src1, args->src2, args->dst, ctx);
  }
}
```

In this case we take the same `exec_state_t` as is being used in the called
kernel as part of the signature. The first argument here is the packed argument
struct. The above is an example of work item per thread, where each call to
`kernel_main` happens once for each work item in a group and each call to this
entry point is expected to perform `vector_add` once per work item but iterate
over all the work groups. This also writes to an element within that struct to
set the `group_id`.

Although this entry point is conveniently similar to the final called kernel it
could be quite different e.g. no arguments at all, and gather information from a
written to part of memory to generate the arguments and `exec_state_t`.
