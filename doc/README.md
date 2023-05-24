# Using Sphinx

oneAPI Construction Kit uses the [Sphinx][sphinx] documentation generator,
setup of the required tooling is performed during CMake configuration and
output in the`<build>/doc` directory. To build the documentation, use the
`doc_html` build target.

## Sphinx Extensions

### Markdown Support

While [reStructuredText][rst] is preferred for new documents a significant
amount of the documentation was written before switching to using
[Sphinx][sphinx]. To support this the [Markedly Structured Text (MyST)][myst]
extension is used and any files which use the `.md` file extension will be
parsed as Markdown. [MyST][myst] then translates the Markdown into a format
[Sphinx][sphinx] understands internally.

It is also possible to use [reStructuredText][rst] features from within a
Markdown document as described in the [MyST Syntax
Guide](https://myst-parser.readthedocs.io/en/latest/using/syntax.html).

An example of using an admonition directive in a Markdown document:

```markdown
:::{warning}
This is an admonition warning the reader of something to be wary of, it will
be rendered with an orange background to grab their attention.
:::
```

There is also support for Markdown tables using the
[sphinx-markdown-tables](https://pypi.org/project/sphinx-markdown-tables/)
extension.

### Doxygen Integration

The C and C++ header files in the oneAPI Construction Kit are largely documented
using [Doxygen][doxygen] comments. These are parsed by [Doxygen][doxygen] which
is configured to output XML files in the `<build>/doc/xml` directory. To
integrate this information into the generated documentation the [breathe][breathe]
extension is used.

[breathe][breathe] has it's drawbacks, the maintainers are not very active and
there are known performance issues due to dependency on a deprecated Python XML
parsing package which leaks memory. To speed up iteration times when working on
non-Doxygen documentation a work around is to temporarily delete
`doc/api-reference.md` and use the `sphinx-build` build target which skips any
use of [Doxygen][doxygen] and [breathe][breathe].

### CMake Integration

KitWare, the creators of [CMake][cmake], use [Sphinx][sphinx] to generate their
documentation. The extension
[sphinxcontrib.moderncmakedomain](https://pypi.org/project/sphinxcontrib-moderncmakedomain/)
provides this integration which we can use to write [reStructedText][rst]
directly within [CMake][cmake] files.

```cmake
#[=======================================================================[.rst:
.. cmake:variable:: CMAKE_BUILD_TYPE

  A CMake option to specify the type of executables to build.
#]=======================================================================]
```

> Note that these comment lines are special, denoting a block of
> [reStructuredText][rst] inside the [CMake][cmake] file.

To include a [CMake][cmake] module in the documentation, it must be included
using:

```rst
.. cmake-module:: ../relative/path/to/the/file.cmake
```

There are two other useful directives we can use to document the build system,
for example:

```rst
.. cmake:variable:: ENABLE_MY_TARGET

  A boolean CMake option to enable adding my fancy target.
```

```rst
.. cmake:command:: add_my_target

  A CMake command to add my fancy target.

  Arguments:
    * ``target`` The name of the target to add.

  Keyword Arguments:
    * ``COMMAND`` The commad to for the target to invoke.
```

It is also possible to cross-reference existing _or not yet existing_ variables
and commands inline using the following syntax:

```rst
Enable :cmake:variable:`ENABLE_MY_TARGET` to make :cmake:command:`add_my_target`
available.
```

### Todo Items

When a section is incomplete, out of data, or otherwise requires more work use
the following [reStructuredText][rst] syntax to render a visually obvious todo
block.

```rst
.. todo::
   :jira:`CA-1` write this section
```

## PDF

It is also possible to build PDF documents of the documentation, do to this the
following packages must be installed:

```console
$ sudo apt install latexmk texlive-xetex xindy texlive-fonts-extra
```

[sphinx]: https://www.sphinx-doc.org/en/master/
[rst]: https://www.sphinx-doc.org/en/master/usage/restructuredtext/index.html
[myst]: https://myst-parser.readthedocs.io/en/latest/
[breathe]: https://pypi.org/project/breathe/
[cmake]: https://cmake.org
