#!/usr/bin/env python
# pylint: disable=missing-docstring

# Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
