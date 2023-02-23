#! /usr/bin/env python
"""Run the FuzzCL tool to improve corpus coverage."""

# Copyright (C) Codeplay Software Limited. All Rights Reserved.

from __future__ import print_function

import argparse
import errno
import os
import re
import subprocess
import sys

DEFAULT_BIN_PATH = "../../build/bin"
GENERATED_OUTPUT_FILE_PREFIX = "FuzzCL_generated_"
SUPPORTED_SANITIZER = ["AddressSanitizer", "LeakSanitizer", "ThreadSanitizer"]


def detect_error_message(output):
    """
    detect a error message and return the index of the start of the message
    """
    error_index = 0
    # find the index of the error message if there is one
    sanitizer_index = 0
    found = False
    while not found and sanitizer_index < len(SUPPORTED_SANITIZER):
        # make sure to exclude SUMMARY lines
        error_index = next(
            (i for i in range(0, len(output))
             if SUPPORTED_SANITIZER[sanitizer_index] in output[i]
             and "SUMMARY" not in output[i]), None)
        found = error_index
        if not found:
            sanitizer_index += 1
    # check if an error message was found
    if sanitizer_index == len(SUPPORTED_SANITIZER):
        return (None, None)

    return (SUPPORTED_SANITIZER[sanitizer_index], error_index)


def main():
    """Main entry point to run_fuzzcl.py."""
    # parse command line arguments
    parser = argparse.ArgumentParser(
        description="Python wrapper around FuzzCL that will run a corpus and "
        "generate a UnitCL test if the ThreadSanitizer finds an issue")
    parser.add_argument('corpus', help="A corpus to be run")
    parser.add_argument('-d',
                        '--device',
                        help="An OpenCL device the corpus should be run on")
    parser.add_argument('-o',
                        '--output',
                        help="Path to the generated UnitCL test")
    parser.add_argument(
        '--bin-path',
        help="Path to the directory containing FuzzCL's executable")

    args = parser.parse_args()

    # make sure to adjust the path given to the script so they are absolute
    args.corpus = os.path.join(os.getcwd(), args.corpus)
    if args.output:
        args.output = os.path.join(os.getcwd(), args.output)
    if args.bin_path:
        args.bin_path = os.path.join(os.getcwd(), args.bin_path)

    # change directory to this file's directory so next paths are accurate
    abspath = os.path.abspath(__file__)
    dname = os.path.dirname(abspath)
    os.chdir(dname)
    # move to bin
    os.chdir(args.bin_path if args.bin_path else DEFAULT_BIN_PATH)

    print("Running FuzzCL...")
    command = ["./FuzzCL", "-c", args.corpus]
    if args.device:
        command += ["-d", args.device]
    try:
        subprocess.check_output(command, stderr=subprocess.STDOUT)

        # this will only be run if the previous was successful
        command += ["--enable-callbacks"]
        subprocess.check_output(command, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        output = e.output.decode().split("\n")
        sanitizer, error_index = detect_error_message(output)

        # if no sanitizer error found
        if error_index is None:
            print(e.output.decode())
            return 1

        # get the name of the file throwing an error by walking backwards from
        # where the error was, the regex will match a corpus file name that
        # looks something like this: 017dcc9f3adee131b6e7cf9728619fa0b95ac949
        filename_regex = re.compile(r"^[0-9a-f]{40}$")
        file_index = error_index
        while file_index >= 0 and not filename_regex.match(output[file_index]):
            file_index = file_index - 1

        # if we get back to the start of the output and still haven't found the
        # file error and exit
        if file_index < 0:
            print("Couldn't find file name in output:\n", e.output.decode())
            return 1

        filename = output[file_index]
        print("Found", sanitizer, "issue on file", filename)

        # change the command to run this file
        command[1] = "-f"
        command[2] = os.path.join(args.corpus, filename)

        # add UnitCL generation to the command
        if args.output:
            command += ["-o", args.output]
        else:
            generated_filename = GENERATED_OUTPUT_FILE_PREFIX + filename
            # generate a pseudo-unique name if no output file specified
            command += ["-o", generated_filename]

        # check_output will raise an exception here even if we already know
        p = subprocess.Popen(command)
        p.wait()

        try:
            p = subprocess.Popen(
                ["clang-format", "-style=file", "-i", command[-1]])
            p.wait()
        except OSError as e:
            if e.errno == errno.ENOENT:
                print(
                    "clang-format not present, test file will not be formatted"
                )
            else:
                raise

        if args.output:
            print("UnitCL test saved to", args.output)
        else:
            # read the generated file and output to stdout
            print("UnitCL test :")
            f = open(generated_filename)
            print(f.read())
            f.close()
            # remove the generated file
            os.remove(generated_filename)
        return 0

    print("No issue was found")
    return 0


if __name__ == "__main__":
    sys.exit(main())
