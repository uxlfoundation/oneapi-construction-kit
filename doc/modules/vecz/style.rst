Vecz Coding Guidelines
======================

Target Audience
---------------

This document is primarily intended for Vecz developers and maintainers. It may
also be useful for developers working on LLVM passes that are not part of Vecz
and for downstream developers who wish to modify or extend Vecz.

This document is unlikely to be useful to developers who only wish to use Vecz.
This document is not intended for a non-engineering audience.

Priority Scale
--------------

Coding guidelines are ranked on the following four-point scale, ranging from 1
(weakest guideline) to 4 (strongest guideline). Most coding guidelines will be
a 1 or a 2. Software projects have requirements that rank as a 3 or 4, but it's
rare that such requirements can be expressed as coding guidelines.

.. table::
  :align: center

  +------+---------------------+----------------+
  | Rank | Name                | Sign-off       |
  +======+=====================+================+
  | 1    | Suggestion          | Any engineer   |
  +------+---------------------+----------------+
  | 2    | Standard            | Merge request  |
  +------+---------------------+----------------+
  | 3    | Requirement         | Product owner  |
  +------+---------------------+----------------+
  | 4    | Crucial requirement | VP Engineering |
  +------+---------------------+----------------+

Priority Descriptions
---------------------

Suggestion (1)
^^^^^^^^^^^^^^

.. _summary1:
.. rubric:: Summary

When there are more than one ways of doing a thing that are all approximately
equally good, then prefer to do the thing in this way.

.. _example1:
.. rubric:: Example

::

  When declaring a class, group public members together, group protected
  members together, and group private members together.

.. _explanation1:
.. rubric:: Explanation

Code consistency is good. ``Suggestion (1)`` guidelines help the person writing
the code to quickly decide how to do a thing so that they can get on with the
real work. These guidelines exist to reduce the amount of time spent thinking,
"Should I do it like this or like that?" These guideline also help the person
reading the code do so more quickly, because the code already looks familiar.

``Suggestion (1)`` guidelines often have exceptions. For the example above, a
class might have closely related public and private member functions, and the
code might be clearer when the closely related functions are grouped together.

.. _codereview1:
.. rubric:: Code Review

When a ``Suggestion (1)`` guideline isn't followed, it usually won't need
a justification in a code comment. It's OK to ask why one of these guidelines
wasn't followed when reviewing an MR, but the discussion shouldn't get drawn
out. The person making the MR probably knows what they're doing.

.. _signoff1:
.. rubric:: Sign-off

Any individual engineer can choose not to follow a ``Suggestion (1)`` guideline
when they feel it is appropriate.

Standard (2)
^^^^^^^^^^^^

.. _summary2:
.. rubric:: Summary

These guidelines are generally seen as best practice within the project, but on
occasion there may be a good reason not to follow one.

.. _example2:
.. rubric:: Example

::

  Use a range-based for loop wherever possible.

.. _explanation2:
.. rubric:: Explanation

Code consistency is very good. ``Standard (2)`` guidelines are ones that
substantially improve performance, readability, compilation time, etc. They
help the person writing the code to remember which way to do things.

``Standard (2)`` guidelines will occasionally have exceptions. For the example
above, a container class might support the use of a range-based for loop, but a
loop that modifies the container in certain ways might not be able to use the
range-based form.

.. _codereview2:
.. rubric:: Code Review

When a ``Standard (2)`` guideline isn't followed, it will usually need a code
comment explaining the reason. When a ``Standard (2)`` guideline is not
applicable to a specific situation, it is probably due to a non-obvious reason,
and the code is improved by explaining that reason in a comment.

When a ``Standard (2)`` guideline isn't followed, the MR in question may need a
discussion weighing the pros and cons of ignoring the guideline.

.. _signoff2:
.. rubric:: Sign-off

Committing code that doesn't meet a ``Standard (2)`` guideline follows the
usual MR review process: The author and the two reviewers must be OK with
ignoring the guideline in the specific instance.

Requirement (3)
^^^^^^^^^^^^^^^

.. _summary3:
.. rubric:: Summary

Customers and/or long-term project goals require doing things like this, but
there may occasionally be good reason to do things differently.

.. _example3:
.. rubric:: Example

::

  Use strncpy() instead of strcpy(), because strcpy() is susceptible to buffer
  overflows.

.. _explanation3:
.. rubric:: Explanation

