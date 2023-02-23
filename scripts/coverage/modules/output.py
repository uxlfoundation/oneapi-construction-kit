# Copyright (C) Codeplay Software Limited. All Rights Reserved.

from __future__ import print_function

import subprocess
import sys

from modules.variables import ERROR_INVOKING_PROCESS, ERROR_TAG


def pOut(line):
    """ Print a line on stdout, then print a newline.

        Arguments:
            line (str): string to print on stdout.
    """

    sys.stdout.write(line + '\n')


def pErr(line):
    """ Print a line on stderr, then print a newline.

        Arguments:
            line (str): string to print on sterr.
    """

    sys.stderr.write(line + '\n')


def errorAndExit(error, code):
    """ Print an error on stderr, then exit the script.

        Arguments:
            error (str): error message to display on stderr.
            code (int): error code to return when the script exits.
    """

    pErr('\n' + ERROR_TAG + error)
    sys.exit(code)


def callProcess(command_line, **kwargs):
    """ Call a process and check if it fails. If it does, exit the script with
        the appropriate error code, displaying stdout and stderr output of the
        called process.

        Arguments:
            command_line (str): command line to call.

        Keword Arguments:
            :kwargs: Are forwarded onto ``Popen``.

        Returns:
            out (str): the standard output of the called process.
    """
    process = subprocess.Popen(command_line,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE,
                               **kwargs)
    out, err = process.communicate()

    with open('coverage_out.txt', 'a') as log:
        log.write(out)
    with open('coverage_err.txt', 'a') as log:
        log.write(err)

    if process.returncode != 0:
        print(err, file=sys.stderr)
        print('{}command failed: {}'.format(ERROR_TAG, ' '.join(command_line)),
              file=sys.stderr)
        pErr('Return code: ' + str(process.returncode))
        errorAndExit('script stopped', ERROR_INVOKING_PROCESS)

    return out
