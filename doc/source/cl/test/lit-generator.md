# Lit test generator

Set of scripts to generate lit tests from arbitrary OpenCL kernels.

This directory contains two scripts:

```
- create_tests.sh
- analyze_scripts.py
```

## `create_tests.sh`

```sh
create_tests.sh [-h] [--help] [-v] [-r REPEAT_COUNT] [-a ALT_KERNEL_DIR]
                [-c COMPARISON_DIR] -k KERNEL_DIR -b BINARY_DIR -l LIT_DEST_DIR

mandatory arguments:
-k KERNEL_DIR      Path to OpenCL kernels.
-b BINARY_DIR      Path to OCLC and OpenCL dynamic library.
-l LIT_DEST_DIR    Path to GeneratedTests directory.

optional arguments:
-v                 Run with verbose output, and save generated files.
-c COMPARISON_DIR  Path to alternative OpenCL driver. If not specified,
                   the driver in BINARY_DIR will be used.
-a ALT_KERNEL_DIR  Path to the OpenCL kernels directory relative to
                   LIT_DEST_DIR/Tests. This only needs to be set if the
                   generated lit tests are meant to be run on a machine
                   separate from the one that this script is being run on.
                   For tests to be run on Jenkins, set this to
                   'Inputs/KernelsCL/kernels/PerfCL'.
-r REPEAT_COUNT    Number of times to run each kernel. Higher values
                   increase run time, but decrease the chances of
                   generating a lit test that crashes or is inconsistent
                   default value is 1. This flag is only needed for debugging.
```

## `analyze_kernels.py`

```sh
analyze_kernels.py [-h] [--binary_dir BINARY_DIR] [--oclc OCLC]
                   [--kernel KERNEL] [--param PARAM] [--lit LIT]
                   [--alt_filename ALT_FILENAME]
                   file

positional arguments:
  file                          Name of the file where the kernel is found.

optional arguments:
  -h, --help                    Show this help message and exit.
  --binary_dir BINARY_DIR       Path to OCLC binary.
  --kernel KERNEL               Kernel name.
  --debug DEBUG                 Print buffer access values at various stages of
                                evaluation.
  --alt_filename ALT_FILENAME   File kernel is found in when lit runs, not
                                neccessarily same as `file`.
```

## Generating and using tests

To generate a set of lit tests from a set of .cl files, run `create_tests.sh`
with all arguments except for -r specified. After execution, the resulting tests
will be found in GeneratedTests/Tests, overwriting anything that was there
already. Like other lit tests these files must be transferred to a build
directory before they can be run by triggering the GeneratedTests target
using make or ninja. Calling lit on OCL/build/test/GeneratedTests will run all
generated tests.

## Jenkins

Kernels which are to be automatically tested by this system should be added to
the `kernels/PerfCL` directory of the KernelsCL ComputeAorta repo. Tests
generated from these kernels in `GeneratedTests` will then run automatically
after the merge request tester has finished, in the ocl_generated_tests job.

## Developing and debugging

To extend the number of lit tests generated, you should modify
`analyze_kernels.py` - the script responsible for generating kernel arguments in
the lit tests. If the -v flag is passed to `create_tests.sh`, it will create a
`files/failures` directory which will list unsupported kernels, classified by
the error messages produced. This is useful when deciding which types of kernels
it would be most useful to support.

`analyze_kernels.py` can be run directly with the --binary_dir,--kernel, and
--debug flags to see what arguments will be supplied to a particular kernel.
the --debug flag should allow you to see which of the major sections of buffer
access evaluation is causing the kernel to be unsupported, aiding debugging

## Running with an alternate driver

To use this tool effectively, you should have a different OpenCL implementation
to compare values with. For example, to get Intel's Linux OpenCL driver,follow
these steps:

1. From [Intel's
   site](https://software.intel.com/en-us/articles/opencl-drivers#latest_linux_SDK_release)
   download the zip file titled 'intel-opencl-r5.0 (SRB5.0) Linux driver
   package' under the 'OpenCL™ 2.0 GPU/CPU driver package for Linux\* (64-bit)'
   heading.
2. Extract `intel-opencl-r5.0-XXXX.x86_64.tar.xz`, and extract the
   `opt/intel/opencl` directory within it to a location of your choice.
3. When running `create_tests.sh`, set the `COMPARISON_DIR` flag to the `opencl`
   directory.

## Limitations

Currently, `analyze_kernels.py` does not support every OpenCL kernel. OpenCL C
language constructs which are known not to work with these scripts include:

* while loops
* case statements
* non-inlined functions
* many built in functions
* structs
