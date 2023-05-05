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

# Imports.
import os
from junit_xml import TestSuite, TestCase
import variables as v


def jUnitWriteTestCase(classname, name, message, failure):
    """ Write a test case in JUnit xml file.

        Arguments:
            classname (str): class of the test (i.e. a not called function, a
                             not executed branch or a not tested branch case).
            name (str): name of the failed test.
            message (str): error message to display.
            failure (str): reason of failure.
    """

    test_case = TestCase(name, classname)
    if failure != '':
        test_case.add_failure_info(message, failure)
    v.JUNIT_TEST_CASES.append(test_case)


def jUnitEditXmlFile(args):
    """ Edit JUnit xml file then flush and close it.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
    """

    test_suite = TestSuite(v.TEST_SUITE_NAME, v.JUNIT_TEST_CASES)
    v.TOTAL_JUNIT_TEST_SUITES.append(test_suite)

    if not args.no_intermediate_files:
        full_path = os.path.join(v.OUTPUT_DIR, v.JUNIT_FILE_NAME)
        with open(full_path, 'w') as junit_file:
            TestSuite.to_file(junit_file, [test_suite], prettyprint=True)


def jUnitMergeTestSuites(junit_xml_output):
    """ Edit JUnit xml file which summarizes every test suites.

        Arguments:
            junit_xml_output (str): path the the output file.
    """

    with open(os.path.abspath(junit_xml_output), 'w') as junit_file:
        TestSuite.to_file(
            junit_file, v.TOTAL_JUNIT_TEST_SUITES, prettyprint=True)
