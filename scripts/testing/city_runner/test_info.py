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
import csv
import datetime
import xml.etree.cElementTree as ET
import json
import re
import shlex


class TestExecutable(object):
    """ Represents an executable that can be used to run one or more tests. """
    def __init__(self, name, path):
        self.name = name
        self.path = path
        self.working_dir = "."


class Pool(object):
    """
    Represents a test that can be run by passing certain arguments to a test
    executable. If we drop Python 2.7 support this class should inherit from
    the Enum Python 3 language feature.
    """
    NORMAL = 1
    THREAD = 2
    MEMORY = 3


class TestInfo(object):
    """ Represents a test that can be run by passing certain arguments to a
    test executable. """
    def __init__(self, name, executable, arguments, device_filter=None):
        self.name = name
        self.executable = executable
        self.arguments = arguments
        self.device_filter = device_filter
        self.ignore = False
        self.disabled = False
        self.unimplemented = False
        self.pool = Pool.NORMAL

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
                            copy.deepcopy(self.arguments),
                            copy.deepcopy(self.device_filter))
        new_test.ignore = self.ignore
        new_test.disabled = self.disabled
        new_test.unimplemented = self.unimplemented
        new_test.pool = self.pool
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

    @classmethod
    def from_file(cls, list_file_paths, disabled_file_path, ignored_file_path, prefix=""):
        """ Load a list of tests from a CTS list file. """
        tests = []
        disabled_tests = []
        ignored_tests = []
        executables = {}

        # Populate list of tests to disable
        filters = [
            (disabled_file_path, disabled_tests),
            (ignored_file_path, ignored_tests)
        ]
        for path, filter_tests in filters:
            if not path:
                continue

            with open(path, "r") as f:
                stripped = (line.strip() for line in f)
                filtered = (
                    line for line in stripped if line and not line.startswith("#"))
                chunked = csv.reader(filtered)
                filter_tests.extend(json.dumps(chunks) for chunks in chunked)

        for list_file_path in list_file_paths:
            with open(list_file_path, "r") as f:
                stripped = (line.strip() for line in f)
                filtered = (
                    line for line in stripped if line and not line.startswith("#"))
                chunked = csv.reader(filtered)
                for chunks in chunked:
                    device_filter = None
                    if chunks and chunks[0].startswith("CL_DEVICE_TYPE_"):
                        device_filter = chunks.pop(0)
                    if len(chunks) < 2:
                        raise Exception("Not enough columns in the CSV file")
                    argv = chunks[1].strip()
                    serialized = json.dumps(chunks)
                    ignored = serialized in ignored_tests
                    disabled = serialized in disabled_tests
                    unimplemented = False

                    pool = Pool.NORMAL
                    if len(chunks) >= 4:
                        pool_str = chunks[3]
                        if pool_str == 'Thread':
                            pool = Pool.THREAD
                        elif pool_str == 'Memory':
                            pool = Pool.MEMORY
                        elif pool_str != 'Normal':
                            raise Exception("Unknown pool '%s'" % pool_str)

                    if len(chunks) >= 3:
                        attribute = chunks[2]
                        if attribute == 'Ignore':
                            ignored = True
                        elif attribute == 'Disabled':
                            disabled = True
                        elif attribute == 'Unimplemented':
                            unimplemented = True
                        elif attribute:
                            raise Exception(
                                "Unknown attribute '%s'" % attribute)
                    executable_name_len = argv.find(" ")
                    if executable_name_len < 0:
                        executable_name = argv
                        arguments = []
                    else:
                        executable_name = argv[:executable_name_len]
                        arguments = shlex.split(
                            argv[executable_name_len + 1:], False)
                    try:
                        executable = executables[executable_name]
                    except KeyError:
                        short_name = executable_name.replace(
                            "conformance_test_", "")
                        if prefix:
                            short_name = "%s/%s" % (prefix, short_name)
                        executable = TestExecutable(
                            short_name, executable_name)
                        executables[executable_name] = executable
                    if arguments:
                        named_arguments = [
                            arg for arg in arguments
                            if arg and not arg.startswith("-")
                        ]
                        test_name = "%s/%s" % (executable.name,
                                               "_".join(named_arguments))
                        # Check for vector width argument and include it in test
                        # name as a '/v{vector_width}' suffix.
                        for arg in arguments:
                            vector_width_arg = re.search("-([1-9]+)", arg)
                            if vector_width_arg:
                                vec_string = vector_width_arg.group(0)
                                test_name += str("/v" + vec_string[1:])
                    else:
                        test_name = executable.name
                    test = TestInfo(test_name, executable, arguments,
                                    device_filter)
                    test.ignore = ignored
                    test.disabled = disabled
                    test.unimplemented = unimplemented

                    # Tests with predetermined pools based on resource usage
                    if test.match("allocations") or test.match("integer_ops"):
                        test.pool = Pool.MEMORY
                    elif test.match("conversions") or test.match("bruteforce"):
                        test.pool = Pool.THREAD
                    else:
                        test.pool = pool

                    tests.append(test)
        return cls(tests, executables)

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
        self.num_skipped = 0
        self.num_timeouts = 0
        self.num_passes_cts = 0
        self.num_total_cts = 0
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
        if run.status == "PASS":
            self.num_passes += 1
        elif run.status == "SKIP":
            self.num_skipped += 1
        elif run.status == "TIMEOUT":
            self.num_timeouts += 1
        else:
            self.num_fails += 1
        if run.total_tests is not None:
            self.num_total_cts += run.total_tests
            if run.passed_tests is not None:
                self.num_passes_cts += run.passed_tests

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

    def write_junit(self, out, suite_name):
        """ Print results to the Junit XML file for reading by Jenkins."""
        root_node = ET.Element('testsuites')
        testsuite_node = ET.Element("testsuite",
                                    name=suite_name,
                                    tests=str(self.num_tests),
                                    failures=str(self.num_fails))
        for run in sorted(self.runs.values(),
                          key=lambda r: r.schedule.end_time):
            run_time = ((run.schedule.end_time -
                         run.schedule.start_time).total_seconds())
            run_node = ET.Element("testcase",
                                  name=run.test.name,
                                  time=str(run_time),
                                  classname='%s.%s' %
                                  (suite_name, run.test.name))
            detail_node = None
            # Report timeout as failure because Jenkins reports timeout as
            # passing which is incorrect since the test result is unknown due
            # to not being completed, which could be hiding a failure.
            if run.status in ["FAIL", "TIMEOUT"]:
                detail_node = ET.Element("failure")
            elif run.status == "SKIP":
                detail_node = ET.Element("skipped")
            if detail_node is not None:
                if run.message:
                    detail_node.set("message", run.message)
                if run.output:
                    detail_node.text = run.output
                run_node.append(detail_node)
            testsuite_node.append(run_node)
        root_node.append(testsuite_node)
        out.write(ET.tostring(root_node).decode("utf8"))
        out.flush()
