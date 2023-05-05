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
import webbrowser
from check import *
from output import *
from junit import *
import variables as v


def lcovGetFlags(args):
    """ Set lcov flags values depending on option flags passed to this
        coverage script.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
    """

    v.LCOV_FLAGS.extend(['--rc', 'genhtml_hi_limit=' + str(args.percentage)])

    fun_enabled = ('0' if args.no_functions else '1')
    bran_enabled = ('0' if args.no_branches else '1')

    v.LCOV_FLAGS.extend(['--rc', 'lcov_function_coverage=' + fun_enabled])
    v.LCOV_FLAGS.extend(['--rc', 'lcov_branch_coverage=' + bran_enabled])

    for regexp in args.cover_filter:
        v.LCOV_FLAGS.extend(['--rc', 'lcov_excl_line=' + regexp])


def lcovGenerateInfoFile(args, dir_path, source_path, folder):
    """ Generate lcov info file for a module.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            dir_path (str): path to objects folder of the current module.
            source_path (str): path to sources folder of the current module.
            folder (str): name of the current module.
    """

    if checkIfModuleExcluded(args, source_path):
        return None

    lcov_file_name = os.path.abspath(source_path).replace('/', '-')
    lcov_file = os.path.join(v.OUTPUT_DIR,
                             'coverage' + lcov_file_name + '.info')

    lcov_command = [
        'lcov', '-t', 'Coverage', '-o', lcov_file, '-c', '-d', dir_path, '-b',
        source_path, '--no-external'
    ]
    lcov_command.extend(v.LCOV_FLAGS)

    if not args.recursive:
        lcov_command.append('--no-recursion')

    callProcess(lcov_command)

    return lcov_file


def lcovMergeModulesInfoFiles(args):
    """ Merge lcov info files of every modules then generate html output.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
    """

    if len(v.COVERAGE_MERGE) == 0:
        return None

    lcov_file = os.path.join(v.OUTPUT_DIR, v.COVERAGE_INFO)
    lcov_merge = 'lcov'

    for cover_file in v.COVERAGE_MERGE:
        lcov_merge += ' -a ' + cover_file

    lcov_merge += ' -o ' + lcov_file + ' ' + ' '.join(v.LCOV_FLAGS)

    callProcess(lcov_merge.split())

    if not args.no_intermediate_files and args.lcov_html_output:
        callProcess([
            'genhtml', '-o', os.path.join(v.OUTPUT_DIR, v.COVERAGE_HTML),
            os.path.join(v.OUTPUT_DIR, v.COVERAGE_INFO), '--demangle-cpp'
        ] + v.LCOV_FLAGS)

    return lcov_file


def lcovMergeTestSuitesInfoFiles(args):
    """ Merge lcov info files of every test suites then generate html output
        and display it in default browser.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
    """

    if len(v.TEST_SUITES_MERGE) > 0:
        lcov_merge = ['lcov']
        lcov_info = os.path.join(v.OUTPUT_DIR, v.TOTAL_COVERAGE_INFO)
        lcov_html = os.path.join(v.OUTPUT_DIR, v.TOTAL_COVERAGE_HTML)

        for test_suite_info in v.TEST_SUITES_MERGE:
            lcov_merge.extend(['-a', test_suite_info])

        lcov_merge.extend(['-o', lcov_info])
        lcov_merge.extend(v.LCOV_FLAGS)

        callProcess(lcov_merge)

        if args.lcov_html_output:
            callProcess([
                'genhtml', '-o', lcov_html, lcov_info, '--demangle-cpp'
            ] + v.LCOV_FLAGS)

        return lcov_info

    return None


def lcovParseLH(args, line, results):
    """ Get the number of covered lines and set results' attribute.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            line (str): .info line containing number of covered lines.
            results (CoverageResults): coverage results of the current module.
    """

    results.nbCoveredLines += int(line.split(":")[1])


def lcovParseLF(args, line, results):
    """ Get the number of executable lines and set results' attribute.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            line (str): .info line containing number of executable lines.
            results (CoverageResults): coverage results of the current module.
    """

    results.nbTestedLines += int(line.split(":")[1])


