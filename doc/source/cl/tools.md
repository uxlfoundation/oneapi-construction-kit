# Tools

## `clc` OpenCL Offline Kernel Compiler

`clc` is a command-line tool that allows compiling OpenCL C, SPIR and SPIR-V
kernels to an implementation-defined binary format, it does not tie in to any
particular Mux target implementation.

### `clc` Use-Cases

Checking a kernel for compile errors without outputting a binary:

```bash
clc -n kernel.cl
```

Compile a kernel to a file:

```bash
clc -o clbin/kernel.bin src/kernel.cl
```

Choose the ComputeAorta's OpenCL implementation on a multi-device system:

```bash
clc -dn "host CPU" -o clbin/kernel.bin src/kernel.cl
```

Compile a kernel with a given include path and definitions:

```bash
clc -Ilib1/cl -Ilib2/cl -DUSE_FEATURE1 -DANSWER=42 -o kernel.bin kernel.cl
```

Save the target-generated binary without the CL-specific binary header for
inspection: (it will not be possible to load it using the OpenCL API)

```bash
clc --strip-binary-header -o kernel_target.bin src/kernel.cl
```

### `clc` Usage

Command-line usage:

```
clc [options] [--] [file1.h file2.h inputfile.cl OR spirfile.bc OR spirvfile.spv]
```

The input files will be concatenated in the order specified and passed to
clBuildProgram.

Supported options:

```
optional arguments:
  -h, --help            show this message and exit
  --version             show program's version number and exit
  -v, --verbose         show more information during execution
  -n, --no-output       suppresses generation of the output file, but runs the
                        rest of the compilation process
  -o file, --output file
                        output file path, defaults to the name of the last
                        input file "<input>.bin" if present, or "-" otherwise
                        to write to standard output
  -d name, --device name
                        a substring of the device name to select
  --list-devices        print the list of available devices and exit
  -X opt                passes an option directly to the OpenCL compiler

optional preprocessor arguments:
  -D name               predefine name as a macro, with definition 1
  -D name=definition    the contents of definition are tokenized and processed
                        as if they appeared during translation phase three in a
                        `#define' directive. In particular, the definition will
                        be truncated by embedded newline characters
  -I dir                adds a directory to the list to be searched for headers

optional math intrinsics arguments:
  -cl-single-precision-constant
                        treat double precision floating-point constants as
                        single precision constants
  -cl-denorms-are-zero  allows flushes of denormalized numbers to zero for
                        optimization

optional optimization arguments:
  -cl-opt-disable       this option disables all optimizations
  -cl-mad-enable        allow a * b + c to be replaced by a mad with reduced
                        accuracy
  -cl-no-signed-zeros   allow optimizations for floating-point arithmetic that
                        ignore the signedness of zero
  -cl-unsafe-math-optimizations
                        allow optimizations for floating-point arithmetic that
                        may violate IEEE 754
  -cl-finite-math-only  allow optimizations for floating-point arithmetic that
                        assume that arguments and results are not NaNs or
                        +/-inf
  -cl-fast-relaxed-math sets -cl-finite-math-only and
                        -cl-unsafe-math-optimizations

optional additional arguments:
  -w                    disables the OpenCL warnings
  -Werror               makes the OpenCL warnings into errors
  -cl-std={CL1.1,CL1.2} determine the OpenCL C language version to use
  -cl-kernel-arg-info   this option allows the compiler to store
                        information for clGetKernelArgInfo

optional SPIR extended arguments:
  -spir-std=1.2         chooses the version of SPIR standard to follow
                        (defaults to 1.2 if SPIR input detected)
  -x spir               indicates the input is in SPIR format (added
                        automatically if SPIR input detected)

optional ComputeAorta extended arguments:
  --strip-binary-header strips the header containing argument and kernel count
                        information, leaving only the binary directly from the
                        target implementation WARNING: The output binary cannot
                        be loaded by the ComputeAorta runtime again!
  -codeplay-soft-math   inhibit use of LLVM intrinsics for mathematical builtins
  -g                    enables generation of debug information, for best
                        results use in combination with -S
  -S file               path of the source file used for source locations when
                        -g is specified
  -cl-llvm-stats        enable reporting LLVM statistics
  -cl-wfv={always,auto,never}
                        sets whole function vectorization mode
  -cl-vec={none,loop,slp,all}
                        enables kernel early vectorization passes
