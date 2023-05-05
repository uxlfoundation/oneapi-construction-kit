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

import os

from utils import (
    ARRAY_SIZE, BINARY_OPERATORS, BITCASTS, CONVERSIONS, COPYRIGHT,
    GENERIC_CONSTANTS, GLSL_BUILTINS, GLSL_LLVM_TYPES, GLSL_TYPES, ID,
    LLVM_ANY_INT, LLVM_COMPOSITE_INT_CONSTANTS, LLVM_CONSTANTS, LLVM_LABEL,
    LLVM_TYPES, MAT_TYPES, NUMBER, NUMERICAL_TYPES, RUN_FORMAT, TEST_EXT,
    VEC_TYPES, get_builtin_test_name, get_llvm_alloca, get_llvm_array_constant,
    get_llvm_array_type, get_llvm_binary_op, get_llvm_bitcast, get_llvm_branch,
    get_llvm_conversion, get_llvm_function_call, get_llvm_getelementptr,
    get_llvm_insert_element, get_llvm_insert_value, get_llvm_load,
    get_llvm_main_declaration, get_llvm_mat_type, get_llvm_metadata_node,
    get_llvm_select, get_llvm_spir_call, get_llvm_spir_func_declaration,
    get_llvm_store_constant, get_llvm_store_variable,
    get_llvm_struct_declaration, get_llvm_vec_constant, get_llvm_vec_type,
    get_test_name_from_op)

TESTS_PATH = None


def get_full_test_code(code, name):
    '''Append the 'CHECK' tag at the beginning of each line of the LLVM-IR
    code. Also append the Codeplay copyright at the beginning of the file.
    '''

    code.insert(0, '; ModuleID = \'test.module\'')

    for i in range(0, len(code)):
        if code[i] != '':
            code[i] = 'CHECK: ' + code[i]

    code_lines = os.linesep.join(code)

    return (COPYRIGHT + RUN_FORMAT.format(name) + os.linesep + os.linesep +
            code_lines)


def edit_test_file(name, code):
    '''Edit a test file with some LLVM-IR code. Files are edited in the folder
    at TESTS_PATH. The extension of test files are set in TEST_EXT.

    Arguments:
        name (string): the name of the file to edit.
        code (string): The LLVM-IR code to put in the file.
    '''

    path = '%s/%s' % (TESTS_PATH, name + TEST_EXT)
    with open(path, 'w') as test_file:
        test_file.write(get_full_test_code(code, name))


def gen_test_files(output_dir):
    '''Generates every test file.'''
    global TESTS_PATH  # pylint: disable=global-statement
    TESTS_PATH = output_dir

    gen_nop_test()

    for operator in BINARY_OPERATORS:
        for glsl_type in BINARY_OPERATORS[operator]:
            if BINARY_OPERATORS[operator][glsl_type]['vec']:
                for i in range(2, 5):
                    gen_binary_vec_operator_test(i, operator, glsl_type)
                    gen_binary_vec_with_scalar_operator_test(
                        i, operator, glsl_type)

            # TODO: For the moment, only squared matrices are tested, because
            # additive and multiplication operations on matrices are not
            # defined for every types of matrices.
            if BINARY_OPERATORS[operator][glsl_type]['mat']:
                for i in range(2, 5):
                    gen_binary_mat_operator_test(i, i, operator, glsl_type)

            gen_binary_operator_test(operator, glsl_type)

            if 'fcmp' not in BINARY_OPERATORS[operator][glsl_type]['inst']:
                gen_op_spec_constant_op_test(operator, glsl_type)

    for src_type in CONVERSIONS:
        for dst_type in CONVERSIONS[src_type]:
            gen_conversion_operator_test(src_type, dst_type)

    for src_type in BITCASTS:
        gen_conversion_operator_test(src_type, BITCASTS[src_type], 'bitcast')
        # we're using the bitcast type list to generate the kernels, so we can
        # do the same for the tests
        gen_forward_ptr_test(src_type)

    gen_forward_ptr_test('Foo')
    gen_branch_test('bool', 'false', 'branch_conditional',
                    'op_branch_conditional_false')
    gen_branch_test('bool', 'true', 'branch_conditional',
                    'op_branch_conditional_true')
    gen_branch_test('int', '42', 'switch')
    gen_branch_test('int', '0', 'switch', 'op_switch_default')
    gen_overflow_inst_tests()
    gen_fcmp_unord_tests()
    gen_ptr_to_int_test()
    gen_rem_tests()
    gen_misc_tests()
    gen_unary_tests()
    gen_null_constant_tests()
    gen_composite_insert_test()
    gen_phi_test()
    gen_composite_extract_tests()
    gen_vector_shuffle_tests()
    gen_op_unreachable_test()
    gen_debug_info_test()
    gen_loop_merge_test()
    gen_source_continued_test()
    gen_builtin_var_tests()
    gen_any_all_tests()
    gen_inf_nan_test()
    gen_dot_tests()
    gen_mod_tests()
    gen_bitcount_tests()
    gen_kernel_builtin_tests()

    for glsl_type in ['int', 'uint', 'float', 'double']:
        gen_select_test(glsl_type)
        gen_memcpy_tests(glsl_type)
        gen_spec_constant_composite_tests(glsl_type)

    for glsl_type in GLSL_TYPES:
        gen_descriptor_set_test(glsl_type)
        gen_negate_op_test(glsl_type)
        gen_array_access_operator_test(glsl_type)
        gen_array_insert_operator_test(glsl_type)
        gen_function_call_ret_scalar_test(glsl_type)
        gen_function_parameter_test(glsl_type)
        gen_push_constant_test(glsl_type)
        gen_spec_constant_test(glsl_type)
        gen_op_array_length_test(glsl_type)
        for i in range(2, 5):
            gen_function_call_ret_vec_test(i, glsl_type)

    for num_type in NUMERICAL_TYPES:
        gen_vec_extract_test(num_type)
        gen_vec_insert_test(num_type)

    # same logic as in glsl_generator: these instructions work on the same
    # subset of types as any bitwse operator
    for glsl_type in BINARY_OPERATORS['&']:
        gen_bitfield_op_tests(glsl_type)
        for i in range(2, 5):
            gen_bitfield_op_tests(glsl_type, i)

    gen_glsl_builtin_tests()


def gen_nop_test():
    '''Generates the expected LLVM-IR output for the nop test.
    '''

    name = 'op_nop'

    code = ['define spir_kernel void @main()', 'ret void']

    edit_test_file(name, code)


def gen_glsl_builtin_function_float_test(glsl_function, abacus_function):
    '''Generates the expected LLVM-IR output for a test which calls a builtin
        GLSL function which operates on a single float and returns a float.

        Arguments:
            glsl_function (string): the name of the GLSL builtin
            abacus_function (string): the name of the Abacus function
                                      (excluding type suffixes) corresponding
                                      to the GLSL builtin
    '''
    name = 'op_glsl_' + glsl_function + '_float'

    args = [
        'float ' + ID,
    ]
    arg_types = ['float']

    code = [
        get_llvm_main_declaration(['%0 addrspace(1)*', '%1 addrspace(1)*'],
                                  True),
        get_llvm_getelementptr(ID, ['0'], True),
        get_llvm_load('float', True, True),
        get_llvm_spir_call('float', abacus_function + 'f', args, True),
        'ret void',
        get_llvm_spir_func_declaration('float', abacus_function + 'f',
                                       arg_types)
    ]

    edit_test_file(name, code)


def gen_binary_operator_test(operator, glsl_type):
    '''Generates the expected LLVM-IR output for a binary operation test.

        Arguments:
            operator (string): the binary operator.
            glsl_type (string): the GLSL type of both operands.
    '''

    name = ('op_' + BINARY_OPERATORS[operator][glsl_type]['inst'].replace(
        ' ', '_') + '_two_' + glsl_type + '_operands')

    constant = LLVM_CONSTANTS[glsl_type]

    if operator == '==' and glsl_type == 'bool':
        llvm_inst = 'and'
    else:
        llvm_inst = BINARY_OPERATORS[operator][glsl_type]['inst']

    if operator == '%':
        llvm_inst = llvm_inst.replace('mod', 'rem')

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca(glsl_type),
        get_llvm_alloca(glsl_type),
        get_llvm_alloca(BINARY_OPERATORS[operator][glsl_type]['ret']),
        get_llvm_store_constant(glsl_type, constant),
        get_llvm_store_constant(glsl_type, constant),
        get_llvm_load(glsl_type),
        get_llvm_load(glsl_type),
        get_llvm_binary_op(llvm_inst, LLVM_TYPES[glsl_type], ID, ID),
        get_llvm_store_variable(BINARY_OPERATORS[operator][glsl_type]['ret']),
        'ret void'
    ]

    edit_test_file(name, code)


def gen_array_access_operator_test(glsl_type):
    '''Generates the expected LLVM-IR output for an array access operation.

        Arguments:
            glsl_type (string): the GLSL type of the array's components.
    '''

    name = 'op_access_array_' + glsl_type

    array_type = get_llvm_array_type(ARRAY_SIZE, glsl_type)
    array_constant = get_llvm_array_constant(ARRAY_SIZE, glsl_type)

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca('int'),
        get_llvm_alloca(array_type, False),
        get_llvm_alloca(glsl_type),
        get_llvm_store_constant('int', LLVM_CONSTANTS['int']),
        get_llvm_store_constant(array_type, array_constant, False),
        get_llvm_load('int'),
        get_llvm_getelementptr(array_type, [ID]),
        get_llvm_load(glsl_type),
        get_llvm_store_variable(glsl_type), 'ret void'
    ]

    edit_test_file(name, code)


def gen_array_insert_operator_test(glsl_type):
    '''Generates the expected LLVM-IR output for an array insertion operation.

        Arguments:
            glsl_type (string): the GLSL type of the array's components.
    '''

    name = 'op_insert_array_' + glsl_type

    array_type = get_llvm_array_type(ARRAY_SIZE, glsl_type)
    array_constant = get_llvm_array_constant(ARRAY_SIZE, glsl_type)
    constant = LLVM_CONSTANTS[glsl_type]

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca('int'),
        get_llvm_alloca(array_type, False),
        get_llvm_store_constant('int', LLVM_CONSTANTS['int']),
        get_llvm_store_constant(array_type, array_constant, False),
        get_llvm_load('int'),
        get_llvm_getelementptr(array_type, [ID]),
        get_llvm_store_constant(glsl_type, constant), 'ret void'
    ]

    edit_test_file(name, code)


