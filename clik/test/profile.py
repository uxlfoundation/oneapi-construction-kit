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

import imp
import os
import re
import argparse
from platform import system
from test_info import TestList


class Profile(object):
    """ Base class for profiles, which determine how tests should be executed."""

    def __init__(self):
        self.args = None
        self.timeout = None
        self.runner = None

    def parse_options(self, argv):
        """ Parse and validate the given command-line arguments. """
        parser = argparse.ArgumentParser()
        self.add_options(parser)
        args = parser.parse_args(argv)
        if args.timeout:
            m = re.match("^(?:(\\d{2,}):)?(\\d{2}):(\\d{2})$", args.timeout)
            if not m:
                raise Exception("Invalid timeout value. Format: [HH:]MM:SS")
            hours = int(m.group(1) or 0)
            minutes = int(m.group(2))
            seconds = int(m.group(3))
            self.timeout = seconds + (minutes * 60) + (hours * 3600)
        if args.env_vars:
            # Parse environment variable definitions into something easier to
            # consume.
            new_vars = []
            for var_def in args.env_vars:
                pos = var_def.find("=")
                if pos > 0:
                    new_vars.append((var_def[:pos], var_def[pos + 1:]))
                elif pos < 0:
                    new_vars.append((var_def, ""))
                else:
                    raise Exception(
                        "Missing environment variable name: '%s'" % var_def)
            args.env_vars = new_vars
        self.args = args
        return args

    def add_options(self, parser):
        """ Add command-line options that can be used with the profile to the
         argument parser. """
        parser.add_argument(
            "-s",
            "--test-source",
            help="File containing a list of tests to run")
        parser.add_argument(
            "-l", "--log-file", type=str, help="File to log test output to")
        parser.add_argument(
            "-f",
            "--fail-file",
            type=str,
            help="File to write the list of failed tests to")
        parser.add_argument(
            "-R",
            "--repeat",
            type=int,
            default=1,
            help="Number of times to repeat the tests")
        parser.add_argument(
            "-j",
            "--jobs",
            type=int,
            default=0,
            help="Number of concurrent jobs to run")
        parser.add_argument(
            "-b",
            "--binary-path",
            type=str,
            default='.',
            help="Path to the directory containing test executables")
        parser.add_argument(
            "-L",
            "--add-lib-path",
            action="append",
            default=[],
            dest="lib_paths",
            help="Add a directory to the library search path")
        parser.add_argument(
            "-e",
            "--add-env-var",
            action="append",
            default=[],
            dest="env_vars",
            help="Add an environment variable to be defined "
            "when invoking test executables")
        # We actually parse this argument manually in the `extract_name` static
        # method, however adding it to argparse means the option gets printed
        # in the help message
        parser.add_argument(
            "-p",
            "--profile",
            type=str,
            help="Select a profile to run different test suites, or execute "
            "tests in alternative ways, such as remotely.")
        parser.add_argument(
            "--test-prefix",
            type=str,
            default='cl12',
            help="Prefix used for test names")
        parser.add_argument(
            "-t",
            "--timeout",
            type=str,
            help="Set a limit to how long the execution of a test can take."
            " The format is: [HH:]MM:SS")
        parser.add_argument(
            "-v",
            "--verbose",
            default=False,
            action="store_true",
            help="Make the output more verbose")
        parser.add_argument(
            "--no-color",
            default=False,
            action="store_true",
            help="Disable the use of color in terminals")
        parser.add_argument(
            "--strict",
            default=False,
            action="store_true",
            help="Return a non-zero exit code when there are test failures")
        parser.add_argument(
            'patterns',
            metavar='PATTERN',
            type=str,
            nargs='*',
            help='Patterns to use for filtering tests')

    def set_default(self, parser, key, value):
        """ Set the default value for the given option key to the desired value. """
        parser.set_defaults(**{key: value})

    def create_run(self, test, worker_state=None):
        """ Create a new test run from a test description.  """
        raise NotImplementedError()

    def load_tests(self, csv_path):
        """ Create the list of tests to run from a CSV. """
        raise NotImplementedError()

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

    def init_workers_state(self, expected_num_entries):
        """
        Entry point for profile specific, per worker state initialization.
        """
        return []
