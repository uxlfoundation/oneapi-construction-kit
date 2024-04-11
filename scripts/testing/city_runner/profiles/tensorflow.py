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
from subprocess import check_output
from platform import system

from city_runner.cts import CTSTestRun
from city_runner.test_info import TestExecutable, TestInfo, TestList
from city_runner.profiles.ssh import SSHProfile
from city_runner.profiles.ssh import find_extension_function
import city_runner.extensions


def create_profile():
    """ Create a new instance of a profile. This is usually called once."""
    return TensorflowProfile()


class TensorflowProfile(SSHProfile):
    """
    This profile executes tests from a Tensorflow executable. Inheriting from
    SSHProfile since we often want to run Tensorflow testing on device.
    """

    def create_run(self, test, worker_state=None):
        """ Create a new test run from a test description.  """
        return TensorflowRun(self, test)

    def add_options(self, parser):
        """
        Add command-line options that can be used with the profile to the
        argument parser. These options are forwarded to the Tensorflow
        executable.
        """
        # Finds ssh, and other inherited options
        super(TensorflowProfile, self).add_options(parser)

        parser.add_argument(
            "--tf_root",
            type=str,
            action="append",
            default=[],
            help="Set the tf root directory")

        # --Tensorflow_argument can be set multiple times for each distinct
        # argument to be passed
        parser.add_argument(
            "--Tensorflow_argument",
            type=str,
            action="append",
            default=[],
            help="Argument to forward to the Tensorflow executable")

    def build_environment_vars(self):
        """
        Builds and returns a dictionary of environment variables to be passed
        to new processes based on command line arguments.
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
        Determines whether we should run Tensorflow over ssh. --ssh-user is the
        only ssh argument without a default, so if it has been set use ssh.
        """
        return self.args.ssh_user is not None

    def parse_options(self, argv):
        """ Parse and validate the given command-line arguments. """
        args = super(SSHProfile, self).parse_options(argv)

        # For SSH we want to default number of threads to 1
        if self.use_ssh() and not args.jobs:
            args.jobs = 1

        return args

    def load_tests(self, csv_paths, disabled_path, ignored_path):
        """ 
        Find the list of tests from CSV or fallback to Tensorflow binary.
        """
        if disabled_path:
            print("Warning: disabled list not supported for tensorflow profile")

        if ignored_path:
            print("Warning: ignored list not supported for tensorflow profile")

        # Find path to Tensorflow executable
        Tensorflow_exe_path = os.path.abspath(self.args.binary_path)
        Tensorflow_dir = os.path.dirname(Tensorflow_exe_path)
        Tensorflow_file = os.path.basename(Tensorflow_exe_path)
        executable = TestExecutable(Tensorflow_file, Tensorflow_dir)

        # Construct arguments to be used when running tests
        bin_args = []
        for arg in self.args.Tensorflow_argument:
            bin_args.append(arg)
        if system() == "Windows":
            bin_args.append("--Tensorflow_color=no")

        parsed_tests = []
        # Load tests from CSV if any were provided
        if csv_paths:
            for csv_path in csv_paths:
                if not os.path.exists(csv_path):
                    raise Exception("Test list file does not exist")

                with open(csv_path, "r") as f:
                    for line in f:
                        # Assume for now that the first item in the CSV is the
                        # fully qualified test name. This may change one we start
                        # using this feature.
                        test_name = os.path.join(
                            self.args.tf_root[0], line.split(",")[0].strip())
                        parsed_tests.append(
                            TestInfo(test_name, executable, bin_args))
        else:
            raise Exception("Please provide a list of tests in CSV format")

        # Error if no tests were found
        if not parsed_tests:
            raise Exception(
                "No tests found for '%s'" % " ".join(find_tests_cmd))

        return TestList(tests=parsed_tests)


class TensorflowRun(CTSTestRun):
    """
      Represents the execution of a particular Tensorflow test.
    """

    def __init__(self, profile, test):
        super(TensorflowRun, self).__init__(profile, test)

    def execute(self):
        """ Execute the test and wait for completion. """
        exe = self.test.executable

        Tensorflow_exe_path = self.test.name

        run_test_cmd = [Tensorflow_exe_path] + \
            self.test.arguments

        if self.profile.use_ssh():
            # Launch Tensorflow binary over ssh
            exports = super(TensorflowProfile,
                            self.profile).construct_cmd_env()
            run_test_string = " ".join(run_test_cmd)
            if exports:
                run_test_string = exports + " && " + run_test_string
            ssh_cmd = super(TensorflowProfile, self.profile).construct_ssh_command(
                run_test_string, True)
            self.create_process(ssh_cmd[0], args=ssh_cmd[1:])
        else:
            # Launch Tensorflow binary locally
            env_vars = self.profile.build_environment_vars()
            self.create_process(
                run_test_cmd[0], args=run_test_cmd[1:], env=env_vars)

        # Parse test output
        self.analyze_process_output()

    def analyze_process_output(self):
        timeout_reboot = False
        # Return code 143 refers to SIGTERM which means the process was killed
        # in our case the usually is a result of a timeout. In some
        # circumstances an application killed can cause problems with a device
        # and as such we need to perform a reboot before continuing.
        if self.return_code == 143:
            timeout_reboot = True
        """ Determine whether a test passed or failed from its return code """
        embedded_reboot = find_extension_function("embedded_reboot")
        if embedded_reboot:
            # Pass test output as parameter since reboot is only performed
            # under certain conditions
            embedded_reboot(self.profile.args.ssh_host,
                            self.output,
                            timeout_reboot)

        self.total_tests = 1

        if self.return_code == 0:
            self.passed_tests = 1
            self.status = "PASS"
        else:
            self.passed_tests = 0
            self.status = "FAIL"