def lcovParseBRH(args, line, results):
    """ Get the number of covered branches if branches' coverage is enabled and
        set results' attribute.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            line (str): .info line containing number of covered branches.
            results (CoverageResults): coverage results of the current module.
    """

    if not args.no_branches:
        results.nbCoveredBranches += int(line.split(":")[1])


def lcovParseBRF(args, line, results):
    """ Get the number of branches if branches' coverage is enabled and set
        results' attribute.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            line (str): .info line containing number of branches.
            results (CoverageResults): coverage results of the current module.
    """

    if not args.no_branches:
        results.nbTestedBranches += int(line.split(":")[1])


def lcovParseFNH(args, line, results):
    """ Get the number of covered functions if functions' coverage is enable
        and set results' attribute.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            line (str): .info line containing number of covered functions.
            results (CoverageResults): coverage results of the current module.
    """

    if not args.no_functions:
        results.nbCoveredFunctions += int(line.split(":")[1])


def lcovParseFNF(args, line, results):
    """ Get the number of functions if functions' coverage is enabled and set
        results' attribute.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            line (str): .info line containing number of functions.
            results (CoverageResults): coverage results of the current module.
    """

    if not args.no_functions:
        results.nbTestedFunctions += int(line.split(":")[1])


def lcovParseDA(args, line, results, cpp_file, stdout):
    """ Get information about a line and fill results in consequences. Edit the
        xml output file if the option is enabled.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            line (str): .info line containing information about a line.
            results (CoverageResults): coverage results of the current module.
            cpp_file (str): path of the current checked source file.
            stdout (bool): Enable or disable display on stdout
    """

    infos = line.split(":")[1]
    nb_line = infos.split(",")[0]
    nb_exec = infos.split(",")[1]
    line = cpp_file + ":" + nb_line

    if nb_exec == '0':
        if line not in results.linesNotCalled:
            results.linesNotCalled.append(line)

        if args.junit_xml_output:
            jUnitWriteTestCase(v.TEST_SUITE_NAME + '.Line', 'Line',
                               'Line not covered', line)

        if args.verbose and not args.quiet and stdout:
            pOut(v.FAILED_TAG + 'Line not covered')
            pOut(v.FINFOS_TAG + '      At: ' + line)

    else:
        if args.junit_xml_output:
            jUnitWriteTestCase(v.TEST_SUITE_NAME + '.Line', 'Line',
                               'Line covered', '')

        if args.verbose and not args.quiet and stdout:
            pOut(v.PASSED_TAG + 'Line covered')
            pOut(v.INFOS_TAG + '      At: ' + line)
            pOut(v.INFOS_TAG + 'Executed: ' + nb_exec + ' times')


def lcovParseBRDA(args, line, results, cpp_file, stdout):
    """ Get information about a branch and fill results in consequences. Edit
        the xml output file if the option is enable. Ignored if --no-branches
        is enabled.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            line (str): .info line containing information about a branch.
            results (CoverageResults): coverage results of the current module.
            cppFile (str): path of the current checked source file.
            stdout (bool): Enable or disable display on stdout
    """

    if args.no_branches:
        return

    infos = line.split(':')[1]
    nb_line = infos.split(',')[0]
    taken = infos.split(',')[3]
    branch = cpp_file + ':' + nb_line

    if taken == '0' or taken == '-':
        if branch not in results.branchesNotCalled:
            results.branchesNotCalled.append(branch)

        if args.junit_xml_output:
            jUnitWriteTestCase(v.TEST_SUITE_NAME + '.Branch', 'Branch',
                               'Branch not covered', branch)

        if args.verbose and not args.quiet and stdout:
            pOut(v.FAILED_TAG + 'Branch not covered')
            pOut(v.FINFOS_TAG + '      At: ' + branch)

    else:
        if args.junit_xml_output:
            jUnitWriteTestCase(v.TEST_SUITE_NAME + '.Branch', 'Branch',
                               'Branch covered', '')

        if args.verbose and not args.quiet and stdout:
            pOut(v.PASSED_TAG + 'Branch covered')
            pOut(v.INFOS_TAG + '      At: ' + branch)
            pOut(v.INFOS_TAG + 'Executed: ' + taken + ' times')


