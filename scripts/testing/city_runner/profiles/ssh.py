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
import warnings
import collections
import os
import re
from time import sleep
from platform import system
from posixpath import join as posixpath_join
from city_runner.profile import Profile
from city_runner.cts import CTSTestRun
from city_runner.execution import SSH_AGGRESSIVE_TIMEOUT, DEVICE_REBOOT_TIME
import city_runner.extensions


def create_profile():
    """ Create a new instance of a profile. This is usually called once."""
    return SSHProfile()


def find_extension_function(func_name):
    """ Queries extension modules for function with a given name.

    Customer specific functionality must be manually placed in the extensions
    folder. This is to protect IP, and hide implementation details. However
    it means that the work of importing must be done dynamically.
    """
    extension_func = None
    for script in city_runner.extensions.__all__:
        customer_module_name = "city_runner.extensions." + script

        # Import customer module
        base_module = __import__(customer_module_name, globals(), locals())
        ext_module = getattr(base_module, "extensions")
        customer_module = getattr(ext_module, script)

        # See if module has attribute with function name
        try:
            func_attribute = getattr(customer_module, func_name)
        except AttributeError:
            continue

        # Assert this attribute is a function
        if callable(func_attribute):
            extension_func = func_attribute
            break

    return extension_func


class Device():
    """
    Encapsulates information about a remote device and the PDU unit associated
    with it.
    """

    def __init__(self,
                 device_hostname=None,
                 device_username=None,
                 device_port=None,
                 pdu_hostname=None,
                 pdu_port=None):
        self.device_hostname = device_hostname
        self.device_username = device_username
        self.device_port = device_port
        self.pdu_hostname = pdu_hostname
        self.pdu_port = pdu_port


