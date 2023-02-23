# Kernel Binary Generation

To generate the binary input files we need from DPC++:

1. Compile sycl program with DPC++ compiler.
2. Run the sycl program generated with `SYCL_DUMP_IMAGES`
```cpp
SYCL_DUMP_IMAGES=true ./simple-sycl-app-cuda.exe
```
You can find the binary file in the same directory of executing the above command.

## CUDA

The generated ptx binary file for cuda will be called `sycl_nvptx641.bin`

## OPENCL and LEVEL-ZERO

There will be 3 generated spirv files for those backends which are: 
`sycl_spir641.spv` - `sycl_spir642.spv` - `sycl_spir643.spv`. The one we need
is the third one as it contains the first 2 files data inside it.
