# Device Hardware Abstraction Layer

### Introduction

The device hardware abstraction layer (HAL) is an API and specification to
enable a ComputeAorta target to interface with a multitude of compute devices.
A HAL separates a MUX target from the specifics of device interaction.
By introducing this interface, one target can execute core without change on
multiple devices.  New devices can also be brought up quickly as they only
need to expose the HAL interface.

To form a working OpenCL implementation three elements are required:

- This HAL API
- A MUX target that supports the HAL interface
- A HAL implementation

It should be noted that this HAL interface by itself is simply an API and
library of utility code to be used by a HAL implementation.  By itself this
repository should not be considered standalone and should not be built in
isolation.


### Current state

Currently only the RISC-V `ComputeAorta` target and `clik` support and use HAL
implementations. The only public implementations of a HAL are:
 
- RISC-V `RefSi` reference HAL and `Refsi tutorial` HAL
- The `clik` cpu HAL


### Specification

For the specifics of the HAL interface, please read the
(hal.md)[docs/hal.md] documentation.
