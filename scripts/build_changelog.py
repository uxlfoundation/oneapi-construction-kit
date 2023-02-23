#!/usr/bin/env python3
# Copyright (C) Codeplay Software Limited. All Rights Reserved.
"""
Build the changelog for a new release by compiling changelog entries from
different files.

Changelog entries are a single .md file in the specified directory and must
match the following format:

    # Optional header that may be ignored:

    Upgrade guidance:
    * list
    * of
    * upgrade
    * guidance

    Feature additions:
    * list
    * of
    * feature
    * additions

    Non-functional changes:
    * list
    * of
    * non-functional
    * changes

    Bug fixes:
    * list
    * of
    * bug
    * fixes

Any empty lines will be stripped appropriately. Any of Upgrade guidance, Feature
additions, Non-funtional changes and Bug fixes may be omitted. The list under
any header must make use of the * character only.
"""

from argparse import ArgumentParser
from pathlib import Path
from os import remove
from sys import exit
from datetime import datetime
from textwrap import dedent


def extract_changelog(changelog_entry, changelog_file):
    """
    Extracts a changelog entry from a single changelog file.
    :param: changelog_entry The master changelog containing
            the collated changelog entries. Is updated by
            this function.
    :param: changelog_file Path to the file containing the
            changelog entry to be extracted.
    """
    with open(changelog_file) as f:
        data = [line.strip('\n') for line in f if line != '\n']

    line_index = 0
    entries_added = 0
    while line_index < len(data):
        line = data[line_index]
        if line in changelog_entry:
            first_line = line_index + 1
            last_line = first_line
            while last_line < len(
                    data) and data[last_line] not in changelog_entry:
                last_line += 1
            changelog_entry[line] += "\n".join(
                data[first_line:last_line]) + "\n"
            line_index = last_line
            entries_added += 1
        else:
            line_index += 1

    if entries_added == 0:
        raise RuntimeError(f'{changelog_file} has no changelog entries. This file should be either fixed or deleted.')


def compile_changelog_entries(changelog_directory):
    """
    Compiles separate changelog entry files into a single changelog entry
    and deletes individual changelog entries from the filesystem.
    :param: changelog_directory Directory containing the changelog files.
    :return: The collated changelog entry.
    """
    master_changelog = {
        'Upgrade guidance:': '',
        'Feature additions:': '',
        'Non-functional changes:': '',
        'Bug fixes:': ''
    }
    pathlist = Path(changelog_directory).rglob('*.md')
    pathlist = filter(lambda p: p.is_file() and p.name != 'README.md',
                      pathlist)
    for path in pathlist:
        extract_changelog(master_changelog, path)

    return master_changelog


def main():
    parser = ArgumentParser(description='Compile individual changelog files.')
    parser.add_argument(
        '--input-dir',
        default='./changelog',
        help=
        'Directory containing .md files to compile into single changelog entry. Defaults to ComputeAorta/changelog'
    )
    parser.add_argument(
        '--delete',
        action='store_true',
        help=
        'If enabled deletes individual changeme files in input directory once they are collated.'
    )
    parser.add_argument(
        '--release-date',
        help='The official date of the release: YYYY-MM-DD. Defaults to today.',
        default=datetime.today().strftime('%Y-%m-%d'),
        metavar='YYYY-MM-DD')
    exclusion_group = parser.add_mutually_exclusive_group(required=True)
    exclusion_group.add_argument(
        'release_version',
        help='The version of the release: Major.Minor.Patch',
        metavar='MAJOR.MINOR.PATCH',
        nargs='?')
    exclusion_group.add_argument(
        '-d',
        '--dry-run',
        action='store_true',
        help='Perform a dry run of building the changelog.')

    args = parser.parse_args()

    changes = compile_changelog_entries(args.input_dir)

    change_lines = ''.join(f'{header}\n\n{changes}\n'
                           for header, changes in changes.items() if changes)
    changelog_entry = f'''## Version {args.release_version} - {args.release_date}

{change_lines}'''

    if args.delete:
        pathlist = Path(args.input_dir).rglob('*.md')
        pathlist = filter(lambda p: p.is_file() and p.name != 'README.md',
                          pathlist)
        for changeme_path in pathlist:
            remove(changeme_path)

    with open(Path('./CHANGELOG.md'), 'r') as changelog_file:
        changelog_file_as_lines = changelog_file.readlines()
    if not args.dry_run:
        with open(Path('./CHANGELOG.md'), 'w') as changelog_file:
            changelog_file.writelines(changelog_file_as_lines[0:1])
            changelog_file.write('\n')
            changelog_file.write(changelog_entry)
            changelog_file.writelines(changelog_file_as_lines[2:])


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        exit(130)
