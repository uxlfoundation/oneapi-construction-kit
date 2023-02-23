# pylint: disable=missing-docstring,too-many-lines,relative-import

# Copyright (C) Codeplay Software Limited. All Rights Reserved.

import posixpath

from utils import (ARRAY_SIZE, BINARY_OPERATORS, CONSTANTS, CONVERSIONS,
                   COPYRIGHT, GLSL_BUILTINS, GLSL_EXT, GLSL_TYPES, MAT_TYPES,
                   VEC_TYPES, get_builtin_test_name,
                   get_glsl_array_declaration, get_glsl_mat_declaration,
                   get_glsl_scalar_declaration, get_glsl_vec_declaration)

FILES = []
OUTPUT_DIR = None


def edit_glsl_file(name, code):
    ''' Edit a GLSL file with some GLSL code. Files are edited in the folder at
        OUTPUT_DIR. The extension of GLSL files are set in GLSL_EXT.

    Arguments:
        name (string): the name of the file to edit.
        code (list): The GLSL code to put in the file.
    '''

    code_string = COPYRIGHT + '\n'.join(code)

    path = posixpath.join(OUTPUT_DIR, name + GLSL_EXT)
    with open(path, 'w') as glsl_file:
        glsl_file.write(code_string)
    FILES.append(path)


def gen_glsl_files(output_dir):
    ''' Generates every GLSL file. '''
    global OUTPUT_DIR  # pylint: disable=global-statement
    OUTPUT_DIR = output_dir

    for operator in BINARY_OPERATORS:
        for glsl_type in BINARY_OPERATORS[operator]:
            if BINARY_OPERATORS[operator][glsl_type]['vec']:
                for i in range(2, 5):
                    gen_binary_vec_operator_glsl(i, operator, glsl_type)
                    gen_binary_vec_with_scalar_operator_glsl(
                        i, operator, glsl_type)

            # TODO: For the moment, only squared matrices are tested, because
            # additive and multiplication operations on matrices are not
            # defined for every types of matrices.
            if BINARY_OPERATORS[operator][glsl_type]['mat']:
                for i in range(2, 5):
                    gen_binary_mat_operator_glsl(i, i, operator, glsl_type)

            gen_binary_operator_glsl(operator, glsl_type)
            if (not glsl_type == 'float') and (not glsl_type == 'double'):
                gen_spec_constant_op_glsl(operator, glsl_type)

    for src_type in CONVERSIONS:
        for dst_type in CONVERSIONS[src_type]:
            gen_conversion_operator_glsl(src_type, dst_type)

    for glsl_type in GLSL_TYPES:
        gen_descriptor_set_glsl(glsl_type)
        gen_negate_op_glsl(glsl_type)
        gen_array_access_operator_glsl(glsl_type)
        gen_array_insert_operator_glsl(glsl_type)
        gen_function_call_ret_scalar_glsl(glsl_type)
        gen_function_parameter_glsl(glsl_type)
        gen_push_constant_glsl(glsl_type)
        gen_runtime_array_glsl(glsl_type)
        for i in range(2, 5):
            gen_function_call_ret_vec_glsl(i, glsl_type)

    # bitfield operations work on the same subset of scalar types as any other
    # bitwise operator, so we can iterate through the correct types by using
    # the list already created for one of them
    for glsl_type in BINARY_OPERATORS['&']:
        gen_bitfield_glsl(glsl_type)

    gen_switch_glsl()
    gen_debug_info_glsl()
    gen_builtin_var_glsl()
    gen_any_all_glsl()
    gen_nan_inf_glsl()
    gen_dot_glsl()
    gen_fmod_glsl()
    gen_bitcount_glsl()

    # Generate builtin calls
    for builtin in GLSL_BUILTINS:
        # Exclude tests which are generated in spvasm_generator:
        if builtin[1] != '':
            gen_glsl_builtin_multiple(builtin[0], builtin[1], builtin[2])

    return FILES