def gen_binary_vec_operator_test(nb_coordinates, operator, glsl_type):
    '''Generates the expected LLVM-IR output for a binary operation between
        two vectors.

        Arguments:
            nb_coordinates (int): the size of the vector.
            operator (string): the binary operator.
            glsl_type (string): the GLSL type of the array's components.
    '''

    vec_type = VEC_TYPES[glsl_type] + str(nb_coordinates)
    name = ('op_' + BINARY_OPERATORS[operator][glsl_type]['inst'].replace(
        ' ', '_') + '_two_' + vec_type + '_' + glsl_type + '_operands')

    vec_type = get_llvm_vec_type(nb_coordinates, glsl_type)
    vec_const = get_llvm_vec_constant(nb_coordinates, glsl_type)
    ret_type = get_llvm_vec_type(nb_coordinates,
                                 BINARY_OPERATORS[operator][glsl_type]['ret'])
    llvm_inst = BINARY_OPERATORS[operator][glsl_type]['inst']

    if operator == '%':
        llvm_inst = llvm_inst.replace('mod', 'rem')

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca(vec_type, False),
        get_llvm_alloca(vec_type, False),
        get_llvm_alloca(ret_type, False),
        get_llvm_store_constant(vec_type, vec_const, False),
        get_llvm_store_constant(vec_type, vec_const, False),
        get_llvm_load(vec_type, False),
        get_llvm_load(vec_type, False),
        get_llvm_binary_op(llvm_inst, ret_type, ID, ID),
        get_llvm_store_variable(vec_type, False), 'ret void'
    ]

    edit_test_file(name, code)


def gen_conversion_operator_test(src_type, dst_type, conversion=''):
    '''Generates the expected LLVM-IR output for a conversion operation.

        Arguments:
            src_type (string): the GLSL type of the variable to be converted.
            dst_type (string): the GLSL type into which the variable is
                               converted.
            conversion (string): optionally provide a conversion operation to
                                 use, this is to facilitate reuse of this
                                 function to generate bitcast tests
    '''

    if conversion:
        name = 'op_' + conversion + '_' + src_type + '_to_' + dst_type
    else:
        name = 'op_convert_' + src_type + '_to_' + dst_type

    constant = LLVM_CONSTANTS[src_type]

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca(src_type),
        get_llvm_alloca(dst_type),
        get_llvm_store_constant(src_type, constant),
        get_llvm_load(src_type),
        get_llvm_conversion(src_type, dst_type, conversion),
        get_llvm_store_variable(dst_type), 'ret void'
    ]

    edit_test_file(name, code)


def gen_binary_vec_with_scalar_operator_test(nb_coordinates, operator,
                                             glsl_type):
    vec_type = VEC_TYPES[glsl_type] + str(nb_coordinates)
    name = ('op_' + BINARY_OPERATORS[operator][glsl_type]['inst'].replace(
        ' ', '_') + '_' + vec_type + '_scalar_' + glsl_type + '_operands')

    ret_type = get_llvm_vec_type(nb_coordinates,
                                 BINARY_OPERATORS[operator][glsl_type]['ret'])
    llvm_vec_type = get_llvm_vec_type(nb_coordinates, glsl_type)
    vec_const = get_llvm_vec_constant(nb_coordinates, glsl_type)

    insts = [get_llvm_insert_element(llvm_vec_type, 'undef', glsl_type)]

    operator_inst = BINARY_OPERATORS[operator][glsl_type]['inst']

    if operator == '%':
        operator_inst = operator_inst.replace('mod', 'rem')

    # OpVectorTimesScalar uses getVectorSplat which generates a vector suffle
    # instead of multiple inserts
    if operator == '*' and ('double' in llvm_vec_type
                            or 'float' in llvm_vec_type):
        shuffle = ID + ' = shufflevector ' + llvm_vec_type + ' ' + ID + \
            ', ' + llvm_vec_type + ' undef, ' + \
            get_llvm_vec_type(nb_coordinates, 'int') + ' zeroinitializer'
        insts.append(shuffle)
    else:
        for _ in range(nb_coordinates - 1):
            insts.append(get_llvm_insert_element(llvm_vec_type, ID, glsl_type))

    constant = LLVM_CONSTANTS[glsl_type]

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca(llvm_vec_type, False),
        get_llvm_alloca(glsl_type),
        get_llvm_alloca(ret_type, False),
        get_llvm_store_constant(llvm_vec_type, vec_const, False),
        get_llvm_store_constant(glsl_type, constant),
        get_llvm_load(llvm_vec_type, False),
        get_llvm_load(glsl_type)
    ]

    code.extend(insts)

    code.extend([
        get_llvm_binary_op(operator_inst, ret_type, ID, ID),
        get_llvm_store_variable(llvm_vec_type, False), 'ret void'
    ])

    edit_test_file(name, code)


# TODO: Fixed matrices operations depending on the operators.
def gen_binary_mat_operator_test(lines, columns, operator, glsl_type):
    '''Generates the expected LLVM-IR output for a binary operation between
        two matrices.

        Arguments:
            lines (int): the number of lines of the matrix.
            column (int): the number of column of the matrix.
            operator (string): the binary operator.
            glsl_type (string): the GLSL type of the matrix's components.
    '''

    mat_type = MAT_TYPES[glsl_type] + str(lines) + 'x' + str(columns)
    name = ('op_' + BINARY_OPERATORS[operator][glsl_type]['inst'].replace(
        ' ', '_') + '_two_' + mat_type + '_' + glsl_type + '_operands')

    code = ['define spir_kernel void @main()']

    # TODO: this works for the element-wise operators (+, -, /) but we need
    # special cases for matrix multiply as SPIR-V has instructions for that

    for i in range(0, columns):
        for _ in range(0, 2):
            mat_type = get_llvm_mat_type(lines, columns, glsl_type)
            extract = get_llvm_binary_op('extractvalue', mat_type, ID, str(i))
            code.append(extract)
        llvm_inst = BINARY_OPERATORS[operator][glsl_type]['inst']
        op = get_llvm_binary_op(llvm_inst,
                                get_llvm_vec_type(lines, glsl_type), ID, ID)
        code.append(op)

    for i in range(0, columns):
        insert = ID + ' = insertvalue ' + get_llvm_mat_type(
            lines, columns, glsl_type) + ' undef, ' + get_llvm_vec_type(
                lines, glsl_type) + ' ' + ID + ', ' + str(i)
        code.append(insert)

    code.append('ret void')

    edit_test_file(name, code)


def gen_function_call_ret_scalar_test(glsl_type):
    '''Generates the expected LLVM-IR output for a function call (which
        returns a scalar) test.

        Arguments:
            glsl_type (string): the function return GLSL type.
    '''

    name = 'func_call_ret_scalar_' + glsl_type

    constant = LLVM_CONSTANTS[glsl_type]

    function_call = '%s = call %s  @func_ret_%s()' % (ID,
                                                      LLVM_TYPES[glsl_type],
                                                      glsl_type)

    function_def = 'define private spir_func ' + LLVM_TYPES[glsl_type] +\
                   ' @func_ret_' + glsl_type + '()'

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca(glsl_type), function_call,
        get_llvm_store_variable(glsl_type), 'ret void', function_def,
        get_llvm_alloca(glsl_type),
        get_llvm_store_constant(glsl_type, constant),
        get_llvm_load(glsl_type), 'ret ' + LLVM_TYPES[glsl_type] + ' ' + ID
    ]

    edit_test_file(name, code)


def gen_function_call_ret_vec_test(nb_coordinates, glsl_type):
    '''Generates the expected LLVM-IR output for a function call (which
        returns a vector) test.

        Arguments:
            nb_coordinates (int): the size of the vector which is returned by
                                  the function.
            glsl_type (string): the GLSL type of the vector's components.
    '''

    name = 'func_call_ret_vec' + str(nb_coordinates) + '_' + glsl_type

    vec_type = get_llvm_vec_type(nb_coordinates, glsl_type)
    vec_const = get_llvm_vec_constant(nb_coordinates, glsl_type)

    function_call = ID + ' = call ' + vec_type + ' @' + name + '()'

    function_def = 'define private spir_func ' + vec_type + ' @' + name + '()'

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca(vec_type, False), function_call,
        get_llvm_store_variable(vec_type, False), 'ret void', function_def,
        get_llvm_alloca(vec_type, False),
        get_llvm_store_constant(vec_type, vec_const, False),
        get_llvm_load(vec_type, False), 'ret ' + vec_type + ' ' + ID
    ]

    edit_test_file(name, code)


def gen_negate_op_test(glsl_type):
    '''Generates the expected LLVM-IR output for the negation operation.

        Arguments:
            glsl_type (string): the GLSL type of the operand to negate.
    '''

    # You can't negate a boolean
    if glsl_type == 'bool':
        return

    zero_constant = {
        'int': '0',
        'uint': '0',
        'float': '-0.000000e+00',
        'double': '-0.000000e+00'
    }[glsl_type]

    name = 'negate_op_' + glsl_type

    constant = LLVM_CONSTANTS[glsl_type]

    llvm_inst = BINARY_OPERATORS['-'][glsl_type]['inst']

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca(glsl_type),
        get_llvm_alloca(glsl_type),
        get_llvm_store_constant(glsl_type, constant),
        get_llvm_load(glsl_type),
        get_llvm_binary_op(llvm_inst, LLVM_TYPES[glsl_type], zero_constant,
                           ID),
        get_llvm_store_variable(glsl_type), 'ret void'
    ]

    edit_test_file(name, code)


