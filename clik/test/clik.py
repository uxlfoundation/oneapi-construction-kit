# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
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

from test_info import TestExecutable, TestInfo, TestList
from profile import Profile
from execution import TestRunBase

# List of tests to run.
# Format: binary name, command-line arguments, validation ID
test_info = [
    ("hello", "-S4 -L1", "hello"),
    ("vector_add", "", None),
    ("copy_buffer", "", None),
    ("hello_async", "-S8 -L4", "hello"),
    ("vector_add_async", "", None),
    ("vector_add_wfv", "", None),
    ("barrier_sum", "", None),
    ("barrier_print", "", "barrier_print"),
    ("matrix_multiply", "", None),
    ("matrix_multiply_tiled", "", None),
    ("ternary_async", "", None),
    ("blur", "", "blur"),
    ("concatenate_dma", "-S8", None),
    ("copy_buffer_async", "", None),
    ("hello_mux", "-S8 -L4", "hello"),
    ("vector_add_mux", "", None),
    ("barrier_sum_mux", "", None),
    ("barrier_print_mux", "", "barrier_print"),
    ("matrix_multiply_mux", "", None),
    ("matrix_multiply_tiled_mux", "", None),
    ("copy_mux", "", None)
]


def create_profile():
    """ Create a new instance of a profile. This is usually called once."""
    return ClikProfile()


class ClikTestInfo(TestInfo):
    def __init__(self, name, executable, arguments):
        super(ClikTestInfo, self).__init__(name, executable, arguments)
        self.validation_id = None


class ClikProfile(Profile):
    """
    This profile runs arbitrary tests described in a csv with the binary in the
    first field and arguments in following fields.

    This profile can be used to run locally or on a remote device through ssh.
    """

    def create_run(self, test, worker_state=None):
        """ Create a new test run from a test description.  """
        return ClikRun(self, test)

    def load_tests(self, csv_path):
        """ Find the list of tests from CSV. """
        parsed_tests = []

        for test_name, arguments, validation_id in test_info:
            test_binary = test_name
            executable = TestExecutable(test_binary, test_binary)
            executable_path = os.path.join(self.args.binary_path, test_binary)
            if not os.path.exists(executable_path):
                continue
            test = ClikTestInfo(test_binary, executable, arguments.split(" "))
            test.validation_id = validation_id
            parsed_tests.append(test)

        return TestList(tests=parsed_tests).filter(self.args.patterns)

class ValidationError(Exception):
    pass