def gen_glsl_builtins(builtin_name, glsl_function_name, result_ty, arg_types):
    ''' Generates GLSL which calls a builtin function

        Arguments:
            glsl_function_name (string): corresponding GLSL builtin function
            result_ty (string): the GLSL result type of the function
            arg_types (list of string): argument GLSL types
    '''
    name = get_builtin_test_name(builtin_name, arg_types)

    # Generate variables for each argument
    code = ['#version 450', 'layout (std430, set=0, binding=0) buffer inA {']

    # List to store argument variable names
    arguments = []

    # Declare variables for each function argument
    argNo = 0
    for type in arg_types:
        code.append(type + ' arg' + str(argNo) + ';')
        arguments.append('arg' + str(argNo))
        argNo += 1
    code.append('};')

    code.append('layout (std430, set=0, binding=1) buffer outR {' + result_ty +
                ' res;};')
    code.append('void main() {')

    function_call = glsl_function_name + '(' + ','.join(arguments) + ')'

    code.append('res = ' + function_call + ';')
    code.append('return;')
    code.append('}')

    edit_glsl_file(name, code)


def gen_glsl_builtin_multiple(builtin_name, glsl_function_name,
                              type_combo_array):
    '''Generates multiple GLSL source files which call a builtin function with
    different sets of argument types.

    Arguments:
        builtin_name (string): name of the builtin GLSL SPIR-V instruction
        glsl_function_name (string): corresponding GLSL builtin function
        type_combo_array (list of list): list of argument type lists
    '''
    for argument_type_set in type_combo_array:
        gen_glsl_builtins(builtin_name, glsl_function_name,
                          argument_type_set[0], argument_type_set[1])


def gen_binary_operator_glsl(operator, glsl_type):
    ''' Generates GLSL code which contains a binary operation in a main
        function. First, the two operands are declared, then the binary
        operation result is stored in a variable. Then generates the expecting
        LLVM-IR translation.

        Arguments:
            operator (string): the binary operator.
            glsl_type (string): the GLSL type of both operands.
    '''

    name = ('op_' + BINARY_OPERATORS[operator][glsl_type]['inst'].replace(
        ' ', '_') + '_two_' + glsl_type + '_operands')

    code = [
        '#version 450', 'void main() {',
        get_glsl_scalar_declaration('a', glsl_type),
        get_glsl_scalar_declaration('b', glsl_type),
        BINARY_OPERATORS[operator][glsl_type]['ret'] + ' c = a ' + operator +
        ' b;', '}'
    ]

    edit_glsl_file(name, code)


def gen_array_access_operator_glsl(glsl_type):
    ''' Generates GLSL code which contains an access to an array. First the
        array is declared, then, the access operation result is stored in a
        variable. Then generates the expecting LLVM-IR translation.

        Arguments:
            glsl_type (string): the GLSL type of the array's components.
    '''

    name = 'op_access_array_' + glsl_type

    code = [
        '#version 450', 'void main() {',
        get_glsl_scalar_declaration('a', 'int'),
        get_glsl_array_declaration('b', ARRAY_SIZE, glsl_type),
        glsl_type + ' c = b[a];', '}'
    ]

    edit_glsl_file(name, code)


def gen_array_insert_operator_glsl(glsl_type):
    ''' Generates GLSL code which contains an insertion into an array. First
        the array is declared, then, the insertion operation result is
        performed. Finally, generates the expecting LLVM-IR translation.

        Arguments:
            glsl_type (string): the GLSL type of the array's components.
    '''

    name = 'op_insert_array_' + glsl_type

    code = [
        '#version 450', 'void main() {',
        get_glsl_scalar_declaration('a', 'int'),
        get_glsl_array_declaration('b', ARRAY_SIZE, glsl_type),
        'b[a] = ' + CONSTANTS[glsl_type] + ';', '}'
    ]

    edit_glsl_file(name, code)


def gen_binary_vec_operator_glsl(nb_coordinates, operator, glsl_type):
    ''' Generates GLSL code which contains a binary operation between two
        vectors in a main function. First, the two operands are declared, then
        the binary operation result is stored in a variable. Then generates the
        expecting LLVM-IR translation.

        Arguments:
            nb_coordinates (int): the size of the vector.
            operator (string): the binary operator.
            glsl_type (string): the GLSL type of the vector's components.
    '''

    vec_type = VEC_TYPES[glsl_type] + str(nb_coordinates)
    name = ('op_' + BINARY_OPERATORS[operator][glsl_type]['inst'].replace(
        ' ', '_') + '_two_' + vec_type + '_' + glsl_type + '_operands')

    ret_type = BINARY_OPERATORS[operator][glsl_type]['ret']
    if ret_type != 'bool':
        ret_type = vec_type

    code = [
        '#version 450', 'void main() {',
        get_glsl_vec_declaration('a', nb_coordinates, glsl_type),
        get_glsl_vec_declaration('b', nb_coordinates, glsl_type),
        ret_type + ' c = a ' + operator + ' b;', '}'
    ]

    edit_glsl_file(name, code)