def gen_bitfield_op_tests(glsl_type, vec_width=0):
    '''Generates the expected LLVM-IR for calling bitfield instructions.

        Arguments:
            glsl_type (string): GLSL type of the operands to pass
            vec_width (int): if type is a vector, this specifies the number of
                             components in it
    '''

    if vec_width != 0:
        is_primitive = False
        type = get_llvm_vec_type(vec_width, glsl_type)
        llvm_type = type
        const = get_llvm_vec_constant(vec_width, glsl_type)
        extract_name = 'op_bitfield_extract_' + VEC_TYPES[glsl_type] + str(
            vec_width)
        insert_name = 'op_bitfield_insert_' + VEC_TYPES[glsl_type] + str(
            vec_width)
        zero = 'zeroinitializer'
        number = LLVM_COMPOSITE_INT_CONSTANTS['vector']
    else:
        is_primitive = True
        type = glsl_type
        # TODO: overhaul the get_*_type functions so this isn't necessary
        llvm_type = LLVM_TYPES[glsl_type]
        const = LLVM_CONSTANTS[glsl_type]
        extract_name = 'op_bitfield_extract_' + glsl_type
        insert_name = 'op_bitfield_insert_' + glsl_type
        zero = '0'
        number = NUMBER

    extract_code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca(type, is_primitive),
        get_llvm_alloca(type, is_primitive),
        get_llvm_store_constant(type, const, is_primitive),
        get_llvm_load(type, is_primitive),
        get_llvm_binary_op('{{(a|l)shr}}', llvm_type, ID, zero),
        get_llvm_binary_op('and', llvm_type, ID, number),
        get_llvm_store_variable(type, is_primitive), 'ret void'
    ]

    edit_test_file(extract_name, extract_code)

    insert_code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca(type, is_primitive),
        get_llvm_alloca(type, is_primitive),
        get_llvm_alloca(type, is_primitive),
        get_llvm_store_constant(type, const, is_primitive),
        get_llvm_store_constant(type, const, is_primitive),
        get_llvm_load(type, is_primitive),
        get_llvm_load(type, is_primitive),
        get_llvm_binary_op('and', llvm_type, number, ID),
        get_llvm_binary_op('shl', llvm_type, ID, zero),
        get_llvm_binary_op('and', llvm_type, ID, 'undef'),
        get_llvm_binary_op('or', llvm_type, ID, ID),
        get_llvm_store_variable(type, is_primitive), 'ret void'
    ]

    edit_test_file(insert_name, insert_code)


def gen_fcmp_unord_tests():
    '''Generates the expected LLVM-IR output for unordered floating point
        comparison test kernels
    '''

    instructions = {
        'OpFUnordEqual': 'ueq',
        'OpFUnordNotEqual': 'une',
        'OpFUnordLessThan': 'ult',
        'OpFUnordGreaterThan': 'ugt',
        'OpFUnordLessThanEqual': 'ule',
        'OpFUnordGreaterThanEqual': 'uge'
    }

    for instruction in instructions:
        name = get_test_name_from_op(instruction)
        type = 'float'
        constant = LLVM_CONSTANTS[type]
        inst = 'fcmp ' + instructions[instruction]

        code = [
            get_llvm_main_declaration([]),
            get_llvm_alloca(type),
            get_llvm_alloca(type),
            get_llvm_alloca('bool'),
            get_llvm_store_constant(type, constant),
            get_llvm_store_constant(type, constant),
            get_llvm_load(type),
            get_llvm_load(type),
            get_llvm_binary_op(inst, type, ID, ID),
            get_llvm_store_variable('bool'), 'ret void'
        ]

        edit_test_file(name, code)


def gen_ptr_to_int_test():
    '''Generates the expected LLVM-IR output from the pointer integer
        convserion test kernel.
    '''

    name = 'ptr_int_conversions'

    array_type = get_llvm_array_type(2, 'uint')
    array_constant = get_llvm_array_constant(2, 'uint')
    ptr_array_type = array_type + '*'

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca(array_type, False),
        get_llvm_alloca('uint'),
        get_llvm_store_constant(array_type, array_constant, False),
        get_llvm_conversion('bool', 'uint', 'ptrtoint').replace(
            'i1', ptr_array_type),
        get_llvm_binary_op('add', LLVM_TYPES['uint'], '1', ID),
        get_llvm_conversion('uint', 'uint', 'inttoptr').replace(
            'i32,', 'i32* '),
        get_llvm_load('uint'),
        get_llvm_store_variable('uint')
    ]

    edit_test_file(name, code)


def gen_forward_ptr_test(glsl_type):
    '''Generates the expected LLVM-IR output from the forward pointer kernel

        Arguments:
            glsl_type(string): type being forward declared
    '''

    name = 'op_forward_pointer_' + glsl_type

    if 'Foo' in glsl_type:
        llvm_type = ID
    else:
        llvm_type = LLVM_TYPES[glsl_type]

    code = [
        get_llvm_struct_declaration(['i32', 'float', llvm_type + '*']),
        get_llvm_main_declaration([]),
        get_llvm_alloca('float'),
        get_llvm_alloca(ID, False),
        get_llvm_getelementptr(ID, '1', False, True),
        get_llvm_load('float'),
        # apparently we're getting double style e notation for this constant
        get_llvm_binary_op('fadd', LLVM_TYPES['float'],
                           LLVM_CONSTANTS['double'], ID),
        get_llvm_store_variable('float'),
        'ret void'
    ]

    edit_test_file(name, code)


def gen_branch_test(condition_type, constant, branch_inst, name=''):
    '''Generates the expected LLVM-IR output for a conditional branch test
        kernel.

        Arguments:
            condition_type (string): glsl type of the value being evaluated
            constant (string): constant being evaluated
            branch_inst (string): which branch instruction to test, at the
                                  moment the options are switch and
                                  branch_conditional
            name (string): optionally supply a name for the test
    '''

    if not name:
        name = 'op_' + branch_inst

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca(condition_type),
        get_llvm_alloca('int'),
        get_llvm_store_constant(condition_type, constant),
        get_llvm_load(condition_type),
        get_llvm_branch(branch_inst), LLVM_LABEL,
        get_llvm_store_constant('int', '24'),
        get_llvm_branch('branch'), LLVM_LABEL,
        get_llvm_store_constant('int', '42'),
        get_llvm_branch('branch'), LLVM_LABEL,
        get_llvm_load('int'),
        get_llvm_binary_op('xor', LLVM_TYPES['int'], ID, '42'),
        get_llvm_store_variable('int'), 'ret void'
    ]

    edit_test_file(name, code)


def gen_select_test(glsl_type):
    '''Generates the expected LLVM-IR output from the OpSelect kernels

        Arguments:
            glsl_type (string): glsl type of the values being selected from
    '''

    name = 'op_select_' + glsl_type

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca('bool'),
        get_llvm_alloca(glsl_type),
        get_llvm_store_constant('bool', LLVM_CONSTANTS['bool']),
        get_llvm_load('bool'),
        get_llvm_select(glsl_type),
        get_llvm_store_variable(glsl_type), 'ret void'
    ]

    edit_test_file(name, code)


def gen_overflow_inst_tests():
    '''Generates the expected output from the overflow arithmetic instruction
        tests.
    '''

    for instruction in ['OpUMulExtended', 'OpSMulExtended']:
        name = get_test_name_from_op(instruction)

        code = [
            get_llvm_struct_declaration(['i32, i32']),
            get_llvm_main_declaration([]),
            get_llvm_alloca('int'),
            get_llvm_alloca('int'),
            get_llvm_store_constant('int', '42'),
            get_llvm_store_constant('int', '42'),
            get_llvm_load('int'),
            get_llvm_load('int'),
            get_llvm_binary_op('mul', 'i32', ID, ID),
            get_llvm_binary_op('and', 'i32', '65535', ID),
            get_llvm_binary_op('and', 'i32', '-65536', ID),
            get_llvm_insert_value(ID, 'undef', 'i32'),
            get_llvm_insert_value(ID, ID, 'i32'),
            get_llvm_store_variable(ID, False), 'ret void'
        ]

        edit_test_file(name, code)

    intrinsic_insts = {
        'OpIAddCarry': 'llvm.uadd.with.overflow.i32',
        'OpISubBorrow': 'llvm.usub.with.overflow.i32'
    }

    for instruction in intrinsic_insts:
        name = get_test_name_from_op(instruction)

        intrinsic_name = intrinsic_insts[instruction]

        code = [
            get_llvm_struct_declaration(['i32, i32']),
            get_llvm_main_declaration([]),
            get_llvm_alloca('int'),
            get_llvm_alloca('int'),
            get_llvm_store_constant('int', '42'),
            get_llvm_store_constant('int', '42'),
            get_llvm_load('int'),
            get_llvm_load('int'),
            get_llvm_function_call('{ i32, i1 }', intrinsic_name,
                                   ['i32', 'i32'], True, True),
            get_llvm_binary_op('extractvalue', '{ i32, i1 }', ID, '0'),
            get_llvm_insert_value(ID, 'undef', 'i32'),
            get_llvm_binary_op('extractvalue', '{ i32, i1 }', ID, '1'),
            get_llvm_conversion('bool', 'int', 'sext'),
            get_llvm_insert_value(ID, ID, 'i32'),
            get_llvm_store_variable(ID, False), 'ret void'
        ]

        edit_test_file(name, code)


def gen_rem_tests():
    '''Generates the expected LLVM-IR output for the rem test kernels '''

    rem_insts = {
        'OpSRem': 'int',
        'OpFRem': 'float'
        # TODO: generate fmod test in here when it is implemented
    }

    for instruction in rem_insts:
        name = get_test_name_from_op(instruction)

        glsl_type = rem_insts[instruction]

        code = [
            get_llvm_alloca(glsl_type),
            get_llvm_alloca(glsl_type),
            get_llvm_alloca(glsl_type),
            get_llvm_store_constant(glsl_type, LLVM_CONSTANTS[glsl_type]),
            get_llvm_store_constant(glsl_type, LLVM_CONSTANTS[glsl_type]),
            get_llvm_load(glsl_type),
            get_llvm_load(glsl_type),
            get_llvm_binary_op(
                instruction.replace('Op', '').lower(), LLVM_TYPES[glsl_type],
                ID, ID),
            get_llvm_store_variable(glsl_type), 'ret void'
        ]

        edit_test_file(name, code)


def gen_vec_extract_test(glsl_type):
    '''Generates the expected LLVM-IR output from an OpVecExtractDynamic kernel
        with the given type

        Arguments:
            glsl_type (string): type being extracted from the vector
    '''

    name = 'op_vec_extract_v3' + glsl_type

    vec_type = get_llvm_vec_type(3, glsl_type)

    vec_const = get_llvm_vec_constant(3, glsl_type)

    extract_inst = '{0} = extractelement {1} {0}, i32 1'.format(ID, vec_type)

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca(vec_type, False),
        get_llvm_alloca(glsl_type),
        get_llvm_store_constant(vec_type, vec_const, False),
        get_llvm_load(vec_type, False), extract_inst,
        get_llvm_store_variable(glsl_type), 'ret void'
    ]

    edit_test_file(name, code)


