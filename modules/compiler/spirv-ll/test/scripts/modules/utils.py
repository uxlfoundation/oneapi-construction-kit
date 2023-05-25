# pylint: disable=missing-docstring,too-many-lines,relative-import

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

from re import findall

# Copyright to write at the beginning of generated files.
COPYRIGHT = (
'''// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

''')

# Extensions of generated files.
GLSL_EXT = '.comp'
TEST_EXT = '.test'

# Identifiers regexp, according to the LLVM-IR specification.
ID = '{{[%@][-a-zA-Z$._0-9][-a-zA-Z$._0-9]*}}'
NUMBER = '{{[0-9]+}}'
LLVM_ANY_FLOAT = '{{(-?[0-9]+\\.[0-9]+e\\+[0-9]+|0x[0-9A-F]+)}}'
LLVM_ANY_INT = '{{i[0-9]+}}'
LLVM_LABEL = '{{; <label>:[0-9]+}}'
METADATA_ID = '!{{[0-9]+}}'

# Useful formats.
RUN_FORMAT = '''\
RUN: spirv-ll-tool %p/{0}.spv > %t
RUN: FileCheck < %t %s'''

# Dictionaries for operand types translation into LLVM-IR.
CONSTANTS = {
    'bool': 'true',
    'double': '-42.42',
    'float': '42.42',
    'int': '-42',
    'uint': '42',
    'long': '-4200000000',
    'ulong': '4200000000'
}

# Dictionary for regexes that match any constant of a given type
GENERIC_CONSTANTS = {
    'bool': '{{true|false}}',
    'int': '{{-?[0-9]+}}',
    'uint': '{{[0-9]+}}',
    'float': LLVM_ANY_FLOAT,
    'double': LLVM_ANY_FLOAT
}

# Arbitrary constants that will be used when creating variables in tests
LLVM_CONSTANTS = {
    'bool': 'true',
    'int': '-42',
    'uint': '42',
    'float': LLVM_ANY_FLOAT,
    'double': LLVM_ANY_FLOAT,
    'long': '-4200000000',
    'ulong': '4200000000'
}

LLVM_COMPOSITE_INT_CONSTANTS = {
    'array': '{{\\[(i32 [0-9]+(, )?)+\\]}}',
    'struct': '{{{ (i32 [0-9]+(, )?)+ }}}',
    'vector': '{{<(i32 [0-9]+(, )?)+>}}'
}

GLSL_TYPES = ['bool', 'int', 'uint', 'float', 'double']

NUMERICAL_TYPES = ['int', 'uint', 'long', 'float', 'double']

LLVM_TYPES = {
    'bool': 'i1',
    'int': 'i32',
    'uint': 'i32',
    'float': 'float',
    'double': 'double',
    'long': 'i64',
    'ulong': 'i64'
}

# Mapping between GLSL types and LLVM types:
GLSL_LLVM_TYPES = {
    'bool': 'i1',
    'int': 'i32',
    'uint': 'i32',
    'float': 'float',
    'double': 'double',
    'long': 'i64',
    'ulong': 'i64',
    'vec2': '<2 x float>',
    'vec3': '<3 x float>',
    'vec4': '<4 x float>',
    'ivec2': '<2 x i32>',
    'ivec3': '<3 x i32>',
    'ivec4': '<4 x i32>',
    'uvec2': '<2 x i32>',
    'uvec3': '<3 x i32>',
    'uvec4': '<4 x i32>',
    'dvec2': '<2 x double>',
    'dvec3': '<3 x double>',
    'dvec4': '<4 x double>',
    # In all kernels currently tested, all pointers point to within the global
    # address space (addrspace = 1)
    'float*': 'float addrspace(1)*',
    'double*': 'double addrspace(1)*',
    'vec2*': '<2 x float> addrspace(1)*',
    'vec3*': '<3 x float> addrspace(1)*',
    'vec4*': '<4 x float> addrspace(1)*',
    'dvec2*': '<2 x double> addrspace(1)*',
    'dvec3*': '<3 x double> addrspace(1)*',
    'dvec4*': '<4 x double> addrspace(1)*',
    'int*': 'i32 addrspace(1)*',
    'ivec2*': '<2 x i32> addrspace(1)*',
    'ivec3*': '<3 x i32> addrspace(1)*',
    'ivec4*': '<4 x i32> addrspace(1)*',
}

VEC_TYPES = {
    'bool': 'bvec',
    'int': 'ivec',
    'uint': 'uvec',
    'float': 'vec',
    'double': 'dvec'
}

MAT_TYPES = {'float': 'mat', 'double': 'dmat'}

# Default array size
ARRAY_SIZE = 4