def gen_conversion_operator_glsl(src_type, dst_type):
    ''' Generates GLSL code which contains a conversion operation in a main
        function. First, the variable to be converted in declared, then the
        conversion operation is performed. Finally, generates the
        expecting LLVM-IR translation.

        Arguments:
            src_type (string): the GLSL type of the variable to be converted.
            dst_type (string): the GLSL type in which the variable is
                               converted.
    '''

    # glsl doesn't have longs so these are hand-written spvasm files
    if 'long' in src_type or 'long' in dst_type:
        return

    name = 'op_convert_' + src_type + '_to_' + dst_type

    code = [
        '#version 450', 'void main() {',
        get_glsl_scalar_declaration('a', src_type),
        dst_type + ' b = ' + dst_type + '(a);', '}'
    ]

    edit_glsl_file(name, code)


def gen_binary_vec_with_scalar_operator_glsl(nb_coordinates, operator, type):
    ''' Generates GLSL code which contains a binary operation between a vector
        and a scalar in a main function. First, the two operands are declared,
        then the result of the operation is stored in a variable. Finally,
        generates the expecting LLVM-IR translation.

        Arguments:
            nb_coordinates (int): the number of component of the vector.
            operator (string): the binary operator.
            type (string): the GLSL type of the vector operand.
    '''

    vec_type = VEC_TYPES[type] + str(nb_coordinates)
    name = 'op_%s_%s_scalar_%s_operands' % (
        BINARY_OPERATORS[operator][type]['inst'].replace(' ', '_'), vec_type,
        type)

    ret_type = BINARY_OPERATORS[operator][type]['ret']
    if ret_type != 'bool':
        ret_type = vec_type

    code = [
        '#version 450', 'void main() {',
        get_glsl_vec_declaration('a', nb_coordinates, type),
        get_glsl_scalar_declaration('b', type),
        ret_type + ' c = a ' + operator + ' b;', '}'
    ]

    edit_glsl_file(name, code)


def gen_binary_mat_operator_glsl(lines, columns, operator, glsl_type):
    ''' Generates GLSL code which contains a binary operation between to
        matrices in a main function. First, the two operands are declared, then
        the result of the operation is stored in a variable. Finally, generates
        the expecting LLVM-IR translation.

        Arguments:
            lines (int): the number of lines in the matrix.
            column (int): the number of column in the matrix.
            operator (string): the binary operator.
            glsl_type (string): the GLSL type of the two matrices' components.
    '''

    mat_type = MAT_TYPES[glsl_type] + str(lines) + 'x' + str(columns)
    name = ('op_' + BINARY_OPERATORS[operator][glsl_type]['inst'].replace(
        ' ', '_') + '_two_' + mat_type + '_' + glsl_type + '_operands')

    ret_type = BINARY_OPERATORS[operator][glsl_type]['ret']
    if ret_type != 'bool':
        ret_type = mat_type

    code = [
        '#version 450', 'void main() {',
        get_glsl_mat_declaration('a', lines, columns, glsl_type),
        get_glsl_mat_declaration('b', lines, columns, glsl_type),
        ret_type + ' c = a ' + operator + ' b;', '}'
    ]

    edit_glsl_file(name, code)


def gen_function_call_ret_scalar_glsl(glsl_type):
    ''' Generates GLSL code which contains a function call, which returns a
        scalar, in a main function. First, the function is declared, then the
        result of the function call is stored in a variable. Finally,
        generates the expecting LLVM-IR translation.

        Arguments:
            glsl_type (string): the GLSL type of the returned value of the
                                function.
    '''

    name = 'func_call_ret_scalar_' + glsl_type

    code = [
        '#version 450', glsl_type + ' func_ret_' + glsl_type + '() {',
        get_glsl_scalar_declaration('a', glsl_type), 'return a;', '}',
        'void main() {', glsl_type + ' a = func_ret_' + glsl_type + '();', '}'
    ]

    edit_glsl_file(name, code)


