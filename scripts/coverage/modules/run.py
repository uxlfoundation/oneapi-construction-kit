#!/usr/bin/env python

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

from __future__ import print_function

import glob
import os
from threading import Lock

from modules.check import checkIfTestSuiteExists
from modules.csvoutput import (csvWriteFile, csvWriteModuleResults,
                               csvWriteModulesHeaders, csvWriteProperties,
                               csvWriteTestSuiteFile, csvWriteTestSuiteHeaders)
from modules.gcovr import gcovrGenerateXmlFile
from modules.junit import jUnitEditXmlFile, jUnitMergeTestSuites
from modules.lcov import (lcovGenerateInfoFile, lcovMergeModulesInfoFiles,
                          lcovMergeTestSuitesInfoFiles, lcovParseInfoFile)
from modules.output import callProcess
from modules.regression import checkRegression
from modules.results import CoverageResults, displayCoverageResults
from modules.threads import ThreadPool
from modules.variables import (COVERAGE_MERGE, EQUALS_TAG, INFOS_TAG,
                               NB_THREADS, SHARPS_TAG, TEST_SUITE_NAME,
                               TEST_SUITE_NAME_COUNTER, TEST_SUITES_MERGE)


def outputCoverageFolderResults(args, folder, results):
    """ Output on the standart input the coverage result of the current folder.
        Of course, this action is performed only if option flags allow it.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            folder (str list): source and object path of the module.
            results (CoverageResults): the coverage results of the current
                                       module.
    """

    dir_path = folder[1]
    source_path = folder[0]

    cpp_files = [
        f for f in os.listdir(source_path)
        if not os.path.isdir(os.path.join(source_path, f))
    ]
    sub_modules = [
        m for m in os.listdir(source_path)
        if os.path.isdir(os.path.join(source_path, m))
    ]

    if not args.quiet and not args.no_module_reporting:
        print()
        print('{}Coverage of {} is starting'.format(EQUALS_TAG, source_path))
        print('{}{} source files'.format(INFOS_TAG, str(len(cpp_files))))

        if args.recursive:
            print('{}{} sub-modules'.format(INFOS_TAG, str(len(sub_modules))))

        print('{}Sources: {}'.format(INFOS_TAG, os.path.abspath(source_path)))
        print('{}Objects: {}'.format(INFOS_TAG, os.path.abspath(dir_path)))

    if not args.quiet and not args.no_module_reporting:
        displayCoverageResults(args, folder, results)

    if args.csv_output != '':
        csvWriteModuleResults(results, source_path)


def runCoverageFolder(args, folder, all_results):
    """ Run coverage script on a module then display results on stdout.
        Generate lcov html files if --lcov has been called.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            folder (str list): source and object path of the module.
            all_results (dict{str, CoverageResults}): results of each module.
    """

    if args.run_lcov:
        results = CoverageResults()
    dir_path = folder[1]
    source_path = folder[0]
    if args.run_lcov:
        lcov_file = lcovGenerateInfoFile(args, dir_path, source_path, folder)

    gcovrGenerateXmlFile(args, dir_path, source_path)

    if args.run_lcov:
        if lcov_file is None:
            all_results[source_path] = None

        lcovParseInfoFile(args, lcov_file, results, True)
        all_results[source_path] = results

        if results.nbTests > 0:
            COVERAGE_MERGE.append(lcov_file)


def runCoverageTestSuite(arg_parser, args, test_suite, flags, env_variables,
                         modules, name=None):
    """ Run coverage tests on a test suite.

        Arguments:
            arg_parser (namespace): arguments parser of this script.
            args (namespace): option flags values.
            testsuite (str): path to the test suite executable.
            modules (str list list): modules to check.
    """

    print('\n{}Test suite: {}'.format(SHARPS_TAG, test_suite))
    print('{}Purge any intermediate coverage files from previous suite runs'.
          format(EQUALS_TAG))
    for m in modules:
        callProcess(['rm', '-f'] + glob.glob(os.path.join(m[1], '*.gcda')))

    checkIfTestSuiteExists(test_suite, flags, name)

    # Copy the current environment as a baseline for executing the test suite
    env = os.environ.copy()
    # Then insert the variables specified for the test suite.
    for key, value in env_variables.items():
        env[key] = value
    print('{}Launching test suite run for{} {}'.format(
        EQUALS_TAG, TEST_SUITE_NAME, str(TEST_SUITE_NAME_COUNTER)))
    # Print the command as it would appear in a Unix shell.
    env_str = ''
    if env_variables:
        env_str = ' '.join(
            ['%s="%s"' % (k, v) for k, v in env_variables.items()]) + ' '
    print('$ %s%s' % (env_str, ' '.join([test_suite] + flags[0].split(';'))))
    # Pass the environment to the subprocess, the current processes environment
    # is untouched.
    callProcess([test_suite] + flags[0].split(";"), env=env)

    if args.csv_output != '':
        csvWriteTestSuiteHeaders(args)

    results = CoverageResults()

    if args.run_lcov:
        if not args.quiet and not args.no_test_suite_reporting:
            print('{}Launching coverage run for{} {}'.format(
                EQUALS_TAG, TEST_SUITE_NAME, str(TEST_SUITE_NAME_COUNTER)))
    else:
        print('{}Launching coverage run for{} {}'.format(
            EQUALS_TAG, TEST_SUITE_NAME, str(TEST_SUITE_NAME_COUNTER)))

    thread_pool = ThreadPool(NB_THREADS)
    lock = Lock()
    all_results = {}
    for m in modules:
        if m not in args.exclude_modules:
            runCoverageFolder(args, m, all_results)
            # thread_pool.addWorker(runCoverageFolder, args, m, all_results)

    if args.run_lcov:
        for m in modules:
            if m not in args.exclude_modules and all_results[m[0]] is not None:
                outputCoverageFolderResults(args, m, all_results[m[0]])

        lcov_file = lcovMergeModulesInfoFiles(args)

        if lcov_file is not None:
            lcovParseInfoFile(args, lcov_file, results, True)

        if results.nbTests > 0:
            TEST_SUITES_MERGE.append(lcov_file)

    if not args.quiet and not args.no_test_suite_reporting:
        print()
        displayCoverageResults(args, ['all modules'], results)

    if args.junit_xml_output != '':
        jUnitEditXmlFile(args)

    if args.csv_output != '':
        csvWriteTestSuiteFile(args)


def runCoverageAllTestSuites(arg_parser, args):
    """ Run coverage tests on every test suites.

        Arguments:
            argParser (namespace): arguments parser of this script.
            args (namespace) option flags values.
    """

    if args.csv_output != '':
        csvWriteProperties(args)
        csvWriteModulesHeaders(args.modules)

    for index, test_cmd in enumerate(args.test_suites):
        test_suite = test_cmd[0]
        flags = test_cmd[1:]
        env_variables = test_cmd[2]
        runCoverageTestSuite(arg_parser, args, test_suite, flags,
                             env_variables[index], args.modules)

    results = CoverageResults()
    lcov_file = lcovMergeTestSuitesInfoFiles(args)

    if lcov_file is not None:
        lcovParseInfoFile(args, lcov_file, results, True)

    if not args.quiet:
        print('\n{}Coverage of all test suites summary\n'.format(SHARPS_TAG))
        displayCoverageResults(args, ['all test suites'], results)

    if args.csv_output != '':
        csvWriteFile(results, args.csv_output)

    if args.junit_xml_output:
        jUnitMergeTestSuites(args.junit_xml_output)

    if args.check_regression:
        checkRegression(args, results)
