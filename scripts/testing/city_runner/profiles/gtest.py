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
import re
from io import BytesIO
from subprocess import check_output
from platform import system
from posixpath import join as posixpath_join

from city_runner.cts import CTSTestRun
from city_runner.test_info import TestExecutable, TestInfo, TestList
from city_runner.profiles.ssh import SSHProfile
from city_runner.profiles.ssh import SSHTestRun


def create_profile():
    """ Create a new instance of a profile. This is usually called once."""
    return GTestProfile()


class GTestProfile(SSHProfile):
    """
    This profile executes tests from a googletest executable. Inheriting from
    SSHProfile since we often want to run googletest testing on device.
    """

    def create_run(self, test, worker_state=None):
        """ Create a new test run from a test description.  """
        return GoogleTestRun(self, test)

    def add_options(self, parser):
        """
        Add command-line options that can be used with the profile to the
        argument parser. These options are forwarded to the googletest
        executable.
        """
        # Finds ssh, and other inherited options
        super(GTestProfile, self).add_options(parser)

        parser.add_argument(
            "--qemu",
            dest="qemu",
            type=str,
            help="Path to the QEMU executable to emulate tests.")

        # --gtest_argument can be set for multiple times for each distinct
        # argument to be passed
        parser.add_argument(
            "--gtest_argument",
            "-g",
            type=str,
            action="append",
            default=[],
            help="Argument to forward to the googletest executable")

        parser.add_argument(
            "--list",
            type=str,
            default='',
            help="Path to file in which to list all available tests. With this option no tests are executed.")

    def build_environment_vars(self):
        """
        Builds and returns dictionary of environment variables to be passed to
        new processes based on command line arguments.
        """
        # Add user-defined environment variables.
        env = {}
        for key, value in self.args.env_vars:
            env[key] = value

        # Parse the list of library paths from the environment.
        env_var_name = None
        env_var_separator = None
        lib_dirs = []
        if system() == "Windows":
            env_var_name = "PATH"
            env_var_separator = ";"
        else:
            env_var_name = "LD_LIBRARY_PATH"
            env_var_separator = ":"

        try:
            orig_path = os.environ[env_var_name]
            if orig_path:
                lib_dirs.extend(orig_path.split(env_var_separator))
        except KeyError:
            pass

        # Add user-defined library paths, making them absolute in the process.
        for user_path in self.args.lib_paths:
            lib_dirs.append(os.path.realpath(user_path))
        if lib_dirs:
            env[env_var_name] = env_var_separator.join(lib_dirs)

        return env

    def use_ssh(self):
        """
        Determines whether we should run googletest over ssh. --ssh-user is the
        only ssh argument without a default, so if it has been set use ssh.
        """
        return self.args.ssh_user is not None

    def parse_options(self, argv):
        """ Parse and validate the given command-line arguments. """
        # Don't call SSHProfile parse_options which will throw an exeception
        # if --ssh-user isn't set. Since we may be executing locally.
        args = super(SSHProfile, self).parse_options(argv)

        if self.use_ssh():
            if not args.ssh_client:
                raise Exception(
                    "A SSH client (--ssh-client) couldn't be found.")

            # For SSH we want to default number of threads to 1
            if not args.jobs:
                args.jobs = 1
        if not self.use_ssh():
            args.binary_path = os.path.abspath(args.binary_path)
        args.binary_name = os.path.basename(args.binary_path)
        args.binary_path = os.path.dirname(args.binary_path)
        return args

    def create_test_info(self, test_name, executable, bin_args):
        # Insert filter at end of command to override any filters set by the
        # user via --gtest_argument to city runner.
        test_bin_args = bin_args[:] + ["--gtest_filter=%s" % test_name]

        # Expand special patterns in the ccommand line options
        test_name_subbed = re.sub(r'/|\\.', '_', test_name)
        test_bin_args_expanded = [
            arg.replace("${TEST_NAME}", test_name_subbed)
            for arg in test_bin_args
        ]

        return TestInfo(test_name, executable, test_bin_args_expanded)

    # Call binary to find the list of matching tests
    def list_tests(self, executable, additional_args):
        list_bin_args = additional_args + ["--gtest_list_tests"]
        # Don't output this to the user's specified file.
        filter_args = filter(lambda x: "--gtest_output" not in x,
                             list_bin_args)

        test_list_info = TestInfo("", executable, list(filter_args))
        test_list = GoogleTestRun(
            self, test_list_info, quiet=True).execute_and_output()

        # Parse output to create list of tests
        test_prefix = ""
        parsed_tests = []
        for line in test_list.splitlines():
            # Python 3 returns a byte object for check_output
            line = line.decode()

            # Tests can have comments after a '#'
            stripped = line.split("#")[0].strip()

            # Test categories end in '.'
            if stripped and (stripped[-1] == "."):
                test_prefix = stripped
            elif test_prefix:
                test_name = test_prefix + stripped
                parsed_tests.append(self.create_test_info(test_name, executable,
                                    additional_args))
        return parsed_tests

    def parse_gtest_csv(self, csv_file):
        test_names = []
        with open(csv_file, "r") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                # Assume for now that the first item in the CSV is the
                # fully qualified test name. This may change one we start
                # using this feature.
                test_name = line.split(",")[0].strip()
                test_names.append(test_name)

        return test_names

    def load_tests(self, csv_paths, disabled_path, ignored_path):
        """ Find the list of tests from CSV or fallback to gtest binary. """
        if disabled_path:
            print("Warning: disabled list not supported for gtest profile")
        executable = TestExecutable(self.args.binary_name,
                                    self.args.binary_name)

        # Construct arguments to be used when running tests
        bin_args = self.args.gtest_argument[:]
        bin_args.append("--gtest_color=no")

        parsed_tests = []
        if self.args.list != '':
            tests = self.list_tests(executable, [])
            with open(self.args.list, "w+") as f:
                for test in tests:
                    f.write(test.name + "\n")
        # Load tests from CSV if one was provided
        elif csv_paths:
            for csv_path in csv_paths:
                if not os.path.exists(csv_path):
                    raise Exception("Test list file does not exist")

                tests = self.parse_gtest_csv(csv_path)
                for test in tests:
                    parsed_tests.append(
                        self.create_test_info(test, executable, bin_args))
        else:
            parsed_tests = self.list_tests(executable, bin_args[:])

        if ignored_path:
            if not os.path.exists(ignored_path):
                raise Exception("Ignored list file does not exist")
            ignored = self.parse_gtest_csv(ignored_path)
            for test in parsed_tests:
                test.ignore = test.name.strip() in ignored

        # Error if no tests were found
        if not self.args.list and not parsed_tests:
            raise Exception("No tests found")

        return TestList(tests=parsed_tests).filter(self.args.patterns)


