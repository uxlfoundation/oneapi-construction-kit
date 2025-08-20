#!/usr/bin/env python
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
"""Update Mux API version in relevant files based on mux.xml"""

from argparse import ArgumentParser
from platform import system
import re
import sys
import xml.etree.ElementTree as XML

# Due to git attribute being set up to use Windows line endings for Markdown
# documents we need to ensure we output the correct thing on all platforms.
EOL = '\r\n' if system() == 'Windows' else '\n'
VERSION_REGEX = re.compile(r'\d+\.\d+\.\d+')
SPEC_PATTERN = '   This is version {} of the specification.\n'


def get_mux_version_string(schema):
    """Get the Mux version string from the mux.xml schema."""
    root = XML.parse(schema).getroot()
    defines = root.find('guard').findall('block')[1].findall('define')[:3]
    return '.'.join(define.find('value').text for define in defines)


def version_tuple(version):
    """Get a version tuple of ints from a version string."""
    return tuple(map(int, version.split('.')))


def abort_if_version_ahead(filename, mux_version, file_version):
    """Check if the file's version is ahead and abort if it is."""
    if version_tuple(mux_version) < version_tuple(file_version):
        print(
            f'{filename} version {file_version} is ahead of {mux_version} in mux.xml',
            file=sys.stderr)
        sys.exit(1)


def update_spec_version(spec_path, version_string):
    """Update the version in spec/*-spec.rst."""
    with open(spec_path, 'r') as spec:
        lines = spec.readlines()
    for index, line in enumerate(lines):
        match = VERSION_REGEX.search(line)
        if match and version_string != match.group():
            abort_if_version_ahead(spec_path, version_string, match.group())
            lines[index] = SPEC_PATTERN.format(version_string)
            break
    with open(spec_path, 'w') as spec:
        spec.write(''.join(lines))


def update_changes_version(changes_path, version_string):
    """Update the version in changes.rst or add a new section."""
    underline = '-' * len(version_string)
    with open(changes_path, 'r') as changes:
        lines = changes.readlines()
    for index, line in enumerate(lines):
        match = VERSION_REGEX.search(line)
        if match and not line.startswith('   Versions prior to'):
            if version_string != match.group():
                abort_if_version_ahead(changes_path, version_string,
                                       match.group())
                insert = [version_string, EOL, underline, EOL, EOL,
                          '* TODO' + EOL, EOL]
                lines = lines[:index] + insert + lines[index:]
            break
    with open(changes_path, 'w') as changes:
        changes.write(''.join(lines))


def main():
    """Main entry point for the script."""
    parser = ArgumentParser(
        description='Update Mux version in relevant files based on mux.xml')
    parser.add_argument('schema', help='path to mux.xml')
    parser.add_argument('runtime_spec', help='path to runtime-spec.rst')
    parser.add_argument('compiler_spec', help='path to compiler-spec.rst')
    parser.add_argument('changes', help='path to CHANGES.md')
    args = parser.parse_args()
    version_string = get_mux_version_string(args.schema)
    update_spec_version(args.runtime_spec, version_string)
    update_spec_version(args.compiler_spec, version_string)
    update_changes_version(args.changes, version_string)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
