Loading Programs on the Device
------------------------------

The next HAL operations which are prerequisites to running a kernel on the RefSi
device relate to loading and unloading kernel programs. The ``program_load``
function loads a kernel binary (in this tutorial, a RISC-V ELF object) and
returns a ``hal_program_t`` handle that can be passed to ``kernel_exec`` when
executing a kernel. At a minimum, it needs to be able to extract the list of
kernels from the passed binary blob. A typical implementation will parse the ELF
object (or other executable format) into a list of sections that can be loaded
in device memory and wait until a kernel is executed before copying the binary
sections to the device. The ``program_free`` function takes a program handle and
releases the resources allocated for the program.

Loading ELF objects is quite a bit more involved than allocating or copying
memory, but thankfully the RefSi driver includes a helper class
(``ELFProgram``) which can be used to simplify the ELF loading process. A
pointer to a ``ELFProgram`` object can be used as an opaque program handle.

Alternatively, to help manage the memory for the kernels in an upcoming step,
we will introduce a wrapper around this ``ELFProgram``: it will become apparent
why later.

.. code:: c++

    // refsi_hal.h
    #include <memory>       // Added
    #include "elf_loader.h" // Added

    ...

    struct refsi_hal_program {
      refsi_hal_program(std::unique_ptr<ELFProgram> program)
          : elf(std::move(program)) {}

      std::unique_ptr<ELFProgram> elf;
    };


.. code:: c++

    // refsi_hal.cpp
    hal::hal_program_t refsi_hal_device::program_load(const void *data,
                                                      hal::hal_size_t size) {
      refsi_locker locker(hal_lock);
      BufferDevice elf_data(data, size);
      std::unique_ptr<ELFProgram> new_program(new ELFProgram());
      if (!new_program->read(elf_data)) {
       return hal::hal_invalid_program;
      }
      auto *refsi_program = new refsi_hal_program(std::move(new_program));
      return (hal::hal_program_t)refsi_program;
    }

    bool refsi_hal_device::program_free(hal::hal_program_t program) {
      refsi_locker locker(hal_lock);
      if (program == hal::hal_invalid_program) {
        return false;
      }
      auto *refsi_program = (refsi_hal_program *)program;
      delete refsi_program;
      return true;
    }

The third and last program-related HAL operation is ``program_find_kernel``.
Given a program handle and the name of a kernel entry point, it returns a kernel
handle if the entry point was found or an invalid value otherwise. We will first
create a new class to use as a kernel handle:

.. code:: c++

    // refsi_hal.h
    #include <string> // Added
    ...
    struct refsi_hal_kernel {
      refsi_hal_kernel(hal::hal_addr_t symbol, std::string name)
          : symbol(symbol), name(std::move(name)) {}

      const hal::hal_addr_t symbol;
      const std::string name;
    };


We will also update our ``refsi_hal_program`` to manage the memory for found
kernel entry points:

.. code:: c++

    // refsi_hal.h
    struct refsi_hal_program {
      refsi_hal_program(std::unique_ptr<ELFProgram> program)
          : elf(std::move(program)) {}

      std::unique_ptr<ELFProgram> elf;
      std::map<std::string, std::unique_ptr<refsi_hal_kernel>> kernels; // Added
    };

The ``program_find_kernel`` function can then be filled in. The ``find_symbol``
function of the ELF program object is used to locate the address of the kernel
entry point function:

.. code:: c++

    // refsi_hal.cpp
    hal::hal_kernel_t refsi_hal_device::program_find_kernel(
        hal::hal_program_t program, const char *name) {
      refsi_locker locker(hal_lock);
      if (program == hal::hal_invalid_program) {
        return hal::hal_invalid_kernel;
      }
      refsi_hal_program *refsi_program = (refsi_hal_program *)program;
      refsi_hal_kernel *refsi_kernel = nullptr;
      if (auto it = refsi_program->kernels.find(name);
          it != refsi_program->kernels.end()) {
        refsi_kernel = it->second.get();
      } else {
        hal::hal_addr_t kernel = refsi_program->elf->find_symbol(name);
        if (kernel == hal::hal_nullptr) {
          return hal::hal_invalid_kernel;
        }
        refsi_program->kernels[name] =
            std::make_unique<refsi_hal_kernel>(kernel, name);
        refsi_kernel = refsi_program->kernels[name].get();
      }
      return reinterpret_cast<hal::hal_kernel_t>(refsi_kernel);
    }

At this point, running clik examples results in a new error:

.. code:: console

    $ bin/hello
    Using device 'RefSi M1 Tutorial'
    Running hello example (Global size: 8, local size: 1)
    Could not execute the kernel.

    $ bin/vector_add
    Using device 'RefSi M1 Tutorial'
    Could not execute the kernel.
