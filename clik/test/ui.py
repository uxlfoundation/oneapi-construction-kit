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

import sys
import math
import datetime
from platform import system, release
from subprocess import check_output

class TestUI(object):
    MESSAGE_TYPE_DEFAULT = 0
    MESSAGE_TYPE_BANNER = 1
    MESSAGE_TYPE_STATUS = 2
    MESSAGE_TYPE_OUTPUT = 3
    MESSAGE_TYPE_RESULTS = 4

    def __init__(self, out=sys.stdout):
        self.out = out
        self.prev_status_width = 0
        self.last_message_type = self.MESSAGE_TYPE_DEFAULT
        # Windows 10 suppports unix colour codes, but older versions don't
        self.use_color = out.isatty() and not (system() == "Windows"
                                               and int(release()) < 10)

    def print_start_banner(self, results, num_workers):
        pass

    def print_test_status(self, run, results):
        test = run.test
        fmt = TextFormat(self.use_color)
        progress = self.calc_progress(run.num, results.num_tests)
        figures = (fmt.white("%3.0f %%" % progress),
                   fmt.red(results.num_fails),
                   fmt.blue(results.num_timeouts),
                   fmt.green(results.num_passes),
                   fmt.white(results.num_tests),
                   self.format_status(run),
                   test.name)
        status = "[%s] [%s:%s:%s/%s] %s %s" % figures
        self.start_message(self.MESSAGE_TYPE_STATUS)
        if self.out.isatty():
            if len(status) < self.prev_status_width:
                self.out.write("\r{}".format(" " * self.get_terminal_width()))
            self.out.write("\r{}".format(status))
            self.prev_status_width = len(status)
        else:
            self.out.write(status + "\n")
        self.out.flush()

    def print_test_output(self, run, results):
        fmt = TextFormat(self.use_color)
        duration = run.schedule.end_time - run.schedule.start_time
        sep = fmt.white("*" * 20)
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
        fmt = TextFormat(self.use_color)
        pass_rate = self.calc_progress(results.num_passes, results.num_tests, 1)
        fail_rate = self.calc_progress(results.num_fails, results.num_tests, 1)
        timeout_rate = self.calc_progress(results.num_timeouts, results.num_tests, 1)
        self.start_message(self.MESSAGE_TYPE_RESULTS)
        if results.fail_list:
            self.out.write(fmt.red("Failed tests:\n"))
            for run in results.fail_list:
                self.out.write("  %s\n" % run.test.name)
            self.out.write("\n")

        if results.timeout_list:
            self.out.write(fmt.blue("Timeout tests:\n"))
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
        # Print test figures.
        self.out.write(fmt.green("Passed:       "))
        self.out.write("%6d (%5.1f %%)\n" % (results.num_passes, pass_rate))
        self.out.write(fmt.red("Failed:       "))
        self.out.write("%6d (%5.1f %%)\n" % (results.num_fails, fail_rate))
        self.out.write(fmt.blue("Timeouts:     "))
        self.out.write("%6d (%5.1f %%)\n" % (results.num_timeouts, timeout_rate))
        self.out.flush()

    def print_worker_exception(self, worker, exc):
        fmt = TextFormat(self.use_color)
        self.out.write(fmt.red("error: %s\n" % str(exc)))
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
        fmt = TextFormat(self.use_color)
        if run.status == "PASS":
            return fmt.green(run.status)
        elif run.status == "FAIL":
            return fmt.red(run.status)
        elif run.status == "TIMEOUT":
            return fmt.blue(run.status)
        elif run.status == "SKIP":
            return fmt.yellow(run.status)
        else:
            return fmt.white(run.status)

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


class TextFormat(object):
    """ Convenience class for coloring console text. """
    CSI = "\x1B["

    def __init__(self, allow_color):
        self.allow_color = allow_color

    def color(self, data, *codes):
        if self.allow_color:
            code_text = ";".join(str(code) for code in codes)
            return "".join((self.CSI, code_text + "m", str(data), self.CSI, "0m"))
        else:
            return str(data)

    def red(self, text):
        return self.color(text, 1, 31)

    def green(self, text):
        return self.color(text, 1, 32)

    def yellow(self, text):
        return self.color(text, 1, 33)

    def blue(self, text):
        return self.color(text, 1, 34)

    def magenta(self, text):
        return self.color(text, 1, 35)

    def cyan(self, text):
        return self.color(text, 1, 36)

    def white(self, text):
        return self.color(text, 1, 37)
