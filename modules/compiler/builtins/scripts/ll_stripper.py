#! /usr/bin/env python
# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""
Script that takes an input LLVM .ll file, removes unwanted metadata from it,
and outputs an llvm .ll file.

This script is invoked by a cmake custom command, but it can be used as a
command line tool as well.

We need to do this as a python/build step, rather than an LLVM pass because:
- we can't edit LLVM/Clang
- this transformation has to be done at build time
- we'd have to use LLVM opt's plugin architecture to run a pass
- Windows can't load passes this way
- and we'd have to (for cross-compile builds) build the pass for host
"""

from re import compile
from argparse import ArgumentParser


def strip(in_file, out_file, half_support):
    """ This function does the actual stripping, to achieve this it iterates over
        the entire input file line by line, replaces any patterns it finds that
        need replacing in each line and then writes it to the output file.

        Arguments:
            in_file (list): Input .ll file to process, as a list of lines
            out_file (string): Path to output .ll file to write
            half_support (bool): Whether half support is enabled
    """
    # Compile all the regular expressions in advance
    replace_hidden = compile('define hidden')
    replace_df16 = compile('DF16_')
    replace_spir_version = compile(
        '!opencl\\.spir\\.version = !{!([0-9]*), [!0-9, ]*}')
    replace_ocl_version = compile(
        '!opencl\\.ocl\\.version = !{!([0-9]*), [!0-9, ]*}')
    replace_llvm_metadata = compile('!llvm\\.ident = !{!([0-9]*), [!0-9, ]*}')
    find_spirfuncDF16 = compile('spir_func .*DF16_[^(]*')
    find_spirfuncDecl = compile('declare (hidden )?spir_func .*Dh[^(]*')
    find_comdatDF16 = compile('\\$.*DF16_[^ ]* = comdat any')

    with open(out_file, 'w') as f:
        for line in in_file:
            # Make all hidden functions internal
            line = replace_hidden.sub('define internal', line)

            # Clang mangles `half` from the OpenCL-C header and `_Float16` from
            # our C++ implementation differently, leading to a mismatch between
            # function definitions and declarations. Resolve this by changing
            # `Float16` definitions to `half` mangling, and removing the hidden
            # `half` declaration.
            if (half_support):
                # If this line is a definition with halfs replace the mangling
                if (find_spirfuncDF16.search(line)):
                    line = replace_df16.sub('Dh', line)
                # If it is a declaration remove it (write a blank line)
                elif (find_spirfuncDecl.search(line)):
                    line = '\n'

                # Handle comdat references similarly to function definitions
                if (find_comdatDF16.search(line)):
                    line = replace_df16.sub('Dh', line)

            # Replace the opencl.spir version metadata
            line = replace_spir_version.sub('!opencl.spir.version = !{!\\1}',
                                            line)
            # Replace the opencl.ocl version metadata
            line = replace_ocl_version.sub('!opencl.ocl.version = !{!\\1}',
                                           line)
            # Replace the llvm.ident metadata
            line = replace_llvm_metadata.sub('!llvm.ident = !{!\\1}', line)
            # write our processed version of the line to the output file
            f.write(line)


def main():
    """ Main entry point """

    # Setup argparse
    parser = ArgumentParser(
        description=('Strips unwanted metadata from an input '
                     '.ll file and writes the result to an output .ll file'))

    parser.add_argument('in_file', help='Input ll file')

    parser.add_argument('out_file', help='Output ll file')

    parser.add_argument(
        '--half-support',
        choices=['on', 'off'],
        type=str.lower,
        help='Enable half support')

    args = parser.parse_args()

    # Read in the input file
    with open(args.in_file) as in_file:
        data = in_file.readlines()

    # Perform the strip and write the output
    strip(data, args.out_file, args.half_support == 'on')


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        exit(130)