def gen_vec_insert_test(glsl_type):
    '''Generates the expected LLVM-IR output from an OpVecInsertDynamic kernel
        with the given type

        Arguments:
            glsl_type (string): type being inserted into the vector
    '''

    name = 'op_vec_insert_v3' + glsl_type

    vec_type = get_llvm_vec_type(3, glsl_type)

    vec_const = get_llvm_vec_constant(3, glsl_type)

    insert_inst = '{0} = insertelement {1} {0}, {2} {3}, i32 1'
    insert_inst = insert_inst.format(ID, vec_type, LLVM_TYPES[glsl_type],
                                     LLVM_CONSTANTS[glsl_type])

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca(vec_type, False),
        get_llvm_alloca(vec_type, False),
        get_llvm_store_constant(vec_type, vec_const, False),
        get_llvm_load(vec_type, False), insert_inst,
        get_llvm_store_variable(vec_type, False), 'ret void'
    ]

    edit_test_file(name, code)


def gen_descriptor_set_test(glsl_type):
    '''Generates the expected LLVM-IR output from a descriptor set test kernel

        Arguments:
            glsl_type (string): glsl type of the input variable
    '''

    name = 'descriptor_set_' + glsl_type

    # glslang generates an i32 from uniform bool for some reason, also == true
    # generates a != false
    if glsl_type == 'bool':
        glsl_type = 'int'
        llvm_type = LLVM_TYPES[glsl_type]
        cmp_inst = get_llvm_binary_op(
            BINARY_OPERATORS['!='][glsl_type]['inst'], llvm_type, ID, '0')
    else:
        llvm_type = LLVM_TYPES[glsl_type]
        cmp_inst = get_llvm_binary_op(
            BINARY_OPERATORS['=='][glsl_type]['inst'], llvm_type, ID,
            LLVM_CONSTANTS[glsl_type])

    block_type = '{0} = type {{ {1} }}'.format(ID, LLVM_TYPES[glsl_type])

    code = [
        block_type,
        get_llvm_alloca('bool'),
        get_llvm_getelementptr(ID, '0').replace('}}*', '}} addrspace(1)*'),
        get_llvm_load(glsl_type).replace(
            llvm_type + '*', llvm_type + ' addrspace(1)*'), cmp_inst,
        get_llvm_store_variable('bool')
    ]

    edit_test_file(name, code)


def gen_memcpy_tests(glsl_type):
    '''Generates the expected LLVM-IR output from the OpCopyMemory kernels

        Arguments:
            glsl_type (string): type of the variable to copy
    '''

    name = 'op_copy_memory_' + glsl_type

    llvm_type = LLVM_TYPES[glsl_type]

    # TODO: replace with whatever we end up generating for this

    memcpy = ('call void @llvm.memcpy.p0i8.p0i8.i{{{{32|64}}}}' +
              '({0} {1}, {0} {1}'.format(
                  'i8*', ID) + 'i{{32|64}} {{(4|8)}}, i32 0, i1 false)')[0]

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca(glsl_type),
        get_llvm_alloca(glsl_type),
        get_llvm_store_constant(glsl_type, LLVM_CONSTANTS[glsl_type]),
        get_llvm_conversion(llvm_type + '*', 'i8*', 'bitcast', False),
        get_llvm_conversion(llvm_type + '*', 'i8*', 'bitcast', False), memcpy,
        'ret void'
    ]

    sized_code = list(code)

    edit_test_file(name, code)

    # the code generated should be the same, but a different instruction is
    # called in the kernel
    name = 'op_copy_memory_sized_' + glsl_type

    edit_test_file(name, sized_code)


def gen_function_parameter_test(glsl_type):
    '''Generates the expected LLVM-IR output from the OpFunctionParameter test
        kernel corresponding to the given GLSL type.

        Arguments:
            glsl_type (string): glsl type of the function parameters in the
            kernel being tested
    '''

    name = 'op_function_parameter_' + glsl_type

    llvm_type = LLVM_TYPES[glsl_type]

    if glsl_type == 'bool':
        eq_inst = 'and'
    else:
        eq_inst = BINARY_OPERATORS['=='][glsl_type]['inst']

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca('bool'),
        get_llvm_alloca(glsl_type),
        get_llvm_alloca(glsl_type),
        get_llvm_store_constant(glsl_type, LLVM_CONSTANTS[glsl_type]),
        get_llvm_store_constant(glsl_type, LLVM_CONSTANTS[glsl_type]),
        '{0} = call i1 @foo({1}* {0}, {1}* {0})'.format(ID, llvm_type),
        get_llvm_store_variable('bool'), 'ret void',
        'define private spir_func i1 @foo({0}*, {0}*) {{'.format(llvm_type),
        get_llvm_load(glsl_type),
        get_llvm_load(glsl_type),
        get_llvm_binary_op(eq_inst, llvm_type, ID, ID), 'ret i1 ' + ID
    ]

    edit_test_file(name, code)


def gen_unary_tests():
    '''Generates the expected LLVM-IR output from the kernels which test
        various unary instructions
    '''

    instructions = {
        'OpNot': {
            'type': 'int',
            'inst': '{0} = xor i32 {0}, -1'.format(ID)
        },
        'OpLogicalNot': {
            'type': 'bool',
            'inst': '{0} = xor i1 {0}, true'.format(ID)
        }
    }

    for instruction in instructions:
        name = get_test_name_from_op(instruction)

        ty = instructions[instruction]['type']
        llvm_inst = instructions[instruction]['inst']

        code = [
            get_llvm_main_declaration([]),
            get_llvm_alloca(ty),
            get_llvm_alloca(ty),
            get_llvm_store_constant(ty, LLVM_CONSTANTS[ty]),
            get_llvm_load(ty), llvm_inst,
            get_llvm_store_variable(ty), 'ret void'
        ]

        edit_test_file(name, code)


def gen_push_constant_test(glsl_type):
    '''Generates the expected LLVM-IR output from the push constant test kernels

        Arguments:
            glsl_type (string): type of the push constant being tested
    '''

    name = 'push_constant_' + glsl_type

    if glsl_type == 'bool':
        # glslang is to blame for this
        code = [
            get_llvm_struct_declaration(['i32']),
            get_llvm_main_declaration(['%0 addrspace(2)*']),
            get_llvm_alloca(glsl_type),
            get_llvm_getelementptr(ID, ['0'], True),
            get_llvm_load('int', True, True),
            get_llvm_binary_op('icmp ne', 'i32', ID, '0'),
            get_llvm_store_variable(glsl_type), 'ret void'
        ]
    else:
        code = [
            get_llvm_struct_declaration([LLVM_TYPES[glsl_type]]),
            get_llvm_main_declaration(['%0 addrspace(2)*']),
            get_llvm_alloca(glsl_type),
            get_llvm_getelementptr(ID, ['0'], True),
            get_llvm_load(glsl_type, True, True),
            get_llvm_store_variable(glsl_type), 'ret void'
        ]

    edit_test_file(name, code)


def gen_spec_constant_test(glsl_type):
    '''Generates the expected llvm output from a spec constant kernel testing
        a given type

        Arguments:
            glsl_type (string): glsl type being tested
    '''

    if glsl_type == 'bool':
        code = [
            get_llvm_struct_declaration([LLVM_TYPES[glsl_type]]),
            get_llvm_main_declaration(['%0 addrspace(1)*'], True),
            get_llvm_getelementptr(ID, '0').replace('}}*', '}} addrspace(1)*'),
            get_llvm_store_constant(glsl_type, '{{true|false}}', True, True),
            'ret void'
        ]

        name = 'op_spec_constant_true_bool'
        true_code = list(code)

        edit_test_file(name, true_code)

        name = 'op_spec_constant_false_bool'
        false_code = list(code)

        edit_test_file(name, false_code)
    else:
        name = 'op_spec_constant_' + glsl_type
        constant = LLVM_CONSTANTS[glsl_type]
        code = [
            get_llvm_struct_declaration([LLVM_TYPES[glsl_type]]),
            get_llvm_main_declaration(['%0 addrspace(1)*'], True),
            get_llvm_getelementptr(ID, '0').replace('}}*', '}} addrspace(1)*'),
            get_llvm_store_constant(glsl_type, constant, True, True),
            'ret void'
        ]
        edit_test_file(name, code)


def gen_spec_constant_composite_tests(glsl_type):
    '''Generates a series of tests which contain the expected LLVM-IR output
        from the corresponding test kernels.

        Arguments:
            glsl_type (string): glsl type being tested
    '''

    vec_name = 'op_spec_constant_composite_{0}_vec'.format(glsl_type)

    vec_type = get_llvm_vec_type(3, glsl_type)
    vec_const = get_llvm_vec_constant(3, glsl_type)

    vec_code = [
        get_llvm_struct_declaration([vec_type]),
        get_llvm_main_declaration(['%0 addrspace(1)*'], True),
        get_llvm_getelementptr(ID, '0').replace('}}*', '}} addrspace(1)*'),
        get_llvm_store_constant(vec_type, vec_const, False, True), 'ret void'
    ]

    edit_test_file(vec_name, vec_code)

    struct_name = 'op_spec_constant_composite_{0}_struct'.format(glsl_type)
    struct_const = '{{ {0} {1} }}'.format(LLVM_TYPES[glsl_type],
                                          LLVM_CONSTANTS[glsl_type])

    struct_code = [
        get_llvm_struct_declaration([ID]),
        get_llvm_struct_declaration([LLVM_TYPES[glsl_type]]),
        get_llvm_main_declaration(['%0 addrspace(1)*'], True),
        get_llvm_getelementptr(ID, '0').replace('}}*', '}} addrspace(1)*'),
        get_llvm_store_constant(ID, struct_const, False, True), 'ret void'
    ]

    edit_test_file(struct_name, struct_code)

    array_name = 'op_spec_constant_composite_{0}_array'.format(glsl_type)

    array_type = get_llvm_array_type(4, glsl_type)
    array_constant = get_llvm_array_constant(4, glsl_type)

    array_code = [
        get_llvm_struct_declaration([array_type]),
        get_llvm_main_declaration(['%0 addrspace(1)*'], True),
        get_llvm_getelementptr(ID, '0').replace('}}*', '}} addrspace(1)*'),
        get_llvm_store_constant(array_type, array_constant, False, True),
        'ret void'
    ]

    edit_test_file(array_name, array_code)


def gen_op_spec_constant_op_test(operator, glsl_type):
    '''Generates the expected LLVM-IR output from an OpSpecConstantOp test
        kernel with the given operator and type.

        Arguments:
            operator (string): The additional operation being performed by
            OpSpecConstantOp
            glsl_type (string): GLSL type of the operands being provided to
            OpSpecConstantOp
    '''

    inst = BINARY_OPERATORS[operator][glsl_type]['inst'].replace(' ', '_')
    name = 'op_spec_constant_op_{0}_{1}'.format(inst, glsl_type)

    ret_type = BINARY_OPERATORS[operator][glsl_type]['ret']

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca(ret_type),
        get_llvm_store_constant(ret_type, GENERIC_CONSTANTS[ret_type]),
        'ret void'
    ]

    edit_test_file(name, code)