class GoogleTestRun(SSHTestRun):
    """
      Represents the execution of a particular GTest test.
    """

    def __init__(self, profile, test, quiet=False):
        super(GoogleTestRun, self).__init__(profile, test, quiet=quiet)

    def execute(self):
        """ Execute the test and wait for completion. """
        args = self.profile.args

        if self.test.ignore:
            self.status = "SKIP"
            self.total_tests = 0
            return

        if self.profile.use_ssh():
            # Disable rebooting for the SSHTestRun, rebooting inside of it is
            # wrong because it doesn't have the correct output analysis,
            # rebooting is handled at the end of this execute function
            reboot_on_fail = self.profile.args.ssh_reboot_on_fail
            reboot_on_timeout = self.profile.args.ssh_reboot_on_timeout
            self.profile.args.ssh_reboot_on_fail = False
            self.profile.args.ssh_reboot_on_timeout = False

            # Launch googletest binary over ssh
            ssh_run = SSHTestRun(self.profile, self.test, quiet=self.quiet)
            self.output = ssh_run.execute_and_output()
            self.return_code = ssh_run.return_code

            self.profile.args.ssh_reboot_on_fail = reboot_on_fail
            self.profile.args.ssh_reboot_on_timeout = reboot_on_timeout
        elif args.qemu:
            # Launch googletest locall in qemu
            qemu = args.qemu.split()[0]
            qemu_args = args.qemu.split()[1:]
            qemu_args.append(
                os.path.join(self.profile.args.binary_path,
                             self.test.executable.path))
            qemu_args.extend(self.test.arguments)
            env_vars = self.profile.build_environment_vars()
            self.create_process(qemu, qemu_args, env=env_vars)
        else:
            # Launch googletest binary locally
            gtest_exe_path = os.path.join(self.profile.args.binary_path,
                                          self.test.executable.path)
            env_vars = self.profile.build_environment_vars()
            self.create_process(gtest_exe_path,
                                self.test.arguments,
                                env=env_vars)

        # Parse test output
        self.analyze_process_output()
        self.rebootOnFail()

    def analyze_process_output(self):
        """ Determine whether a test passed or failed from its output. """
        # When querying GTest for the test list the test name is empty.
        # Do not analyze the process output.
        if not self.test.name:
            return

        # Regex expressions to match
        total_tests_pattern = re.compile(
            b".*?(\\d+) (test cases?|tests? from \\d+ test suites?) ran\\.")
        pass_pattern = re.compile(b"^\\[  PASSED  \\] (\\d+) tests?\\.$")
        skip_pattern = re.compile(b"^\\[  SKIPPED \\] (\\d+) tests?")

        total_tests = 0
        pass_num = 0
        skip_num = 0
        # Look for specific patterns in the output text.
        for line in BytesIO(self.output):
            line = line.rstrip()
            total_tests_match = total_tests_pattern.match(line)
            pass_match = pass_pattern.match(line)
            skip_match = skip_pattern.match(line)
            if total_tests_match:
                total_tests = int(total_tests_match.group(1))
            elif pass_match:
                pass_num = int(pass_match.group(1))
            elif skip_match:
                skip_num = int(skip_match.group(1))

        self.total_tests = total_tests
        self.passed_tests = pass_num

        # We assume that total_tests == 1, otherwise we fail even if some tests
        # succeed and others are skipped.
        if self.return_code:
            # Test could segfault before any output is printed
            if not self.total_tests:
                self.total_tests = 1
            super(CTSTestRun, self).analyze_process_output()
        elif total_tests == 0:
            # googletest can have tests marked disabled which don't run
            self.status = "SKIP"
        elif total_tests == pass_num:
            self.status = "PASS"
        elif total_tests == skip_num:
            # googletest can have tests which are run but are marked to be
            # skipped from within the test
            self.status = "SKIP"
        else:
            self.status = "FAIL"