def gen_function_call_ret_vec_glsl(nb_coordinates, glsl_type):
    ''' Generates GLSL code which contains a function call, which returns a
        vector, in a main function. First, the function is declared, then the
        result of the function call is stored in a variable. Finally,
        generates the expecting LLVM-IR translation.

        Arguments:
            nb_coordinates (int): the size of the returned vector by the
                                  function.
            glsl_type (string): the GLSL type of the vector's components which
                                is returned by the function.
    '''

    name = 'func_call_ret_vec' + str(nb_coordinates) + '_' + glsl_type

    code = [
        '#version 450',
        VEC_TYPES[glsl_type] + str(nb_coordinates) + ' ' + name + '() {',
        get_glsl_vec_declaration('a', nb_coordinates,
                                 glsl_type), 'return a;', '}', 'void main() {',
        VEC_TYPES[glsl_type] + str(nb_coordinates) + ' a = ' + name + '();',
        '}'
    ]

    edit_glsl_file(name, code)


def gen_negate_op_glsl(glsl_type):
    ''' Generates GLSL code which contains the negate operation on a scalar
        First, the operand is declared, then the unary operation result is
        stored in a variable. Then generates the expecting LLVM-IR translation.

        Arguments:
            glsl_type (string): the GLSL type of the negated operand.
    '''

    # You can't negate a boolean
    if glsl_type == 'bool':
        return

    name = 'negate_op_' + glsl_type

    code = [
        '#version 450', 'void main() {',
        get_glsl_scalar_declaration('a', glsl_type), glsl_type + ' b = -a;',
        '}'
    ]

    edit_glsl_file(name, code)


def gen_bitfield_glsl(glsl_type):
    '''Generate GLSL files that call the bitfield built-ins with all possible
    argument types.

    Arguments:
        glsl_type (string): the GLSL type to pass to the functions
    '''

    scal_insert_name = 'op_bitfield_insert_' + glsl_type

    scal_insert_code = [
        '#version 450', 'void main() {',
        get_glsl_scalar_declaration('base', glsl_type),
        get_glsl_scalar_declaration('insert', glsl_type),
        glsl_type + ' a = bitfieldInsert(base, insert, 0, 8);', '}'
    ]

    edit_glsl_file(scal_insert_name, scal_insert_code)

    for vec_width in range(2, 5):
        vec_insert_name = 'op_bitfield_insert_' + VEC_TYPES[glsl_type] + str(
            vec_width)

        glsl_vec_type = VEC_TYPES[glsl_type] + str(vec_width)

        vec_insert_code = [
            '#version 450', 'void main() {',
            get_glsl_vec_declaration('base', vec_width, glsl_type),
            get_glsl_vec_declaration('insert', vec_width, glsl_type),
            glsl_vec_type + ' res = bitfieldInsert(base, insert, 0, 8);', '}'
        ]

        edit_glsl_file(vec_insert_name, vec_insert_code)

    scal_extract_name = 'op_bitfield_extract_' + glsl_type

    scal_extract_code = [
        '#version 450', 'void main() {',
        get_glsl_scalar_declaration('value', glsl_type),
        glsl_type + ' res = bitfieldExtract(value, 0, 8);', '}'
    ]

    edit_glsl_file(scal_extract_name, scal_extract_code)

    for vec_width in range(2, 5):
        vec_extract_name = 'op_bitfield_extract_' + VEC_TYPES[glsl_type] + str(
            vec_width)

        glsl_vec_type = VEC_TYPES[glsl_type] + str(vec_width)

        vec_extract_code = [
            '#version 450', 'void main() {',
            get_glsl_vec_declaration('value', vec_width, glsl_type),
            glsl_vec_type + ' res = bitfieldExtract(value, 0, 8);', '}'
        ]

        edit_glsl_file(vec_extract_name, vec_extract_code)

    # TODO: bitfield reverse shaders when it is implemented