def gen_misc_tests():
    '''Generates test files for one off test kernels, which generally exist to
        test for specific features or bugs
    '''

    name = 'op_spec_constant_deps'

    code = [
        get_llvm_struct_declaration(['<2 x i32>']),
        get_llvm_main_declaration([]),
        get_llvm_alloca(ID, False),
        get_llvm_store_constant(ID, '{ <2 x i32> <i32 84, i32 84> }', False),
        'ret void'
    ]

    edit_test_file(name, code)


def gen_null_constant_tests():
    '''Generates the expected LLVM-IR output from the OpConstantNull test
        kernels
    '''

    null_constants = {
        'int': {
            'type': 'i32',
            'constant': '0'
        },
        'float': {
            'type': 'float',
            'constant': '0.000000e+00'
        },
        'ptr': {
            'type': 'i32*',
            'constant': 'null'
        },
        'struct': {
            'type': ID,
            'constant': 'zeroinitializer'
        },
        'vec': {
            'type': '<3 x i32>',
            'constant': 'zeroinitializer'
        }
    }

    for ty in null_constants:
        name = 'op_constant_null_{0}'.format(ty)

        llvm_type = null_constants[ty]['type']
        constant = null_constants[ty]['constant']

        code = [
            get_llvm_main_declaration([]),
            get_llvm_alloca(llvm_type, False),
            get_llvm_store_constant(llvm_type, constant, False), 'ret void'
        ]

        if ty == 'struct':
            code.insert(0, get_llvm_struct_declaration(['i32']))

        edit_test_file(name, code)


def gen_composite_insert_test():
    '''Generates the expected LLVM-IR output from the OpCompositeInsert kernels
    '''

    types = {'array': '[3 x i32]', 'struct': ID, 'vector': '<3 x i32>'}

    for ty in types:
        name = 'op_composite_insert_{0}'.format(ty)

        constant = LLVM_COMPOSITE_INT_CONSTANTS[ty]

        code = [
            get_llvm_main_declaration([]),
            get_llvm_alloca(types[ty], False),
            get_llvm_store_constant(types[ty], constant, False), 'ret void'
        ]

        if ty == 'struct':
            code.insert(0, get_llvm_struct_declaration(['i32', 'i32', 'i32']))

        edit_test_file(name, code)


def gen_composite_extract_tests():
    '''Generates the expected LLVM-IR output from the OpCompositeExtract test
        kernels '''

    for vec_width in range(2, 5):
        name = 'op_composite_extract_vec{0}'.format(vec_width)

        vec_type = get_llvm_vec_type(vec_width, 'int')
        constant = get_llvm_vec_constant(vec_width, 'int')
        instruction = '{0} = extractelement {1} {0}, i64 1'.format(
            ID, vec_type)

        code = [
            get_llvm_alloca(vec_type, False),
            get_llvm_alloca('int'),
            get_llvm_store_constant(vec_type, constant, False),
            get_llvm_load(vec_type, False), instruction,
            get_llvm_store_variable('int')
        ]

        edit_test_file(name, code)


def gen_phi_test():
    '''Generates the expected LLVM-IR output from the OpPhi test kernel '''

    name = 'op_phi'

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca('bool'),
        get_llvm_alloca('int'),
        get_llvm_store_constant('bool', 'false'),
        get_llvm_load('bool'),
        get_llvm_branch('branch_conditional'), LLVM_LABEL,
        get_llvm_branch('branch'), LLVM_LABEL,
        get_llvm_branch('branch'), LLVM_LABEL,
        '{0} = phi i32 [ 50, %2 ], [ 8, %3 ]'.format(ID),
        get_llvm_store_variable('int'), 'ret void'
    ]

    edit_test_file(name, code)


def gen_vector_shuffle_tests():
    '''Generates the expected LLVM-IR output from the OpShuffleVector test
        kernels
    '''

    for glsl_type in NUMERICAL_TYPES:
        name = 'op_vector_shuffle_{0}'.format(glsl_type)

        vec_type = get_llvm_vec_type(3, glsl_type)

        vec_const = get_llvm_vec_constant(3, glsl_type)

        code = [
            get_llvm_main_declaration([]),
            get_llvm_alloca(vec_type, False),
            get_llvm_store_constant(vec_type, vec_const, False), 'ret void'
        ]

        # copy code the first time otherwise when we put it through the edit
        # file function the second time all the filecheck annotation will get
        # added again
        edit_test_file(name, list(code))

        # since all the values are constants the swizzle kernels produce the
        # same output, they just take an undef as the second vector to perform
        # a swizzle operation on one vector instead of a shuffle on two
        name = 'op_vector_swizzle_{0}'.format(glsl_type)

        edit_test_file(name, code)


def gen_op_unreachable_test():
    '''Generates the expected LLVM-IR output from the OpUnreachable test kernel
    '''

    name = 'op_unreachable'

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca('int'),
        get_llvm_store_constant('int', '42'),
        get_llvm_load('int'),
        get_llvm_branch('switch'), LLVM_LABEL,
        get_llvm_load('int'),
        get_llvm_binary_op('add', 'i32', ID, '42'),
        get_llvm_store_variable('int'),
        get_llvm_branch('branch'), LLVM_LABEL, 'unreachable', LLVM_LABEL,
        'ret void'
    ]

    edit_test_file(name, code)


def gen_debug_info_test():
    '''Generates the expected LLVM-IR output from the debug info test kernel'''

    name = 'debug_info'

    dbg_suffix = ', !dbg !{{[0-9]+}}'

    node_ref = '!{{[0-9]+}} = '

    # TODO: turn each entry in this dict into a function if we end up with
    # another test that looks for metadata
    md_nodes = {
        'file': ('!DIFile(filename: "{{[A-Za-z._]+}}", directory: '
                 '"{{[A-Za-z0-9/\\:_!\\$%@ \\-]+}}")'),
        'node':
        '!{{!?.*}}',
        'location':
        ('!DILocation(line: {{[0-9]+}}, column: {{[0-9]+}}, scope: '
         '!{{[0-9]+}})'),
        'block': ('distinct !DILexicalBlock(scope: !{{[0-9]+}}, file: '
                  '!{{[0-9]+}}, line: {{[0-9]+}}, column: {{[0-9]+}})'),
        'compile_unit': ('distinct !DICompileUnit(language: DW_LANG_OpenCL, '
                         'file: !{{[0-9]+}},'
                         '{{( producer: "Codeplay SPIR-V  translator",)?}} '
                         'isOptimized: false, runtimeVersion: 0, '
                         'emissionKind: {{[0-9A-Za-z]+}}, enums: !{{[0-9]+}}'
                         '{{(, subprograms: ![0-9]+)?}})'),
        'subprogram': ('distinct !DISubprogram(name: "main", linkageName: '
                       '"main", scope: null, file: !{{[0-9]+}}, line: '
                       '{{[0-9]+}}, type: !{{[0-9]+}}, isLocal: true, '
                       'isDefinition: true, scopeLine: 1, isOptimized: '
                       'false, {{(unit: ![0-9], )?}}variables: !{{[0-9]+}})'),
        'subroutine_type':
        '!DISubroutineType(types: !{{[0-9]+}})'
    }

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca('bool') + dbg_suffix,
        get_llvm_store_constant('bool', 'false') + dbg_suffix,
        get_llvm_alloca('int') + dbg_suffix,
        get_llvm_store_constant('int', '42') + dbg_suffix,
        get_llvm_load('bool') + dbg_suffix,
        get_llvm_branch('branch_conditional') + dbg_suffix, LLVM_LABEL,
        get_llvm_load('int') + dbg_suffix,
        get_llvm_binary_op('add', 'i32', ID, '42') + dbg_suffix,
        get_llvm_store_variable('int') + dbg_suffix,
        get_llvm_branch('branch') + dbg_suffix, LLVM_LABEL, 'ret void',
        '!llvm.dbg.cu = !{!{{[0-9]+}}}', '!llvm.ident = !{!{{[0-9]+}}}',
        node_ref + md_nodes['compile_unit'], node_ref + md_nodes['file'],
        node_ref + md_nodes['node'], node_ref + md_nodes['location'],
        node_ref + md_nodes['block'], node_ref + md_nodes['subprogram'],
        node_ref + md_nodes['subroutine_type'],
        node_ref + md_nodes['location'], node_ref + md_nodes['location'],
        node_ref + md_nodes['location'], node_ref + md_nodes['block'],
        node_ref + md_nodes['location']
    ]

    edit_test_file(name, code)


def gen_loop_merge_test():
    '''Generates the expected LLVM-IR output from the OpLoopMerge test kernel
    '''

    name = 'op_loop_merge'

    loop_md = ', !llvm.loop ' + get_llvm_metadata_node('1')
    branch_weight_md = ', !prof ' + get_llvm_metadata_node('2')

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca('int'),
        get_llvm_store_constant('int', '42'),
        get_llvm_branch('branch') + loop_md, LLVM_LABEL,
        get_llvm_branch('branch'), LLVM_LABEL,
        get_llvm_load('int'),
        get_llvm_binary_op('icmp sgt', 'i32', ID, '0'),
        get_llvm_branch('branch_conditional') + branch_weight_md, LLVM_LABEL,
        get_llvm_load('int'),
        get_llvm_binary_op('sub', 'i32', ID, '1'),
        get_llvm_store_variable('int'),
        get_llvm_branch('branch'), LLVM_LABEL,
        get_llvm_branch('branch'), LLVM_LABEL, 'ret void',
        '!llvm.ident = !{!0}',
        get_llvm_metadata_node('0') + ' = ' +
        '!{!"Source language: GLSL, Version: 450"}',
        get_llvm_metadata_node('1') + ' = ' + '!{!"llvm.loop.unroll.enable"}',
        get_llvm_metadata_node('2') + ' = ' +
        '!{!"branch_weights", i32 5, i32 2}'
    ]

    edit_test_file(name, code)