class SSHProfile(Profile):

    def __init__(self):
        super(SSHProfile, self).__init__()

        self.devices = []
        self.pdus = []

    Device = collections.namedtuple("Device", "username host port")
    PDU = collections.namedtuple("PDU", "device_host pdu_host pdu_port")

    """ This profile executes tests remotely using SSH. """

    def create_run(self, test, worker_state=None):
        """ Create a new test run from a test description.  """
        return SSHTestRun(self, test, worker_state)

    def init_workers_state(self, expected_num_entries):
        """
        When using multiple devices (and PDUs) the credentials need to be
        passed to workers (to make them thread safe) as a predefined state.
        Initialize it here.
        """
        res = []
        for device in self.devices:
            pdu = [
                item for item in self.pdus if item.device_host == device.host
            ]
            if len(pdu) > 1:
                warnings.warn("More than one PDU found, using the first one.")
            elif not pdu:
                warnings.warn("No PDU found, HW reboot will only be triggered"
                              " with environment variable.")
                res.append(
                    Device(
                        device_hostname=device.host,
                        device_username=device.username,
                        device_port=device.port))
            else:
                # only 1 PDU found
                res.append(
                    Device(
                        device_hostname=device.host,
                        device_username=device.username,
                        device_port=device.port,
                        pdu_hostname=pdu[0].pdu_host,
                        pdu_port=pdu[0].pdu_port))

        if res and (len(res) != expected_num_entries):
            raise Exception("Number of worker state entries not equal to the"
                            " number of jobs.")
        return res

    def find_ssh_client(self):
        """ Locate a SSH client executable on the host machine. """
        if system() == "Windows":
            # plink comes with putty, the most common windows ssh client
            clients = ["ssh.exe", "plink.exe"]
        else:
            clients = ["ssh"]

        # Try to find a client in the PATH
        for client in clients:
            for path in os.environ["PATH"].split(os.pathsep):
                exe_file = os.path.join(path, client)
                if os.path.isfile(exe_file) and os.access(exe_file, os.X_OK):
                    return exe_file

        return None

    def add_options(self, parser):
        """ Add command-line options that can be used with the profile to the
         argument parser. """
        super(SSHProfile, self).add_options(parser)
        parser.add_argument(
            "--ssh-client",
            type=str,
            default=self.find_ssh_client(),
            help="Path to the SSH client to use to run tests remotely.")
        parser.add_argument(
            "--ssh-host",
            type=str,
            default="localhost",
            help="Remote host name to connect to.")
        parser.add_argument(
            "--ssh-devices",
            type=argparse.FileType('r'),
            help="File containing details of remote hosts to connect.\n"
            "Expected format (per line): \"username hostname:port\"")
        parser.add_argument(
            "--ssh-pdu-map",
            type=argparse.FileType('r'),
            help="File containing a map of device hostname to pdu:port.\n"
            "Expected format (per line): \"device_hostname pdu_hostname:pdu_port\"")
        parser.add_argument(
            "--ssh-port",
            type=int,
            default=22,
            help="Remote port to connect to.")
        parser.add_argument(
            "--ssh-user",
            type=str,
            help="Remote user to select for remote login.")
        parser.add_argument(
            "--ssh-key", type=str, help="Path to SSH private key.")
        parser.add_argument(
            "--ssh-reboot-on-fail",
            action='store_true',
            help="Reboot device when a test fails.")
        parser.add_argument(
            "--ssh-reboot-on-timeout",
            action='store_true',
            help="Reboot device on a test timeout.")

    def parse_options(self, argv):
        """ Parse and validate the given command-line arguments. """
        args = super(SSHProfile, self).parse_options(argv)
        if not args.ssh_user and not args.ssh_devices:
            raise Exception("The SSH login (--ssh-user) or a device list"
                            " (--ssh-devices) must be specified.")

        if not args.ssh_client and not args.ssh_devices:
            raise Exception("A SSH client (--ssh-client) or a device list"
                            " (--ssh-devices) must be specified.")

        if args.ssh_devices:
            device_pattern = re.compile(
                r'(^[\w-]+) ((\w|[$-_@.&+]|[!*\(\),]|(?:%[\da-fA-F]{2}))+):'
                r'([1-9]\d*)$')
            with args.ssh_devices as ssh_devices_file:
                for device in ssh_devices_file:
                    match = device_pattern.match(device)
                    if not match:
                        raise Exception("Incorrect SSH entry found in the"
                                        " file. Expected: \'username host:"
                                        "port\'")
                    user = match.group(1)
                    host = match.group(2)
                    port = match.group(4)

                    self.devices.append(
                        self.Device(username=user, host=host, port=port))

            if args.jobs:
                warnings.warn("Number of jobs will be set to the number of "
                              "available devices, \'--jobs\'/\'-j\' ignored.")
            args.jobs = len(self.devices)
            if (args.ssh_client or args.ssh_user or args.ssh_port):
                warnings.warn("SSH information specified in devices file has "
                              "priority over command line options (--ssh-user,"
                              " --ssh-device, --ssh-port).")

        if args.ssh_pdu_map:
            pdu_pattern = re.compile(
                r'(^(\w|[$-_@.&+]|[!*\(\),]|(?:%[\da-fA-F]{2}))+) '
                r'((\w|[$-_@.&+]|[!*\(\),]|(?:%[\da-fA-F]{2}))+):([1-9]\d*)$')
            with args.ssh_pdu_map as pdu_map_file:
                for pdu_entry in pdu_map_file:
                    match = pdu_pattern.match(pdu_entry)
                    if not match:
                        raise Exception(
                            "Incorrect pdu_device entry.\nExpected:"
                            " \'device_hostname pdu_hostname:pdu_port'")
                    device_host = match.group(1)
                    pdu_host = match.group(3)
                    pdu_port = match.group(5)
                    self.pdus.append(
                        self.PDU(
                            device_host=device_host,
                            pdu_host=pdu_host,
                            pdu_port=pdu_port))

        # Default number of threads is 1.
        if not args.jobs:
            args.jobs = 1

        return args

    def construct_ssh_command(self,
                              remote_cmd,
                              include_client=False,
                              worker_state=None,
                              quiet=False):
        """ Build an SSH command for running 'remote_cmd' on board. """
        if worker_state:
            ssh_host = worker_state.device_hostname
            ssh_user = worker_state.device_username
            ssh_port = int(worker_state.device_port)
        else:
            ssh_host = self.args.ssh_host
            ssh_user = self.args.ssh_user
            ssh_port = self.args.ssh_port

        if "plink" in self.args.ssh_client:
            ssh_args = ["-P", "%d" % ssh_port]
        else:
            ssh_args = ["-p %d" % ssh_port]

        if quiet:
            ssh_args.append('-q')

        # Private key(identity file) is set using the -i flag on both plink and
        # OpenSSH ssh clients
        if self.args.ssh_key:
            ssh_args.extend(["-i", self.args.ssh_key])

        ssh_args.append("@".join((ssh_user, ssh_host)))
        ssh_args.append(remote_cmd)

        if include_client:
            ssh_args.insert(0, self.args.ssh_client)
        return ssh_args

    def construct_cmd_env(self):
        """ Builds a shell command for exporting env vars. """
        exports = []

        for key, value in self.args.env_vars:
            exports.append("export %s='%s'" % (key, value))

        if self.args.lib_paths:
            exports.append(
                "export LD_LIBRARY_PATH='%s'" % ':'.join(self.args.lib_paths))

        export_cmd = " && ".join(exports)
        return export_cmd

    def construct_timeout_cmd(self):
        """ Returns a string for the timeout command if enabled. """
        if self.timeout:
            # Don't just rely on killing the ssh process for timeouts, also use a
            # timeout on the board itself so that the command doesn't manage to
            # keep on running (this won't happen indefinitely, but given network
            # delays etc a command may keep running for a while).  Allow the ssh
            # command to timeout slightly before the ssh command though, to
            # reduce the chance of timing issues (e.g. if ssh timeouts before the
            # client a second ssh command could start before the first has
            # finished).
            timeout = max(1, self.timeout - SSH_AGGRESSIVE_TIMEOUT)
            return "timeout %ds" % timeout
        else:
            return ""