def gen_descriptor_set_glsl(glsl_type):
    ''' Generates GLSL code to test translating kernels that take external
        inputs in the form of a uniform block with a descriptor set binding

        Arguments:
            glsl_type (string): type of variable in the uniform block
    '''

    name = 'descriptor_set_' + glsl_type

    uniform_block = 'layout(set=0, binding=0) uniform ablock { %s a; };' % (
        glsl_type)

    code = [
        '#version 450', uniform_block, 'void main() {',
        'bool res = a == {0};'.format(CONSTANTS[glsl_type]), '}'
    ]

    edit_glsl_file(name, code)


def gen_push_constant_glsl(glsl_type):
    ''' Generates glsl files that test the push constant functionality

        Arguments:
            glsl_type (string): glsl type of the push constant to test
    '''

    name = 'push_constant_' + glsl_type

    code = [
        '#version 450', 'layout(push_constant) uniform push{',
        glsl_type + ' test;', '} test_push;', 'void main(){',
        glsl_type + ' result = test_push.test;', '}'
    ]

    edit_glsl_file(name, code)


def gen_function_parameter_glsl(glsl_type):
    ''' Generates GLSL that tests creating a function which takes parameters of
        the given type.

        Arguments:
            glsl_type (string): GLSL type the function's parameters will be
    '''

    name = 'op_function_parameter_' + glsl_type
    constant = CONSTANTS[glsl_type]

    code = [
        '#version 450', 'bool foo({0} a, {0} b){{ '.format(glsl_type),
        'return a == b;', '}', 'void main() {',
        'bool res = foo({0}, {0});'.format(constant), '}'
    ]

    edit_glsl_file(name, code)


def gen_switch_glsl():
    ''' Generates GLSL that tests switch functionality by creating one kernel
        that hits a case and one which falls through to default.
    '''

    name = 'op_switch'

    code = [
        '#version 450', 'void main() {', 'int cond = 42;'
        'int a;'
        'switch(cond){'
        'case 42: a=42; break;'
        'default: a=24;'
        '}'
        'int res = a ^ 42;'
        '}'
    ]

    edit_glsl_file(name, code)

    name = 'op_switch_default'

    code = [
        '#version 450', 'void main() {', 'int cond = 0;'
        'int a;'
        'switch(cond){'
        'case 42: a=42; break;'
        'default: a=24;'
        '}'
        'int res = a ^ 42;'
        '}'
    ]

    edit_glsl_file(name, code)


def gen_spec_constant_op_glsl(operator, glsl_type):
    ''' Generates GLSL files that test variations of the OpSpecConstantOp
        instruction.

        Arguments:
            operator (string): Operator that defines which instruction to test
            OpSpecConstantOp with
            glsl_type (string): GLSL type being used in the test case
    '''

    inst = BINARY_OPERATORS[operator][glsl_type]['inst'].replace(' ', '_')

    name = 'op_spec_constant_op_{0}_{1}'.format(inst, glsl_type)

    # if the shift operand passed to a shift instruction exceeds the bit width
    # of the base operand the result will be undefined
    if 'sh' in inst:
        constant = '21'
    else:
        constant = CONSTANTS[glsl_type]

    ret_type = BINARY_OPERATORS[operator][glsl_type]['ret']

    spec_constant_decl = 'layout(constant_id=0) const {0} test = {1};'.format(
        glsl_type, constant)

    code = [
        '#version 450', spec_constant_decl, 'void main() {',
        '{0} res = test {1} {2};'.format(ret_type, operator,
                                         constant.replace('-', '')), '}'
    ]

    edit_glsl_file(name, code)


def gen_debug_info_glsl():
    ''' Generates the .comp shader that all the debug info in the debug_info
    test kernel refers to, but doesn't add it to the file list as we don't
    want to actually compile this file, it just exists as a reference '''

    name = 'debug_info.comp'

    code = '''
#version 450

void main(){
    bool a = false;
    int b = 42;
    if(a){
        b+=42;
    }
}'''

    with open(posixpath.join(OUTPUT_DIR, name), 'w') as f:
        f.write(code)