def gen_op_array_length_test(glsl_type):
    '''Generates the expected LLVM-IR output from the OpArrayLength test kernels

        Arguments:
            glsl_type (string): glsl type being tested
    '''

    name = 'op_array_length_{0}'.format(glsl_type)

    code = [
        get_llvm_main_declaration(['%0 addrspace(1)*'], True),
        get_llvm_alloca('int'),
        get_llvm_getelementptr('i32', ['42'], True).replace(', i32 42', ''),
        get_llvm_binary_op('sub', 'i32', ID, '4'),
        get_llvm_binary_op('udiv', 'i32', ID, '{{4|8}}'),
        get_llvm_store_variable('int'), 'ret void'
    ]

    edit_test_file(name, code)


def gen_source_continued_test():
    '''Generates the expected LLVM-IR output for the source_continued test'''

    name = 'op_source_continued'

    ident0 = ('!{!"Source language: GLSL, Version: 450, '
              'Source file: fakeShaderName.comp\\0D\\0A'
              'Test A Test B"}')

    ident1 = ('!{!"Source language: ESSL, Version: 100, '
              'Source file: otherShader.comp\\0D\\0A'
              'Test C"}')

    code = [
        'define spir_kernel void @main() {', 'ret void',
        '!llvm.ident = !{!0, !1}',
        get_llvm_metadata_node('0') + ' = ' + ident0,
        get_llvm_metadata_node('1') + ' = ' + ident1
    ]

    edit_test_file(name, code)


def gen_builtin_var_tests():
    '''Generates the expected LLVM IR output for the builtin variable test
        kernels.
    '''

    builtin_vars = {
        'gl_NumWorkGroups': '__mux_get_num_groups',
        'gl_WorkGroupID': '__mux_get_group_id',
        'gl_LocalInvocationID': '__mux_get_local_id',
        'gl_GlobalInvocationID': '__mux_get_global_id'
    }

    for builtin_var in builtin_vars:
        name = 'builtin_var_{0}'.format(builtin_var)

        mangled_func_name = builtin_vars[builtin_var]

        code = [
            'declare spir_func {0} @{1}(i32)'.format(LLVM_ANY_INT,
                                                     mangled_func_name),
            get_llvm_main_declaration([]),
            get_llvm_alloca('<3 x i32>', False),
            get_llvm_function_call(LLVM_ANY_INT, mangled_func_name, ['i32 0'],
                                   False),
            get_llvm_insert_element('<3 x i32>', 'undef', 'int'),
            get_llvm_function_call(LLVM_ANY_INT, mangled_func_name, ['i32 1'],
                                   False),
            get_llvm_insert_element('<3 x i32>', ID, 'int'),
            get_llvm_function_call(LLVM_ANY_INT, mangled_func_name, ['i32 2'],
                                   False),
            get_llvm_insert_element('<3 x i32>', ID, 'int'),
            get_llvm_store_variable('<3 x i32>', False),
            get_llvm_alloca('int'),
            get_llvm_getelementptr('<3 x i32>', ['0']),
            get_llvm_load('int'),
            get_llvm_store_variable('int'), 'ret void'
        ]

        edit_test_file(name, code)

    name = 'builtin_var_gl_LocalInvocationIndex'

    code = [
        'declare spir_func {0} @__mux_get_local_id(i32)'.format(LLVM_ANY_INT),
        get_llvm_main_declaration([]),
        get_llvm_alloca('int'),
        get_llvm_function_call(LLVM_ANY_INT, '__mux_get_local_id', ['i32 0'],
                               False),
        get_llvm_function_call(LLVM_ANY_INT, '__mux_get_local_id', ['i32 1'],
                               False),
        get_llvm_function_call(LLVM_ANY_INT, '__mux_get_local_id', ['i32 2'],
                               False),
        get_llvm_binary_op('mul', 'i32', ID, NUMBER),
        get_llvm_binary_op('mul', 'i32', ID, NUMBER),
        get_llvm_binary_op('add', 'i32', ID, ID),
        get_llvm_binary_op('add', 'i32', ID, ID),
        get_llvm_store_variable('int'),
        get_llvm_alloca('int'),
        get_llvm_load('int'),
        get_llvm_store_variable('int'), 'ret void'
    ]

    edit_test_file(name, code)


def generate_Fract_IR_dictionary():
    '''Generates the IR to match the implementation of the GLSL Fract extended
        instruction.

        Returns:
            (dictionary): dictionary matching the argument types to the
                expected IR
    '''
    type_suffixes = {
        'float': 'fPf',
        'vec2': 'Dv2_fPS_',
        'vec3': 'Dv3_fPS_',
        'vec4': 'Dv4_fPS_',
        'double': 'dPd',
        'dvec2': 'Dv2_dPS_',
        'dvec3': 'Dv3_dPS_',
        'dvec4': 'Dv4_dPS_',
    }

    return_dictionary = {}

    for argtype, suffix in type_suffixes.items():
        # Get corresponding LLVM type
        llvm_arg_type = GLSL_LLVM_TYPES[argtype]

        # Calls into the builtins fract have two paramaters: only the first one
        # is used (the second one is a pointer which the abacus builtin uses to
        # store the whole number part)
        function_arg_types = (llvm_arg_type, llvm_arg_type + '*')

        ir_list = [
            get_llvm_alloca(llvm_arg_type, False),
            get_llvm_spir_call(llvm_arg_type, '_Z5fract' + suffix,
                               function_arg_types),
            get_llvm_store_variable(llvm_arg_type, False, ID, True),
            'ret void',
            get_llvm_spir_func_declaration(llvm_arg_type, '_Z5fract' + suffix,
                                           function_arg_types),
        ]

        return_dictionary[(argtype, )] = ir_list

    return return_dictionary


