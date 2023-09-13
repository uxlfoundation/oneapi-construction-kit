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

import io
import re
from sys import stdout
from city_runner.execution import TestRunBase

class CTSTestRun(TestRunBase):
    """
    Represent the execution of a particular CTS test.
    """
    def __init__(self, profile, test):
        super(CTSTestRun, self).__init__(profile, test)

    def analyze_process_output(self):
        """
        Determine whether a test passed or failed from its output.

        The reason the return code is not simply used to determine pass/fail is
        because you don't always get the return code (e.g. Android). I'd rather
        be safe and only mark a test as 'passed' if the output contains a
        message that says it has passed. Otherwise if your test fails with
        never-seen-before output (like file not found, dll could not be loaded,
        etc) you don't hide failures.

        This is also used to compute a more accurate CTS pass figure, where
        running one test executable runs multiple tests.
        """
        stream = io.BytesIO(self.output)
        # Extended pass_single_pattern. See Redmine issue #6542.
        pass_single_pattern = re.compile(
            b"^(PASSED .+\.|.+PASSED\.?|passed .+\.|.+passed\.?)|All tests passed \(.*\)$")
        pass_pair_pattern = re.compile(b"^PASSED (\d+) of (\d+) tests\.$")
        fail_single_pattern = re.compile(b"^(FAILED .+\.|.+FAILED\.?)$")
        fail_pair_pattern = re.compile(b"^FAILED (\d+) of (\d+) tests\.$")
        skipped_pattern = re.compile(
            b"(.*Skipping test\.+|skipping|SKIPPED.+|No tests ran)$")
        zerorun_pattern = re.compile(b"^Tests completed: 0$")
        doubles_unsupported_pattern = re.compile(b".*Has Double\? NO$")
        extension_unsupported_pattern = re.compile(
            b"\w+ extension version \d\.\d is not supported by the device")

        # Individual SPIR tests use yet another format to report test results.
        # To avoid false positive we want to match the current test name.
        spir_test_name = None
        spir_pass_pattern = None
        if self.test.executable.path == "conformance_test_spir":
            for argument in reversed(self.test.arguments):
                if argument.find(".") >= 0:
                    spir_test_name = argument
                    break
            if spir_test_name:
                spir_pass_expr = "^%s passed\.$" % spir_test_name
                spir_pass_pattern = re.compile(spir_pass_expr.encode("utf8"))

        pass_single = False
        fail_single = False
        fail_pair = None
        pass_pair = None
        skipped = False
        zero_tests_ran = False
        doubles_supported = True

        # Look for specific patterns in the output text.
        for line in stream:
            line = line.rstrip()
            fail_pair_match = fail_pair_pattern.match(line)
            if fail_pair_match:
                fail_pair = (int(fail_pair_match.group(1)),
                             int(fail_pair_match.group(2)))
                break
            elif fail_single_pattern.match(line):
                fail_single = True
                break
            elif doubles_unsupported_pattern.match(line):
                doubles_supported = False
            elif zerorun_pattern.match(line):
                zero_tests_ran = True
            elif extension_unsupported_pattern.match(line):
                skipped = True
            else:
                pass_pair_match = pass_pair_pattern.match(line)
                if pass_pair_match:
                    pass_pair = (int(pass_pair_match.group(1)),
                                 int(pass_pair_match.group(2)))
                    break
                elif skipped_pattern.match(line):
                    skipped = True
                    break
                elif pass_single_pattern.match(line):
                    pass_single = True
                    break
                elif spir_pass_pattern and spir_pass_pattern.match(line):
                    pass_single = True

        # Analyze the results.
        if fail_pair:
            self.status = "FAIL"
            # Frailties in how the CTS is implemented means we can end up with
            # more failed tests than tests run. In the case where we've only
            # run a single test we can reasonably set the number of fails to 1.
            if fail_pair[0] > fail_pair[1]:
                if fail_pair[1] == 1:
                    fail_pair = (1, 1)
                    stdout.write(
                        "WARNING: More than a single fail, defaulting "
                        "number of failing tests to 1.")
                else:
                    stdout.write("WARNING: Number of failed tests is greater "
                                 "than the total tests run! This will cause "
                                 "inaccuracies in calculated pass percentage.")

            self.total_tests = fail_pair[1]
            self.passed_tests = self.total_tests - fail_pair[0]
        elif fail_single:
            self.status = "FAIL"
        elif pass_pair:
            if pass_pair[0] < pass_pair[1]:
                self.status = "FAIL"
            else:
                self.status = "PASS"
            self.passed_tests, self.total_tests = pass_pair
        elif pass_single:
            self.status = "PASS"
        elif skipped:
            self.status = "SKIP"
            self.total_tests = 0  # There were no tests.
        elif zero_tests_ran:
            skip_double = "double" in self.test.name and not doubles_supported
            if skip_double:
                # We expected zero tests to run.
                self.total_tests = 0
                self.status = "SKIP"
            else:
                self.status = "FAIL"
        else:
            self.status = "FAIL"

        # Special cases.
        if self.return_code:
            # Handle failures due to signals.
            super(CTSTestRun, self).analyze_process_output()
        else:
            # Handle tests that do not print 'PASSED'.
            if self.test.executable.name.find("headers") >= 0:
                # The 'headers' executable always returns zero.
                self.status = "PASS"

        # Detect the total number of tests.
        if self.total_tests is None:
            if self.test.executable.name.endswith("half"):
                # 'half' tests either report 'FAILED x of 2 tests' or
                # 'PASSED test' which is inconsistent.
                self.total_tests = 2
            else:
                self.total_tests = 1

        # Detect the number of passed tests.
        if self.passed_tests is None:
            if self.status in "PASS":
                self.passed_tests = self.total_tests
            else:
                self.passed_tests = 0
