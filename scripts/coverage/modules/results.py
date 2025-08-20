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
from variables import *
from output import *
from percentage import *


class CoverageResults:
    def __init__(self):
        """ Initialize the coverage results, setting all the variables with 0
            and making the lists empty.
        """

        self.nbFailedTests = 0
        self.nbPassedTests = 0
        self.nbTests = 0
        self.nbTestedBranches = 0
        self.nbTestedFunctions = 0
        self.nbCoveredBranches = 0
        self.nbCoveredFunctions = 0
        self.nbTestedLines = 0
        self.nbCoveredLines = 0

        self.functionsNotCalled = []
        self.branchesNotCalled = []
        self.linesNotCalled = []

    def computeResults(self):
        """ Some of the member variables are not incremented through the
            execution of the script. This function set them depending on the
            current results.
        """

        self.nbTests = (self.nbTestedBranches + self.nbTestedFunctions +
                        self.nbTestedLines)
        self.nbPassedTests = (self.nbCoveredBranches + self.nbCoveredFunctions
                              + self.nbCoveredLines)
        self.nbFailedTests = self.nbTests - self.nbPassedTests

    def displayFunctions(self, args):
        """ Display the number and the percentage of covered functions.

            Arguments:
                args (namespace): returned by the arguments parser and contains
                                  values of coverage script's flags.
        """

        if not args.no_functions and self.nbTestedFunctions != 0:
            percent = percentage(self.nbCoveredFunctions,
                                 self.nbTestedFunctions)
            tag = getTagFromPercentage(args, percent)

            pOut(tag + '-> ' + str(self.nbCoveredFunctions) + '/' +
                 str(self.nbTestedFunctions) + ' (' + percent + '%) ' +
                 'functions are covered')

    def displayBranches(self, args):
        """ Display the number and the percentage of covered branches.

            Arguments:
                args (namespace): returned by the arguments parser and contains
                                  values of coverage script's flags.
        """

        if not args.no_branches and self.nbTestedBranches != 0:
            percent = percentage(self.nbCoveredBranches, self.nbTestedBranches)
            tag = getTagFromPercentage(args, percent)

            pOut(tag + '-> ' + str(self.nbCoveredBranches) + '/' +
                 str(self.nbTestedBranches) + ' (' + percent + '%) ' +
                 'branches are covered')

    def displayLines(self, args):
        """ Display the number and the percentage of covered lines.

            Arguments:
                args (namespace): returned by the arguments parser and contains
                                  values of coverage script's flags.
        """

        if self.nbTestedLines != 0:
            percent = percentage(self.nbCoveredLines, self.nbTestedLines)
            tag = getTagFromPercentage(args, percent)

            pOut(tag + '-> ' + str(self.nbCoveredLines) + '/' +
                 str(self.nbTestedLines) + ' (' + percent + '%) ' +
                 'lines are covered')


def displayCoverageResults(args, folder, results):
    """ Display on stdout the results' summary of a module or of all tests.

    Arguments:
        args (namespace): returned by the arguments parser and contains values
            of coverage script's flags.
        folder (str list): sources and objects of the module
            (or "all the modules").
        results (CoverageResults): coverage results of the module (or of all
            modules).
    """

    pOut(EQUALS_TAG + 'Coverage of ' + folder[0] + ' finished')
    if results.nbTests == 0:
        pOut(DASH_TAG + 'No test has been performed')
        return

    nbTests = str(results.nbTests)
    nbPassedTests = str(results.nbPassedTests)
    nbFailedTests = str(results.nbFailedTests)
    nbFunctions = str(results.nbTestedFunctions)
    percent = percentage(results.nbPassedTests, results.nbTests)

    pOut(DASH_TAG + nbTests + ' tests performed')
    pOut(PASSED_TAG + nbPassedTests + ' tests (' + percent + '%)')

    if results.nbFailedTests > 0:
        pOut(INFOS_TAG + nbFailedTests + ' tests. Details below:')
        results.displayFunctions(args)
        results.displayBranches(args)
        results.displayLines(args)