# Dictionary which stores LLVM code for builtins which are implemented as more
# than a simple function call. Any IR for a manually implemented function is
# located at GLSL_MANUAL_BUILTINS_LLVM[<glsl func name>][<list of arg types>]
# if it exists, otherwise it is assumed to be a function call.
GLSL_MANUAL_BUILTINS_LLVM = {
    'Fract': generate_Fract_IR_dictionary(),
    'PackSnorm4x8': {
        ('vec4', ): [
            get_llvm_spir_call('<4 x i8>',
                               '_Z29codeplay_pack_normalize_char4Dv4_f',
                               ['<4 x float>'], False),
            get_llvm_bitcast('<4 x i8>', 'i32'),
        ],
    },
    'PackUnorm4x8': {
        ('vec4', ): [
            get_llvm_spir_call('<4 x i8>',
                               '_Z30codeplay_pack_normalize_uchar4Dv4_f',
                               ['<4 x float>'], False),
            get_llvm_bitcast('<4 x i8>', 'i32'),
        ],
    },
    'PackSnorm2x16': {
        ('vec2', ): [
            get_llvm_spir_call('<2 x i16>',
                               '_Z30codeplay_pack_normalize_short2Dv2_f',
                               ['<2 x float>'], False),
            get_llvm_bitcast('<2 x i16>', 'i32'),
        ],
    },
    'PackUnorm2x16': {
        ('vec2', ): [
            get_llvm_spir_call('<2 x i16>',
                               '_Z31codeplay_pack_normalize_ushort2Dv2_f',
                               ['<2 x float>'], False),
            get_llvm_bitcast('<2 x i16>', 'i32')
        ],
    },
    'PackHalf2x16': {
        ('vec2', ): [
            get_llvm_spir_call('<2 x i16>',
                               '_Z19codeplay_pack_half2Dv2_f',
                               ['<2 x float>'], False),
            get_llvm_bitcast('<2 x i16>', 'i32')
        ],
    },
    'PackDouble2x32': {
        ('uvec2', ): [
            get_llvm_bitcast('<2 x i32>', 'double'),
        ],
    },
    'UnpackDouble2x32': {
        ('double', ): [
            get_llvm_bitcast('double', '<2 x i32>'),
        ],
    },
    'UnpackSnorm2x16': {
        ('uint', ): [
            get_llvm_bitcast('i32', '<2 x i16>'),
            get_llvm_spir_call('<2 x float>',
                               '_Z25codeplay_unpack_normalizeDv2_s',
                               ['<2 x i16>'], False),
        ],
    },
    'UnpackUnorm2x16': {
        ('uint', ): [
            get_llvm_bitcast('i32', '<2 x i16>'),
            get_llvm_spir_call('<2 x float>',
                               '_Z25codeplay_unpack_normalizeDv2_t',
                               ['<2 x i16>'], False),
        ],
    },
    'UnpackHalf2x16': {
        ('uint', ): [
            get_llvm_bitcast('i32', '<2 x i16>'),
            get_llvm_spir_call('<2 x float>',
                               '_Z21codeplay_unpack_half2Dv2_s',
                               ['<2 x i16>'], False),
        ],
    },
    'UnpackSnorm4x8': {
        ('uint', ): [
            get_llvm_bitcast('i32', '<4 x i8>'),
            get_llvm_spir_call('<4 x float>',
                               '_Z25codeplay_unpack_normalizeDv4_c',
                               ['<4 x i8>'], False),
        ],
    },
    'UnpackUnorm4x8': {
        ('uint', ): [
            get_llvm_bitcast('i32', '<4 x i8>'),
            get_llvm_spir_call('<4 x float>',
                               '_Z25codeplay_unpack_normalizeDv4_h',
                               ['<4 x i8>'], False),
        ],
    },
    'SSign': {
        ('int', ): [
            get_llvm_spir_call('i32', '_Z5clampiii',
                               ['i32 ' + ID, 'i32 -1', 'i32 1'], True),
        ],
        ('ivec2', ): [
            get_llvm_spir_call('<2 x i32>', '_Z5clampDv2_iS_S_', [
                '<2 x i32> ' + ID, '<2 x i32> <i32 -1, i32 -1>',
                '<2 x i32> <i32 1, i32 1>'
            ], True),
        ],
        ('ivec3', ): [
            get_llvm_spir_call('<3 x i32>', '_Z5clampDv3_iS_S_', [
                '<3 x i32> ' + ID, '<3 x i32> <i32 -1, i32 -1, i32 -1>',
                '<3 x i32> <i32 1, i32 1, i32 1>'
            ], True),
        ],
        ('ivec4', ): [
            get_llvm_spir_call('<4 x i32>', '_Z5clampDv4_iS_S_', [
                '<4 x i32> ' + ID,
                '<4 x i32> <i32 -1, i32 -1, i32 -1, i32 -1>',
                '<4 x i32> <i32 1, i32 1, i32 1, i32 1>'
            ], True),
        ],
    },
    'FrexpStruct': {
        ('float', ): [
            get_llvm_alloca('i32', False),
            get_llvm_spir_call('float', '_Z5frexpfPi', ['float', 'i32*'],
                               False),
            get_llvm_insert_value(ID, 'undef', 'float'),
            get_llvm_load('i32', False),
            get_llvm_insert_value(ID, ID, 'i32'),
            get_llvm_store_variable(ID, False, ID, True),
        ],
        ('vec2', ): [
            get_llvm_alloca('<2 x i32>', False),
            get_llvm_spir_call('<2 x float>', '_Z5frexpDv2_fPDv2_i',
                               ['<2 x float>', '<2 x i32>*'], False),
            get_llvm_insert_value(ID, 'undef', '<2 x float>'),
            get_llvm_load('<2 x i32>', False),
            get_llvm_insert_value(ID, ID, '<2 x i32>'),
            get_llvm_store_variable(ID, False, ID, True),
        ],
        ('vec3', ): [
            get_llvm_alloca('<3 x i32>', False),
            get_llvm_spir_call('<3 x float>', '_Z5frexpDv3_fPDv3_i',
                               ['<3 x float>', '<3 x i32>*'], False),
            get_llvm_insert_value(ID, 'undef', '<3 x float>'),
            get_llvm_load('<3 x i32>', False),
            get_llvm_insert_value(ID, ID, '<3 x i32>'),
            get_llvm_store_variable(ID, False, ID, True)
        ],
        ('vec4', ): [
            get_llvm_alloca('<4 x i32>', False),
            get_llvm_spir_call('<4 x float>', '_Z5frexpDv4_fPDv4_i',
                               ['<4 x float>', '<4 x i32>*'], False),
            get_llvm_insert_value(ID, 'undef', '<4 x float>'),
            get_llvm_load('<4 x i32>', False),
            get_llvm_insert_value(ID, ID, '<4 x i32>'),
            get_llvm_store_variable(ID, False, ID, True)
        ],
        ('double', ): [
            get_llvm_alloca('i32', False),
            get_llvm_spir_call('double', '_Z5frexpdPi', ['double', 'i32*'],
                               False),
            get_llvm_insert_value(ID, 'undef', 'double'),
            get_llvm_load('i32', False),
            get_llvm_insert_value(ID, ID, 'i32'),
            get_llvm_store_variable(ID, False, ID, True),
        ],
        ('dvec2', ): [
            get_llvm_alloca('<2 x i32>', False),
            get_llvm_spir_call('<2 x double>', '_Z5frexpDv2_dPDv2_i',
                               ['<2 x double>', '<2 x i32>*'], False),
            get_llvm_insert_value(ID, 'undef', '<2 x double>'),
            get_llvm_load('<2 x i32>', False),
            get_llvm_insert_value(ID, ID, '<2 x i32>'),
            get_llvm_store_variable(ID, False, ID, True),
        ],
        ('dvec3', ): [
            get_llvm_alloca('<3 x i32>', False),
            get_llvm_spir_call('<3 x double>', '_Z5frexpDv3_dPDv3_i',
                               ['<3 x double>', '<3 x i32>*'], False),
            get_llvm_insert_value(ID, 'undef', '<3 x double>'),
            get_llvm_load('<3 x i32>', False),
            get_llvm_insert_value(ID, ID, '<3 x i32>'),
            get_llvm_store_variable(ID, False, ID, True)
        ],
        ('dvec4', ): [
            get_llvm_alloca('<4 x i32>', False),
            get_llvm_spir_call('<4 x double>', '_Z5frexpDv4_dPDv4_i',
                               ['<4 x double>', '<4 x i32>*'], False),
            get_llvm_insert_value(ID, 'undef', '<4 x double>'),
            get_llvm_load('<4 x i32>', False),
            get_llvm_insert_value(ID, ID, '<4 x i32>'),
            get_llvm_store_variable(ID, False, ID, True)
        ],
    },
    'ModfStruct': {
        ('float', ): [
            get_llvm_alloca('float', False),
            get_llvm_spir_call('float', '_Z4modffPf', ['float', 'float*'],
                               False),
            get_llvm_insert_value(ID, 'undef', 'float'),
            get_llvm_load('float', False),
            get_llvm_insert_value(ID, ID, 'float'),
            get_llvm_store_variable(ID, False, ID, True),
        ],
        ('vec2', ): [
            get_llvm_alloca('<2 x float>', False),
            get_llvm_spir_call('<2 x float>', '_Z4modfDv2_fPS_',
                               ['<2 x float>', '<2 x float>*'], False),
            get_llvm_insert_value(ID, 'undef', '<2 x float>'),
            get_llvm_load('<2 x float>', False),
            get_llvm_insert_value(ID, ID, '<2 x float>'),
            get_llvm_store_variable(ID, False, ID, True),
        ],
        ('vec3', ): [
            get_llvm_alloca('<3 x float>', False),
            get_llvm_spir_call('<3 x float>', '_Z4modfDv3_fPS_',
                               ['<3 x float>', '<3 x float>*'], False),
            get_llvm_insert_value(ID, 'undef', '<3 x float>'),
            get_llvm_load('<3 x float>', False),
            get_llvm_insert_value(ID, ID, '<3 x float>'),
            get_llvm_store_variable(ID, False, ID, True)
        ],
        ('vec4', ): [
            get_llvm_alloca('<4 x float>', False),
            get_llvm_spir_call('<4 x float>', '_Z4modfDv4_fPS_',
                               ['<4 x float>', '<4 x float>*'], False),
            get_llvm_insert_value(ID, 'undef', '<4 x float>'),
            get_llvm_load('<4 x float>', False),
            get_llvm_insert_value(ID, ID, '<4 x float>'),
            get_llvm_store_variable(ID, False, ID, True)
        ],
        ('double', ): [
            get_llvm_alloca('double', False),
            get_llvm_spir_call('double', '_Z4modfdPd', ['double', 'double*'],
                               False),
            get_llvm_insert_value(ID, 'undef', 'double'),
            get_llvm_load('double', False),
            get_llvm_insert_value(ID, ID, 'double'),
            get_llvm_store_variable(ID, False, ID, True),
        ],
        ('dvec2', ): [
            get_llvm_alloca('<2 x double>', False),
            get_llvm_spir_call('<2 x double>', '_Z4modfDv2_dPS_',
                               ['<2 x double>', '<2 x double>*'], False),
            get_llvm_insert_value(ID, 'undef', '<2 x double>'),
            get_llvm_load('<2 x double>', False),
            get_llvm_insert_value(ID, ID, '<2 x double>'),
            get_llvm_store_variable(ID, False, ID, True),
        ],
        ('dvec3', ): [
            get_llvm_alloca('<3 x double>', False),
            get_llvm_spir_call('<3 x double>', '_Z4modfDv3_dPS_',
                               ['<3 x double>', '<3 x double>*'], False),
            get_llvm_insert_value(ID, 'undef', '<3 x double>'),
            get_llvm_load('<3 x double>', False),
            get_llvm_insert_value(ID, ID, '<3 x double>'),
            get_llvm_store_variable(ID, False, ID, True)
        ],
        ('dvec4', ): [
            get_llvm_alloca('<4 x double>', False),
            get_llvm_spir_call('<4 x double>', '_Z4modfDv4_dPS_',
                               ['<4 x double>', '<4 x double>*'], False),
            get_llvm_insert_value(ID, 'undef', '<4 x double>'),
            get_llvm_load('<4 x double>', False),
            get_llvm_insert_value(ID, ID, '<4 x double>'),
            get_llvm_store_variable(ID, False, ID, True)
        ],
    },
}

# Mapping between the builtin function and corresponding builtin function to
# call
FUNCTION_MAPPINGS = {
    'Round': '_Z5round',
    'RoundEven': '_Z4rint',
    'Trunc': '_Z5trunc',
    'FAbs': '_Z4fabs',
    'SAbs': '_Z3abs',
    'FSign': '_Z4sign',
    'SSign': '',
    'Floor': '_Z5floor',
    'Ceil': '_Z4ceil',
    'Fract': '_Z5fract',
    'Radians': '_Z7radians',
    'Degrees': '_Z7degrees',
    'Sin': '_Z3sin',
    'Cos': '_Z3cos',
    'Tan': '_Z3tan',
    'Asin': '_Z4asin',
    'Acos': '_Z4acos',
    'Atan': '_Z4atan',
    'Sinh': '_Z4sinh',
    'Cosh': '_Z4cosh',
    'Tanh': '_Z4tanh',
    'Asinh': '_Z5asinh',
    'Acosh': '_Z5acosh',
    'Atanh': '_Z5atanh',
    'Atan2': '_Z5atan2',
    'Pow': '_Z3pow',
    'Exp': '_Z3exp',
    'Log': '_Z3log',
    'Exp2': '_Z4exp2',
    'Log2': '_Z4log2',
    'Sqrt': '_Z4sqrt',
    'InverseSqrt': '_Z5rsqrt',
    'Determinant': '',
    'MatrixInverse': '',
    'Modf': '_Z4modf',
    'ModfStruct': '',
    'FMin': '_Z4fmin',
    'UMin': '_Z3min',
    'SMin': '_Z3min',
    'FMax': '_Z4fmax',
    'UMax': '_Z3max',
    'SMax': '_Z3max',
    'FClamp': '_Z5clamp',
    'UClamp': '_Z5clamp',
    'SClamp': '_Z5clamp',
    'FMix': '_Z3mix',
    'IMix': '',  # <- not in standard
    'Step': '_Z4step',
    'SmoothStep': '_Z10smoothstep',
    'Fma': '_Z3fma',
    'Frexp': '_Z5frexp',
    'FrexpStruct': '',
    'Ldexp': '_Z5ldexp',
    # TODO: corresponding builtin calls for pack/unpacks
    'PackSnorm4x8': '',
    'PackUnorm4x8': '',
    'PackSnorm2x16': '',
    'PackUnorm2x16': '',
    'PackHalf2x16': '',
    'PackDouble2x32': '',
    'UnpackSnorm2x16': '',
    'UnpackUnorm2x16': '',
    'UnpackHalf2x16': '',
    'UnpackSnorm4x8': '',
    'UnpackUnorm4x8': '',
    'UnpackDouble2x32': '',
    'Length': '_Z6length',
    'Distance': '_Z8distance',
    'Cross': '_Z5cross',
    'Normalize': '_Z9normalize',
    'FaceForward': '_Z21codeplay_face_forward',
    'Reflect': '_Z16codeplay_reflect',
    'Refract': '_Z16codeplay_refract',
    'FindILsb': '_Z17codeplay_find_lsb',
    'FindSMsb': '_Z17codeplay_find_msb',
    'FindUMsb': '_Z17codeplay_find_msb',
    # Interpolation commands for fragment shaders only which are not supported
    'InterpolateAtCentroid': '',
    'InterpolateAtSample': '',
    'InterpolateAtOffset': '',
    # These map the same as FMax/FMin/FClamp
    'NMin': '_Z4fmin',
    'NMax': '_Z4fmax',
    'NClamp': '_Z5clamp'
}


