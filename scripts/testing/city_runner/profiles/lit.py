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
import os
import sys
import time
import csv
import platform

import lit.cl_arguments
import lit.discovery
import lit.LitConfig
import lit.run
import lit.Test
import lit.util

import os
import re
from posixpath import join as posixpath_join

from city_runner.profiles.default import DefaultProfile
from city_runner.profiles.default import DefaultTestRun
from city_runner.test_info import TestInfo, TestList


def create_profile():
    """ Create a new instance of a profile. This is usually called once."""
    return LitProfile()


class LitProfile(DefaultProfile):
    """
    A profile which can be used with lit.

    New command line arguments:
        --lit_dir    -- should be used to dictate the lit directory or directories to use.
        --lit_filter -- will work the same way as on lit.
        --lit_list   -- will list all the tests.
        --lit_param  -- will work the same way as on lit.


    This uses the lit discovery and does not support any csv files apart from the override
    one which should be of the form <lit suite-name>,<test-name>,<attribute>.
    """

    def create_run(self, test, worker_state=None):
        """ Create a new test run from a test description.  """
        return LitProfileTestRun(self, test)

    def add_options(self, parser):
        """
        Add command-line options that can be used with the profile to the
        argument parser.
        """
        super(LitProfile, self).add_options(parser)

        parser.add_argument(
            "--lit_filter",
            type=str,
            default=None,
            help="Regex on full path name.")
        parser.add_argument(
            "--lit_dir",
            type=str,
            required=True,
            action="append",
            help="directory to forward to lit, can be used multiple times for a number of directories")
        parser.add_argument(
            "--lit_param",
            type=str,
            action="append",
            default=[],
            help="lit parameters, may be repeated"),
        parser.add_argument(
            "--lit_list",
            action='store_true',
            help="Output all found tests.")

    def load_tests(self, csv_paths, disabled_path, ignored_path, override_path):
        """ This uses lit to discover the test lists. """
        if csv_paths:
            print("Warning: csv list not supported for Lit profile")
        if disabled_path:
            print("Warning: disabled list not supported for Lit profile")
        if ignored_path:
            print("Warning: ignored_path list not supported for Lit profile")

        env_vars = self.build_environment_vars()
        os.environ.update(env_vars)

        self.lit_config = lit.LitConfig.LitConfig(
            progname="lit",
            path=[],
            quiet=True,
            useValgrind=False,
            valgrindLeakCheck=False,
            valgrindArgs=[],
            noExecute=False,
            debug=False,
            isWindows=(platform.system() == "Windows"),
            order=lit.cl_arguments.TestOrder.SMART,
            params=self.args.lit_param,
            config_prefix=""
        )
        if self.timeout is not None:
            self.lit_config.maxIndividualTestTime = self.timeout
        self.discovered_tests = lit.discovery.find_tests_for_inputs(
            self.lit_config, self.args.lit_dir)
        override_tests = []
        if override_path:
            override_tests = TestList.read_csv_file(override_path)
        test_info_list = []

        if self.args.lit_filter:
            try:
                rex = re.compile(self.args.lit_filter)
            except:
                print("Error: invalid regular expression for --filter: %r" %
                      (self.args.lit_filter))
                sys.exit(1)
            self.discovered_tests = [
                result_test for result_test in self.discovered_tests if rex.search(result_test.getFullName())]
        if self.args.lit_list:
            print("Lit discovered tests:\n")
            for d in self.discovered_tests:
                print(d.suite.config.name + "/" + '/'.join(d.path_in_suite))
            sys.exit(0)

        for cfg in {t.config for t in self.discovered_tests}:
            cfg.environment.update(self.tmp_dir_envs)

        # Add from discovered tests, adding to attributes as necessary from the csv files
        for d in self.discovered_tests:
            test_info = TestInfo(d.suite.config.name +
                                 "/" + '/'.join(d.path_in_suite), d, [])
            # Set the default unknown attribute based on the command arg
            test_info.unknown = self.args.default_unknown
            # Check against override files and update the attributes
            # Note this will normally set the Unknown attribute to False unless the attribute
            # is in the csv files
            for chunks in override_tests:
                if len(chunks) >= 2 and chunks[0] == d.suite.config.name and chunks[1] == '/'.join(d.path_in_suite):
                    test_info.update_test_info_from_attribute(chunks[2] if len(chunks) >= 3 else "")

            test_info_list.append(test_info)

        return TestList(tests=test_info_list)

    def init_workers_state(self, expected_num_entries):
        self.manages_timeout = True
        # Create a temp directory inside the normal temp directory so that we can
        # try to avoid temporary test file leaks as done with lit. LIT_PRESERVES_TMP is
        # supported in the same way.
        self.tmp_dir = None
        self.tmp_dir_envs = {}
        if "LIT_PRESERVES_TMP" not in os.environ:
            import tempfile
            self.tmp_dir = tempfile.mkdtemp(prefix="lit-tmp-")
            self.tmp_dir_envs = {k: self.tmp_dir for k in [
                "TMP", "TMPDIR", "TEMP", "TEMPDIR"]}
            os.environ.update(self.tmp_dir_envs)

    def release_workers_state(self, expected_num_entries):
        if self.tmp_dir:
            try:
                import shutil

                shutil.rmtree(self.tmp_dir)
            except Exception as e:
                lit_config.warning(
                    "Failed to delete temp directory '%s', try upgrading your version of Python to fix this"
                    % self.tmp_dir
                )


class LitProfileTestRun(DefaultTestRun):
    """
      Represents the execution of a particular Lit test.
    """

    def __init__(self, profile, test, quiet=False):
        super(LitProfileTestRun, self).__init__(profile, test)

    def execute(self):
        """ Execute the test and wait for completion. """
        self.run = lit.run.Run(
            [self.test.executable], self.profile.lit_config, 1, None, None, self.profile.timeout)

        timeout_caught = False
        try:
            self.run.execute()
        except lit.run.TimeoutError:
            timeout_caught = True

        for test in self.run.tests:
            if test.result.code.name == "PASS":
                self.status = "PASS"
                self.passed_tests = 1
            elif test.result.code.name == "UNSUPPORTED" or test.result.code.name == "UNRESOLVED":
                self.status = "SKIP"
            elif test.result.code.name == "FLAKYPASS":
                self.status = "MAYFAIL"
            elif test.result.code.name == "XFAIL":
                self.status = "XFAIL"
            elif test.result.code.name == "TIMEOUT" or timeout_caught:
                self.status = "TIMEOUT"
            elif test.result.code.name == "FAIL":
                self.status = "FAIL"
            else:
                print("Warning : Unexpected lit result ", test.result.code.name)
                self.status = "FAIL"
            self.total_tests = 1
            self.output = bytes(test.result.output, 'utf-8')
