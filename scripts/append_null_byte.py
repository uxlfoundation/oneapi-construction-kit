#! /usr/bin/env python
# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

"""
Script takes the path to a file as a command line argument and appends a
null byte to the end of the file.

This is needed so that builtin files can be treated as null terminated
C strings in llvm::MemoryBuffer.
"""

import argparse
import os.path
import sys


def main():
    parser = argparse.ArgumentParser(
           description="Append a null byte to end of input file")
    parser.add_argument(
           "input_file", help="Path to file null byte will be appended to")
    args = parser.parse_args()

    if not os.path.isfile(args.input_file):
        print("file '{0}' doesn't exist".format(args.input_file))
        return 1  # return error code

    try:
        fd = open(args.input_file, "ab")
    except Exception as e:
        print("Error opening file: " + e)
        return 2  # return error code

    try:
        fd.write(b"\x00")
    except Exception as e:
        print("Error writing to file: " + e)
        return 3  # return error code

    return 0

if __name__ == "__main__":
    sys.exit(main())
