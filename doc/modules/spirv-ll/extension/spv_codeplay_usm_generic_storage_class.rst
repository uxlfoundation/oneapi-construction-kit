USM Generic Storage Class - ``spv_codeplay_usm_generic_storage_class``
======================================================================

Name Strings
------------

``SPV_codeplay_usm_generic_storage_class``

Contact
-------

*  Aaron Greig, Codeplay Software Ltd. (aaron.greig 'at' codeplay.com)

Contributors
------------

*  Aaron Greig, Codeplay Software Ltd.
*  Ewan Crawford, Codeplay Software Ltd.

Version
-------

+--------------------+------------+
| Last Modified Date | 2020-10-29 |
+--------------------+------------+
| Revision           | 1          |
+--------------------+------------+

Status
------

Proposal

Dependencies
------------

This extension is written against the Unified SPIR-V Specification, Version 1.5,
Revision 3.

This extension requires SPIR-V 1.0.

Overview
--------

This extension allows a module to indicate that it is omitting storage class
information from ``OpTypePointer`` and ``OpTypeForwardPointer`` instructions.
In this case, instead of including storage class information in these
instructions the module passes storage class ``Generic`` to indicate that the
storage class has no semantic meaning. Note that modules using this extension do
not need to declare the ``GenericPointer`` capability, and pointer types
declared in the manner outlined above do not retain any of the semantics of a
regular ``Generic`` pointer type declaration.

Extension Name
--------------

To use this extension within a SPIR-V module, the following **OpExtension** must
be present in the module:

.. code-block:: none

   OpExtension "SPV_codeplay_usm_generic_storage_class"

New Capabilities
----------------

None.

New Builtins
------------

None.

New Instructions
----------------

None.

Token Number Assignments
------------------------

None.

Modifications to the SPIR-V Specification
-----------------------------------------

Modify section 3.7, Storage Class, changing the following entry in the Storage
Class table:

+---+-------------------------------------------------------+----------------+
| 8 | Generic                                               | GenericPointer |
|   | For generic pointers, which overload the Function,    |                |
|   | Workgroup, and CrossWorkgroup Storage Classes. Also   |                |
|   | used to declare pointer types without a storage class |                |
|   | when the "SPV_codeplay_usm_generic_storage_class"     |                |
|   | extension is enabled.                                 |                |
+---+-------------------------------------------------------+----------------+

Modify section 3.68.8, Memory Instructions, changing the description of
``OpVariable``:

+-------------------------------------------------------------------------------+
| ...                                                                           |
| Storage Class is the Storage Class of the memory holding the object. It       |
| cannot be Generic unless OpExtension "SPV_codeplay_usm_generic_storage_class" |
| was declared.                                                                 |
| ...                                                                           |
+-------------------------------------------------------------------------------+

Validation Rules
----------------

An OpExtension must be added to the SPIR-V for validation layers to check legal
use of this extension:

``OpExtension "SPV_codeplay_usm_generic_storage_class"``

Revision History
----------------

+-----+------------+-------------------+----------------------------------+
| Rev | Date       | Author            | Changes                          |
+=====+============+===================+==================================+
| 1   | 2020/10/29 | Aaron Greig       | Initial revision                 |
+-----+------------+-------------------+----------------------------------+
