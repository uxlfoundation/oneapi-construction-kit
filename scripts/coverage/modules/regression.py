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
# import numpy as np
# import matplotlib.pyplot as plt
import datetime
import variables as v
from output import *
from results import *
from lcov import *


def attachLabels(rects, ax):
    """ Attach y values as labels on plot's bars.

        Arguments:
            rects (list): plot's bars.
            ax (namespace): subplot.
    """

    for rect in rects:
        height = rect.get_height()
        ax.text(
            rect.get_x() + rect.get_width() / 2.,
            0.5 * height,
            '%d' % int(height),
            ha='center',
            va='bottom')


# FIXME: names of variables
def outputRegressionComparingResults(args, prev_results, new_results):
    """ Output on stdout (if not in quiet mode) a summary of regression
        comparaison.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            prev_results (CoverageResults):  previous results.
            new_results (CoverageResults):  new results.
    """

    if args.quiet:
        return

    diff_tests = new_results.nbTests - prev_results.nbTests
    diff_passed_tests = new_results.nbPassedTests - prev_results.nbPassedTests
    diff_failed_tests = new_results.nbFailedTests - prev_results.nbFailedTests
    diff_fun = new_results.nbTestedFunctions - prev_results.nbTestedFunctions
    diff_bran = new_results.nbTestedBranches - prev_results.nbTestedBranches
    diff_lines = new_results.nbTestedLines - prev_results.nbTestedLines
    diffCFun = new_results.nbCoveredFunctions - prev_results.nbCoveredFunctions
    diffCBran = new_results.nbCoveredBranches - prev_results.nbCoveredBranches
    diffCLines = new_results.nbCoveredLines - prev_results.nbCoveredLines

    wFun = (' removed' if diffFun < 0 else ' added')
    wBran = (' removed' if diffBran < 0 else ' added')
    wLines = (' removed' if diffLines < 0 else ' added')

    tFun = (v.FAILED_TAG if diffCFun < 0 else v.PASSED_TAG)
    tBran = (v.FAILED_TAG if diffCBran < 0 else v.PASSED_TAG)
    tLines = (v.FAILED_TAG if diffCLines < 0 else v.PASSED_TAG)

    wCFun = (' less ' if diffCFun < 0 else ' more ')
    wCBran = (' less ' if diffCBran < 0 else ' more ')
    wCLines = (' less ' if diffCLines < 0 else ' more ')

    diffFun = abs(diffFun)
    diffBran = abs(diffBran)
    diffLines = abs(diffLines)

    diffCFun = abs(diffCFun)
    diffCBran = abs(diffCBran)
    diffCLines = abs(diffCLines)

    pOut('\n' + v.EQUALS_TAG + 'Coverage regression results')
    pOut(INFOS_TAG + str(diffFun) + ' functions' + wFun)
    pOut(tFun + '-> ' + str(diffCFun) + wCFun + 'covered')
    pOut(INFOS_TAG + str(diffBran) + ' branches' + wBran)
    pOut(tBran + '-> ' + str(diffCBran) + wCBran + 'covered')
    pOut(INFOS_TAG + str(diffLines) + " lines" + wLines)
    pOut(tLines + '-> ' + str(diffCLines) + wCLines + 'covered')


def generatePieCharts(args, ax, results):
    """ Generate a pie charts representing coverage results.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            fig (namespace): figure where the pie charts is displayed.
            results (CoverageResults): coverage information of a module
    """

    pc_fun = percentage(results.nbCoveredFunctions, results.nbTests)
    pc_bran = percentage(results.nbCoveredBranches, results.nbTests)
    pc_lines = percentage(results.nbCoveredLines, results.nbTests)

    pf_fun = percentage(results.nbTestedFunctions - results.nbCoveredFunctions,
                        results.nbTests)
    pf_bran = percentage(results.nbTestedBranches - results.nbCoveredBranches,
                         results.nbTests)
    pf_lines = percentage(results.nbTestedLines - results.nbCoveredLines,
                          results.nbTests)

    colors = ['lightgreen', 'green', 'limegreen', 'tomato', 'red', 'firebrick']
    labels = ('Covered functions', 'Covered Branches', 'Covered Lines',
              'Failed Functions', 'Failed Branches', 'Failed Lines')
    sizes = [pc_fun, pc_bran, pc_lines, pf_fun, pf_bran, pf_lines]
    explode = (0, 0, 0, 0.1, 0.1, 0.1)

    ax.pie(
        sizes,
        explode=explode,
        labels=labels,
        colors=colors,
        autopct='%1.1f%%',
        shadow=True,
        startangle=90)