class SSHTestRun(CTSTestRun):
    """ Represent the execution of a particular test. """

    def __init__(self, profile, test, worker_state=None, quiet=False):
        super(SSHTestRun, self).__init__(profile, test)
        self.worker_state = worker_state
        self.quiet = quiet

    def rebootOnFail(self):
        args = self.profile.args
        if (args.ssh_reboot_on_fail and
            (self.status == "FAIL")) or (args.ssh_reboot_on_timeout and
                                         (self.status == "TIMEOUT")):
            # Try custom reboot, otherwise fallback to /sbin/reboot
            embedded_reboot = find_extension_function("embedded_reboot")
            # Check if there is an sbin/reboot behaviour specified, otherwise revert to
            # default behaviour
            sbin_reboot = find_extension_function("sbin_reboot")
            if embedded_reboot:
                # Pass test output as parameter since reboot is only performed
                # under certain conditions
                embedded_reboot(
                    args.ssh_host, self.output, worker_state=self.worker_state)
            elif sbin_reboot:
                # Pass test output as parameter since reboot is only performed
                # under certain conditions
                sbin_reboot(args.ssh_host, args.ssh_user, self.output)
            else:
                ssh_reboot_args = self.profile.construct_ssh_command(
                    "/sbin/reboot")
                self.create_process(args.ssh_client, ssh_reboot_args)
                # Sleep while device reboots.
                sleep(DEVICE_REBOOT_TIME)

    def execute(self):
        """ Execute the test and wait for completion. """
        exe = self.test.executable
        args = self.profile.args

        # Don't use os.path.join since we could have a windows host
        exe_path = posixpath_join(args.binary_path, exe.path)

        # Create command for running binary with test arguments
        exe_cmd = " ".join([exe_path] + self.test.arguments)

        # Add timeout command
        timeout_cmd = self.profile.construct_timeout_cmd()

        # Build export commands for environment vars
        export_cmd = self.profile.construct_cmd_env()

        # cd into binary directory
        cd_cmd = "cd %s" % args.binary_path

        # Combine individual command components
        if export_cmd:
            combined_cmd = [cd_cmd, export_cmd, timeout_cmd]
        else:
            combined_cmd = [cd_cmd, timeout_cmd]

        remote_cmd = " && ".join(combined_cmd)
        remote_cmd += " %s" % exe_cmd

        # Wrap in SSH command.
        ssh_cmd = self.profile.construct_ssh_command(
            remote_cmd, worker_state=self.worker_state, quiet=self.quiet)

        # Execute the 'outer' SSH command.
        self.create_process(args.ssh_client, ssh_cmd)
        self.analyze_process_output()

        self.rebootOnFail()