def gen_runtime_array_glsl(glsl_type):
    ''' Generates GLSL files which test the OpArrayLength (and by extension
    OpTypeRuntimeArray) instructions.

    Arguments:
        glsl_type (string): GLSL type being used in the test case'''

    name = 'op_array_length_{0}'.format(glsl_type)

    code = [
        '''
#version 450

layout(set=0, binding=0) buffer block{{
    int a;
    {0}[] runtime_array;
}};

void main(){{
    int result = runtime_array.length();
}}
'''.format(glsl_type)
    ]

    edit_glsl_file(name, code)


def gen_builtin_var_glsl():
    ''' Generates GLSL files that test each of the built-in compute shader inputs.
    '''

    vec_builtins = [
        'gl_NumWorkGroups',
        'gl_WorkGroupID',
        'gl_LocalInvocationID',
        'gl_GlobalInvocationID',
    ]

    for vec_builtin in vec_builtins:
        name = 'builtin_var_{0}'.format(vec_builtin)

        code = [
            '''
#version 450
void main(){{
    uint result = {0}.x;
}}
'''.format(vec_builtin)
        ]

        edit_glsl_file(name, code)

    name = 'builtin_var_gl_LocalInvocationIndex'

    code = [
        '''
#version 450
void main(){
    uint result = gl_LocalInvocationIndex;
}
'''
    ]

    edit_glsl_file(name, code)


def gen_any_all_glsl():
    ''' Generates GLSL files that test the any and all builtins
    '''

    builtins = ['any', 'all']

    for builtin in builtins:
        for width in range(2, 5):

            name = 'op_{0}_bvec{1}'.format(builtin, width)

            code = [
                '#version 450', 'void main() {',
                get_glsl_vec_declaration('vec', width, 'bool'),
                'bool result = {0}(vec);'.format(builtin), '}'
            ]

            edit_glsl_file(name, code)


def gen_nan_inf_glsl():
    ''' Generates GLSL files that test the isnan and isinf builtins
    '''

    for builtin in ['isinf', 'isnan']:
        for type in ['float', 'double']:
            name = 'op_{0}_{1}'.format(builtin, type)

            code = [
                '#version 450', 'void main(){',
                get_glsl_scalar_declaration('value', type),
                'bool res = {0}(value);'.format(builtin), '}'
            ]

            edit_glsl_file(name, code)


def gen_dot_glsl():
    ''' Generates GLSL files that test the dot builtin
    '''

    for width in range(2, 5):
        for type in ['float', 'double']:
            name = 'op_dot_{0}{1}'.format(VEC_TYPES[type], width)

            code = [
                '#version 450', 'void main() {',
                get_glsl_vec_declaration('x', width, type),
                get_glsl_vec_declaration('y', width, type),
                '{0} res = dot(x, y);'.format(type), '}'
            ]

            edit_glsl_file(name, code)


def gen_fmod_glsl():
    ''' Generates glsl files that test the mod (OpFMod in SPIR-V) builtin
    '''

    for type in ['float', 'double']:
        name = 'op_fmod_' + type

        code = [
            '#version 450', 'void main() {',
            get_glsl_scalar_declaration('x', type),
            get_glsl_scalar_declaration('y', type),
            '{0} res = mod(x, y);'.format(type), '}'
        ]

        edit_glsl_file(name, code)

        for width in range(2, 5):
            name = 'op_fmod_{0}{1}'.format(VEC_TYPES[type], width)

            code = [
                '#version 450', 'void main() {',
                get_glsl_vec_declaration('x', width, type),
                get_glsl_vec_declaration('y', width, type),
                '{0} res = mod(x, y);'.format(VEC_TYPES[type] + str(width)),
                '}'
            ]

            edit_glsl_file(name, code)


def gen_bitcount_glsl():
    ''' Generates glsl files that test the bitCount builtin
    '''

    name = 'op_bitcount_int'

    code = [
        '#version 450', 'void main() {',
        get_glsl_scalar_declaration('val', 'int'), 'int res = bitCount(val);',
        '}'
    ]

    edit_glsl_file(name, code)

    for width in range(2, 5):
        name = 'op_bitcount_ivec' + str(width)

        code = [
            '#version 450', 'void main() {',
            get_glsl_vec_declaration('val', width, 'int'),
            'ivec{0} res = bitCount(val);'.format(str(width)), '}'
        ]

        edit_glsl_file(name, code)