```

Any other options not defined above will be passed to clBuildProgram unchanged.

Input and output files are both set to "-" by default to use standard
input/output. The default output path when only an input file was given is:
`lastinfile.bin`.

## `oclc` OpenCL-C Compiler and Intermediate State Kernel Inspector

`oclc` is a command-line tool that allows compiling OpenCL C kernels to LLVM IR
(.ll) or assembly (.s). Its primary purpose is to provide insight into the
operation of the compiler components of ComputeAorta, whether to aid debugging
or the improve the quality of the generated code.

`oclc` may also be used to execute OpenCL kernels with specified parameters, and
view parameter values post-execution.

`oclc` requires an OpenCL 1.2 implementation that implements the
`cl_codeplay_program_snapshot` extension. In practice this means a correctly
configured version of ComputeAorta's OpenCL implementation.

Codeplay's snapshot extension works by capturing kernels at specified stages in
compilation. Then returning a dump of that capture, the snapshot, via a callback
to user code. `oclc` contains such a callback which performs the task of
printing the snapshot to standard out if the format is textual (.ll or .s), or
to a file for binary format (.bc or .o).

The available snapshot stages can be queried through the extension interface.
Communicating with the compiler module to get possible stages from the target.
As a result there is now no hard-coded list of snapshot stages in `oclc`, since
the available stages change depending on target. Instead the stage the user sets
with `-stage` now finds the first substring match from the list of possible
snapshot stages.

> Note that although this is an offline compile tool it hooks directly into the
> ComputeAorta OpenCL library, and thus any environment variables that affect
> the libraries behaviour will also affect `oclc`.

### `oclc` Use-Cases

If you want to see the list of possible stages to set a snapshot at the `-list`
flag should be used:

```bash
./oclc -list foo.cl
cl_snapshot_compilation_default
cl_snapshot_compilation_front_end
cl_snapshot_compilation_linking
cl_snapshot_compilation_simd_prepare
cl_snapshot_compilation_scalarized
cl_snapshot_compilation_linearized
cl_snapshot_compilation_simd_packetized
cl_snapshot_compilation_spir
cl_snapshot_compilation_builtins_materialized
cl_snapshot_compilation_barrier_expansion
cl_snapshot_compilation_backend
cl_snapshot_compilation_mc_final
```

> Note: These are the stages available for the Host target at time of writing.
> Some of the later use-cases specify stages from the legacy X86 target since
> it supports the vectorizer.

Sometimes a snapshot stage may not be hit if it requires a kernel to be
enqueued. To enqueue a kernel the `-enqueue` option can be used with a specified
kernel name to enqueue it as a task (single work item). The kernel won't
actually be run however, which is expected behaviour due to user events inside
`oclc`:

```bash
oclc foo.cl -stage cl_snapshot_host_scheduled -enqueue my_kernel
```

To pass build flags through to the OpenCL compiler `-cl-options` can be used,
this is particularly useful for enabling debug info:

```bash
oclc foo.cl -cl-options "-g -cl-opt-disable" -stage spir
```

If you want to see what the LLVM IR looks like before any optimizations or
rewriting:

```bash
./oclc -stage frontend foo.cl > foo.ll
```

If you want to see the output from the vectorizer, in LLVM IR, before builtins
are inlined or barriers are rewritten:

```bash
# The vectorizer does not run unless the `-cl-wfv=always` or `-cl-wfv=auto`
# option is provided.
./oclc -cl-options "-cl-wfv=always" -stage packetized foo.cl > foo.ll
```

> Note: Previously setting the stage to `simd` would match the
> `cl_snapshot_compilation_simd_packetized` stage. However after the change
> removing hard-coded stages from `oclc` `simd` now matches
> `cl_snapshot_compilation_simd_prepare`, as it's the first stage with `simd` as
> a substring.

If you want to see the assembly output for the auto-detected CPU, with
vectorization (and inlining of builtins, rewriting of barriers, and creation of
local work-item loops):

```bash
# The vectorizer does not run unless the `-cl-wfv=always` or `-cl-wfv=auto`
# option is provided.
./oclc -cl-options "-cl-wfv=always" -stage mc foo.cl > foo.S
```

### `oclc` Usage

```
./bin/oclc [options] <CL kernel file>
```

For textual outputs the result will be printed directly, however binary outputs
are saved to a file by default. These files will be named after the original
file path plus [.bc|.o] appended, e.g. `foo.cl.bc`. To set a more convenient
output file manually use the `-o` option which doesn't add any file extension,
this can also be used for textual outputs.

When using `oclc` to view generated LLVM IR/assembly, you may ignore all options
from `-execute` downward. When using `oclc` to execute a kernel, all kernel
arguments should be specified at least once using one of the `-arg`, `-print`,
`-show`, or `-compare` flags.

Program options:

```
-o <output_file>                                        Set the output file to write the snapshot to.
-v                                                      Run oclc in verbose mode.
-format <output_format>                                 Set the output file format.
-stage <compilation_stage>                              Set the compilation stage to take a snapshot at.
                                                        Matches the first occurrence of stage as a substring
                                                        against options from '-list'.