# Dictionary for binary operators translation into LLVM-IR.
BINARY_OPERATORS = {
    '+': {
        'int': {
            'inst': 'add',
            'ret': 'int',
            'vec': True,
            'mat': False
        },
        'uint': {
            'inst': 'add',
            'ret': 'uint',
            'vec': True,
            'mat': False
        },
        'float': {
            'inst': 'fadd',
            'ret': 'float',
            'vec': True,
            'mat': True
        },
        'double': {
            'inst': 'fadd',
            'ret': 'double',
            'vec': True,
            'mat': True
        }
    },
    '-': {
        'int': {
            'inst': 'sub',
            'ret': 'int',
            'vec': True,
            'mat': False
        },
        'uint': {
            'inst': 'sub',
            'ret': 'uint',
            'vec': True,
            'mat': False
        },
        'float': {
            'inst': 'fsub',
            'ret': 'float',
            'vec': True,
            'mat': True
        },
        'double': {
            'inst': 'fsub',
            'ret': 'double',
            'vec': True,
            'mat': True
        }
    },
    '*': {
        'int': {
            'inst': 'mul',
            'ret': 'int',
            'vec': True,
            'mat': False
        },
        'uint': {
            'inst': 'mul',
            'ret': 'uint',
            'vec': True,
            'mat': False
        },
        # TODO: SPIR-V has instructions for matrix multiply operations,
        # re-enable matrix multiply tests when they are implemented
        'float': {
            'inst': 'fmul',
            'ret': 'float',
            'vec': True,
            'mat': False
        },
        'double': {
            'inst': 'fmul',
            'ret': 'double',
            'vec': True,
            'mat': False
        }
    },
    '/': {
        'int': {
            'inst': 'sdiv',
            'ret': 'int',
            'vec': True,
            'mat': False
        },
        'uint': {
            'inst': 'udiv',
            'ret': 'uint',
            'vec': True,
            'mat': False
        },
        'float': {
            'inst': 'fdiv',
            'ret': 'float',
            'vec': True,
            'mat': True
        },
        'double': {
            'inst': 'fdiv',
            'ret': 'double',
            'vec': True,
            'mat': True
        }
    },
    # TODO: In GLSL, modulo operator is not defined for non-integers types.
    # It's not the case in SPIR-V and LLVM-IR. Then, we'll have to handwrite
    # tests for modulo operator with float and double operands.
    '%': {
        'int': {
            'inst': 'smod',
            'ret': 'int',
            'vec': True,
            'mat': False
        },
        'uint': {
            'inst': 'umod',
            'ret': 'uint',
            'vec': True,
            'mat': False
        }
    },
    '&&': {
        'bool': {
            'inst': 'and',
            'ret': 'bool',
            'vec': False,
            'mat': False
        }
    },
    '||': {
        'bool': {
            'inst': 'or',
            'ret': 'bool',
            'vec': False,
            'mat': False
        }
    },
    '^^': {
        'bool': {
            'inst': 'xor',
            'ret': 'bool',
            'vec': False,
            'mat': False
        }
    },
    # TODO: For the moment, only 'ordered' comparaison are generated for double
    # and float operands. It means that NaN is not a possible value for both
    # floating points operands.
    # TODO: re-enable vector and matrix comparison tests when OpAll is
    # implemented with abacus
    '==': {
        'int': {
            'inst': 'icmp eq',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'uint': {
            'inst': 'icmp eq',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'float': {
            'inst': 'fcmp oeq',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'double': {
            'inst': 'fcmp oeq',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'bool': {
            'inst': 'cmp eq',
            'ret': 'bool',
            'vec': False,
            'mat': False
        }
    },
    '!=': {
        'int': {
            'inst': 'icmp ne',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'uint': {
            'inst': 'icmp ne',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'float': {
            'inst': 'fcmp one',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'double': {
            'inst': 'fcmp one',
            'ret': 'bool',
            'vec': False,
            'mat': False
        }
    },
    '>=': {
        'int': {
            'inst': 'icmp sge',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'uint': {
            'inst': 'icmp uge',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'float': {
            'inst': 'fcmp oge',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'double': {
            'inst': 'fcmp oge',
            'ret': 'bool',
            'vec': False,
            'mat': False
        }
    },
    '>': {
        'int': {
            'inst': 'icmp sgt',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'uint': {
            'inst': 'icmp ugt',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'float': {
            'inst': 'fcmp ogt',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'double': {
            'inst': 'fcmp ogt',
            'ret': 'bool',
            'vec': False,
            'mat': False
        }
    },
    '<=': {
        'int': {
            'inst': 'icmp sle',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'uint': {
            'inst': 'icmp ule',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'float': {
            'inst': 'fcmp ole',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'double': {
            'inst': 'fcmp ole',
            'ret': 'bool',
            'vec': False,
            'mat': False
        }
    },
    '<': {
        'int': {
            'inst': 'icmp slt',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'uint': {
            'inst': 'icmp ult',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'float': {
            'inst': 'fcmp olt',
            'ret': 'bool',
            'vec': False,
            'mat': False
        },
        'double': {
            'inst': 'fcmp olt',
            'ret': 'bool',
            'vec': False,
            'mat': False
        }
    },
    '<<': {
        'int': {
            'inst': 'shl',
            'ret': 'int',
            'vec': True,
            'mat': False
        },
        'uint': {
            'inst': 'shl',
            'ret': 'uint',
            'vec': True,
            'mat': False
        }
    },
    '>>': {
        'int': {
            'inst': 'ashr',
            'ret': 'int',
            'vec': True,
            'mat': False
        },
        'uint': {
            'inst': 'lshr',
            'ret': 'uint',
            'vec': True,
            'mat': False
        }
    },
    '&': {
        'int': {
            'inst': 'and',
            'ret': 'int',
            'vec': True,
            'mat': False
        },
        'uint': {
            'inst': 'and',
            'ret': 'uint',
            'vec': True,
            'mat': False
        }
    },
    '|': {
        'int': {
            'inst': 'or',
            'ret': 'int',
            'vec': True,
            'mat': False
        },
        'uint': {
            'inst': 'or',
            'ret': 'uint',
            'vec': True,
            'mat': False
        }
    },
    '^': {
        'int': {
            'inst': 'xor',
            'ret': 'int',
            'vec': True,
            'mat': False
        },
        'uint': {
            'inst': 'xor',
            'ret': 'uint',
            'vec': True,
            'mat': False
        }
    }
}

# Dictionary for convertion operations.
CONVERSIONS = {
    'double': {
        'float': 'fptrunc',
        'int': 'fptosi',
        'uint': 'fptoui'
    },
    'float': {
        'double': 'fpext',
        'int': 'fptosi',
        'uint': 'fptoui'
    },
    'int': {
        'double': 'sitofp',
        'float': 'sitofp',
        'long': 'sext'
    },
    'uint': {
        'double': 'uitofp',
        'float': 'uitofp',
        'ulong': 'zext'
    },
    'long': {
        'int': 'trunc'
    },
    'ulong': {
        'uint': 'trunc'
    }
}

# Dictionary for bitcasts
BITCASTS = {'double': 'long', 'long': 'double', 'int': 'float', 'float': 'int'}

# Lists of GLSL argument combinations and the corresponding Abacus call mangled
# type suffix Each element is a tuple consisting of (return type, list of
# argument types, Itanium mangled type suffix)
# Note: the reason these are not generated is so that the name mangling can be
# checked by hand: using a name_mangler function to test another name_mangler
# function doesn't really test anything as both implementations could be wrong
# yet consistant.
ARGSET_1_FLOAT = [
    ('float', ('float', ), 'f'),
    ('vec2', ('vec2', ), 'Dv2_f'),
    ('vec3', ('vec3', ), 'Dv3_f'),
    ('vec4', ('vec4', ), 'Dv4_f'),
]

ARGSET_2_FLOAT = [
    ('float', ('float', ) * 2, 'ff'),
    ('vec2', ('vec2', ) * 2, 'Dv2_fS_'),
    ('vec3', ('vec3', ) * 2, 'Dv3_fS_'),
    ('vec4', ('vec4', ) * 2, 'Dv4_fS_'),
]

ARGSET_3_FLOAT = [
    ('float', ('float', ) * 3, 'fff'),
    ('vec2', ('vec2', ) * 3, 'Dv2_fS_S_'),
    ('vec3', ('vec3', ) * 3, 'Dv3_fS_S_'),
    ('vec4', ('vec4', ) * 3, 'Dv4_fS_S_'),
]

ARGSET_1_DOUBLE = [
    ('double', ('double', ), 'd'),
    ('dvec2', ('dvec2', ), 'Dv2_d'),
    ('dvec3', ('dvec3', ), 'Dv3_d'),
    ('dvec4', ('dvec4', ), 'Dv4_d'),
]

ARGSET_2_DOUBLE = [
    ('double', ('double', ) * 2, 'dd'),
    ('dvec2', ('dvec2', ) * 2, 'Dv2_dS_'),
    ('dvec3', ('dvec3', ) * 2, 'Dv3_dS_'),
    ('dvec4', ('dvec4', ) * 2, 'Dv4_dS_'),
]

ARGSET_3_DOUBLE = [
    ('double', ('double', ) * 3, 'ddd'),
    ('dvec2', ('dvec2', ) * 3, 'Dv2_dS_S_'),
    ('dvec3', ('dvec3', ) * 3, 'Dv3_dS_S_'),
    ('dvec4', ('dvec4', ) * 3, 'Dv4_dS_S_'),
]

ARGSET_1_INT = [
    ('int', ('int', ), 'i'),
    ('ivec2', ('ivec2', ), 'Dv2_i'),
    ('ivec3', ('ivec3', ), 'Dv3_i'),
    ('ivec4', ('ivec4', ), 'Dv4_i'),
]

ARGSET_2_INT = [
    ('int', ('int', ) * 2, 'ii'),
    ('ivec2', ('ivec2', ) * 2, 'Dv2_iS_'),
    ('ivec3', ('ivec3', ) * 2, 'Dv3_iS_'),
    ('ivec4', ('ivec4', ) * 2, 'Dv4_iS_'),
]

ARGSET_3_INT = [
    ('int', ('int', ) * 3, 'iii'),
    ('ivec2', ('ivec2', ) * 3, 'Dv2_iS_S_'),
    ('ivec3', ('ivec3', ) * 3, 'Dv3_iS_S_'),
    ('ivec4', ('ivec4', ) * 3, 'Dv4_iS_S_'),
]

ARGSET_1_UINT = [
    ('uint', ('uint', ), 'j'),
    ('uvec2', ('uvec2', ), 'Dv2_j'),
    ('uvec3', ('uvec3', ), 'Dv3_j'),
    ('uvec4', ('uvec4', ), 'Dv4_j'),
]

ARGSET_2_UINT = [
    ('uint', ('uint', ) * 2, 'jj'),
    ('uvec2', ('uvec2', ) * 2, 'Dv2_jS_'),
    ('uvec3', ('uvec3', ) * 2, 'Dv3_jS_'),
    ('uvec4', ('uvec4', ) * 2, 'Dv4_jS_'),
]

ARGSET_3_UINT = [
    ('uint', ('uint', ) * 3, 'jjj'),
    ('uvec2', ('uvec2', ) * 3, 'Dv2_jS_S_'),
    ('uvec3', ('uvec3', ) * 3, 'Dv3_jS_S_'),
    ('uvec4', ('uvec4', ) * 3, 'Dv4_jS_S_'),
]

# The pointer points to inside the output buffer which is a global variable,
# hence the address space = 1.
ARGSET_MODF = [
    ('float', ('float', 'float*'), 'fPU3AS1f'),
    ('vec2', ('vec2', 'vec2*'), 'Dv2_fPU3AS1S_'),
    ('vec3', ('vec3', 'vec3*'), 'Dv3_fPU3AS1S_'),
    ('vec4', ('vec4', 'vec4*'), 'Dv4_fPU3AS1S_'),
    ('double', ('double', 'double*'), 'dPU3AS1d'),
    ('dvec2', ('dvec2', 'dvec2*'), 'Dv2_dPU3AS1S_'),
    ('dvec3', ('dvec3', 'dvec3*'), 'Dv3_dPU3AS1S_'),
    ('dvec4', ('dvec4', 'dvec4*'), 'Dv4_dPU3AS1S_'),
]

ARGSET_FREXP = [
    ('float', ('float', 'int*'), 'fPU3AS1i'),
    ('vec2', ('vec2', 'ivec2*'), 'Dv2_fPU3AS1Dv2_i'),
    ('vec3', ('vec3', 'ivec3*'), 'Dv3_fPU3AS1Dv3_i'),
    ('vec4', ('vec4', 'ivec4*'), 'Dv4_fPU3AS1Dv4_i'),
    ('double', ('double', 'int*'), 'dPU3AS1i'),
    ('dvec2', ('dvec2', 'ivec2*'), 'Dv2_dPU3AS1Dv2_i'),
    ('dvec3', ('dvec3', 'ivec3*'), 'Dv3_dPU3AS1Dv3_i'),
    ('dvec4', ('dvec4', 'ivec4*'), 'Dv4_dPU3AS1Dv4_i'),
]

ARGSET_LENGTH = [
    ('float', ('float', ), 'f'),
    ('float', ('vec2', ), 'Dv2_f'),
    ('float', ('vec3', ), 'Dv3_f'),
    ('float', ('vec4', ), 'Dv4_f'),
    ('double', ('double', ), 'd'),
    ('double', ('dvec2', ), 'Dv2_d'),
    ('double', ('dvec3', ), 'Dv3_d'),
    ('double', ('dvec4', ), 'Dv4_d'),
]

ARGSET_DISTANCE = [
    ('float', ('float', ) * 2, 'ff'),
    ('float', ('vec2', ) * 2, 'Dv2_fS_'),
    ('float', ('vec3', ) * 2, 'Dv3_fS_'),
    ('float', ('vec4', ) * 2, 'Dv4_fS_'),
    ('double', ('double', ) * 2, 'dd'),
    ('double', ('dvec2', ) * 2, 'Dv2_dS_'),
    ('double', ('dvec3', ) * 2, 'Dv3_dS_'),
    ('double', ('dvec4', ) * 2, 'Dv4_dS_'),
]

ARGSET_CROSS = [
    ('vec3', ('vec3', ) * 2, 'Dv3_fS_'),
    ('dvec3', ('dvec3', ) * 2, 'Dv3_dS_'),
]

ARGSET_LDEXP = [
    ('float', ('float', 'int'), 'fi'),
    ('vec2', ('vec2', 'ivec2'), 'Dv2_fDv2_i'),
    ('vec3', ('vec3', 'ivec3'), 'Dv3_fDv3_i'),
    ('vec4', ('vec4', 'ivec4'), 'Dv4_fDv4_i'),
    ('double', ('double', 'int'), 'di'),
    ('dvec2', ('dvec2', 'ivec2'), 'Dv2_dDv2_i'),
    ('dvec3', ('dvec3', 'ivec3'), 'Dv3_dDv3_i'),  # TODO: <- FIX THIS
    ('dvec4', ('dvec4', 'ivec4'), 'Dv4_dDv4_i'),
]

ARGSET_REFRACT = [
    ('float', ('float', 'float', 'float'), 'fff'),
    ('vec2', ('vec2', 'vec2', 'float'), 'Dv2_fS_f'),
    ('vec3', ('vec3', 'vec3', 'float'), 'Dv3_fS_f'),
    ('vec4', ('vec4', 'vec4', 'float'), 'Dv4_fS_f'),
    # there is a mismatch here between the GLSL spec and the
    # glslc output, as the spec states that the final argument
    # must be a 16 or 32 bit float but glslc produces
    # doubles:
    ('double', ('double', 'double', 'double'), 'ddd'),
    ('dvec2', ('dvec2', 'dvec2', 'double'), 'Dv2_dS_d'),
    ('dvec3', ('dvec3', 'dvec3', 'double'), 'Dv3_dS_d'),
    ('dvec4', ('dvec4', 'dvec4', 'double'), 'Dv4_dS_d'),
]

# TODO: Type mangling suffixes for the following are unused as they
# have special implementations

# Required for PackSnorm4x8, PackUnorm4x8
ARGSET_PACK4x8 = [
    ('uint', ('vec4', ), 'Dv4_f'),
]
# ARGSET_UPACK4x8 = [('int', ('uvec4'), 'Dv4_f'),]

# Required for PackSnorm2x16, PackUnorm2x16, PackHalf2x16
ARGSET_PACK2x16 = [
    ('uint', ('vec2', ), 'Dv2_f'),
]

# Required for PackDouble2x32
ARGSET_PACK2x32 = [
    ('double', ('uvec2', ), 'Dv2_j'),
]

# Required for UnpackSnorm4x8, UnpackUnorm4x8
# TODO: Should there be seperate signed and unsigned versions?
ARGSET_UNPACK4x8 = [
    ('vec4', ('uint', ), 'j'),
]

# Required for UnpackSnorm2x16, UnpackUnorm2x16, UnpackHalf2x16
# TODO: As above
ARGSET_UNPACK2x16 = [
    ('vec2', ('uint', ), 'j'),
]

# Required for UnpackDouble2x32
# TODO: as above but for result type
ARGSET_UNPACK2x32 = [
    ('uvec2', ('double', ), 'd'),
]

# Note above is not tested with signed ints!

# yapf: enable

# Floating point argument sets (half, float and double)
# TODO: add halfs once implemented in builtins
ARGSET_1_FP = ARGSET_1_FLOAT + ARGSET_1_DOUBLE
ARGSET_2_FP = ARGSET_2_FLOAT + ARGSET_2_DOUBLE
ARGSET_3_FP = ARGSET_3_FLOAT + ARGSET_3_DOUBLE

ARGSET_1_FP_OR_INT = ARGSET_1_FP + ARGSET_1_INT

ARGSET_1_HALF_FLOAT = ARGSET_1_FLOAT  # TODO: Add halfs
ARGSET_2_HALF_FLOAT = ARGSET_2_FLOAT  # TODO: Add halfs

ARGSET_FINDUMSB = [
    ('int', ('uint', ), 'j'),
    ('ivec2', ('uvec2', ), 'Dv2_j'),
    ('ivec3', ('uvec3', ), 'Dv3_j'),
    ('ivec4', ('uvec4', ), 'Dv4_j'),
]

# Argument set for FrexpStruct which is manually generated and implemented
ARGSET_FREXP_STRUCT = [
    ('FrexpStructfloat', ('float', ), 'f'),
    ('FrexpStructvec2', ('vec2', ), 'Dv2_f'),
    ('FrexpStructvec3', ('vec3', ), 'Dv3_f'),
    ('FrexpStructvec4', ('vec4', ), 'Dv3_f'),
    ('FrexpStructdouble', ('double', ), 'd'),
    ('FrexpStructdvec2', ('dvec2', ), 'Dv2_d'),
    ('FrexpStructdvec3', ('dvec3', ), 'Dv3_d'),
    ('FrexpStructdvec4', ('dvec4', ), 'Dv4_d'),
]

# Argument set for ModfStruct which is manually generated and implemented
ARGSET_MODF_STRUCT = [
    ('ModfStructfloat', ('float', ), 'f'),
    ('ModfStructvec2', ('vec2', ), 'Dv2_f'),
    ('ModfStructvec3', ('vec3', ), 'Dv3_f'),
    ('ModfStructvec4', ('vec4', ), 'Dv3_f'),
    ('ModfStructdouble', ('double', ), 'd'),
    ('ModfStructdvec2', ('dvec2', ), 'Dv2_d'),
    ('ModfStructdvec3', ('dvec3', ), 'Dv3_d'),
    ('ModfStructdvec4', ('dvec4', ), 'Dv4_d'),
]
# Notes about how GLSL builtins are tested.
#
# Input for GLSL tests are generated in two different ways:
# * A GLSL kernel is generated by the glsl_generator script. This kernel
#   contains a call to a GLSL builtin. This file is compiled by glslc to
#   produce the corresponding SPIR-V.
# * For SPIR-V builtins which cannot be generated by glslc, raw SPVASM is
#   generated by the spvasm_generator script.
#
# Tests files are generated in two different ways:
# * For function whose implementation is simply a function call, the Lit test
#   file checks that this function call is generated correctly with the
#   correct name and mangling.
# * For functions which have a different implementation, custom Lit code is
#   used to check the final result.
#
# GLSL_BUILTINS is a large table which contains details about the various
# builtins. The table is used in various ways:
# * The first and third items in each tuple (the builtin name and the argument
#   set) are used to generate the names of the input and test files.
# * The second item in each tuple contains the name of GLSL function which
#   the tuple refers to, UNLESS this builtin is generated manually, in which
#   case the second item is ''.
# * The third item in each tuple contains an argument set. An argument set
#   is a list which has as elements tuples containing the following:
#     * The GLSL return type of the function
#     * The GLSL argument types of the function
#     * The Itanium name mangling for the types which is appended to the
#       mangled function name.
#
# If the GLSL_BUILTIN is generated by spvasm_generator, then no data from
# the table is used to generate the input. The argument set and instruction
# name are still used to name the test file, however.
#
# If the GLSL_BUILTIN is implemented manually (i.e. not just a simple
# function call), then only the builtin name and the argument types are
# referenced: all mangling and return type infomation is ignored.
#
# Importantly, the first item in each tuple (builtin name) and the
# function's arguments types (contained with the argument set) are always
# used , independant of how the function is tested, to delineate and
# identify specific tests and so should always contain meaningful values.
#
# For the purposes of test names, GLSL type names are always used even if the
# .spv is not generated by glslc. This convention makes it easier to identify
# specific types and tests.

# Associate each builtin function to the glsl function and to the argument set
# Each tuple consists of (Builtn name, glsl function name , argument set)
# If the GLSL function name is '', the test is assumed to be implemented
# manually in spvasm_generator.py.
# If both the GLSL function name is '' and the argument set is [], then the
# builtin is not implemented.
GLSL_BUILTINS = [
    ('Round', 'round', ARGSET_1_FP),
    ('RoundEven', 'roundEven', ARGSET_1_FP),
    ('Trunc', 'trunc', ARGSET_1_FP),
    ('FAbs', 'abs', ARGSET_1_FP),
    ('SAbs', 'abs', ARGSET_1_INT),
    ('FSign', 'sign', ARGSET_1_FP),
    ('SSign', 'sign', ARGSET_1_INT),
    ('Floor', 'floor', ARGSET_1_FP),
    ('Ceil', 'ceil', ARGSET_1_FP),
    ('Fract', 'fract', ARGSET_1_FP),
    ('Radians', 'radians', ARGSET_1_HALF_FLOAT),
    ('Degrees', 'degrees', ARGSET_1_HALF_FLOAT),
    ('Sin', 'sin', ARGSET_1_HALF_FLOAT),
    ('Cos', 'cos', ARGSET_1_HALF_FLOAT),
    ('Tan', 'tan', ARGSET_1_HALF_FLOAT),
    ('Asin', 'asin', ARGSET_1_HALF_FLOAT),
    ('Acos', 'acos', ARGSET_1_HALF_FLOAT),
    ('Atan', 'atan', ARGSET_1_HALF_FLOAT),
    ('Sinh', 'sinh', ARGSET_1_HALF_FLOAT),
    ('Cosh', 'cosh', ARGSET_1_HALF_FLOAT),
    ('Tanh', 'tanh', ARGSET_1_HALF_FLOAT),
    ('Asinh', 'asinh', ARGSET_1_HALF_FLOAT),
    ('Acosh', 'acosh', ARGSET_1_HALF_FLOAT),
    ('Atanh', 'atanh', ARGSET_1_HALF_FLOAT),
    ('Atan2', 'atan', ARGSET_2_HALF_FLOAT),
    ('Pow', 'pow', ARGSET_2_HALF_FLOAT),
    ('Exp', 'exp', ARGSET_1_HALF_FLOAT),
    ('Log', 'log', ARGSET_1_HALF_FLOAT),
    ('Exp2', 'exp2', ARGSET_1_HALF_FLOAT),
    ('Log2', 'log2', ARGSET_1_HALF_FLOAT),
    ('Sqrt', 'sqrt', ARGSET_1_FP),
    ('InverseSqrt', 'inversesqrt', ARGSET_1_FP),
    # TODO: find out how arguments are passed:
    # TODO: row major/row minor
    ('determinant', '', []),
    ('inverse', '', []),
    # TODO: Correctly implement tests for Modf. See JIRA: CA-388
    ('Modf', '', ARGSET_MODF),
    ('ModfStruct', '', ARGSET_MODF_STRUCT),
    ('FMin', 'min', ARGSET_2_FP),
    ('UMin', 'min', ARGSET_2_UINT),
    ('SMin', 'min', ARGSET_2_INT),
    ('FMax', 'max', ARGSET_2_FP),
    ('UMax', 'max', ARGSET_2_UINT),
    ('SMax', 'max', ARGSET_2_INT),
    ('FClamp', 'clamp', ARGSET_3_FP),
    ('UClamp', 'clamp', ARGSET_3_UINT),
    ('SClamp', 'clamp', ARGSET_3_INT),
    ('FMix', 'mix', ARGSET_3_FP),
    ('Step', 'step', ARGSET_2_FP),
    ('SmoothStep', 'smoothstep', ARGSET_3_FP),
    ('Fma', 'fma', ARGSET_3_FP),
    ('Frexp', '', ARGSET_FREXP),
    ('FrexpStruct', '', ARGSET_FREXP_STRUCT),
    ('Ldexp', 'ldexp', ARGSET_LDEXP),
    ('PackSnorm4x8', 'packSnorm4x8', ARGSET_PACK4x8),
    ('PackUnorm4x8', 'packUnorm4x8', ARGSET_PACK4x8),
    ('PackSnorm2x16', 'packSnorm2x16', ARGSET_PACK2x16),
    ('PackUnorm2x16', 'packUnorm2x16', ARGSET_PACK2x16),
    ('PackHalf2x16', 'packHalf2x16', ARGSET_PACK2x16),
    # dependant on implementation of halves
    ('PackDouble2x32', 'packDouble2x32', ARGSET_PACK2x32),
    ('UnpackSnorm2x16', 'unpackSnorm2x16', ARGSET_UNPACK2x16),
    ('UnpackUnorm2x16', 'unpackUnorm2x16', ARGSET_UNPACK2x16),
    ('UnpackHalf2x16', 'unpackHalf2x16', ARGSET_UNPACK2x16),
    # dependant on implementation of halves
    ('UnpackSnorm4x8', 'unpackSnorm4x8', ARGSET_UNPACK4x8),
    ('UnpackUnorm4x8', 'unpackUnorm4x8', ARGSET_UNPACK4x8),
    ('UnpackDouble2x32', 'unpackDouble2x32', ARGSET_UNPACK2x32),
    ('Length', 'length', ARGSET_LENGTH),
    ('Distance', 'distance', ARGSET_DISTANCE),
    ('Cross', 'cross', ARGSET_CROSS),
    ('Normalize', 'normalize', ARGSET_1_FP),
    ('FaceForward', 'faceforward', ARGSET_3_FP),
    ('Reflect', 'reflect', ARGSET_2_FP),
    ('Refract', 'refract', ARGSET_REFRACT),
    ('FindILsb', 'findLSB', ARGSET_1_UINT),
    ('FindSMsb', 'findMSB', ARGSET_1_INT),
    ('FindUMsb', 'findMSB', ARGSET_FINDUMSB),
    # These are for fragment shaders and are therefor not implemented:
    ('InterpolateAtCentroid', '', []),
    ('InterpolateAtSample', '', []),
    ('InterpolateAtOffset', '', []),
    # These have the same implementation as Fmin FMax and FClamp:
    ('NMin', 'min', ARGSET_2_FP),
    ('NMax', 'max', ARGSET_2_FP),
    ('NClamp', 'clamp', ARGSET_3_FP),
]


def get_test_name_from_op(op):
    ''' Returns instruction name converted into a standard format for use as a
        test name

        Arguments:
            op (string): instruction name

        Returns:
            (string) test name derived from instruction name
    '''
    return '_'.join(findall('[A-Z][a-z]*', op)).lower()


def get_llvm_alloca(glsl_type, is_primitive=True):
    """ Returns the allocation line of a variable in LLVM, depending on the
        type of the variable.

        Arguments:
            glsl_type (string): the GLSL type of the variable to allocate.
            is_primitive (bool): if False, glsl_type is treated as an LLVM type
                (no conversion lookup is performed)

        Returns:
            (string): the allocation of the variable in LLVM-IR.
    """
    if is_primitive:
        ty = LLVM_TYPES[glsl_type]
    else:
        ty = glsl_type
    return '{0} = alloca {1}'.format(ID, ty)


def get_llvm_store_constant(glsl_type,
                            constant,
                            is_primitive=True,
                            addrspace=False):
    """ Returns the store line of a constant in LLVM, depending on the type of
        the constant.

        Arguments:
            glsl_type (string): the GLSL type of the constant to store.
            constant (string): the constant to store.
            is_primitive (bool): if false no type lookups will be performed and
            the string passed as glsl_type will be put directly into the
            returned instruction
            addrsapce (bool): if set the variable being stored will have an
            address space annotation appended to its ID

        Returns:
            (string): the store of the constant in LLVM-IR.
    """

    if is_primitive:
        glsl_type = LLVM_TYPES[glsl_type]

    if addrspace:
        dst_ptr = ' addrspace({{[0-9]}})* ' + ID
    else:
        dst_ptr = '* ' + ID

    return 'store {0} {1}, {0}{2}'.format(glsl_type, constant, dst_ptr)


def get_llvm_store_variable(glsl_type,
                            is_primitive=True,
                            dst_ptr=ID,
                            addrspace=False):
    """ Returns the store line of a variable in LLVM, depending on the type of
        the variable.

        Arguments:
            glsl_type (string): the GLSL type of the variable to store.
            is_primitive (bool): if false no llvm type lookup will be performed
            on glsl_type
            dst_ptr (string): optionally provide the ID of the destination
            pointer
            addrspace (bool): if set the variable being stored will have an
            address space annotation appended to its ID
        Returns:
            (string): the store of the variable in LLVM-IR.
    """

    if is_primitive:
        ty = LLVM_TYPES[glsl_type]
    else:
        ty = glsl_type

    dst_ptr_type = ty

    if addrspace:
        dst_ptr_type += ' addrspace({{[0-9]}})*'
    else:
        dst_ptr_type += '*'

    return 'store {0} {1}, {2} {3}'.format(ty, ID, dst_ptr_type, dst_ptr)


def get_llvm_load(glsl_type, is_primitive=True, addrspace=False, src_ptr=ID):
    """ Returns the load line of a variable in LLVM, depending on the type of
        the variable.

        Arguments:
            glsl_type (string): the GLSL type of the variable to load.
            is_primitive (bool): if false no llvm type lookup will be performed
            on glsl_type
            addrspace (bool): if set an address space qualifier will be added
            to the target pointer
            src_ptr (string): optionally provide the ID of the pointer to load
            from
        Returns:
            (string): the load of the variable in LLVM-IR.
    """

    if is_primitive:
        load_type = LLVM_TYPES[glsl_type]
    else:
        load_type = glsl_type

    if addrspace:
        ptr = ' addrspace({{[0-9]}})* '
    else:
        ptr = '* '

    return '{0} = load {1}, {1}{2}{3}'.format(ID, load_type, ptr, src_ptr)


def get_glsl_scalar_declaration(var, glsl_type):
    ''' Returns a declaration of a scalar in GLSL. The value of the variable is
        a default value, depending on its type, declared in CONSTANTS.

        Arguments:
            var (string): the name of the variable to declare.
            glsl_type (string): the GLSL type of the variable to declare.

        Returns:
            (string): the declaration line of the variable in GLSL.
    '''

    return '{0} {1} = {2};'.format(glsl_type, var, CONSTANTS[glsl_type])


def get_llvm_array_type(size, glsl_type):
    ''' Returns the LLVM-IR type of an array, depending on the GLSL type of its
        components and its size.

        Arguments:
            size (int): the length of the array.
            glsl_type (string): the GLSL type of the array's values.

        Returns:
            (string): the LLVM-IR type of the array.
    '''

    return '[{0} x {1}]'.format(str(size), LLVM_TYPES[glsl_type])


def get_llvm_array_constant(size, glsl_type):
    ''' Returns an LLVM-IR constant of an array, depending on its size and the
        GLSL type of its components. The values of the array's components will
        be set depending on the values set in the CONSTANTS dictionary.

        Arguments:
            size (int): the length of the array.
            glsl_type (string): the GLSL type of the array's values.

        Returns:
            (string): a default LLVM-IR array value.
    '''
    element = '{0} {1}'.format(LLVM_TYPES[glsl_type],
                               LLVM_CONSTANTS[glsl_type])
    return '[' + ', '.join([element] * size) + ']'


def get_glsl_array_declaration(var, size, glsl_type):
    ''' Returns a GLSL declaration of an array, depending on its size and the
        GLSL type of its components. The values of the array's components will
        be set depending on the values set in the CONSTANTS dictionary.

        Arguments:
            var (string): the name of the array to declare.
            size (int): the length of the array.
            glsl_type (string): the GLSL type of the array's values.

        Returns:
            (string): the GLSL declaration of the array.
    '''
    dec = '{0} {1}[{2}] = {0}[{2}]('.format(glsl_type, var, str(size))
    dec += ', '.join([CONSTANTS[glsl_type]] * size)
    return dec + ');'


def get_glsl_vec_constant(vec_width, glsl_type):
    ''' Returns a GLSL vector constant, depending on its size of the GLSL type
        of its components. The values of the array's components will
        be set depending on the values set in the CONSTANTS dictionary.

        Arguments:
            vec_width (int): the size of the vector.
            glsl_type (string): the GLSL type of the vector's values.

        Returns:
            (string): a GLSL vector constant.
    '''
    constant = VEC_TYPES[glsl_type] + str(vec_width) + '('
    constant += ', '.join([CONSTANTS[glsl_type]] * vec_width)
    return constant + ')'


def get_llvm_vec_type(vec_width, glsl_type):
    ''' Returns a LLVM-IR vector type, depending on its size and the GLSL type
        of its components.

        Arguments:
            vec_width (int): the size of the vector.
            glsl_type (string): the GLSL type of the vector's values.

        Returns:
            (string): the LLVM-IR vector type.
    '''
    return '<{0} x {1}>'.format(str(vec_width), LLVM_TYPES[glsl_type])


def get_llvm_vec_constant(vec_width, glsl_type):
    ''' Returns a LLVM-IR vector constant, depending on its size and the GLSL
        type of its components. The values of the vector's components will
        be set depending on the values set in the CONSTANTS dictionary.

        Arguments:
            vec_width (int): the size of the vector.
            glsl_type (string): the GLSL type of the vector's values.

        Returns:
            (string): a GLSL vector constant.
    '''
    element = '%s %s' % (LLVM_TYPES[glsl_type], LLVM_CONSTANTS[glsl_type])
    return '<' + ', '.join([element] * vec_width) + '>'


def get_llvm_binary_op(op, type, lhs, rhs):
    ''' Returns a regex to match calling the given llvm binary instruction on
        the given operands.

        Arguments:
            op (string): the llvm instruction being called
            type (string): the llvm type of the operands
            lhs (string): name or regex to match potential names of the lhs
                          operand
            rhs (string): name or regex to match potential names of the rhs
                          operand

        Returns:
            (string): regex to match the called instruction in a .ll file
    '''

    inst = [ID, '=', op, type, lhs + ',', rhs]

    return ' '.join(inst)


def get_glsl_vec_declaration(var, vec_width, glsl_type):
    ''' Returns a GLSL declaration of a vector, depending on its size and
        the GLSL type of its components.

        Arguments:
            var (string): the name of the vector to declare.
            vec_width (int): the size of the vector.
            glsl_type (string): the GLSL type of the vector's values.

        Returns:
            (string): the LLVM-IR declaration of the vector.
    '''

    vec_type = VEC_TYPES[glsl_type] + str(vec_width)
    vec_constant = get_glsl_vec_constant(vec_width, glsl_type)

    return '{0} {1} = {2};'.format(vec_type, var, vec_constant)


def get_glsl_mat_constant(lines, columns, glsl_type):
    ''' Returns a GLSL matrix constant, depending of its dimensions and the
        GLSL type of its components. The values of the vector's components will
        be set depending on the values set in the CONSTANTS dictionary.

        Arguments:
            lines (int): the number of lines in the matrix.
            columns (int): the number of columns in the matrix.
            glsl_type (string): the GLSL type of the matrix's values.

        Returns:
            (string): a LLVM-IR matrix constant.
    '''
    element = CONSTANTS[glsl_type]
    return '{' + ', '.join(
        ['{' + ', '.join([element] * columns) + '}'] * lines) + '}'


def get_llvm_mat_type(lines, columns, glsl_type):
    ''' Returns a LLVM-IR matrix type, depending on its dimensions and the GLSL
        type of its components. Matrices are represented as arrays of vectors
        since the matrix type doesn't exist in LLVM-IR.

        Arguments:
            lines (int): the number of lines in the matrix.
            columns (int): the number of columns in the matrix.
            glsl_type (string): the GLSL type of the matrix's values.

        Returns:
            (string): the representation of LLVM-IR matrix type.
    '''

    return '[{0} x <{1} x {2}>]'.format(
        str(lines), str(columns), LLVM_TYPES[glsl_type])


def get_llvm_mat_constant(lines, columns, glsl_type):
    ''' Returns a LLVM-IR matrix constant, depending on its dimensions and the
        GLSL type of its components. The values of the matrix's components will
        be set depending on the values set in the CONSTANTS dictionary.

        Arguments:
            lines (int): the number of lines in the matrix.
            columns (int): the number of columns in the matrix.
            glsl_type (string): the GLSL type of the matrix's values.

        Returns:
            (string): a LLVM-IR matrix constant.

    '''
    element = '{0} {1}'.format(LLVM_TYPES[glsl_type],
                               LLVM_CONSTANTS[glsl_type])

    column_type = '<{0} x {1}>'.format(str(columns), LLVM_TYPES[glsl_type])
    return '[' + ', '.join(
        [column_type + '<' + ', '.join([element] * columns) + '>'
         ] * lines) + ']'


def get_glsl_mat_declaration(var, lines, columns, glsl_type):
    ''' Returns a GLSL matrix declaration, depending on its dimensions and the
        GLSL type of its components.

        Arguments:
            var (string): the name of the matrix to declare.
            lines (int): the number of lines in the matrix.
            columns (int): the number of columns in the matrix.
            glsl_type (string): the GLSL type of the matrix's values.

        Returns:
            (string): a GLSL matrix declaration.
    '''

    mat_type = MAT_TYPES[glsl_type] + str(lines) + 'x' + str(columns)
    mat_constant = get_glsl_mat_constant(lines, columns, glsl_type)

    return '{0} {1} = {2};'.format(mat_type, var, mat_constant)


def get_llvm_insert_element(vec_type, vec, elt_type):
    ''' Returns a LLVM insertelement instruction. This instruction basically
        insert a value with, as type, elt_type in a vector vec of type
        vec_type.

        Arguments:
            vec_type (string): the LLVM type of the vector in which the element
                               is inserted.
            vec (string): the ID of the vector in which the element is
                          inserted.
            elt_type (string): the GLSL type of the element to insert.

        Returns:
            (string): the LLVM insertelement instruction.
    '''

    return '{0} = insertelement {1} {2}, {3} {0}, {4} {5}'.format(
        ID, vec_type, vec, LLVM_TYPES[elt_type], LLVM_ANY_INT, NUMBER)


def get_llvm_insert_value(agg_type, agg, val_type):
    ''' Returns an LLVM insertvalue instruction. This is semantically the same
        as insertelement except that it operates on aggregate types instead of
        vectors.

        Arguments:
            agg_type (string): LLVM type of the aggregate type into which the
                               value will be inserted
            agg (string): ID of the aggregate into which the value will be
                          inserted
            val_type (string): LLVM type of the value to be inserted
    '''

    return '{0} = insertvalue {1} {2}, {3} {0}, {4}'.format(
        ID, agg_type, agg, val_type, NUMBER)


def get_llvm_conversion(src_type, dst_type, inst='', primitive=True):
    ''' Returns a regex that matches the expected output from converting
        src_type to dst_type.

        Arguments:
            src_type (string): the LLVM type being converted from
            dst_type (string): the LLVM type being converted to
            inst (string): optional, manually specify the LLVM instruction to
            use for the conversion
            primitive (bool): if this is false no type look-ups will be
            performed, meaning the raw strings passed into src_type and
            dst_type will be put directly into the return instruction

        Returns:
            (string): regex that matches a conversion from src_type to dst_type
    '''

    if not inst:
        inst = CONVERSIONS[src_type][dst_type]

    if primitive:
        src_type = LLVM_TYPES[src_type]
        dst_type = LLVM_TYPES[dst_type]

    return '{0} = {1} {2} {0} to {3}'.format(ID, inst, src_type, dst_type)


def get_llvm_getelementptr(composite_type,
                           indices,
                           addrspace=False,
                           inbounds=False):
    ''' Returns a regex to match the expected output from an OpAccessChain
        (SPIR-V equivelant of gep)

        Arguments:
            composite_type (string): the top level composite type that is being
            indexed into
            indices (string[]): optionally provide a list of indices, or
            regexes to match indices, to be used in the gep
            addrspace (bool): indicates whether this instruction needs an
            addrspace qualifier before the *

        Returns:
            (string): regex
    '''
    ptr = ' addrspace({{[0-9]}})* ' if addrspace else '* '
    inst = 'getelementptr inbounds' if inbounds else 'getelementptr'
    return '{0} = {1} {2}, {2}{3}{0}, i32 0, i32 {4}'.format(
        ID, inst, composite_type, ptr, ', i32 '.join(indices))


def get_llvm_branch(branch_inst):
    ''' Returns a regex to match the expected output from branch and branch
        conditional instructions

        Arguments:
            branch_inst (string): which branch instruction to return
    '''

    return {
        'branch':
        'br label {0}',
        'branch_conditional':
        'br i1 {0}, label {0}, label {0}',
        'switch':
        'switch i32 {0}, label {0} [' + '\n  i32 ' + LLVM_CONSTANTS['int'] +
        ', label {0}\n]'
    }[branch_inst].format(ID)


def get_llvm_select(glsl_type):
    ''' Returns a regex to match an llvm select instruction

        Arguments:
            glsl_type (string): glsl type of the values being selected from
    '''

    constant = LLVM_CONSTANTS[glsl_type]

    llvm_type = LLVM_TYPES[glsl_type]

    return '{0} = select i1 {0}, {1} {2}, {1} {2}'.format(
        ID, llvm_type, constant)


def get_llvm_struct_declaration(types):
    ''' Return a regex matching an LLVM struct declaration with the given types

        Arguments:
            types (string[]):  list of member types in the struct to be
            declared
    '''

    types = ', '.join(types)

    return '{0} = type {{ {1} }}'.format(ID, types)


def get_llvm_main_declaration(args, descriptor_set=False):
    ''' Return a regex matching a spir kernel entry point declaration in LLVM-IR

        Arguments:
            args (string[]): List of entry point argument types
            descriptor_set (bool): Whether the given args represent descriptor
            set bindings (if they don't the buffer sizes array will be omitted)
    '''

    if args:
        args = ', '.join(args)
        if descriptor_set:
            args += ', i32 addrspace(1)*'
    else:
        args = ''

    return 'define spir_kernel void @main({0}) {{'.format(args)


def get_llvm_metadata_node(id):
    ''' Returns a regex to match an LLVM metadata node

        Arguments:
            id - the ID of the metadata node
    '''
    return '!' + id


def get_llvm_function_call(ret, name, args, ids=True, intrinsic=False):
    ''' Returns a regex to match an LLVM function call

        Arguments:
            ret_type - Return type of the function being called
            name - Name of the function being called
            args - List of arguments passed to the function (with llvm types)
            ids - Whether IDs should be added to the list of args (otherwise
                  it is assumed that values have been passed and they will
                  only be separated by commas)
            intrinsic - Whether the called function is an LLVM intrinsic
                        (they don't get the spir_func prefix)

    '''
    if args:
        if ids:
            id_join = ' {0}, '.format(ID)
            args = id_join.join(args) + ' ' + ID
        else:
            args = ', '.join(args)
    else:
        args = ''

    if intrinsic:
        call = 'call'
    else:
        call = 'call spir_func'

    if 'void' in ret:
        return '{0} {1} @{2}({3})'.format(call, ret, name, args)
    return '{0} = {1} {2} @{3}({4})'.format(ID, call, ret, name, args)


def get_llvm_spir_call(ret_type, name, args, named=False):
    ''' Returns a regex to match an LLVM function call to a spir function.
    Shorthand for get_llvm_function_call(ret_type, name, args, named, False)

    Arguments:
        ret (string): Return type of the function being called
        name (string): Name of the function being called
        args (list):  List of arguments passed to the function (with llvm
        types)
        named (bool): If False, a generic ID is appended to each arg
    '''
    return get_llvm_function_call(ret_type, name, args, not named, False)


def get_llvm_spir_func_declaration(ret_type, func_name, arg_types):
    ''' Returns an LLVM function declaration

        Arguments:
            ret_type: Return type of the function being declared
            func_name: Name of the function being declared
            arg_types: List of function argument types (with llvm types)
    '''
    str_declare = 'declare spir_func ' + ret_type + ' @'
    str_declare += func_name + '('
    str_declare += ', '.join(arg_types)
    str_declare += ')'
    return str_declare


def get_builtin_test_name(builtin_name, arg_types):
    ''' Returns the name of the test for a GLSL builtin

        Arguments:
            builtin_name: name of the GLSL SPIR-V builtin being used
            arg_types: List of function argument GLSL types
    '''
    name = ('op_glsl_' + builtin_name + '_' + '_'.join(arg_types))
    return name.replace('*', 'Ptr')


def get_llvm_bitcast(llvm_source_type, llvm_dest_type):
    ''' Returns an LLVM bitcast instruction

        Arguments:
            llvm_source_type: the LLVM type of the source
            llvm_dest_type: the LLVM type of the result
    '''
    return '{0} = bitcast {1} {0} to {2}'.format(ID, llvm_source_type,
                                                 llvm_dest_type)
