# ELF loader

This is a library that can be used by targets that want to use relocatable ELF
files as their binary format. It's split into three parts: the
platform-independent ELF file header and structure parser, which is located in
the header `<loader/elf.h>`; relocation support functions for x86, x86_64,
little-endian Arm and little-endian AArch64 in `<loader/relocations.h>`; and
utilities for using host OS's virtual memory mapping and protection facilities
in `<loader/mapper.h>`.

Even though the ELF specification is mostly platform-independent, the meaning
of many fields and section is reserved for each platform to define, so please
refer to the target operating system's and ABI descriptions for those fields.

## The steps to execute code in an ELF file

1. Load the ELF file into an array aligned to an 8-byte boundary.
2. Create a `loader::ElfFile` instance from that array.
3. Parse and handle any platform-specific fields in the ELF header.
4. Iterate over the sections in the ELF file and allocate memory for them both
   in host's memory and the target device (they may be the same).
5. Copy data from the ELF sections to the host's mapped memory.
6. Construct an `loader::ElfMap` object and add all those mappings there.
7. Handle relocations in the ELF file, the `ElfMap` can be used in the
   relocation support functions, or a custom solution for the target
   architecture can be used.
8. Copy over the memory from host to the target device if they're distinct.
9. Set the right protection on the device memory according to section's flags.
10. The mapped memory is now ready to be executed.
