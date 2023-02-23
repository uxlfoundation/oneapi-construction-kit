#!/usr/bin/env python

# Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