class ClikRun(TestRunBase):
    """
      Represents the execution of a particular test.
    """

    def __init__(self, profile, test):
        super(ClikRun, self).__init__(profile, test)

    def analyze_process_output(self):
        """
        Determine whether the test passed or failed from the return code.
        """
        if self.test.validation_id:
            # Some tests require special validation of their output.
            func_name = "analyze_{0}".format(self.test.validation_id)
            if hasattr(self, func_name):
                validation_func = getattr(self, func_name)
                try:
                    if validation_func():
                        self.status = "PASS"
                        return
                    else:
                        self.add_output("\nvalidation error: unknown failure\n")
                except ValidationError as e:
                    self.add_output("\nvalidation error: {0}\n".format(e))
        elif self.return_code == 0:
            # Most tests only need their return code validated.
            self.status = "PASS"
            return

        # Distinguish between failures and timeouts.
        if self.schedule.aborted:
            self.status = "TIMEOUT"
        else:
            self.status = "FAIL"

    def execute(self):
        """ Execute the test and wait for completion. """
        # Launch the binary locally
        exe_path = os.path.join(self.profile.args.binary_path,
            self.test.executable.path)
        env_vars = self.profile.build_environment_vars()
        self.create_process(exe_path, self.test.arguments, env=env_vars)

        # Parse test output
        self.analyze_process_output()
    
    def analyze_hello(self):
        banner_expr = re.compile(rb"^Running hello[a-zA-Z0-9_]* example "
            rb"\(Global size: (\d+), local size: (\d+)\)$")
        workitem_expr = re.compile(rb"^Hello from ([a-zA-Z0-9_]+)! "
            rb"tid=(\d+), lid=(\d+), gid=(\d+)$")

        # Parse the test output.
        items = {}
        global_size, local_size = None, None
        for line in self.output.split(b"\n"):
            m = banner_expr.match(line)
            if m:
                global_size, local_size = int(m.group(1)), int(m.group(2))
                size = (global_size, local_size)
                continue
            m = workitem_expr.match(line)
            if m:
                name, tid, lid, gid = (m.group(1), int(m.group(2)),
                    int(m.group(3)), int(m.group(4)))
                items[tid] = (name, tid, lid, gid)

        # Make sure we have parsed enough items.
        if not global_size or not local_size:
            raise ValidationError("Test banner was not found in the output")
        elif len(items) != global_size:
            raise ValidationError("Found {0} items in output, but expected {1}"
                .format(len(items), global_size))

        # Make sure all global IDs are present and the local/group IDs are
        # consistent with the global IDs.
        names = set()
        for i in range(0, global_size):
            item = items.get(i, None)
            if item is None:
                raise ValidationError("Could not find item {0}".format(i))
            name, tid, lid, gid = item
            if lid != (tid % local_size):
                raise ValidationError("Local ID {0} does not match other IDs".format(lid))
            elif gid != (tid // local_size):
                raise ValidationError("Group ID {0} does not match other IDs".format(gid))
            names.add(name)

        # Make sure all items printed the same name.
        if len(names) != 1:
            raise ValidationError("Multiple names printed: {0}".format(names))

        return True

    def analyze_barrier_print(self):
        """ Validate the output of a 'barrier_print' test. """
        banner_expr = re.compile(rb"^Running barrier_print[a-zA-Z0-9_]* example "
            rb"\(Global size: (\d+), local size: (\d+)\)$")
        workitem_expr = re.compile(rb"^Kernel part (A|B) \(tid = (\d+)\)$")

        # Parse the test output.
        items = []
        global_size, local_size = None, None
        for line in self.output.split(b"\n"):
            m = banner_expr.match(line)
            if m:
                global_size, local_size = int(m.group(1)), int(m.group(2))
                size = (global_size, local_size)
                continue
            m = workitem_expr.match(line)
            if m:
                name, tid = (m.group(1), int(m.group(2)))
                items.append((name, tid))

        # Make sure we have parsed enough items.
        part_names = [b"A", b"B"]
        if not global_size or not local_size:
            raise ValidationError("Test banner was not found in the output")
        expected_item_count = global_size * len(part_names)
        if len(items) != expected_item_count:
            raise ValidationError("Found {0} items in output, but expected {1}"
                .format(len(items), expected_item_count))

        # Sort items into parts and make sure there is one item per global ID.
        items_by_part = {part_name: {} for part_name in part_names}
        for name, tid in items:
            items_by_part[name][tid] = tid
        for part_name, part_items in items_by_part.items():
            for i in range(0, global_size):
                if part_items.get(i, None) is None:
                    raise ValidationError("Could not find item {0} for part {1}"
                        .format(i, part_name))

        # Ensure that barriers are enforced.
        items_seen_by_part = {part_name: 0 for part_name in part_names}
        for part, tid in items:
            items_seen_by_part[part] += 1
            part_index = part_names.index(part)
            if part_index < 0:
                return False
            for succ_part in part_names[part_index + 1:]:
                if items_seen_by_part[succ_part] > 0:
                    raise ValidationError("Item with part {0} (tid = {2}) executed after "
                        "previous item with part {1}"
                         .format(part.decode("utf8"), succ_part.decode("utf8"), tid))

        return True

    def analyze_blur(self):
        """ Validate the output of the 'blur' test. """
        expected = [
            [0,  0,  0,  0,  0,  0,  0,  0, 22, 33, 33, 11,  0,  0,  0],
            [0, 11, 22, 33, 33, 33, 33, 22, 22, 22, 33, 22, 11,  0,  0],
            [0, 22, 44, 66, 66, 66, 66, 44, 22, 11, 22, 25, 14,  3,  0],
            [0, 33, 66, 99, 99, 99, 99, 66, 33,  0, 11, 14, 18,  7,  3],
            [0, 33, 66, 99, 99, 99, 99, 73, 40,  7,  0,  3,  7, 11, 11],
            [0, 22, 44, 66, 73, 80, 88, 73, 51, 22,  7,  0,  3,  7, 11],
            [0, 11, 22, 33, 47, 62, 77, 73, 62, 36, 14,  0,  0,  3,  7],
            [0,  0,  0,  0, 22, 44, 66, 66, 66, 44, 22,  0,  0,  0,  0],
            [0,  0,  0,  0, 14, 36, 58, 66, 58, 36, 14,  0 , 0,  0,  0],
            [0,  0,  0,  0,  7, 22, 36, 44, 36, 22,  7,  0,  0,  0,  0],
            [0,  0,  0,  0,  0,  7, 14, 22, 14,  7,  0,  0,  0,  2,  4],
            [0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  8]
        ]

        # Parse test output.
        number_list_expr = re.compile(rb"^(\s*\d+)+$")
        actual = []
        for line in self.output.split(b"\n"):
            m = number_list_expr.match(line)
            if m:
                values = [int(v) for v in m.group(0).strip().split()]
                actual.append(values)

        # Validate results.
        expected_lines = len(expected)
        actual_lines = len(actual)
        if expected_lines != actual_lines:
            raise ValidationError("Found {0} rows in the output, but expected {1}"
                .format(actual_lines, expected_lines))
        for i, (expected_line, actual_line) in enumerate(zip(expected, actual)):
            if expected_line != actual_line:
                raise ValidationError("Mismatch in row {0}, expected: {1}"
                    .format(i, expected_line))

        return True