def gen_glsl_builtin_tests():
    '''Generates tests for all GLSL builtin functions
    '''
    for builtin in GLSL_BUILTINS:
        builtin_name = builtin[0]
        argsets = builtin[2]
        for set in argsets:
            return_type = set[0]
            arg_types = set[1]
            func_suffix = set[2]

            name = get_builtin_test_name(builtin_name, arg_types)

            # Get the argument types in LLVM IR form
            llvm_arg_types = [GLSL_LLVM_TYPES[x] for x in arg_types]

            # All tested kernels are assumed to consist of a series of loads,
            # one per variable in the order the variables are declared,
            # followed by the implementation of the builtin.

            # Pointer types can be ignored
            llvm_load_arg_types = [x for x in llvm_arg_types if x[-1] != '*']

            # Generate a load for each variable
            code = [get_llvm_load(x, False, True) for x in llvm_load_arg_types]

            # Most builtins insert function calls, but certain builtins may be
            # implemented differently
            if (builtin_name in GLSL_MANUAL_BUILTINS_LLVM
                    and arg_types in GLSL_MANUAL_BUILTINS_LLVM[builtin_name]):
                code += GLSL_MANUAL_BUILTINS_LLVM[builtin_name][arg_types]
            else:
                # Generate the mangled name:
                func_name = FUNCTION_MAPPINGS[builtin_name] + func_suffix

                llvm_return_type = GLSL_LLVM_TYPES[return_type]

                code += [
                    get_llvm_spir_call(llvm_return_type, func_name,
                                       llvm_arg_types, False)
                ]
                code += [
                    get_llvm_store_variable(llvm_return_type, False, ID, True),
                    'ret void',
                ]
                code += [
                    get_llvm_spir_func_declaration(llvm_return_type, func_name,
                                                   llvm_arg_types)
                ]

            edit_test_file(name, code)


def gen_any_all_tests():
    '''Generates the expected LLVM IR output from the any and all builtin
        test kernels
    '''

    builtins = {'any': '_Z3anyDv{0}_i', 'all': '_Z3allDv{0}_i'}

    for builtin in builtins:
        for width in range(2, 5):
            vec_type = get_llvm_vec_type(width, 'bool')
            vec_constant = get_llvm_vec_constant(width, 'bool')

            # these builtins return their bool result in the form of an
            # i32 which must then be truncated
            int_vec_type = get_llvm_vec_type(width, 'int')
            mangled_builtin = builtins[builtin].format(str(width))

            name = 'op_{0}_bvec{1}'.format(builtin, width)

            code = [
                get_llvm_main_declaration([]),
                get_llvm_alloca(vec_type, False),
                get_llvm_alloca('bool'),
                get_llvm_store_constant(vec_type, vec_constant, False),
                get_llvm_load(vec_type, False),
                get_llvm_conversion(vec_type, int_vec_type, 'sext', False),
                get_llvm_spir_call('i32', mangled_builtin, [int_vec_type]),
                get_llvm_conversion('int', 'bool', 'trunc'),
                get_llvm_store_variable('bool'), 'ret void',
                get_llvm_spir_func_declaration('i32', mangled_builtin,
                                               [int_vec_type])
            ]

            edit_test_file(name, code)


def gen_inf_nan_test():
    '''Generates the expected LLVM IR output from the inf and nan builtin
        test kernels
    '''

    builtins = {
        'inf': {
            'float': '_Z5isinff',
            'double': '_Z5isinfd'
        },
        'nan': {
            'float': '_Z5isnanf',
            'double': '_Z5isnand'
        }
    }

    for builtin in builtins:
        for type in ['float', 'double']:
            if type == 'float':
                builtin_res = 'int'
            else:
                builtin_res = 'long'

            mangled_name = builtins[builtin][type]
            mangled_name = builtins[builtin][type]

            name = 'op_is{0}_{1}'.format(builtin, type)

            code = [
                get_llvm_alloca(type),
                get_llvm_alloca('bool'),
                get_llvm_store_constant(type, LLVM_CONSTANTS[type]),
                get_llvm_load(type),
                get_llvm_spir_call(LLVM_TYPES[builtin_res], mangled_name,
                                   [type]),
                get_llvm_conversion(builtin_res, 'bool', 'trunc'),
                get_llvm_store_variable('bool'), 'ret void',
                get_llvm_spir_func_declaration(LLVM_TYPES[builtin_res],
                                               mangled_name, [type])
            ]

            edit_test_file(name, code)


def gen_dot_tests():
    '''Generates the expected LLVM-IR output from the dot builtin kernels
    '''

    builtins = {'float': '_Z3dotDv{0}_fS_', 'double': '_Z3dotDv{0}_dS_'}

    for width in range(2, 5):
        for type in ['double', 'float']:
            name = 'op_dot_{0}{1}'.format(VEC_TYPES[type], width)

            vec_type = get_llvm_vec_type(width, type)
            vec_constant = get_llvm_vec_constant(width, type)

            mangled_name = builtins[type].format(str(width))

            code = [
                get_llvm_alloca(vec_type, False),
                get_llvm_alloca(vec_type, False),
                get_llvm_alloca(type),
                get_llvm_store_constant(vec_type, vec_constant, False),
                get_llvm_store_constant(vec_type, vec_constant, False),
                get_llvm_load(vec_type, False),
                get_llvm_load(vec_type, False),
                get_llvm_spir_call(type, mangled_name, [vec_type, vec_type]),
                'ret void',
                get_llvm_spir_func_declaration(type, mangled_name,
                                               [vec_type, vec_type])
            ]

            edit_test_file(name, code)


def gen_mod_tests():
    '''Generates the expected LLVM-IR output from the fmod builtin kernels
    '''

    builtins = {'float': '_Z4fmodff', 'double': '_Z4fmoddd'}

    for type in ['float', 'double']:
        name = 'op_fmod_' + type

        mangled_name = builtins[type]

        code = [
            get_llvm_main_declaration([]),
            get_llvm_alloca(type),
            get_llvm_alloca(type),
            get_llvm_alloca(type),
            get_llvm_store_constant(type, LLVM_CONSTANTS[type]),
            get_llvm_store_constant(type, LLVM_CONSTANTS[type]),
            get_llvm_load(type),
            get_llvm_load(type),
            get_llvm_spir_call(type, mangled_name, [type, type]),
            get_llvm_store_variable(type), 'ret void',
            get_llvm_spir_func_declaration(type, mangled_name, [type, type])
        ]

        edit_test_file(name, code)


def gen_bitcount_tests():
    '''Generates the expected LLVM-IR output from the bitcount builtin kernels
    '''

    name = 'op_bitcount_int'

    mangled_name = '_Z8popcounti'

    code = [
        get_llvm_main_declaration([]),
        get_llvm_alloca('int'),
        get_llvm_alloca('int'),
        get_llvm_store_constant('int', LLVM_CONSTANTS['int']),
        get_llvm_load('int'),
        get_llvm_spir_call('i32', mangled_name, ['i32']),
        get_llvm_store_variable('int'), 'ret void',
        get_llvm_spir_func_declaration('i32', mangled_name, ['i32'])
    ]

    edit_test_file(name, code)

    builtin = '_Z8popcountDv{0}_i'

    for width in range(2, 5):
        name = 'op_bitcount_ivec' + str(width)

        vec_type = get_llvm_vec_type(width, 'int')
        vec_constant = get_llvm_vec_constant(width, 'int')

        mangled_name = builtin.format(str(width))

        code = [
            get_llvm_alloca(vec_type, False),
            get_llvm_alloca(vec_type, False),
            get_llvm_store_constant(vec_type, vec_constant, False),
            get_llvm_load(vec_type, False),
            get_llvm_spir_call(vec_type, mangled_name, [vec_type]),
            get_llvm_store_variable(vec_type, False), 'ret void',
            get_llvm_spir_func_declaration(vec_type, mangled_name, [vec_type])
        ]

        edit_test_file(name, code)


def gen_kernel_builtin_tests():
    '''Generates the expected LLVM-IR output from the kernel capability abacus
    builtin instruction test kernels'''

    builtins = {
        'OpIsFinite': '_Z8isfinitef',
        'OpIsNormal': '_Z8isnormalf',
        'OpSignBitSet': '_Z7signbitf',
        'OpUnordered': '_Z11isunorderedff',
        'OpOrdered': '_Z9isorderedff',
        'OpLessOrGreater': '_Z13islessgreaterff'
    }

    for inst in ['OpIsFinite', 'OpIsNormal', 'OpSignBitSet']:
        name = get_test_name_from_op(inst)

        mangled_name = builtins[inst]

        code = [
            get_llvm_main_declaration([]),
            get_llvm_alloca('float'),
            get_llvm_alloca('bool'),
            get_llvm_store_constant('float', LLVM_CONSTANTS['float']),
            get_llvm_load('float'),
            get_llvm_spir_call('i32', mangled_name, ['float']),
            get_llvm_conversion('int', 'bool', 'trunc'),
            get_llvm_store_variable('bool'), 'ret void',
            get_llvm_spir_func_declaration('i32', mangled_name, ['float'])
        ]

        edit_test_file(name, code)

    for inst in ['OpUnordered', 'OpOrdered', 'OpLessOrGreater']:
        name = get_test_name_from_op(inst)

        mangled_name = builtins[inst]

        code = [
            get_llvm_main_declaration([]),
            get_llvm_alloca('float'),
            get_llvm_alloca('float'),
            get_llvm_alloca('bool'),
            get_llvm_store_constant('float', LLVM_CONSTANTS['float']),
            get_llvm_store_constant('float', LLVM_CONSTANTS['float']),
            get_llvm_load('float'),
            get_llvm_load('float'),
            get_llvm_spir_call('i32', mangled_name, ['float', 'float']),
            get_llvm_conversion('int', 'bool', 'trunc'),
            get_llvm_store_variable('bool'), 'ret void',
            get_llvm_spir_func_declaration('i32', mangled_name,
                                           ['float', 'float'])
        ]

        edit_test_file(name, code)