Sometimes a coding decision makes sense from a code perspective, but has
broader implications on the project or affects customers. For the example
above, a specific use of ``strcpy()`` may be obviously safe, because the
destination buffer is allocated using the size of the source buffer. However,
we may have a contractual obligation to customers that the project's code
passes a linting tool, and the linting tool might not allow ``strcpy()``.

.. _codereview3:
.. rubric:: Code Review

When a ``Requirement (3)`` guideline isn't followed, there must be a code
comment explaining the reasons. Furthermore, the relevant product owner(s)
(ComputeAorta and/or customer team(s)) must OK the change. There may be an
extended discussion involved. The result of the discussion may very well be
that the guideline must be followed and that an alternate solution must be
found.

.. _signoff3:
.. rubric:: Sign-off

Relevant product owners must sign off when a ``Requirement (3)`` isn't
followed.

Crucial Requirement (4)
^^^^^^^^^^^^^^^^^^^^^^^

.. _summary4:
.. rubric:: Summary

Business and/or legal reasons require doing things like this.

.. _example4:
.. rubric:: Example

::

   Do not incorporate an open source library that uses a license that is
   different from the open source licenses already used in ComputeAorta.

.. _explanation4:
.. rubric:: Explanation

There might be an open source library that provides a useful feature. That
library's license might be compatible with the closed-source nature of Vecz and
ComputeAorta. However, if the license is not already on the list of open-source
licenses used by ComputeAorta, then using the library would require adding the
license to the list, which requires updating contracts with all existing
customers. Updating contracts is not impossible, but it is non-trivial and has
business and legal implications.

.. _codereview4:
.. rubric:: Code Review

Committing code that does not follow a ``Crucial Requirement (4)`` guideline
will probably require a process beyond normal GitLab MR review. The process
does not currently exist.

.. _signoff4:
.. rubric:: Sign-off

The VP of Engineering (or similar) must sign off on code that does not meet a
crucial requirement.

Guidelines
----------

.. table::
  :align: center

  +----------------+------+
  | Name           | Rank |
  +================+======+
  | :ref:`gdln001` | 2    |
  +----------------+------+
  | :ref:`gdln002` | 2    |
  +----------------+------+
  | :ref:`gdln003` | 2    |
  +----------------+------+
  | :ref:`gdln004` | 2    |
  +----------------+------+
  | :ref:`gdln005` | 2    |
  +----------------+------+
  | :ref:`gdln006` | 2    |
  +----------------+------+
  | :ref:`gdln007` | 2    |
  +----------------+------+
  | :ref:`gdln008` | 2    |
  +----------------+------+
  | :ref:`gdln009` | 2    |
  +----------------+------+
  | :ref:`gdln010` | 2    |
  +----------------+------+
  | :ref:`gdln011` | 1    |
  +----------------+------+
  | :ref:`gdln012` | 2    |
  +----------------+------+
  | :ref:`gdln013` | 2    |
  +----------------+------+
  | :ref:`gdln014` | 2    |
  +----------------+------+
  | :ref:`gdln015` | 2    |
  +----------------+------+
  | :ref:`gdln016` | 1    |
  +----------------+------+
  | :ref:`gdln017` | 3    |
  +----------------+------+

.. _gdln001:

001: Avoid ``vecz_`` prefix in identifiers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Rank**: 2

Vecz identifiers must not begin with ``vecz_``. Vecz is C++ code that lives
inside an appropriately named C++ namespace. The prefix just takes up extra
space.

.. _gdln002:

002: Prefer one class per external ``.h`` file
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Rank**: 2

An external ``.h`` will preferably contain at most one class definition. This
helps keep files small, making them easier to understand and faster to build.

One notable exception is :ref:`gdln003`.

.. note::

   This guideline applies to external ``.h`` files. Internal ``.h`` files may
   have different considerations.

.. _gdln003:

003: Keep a class declaration and helper class declarations in the same ``.h``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Rank**: 2

Sometimes a class needs to declare a helper class for its interface. Examples
of helper classes include structs for passing parameters and results from
analysis passes. If the helper class is *only* used to interface with the
primary class, then declare both in the same ``.h`` file.

If the helper class might be used witout the primary class, then this guideline
does not apply. If the helper class is only used inside the primary class, then
it's an inner class; see :ref:`gdln004`.

.. _gdln004:

004: Keep an inner class declaration in the ``.cpp`` file
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Rank**: 2

When a ``.cpp`` file uses a helper class and that helper class is only used
with the one ``.cpp`` file, then the inner class declaration and definition
should both be in the ``.cpp`` file. This helps keep the interface in the
corresponding ``.h`` file clean.

