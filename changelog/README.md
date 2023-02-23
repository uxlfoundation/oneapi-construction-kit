# Changelog Entries

This directory holds individual changelog entries that should be collated and
then removed before a release. Changelog entries in this directory **must**
match the following markdown template:

```
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
```

Individual changelog entries **must** be written in markdown and will be
rendered as such, so **may** make use of markdown formatting.

Although it is not used in the output, the name of any changelog entry file
**should** be descriptive of the change. Any empty lines will be stripped
appropriately. Any of Upgrade guidance, Feature additions, Non-funtional
changes and Bug fixes may be omitted. The list under any header must make use
of the `*` character only.


The changelog files in this directory can then be compiled into a single changelog
entry by making use of the `ComputeAorta/scripts/build_changelog.py` script.

The rational behind this process is that the author of an MR has more context
and knowledge of their patch than anyone else, so they can write the most
meaningful changelog entry. This removes the overhead of trying to write
changelog entries from the engineer doing the release and is a step towards a
more automated release process. Not every patch needs a changelog entry, but
the author should consider if one is required when opening an MR.

The steps to generating a compiled changelog entry for all the files in this
directory are:
1. Run:
```console
$ python build_changelog.py --delete -- release_version
```
* `release_version` is the semantic ComputeAorta release version  and **must**
  be of the form `Major.Minor.Patch`.
* By default the script will run on this directory (`ComputeAorta/changelog`).
  The developer may force the script to run on changelog entries in a
  directory of their choice by passing the `--input-dir /path/to/directory`
  option.
* The script will not delete the contents of the changelog directory by
  default, however this can be enabled by passing the `--delete` option (this
  file will not be deleted).
* By default the script will use the date on which it is run as release date in
  the collated changelog entry. This can be manually overridden by the developer
  if they pass the `--release-date YYYY-MM-DD` option.

2. Delete all files in this directory (except this one) if the script was not
   passed the `--delete` option.

3. Open an MR with the removed files and updated `CHANGES.md`.
