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

import threading
import subprocess
import multiprocessing
import traceback
import signal
import sys
import os

from test_info import TestRun

# This is needed to avoid blocking SIGINT on Python 2.x.
CV_TIMEOUT = 10.0


def get_cpu_count(default=4):
    """
    Try to detect the number of CPUs available on the system.
    """
    try:
        return multiprocessing.cpu_count()
    except NotImplementedError:
        try:
            return os.sysconf('SC_NPROCESSORS_ONLN')
        except Exception:
            try:
                return int(os.environ["NUMBER_OF_PROCESSORS"])
            except KeyError:
                pass

    # We cannot detect the number of cores, use the default value.
    return default


class TestQueue(object):
    """
    FIFO queue of tests for use with multiple workers.
    """
    def __init__(self, tests=[]):
        """
        Create a new blocking FIFO test queue.
        :param tests: List of tests to add to the queue.
        :return: None
        """
        self.tests = tests[:]
        self.closed = False
        self.lock = threading.RLock()
        self.cond = threading.Condition(self.lock)

    def close(self):
        """
        Close the queue, causing other operations to raise EOFError().
        """
        with self.lock:
            self.closed = True
            self.cond.notify_all()

    @property
    def is_empty(self):
        """
        Determine whether the queue is empty or not.
        """
        with self.lock:
            return not self.tests or (len(self.tests) == 0)

    def enqueue(self, test):
        """
        Add a test at the bottom of the queue.
        """
        with self.lock:
            if self.closed:
                raise EOFError("The queue was closed")
            self.tests.append(test)
            self.cond.notify_all()

    def dequeue(self):
        """
        Remove a test from the top of the queue.
        """
        with self.lock:
            if self.closed:
                raise EOFError("The queue was closed")

            if self.is_empty:
                return None

            test = self.tests.pop(0)
            self.cond.notify_all()
            return test


class Worker(object):
    """
    Task that executes tests, one at a time, on a separate thread.
    """
    def __init__(self, id, runner):
        self.id = id
        self.runner = runner
        self.thread = threading.Thread(target=self.entry)

    def start(self):
        """
        Start the worker thread.
        """
        self.thread.start()

    def entry(self):
        """
        Entry point for the worker thread.
        """
        profile = self.runner.profile
        try:
            state = self.runner.request_worker_state(self.id)
            self.runner.worker_started(self)
            while True:
                try:
                    test = self.runner.dequeue_test()
                except EOFError:
                    # The queue was closed, stop executing tests.
                    break
                run = profile.create_run(test, state)
                try:
                    self.runner.test_started(run)
                    run.execute()
                except Exception as e:
                    run.message = str(e)
                    run.status = "FAIL"
                    run.add_output(traceback.format_exc())
                    self.runner.test_finished(run)
                else:
                    self.runner.test_finished(run)
        except Exception as e:
            traceback.print_exc()
            self.runner.worker_stopped(self, e)
        else:
            self.runner.worker_stopped(self)


class TestSchedule(object):
    """
    Contains information about a test's schedule, such as its deadline
    and any running process. This state is accessed across multiple threads.
    This is why it is separated from TestRun.
    """
    def __init__(self):
        self.start_time = None
        self.end_time = None
        self.deadline = None
        self.aborted = False
        self.processes = []


class TestRunBase(TestRun):
    """
    Represent the execution of a particular test.
    """
    def __init__(self, profile, test):
        super(TestRunBase, self).__init__(test)
        self.profile = profile
        self.schedule = TestSchedule()

    def execute(self):
        """
        Execute the test and wait for completion.
        """
        raise NotImplementedError()

    def execute_and_output(self):
        """
        Execute the test, wait for completion and return the test output.
        """
        self.execute()
        return self.output

    def create_process(self, cmd, args=None, working_dir=None, env=None):
        """
        Create a new process and wait for it to finish. The output
        from stdout/stderr as well as return code are also captured.
        """
        if args is None:
            args = []
        args.insert(0, cmd)
        real_env = env
        if env:
            real_env = os.environ.copy()
            real_env.update(env)
        # Print commands that are executed in the test output.
        if self.profile.args.verbose:
            args_text = []
            if env:
                for name, val in env.items():
                    args_text.append("%s='%s'" % (name, val))
            for arg in args:
                if arg.find(" ") >= 0:
                    args_text.append('"%s"' % arg)
                else:
                    args_text.append(arg)
            self.add_output(" ".join(args_text) + "\n\n")
        self.process = subprocess.Popen(args,
                                        cwd=working_dir,
                                        env=real_env,
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.STDOUT)
        self.profile.runner.test_created_process(self.schedule, self.process)
        stdout, _ = self.process.communicate()
        self.add_output(stdout)
        self.return_code = self.process.returncode

    def add_output(self, text):
        """
        Add some text to the test output.
        """
        if self.output is None:
            self.output = bytes()
        if hasattr(text, "encode"):
            self.output += text.encode("utf8")
        else:
            self.output += text

    def analyze_process_output(self):
        """
        Determine whether the test passed or failed from the process
        output and return code.
        """
        if self.return_code != 0:
            if self.schedule.aborted:
                self.status = "TIMEOUT"
            else:
                self.status = "FAIL"
            self.message = self.format_return_code(self.return_code)
        else:
            self.status = "PASS"

    def format_return_code(self, return_code):
        """
        Create a message based on the process' return code.
        Some return codes are significant (e.g. signal codes on Linux).
        """
        message = "exited with code %d" % return_code
        if sys.platform.startswith("linux") and (return_code < 0):
            for name in dir(signal):
                if not name.startswith("SIG") or (name.find("_") >= 0):
                    continue
                value = getattr(signal, name)
                if -value == return_code:
                    message = name
        return message
