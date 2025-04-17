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

from __future__ import unicode_literals

import datetime
import os
import re
import sys
import threading
import traceback

from city_runner.execution import CV_TIMEOUT, TestQueue, Worker, get_cpu_count
from city_runner.profile import Profile
from city_runner.test_info import Pool, TestResults
from city_runner.ui import ColorMode, TestUI

try:
    FileNotFoundError
except NameError:
    FileNotFoundError = IOError


def escape_byte(m):
    """
    Helper function that replaces an invalid character with an hex escape.
    :param m: Match object as returned from a 're' function.
    :return: Replacement bytes.
    """
    byte = m.group(0)[0]
    return ("\\x%02x" % byte).encode("ascii")


class CityRunner(object):
    def __init__(self):
        self.profile = None
        self.args = None
        self.tests = None
        self.num_tests = 0
        self.test_pools = {
            Pool.NORMAL: None,
            Pool.THREAD: None,
            Pool.MEMORY: None
        }
        self.results = None
        self.workers = []
        self.workers_state = []
        self.num_workers = 0
        self.running_workers = 0
        self.live_tests = set()
        self.aborted = False
        self.lock = threading.RLock()
        self.cond = threading.Condition(self.lock)
        self.ui = None
        self.log_file = None
        self.log = None

    def load_profile(self):
        """ Initialize the test runner by loading the profile and parsing
        options. """
        new_argv = self.preprocess_args(sys.argv[1:])
        profile_name, new_argv = Profile.extract_name(new_argv)
        self.profile = Profile.load(profile_name)
        self.profile.runner = self
        self.args = self.profile.parse_options(new_argv)
        self.workers_state = self.profile.init_workers_state(self.args.jobs)
        # Set up the "UI".
        self.ui = TestUI(color_mode=self.args.color)

    def preprocess_args(self, args):
        """
        Pre-process command-line arguments, expanding response files.
        :param args: List of arguments to pre-process.
        :return: List of pre-processed arguments.
        """
        new_args = []
        for arg in args:
            if not arg.startswith("@"):
                new_args.append(arg)
                continue
            response_file = arg[1:]
            if not os.path.exists(response_file):
                raise FileNotFoundError("Response file not found: %s" %
                                        response_file)
            with open(response_file, "r") as f:
                for line in f:
                    new_args.append(line.strip())
        return new_args

    def print_exception(self, exc):
        if self.profile and self.profile.args.verbose:
            traceback.print_exc()
        else:
            sys.stderr.write("error: %s\n" % exc)

    def request_worker_state(self, worker_id):
        """
        Assumes that the worker state info will be provided for either all of
        the workers or none of them.
        """
        with self.lock:
            if not self.workers_state:
                return None
            return self.workers_state[worker_id]

    def execute(self):
        # Use profile to find list of tests
        test_source = self.args.test_source  # CSV
        disabled_source = self.args.disabled_source # Disabled CSV
        ignored_source = self.args.ignored_source # Ignored CSV
        override_source = self.args.override_source # Override CSV
        self.tests = self.profile.load_tests(test_source,
                                             disabled_source,
                                             ignored_source,
                                             override_source)
        if self.args.repeat > 1:
            self.tests *= self.args.repeat
        self.num_tests = len(self.tests)

        # Determine how many workers should be used.
        if self.args.jobs:
            if self.args.jobs < 0:
                raise Exception("Invalid number of workers: %d" %
                                self.args.jobs)
            self.num_workers = self.args.jobs
        else:
            self.num_workers = get_cpu_count()
        # Spread tests between workers, using a queue.
        self.results = TestResults(self.tests)
        self.results.start_time = datetime.datetime.now()
        if self.args.log_file:
            self.log_file = open(self.args.log_file, "w")
            self.log = TestUI(ColorMode.never, self.log_file)
            self.log.print_start_banner(self.results, self.num_workers)
        self.ui.print_start_banner(self.results, self.num_workers)

        runnable_tests = []
        skipped_test_time = datetime.datetime.now()
        for test in self.tests.tests:
            if test.ignore or test.disabled or test.unimplemented:
                run = self.profile.create_run(test)
                if test.ignore:
                    # We ignore tests that skip because the feature isn't
                    # implemented when we have no intention of ever
                    # implementing the feature. The GL sharing test is an
                    # example of this.
                    run.status = "SKIP"
                elif test.disabled:
                    run.status = "FAIL"
                    run.message = "Marked disabled in CSV"
                    # Needed for fail to count in final results summary
                    run.total_tests = 1
                elif test.unimplemented:
                    # Some tests have been left unimplemented by the cts,
                    # or test things that aren't in CL 1.x. We always
                    # consider these tests passed.
                    run.status = "PASS"
                    run.total_tests = 1
                    run.passed_tests = 1
                run.schedule.start_time = skipped_test_time
                run.schedule.end_time = skipped_test_time
                self.results.add_run(run)
                self.ui.print_test_status(run, self.results)
                if self.log:
                    self.log.print_test_status(run, self.results)
            else:
                runnable_tests.append(test)

        def query_by_pool(pool_type):
            """
            Returns subset of tests assigned to the parameterized task pool
            :param pool_type: Enum from Pool class
            """
            return [t for t in runnable_tests if t.pool == pool_type]

        for pool_key in self.test_pools:
            self.test_pools[pool_key] = TestQueue(query_by_pool(pool_key))

        with self.lock:
            self.running_workers = self.num_workers
            self.results.start()
        for i in range(0, self.num_workers):
            worker = Worker(i, self)
            self.workers.append(worker)
            worker.start()
        # Wait for workers to finish.
        self.wait_for_workers()
        self.profile.release_workers_state(self.args.jobs)
        return self.process_results()

    def wait_for_workers(self):
        """ Wait for workers to finish executing tests. This also handles
        the task of aborting tests that exceed their deadline. """
        sleep_interval = CV_TIMEOUT
        with self.lock:
            if self.profile.timeout:
                sleep_interval = 1
            while not self.results.finished and (self.running_workers > 0):
                try:
                    self.cond.wait(sleep_interval)
                except KeyboardInterrupt:
                    self.aborted = True
                    break
                self.enforce_deadlines()
            self.results.stop()

            for pool_key in self.test_pools:
                self.test_pools[pool_key].close()

            while self.running_workers > 0:
                self.cond.wait(CV_TIMEOUT)
            self.results.finish(self.profile)

    def enforce_deadlines(self):
        """ Abort tests that have exceeded their deadline. """
        if not self.profile.timeout:
            return
        current_time = datetime.datetime.now()
        for schedule in self.live_tests:
            if schedule.deadline and (schedule.deadline <= current_time):
                for process in schedule.processes:
                    try:
                        process.terminate()
                    except Exception:
                        pass
                schedule.aborted = True

    def process_results(self):
        """
        Print test results to the console and create result files.
        :return: 1 if there were test failures or 0 for no failure.
        """
        # Print the results.
        self.ui.print_results(self.results)
        if self.log:
            self.log.print_results(self.results)
        # Write the list of failures.
        if self.args.fail_file:
            with open(self.args.fail_file, "w") as f:
                for run in self.results.fail_list:
                    f.write(run.test.name + "\n")
        # Write JUnit results.
        if self.args.junit_result_file:
            with open(self.args.junit_result_file, "w") as f:
                self.results.write_junit(f, self.args.test_prefix)
        # Close the log file.
        if self.log_file:
            self.log_file.close()
            self.log_file = None
        # Return the city runner exit code.
        if self.aborted:
            return 130
        if not self.args.relaxed:
            if self.results.num_fails > 0:
                return 1
            if self.results.num_xpasses > 0:
                return 1
        return 0

    def process_output(self, run):
        """
        Decode the test process' output and process it so that it can be
        printed and logged safely.
        :param run: Test run whose output to process.
        """
        if run.output:
            # Escape non-printable and non-ASCII characters from the test
            # output.
            escaped_output = re.sub(b"[\x00-\x08\x0b\x0c\x0e-\x1f\x80-\xff]",
                                    escape_byte, run.output)
            run.output = escaped_output.decode("ascii")

    def test_started(self, run):
        """
        This function is called when a test has started executing.
        :param run: Test which has started executing.
        """
        with self.lock:
            run.schedule.start_time = datetime.datetime.now()
            if self.profile.timeout and not self.profile.manages_timeout:
                timeout = datetime.timedelta(0, self.profile.timeout)
                run.schedule.deadline = run.schedule.start_time + timeout
            self.live_tests.add(run.schedule)

    def test_created_process(self, schedule, process):
        """
        This function is called when a test has spawned a process.
        :param schedule: Schedule of the test which has spawned the process.
        :param process: Process that was spawned.
        """
        with self.lock:
            schedule.processes.append(process)

    def dequeue_test(self):
        """
        This function looks through the test pools, in order of most
        constrained, and pops a test which is eligible to execute.
        :return: Test for worker thread to run.
        """
        with self.lock:

            def query_live(pool_type):
                """
                Returns list of tests currently running assigned to the
                parameterized threading pool.
                :param pool_type: Enum from Pool class
                """
                return [t for t in self.live_tests if t.pool == pool_type]

            # Only 2 tests assigned to the memory pool can execute at a time
            memory_live = query_live(Pool.MEMORY)
            if len(memory_live) < 2:
                test = self.test_pools[Pool.MEMORY].dequeue()
                if test:
                    return test

            # The number of tests assigned to the thread pool which can run
            # concurrently is `N/2`, where `N` is the total number of worker
            # threads.
            thread_live = query_live(Pool.THREAD)
            thread_pool_limit = (self.num_workers /
                                 2) if self.num_workers > 1 else 1
            if len(thread_live) < thread_pool_limit:
                test = self.test_pools[Pool.THREAD].dequeue()
                if test:
                    return test

            # Any number of tests from the normal thread pool may run in
            # parallel
            test = self.test_pools[Pool.NORMAL].dequeue()
            if test:
                return test

            # Causes Worker thread to terminate
            raise EOFError("No eligible tests")

    def test_finished(self, run):
        """
        This function is called when a test has been executed.
        :param run: Test which has been executed.
        """
        with self.lock:
            run.schedule.end_time = datetime.datetime.now()
            self.live_tests.remove(run.schedule)
            self.results.add_run(run)
            self.ui.print_test_status(run, self.results)
            if self.log:
                self.log.print_test_status(run, self.results)
            self.process_output(run)
            if self.args.verbose or run.status not in ["PASS", "SKIP"]:
                self.ui.print_test_output(run, self.results)
            if self.log:
                self.log.print_test_output(run, self.results)
            self.cond.notify_all()

    def worker_started(self, worker):
        """
        This function is called when a worker has started executing tests.
        :param worker: Worker which started executing tests.
        """
        pass

    def worker_stopped(self, worker, exc=None):
        """
        This function is called when a worker stops executing tests.
        :param worker: Worker which stopped executing tests.
        :param exc: Optional exception that resulted in the stop.
        """
        aborted = False
        if exc:
            self.ui.print_worker_exception(worker, exc)
            aborted = True
        with self.lock:
            self.running_workers -= 1
            if aborted:
                self.aborted = True
            self.cond.notify_all()
