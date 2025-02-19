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

import datetime
import math
import sys
from enum import Enum
from platform import release, system
from subprocess import check_output


class ColorMode(Enum):
    always = 'always'
    never = 'never'
    auto = 'auto'

    def __str__(self):
        return self.value


class TestUI(object):
    MESSAGE_TYPE_DEFAULT = 0
    MESSAGE_TYPE_BANNER = 1
    MESSAGE_TYPE_STATUS = 2
    MESSAGE_TYPE_OUTPUT = 3
    MESSAGE_TYPE_RESULTS = 4

    def __init__(self, color_mode, out=sys.stdout):
        self.out = out
        self.prev_status_width = 0
        self.last_message_type = self.MESSAGE_TYPE_DEFAULT
        if color_mode == ColorMode.always:
            self.use_color = True
        elif color_mode == ColorMode.never:
            self.use_color = False
        else:
            # Windows 10 suppports unix colour codes, but older versions don't
            self.use_color = out.isatty() and not (system() == "Windows"
                                                   and int(release()) < 10)
        self.fmt = ColorFormat() if self.use_color else NoColorFormat()
        self.term_width = self.get_terminal_width()

    def print_start_banner(self, results, num_workers):
        self.start_message(self.MESSAGE_TYPE_BANNER)
        self.out.write(self.fmt.white("Running %d tests using %d workers.\n" %
                                     (results.num_tests, num_workers)))
        self.out.flush()

    def print_test_status(self, run, results):
        test = run.test
        progress = self.calc_progress(run.num, results.num_tests)
        figures = (self.fmt.white("%3.0f %%" % progress),
                   self.fmt.red(results.num_fails),
                   self.fmt.yellow(results.num_skipped),
                   self.fmt.blue(results.num_timeouts),
                   self.fmt.green(results.num_passes),
                   self.fmt.white(results.num_tests),
                   self.format_status(run),
                   test.name)
        status = "[%s] [%s:%s:%s:%s/%s] %s %s" % figures
        self.start_message(self.MESSAGE_TYPE_STATUS)
        if self.out.isatty():
            if len(status) < self.prev_status_width:
                self.out.write("\r{}".format(" " * self.term_width))
            self.out.write("\r{}".format(status))
            self.prev_status_width = len(status)
        else:
            self.out.write(status + "\n")
        self.out.flush()

    def print_test_output(self, run, results):
        duration = run.schedule.end_time - run.schedule.start_time
        sep = self.fmt.white("*" * 20)
        chunks = [sep, run.test.name, self.format_status(run)]
        if run.message:
            chunks.append("(%s)" % run.message)
        chunks.append("in")
        chunks.append(str(duration))
        chunks.append(sep)
        self.start_message(self.MESSAGE_TYPE_OUTPUT)
        self.out.write("%s\n" % " ".join(chunks))
        if run.output:
            self.out.write(run.output)
        self.out.write("\n%s\n" % (sep * 4))
        self.out.flush()

    def print_results(self, results):
        pass_rate = self.calc_progress(results.num_passes, results.num_tests, 1)
        xfail_exp_fail_rate = self.calc_progress(results.num_xfail_expected_fails, results.num_tests, 1)
        xfail_unexp_pass_rate = self.calc_progress(results.num_xfail_unexpectedly_passed, results.num_tests, 1)
        mayfail_fail_rate = self.calc_progress(results.num_mayfail_fails, results.num_tests, 1)        
        fail_rate = self.calc_progress(results.num_fails, results.num_tests, 1)
        timeout_rate = self.calc_progress(results.num_timeouts, results.num_tests, 1)
        skip_rate = self.calc_progress(results.num_skipped, results.num_tests, 1)
        cts_rate = self.calc_progress(results.num_passes_cts, results.num_total_cts, 1)
        cts_fail_rate = self.calc_progress(results.num_total_cts-results.num_passes_cts,
                                           results.num_total_cts, 1)
        self.start_message(self.MESSAGE_TYPE_RESULTS)
        if results.fail_list:
            self.out.write(self.fmt.red("Failed tests:\n"))
            for run in results.fail_list:
                self.out.write("  %s\n" % run.test.name)
            self.out.write("\n")

        if results.xfail_unexpectedly_passed_list:
            self.out.write(self.fmt.red("Unexpected passing XFail tests:\n"))
            for run in results.xfail_unexpectedly_passed_list:
                self.out.write("  %s\n" % run.test.name)
            self.out.write("\n")

        if results.may_fail_failed_list:
            self.out.write(self.fmt.red("May Fail failing tests:\n"))
            for run in results.may_fail_failed_list:
                self.out.write("  %s\n" % run.test.name)
            self.out.write("\n")
            
        if results.timeout_list:
            self.out.write(self.fmt.blue("Timeout tests:\n"))
            for run in results.timeout_list:
                self.out.write("  %s\n" % run.test.name)
            self.out.write("\n")

        # Round the duration to the nearest second.
        duration = results.duration
        days, seconds, micros = (duration.days, duration.seconds,
                                 duration.microseconds)
        if duration.total_seconds() < 1.0:
            seconds = 1
            micros = 0
        else:
            if micros >= 500 * 1000:
                seconds += 1
            micros = 0
        duration = datetime.timedelta(days, seconds, micros)
        self.out.write(self.fmt.white("Finished in "))
        self.out.write("%s\n" % duration)
        # Print test figures.
        self.out.write(self.fmt.green("\nPassed expectedly:  "))
        self.out.write("%6d (%5.1f %%)\n" % (results.num_passes, pass_rate))
        self.out.write(self.fmt.red("Failed unexpectedly:"))
        self.out.write("%6d (%5.1f %%)\n" % (results.num_fails, fail_rate))
        if results.num_xfail_unexpectedly_passed > 0:
            self.out.write(self.fmt.red("Passing unexpectedly:"))
            self.out.write("%5d (%5.1f %%)\n" % (results.num_xfail_unexpectedly_passed, xfail_unexp_pass_rate))
        if results.num_xfail_expected_fails > 0:
            self.out.write("Failed expectedly:   %5d (%5.1f %%)\n" % (results.num_xfail_expected_fails, xfail_exp_fail_rate))
        if results.num_mayfail_fails > 0:
            self.out.write(self.fmt.red("Failing may fail:     "))            
            self.out.write("%4d (%5.1f %%)\n" % (results.num_mayfail_fails, mayfail_fail_rate))
        self.out.write(self.fmt.blue("Timeouts:           "))
        self.out.write("%6d (%5.1f %%)\n" % (results.num_timeouts, timeout_rate))
        self.out.write(self.fmt.yellow("Skipped:            "))
        self.out.write("%6d (%5.1f %%)\n" % (results.num_skipped, skip_rate))
        self.out.write(self.fmt.white("Overall Pass:       "))
        self.out.write("%6d (%5.1f %%)\n" % (results.num_passes_cts, cts_rate))
        self.out.write(self.fmt.white("Overall Fail:       "))
        self.out.write("%6d (%5.1f %%)\n"
                       % (results.num_total_cts - results.num_passes_cts,
                          cts_fail_rate))
        self.out.flush()

    def print_worker_exception(self, worker, exc):
        self.out.write(self.fmt.red("error: %s\n" % str(exc)))
        self.out.flush()

    def start_message(self, type):
        types_need_line_before = [
            self.MESSAGE_TYPE_RESULTS
        ]
        types_need_line_after = [
            self.MESSAGE_TYPE_BANNER,
            self.MESSAGE_TYPE_OUTPUT
        ]
        if ((self.last_message_type == self.MESSAGE_TYPE_STATUS) and
            (type != self.MESSAGE_TYPE_STATUS) and self.out.isatty()):
            self.out.write("\n")
        if ((type in types_need_line_before) or
            (self.last_message_type in types_need_line_after)):
            self.out.write("\n")
        self.last_message_type = type

    def format_status(self, run):
        if run.status == "PASS":
            return self.fmt.green(run.status)
        elif run.status == "FAIL":
            return self.fmt.red(run.status)
        elif run.status == "TIMEOUT":
            return self.fmt.blue(run.status)
        elif run.status == "SKIP":
            return self.fmt.yellow(run.status)
        else:
            return self.fmt.white(run.status)

    def calc_progress(self, numerator, denominator, frac_digits=0):
        """ Calculate a percentage out of a fraction, returning 100.0 only if
        the fraction is equal to one.
        :param numerator: Numerator of the fraction.
        :param denominator: Denominator of the fraction.
        :param frac_digits: Number of fractional digits to display.
        :return: Float representing the percentage.
        """
        if denominator == 0:
            return float('nan')
        elif numerator == 0:
            return 0.0
        elif numerator == denominator:
            return 100.0
        else:
            result = (float(numerator) / float(denominator)) * 100.0
            threshold = 100.0 - (math.pow(10.0, -frac_digits) * 0.5)
            # Prevent the result being rounded up when going over the threshold.
            if result >= threshold:
                result = (math.floor(result * math.pow(10.0, frac_digits)) *
                          math.pow(10.0, -frac_digits))
            return result

    def get_terminal_width(self):
        """ Get the width of the terminal.
        :return: The width of the terminal or ``None``.
        """
        if self.out.isatty():
            if system() == 'Windows':
                for line in check_output(['mode.com'
                                          ]).decode('utf-8').splitlines():
                    if 'Columns' in line:
                        terminal_width = line[line.find(':') + 1:].strip()
                        return int(terminal_width) - 1
            else:
                return int(
                    check_output(['stty', 'size']).decode('utf-8').split()[1])
        return None


class ColorFormat(object):
    """ Convenience class for coloring console text. """
    def red(self, text):
        return "\x1B[1;31m{}\x1B[0m".format(str(text))

    def green(self, text):
        return "\x1B[1;32m{}\x1B[0m".format(str(text))

    def yellow(self, text):
        return "\x1B[1;33m{}\x1B[0m".format(str(text))

    def blue(self, text):
        return "\x1B[1;34m{}\x1B[0m".format(str(text))

    def magenta(self, text):
        return "\x1B[1;35m{}\x1B[0m".format(str(text))

    def cyan(self, text):
        return "\x1B[1;36m{}\x1B[0m".format(str(text))

    def white(self, text):
        return "\x1B[1;37m{}\x1B[0m".format(str(text))

class NoColorFormat(object):
    """ Convenience class when coloring is disabled. """
    def red(self, text):
        return str(text)

    def green(self, text):
        return str(text)

    def yellow(self, text):
        return str(text)

    def blue(self, text):
        return str(text)

    def magenta(self, text):
        return str(text)

    def cyan(self, text):
        return str(text)

    def white(self, text):
        return str(text)
