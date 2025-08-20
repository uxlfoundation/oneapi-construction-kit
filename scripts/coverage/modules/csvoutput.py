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
import csv
import variables as v
from percentage import *


def csvWriteProperties(args):
    """ Write option flags' values in the csv file.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
    """

    v.TOTAL_CSV_LINES.append(['Flag names', 'Flag values'])
    for arg in vars(args):
        if arg != 'test_suites' and arg != 'modules':
            v.TOTAL_CSV_LINES.append([arg, str(getattr(args, arg))])

    v.TOTAL_CSV_LINES.append(['.', '.'])
    v.TOTAL_CSV_LINES.append(['Modules sources', 'Modules objects'])

    for m in args.modules:
        v.TOTAL_CSV_LINES.append([m[0], m[1]])

    v.TOTAL_CSV_LINES.append(['.', '.'])
    v.TOTAL_CSV_LINES.append(['Test suites', 'Flags'])

    for test_suite_flags in args.test_suites:
        v.TOTAL_CSV_LINES.append(
            [test_suite_flags[0], ' '.join(test_suite_flags[1:])])

    v.TOTAL_CSV_LINES.append(['.', '.'])


def csvWriteModulesHeaders(modules):
    """ Write titles of modules columns in the csv file.

        Arguments:
            modules (str list list): modules to cover.
    """

    v.TOTAL_CSV_LINES.append(['Modules'])
    for module in modules:
        for i in range(0, 3):
            v.TOTAL_CSV_LINES[-1].append(module[0])

    v.TOTAL_CSV_LINES.append(['Test Suites'])
    for module in modules:
        v.TOTAL_CSV_LINES[-1].extend(['Functions', 'Branches', 'Lines'])


def csvWriteModuleResults(results, module):
    """ Write coverage results of a module in the csv file.

        Arguments:
            results (CoverageResults): coverage results of the current module.
            module (str): path to the sources of the module.
    """

    fun_covered = results.nbCoveredFunctions
    bran_covered = results.nbCoveredBranches

    percent_functions = percentage(fun_covered, results.nbTestedFunctions)
    percent_branches = percentage(bran_covered, results.nbTestedBranches)
    percent_lines = percentage(results.nbCoveredLines, results.nbTestedLines)

    rate_fun = str(fun_covered) + '/' + str(results.nbTestedFunctions)
    rate_bran = str(bran_covered) + '/' + str(results.nbTestedBranches)
    rate_lines = str(results.nbCoveredLines) + '/' + str(results.nbTestedLines)

    v.TOTAL_CSV_LINES[-1].append(rate_fun + ' (' + percent_functions + '%)')
    v.TOTAL_CSV_LINES[-1].append(rate_bran + ' (' + percent_branches + '%)')
    v.TOTAL_CSV_LINES[-1].append(rate_lines + ' (' + percent_lines + '%)')

    if module != '':
        v.CSV_LINES[-1].append(module)
        v.CSV_LINES[-1].append(rate_fun + ' (' + percent_functions + '%)')
        v.CSV_LINES[-1].append(rate_bran + ' (' + percent_branches + '%)')
        v.CSV_LINES[-1].append(rate_lines + ' (' + percent_lines + '%)')
        v.CSV_LINES.append([])


def csvWriteTestSuiteHeaders(args):
    v.CSV_LINES.append(['Flag names', 'Flag values'])
    for arg in vars(args):
        if arg != 'test_suites' and arg != 'modules':
            v.CSV_LINES.append([arg, str(getattr(args, arg))])

    v.CSV_LINES.append(['test_suite', v.TEST_SUITE_PATH])
    v.CSV_LINES.append(['test_suite_flags', v.TEST_SUITE_FLAGS])
    v.CSV_LINES.append(['timestamp', v.TIME])

    v.CSV_LINES.append(['.', '.'])
    v.CSV_LINES.append(['Modules', 'Functions', 'Branches', 'Lines'])
    v.CSV_LINES.append([])

    v.TOTAL_CSV_LINES.append([v.TEST_SUITE_NAME])


def csvWriteTestSuiteFile(args):
    with open(os.path.join(v.OUTPUT_DIR, v.CSV_FILE_NAME), 'w') as csv_file:
        writer = csv.writer(csv_file, delimiter=',')
        for line in v.CSV_LINES:
            writer.writerow(line)

    del v.CSV_LINES[:]


def csvWriteFile(results, csv_file_path):
    """ Edit the csv file, then flush and close it.

        Arguments:
            results (CoverageResults): total coverage results.
            csv_file_path (str): path the the csv output file.
    """

    v.TOTAL_CSV_LINES.append(['.', '.', '.', '.'])
    v.TOTAL_CSV_LINES.append(['.', 'Functions', 'Branches', 'Lines'])

    v.TOTAL_CSV_LINES.append(['Total'])
    csvWriteModuleResults(results, '')

    with open(os.path.abspath(csv_file_path), 'w') as csv_file:
        writer = csv.writer(csv_file, delimiter=',')
        for line in v.TOTAL_CSV_LINES:
            writer.writerow(line)
