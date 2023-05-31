Creating a New ComputeMux Target: Quick Start
=============================================

While adding support for running OpenCL and SYCL applications on new hardware
may seem like a complex and lengthy endeavor, the oneAPI Construction Kit
provides several tools, libraries and facilities to greatly speed this process
up. In many cases it is possible to get simple compute kernels running on
hardware very quickly and with minimal development effort. This is done by using
clik and HAL as scaffolding for creating a new device target. Once basic compute
capabilities have been implemented and successfully tested, this target can easily
be adapted to work with ComputeMux and integrated with the oneAPI Construction Kit.
By that stage the scaffolding of clik will have been removed, and the structure of
HAL can also be removed if it is hindering supporting advanced features or
optimization opportunities on the target hardware.

Clik
----

clik is a simple compute library that demonstrates how offloading code to an
accelerator works. Multiple devices can be supported through 'HALs', which are
libraries handling tasks such as memory allocation, data transfers and kernel
execution on the device. A reference HAL that targets the system's CPU is
included with clik. New HALs targeting different devices can easily be created
and used through the same framework.

clik can be used as an introduction to how offloading to an accelerator works,
but also as scaffolding when adding support for a new device in the oneAPI
Construction Kit. It has been designed to be as simple as possible, so that it is
easy to understand and troubleshoot. Several basic examples are included, covering
essential functions such as creating buffers, data transfers, executing kernels,
work-group barriers and local memory. When these examples all run successfully,
clik's role as scaffolding is complete. However, it is also possible to add new
examples as needed.

clik kernels are written in C and can be compiled using a stand-alone compiler
such as GCC. This means that compared to more advanced compute libraries a
compiler back-end does not need to be integrated into the compute library. As a
result, bringing up a new target can be faster due to being able to develop the
runtime part of the device code before needing to integrate a compiler.

The concept of HAL, a library that abstracts the interface to a device, is
central to clik. This framework has been designed with the idea that multiple
devices can be targeted using the same simple API and examples. In addition, a
HAL that has been developed using clik can then be integrated in the oneAPI
Construction Kit. The source code for the HAL does not need to be rewritten for
the oneAPI Construction Kit, instead it can seamlessly be loaded by a generic
ComputeMux target.

HAL
---

The device hardware abstraction layer (HAL) is an API and specification to
enable an oneAPI Construction Kit target to interface with a multitude of compute
devices. A HAL separates a ComputeMux target from the specifics of device
interaction. By introducing this interface, one target can execute code without
change on multiple devices.  New devices can also be brought up quickly as they
only need to expose the HAL interface.

For details see the :doc:`HAL Specification </specifications/hal>`, this
section outlines how to use that specification within a ComputeMux
implementation if you wish to do so.

To form a working OpenCL and SYCL implementation by following the HAL approach
three elements are required:

1. This HAL API, which is provided as part of the oneAPI Construction Kit.
2. A ComputeMux target that supports the HAL interface. The RISC-V ComputeMux
   target is the reference for using the HAL interface and is also provided as
   part of the oneAPI Construction Kit.
3. A HAL implementation which has to be developed for the particular hardware
   device that is targeted. oneAPI Construction Kit comes with the Spike HAL
   which allows executing compute kernels compiled to RISC-V using the Spike
   simulator.

The HAL abstracts the fundamental operations the hardware needs to perform in
order to be used to run compute applications. This includes managing memory
transfers to and from the device, allocating device memory and executing kernels
on hardware. The provided HAL API is very simple, so that getting started is
a quick and easy process. As a result it may not allow for all hardware features
to be exposed initially or enable the full performance of the hardware to be
harnessed. However, it is meant as a stepping stone and intended to be used only
for the initial phase of hardware bring-up.

Quick Start Process
-------------------

In order to quickly run compute kernels on new hardware, the suggested process
is to create a new HAL for that hardware and build it as part of clik. The test
suite provided with clik is then used to ensure that basic compute features such
as executing kernels, using local memory and work-group barriers are working on
the target hardware. Once this is done, the new HAL can be used with a reference
ComputeMux target that provides a bridge between a HAL and the oneAPI Construction
Kit. With this target, a much broader set of compute tests and applications can be
executed on the device, including the OpenCL and SYCL CTS test suites. This
allows further development of the target so that a larger feature set can be
exposed to the user. Once the stability and feature set of the device HAL
reaches a desired level, the reference ComputeMux target can be repurposed as a
ComputeMux target for the device. Hardware-specific changes can then be done to
the target and the HAL API removed in favor of using the device-specific code
directly.
