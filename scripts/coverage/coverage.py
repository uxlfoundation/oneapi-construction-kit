#!/usr/bin/env python

# Copyright (C) Codeplay Software Limited. All Rights Reserved.

# Imports.
import sys
from platform import system
from modules import *


def main():
    """ Main function: entry point of the script. First of all, the system is
        checked. If the program is run on Windows, the script raises an
        error and exit with the appropriate error code. Otherwise, option flags
        are parsed and the code coverage checking starts.
    """

    if system() == 'Windows':
        errorAndExit('this script is not supported on Windows', ERROR_BAD_OS)

    arg_parser = setParseArguments()
    args = arg_parser.parse_args(sys.argv[1:])

    if args.run_lcov:
       checkIfPercentageIsCorrect(args)
       checkIfOutputDirExists(args)
    checkIfThreadsNumberIsCorrect(args)
    if args.run_lcov:
       lcovGetFlags(args)

    if args.xml_input:
        runFromXml(arg_parser, args)
    else:
        checkIfModulesExists(args, args.modules)
        runCoverageAllTestSuites(arg_parser, args)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
