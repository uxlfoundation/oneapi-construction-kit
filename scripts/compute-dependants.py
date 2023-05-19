#!/usr/bin/env python3

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

"""
Given a build directory and a set of C/C++ source/header files, compute the
complete set of files depending on the inputs according to the compilation
config given by a compile_commands.json compilation database
"""

import os
import sys
import json
import shlex
import subprocess
import collections
import re
import multiprocessing

KEY_RE = re.compile(r'^([^\s][^:]+):\s*\\?$')
VAL_RE = re.compile(r'^\s+([^\\]+)\s*\\?$')
KEY_VAL_RE = re.compile(r'^([^\s][^:]+):\s+([^\s\\]+)\s*(\\$)?')


def parse_makedeps(text):
    """
    parse a Makefile dependency list
    """
    lines = text.splitlines()
    depfile = collections.defaultdict(list)
    for line in lines:
        kv = KEY_VAL_RE.match(line)
        if kv:
            key, v = kv.groups()[0:2]
            for v in v.split():
                depfile[key].append(v)
            continue

        k = KEY_RE.match(line)
        if k:
            key = k.groups()[0]
            continue

        if not line.strip():
            continue
        dep = VAL_RE.match(line)
        assert dep
        for v in dep.groups()[0].split():
            depfile[key].append(v)

    return depfile


def _get_deps(src_dir_cmd):
    """
    map helper for ``get_project_deps``
    """
    src, dir, cmd = src_dir_cmd
    cmd += ['-fdiagnostics-color=never']
    # ComputeAorta does local #includes with either <> or "" which is wrong, but
    # enough to confound makedeps so we must not include '-MM' below
    cmd += ['-M', '-MG', '-MT', src]
    output = subprocess.check_output(cmd, cwd=dir, universal_newlines=True)
    if not output:
        return {}
    deps = parse_makedeps(output)
    assert len(deps) >= 1
    try:
        vals = list(deps.values())[0]
    except IndexError:
        print(
            f"""
            {repr(output)} failed to parse makedepends for
            {' '.join(map(shlex.quote, cmd))}
            in {dir}""",
            file=sys.stderr,
        )
        return {}

    vals = set(os.path.abspath(os.path.join(dir, dep)) for dep in vals)
    return {src: vals}


def _cc_cmds(compile_commands):
    with open(compile_commands) as f:
        cmds = json.load(f)
    for c in cmds:
        yield c['file'], c['directory'], c['command']


def get_project_deps(build_dir, exclude_filter=None):
    """
    Use the compile_commands.json in ``build_dir`` to compute a mapping from
    source file to a list of header dependencies for that source file. Expects a
    gcc-like compiler driver that supports -MM -MT -MG and prints such a result
    on stdout
    """
    compile_commands = os.path.join(build_dir, 'compile_commands.json')
    cmdlist = []
    for src, dir, cmd in _cc_cmds(compile_commands):
        if exclude_filter and exclude_filter(src):
            continue
        cmd = shlex.split(cmd)
        try:
            o = cmd.index('-o')
            cmd.pop(o)
            cmd.pop(o)
        except ValueError:
            pass
        try:
            o = cmd.index('-c')
            cmd.pop(o)
        except ValueError:
            pass
        try:
            o = cmd.index('-MD')
            cmd.pop(o)
            cmd.pop(o)
        except ValueError:
            pass
        cmdlist.append((src, dir, cmd))

    cpu_count = multiprocessing.cpu_count()
    cpu_count += (cpu_count // 10) + 2
    try:
        pool = multiprocessing.Pool(cpu_count)
        results = pool.map(_get_deps, cmdlist)
    except KeyboardInterrupt:
        sys.exit(1)
    pool.terminate()

    return {k: v for result in results for k, v in result.items()}


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument(
        '--build-dir',
        required=True,
        type=os.path.abspath,
        help="a build directory contains a compile_commands.json",
    )
    parser.add_argument(
        '--exclude-filter',
        type=lambda x: re.compile(x).search,
        required=False,
        help="Python regex pattern for source files to exclude from the search",
    )
    parser.add_argument(
        'files',
        nargs=argparse.REMAINDER,
        type=os.path.abspath,
        help="list of sources of which to compute dependants",
    )
    args = parser.parse_args()

    all_deps = get_project_deps(args.build_dir, args.exclude_filter)
    all_deps_srcs = set(all_deps.keys())
    all_changes = set()
    for changed in args.files:
        for src, deps in all_deps.items():
            if changed in deps:
                all_changes.add(src)

    for change in all_changes:
        if change in all_deps_srcs:
            print(change)
