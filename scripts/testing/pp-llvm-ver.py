#!/usr/bin/env python3

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

"""Preprocess FileCheck check prefixes for a given LLVM version.

This script is intended to be used to preprocess a test file used to check tool
output across multiple LLVM versions so that it is ready to be run on a single
particular LLVM version.

It expects check prefixes in a structured form, using the same prefix form that
FileCheck expects and accepts. This script singles out a particular component
of prefixes using a vocabulary of LT,LE,EQ,GE,GT and a major version number.
These are expected to be at least the second hyphenated component in a check
prefix.

   CHECK-LT13, CHECK-LT13-NOT, CHECK-EQ9, FOO-GT14, FOO-BAR-LT9-LABEL,
   CHECK-LT13GE9, FOO-EQ8-GE12-NOT, etc.

Individual either/or check components can be separated by hyphens, and each
component supplies a series of 'all' checks:

    GE9LT13        - greater-than-equal-to 9 AND less-than 13
                     ==> (9,10,11,12)
    EQ6-GE9LT13    - equal-to 6 OR (greater-than-equal-to 9 AND less-than 13)
                     ==> (6,9,10,11,12)
    GT4LT8-GE9LT13 - (greater-than 4 AND less-than 8)
                     OR (greater-than-equal-to 9 AND less-than 13)
                     ==> (5,6,7,9,10,11,12)

These prefix patterns are not predetermined and can be mixed and matched within
the same test file.

These checks should be written such that they apply only to LLVM major versions
less-than/less-than-or-equal/equal-to/greater-than-or-qual-to/greater-than the
specified version.

The preprocessing script then looks for check prefixes of the above form, and
if the current LLVM version (configured in this script, or passed to it as an
override) is compatible with the check prefix, it scrubs out the version check,
e.g., for LLVM 13:

    CHECK-GT12 -> CHECK
    FOO-GE13-NOT -> FOO-NOT

If the current LLVM version is incompatible with a check prefix, it is
scrubbed out with a comment explaining so. For example, for LLVM 13,
CHECK-LT12, CHECK-GT13, FOO-EQ9, would all be scrubbed out and replaced with a
comment.

This system allows checks to be easily mixed, allowing multiple checks to catch
common cases with as few repetitive checks as possible. For example, given this
series of checks:

    CHECK-GE12: foo
    CHECK-EQ12: bar
    CHECK-GT12: baz
    CHECK-GE12: foo

On LLVM 12 the script would output:
    CHECK: foo
    CHECK: bar
    CHECK: foo

And on LLVM 13 this script would output:
    CHECK: foo
    CHECK: baz
    CHECK: foo
"""

import re
import sys
import argparse


def and_components(comps):
    """ Splits up a string consisting of comparisons, e.g., GE9LT11GE12LT14,
    and yields all individual comparisons as tuples: ('GE', 9), ('LT', 11),
    etc. """
    for x, y in re.findall(r'(LT|LE|EQ|GE|GT)([0-9]+)', comps):
        yield (x, int(y))


def matches_ver_check(comp, ver, the_llvm_ver):
    """ Returns whether a comparison and version number matches the LLVM
    version being processed """
    return ((ver == the_llvm_ver and comp in ('LE', 'EQ', 'GE')) or
            (ver < the_llvm_ver and comp in ('GT', 'GE')) or
            (ver > the_llvm_ver and comp in ('LT', 'LE')))


def main():
    parser = argparse.ArgumentParser(
        description="Preprocess FileCheck CHECKs for the current LLVM version")
    parser.add_argument('--llvm-ver', type=int)
    parser.add_argument(
        'input',
        nargs='?',
        type=argparse.FileType('r'),
        default=sys.stdin
    )
    # Note: NOT of type FileType because we don't want to open it until we've
    # read the input: reading and writing to the same file is a valid use of
    # this script.
    parser.add_argument('-o', '--output', default='-')

    args = parser.parse_args()

    lines = args.input.readlines()

    args.input.close()

    # Due to greedy matching of multiple predicates split by hyphens it's easier
    # to split our line matching and splitting into two distinct regexes
    check_line_pat = re.compile(
        r'^(?P<comment>\s*\w+\s)?'
        r'(?P<check_prefix>[^:]*)'
        r'(?P<check_suffix>:.*)$'
    )
    preds_pat = re.compile(r'((LT|LE|EQ|GE|GT)[0-9]+)+')
    for i, line in enumerate(map(str.lstrip, lines)):
        # Valid FileCheck check prefixes must start with a letter and contain
        # only alphanumeric characters, hyphens and underscores. They must then
        # be followed by a colon.
        # The prefixes we're interested in differentiate between LLVM versions
        # using the last hyphenated component(s), containing one or more sequences
        # of LT/LE/EQ/GE/GT and an LLVM version number. Individual *either/or*
        # check components are separated by hyphens, and within each component,
        # *all* checks must match, so GE9LT12 matches versions 9,10,11 but
        # EQ8EQ9 matches nothing (contradiction). EQ8-GE12 matches 8,12,13+,
        # GE8LE10-GE12LE14 matches 8,9,10,12,13,14.
        # We also match these prefixes when followed by special alphabetical
        # sequences that FileCheck realises: LABEL,NOT,DAG, etc.

        # Try to greedily match many such components split by hyphens. Due to
        # greedy matching this fails to match a comparison check without a
        # hyphen.

        # Grab everything up until the *first* colon as our check prefix.
        line_parts = check_line_pat.match(line)
        if not line_parts:
            continue

        line_parts_g = line_parts.groupdict()
        comment = line_parts_g['comment'] or ''
        check_prefix = line_parts_g['check_prefix']
        check_suffix = line_parts_g['check_suffix']

        # Split the prefix into hyphenated components. Some of these may
        # contain version checks.
        components = check_prefix.split('-')

        idx = 0
        while idx < len(components) and not preds_pat.match(components[idx]):
            idx += 1

        # We don't support version checks as the first component
        if idx == 0 or idx == len(components):
            continue

        first_idx = idx

        # Now greedily match all version checks. Version checks must be
        # contiguous: everything after the last matching version check is kept
        # as-is.
        version_checks = []
        while idx < len(components) and preds_pat.match(components[idx]):
            version_checks.append(components[idx])
            idx += 1

        # A compatible check has its comparisons taken out
        if any((all(matches_ver_check(comp, ver, args.llvm_ver)
                    for comp, ver in and_components(ver_checks))
                for ver_checks in version_checks)):
            line = (
                comment +
                '-'.join(components[:first_idx] + components[idx:])
                + check_suffix
            )
        else:
            # An incompatible check is scrubbed out
            line = ''
        lines[i] = f'{line}\n'

    if args.output == '-':
        for line in lines:
            sys.stdout.write(line)
    else:
        with open(args.output, 'w', encoding='utf-8') as file:
            file.writelines(lines)


if __name__ == '__main__':
    main()
