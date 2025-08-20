#!/usr/bin/env python

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

# Imports.
import os
import variables as v
from output import *
import multiprocessing


def checkIfTestSuiteExists(test_suite, flags, name=None):
    """ Check if test suite passed as argument to the coverage script exists.
        Set global variables if it exists. Exit the script otherwise.

        Arguments:
            test_suite (str): path to the test suite.
            flags (str list): test suite flags.
    """

    if not os.path.exists(test_suite):
        errorAndExit(test_suite + ': test suite doesn\'t exist',
                     v.ERROR_TEST_SUITE_DOESNT_EXIST)

    if name:
        v.setTestSuiteName(name)
    else:
        v.setTestSuiteName(os.path.split(test_suite)[1])
    v.setTestSuitePath(os.path.abspath(test_suite))
    v.setTestSuiteFlags(' '.join(flags))

    test_suite_name = v.TEST_SUITE_NAME.lower()
    v.setCoverageInfo(test_suite_name + '-coverage-' + v.TIME + '.info')
    v.setCoverageHtml(test_suite_name + '-coverage-html-' + v.TIME)
    v.setJunitFileName(test_suite_name + '-coverage-' + v.TIME + '.xml')
    v.setCsvFileName(test_suite_name + '-coverage-' + v.TIME + '.csv')

    del v.COVERAGE_MERGE[:]
    del v.JUNIT_TEST_CASES[:]


def checkIfModulesExists(args, modules):
    """ Check if modules passed as argument to the coverage script exist. Exit
        the script if one of them doesn't.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            modules (str list list): modules' paths to check.
    """

    for m in modules:
        if not os.path.isdir(m[0]):
            errorAndExit(
                m[0] + ': module source directory ' + 'doesn\'t exist',
                v.ERROR_MODULE_DOESNT_EXIST)

        if not os.path.isdir(m[1]):
            errorAndExit(
                m[1] + ": module object directory " + 'doesn\'t exist',
                v.ERROR_MODULE_DOESNT_EXIST)


def checkIfOutputDirExists(args):
    """ Check if the output directory passed as argument to the coverage script
        exists. Exit the script if it's not the case. Set OUTPUT_DIR otherwise.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
    """

    if args.output_directory is not None:
        if not os.path.isdir(args.output_directory):
            errorAndExit(
                args.output_directory + ': output directory doesn\'t exist',
                v.ERROR_OUTPUT_DIR_DOESNT_EXIST)

        v.setOutputDir(os.path.abspath(args.output_directory))


def checkIfModuleExcluded(args, module):
    """ Check if a module is tagged as excluded. Return true if this is the
        case. Return false otherwise.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            module (str): path to the sources of the module.
    """

    for path in args.exclude_modules:
        if os.path.abspath(module) == os.path.abspath(path):
            return True

    return False


def checkIfThreadsNumberIsCorrect(args):
    """ Check if the number of threads passed as argument to the script is
        correct. If it's not, raise an error. If the option --threads has not
        been called, set the number of threads to use with the number of
        available CPU threads.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
    """

    if args.threads is None:
        v.setNbThreads(multiprocessing.cpu_count())
    elif args.threads <= 0 or args.threads >= multiprocessing.cpu_count():
        errorAndExit(args.threads + ': invalid number of threads',
                     v.ERROR_INCORRECT_NUMBER_THREADS)
    else:
        v.setNbThreads(args.threads)

def checkIfOutputFileCanBeDeleted(output_file):
    """ Check if a gcovr output file can be deleted. Return true if file
        contains no Cobertura "package" info (i.e. no coverage data)
        Return false otherwise.

        Arguments:
            output_file (str): path to the output file.
    """
    if '<packages/>' in open(output_file).read():
        return True

    return False
