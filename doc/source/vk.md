# Vulkan

Codeplay's compute only implementation of the [Vulkan API][vk-reg].

The Vulkan specification comes in both [html][vk-html] and [pdf][vk-pdf] form,
this is the canonical reference text for the standard. Portions of the Vulkan
specification target graphics, this driver does not attempt to implement these
features, _only_ the compute portions of the specification are targeted.

The open source [Vulkan-CTS][vk-cts] repository contains the Conformance Test
Suite (CTS), it is necessary to pass these tests in order to state that a driver
conforms to the Vulkan specification. At the time of writing the CTS does not
cover compute only implementations.

## Dependencies

The [Vulkan SDK][vk-sdk] is the only required build dependency beyond those
required for ComputeAorta as a whole. Ensure it is installed on your system.

> On Linux pre-built binaries for some tools are not supplied with the
> tarball distribution or older Ubuntu packages of the [SDK][vk-sdk] and must be
> compiled by the user. Ensure that the `build_tools.sh` script has been invoked
> to build the tools. In addition, ensure that the `setup-env.sh` script has
> been sourced before configuring the CMake build directory to make sure that
> Vulkan support is enabled.

## Building

Vulkan support is enabled when:

* If the `VULKAN_SDK` environment variable is set and points to a valid
  [Vulkan SDK][vk-sdk] install, see [dependencies](#dependencies).
  The installation of newer Vulkan SDK for Ubuntu packages does not require
  the `VULKAN_SDK` environment variable to be set as CMake will automatically
  find the SDK in the standard system directories.
* `CA_ENABLE_API` CMake variable is an empty string `""` or contains `"vk"`

## spirv-ll

As specified in [Appendix A][vk-spirv] of the [spec][vk-html] an implementation
must consume [SPIR-V][spirv-reg] supporting the [GLSL extended instruction
set][spirv-ext]. ComputeAorta uses [LLVM][llvm] so `spirv-ll` provides a
[SPIR-V][spirv-reg] to [LLVM-IR][llvm-ir] translator to implement
[SPIR-V][spirv-reg] consumption.

### Generation

[SPIR-V][spirv-reg] provides machine readable descriptions of the instructions
in the core spec and [extended specs][spirv-ext]. `spirv-ll` takes advantage of
these JSON files to generate C++ source to enable working with
[SPIR-V][spirv-reg] binary data. These C++ files are generated using the
following Python scripts:

* `generate_builder_calls_inc.py` generates `spv_builder_calls.inc.h`, update
  and run this every time a new set of extended instructions are added
* `generate_glsl_builder_calls_cpp.py` generates `spv_glsl_builder_calls.cpp`,
  update and run this every time a new GLSL function is implemented in Abacus
* `generate_spirv_opcode_h.py` - used to generate `spv_opcodes.h` header file,
  update and run this every time a new set of extended instructions are added
* `generate_spirv_opcode_cpp.py` generates `spv_opcodes.cpp`, update and run
  this every time a new set of extended instructions are added.

> It is only required to use these generation scripts when the
> [SPIR-V][spirv-reg] standard releases a new version which ComputeAorta must
> support or if the `spirv-ll` API is to be changed.

## Optimization

This section is intended to document any areas of note for either future performance optimization or
for writing optimization guides.

* The command buffer usage flag `VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT` should be 
  avoided by users and its implementation revisited when mux is better equipped to accommodate the
  functionality it represents. Currently instead of true simultaneous use command buffers are copied
  in their entirety when simultaneous use is required and the copy is then used simultaneously.

[vk-reg]: https://www.khronos.org/registry/vulkan
[vk-html]: https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html
[vk-pdf]: https://www.khronos.org/registry/vulkan/specs/1.0/pdf/vkspec.pdf
[vk-cts]: https://github.com/KhronosGroup/Vulkan-CTS
[vk-sdk]: https://vulkan.lunarg.com
[vk-spirv]: https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#spirvenv
[spirv-reg]: https://www.khronos.org/registry/spir-v
[spirv-ext]: https://www.khronos.org/registry/spir-v/#extins
[llvm-ir]: https://llvm.org/docs/LangRef.html
