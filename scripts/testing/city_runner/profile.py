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

import argparse
import importlib
import os
import re

from city_runner.test_info import TestList
from city_runner.ui import ColorMode


class ProfileError(Exception):
    pass


class Profile(object):
    """ Base class for profiles, which determine how tests should be executed.
     The default profile executes tests locally, but other profiles could
     use SSH to run tests remotely for example. """

    def __init__(self):
        self.args = None
        self.timeout = None
        self.runner = None

    @staticmethod
    def extract_name(argv):
        """
        Try to extract the profile name from the argument list.
        :param argv: Argument list to parse.
        :return: Pair containing the profile name and remaining arguments.
        """
        for i in range(0, len(argv)):
            if ((argv[i] == "-p") or
                (argv[i] == "--profile")) and (i + 1) < len(argv):
                name = argv[i + 1]
                new_argv = argv[:i] + argv[i + 2:]
                return name, new_argv
        return "default", argv

    @staticmethod
    def load(name):
        """ Try to load the profile with the given name. """
        try:
            module = importlib.import_module(f'city_runner.profiles.{name}')
        except ModuleNotFoundError as error:
            raise ProfileError(f"Could not load profile '{name}'") from error
        try:
            return getattr(module, "create_profile")()
        except AttributeError as error:
            raise ProfileError(
                "The profile module does not implement 'create_profile'"
            ) from error

    def parse_options(self, argv):
        """ Parse and validate the given command-line arguments. """
        parser = argparse.ArgumentParser()
        self.add_options(parser)
        args = parser.parse_args(argv)
        if args.timeout:
            hours = minutes = seconds = 0
            m = re.match(r"^(?:(\d{2,}):)?(\d{2}):(\d{2})$", args.timeout)
            if m:
                hours = int(m.group(1) or 0)
                minutes = int(m.group(2))
                seconds = int(m.group(3))
            else:
                try:
                    seconds = int(args.timeout)
                except ValueError:
                    raise ValueError(
                        "Invalid timeout value. Format: [HH:]MM:SS or integer number of seconds"
                    )
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
                    raise ProfileError(
                        "Missing environment variable name: '%s'" % var_def)
            args.env_vars = new_vars
        self.args = args
        return args

    def add_options(self, parser):
        """ Add command-line options that can be used with the profile to the
         argument parser. """
        parser.add_argument(
            "-s",
            "--test-source", nargs="+",
            help="File containing a list of tests to run")
        parser.add_argument(
            "-d",
            "--disabled-source",
            default="",
            help="File containing a list of disabled tests to skip and count"
            " as failed")
        parser.add_argument(
            "-i",
            "--ignored-source",
            default="",
            help="File containing a list of ignored tests to skip.")
        parser.add_argument(
            "-l", "--log-file", type=str, help="File to log test output to")
        parser.add_argument(
            "-f",
            "--fail-file",
            type=str,
            help="File to write the list of failed tests to")
        parser.add_argument(
            "-r",
            "--junit-result-file",
            type=str,
            help="File to write the test results in JUnit format to")
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
            "--color",
            type=ColorMode,
            choices=list(ColorMode),
            default=ColorMode.auto,
            help="Control the use of color in terminals ")
        # Retained for backwards compatibility in test scripts. The previous
        # behaviour of --strict is now the default, it's hidden from --help.
        parser.add_argument(
            "--strict",
            default=True,
            action="store_true",
            help=argparse.SUPPRESS)
        parser.add_argument(
            "--relaxed",
            default=False,
            action="store_true",
            help="Return a zero exit code when there are test failures")
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

    def load_tests(self, csv_paths, disabled_path, ignored_path):
        """ Create the list of tests to run from a CSV. """
        if not csv_paths or any(not csv_path or not os.path.exists(csv_path) for csv_path in csv_paths):
            raise Exception("Test list file not specified or does not exist")
        if disabled_path and not os.path.exists(disabled_path):
            raise Exception("Disabled test list file does not exist")
        if ignored_path and not os.path.exists(ignored_path):
            raise Exception("Ignored test list file does not exist")
        tests = (TestList
                 .from_file(csv_paths, disabled_path, ignored_path, self.args.test_prefix)
                 .filter(self.args.patterns))
        return tests

    def init_workers_state(self, expected_num_entries):
        """
        Entry point for profile specific, per worker state initialization.
        """
        return []
