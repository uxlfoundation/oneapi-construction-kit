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
from platform import system

from city_runner.test_info import TestExecutable, TestInfo, TestList
from city_runner.profiles.ssh import SSHProfile
from city_runner.profiles.ssh import SSHTestRun


def create_profile():
    """ Create a new instance of a profile. This is usually called once."""
    return BasicProfile()

class BasicProfile(SSHProfile):
    """
    This profile runs arbitrary tests described in a csv with the binary in the
    first field and arguments in following fields.

    This profile can be used to run locally or on a remote device through ssh.
    """

    def create_run(self, test, worker_state=None):
        """ Create a new test run from a test description.  """
        return BasicRun(self, test)

    def parse_options(self, argv):
        args = super(SSHProfile, self).parse_options(argv)
        if self.use_ssh():
            if not args.ssh_client:
                raise Exception("A SSH client (--ssh-client) couldn't be found.")

            # For SSH we want to default number of threads to 1
            if not args.jobs:
                args.jobs = 1
        return args

    def add_options(self, parser):
        """
        Add command-line options that can be used with the profile to the
        argument parser.
        """
        # Finds ssh, and other inherited options
        super(BasicProfile, self).add_options(parser)

    def use_ssh(self):
        """
        Determines whether we should run over ssh. --ssh-user is the
        only ssh argument without a default, so if it has been set use ssh.
        """
        return self.args.ssh_user is not None

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

    def load_tests(self, csv_paths, disabled_path, ignored_path):
        """ Find the list of tests from CSV. """
        if disabled_path:
            print("Warning: disabled list not supported for basic profile")

        if ignored_path:
            print("Warning: ignored list not supported for basic profile")

        parsed_tests = []
        # Load tests from CSV if any were provided
        if csv_paths:
            for csv_path in csv_paths:
                if not os.path.exists(csv_path):
                    raise Exception("Test list file does not exist")

                with open(csv_path, "r") as f:
                    for line in f:
                        # Skip commented out lines
                        if line.startswith('#'):
                            continue

                        # The first field of the csv is the executable name, and
                        # the following fields are the arguments
                        test_binary = line.split(",")[0].strip()
                        executable = TestExecutable(test_binary, test_binary)
                        parsed_tests.append(
                            TestInfo(test_binary, executable, line.split(",")[1:]))

        # Error if no tests were found
        if not parsed_tests:
            raise Exception("No tests found")

        return TestList(tests=parsed_tests).filter(self.args.patterns)

class BasicRun(SSHTestRun):
    """
      Represents the execution of a particular test.
    """

    def __init__(self, profile, test):
        super(BasicRun, self).__init__(profile, test)

    def analyze_process_output(self):
        """
        Determine whether the test passed or failed from the return code.
        """
        if self.return_code != 0:
            if self.schedule.aborted:
                self.status = "TIMEOUT"
            elif self.return_code == 124:
                # /usr/bin/timeout returns '124' when it aborts due to a
                # timeout, this is used when executing commands over ssh when a
                # timeout is set.
                self.status = "TIMEOUT"
            else:
                self.status = "FAIL"
            self.message = self.format_return_code(self.return_code)
        else:
            self.status = "PASS"

    def execute(self):
        """ Execute the test and wait for completion. """
        if self.profile.use_ssh():
            # Launch the binary over ssh
            ssh_run = SSHTestRun(self.profile, self.test)
            self.output = ssh_run.execute_and_output()
            self.return_code = ssh_run.return_code
        else:
            # Launch the binary locally
            exe_path = os.path.join(self.profile.args.binary_path,
                                    self.test.executable.path)
            env_vars = self.profile.build_environment_vars()
            self.create_process(exe_path, self.test.arguments, env=env_vars)

        # Parse test output
        self.analyze_process_output()
        if self.profile.use_ssh():
            self.rebootOnFail()