-cl-options 'options...'                                OpenCL options to use when compiling the kernel.
-list                                                   List the compilation stages oclc can take a snapshot at.
-enqueue <kernel name>                                  Enqueues a kernel to hit snapshots on work-group
                                                        size specific transformations.
-execute                                                Executes the enqueued kernel.
-seed <value>                                           Set the seed of the random number engine used in rand() calls.
                                                        The seed is set to a default value if this is not set.
-arg <name>[,<width>[,<height>]],<list>                 Assigns a list value (as described below) to the
                                                        named argument when the kernel is executed.
                                                        If the argument is a 2D image, a width in pixels must be provided.
                                                        if the argument is a 3D image, a height in pixels must also be provided.
                                                        if the argument is an image, 4 values must be provided per pixel,
                                                        as images are treated as unsigned 8 bit RGBA arrays by default.
-arg <name>[,<width>[,<height>]],<list>:<filename>      Assigns a list value (as described below), held in a
                                                        file, to the named argument when the kernel is executed.
-print <name>[,<offset>],<size>                         Prints a given number of elements from the given
                                                        named argument after execution to stdout, possibly
                                                        starting from some offset.
-print <name>[,<offset>],<size>:<filename>              Prints a given number of elements from the given
                                                        named argument after execution to a file, possibly
                                                        starting from some offset.
-show <name>,<width>,[,<height>[,<depth>]][:<filename>] Prints the named image argument of the specified size to stdout,
                                                        or a file, if one is provided.
-compare <name>,<expected>                              Compares the named buffer to an expected list.
-compare <name>:<filename>                              Compares the named buffer to an expected list, held in a file.
-global <g1>,<g2>,...                                   Sets the global work size to the given array of values.
-local <l1>,<l2>,...                                    Sets the local work size to the given array of values.
-ulp-error <tolerance>                                  Sets the maximum ULP error between the actual and target values accepted.
                                                        as a 'match' when -compare is applied to float or double values. Defaults to 0.
-char-error <tolerance>                                 Sets the maximum difference between the actual and target values accepted
                                                        as a 'match' when -compare is applied to char or uchar values. Defaults to 0.
-repeat-execution <N>                                   Executes the kernel N times. -global, -local, and -arg
                                                        arguments may be set to {<list>},{<list>},... to take on
                                                        different values on each execution.
```

Acceptable kernel argument values:

```
<list>  ::= <el>
         |  <el> "," <list>
         |  <cl_bool> "," <cl_addressing_mode> "," <cl_filter_mode>" (for specifying sampler_t only)

<el>    ::= <integer or decimal>
         |  "repeat(" <unsigned integer> "," <list> ")"
         |  "rand(" <decimal> "," <decimal> ")"
         |  "randint(" <decimal> "," <decimal> ")"
         |  "range(" <integer or decimal> "," <integer or decimal> ")"
         |  "range(" <integer or decimal> "," <integer or decimal> "," <integer or decimal> ")"

<cl_bool>            ::= "CL_TRUE" | "CL_FALSE"
<cl_addressing_mode> ::= "CL_ADDRESS_NONE" | "CL_ADDRESS_CLAMP_TO_EDGE" | "CL_ADDRESS_CLAMP"
                      |  "CL_ADDRESS_REPEAT" | "CL_ADDRESS_MIRRORED_REPEAT"
<cl_filter_mode>     ::= "CL_FILTER_NEAREST" | "CL_FILTER_LINEAR"
```

Special kernel argument values:

```
repeat(N,list)                              creates a list containing `list` repeated `N` times
                                            repeat(3,2,4) => 2,4,2,4,2,4
rand(min,max)                               creates a random floating point number in [min,max]
                                            rand(1.2,4) => 3.195201 (potentially)
randint(min,max)                            creates a random integer number in [min,max]
                                            randint(1,4) => 3 (potentially)
range(min,max,stride)                       produces a list beginning at `a`, moving in the direction of `b`
                                            by `stride` units. if `stride` is not stated, it defaults to 1.
                                            range(-4,21,5) => -4,1,6,11,16,21
```

Available format types (for use with -format):

```
text                         textual format such as LLVM IR or assembly
binary                       binary format such as LLVM BC or ELF
```

Note that the SPIR output in the above does not match the SPIR-1.2
specification, as that requires LLVM IR output from LLVM 3.2. It should not be
assumed that other OpenCL implementations will be able to consume this SPIR, if
you want to do that use the [Khronos SPIR
generator](https://github.com/KhronosGroup/SPIR).
