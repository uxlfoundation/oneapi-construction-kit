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

from __future__ import print_function
import datetime
import time

# Name of this program.
SCRIPT_NAME = 'coverage.py'

# Colors for tags.
FAILED_COLOR = '\033[31m'
PASSED_COLOR = '\033[32m'
DEF_COLOR = '\033[0m'

# Tags for pretty display.
FAILED_TAG = FAILED_COLOR + ' [   FAILED ] ' + DEF_COLOR
PASSED_TAG = PASSED_COLOR + ' [       OK ] ' + DEF_COLOR
DASH_TAG = PASSED_COLOR + ' [----------] ' + DEF_COLOR
INFOS_TAG = PASSED_COLOR + ' [    INFOS ] ' + DEF_COLOR
FINFOS_TAG = FAILED_COLOR + ' [    INFOS ] ' + DEF_COLOR
EQUALS_TAG = PASSED_COLOR + ' [==========] ' + DEF_COLOR
SHARPS_TAG = PASSED_COLOR + ' [##########] ' + DEF_COLOR
ERROR_TAG = SCRIPT_NAME + ': error: '

# Error codes (2 is for argparse errors).
SUCCESS = 0
ERROR_BAD_OS = 1
ERROR_TEST_SUITE_DOESNT_EXIST = 3
ERROR_MODULE_DOESNT_EXIST = 4
ERROR_OUTPUT_DIR_DOESNT_EXIST = 5
ERROR_INVOKING_PROCESS = 6
ERROR_INCORRECT_PERCENTAGE = 7
ERROR_INCORRECT_FILE_FORMAT_REGRESSION = 8
ERROR_INCORRECT_NUMBER_THREADS = 9

# Timestamp.
TIME = datetime.datetime.fromtimestamp(
    time.time()).strftime('%Y-%m-%d-%H-%M-%S')

# Coverage variables.
COVERAGE_INFO = ''
COVERAGE_HTML = ''
TOTAL_COVERAGE_INFO = 'total-coverage-' + TIME + '.info'
TOTAL_COVERAGE_HTML = 'total-coverage-html-' + TIME
COMPARAISON_COVERAGE_PNG = 'comparaison-coverage-' + TIME + '.png'
REGRESSION_COVERAGE_PNG = 'comparaison-coverage-' + TIME + '.png'
COVERAGE_MERGE = []
TEST_SUITES_MERGE = []
LCOV_FLAGS = []

# JUnit output variables.
JUNIT_FILE_NAME = ''
JUNIT_TEST_CASES = []
TOTAL_JUNIT_TEST_SUITES = []

# Test suite name.
TEST_SUITE_NAME_COUNTER = 0
TEST_SUITE_NAME = ''
TEST_SUITE_PATH = ''
TEST_SUITE_FLAGS = ''

# Csv output variables.
CSV_FILE_NAME = ''
CSV_LINES = []
TOTAL_CSV_LINES = []

# Output directory (/tmp by default).
OUTPUT_DIR = "/tmp"

# Number of threads used.
NB_THREADS = 0


def setOutputDir(path):
    """ Set OUTPUT_DIR with path's value.

       Arguments:
            path (str): output directory path.
    """

    global OUTPUT_DIR
    OUTPUT_DIR = path


def setCoverageInfo(name):
    """ Set COVERAGE_INFO with name's value.

        Arguments:
            name (str): coverage info file name.
    """

    global COVERAGE_INFO
    COVERAGE_INFO = name


def setCoverageHtml(name):
    """ Set COVERAGE_HTML with name's value.

        Arguments:
            name (str): coverage html directory name.
    """

    global COVERAGE_HTML
    COVERAGE_HTML = name


def setJunitFileName(name):
    """ Set JUNIT_FILE_NAME with name's value.

        Arguments:
            name (str): JUnit XML file name.
    """

    global JUNIT_FILE_NAME
    JUNIT_FILE_NAME = name


def setCsvFileName(name):
    """ Set CSV_FILE_NAME with name's value.

        Arguments:
            name (str): CSV file name.
    """

    global CSV_FILE_NAME
    CSV_FILE_NAME = name


def setTestSuiteName(name):
    """ Set TEST_SUITE_NAME with name's value.

        Arguments:
            name (str): test suite name.
    """

    global TEST_SUITE_NAME
    global TEST_SUITE_NAME_COUNTER
    global EQUALS_TAG
    TEST_SUITE_NAME_COUNTER += 1
    TEST_SUITE_NAME = name
    print(EQUALS_TAG + 'Set test suite name to: ' + TEST_SUITE_NAME)
    print(EQUALS_TAG + 'Set test suite name counter to: ' +
                        str(TEST_SUITE_NAME_COUNTER))


def setTestSuitePath(path):
    """ Set TEST_SUITE_PATH with path's value.

        Arguments:
            path (str): test suite path.
    """

    global TEST_SUITE_PATH
    TEST_SUITE_PATH = path


def setTestSuiteFlags(flags):
    """ Set TEST_SUITE_FLAGS with flags' value.

        Arguments:
            flags (str): test suite flags.
    """

    global TEST_SUITE_FLAGS
    TEST_SUITE_FLAGS = flags


def setNbThreads(nb_threads):
    """ Set NB_THREADS with nb_threads' value.

        Arguments:
            nb_threads (int): the number of threads to use to parralalize (or
                              not) this script.
    """

    global NB_THREADS
    NB_THREADS = nb_threads


def setCurrentFileNumber(num):
    """ Set CURRENT_FILE_NUMBER with num's value.

        Arguments:
            num (int): num'th file on which code coverage checking has been
                       performed.
    """

    global CURRENT_FILE_NUMBER
    CURRENT_FILE_NUMBER = num
