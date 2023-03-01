#!/usr/bin/env python
"""Update RUN commands for all *.spvasm files in this directory.

Finds all ``.spvasm`` files in this scripts directory, parses the OpCapability,
OpExtension, and OpMemoryModel to determine the command line arguments to
invoke ``spirv-ll-tool`. This information is then used to replace lines
starting with:

::
    ; RUN: %spirv-ll

With a fully formed command of the form:

::
    ; RUN: %spirv-ll -a <api> [-b <bits>] [-c <cap>...] [-e <ext>...] %p<name>.spv -o %t

Then overwrites the ``.spvasm`` file with the updated ``RUN:`` command.
"""

from __future__ import print_function

from os import listdir
from os.path import abspath, basename, dirname, join, relpath, splitext

SCRIPT_DIR = abspath(dirname(__file__))
OPENCL_REQUIRED_CAPS = [
    'Addresses',
    'Float16Buffer',
    'Groups',
    'Int64',
    'Int16',
    'Int8',
    'Kernel',
    'Linkage',
    'Vector16',
]
VULKAN_REQUIRED_CAPS = [
    'Matrix',
    'Shader',
    'InputAttachment',
    'Sampled1D',
    'Image1D',
    'SampledBuffer',
    'ImageBuffer',
    'ImageQuery',
    'DerivativeControl',
]


def main():
    """Update the RUN commands for all *.spvasm files in this directory."""
    paths = []
    # Find *.spvasm files and store their absolute paths.
    for path in listdir(SCRIPT_DIR):
        if splitext(path)[1] == '.spvasm':
            paths.append(join(SCRIPT_DIR, path))
    for path in paths:
        # Read the lines of the *.spvasm file.
        with open(path, 'r') as file:
            lines = file.readlines()
        api = None
        capabilities = []
        extensions = []
        address_bits = None
        for line in lines:
            ln = line.strip()
            # Store the OpCapability used, strip those required by OpenCL or
            # Vulkan as they are enabled by default by spirv-ll-tool.
            if ln.startswith('OpCapability'):
                capability = ln[len('OpCapability') + 1:].strip()
                if capability == 'Kernel':
                    api = 'OpenCL'
                elif capability == 'Shader':
                    api = 'Vulkan'
                else:
                    if (api == 'OpenCL'
                            and capability not in OPENCL_REQUIRED_CAPS) or (
                                api == 'Vulkan'
                                and capability not in VULKAN_REQUIRED_CAPS):
                        capabilities.append(capability)
                if not api:
                    # Kernel or Shader has not been found yet but this
                    # capability is still required, so add it to the list.
                    capabilities.append(capability)
            # Store all OpExtension used.
            if ln.startswith('OpExtension'):
                extensions.append(ln[len('OpExtension') + 1:-1].strip())
            # Store the address bits for OpenCL physical addressing models.
            if ln.startswith('OpMemoryModel'):
                _, addressing_model, _ = ln.split()
                address_bits = {
                    'Logical': None,
                    'Physical32': '32',
                    'Physical64': '64',
                }[addressing_model]
        newlines = []
        for line in lines:
            # Rewrite the *.spvasm file with an updated RUN command.
            ln = line.strip()
            if ln.startswith('; RUN: %spirv-ll'):
                run = run_command(api, address_bits, capabilities, extensions,
                                  path)
                newlines.append(run)
                if ln != run.strip():
                    print('updated:', relpath(path, SCRIPT_DIR))
            else:
                newlines.append(line)
        with open(path, 'w') as file:
            file.writelines(newlines)


def run_command(api, address_bits, capabilities, extensions, path):
    """Construct a run command."""
    bits = ' -b %s' % address_bits if address_bits else ''
    caps = ' ' + ' '.join(['-c %s' % cap
                           for cap in capabilities]) if capabilities else ''
    exts = ' ' + ' '.join(['-e %s' % ext
                           for ext in extensions]) if extensions else ''
    spvpath = '%p/{}.spv'.format(splitext(basename(path))[0])
    run = '; RUN: %spirv-ll -a {api}{bits}{caps}{exts} {path} -o %t\n'
    return run.format(api=api, bits=bits, caps=caps, exts=exts, path=spvpath)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        exit(130)
