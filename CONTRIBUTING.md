# Contributing to the oneAPI Construction Kit

Thank you for your interest in contributing to the development of the oneAPI
Construction Kit! The following is a set of guidelines on how to contribute to
the project, whether you are opening a bug report, suggesting an enhancement,
or submitting your own code contribution to the project.

These are intended to be guidelines rather than rules, so please use your best
judgement when contributing, and feel free to propose changes to this document
where appropriate.

# Table of contents

* [Code of conduct](#code-of-conduct)
* [I have a question](#i-have-a-question)
* [I want to contribute](#i-want-to-contribute)
    * [Legal notice](#legal-notice)
    * [Reporting a bug](#reporting-a-bug)
        * [Before submitting a bug report](#before-submitting-a-bug-report)
        * [Responsible disclosure](#responsible-disclosure)
        * [Submitting a bug report](#submitting-a-bug-report)
    * [Suggesting enhancements](#suggesting-enhancements)
        * [Before suggesting an enhancement](#before-suggesting-an-enhancement)
        * [Submitting an enhancement suggestion](#submitting-an-enhancement-suggestion)
    * [Code contributions](#code-contributions)
        * [Before opening a pull request](#before-opening-a-pull-request)
        * [Commit messages](#commit-messages)
        * [Submitting a pull request](#submitting-a-pull-request)
* [Getting in touch](#getting-in-touch)

# Code of conduct

This project and everyone participating in it are governed by the [oneAPI
Construction Kit Code of Conduct](blob/main/CODE_OF_CONDUCT.md). By
participating, you are expected to uphold this code and use your best judgement
where it's unclear if your behaviour would violate the code. Please report
unacceptable behaviour to
[oneapi.construction.kit@codeplay.com](mailto:oneapi.construction.kit@codeplay.com).

# I have a question

Before asking a question, it is best to search through the available resources
in case your question has already been answered.

* We assume that you have read through the relevant
  [Documentation](https://developer.codeplay.com/products/oneapi/construction-kit/guides).
* There may be existing
  [issues](https://github.com/codeplaysoftware/oneapi-construction-kit/issues)
  that might help you.
* An answer may have been provided elsewhere on the internet.

In case you have found a suitable
[issue](https://github.com/codeplaysoftware/oneapi-construction-kit/issues) and
still need clarification, you can feel free to write your question in the
existing issue.

If your question has not been answered or you still feel that you need
clarification, we ask that you follow these steps.

* Open an
  [issue](https://github.com/codeplaysoftware/oneapi-construction-kit/issues/new/choose).
* Select the **Question** issue template and fill in the requested details.
* Provide as much context as you can about the problem or question you have.
* Provide project version and any relevant system details (e.g. operating
  system).

Once your issue has been opened, we will take a look and try to help you as
soon as possible.

# I want to contribute

## Legal notice

When contributing to this project, you acknowledge that you have the necessary
rights to the content you contribute, and that the content may be provided
under the [project license](LICENSE.txt).

## Reporting a bug

This section guides you through submitting a bug report. Following these
guidelines will help maintainers and the community in reproducing and fixing
reported bugs.

### Before submitting a bug report

A good bug report shouldn't leave others needing to chase you up for more
information. Therefore, when reporting a bug, we ask that you investigate
carefully, collect information, and describe the issue in detail in your
report. Please complete the following steps in advance in order to help us deal
with potential bugs as quickly and as smoothly as possible.

* Make sure that you are using the latest release of the software, available
  [here](https://github.com/codeplaysoftware/oneapi-construction-kit/releases).
* If possible, check to see if the bug is still present on the
  [main branch](https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main).
* Attempt to determine if your bug is really a bug and not an error on your end
  (e.g. incompatible environment, unsupported operating system, etc).
* Check to see if a bug report has already been opened in
  [Issues](https://github.com/codeplaysoftware/oneapi-construction-kit/issues).
  It may be the case that a temporary fix has been posted in a similar report.
* Search the internet (e.g. Stack Overflow, etc) to see if users outside of the
  GitHub community have discussed the issue and discovered a workaround or fix.
* Collect relevant information about the bug, i.e. error log, stack trace, etc.
* Provide your operating system version, compiler version, runtime environment,
  etc, depending on what seems relevant to the issue.
* Where relevant, provide your hardware information, e.g. CPU, GPU, driver
  versions, etc.
* If the bug is triggered by certain inputs or if your expected output is
  incorrect, provide this information so that we can reproduce it.
* If the bug is present on the current stable release branch, please provide
  this info. If the bug is severe enough then it may warrant making a patch
  release.

### Responsible disclosure

If you have discovered a security vulnerability, sensitive information leak, or
a similar security-related issue, please do **not** report this directly to the
issue tracker. Instead, please send your report by email to
[oneapi.construction.kit@codeplay.com](mailto:oneapi.construction.kit@codeplay.com),
making sure to write `DISCLOSURE:` at the beginning of the subject line so that
we can prioritize accordingly.

### Submitting a bug report

Once you have prepared to submit your report, we ask that you follow these
steps.

* Open an
  [issue](https://github.com/codeplaysoftware/oneapi-construction-kit/issues/new/choose).
* Select the **Bug** issue template and fill in the requested details.
* Describe the expected behaviour and the actual behaviour you are seeing.
* Provide as much context as you can about the bug and describe the steps
  required to *reproduce* the bug, so that we can recreate the issue on our
  end.
* Where possible, provide a minimal test case demonstrating the bug. This
  should be a self-contained submission of code that will reproduce the bug
  without any extraneous effects, making it easier for us to isolate where the
  problem is.
* Provide the information collected in the previous section during preparation.

## Suggesting enhancements

This section guides you through submitting an enhancement suggestion for the
oneAPI Construction Kit, whether it's a quality of life improvement or a
completely new feature. Following these guidelines will help maintainers and
the community in fully understanding your suggestion and determining how best
to incorporate it in the project.

### Before suggesting an enhancement

Before submitting an enhancement suggestion, we ask that you follow these steps
in preparation for your suggestion.

* Make sure that you are using the latest version of the software, available on
  the
  [main branch](https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main).
* Read the
  [Documentation](https://developer.codeplay.com/products/oneapi/construction-kit/guides)
  carefully and make sure your suggested functionality is not already covered
  by the project or by an individual configuration of the project.
* Check to see if the enhancement has already been suggested in
  [Issues](https://github.com/codeplaysoftware/oneapi-construction-kit/issues).
  Make sure to search for closed tickets in case the enhancement has been
  previously rejected.

### Submitting an enhancement suggestion

Once you have prepared to submit your enhancement suggestion, we ask that you
follow these steps.

* Open an
  [issue](https://github.com/codeplaysoftware/oneapi-construction-kit/issues/new/choose).
* Select the **Enhancement** issue template and fill in the requested details.
* Use a clear and descriptive title for the issue to identify the suggestion.
* Describe the current behaviour and explain what expected behaviour you are
  proposing and why. You may also describe alternative approaches and explain
  why those do not work for your use case.
* If you have any resources that demonstrate why the suggestion should be
  accepted, feel free to reference these. You may include graphics,
  screenshots, code snippets, etc.
* Explain why the enhancement will be useful to most oneAPI Construction Kit
  users. Where relevant, you may want to reference other projects that serve as
  evidence of why the enhancement is a good idea.

## Code contributions

This section guides you through submitting a pull request with your own code
contribution to the oneAPI Construction Kit project. Following these guidelines
will help maintainers and the community in understanding your submission and
will help minimize code formatting issues, etc.

### Before opening a pull request

Before submitting a pull request, we ask that you follow these steps to ensure
that the review process goes smoothly.

* Make sure the project builds successfully. If your change produces errors in
  the build process, we cannot fix your code for you, so please resolve these
  before submitting your pull request.
* Run tests locally. Pull requests will automatically run across our testing
  infrastructure, but running your own preliminary testing beforehand is a good
  way of avoiding undue headaches and hassle.
* If your change is substantial, a changelog entry may be needed in order to
  capture the details of what has changed in the changelog for the next
  upcoming release. The format of changelog entries **must** match the template
  in [changelog/README.md](changelog/README.md). **Note:** If a changelog entry
  is required, a project maintainer will inform you on your pull request.
* Run [clang-format-16](https://clang.llvm.org/docs/ClangFormat.html) on any
  new or modified code. Our testing infrastructure requires code to be
  formatted correctly, so doing this beforehand avoids potential delays.
* Feel free to add your name to the [list of contributors](CONTRIBUTORS.md).
  Contributor names must be sorted alphabetically.

### Commit messages

When writing commit messages, please include a clear and descriptive short
message on the subject line, providing a brief overview of what the patch does
for anyone reading through the one-line commit messages.

Please also include a detailed description in the commit body, explaining what
the patch does and how it accomplishes it, as well as including any other
relevant details.

The following is a good example of a clear and descriptive commit message:

```
Write contributing guide

This patch adds a contributing guide under CONTRIBUTING.md, detailing
how contributions to the project should be made. The guide touches on
the code of conduct and covers how to ask questions, as well as
detailing the processes that should be followed when reporting bugs,
responsibly disclosing security-related issues, suggesting project
enhancements, and contributing patches to the repository.
```

### Submitting a pull request

Once you have prepared your code contribution, we ask that you follow these
steps.

* Open a
  [pull request](https://github.com/codeplaysoftware/oneapi-construction-kit/compare).
* Add the ![#d73a4a](https://placehold.co/20x20/d73a4a/d73a4a.png) **bug** or
  ![#a2eeef](https://placehold.co/20x20/a2eeef/a2eeef.png) **enhancement**
  labels as appropriate.
* Use a clear and descriptive title to identify the pull request.
* Describe the current behaviour and explain what expected behaviour you are
  proposing and why.
* Provide a general overview of how the code contribution works, detailing the
  effects and consequences of the change.
* Wait for CI to be approved and resolve any test failures.
* Address feedback from reviewers.
* Wait for a project maintainer to make an approving review.
* Wait for your pull request to be merged.

For CI you will need to wait for someone from @ock-reviewers to enable the CI
(or @ock-workflow-reviewers if it is workflow related). The CI can then be
approved based on the PR. Note that currently some of the CI PR testing is done
outside of github and you will be notified of any related failures. This is
currently being addressed. Once it has been approved you can ping @ock-reviewers
to merge it.

It is also possible that after merging to the main branch an overnight failure
may show up issues and a comment will be added to the merged PR.

As a final note, try to keep your pull request branch in good condition. If you
think it makes sense to split your changes across multiple commits, or later
squash multiple commits into one larger commit, please do so. In all
likelihood, keeping your branch tidy will make it easier for project
maintainers to review your changes.

# Getting in touch

Once again, thank you for expressing interest in contributing to the project.
If you'd like to contribute in any way but are unclear on the guidelines listed
above, please don't hesitate to get in touch by [asking a
question](#i-have-a-question).

Finally, if you would like to get in touch with us directly to discuss the
oneAPI Construction Kit, please contact
[oneapi.construction.kit@codeplay.com](mailto:oneapi.construction.kit@codeplay.com)
and include all relevant details.
