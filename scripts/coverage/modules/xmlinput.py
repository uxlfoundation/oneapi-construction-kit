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
import xml.etree.ElementTree as ET
from check import *
from csvoutput import *
from run import *
from regression import *
import variables as v


def getXmlProperties(root, args):
    """ Get option flags values from the xml input file.

        Arguments:
            root (namespace): tree representation of the xml file.
            args (namespace): option flags values.
    """

    properties = []

    for prop in root.iter('property'):
        if prop.get('value') == 'True':
            value = True
        elif prop.get('value') == 'False':
            value = False
        else:
            if (prop.get('name') == 'exclude-modules' or
                    prop.get('name') == 'cover-filter'):
                value = prop.get('value').split()
            else:
                value = prop.get('value')

        setattr(args, prop.get('name').replace('-', '_'), value)


def getTestSuitesFromXml(root, args):
    """ Get test suites and their flags from the xml input file.

        Arguments:
            root (namespace): tree representation of the xml file.
            args (namespace): option flags values.

        Returns:
            test_suites (str list): paths to the test suites.
            flags (str list): flags of each test suite.
            env_variables (str list): environment variables to set before
                executing the test.
    """

    test_suites = []
    flags = []
    test_suite_flag = []
    env_variables = []
    names = []

    for test_suite in root.iter('testsuite'):
        test_suites.append(test_suite.get('path'))
        flags.append([test_suite.get('flags')])
        names.append(test_suite.get('name'))
        env_dict = {}
        for env in test_suite.get('env_variables').split(';'):
            split_pos = env.find('=')
            key = env[:split_pos]
            if key:
                env_dict[key] = env[split_pos + 1:]
        env_variables.append(env_dict)

    for i, test_suite in enumerate(test_suites):
        test_suite_flag.append([test_suite, " ".join(flags[i])])

    setattr(args, 'test_suites', test_suite_flag)

    return test_suites, flags, env_variables, names


def getModulesFromXml(root, args):
    """ Get modules' sources and objects directories from the xml input file.

        Arguments:
            root (namespace): tree representation of the xml file.
            args (namespace): option flags values.

        Returns:
            modules (str list list): list of pairs [sources, objects] of the
                                     modules to cover.
    """

    modules = []

    for module in root.iter('module'):
        modules.append([module.get('sources'), module.get('objects')])

    setattr(args, 'modules', modules)

    return modules


def runFromXml(arg_parser, args):
    """ Launch coverage tests from an xml input file.

        Arguments:
            arg_parser (namespace): arguments parser of this script.
            args (namespace): option flags values.
    """

    root = ET.parse(args.xml_input).getroot()
    properties = getXmlProperties(root, args)
    test_suites, flags, env_variables, names = getTestSuitesFromXml(root, args)
    modules = getModulesFromXml(root, args)

    checkIfModulesExists(args, modules)

    if args.csv_output != '':
        csvWriteProperties(args)
        csvWriteModulesHeaders(modules)

    for i, test_suite in enumerate(test_suites):
        runCoverageTestSuite(arg_parser, args, test_suite, flags[i],
                             env_variables[i], modules, names[i])

    if args.run_lcov:
        results = CoverageResults()
        lcov_file = lcovMergeTestSuitesInfoFiles(args)

        if lcov_file is not None:
            lcovParseInfoFile(args, lcov_file, results, True)

        if not args.quiet:
            pOut('\n' + v.SHARPS_TAG + 'Coverage of all test suites summary\n')
            displayCoverageResults(args, ['all test suites'], results)

    if args.csv_output != '':
        csvWriteFile(results, args.csv_output)

    if args.junit_xml_output != '':
        jUnitMergeTestSuites(args.junit_xml_output)

    if args.check_regression:
        checkRegression(args, results)