def checkRegressionComparingResults(args, prev_results, new_results):
    """ Compare previous coverage results with new ones. Generate a bar plot
        summarizing the evolution.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            prev_results (CoverageResults):  previous results.
            new_results (CoverageResults):  new results.
    """

    passed_tests = [newResults.nbPassedTests, prevResults.nbPassedTests]
    failed_tests = [newResults.nbFailedTests, prevResults.nbFailedTests]
    tests = [newResults.nbTests, prevResults.nbTests]

    time = datetime.datetime.strftime(datetime.datetime.now(),
                                      '%d/%m/%Y %H:%M:%S')

    plt.close('all')
    plt.figure(figsize=(18, 12))
    ax = plt.subplot(211)
    ind = np.arange(2)
    width = 0.25
    rects_tests = ax.bar(ind, tests, width, color='blue')
    rects_passed = ax.bar(ind + width, passed_tests, width, color='green')
    rects_failed = ax.bar(ind + 2 * width, failed_tests, width, color='red')

    ax.set_xlim(-width, len(ind) + width)
    ax.set_ylabel('Tests number')
    ax.set_title('Coverage comparaison - ' + time)
    ax.set_xticks(ind + width)
    x_tick_names = ax.set_xticklabels(['Previous Coverage', 'New Coverage'])
    ax.legend((rects_passed[0], rects_failed[0], rects_tests[0]),
              ('Passed tests', 'Failed tests', 'Total tests'))
    ax.grid()

    attachLabels(rects_tests, ax)
    attachLabels(rects_passed, ax)
    attachLabels(rects_failed, ax)

    outputRegressionComparingResults(args, prev_results, new_results)

    ax1 = plt.subplot(223)
    ax2 = plt.subplot(224)
    generatePieCharts(args, ax1, prev_results)
    generatePieCharts(args, ax2, new_results)

    plt.tight_layout()
    plt.savefig(os.path.join(v.OUTPUT_DIR, v.COMPARAISON_COVERAGE_PNG))


def checkRegressionFromInfo(args, info_path, new_results):
    """ Launch parsing of an info file to get the previous coverage results,
        then compare those two sets of coverage datas.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            info_path (str) path of the info file to parse.
            new_results (CoverageResults): new coverage results to compare with
                                           previous ones.
    """

    previous_results = CoverageResults()
    lcovParseInfoFile(args, info_path, previous_results, False)
    checkRegressionComparingResults(args, previous_results, new_results)


def checkRegressionFromDir(args):
    """ Launch parsing on every info file in args.check_regression directory.
        Collect coverage results and generate a graphic summarizing those datas
        depending on timestamps.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
    """

    y_passed = []
    y_failed = []
    x = []

    for f in os.listdir(args.check_regression):
        if f.split('.')[-1] == 'info' and f.split('-')[0] == 'total':
            results = CoverageResults()
            lcovParseInfoFile(args,
                              os.path.join(args.check_regression, f), results,
                              False)
            time = f.split("-")[-1].split(".")[0]

            x.append(time)
            y_passed.append(results.nbPassedTests)
            y_failed.append(results.nbTests)

    y_passed = np.array(y_passed)
    y_failed = np.array(y_pailed)
    x = np.array(x)

    plt.plot(x, y_passed)
    plt.plot(x, y_failed)
    plt.savefig(os.path.join(v.OUTPUT_DIR, v.REGRESSION_COVERAGE_PNG))


def checkRegressionFromFile(args, new_results):
    """ Check if the file passed in argument of --check-regression is a .info
        file. Exit the script if it's not the case.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            new_results (CoverageResults): new coverage results to compare with
                                          previous ones.
    """

    extension = args.check_regression.split('.')[-1]

    if extension == 'info':
        checkRegressionFromInfo(args, args.check_regression, new_results)
    else:
        errorAndExit(args.check_regression + ': not supported format',
                     v.ERROR_INCORRECT_FILE_FORMAT_REGRESSION)


def checkRegression(args, new_results):
    """ Launch regression checking depending on if argument passed to
        --check-regression is a directory or a .info file.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            new_results (CoverageResults): new coverage results to compare with
                                           previous ones.
    """

    if os.path.isdir(args.check_regression):
        checkRegressionFromDir(args)
    else:
        checkRegressionFromFile(args, new_results)
