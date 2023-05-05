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

import copy
import datetime
import xml.etree.cElementTree as ET
import re


class TestExecutable(object):
    """ Represents an executable that can be used to run one or more tests. """
    def __init__(self, name, path):
        self.name = name
        self.path = path
        self.working_dir = "."


class TestInfo(object):
    """ Represents a test that can be run by passing certain arguments to a
    test executable. """
    def __init__(self, name, executable, arguments):
        self.name = name
        self.executable = executable
        self.arguments = arguments

    def match(self, pattern):
        """ Determine whether the test name matches the pattern or not. """
        if not pattern:
            return True
        return self.name.find(pattern) >= 0

    def clone(self, append_name):
        """Clone this TestInfo.

        Append an extra string to the name so that it has a unique lookup. Deep
        copy the non-integral members because parts of the code will Mutate
        TestInfos and shallow copies with result in the same mutation being
        applied multiple times.

        :param append_name: Name to append to the cloned test.
        """
        new_test = TestInfo("%s#%s" % (self.name, append_name),
                            copy.deepcopy(self.executable),
                            copy.deepcopy(self.arguments))
        return new_test


class TestList(object):
    """ Holds a list of tests. """
    def __init__(self, tests=None, executables=None):
        self.tests = tests if tests else []
        self.executables = executables if executables else {}

    def __iter__(self):
        return iter(self.tests)

    def __len__(self):
        return len(self.tests)

    def __mul__(self, x):
        """ Return a new TestList that is 'x' times longer than the current
        list, with cloned tests. """
        new_tests = []
        for i in range(x):
            new_tests += [test.clone(str(i)) for test in self.tests]
        return TestList(new_tests, self.executables)

    def filter(self, patterns):
        """ Return a list of tests matching the given patterns. """
        if not patterns:
            return self
        matched_all = True
        filtered = []
        for test in self.tests:
            matched = False
            for pattern in patterns:
                if test.match(pattern):
                    matched = True
                    break
            if not matched:
                matched_all = False
                continue
            filtered.append(test)
        if matched_all:
            return self
        return TestList(filtered, self.executables)

    def sort(self):
        """ Sort the tests by name. """
        self.tests.sort(key=lambda t: t.name)
        return self


class TestRun(object):
    """ Represent the execution of a particular test. """
    def __init__(self, test):
        self.test = test
        self.num = None
        self.process = None
        self.passed_tests = None
        self.total_tests = None
        self.return_code = None
        self.status = None
        self.message = None
        self.exc = None
        self.output = None


class TestResults(object):
    """ Holds the results for a set of tests. """
    def __init__(self, tests):
        self.tests = tests
        self.runs = {}
        self.num_tests = len(tests)
        self.num_passes = 0
        self.num_fails = 0
        self.num_timeouts = 0
        self.start_time = None
        self.end_time = None
        self.fail_list = []
        self.timeout_list = []

    @property
    def finished(self):
        """ Determine whether all the tests have been executed or not. """
        return len(self.runs) == self.num_tests

    @property
    def duration(self):
        """ Return how long it took to run all tests. """
        if (self.start_time is None) or (self.end_time is None):
            return None
        return self.end_time - self.start_time

    def add_run(self, run):
        """ Add a test run to the list of results. """
        self.runs[run.test.name] = run
        run.num = len(self.runs)
        self.refresh()

    def refresh(self):
        """ Update the pass/fail figures. """
        self.num_passes = 0
        self.num_fails = 0
        self.num_timeouts = 0
        for run in self.runs.values():
            if run.status == "PASS":
                self.num_passes += 1
            elif run.status == "TIMEOUT":
                self.num_timeouts += 1
            else:
                self.num_fails += 1

    def start(self):
        """ This function is called before the first test is executed. """
        self.start_time = datetime.datetime.now()

    def stop(self):
        """ This function is called after all tests have been executed. """
        self.end_time = datetime.datetime.now()

    def finish(self, profile):
        """ Finalize the test results. """
        # Make sure all tests have results, even if they have not been run.
        not_runs = []
        self.fail_list = []
        self.timeout_list = []
        for test in self.tests:
            try:
                run = self.runs[test.name]
            except KeyError:
                not_runs.append(test)
            else:
                if run.status == "FAIL":
                    self.fail_list.append(run)
                elif run.status == "TIMEOUT":
                    self.timeout_list.append(run)
        for test in not_runs:
            run = profile.create_run(test)
            run.status = "FAIL"
            run.message = "Test not run"
            run.passed_tests = 0
            run.total_tests = 1
            run.schedule.start_time = datetime.datetime.now()
            run.schedule.end_time = run.schedule.start_time
            self.add_run(run)
            self.fail_list.append(run)
        self.fail_list.sort(key=lambda r: r.test.name)
