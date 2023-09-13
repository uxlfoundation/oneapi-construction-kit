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

import os
import io
import re
from city_runner.profiles.default import DefaultProfile
from city_runner.profiles.default import DefaultTestRun
from city_runner.execution import TestRunBase

def create_profile():
    """ Create a new instance of a profile. This is usually called once."""
    return SyclCTSProfile()


class SyclCTSProfile(DefaultProfile):
    """ This profile executes tests locally. """
    def create_run(self, test, worker_state=None):
        """ Create a new test run from a test description.  """
        return SyclCTSTestRun(self, test)


class SyclCTSTestRun(DefaultTestRun):
    """ Represent the execution of a particular test. """
    def __init__(self, profile, test):
        super(SyclCTSTestRun, self).__init__(profile, test)

    def analyze_process_output(self):
        """
        Determine whether a test passed or failed from its output.
        """
        stream = io.BytesIO(self.output)

        # There are 3 main possible outputs from a test:
        #   test cases:  <total> |  <p> passed | <f> failed | <s> skipped | <e> failed as expected
        #   All tests passed : with no information on how many
        #   No tests ran
        #
        # <total>,<p>,<f>,<s>, and <e> are all decimal digits
        #
        # Note that test cases should be seen as a list so it won't say 0 skipped for example
        # This means that there is a number of ways to look for a pass:
        #    If it says "All tests passed" we view it a pass
        #    If "No tests ran" we view it as a skipped
        #    If it is "test cases" then we iterate through the list:
        #       set pass if failed as expected
        #       set fail if there are any fail items
        #       else set pass if a pass item
        #       else set skipped if skipped item.
        #    Iterating through the list can end up with pass, fail, and skipped, but
        #    we prioritise failed for setting status, then pass so this is okay.
        
        # Due to the way it reports (e.g. all tests passed) we don't try to
        # captures numbers within a single test  but just check for some form of
        # all passed or failed and count it as one test
        test_cases_pattern = re.compile(b"^test cases: (.*$)")
        all_test_passed_pattern = re.compile(b"^All tests passed")
        no_test_run_pattern = re.compile(b"^No tests ran$")
 
        passed = False
        failed = False
        skipped = False
 
        # Look for specific patterns in the output text.
        for line in stream:
            test_cases_match = test_cases_pattern.match(line)
            if test_cases_match:
                # We know we will have test cases <total> | then a list separated by '|'
                # So split and only focus on the 2nd item onward
                test_case_output = str(test_cases_match.group(0)).split("|")[1:]
                for t in test_case_output:
                  # Count failed as expected like a pass
                  if "failed as expected" in t:
                      passed = True                    
                  elif "failed" in t:
                      failed = True
                  # If we see a pass then it must be all pass or skipped so pass
                  elif "passed" in t:
                      passed = True
                  # If skipped found then mark as skipped
                  elif "skipped" in t:
                      skipped = True

            # If all tests passed just mark as passed
            elif all_test_passed_pattern.match(line):
                passed = True
            # If No tests ran then we count as a skip
            elif no_test_run_pattern.match(line):
                skipped = True

        # Because we parse different parts we can get pass, fail and skip flags
        # so process in order of fail, pass and then skip flags
        if failed:
            self.status = "FAIL"
        elif passed:
            self.status = "PASS"
        elif skipped:
            self.status = "SKIP"
        else:
            stdout.write(
                "WARNING: Unable to tell if test pass or fail")
            self.status = "FAIL"

        # Special cases e.g SIGTERM
        if self.return_code:
            TestRunBase.analyze_process_output(self)

        # Detect the number of passed tests.
        self.total_tests = 1
        if self.passed_tests is None:
            if self.status in "PASS":
                self.passed_tests = self.total_tests
            else:
                self.passed_tests = 0