def lcovParseFNDA(args, line, results, fn, cpp_file, stdout):
    """ Get information about a branch and fill results in consequences. Edit
        the xml output file if the option is enable. Ignored if --no-functions
        is enabled.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            line (str): .info line containing information about a function.
            results (CoverageResults): coverage results of the current module.
            cppFile (str): path of the current checked source file.
            stdout (bool): Enable or disable display on stdout
    """

    if args.no_functions:
        return

    infos = line.split(":")[1]
    nb_exec = infos.split(",")[0]
    fun_name = infos.split(",")[1]

    if fn.get(fun_name) is not None:
        nb_line = fn[fun_name]
    else:
        nb_line = 'unknown'

    function = (callProcess(['c++filt', infos.split(',')[1]]).strip() + ' - ' +
                cpp_file + ':' + nb_line)

    if nb_exec == '0':
        if function not in results.functionsNotCalled:
            results.functionsNotCalled.append(function)

        if args.junit_xml_output:
            jUnitWriteTestCase(v.TEST_SUITE_NAME + '.Function', 'Function',
                               'Function not covered', function)

        if args.verbose and not args.quiet and stdout:
            pOut(v.FAILED_TAG + 'Function not covered')
            pOut(v.FINFOS_TAG + '    Name: ' + fun_name)
            pOut(v.FINFOS_TAG + '      At: ' + cpp_file + ':' + nb_line)

    else:
        if function in results.functionsNotCalled:
            results.functionsNotCalled.remove(function)

        if args.junit_xml_output:
            jUnitWriteTestCase(v.TEST_SUITE_NAME + '.Function', 'Function',
                               'Function covered', '')

        if args.verbose and not args.quiet and stdout:
            pOut(v.PASSED_TAG + 'Function covered')
            pOut(v.INFOS_TAG + '    Name: ' + fun_name)
            pOut(v.INFOS_TAG + '      At: ' + cpp_file + ':' + nb_line)
            pOut(v.INFOS_TAG + 'Executed: ' + nb_exec + ' times')


def lcovParseFN(args, line, fn):
    """ Get a function name and its location in line, then store those
        information in fn.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            line (str): .info line containing information about a function.
            fn (dict): map functions' location with their names.
    """

    infos = line.split(':')[1]
    nb_line = infos.split(',')[0]
    fun_name = infos.split(',')[1]
    fn[fun_name] = nb_line


def lcovParseInfoFile(args, lcov_path, results, stdout):
    """ Parse a .info file generated with lcov. Fill results with the coverage
        datas in consequences.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            lcov_path (str): .info file path.
            results (CoverageResults): coverage results of the current module.
            stdout (bool): Enable or disable display on stdout
    """

    lcov_file = open(lcov_path, 'r')
    fn = dict([])

    for line in lcov_file:
        line = line.strip()

        if line.startswith('SF:'):
            cpp_file = line.split(':')[1]
        elif line.startswith('FN:'):
            lcovParseFN(args, line, fn)
        elif line.startswith('FNDA:'):
            lcovParseFNDA(args, line, results, fn, cpp_file, stdout)
        elif line.startswith('FNF:'):
            lcovParseFNF(args, line, results)
        elif line.startswith('FNH:'):
            lcovParseFNH(args, line, results)
        elif line.startswith('BRDA:'):
            lcovParseBRDA(args, line, results, cpp_file, stdout)
        elif line.startswith('BRF:'):
            lcovParseBRF(args, line, results)
        elif line.startswith('BRH:'):
            lcovParseBRH(args, line, results)
        elif line.startswith('DA:'):
            lcovParseDA(args, line, results, cpp_file, stdout)
        elif line.startswith('LH:'):
            lcovParseLH(args, line, results)
        elif line.startswith('LF:'):
            lcovParseLF(args, line, results)
        else:
            fn = dict([])

    lcov_file.close()
    results.computeResults()