.. _gdln005:

005: Declare self-contained passes in the shared header
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Rank**: 2

When a compiler pass is only ever accessed via a ``create()`` factory method
and does not declare any data structures for its interface, then declare the
pass in the shared header in ``source/include/transform/passes.h``.

I.e., if a pass declares a structure that stores an analysis result, then this
guideline doesn't apply. See :ref:`gdln006` instead.

The pass can be defined in either the shared passes source file,
``source/transform/passes.cpp``, or in its own source file.

.. note::

   An analysis pass will always need a data structure on its interface, so it
   will always need a separate header. Consequently, the shared header is in
   the ``transform`` directory.

.. _gdln006:

006: Use a pass-specific header for a pass with helper structures
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Rank**: 2

When a compiler pass declares a helper data structure, such as one used for
storing analysis results, then declare the pass and any helper structures in a
``.h`` file specific to that pass.

See also: :ref:`gdln003`

.. _gdln007:

007: Place external headers in ``include/vecz/``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Rank**: 2

Headers that are part of the interface to vecz must all be in ``include/vecz``.
Headers that are not part of the vecz interface must not be in
``include/vecz``.

.. _gdln008:

008: Place internal headers in ``source/include/``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Rank**: 2

Headers that are only used internally in vecz must all be in
``source/include/``. Headers that are part of the external vecz interface must
not be in ``source/include/``.

.. _gdln009:

009: Use ``{}`` around control flow blocks
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Rank**: 2

Control flow blocks must always use ``{}``, even when there is only one line of
code in the block. This avoids ambiguity:

.. code-block:: c++

  if(foo)
    do_bar();
    do_baz(); // PROBABLY WRONG!!!

.. _gdln010:

010: Place Doxygen documentation in headers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Rank**: 2

Classes, class members, free functions, etc. must be documented with Doxygen
comments. Doxygen comments must be in ``.h`` files.

.. _gdln011:

011: Prefer pass-by-reference over pass-by-pointer
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Rank**: 1

Prefer to pass function parameters by reference instead of by pointer.
References are safer.

A common exception is when the parameter may be a ``nullptr``, since
pass-by-pointer is then required.

Another exception is when the function performs pointer comparisons, because in
this case, passing by pointer can result in clearer and more concise code.
However, if the function only needs to take the address of a parameter so that
it can pass the address to a second function, then pass-by-reference is still
preferred for the first function. The first function can just take the address
of the parameter when it calls the second function.

.. _gdln012:

012: Prefer forward declarations over include files
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Rank**: 2

Use forward declarations for classes and structs instead of ``#include``-ing
header files wherever possible to reduce compilation time.

.. _gdln013:

013: Avoid ``vecz_`` prefix in file names
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Rank**: 2

Vecz header and source files must not begin with ``vecz_``. The ``vecz_``
prefix was used in the past when Vecz files shared a directory with other
files. As this is no longer the case, the prefix serves no purpose and only
makes file names longer.

.. _gdln014:

014: Place passes in the appropriate sub-directory
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Rank**: 2

Place analysis pass header files in ``source/include/analysis/`` and analysis
pass source files in ``source/analysis/``.  Place transformation pass header
files in ``source/include/transform/`` and transformation pass source files in
``source/transform/``.

.. _gdln015:

015: Use descriptive file names in ``snake_case``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Rank**: 2

Use descriptive file names. Use snake case. There is nothing wrong with a long
file name if it makes the contents of the file clearer, e.g.,
``common_gep_elimination_pass.cpp``.

.. _gdln016:

016: Consider using ``_analysis`` and ``_pass`` file name suffixes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Rank**: 1

There are no required file name prefixes or suffixes for files containing
compiler passes. However, for historical reasons, many analysis passes have an
``_analysis`` suffix, and many transformation passes have a ``_pass`` suffix.
Consider using these suffixes if they make the file name clearer.

See also :ref:`gdln014`.

.. _gdln017:

017: Use correctly named include guards in headers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Rank**: 3

Vecz headers must use include guards. Include guards must be named using this
convention:

::

  VECZ_TRANSFORM_FILE_NAME_H_INCLUDED

I.e., the string ``VECZ_``, followed by the type of pass if applicable (either
``ANALYSIS`` or ``TRANSFORM``), followed by the file name in all caps, followed
by the string ``_H_INCLUDED``.
