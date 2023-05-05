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
import variables as v

#from output import *


def checkIfPercentageIsCorrect(args):
    """ Check if the percentage passed as argument to the script is valid (i.e.
        75 <= percentage <= 100). Exit the script if the percentage is invalid.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
    """

    if args.percentage > 100 or args.percentage < 75:
        errorAndExit(
            str(args.percentage) + ': invalid percentage',
            v.ERROR_INCORRECT_PERCENTAGE)


def getTagFromPercentage(args, percentage):
    """ Return the failed or passed tag depending on the percentage value.

        Arguments:
            args (namespace): returned by the arguments parser and contains
                              values of coverage script's flags.
            percentage (int): the percentage to check.
    """

    if float(percentage) >= args.percentage:
        return v.PASSED_TAG

    return v.FAILED_TAG


def percentage(num, den):
    """ Return the percentage num/den * 100.

        Arguments:
            num (int): numerator
            den (int): denominator

        Returns:
            (str): the percentage
    """

    if den == 0:
        return '(N/D)'

    return str(num / float(den) * 100)[:5]
