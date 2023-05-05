#!/usr/bin/env python
# pylint: disable=missing-docstring

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

from argparse import ArgumentParser
from os.path import basename, join

from modules import gen_glsl_files, gen_spvasm_files, gen_test_files


def main():
    '''Main entry point'''
    parser = ArgumentParser()
    parser.add_argument(
        'type',
        choices=['glsl', 'spvasm', 'test'],
        help='type of output to generate')
    parser.add_argument(
        '-o',
        default='',
        help='directory where .spvasm files will be generated',
        dest='output_dir')
    parser.add_argument(
        'extension',
        help='file containing extended SPIR-V grammar in json form')

    args = parser.parse_args()

    if args.type == 'glsl':
        glsl_files = gen_glsl_files(args.output_dir)
        with open(join(args.output_dir, args.type, 'glsl.cmake'),
                  'w') as glsl_cmake_file:
            glsl_cmake_file.write('set(GLSL_FILES\n  %s)\n' % '\n  '.join(
                [basename(glsl) for glsl in glsl_files]))
    elif args.type == 'spvasm':
        spvasm_files = gen_spvasm_files(args)
        with open(join(args.output_dir, args.type, 'spvasm.cmake'),
                  'w') as spirv_cmake_file:
            spirv_cmake_file.write('set(SPVASM_FILES\n  %s)\n' % '\n  '.join(
                [basename(spvasm) for spvasm in spvasm_files]))
    elif args.type == 'test':
        gen_test_files(args.output_dir)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
