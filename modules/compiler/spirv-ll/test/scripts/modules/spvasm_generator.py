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

# Imports.
import json
import posixpath
import re

from utils import (BITCASTS, CONSTANTS, NUMERICAL_TYPES, get_builtin_test_name,
                   get_test_name_from_op)

GRAMMAR = {}

HEADER = '''\
            OpCapability Shader
            OpCapability Float64
            OpCapability Int64
       %1 = OpExtInstImport "GLSL.std.450"
            OpMemoryModel Logical GLSL450
            OpEntryPoint GLCompute %main "main"
            OpExecutionMode %main LocalSize 1 1 1
            OpSource GLSL 450
            OpName %main "main"
            '''

TYPE_DECLARATIONS = {
    'bool': 'OpTypeBool',
    'double': 'OpTypeFloat 64',
    'float': 'OpTypeFloat 32',
    'int': 'OpTypeInt 32 1',
    'uint': 'OpTypeInt 32 0',
    'long': 'OpTypeInt 64 1',
    'ulong': 'OpTypeInt 64 0',
}

RESTRICTIONS = {
    'Round': ['float', 'double'],
    'RoundEven': ['float', 'double'],
    'Trunc': ['float', 'double'],
    'FAbs': ['float', 'double'],
    'SAbs': ['int'],
    'FSign': ['float', 'double'],
    'SSign': ['int'],
    'Floor': ['float', 'double'],
    'Ceil': ['float', 'double'],
    'Fract': ['float', 'double'],
    'Radians': ['float'],
    'Degrees': ['float'],
    'Sin': ['float'],
    'Cos': ['float'],
    'Tan': ['float'],
    'Asin': ['float'],
    'Acos': ['float'],
    'Atan': ['float'],
    'Sinh': ['float'],
    'Cosh': ['float'],
    'Tanh': ['float'],
    'Asinh': ['float'],
    'Acosh': ['float'],
    'Atanh': ['float'],
    'Atan2': ['float'],
    'Pow': ['float'],
    'Exp': ['float'],
    'Log': ['float'],
    'Exp2': ['float'],
    'Log2': ['float'],
    'Sqrt': ['float', 'double'],
    'InverseSqrt': ['float', 'double'],
    'Determinant': [],  # TODO: A square matrix as operand type.
    'MatrixInverse': [],  # TODO: A square matrix as operand type.
    'Modf': ['float', 'double'],
    'ModfStruct': [],  # TODO: Return type.
    'FMin': ['float', 'double'],
    'UMin': ['uint'],
    'SMin': ['int'],
    'FMax': ['float', 'double'],
    'UMax': ['uint'],
    'SMax': ['int'],
    'FClamp': ['float', 'double'],
    'UClamp': ['uint'],
    'SClamp': ['int'],
    # IMix has been removed in version 0.99, but still present in the json
    # file. Then, we'll have to add it here.
    'IMix': [],
    'FMix': ['float', 'double'],
    'Step': ['float', 'double'],
    'SmoothStep': ['float', 'double'],
    'Fma': ['float', 'double'],
    'Frexp': [],  # TODO: exp operand type.
    'FrexpStruct': [],  # TODO: exp operand type.
    'Ldexp': [],  # TODO: exp operand type.
    # TODO: Pack* instructions. #
    'PackSnorm4x8': [],  #
    'PackUnorm4x8': [],  #
    'PackSnorm2x16': [],  #
    'PackUnorm2x16': [],  #
    'PackHalf2x16': [],  #
    'PackDouble2x32': [],  #
    'UnpackSnorm2x16': [],  #
    'UnpackUnorm2x16': [],  #
    'UnpackHalf2x16': [],  #
    'UnpackSnorm4x8': [],  #
    'UnpackUnorm4x8': [],  #
    'UnpackDouble2x32': [],  #
    ##############################
    'Length': ['float', 'double'],
    'Distance': ['float', 'double'],
    'Cross': [],  # TODO: Only vector3 of floating points.
    'Normalize': ['float', 'double'],
    'FaceForward': ['float', 'double'],
    'Reflect': ['float', 'double'],
    'Refract': [],  # TODO: eta operand type.
    'FindILsb': ['uint', 'int'],
    'FindSMsb': ['int'],
    'FindUMsb': ['uint'],
    'InterpolateAtCentroid': ['float'],
    'InterpolateAtSample': ['float'],
    'InterpolateAtOffset': [],  # TODO: offset operand type.
    'NMin': ['float', 'double'],
    'NMax': ['float', 'double'],
    'NClamp': ['float', 'double'],
}


def gen_remainder_kernels(args):
    ''' Generates .spvasm files which test the remainder instructions(mod and
        rem)

        Arguments:
            args: argument parser object setup by the script
    '''

    files = []

    remainder_insts = {
        'OpSRem': 'int',
        'OpFRem': 'float',
        'OpFMod': 'float',
    }

    for instruction in remainder_insts:
        name = get_test_name_from_op(instruction) + '.spvasm'
        name = posixpath.join(args.output_dir, name.lower())

        glsl_type = remainder_insts[instruction]

        with open(name, 'w') as f:
            f.write(HEADER + '''OpName %a "a"
               OpName %b "b"
               OpName %res "res"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
        %{0} = {1}
%ptr_{0} = OpTypePointer Function %{0}
          %9 = OpConstant %{0} {2}
         %11 = OpConstant %{0} {2}
       %main = OpFunction %void None %3
          %5 = OpLabel
          %a = OpVariable %ptr_{0} Function
          %b = OpVariable %ptr_{0} Function
        %res = OpVariable %ptr_{0} Function
               OpStore %a %9
               OpStore %b %11
         %13 = OpLoad %{0} %a
         %14 = OpLoad %{0} %b
         %15 = {3} %{0} %13 %14 ; testing this
               OpStore %res %15
               OpReturn
               OpFunctionEnd
'''.format(glsl_type, TYPE_DECLARATIONS[glsl_type], CONSTANTS[glsl_type],
           instruction))

        files.append(name)

    return files


def gen_select_kernels(args):
    ''' Generates .spvasm files which test the OpSelect instruction

        Arguments:
            args: argument parser object setup by the script
    '''

    files = []

    for glsl_type in NUMERICAL_TYPES:
        name = 'op_select_' + glsl_type + '.spvasm'
        name = posixpath.join(args.output_dir, name)

        with open(name, 'w') as f:
            f.write(HEADER + '''OpName %a "a"
               OpName %b "b"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
       %bool = OpTypeBool
%_ptr_Function_bool = OpTypePointer Function %bool
          %9 = OpConstantTrue %bool
        %{0} = {1}
    %ptr_{0} = OpTypePointer Function %{0}
         %17 = OpConstant %{0} {2}
         %19 = OpConstant %{0} {2}
       %main = OpFunction %void None %3
          %5 = OpLabel
          %a = OpVariable %_ptr_Function_bool Function
          %b = OpVariable %ptr_{0} Function
          %b = OpVariable %ptr_{0} Function
               OpStore %a %9
         %14 = OpLoad %bool %a
         %15 = OpSelect %{0} %14 %17 %19 ; testing this
               OpStore %b %15
               OpReturn
               OpFunctionEnd
'''.format(glsl_type, TYPE_DECLARATIONS[glsl_type], CONSTANTS[glsl_type]))

        files.append(name)

    return files


def gen_bitcast_kernels(args):
    ''' Generates .spvasm files that test the OpBitcast instruction

        Arguments:
            args: argument parser object setup by the script
    '''

    files = []

    for src_type in BITCASTS:
        dst_type = BITCASTS[src_type]

        name = 'op_bitcast_' + src_type + '_to_' + dst_type + '.spvasm'
        name = posixpath.join(args.output_dir, name)

        src_decl = TYPE_DECLARATIONS[src_type]
        src_const = CONSTANTS[src_type]
        dst_decl = TYPE_DECLARATIONS[dst_type]

        with open(name, 'w') as f:
            f.write(HEADER + '''OpName %src "src"
               OpName %dst "dst"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
        %{0} = {1}
    %ptr_{0} = OpTypePointer Function %{0}
          %9 = OpConstant %{0} {2}
        %{3} = {4}
    %ptr_{3} = OpTypePointer Function %{3}
       %main = OpFunction %void None %3
          %5 = OpLabel
        %src = OpVariable %ptr_{0} Function
        %dst = OpVariable %ptr_{3} Function
               OpStore %src %9
         %13 = OpLoad %{0} %src
         %14 = OpBitcast %{3} %13
               OpStore %dst %14
               OpReturn
               OpFunctionEnd
'''.format(src_type, src_decl, src_const, dst_type, dst_decl))

        files.append(name)

    return files


def gen_overflow_arithmetic(args):
    ''' Generates .spvasm files designed to test the overflow arithmetic
        instructions

        Arguments:
            args: the argument parser object setup by the script
    '''
    files = []

    instructions = {
        'OpIAddCarry': '0',
        'OpISubBorrow': '0',
        'OpUMulExtended': '0',
        'OpSMulExtended': '1'
    }

    for instruction in instructions:
        name = get_test_name_from_op(instruction) + '.spvasm'
        name = posixpath.join(args.output_dir, name.lower())
        signed = instructions[instruction]

        with open(name, 'w') as f:
            f.write('''
; SPIR-V
; Version: 1.0
; Generator: Khronos Glslang Reference Front End; 1
; Bound: 21
; Schema: 0
               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpSource GLSL 450
               OpSourceExtension "GL_GOOGLE_cpp_style_line_directive"
               OpSourceExtension "GL_GOOGLE_include_directive"
               OpName %main "main"
               OpName %Foo "Foo"
               OpMemberName %Foo 0 "a"
               OpMemberName %Foo 1 "b"
               OpName %foo "foo"
               OpName %a "a"
               OpName %b "b"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
                  %int = OpTypeInt 32 {0}
        %Foo = OpTypeStruct %int %int
    %ptr_Foo = OpTypePointer Function %Foo
    %ptr_int = OpTypePointer Function %int
         %14 = OpConstant %int {1}
         %16 = OpConstant %int {1}
       %main = OpFunction %void None %3
          %5 = OpLabel
        %foo = OpVariable %ptr_Foo Function
          %a = OpVariable %ptr_int Function
          %b = OpVariable %ptr_int Function
               OpStore %a %14
               OpStore %b %16
         %18 = OpLoad %int %a
         %19 = OpLoad %int %b
         %20 = {2} %Foo %18 %19 ;testing this
               OpStore %foo %20
               OpReturn
               OpFunctionEnd
'''.format(signed, CONSTANTS['uint'], instruction))

        files.append(name)

    return files


def gen_spec_constant_kernels(args):
    ''' Generates .spvasm files to test the OpSpecConstant instructions

        Arguments:
            args: the argument parser object setup by the script
    '''

    files = []

    instructions = {
        'OpSpecConstantTrue': {
            'bool': ''
        },
        'OpSpecConstantFalse': {
            'bool': ''
        },
        'OpSpecConstant': {
            'int': '-42',
            'uint': '42',
            'float': '42.42',
            'double': '-42.42',
            'long': '-42000000000'
        }
    }

    for inst in instructions:
        for glsl_type in instructions[inst]:
            name = '_'.join(re.findall('[A-Z][a-z]*', inst)) + '_' + glsl_type
            name = args.output_dir + '/' + name.lower() + '.spvasm'

            constant = instructions[inst][glsl_type]

            type_decl = TYPE_DECLARATIONS[glsl_type]

            with open(name, 'w') as f:
                f.write(HEADER + '''OpName %a_block "a_block"
               OpMemberName %a_block 0 "test_out"
               OpName %_ ""
               OpMemberDecorate %a_block 0 Offset 0
               OpDecorate %a_block BufferBlock
               OpDecorate %_ DescriptorSet 0
               OpDecorate %_ Binding 0
               OpDecorate %11 SpecId 0
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
        %{0} = {1}
   %idx_type = OpTypeInt 32 0
    %a_block = OpTypeStruct %{0}
%ptr_Uniform_block = OpTypePointer Uniform %a_block
          %_ = OpVariable %ptr_Uniform_block Uniform
         %10 = OpConstant %idx_type 0
         %11 = {2} %{0} {3} ; testing this
%ptr_Uniform_{0} = OpTypePointer Uniform %{0}
       %main = OpFunction %void None %3
          %5 = OpLabel
         %13 = OpAccessChain %ptr_Uniform_{0} %_ %10
               OpStore %13 %11
               OpReturn
               OpFunctionEnd
'''.format(glsl_type, type_decl, inst, constant))

            files.append(name)

    return files


def gen_spec_constant_composite_kernels(args):
    ''' Generates kernels which test the OpConstantComposite instruction

        Arguments:
            args: the argument parser object setup by the script
    '''

    files = []

    out_dir = args.output_dir

    for glsl_type in NUMERICAL_TYPES:
        vec_name = 'op_spec_constant_composite_{0}_vec'.format(glsl_type)
        vec_name = out_dir + '/' + vec_name + '.spvasm'

        with open(vec_name, 'w') as f:
            f.write(HEADER + '''OpName %test_block "test_block"
               OpMemberName %test_block 0 "test_out"
               OpName %_ ""
               OpMemberDecorate %test_block 0 Offset 0
               OpDecorate %test_block BufferBlock
               OpDecorate %_ DescriptorSet 0
               OpDecorate %_ Binding 0
               OpDecorate %12 SpecId 0
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
   %idx_type = OpTypeInt 32 0
        %{0} = {1}
    %vec_{0} = OpTypeVector %{0} 3
 %test_block = OpTypeStruct %vec_{0}
%_ptr_Uniform_test_block = OpTypePointer Uniform %test_block
          %_ = OpVariable %_ptr_Uniform_test_block Uniform
         %11 = OpConstant %idx_type 0
         %12 = OpSpecConstant %{0} {2}
         %13 = OpSpecConstantComposite %vec_{0} %12 %12 %12 ; testing this
%ptr_vec_{0} = OpTypePointer Uniform %vec_{0}
       %main = OpFunction %void None %3
          %5 = OpLabel
         %15 = OpAccessChain %ptr_vec_{0} %_ %11
               OpStore %15 %13
               OpReturn
               OpFunctionEnd
'''.format(glsl_type, TYPE_DECLARATIONS[glsl_type], CONSTANTS[glsl_type]))

        files.append(vec_name)

        struct_name = 'op_spec_constant_composite_{0}_struct'.format(glsl_type)
        struct_name = out_dir + '/' + struct_name + '.spvasm'

        with open(struct_name, 'w') as f:
            f.write(HEADER + '''OpName %Foo "Foo"
               OpMemberName %Foo 0 "a"
               OpName %test_block "test_block"
               OpMemberName %test_block 0 "test_out"
               OpName %_ ""
               OpMemberDecorate %Foo 0 Offset 0
               OpMemberDecorate %test_block 0 Offset 0
               OpDecorate %test_block BufferBlock
               OpDecorate %_ DescriptorSet 0
               OpDecorate %_ Binding 0
               OpDecorate %12 SpecId 0
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
        %{0} = {1}
   %idx_type = OpTypeInt 32 0
        %Foo = OpTypeStruct %{0}
 %test_block = OpTypeStruct %Foo
%_ptr_Uniform_test_block = OpTypePointer Uniform %test_block
          %_ = OpVariable %_ptr_Uniform_test_block Uniform
         %11 = OpConstant %idx_type 0
         %12 = OpSpecConstant %{0} {2}
         %14 = OpSpecConstantComposite %Foo %12 ; testing this
%_ptr_Uniform_Foo = OpTypePointer Uniform %Foo
       %main = OpFunction %void None %3
          %5 = OpLabel
         %16 = OpAccessChain %_ptr_Uniform_Foo %_ %11
               OpStore %16 %14
               OpReturn
               OpFunctionEnd
'''.format(glsl_type, TYPE_DECLARATIONS[glsl_type], CONSTANTS[glsl_type]))

        files.append(struct_name)

        array_name = 'op_spec_constant_composite_{0}_array'.format(glsl_type)
        array_name = out_dir + '/' + array_name + '.spvasm'

        with open(array_name, 'w') as f:
            f.write(HEADER + '''OpName %test_block "test_block"
               OpMemberName %test_block 0 "test_out"
               OpName %_ ""
               OpMemberDecorate %test_block 0 Offset 0
               OpDecorate %test_block BufferBlock
               OpDecorate %_ DescriptorSet 0
               OpDecorate %_ Binding 0
               OpDecorate %14 SpecId 0
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
        %{0} = {1}
   %idx_type = OpTypeInt 32 0
          %8 = OpConstant %idx_type 4
    %arr_{0} = OpTypeArray %{0} %8
 %test_block = OpTypeStruct %arr_{0}
%_ptr_Uniform_test_block = OpTypePointer Uniform %test_block
          %_ = OpVariable %_ptr_Uniform_test_block Uniform
         %13 = OpConstant %idx_type 0
         %14 = OpSpecConstant %{0} {2}
         %15 = OpSpecConstantComposite %arr_{0} %14 %14 %14 %14 ; testing this
%ptr_arr_{0} = OpTypePointer Uniform %arr_{0}
       %main = OpFunction %void None %3
          %5 = OpLabel
         %17 = OpAccessChain %ptr_arr_{0} %_ %13
               OpStore %17 %15
               OpReturn
               OpFunctionEnd
'''.format(glsl_type, TYPE_DECLARATIONS[glsl_type], CONSTANTS[glsl_type]))

        files.append(array_name)

    return files


def gen_vec_kernels(args):
    ''' Generates kernels which test OpVectorExtractDynamic and
        OpVectorInsertDynamic

        Arguments:
            args: argument parser object setup by the script
    '''

    files = []

    for glsl_type in NUMERICAL_TYPES:
        name = 'op_vec_extract_v3' + glsl_type + '.spvasm'
        name = posixpath.join(args.output_dir, name)

        with open(name, 'w') as f:
            f.write(HEADER + '''OpName %a "a"
               OpName %res "res"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
        %{0} = {1}
     %idx_ty = OpTypeInt 32 0
         %v3 = OpTypeVector %{0} 3
     %ptr_v3 = OpTypePointer Function %v3
          %8 = OpConstant %{0} {2}
          %9 = OpConstant %{0} {2}
         %10 = OpConstant %{0} {2}
         %11 = OpConstant %idx_ty 1
         %12 = OpConstantComposite %v3 %8 %9 %10
    %ptr_{0} = OpTypePointer Function %{0}
       %main = OpFunction %void None %3
         %16 = OpLabel
          %a = OpVariable %ptr_v3 Function
        %res = OpVariable %ptr_{0} Function
               OpStore %a %12
         %19 = OpLoad %v3 %a
         %20 = OpVectorExtractDynamic %{0} %19 %11
               OpStore %res %20
               OpReturn
               OpFunctionEnd
'''.format(glsl_type, TYPE_DECLARATIONS[glsl_type], CONSTANTS[glsl_type]))

        files.append(name)

        name = 'op_vec_insert_v3' + glsl_type + '.spvasm'
        name = posixpath.join(args.output_dir, name)

        with open(name, 'w') as f:
            f.write(HEADER + '''OpName %a "a"
               OpName %res "res"
       %void = OpTypeVoid
          %2 = OpTypeFunction %void
        %{0} = {1}
     %idx_ty = OpTypeInt 32 0
         %v3 = OpTypeVector %{0} 3
     %ptr_v3 = OpTypePointer Function %v3
          %3 = OpConstant %{0} {2}
          %4 = OpConstant %{0} {2}
          %5 = OpConstant %{0} {2}
          %6 = OpConstant %idx_ty 1
          %7 = OpConstantComposite %v3 %3 %4 %5
       %main = OpFunction %void None %2
          %9 = OpLabel
          %a = OpVariable %ptr_v3 Function
        %res = OpVariable %ptr_v3 Function
               OpStore %a %7
         %10 = OpLoad %v3 %a
         %11 = OpVectorInsertDynamic %v3 %10 %5 %6
               OpStore %res %11
               OpReturn
               OpFunctionEnd
'''.format(glsl_type, TYPE_DECLARATIONS[glsl_type], CONSTANTS[glsl_type]))

        files.append(name)

    return files


def gen_memcpy_kernels(args):
    ''' Generates kernels that test the OpCopyMemory and OpCopyMemorySized
        instructions.

        Arguments:
            args: argument parser object setup by the script
    '''

    files = []

    for glsl_type in NUMERICAL_TYPES:
        name = 'op_copy_memory_' + glsl_type + '.spvasm'
        name = posixpath.join(args.output_dir, name)

        type_decl = TYPE_DECLARATIONS[glsl_type]

        # TODO: for 100% coverage of the function write some variations with
        # memory access modifiers
        with open(name, 'w') as f:
            f.write(HEADER + '''OpName %main "main"
               OpName %a "a"
               OpName %b "b"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
        %{0} = {1}
    %ptr_{0} = OpTypePointer Function %{0}
         %14 = OpConstant %{0} {2}
       %main = OpFunction %void None %3
          %5 = OpLabel
          %a = OpVariable %ptr_{0} Function
          %b = OpVariable %ptr_{0} Function
               OpStore %a %14
               OpCopyMemory %b %a
               OpReturn
               OpFunctionEnd'''
                    .format(glsl_type, type_decl, CONSTANTS[glsl_type]))

        files.append(name)

        name = 'op_copy_memory_sized_' + glsl_type + '.spvasm'
        name = posixpath.join(args.output_dir, name)

        if '32' in type_decl:
            size = 4
        else:
            size = 8

        with open(name, 'w') as f:
            f.write(HEADER + '''OpName %main "main"
               OpName %a "a"
               OpName %b "b"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
     %size_t = OpTypeInt 32 0
        %{0} = {1}
    %ptr_{0} = OpTypePointer Function %{0}
         %14 = OpConstant %{0} {2}
         %15 = OpConstant %size_t {3}
       %main = OpFunction %void None %3
          %5 = OpLabel
          %a = OpVariable %ptr_{0} Function
          %b = OpVariable %ptr_{0} Function
               OpStore %a %14
               OpCopyMemorySized %b %a %15
               OpReturn
               OpFunctionEnd'''
                    .format(glsl_type, type_decl, CONSTANTS[glsl_type], size))

        files.append(name)

    return files


def gen_unord_cmp_kernels(args):
    ''' Generates kernels which test the various unordered floating point
        comparison instructions

        Arguments:
            args: argument parser object setup by the script
    '''

    files = []

    instructions = [
        'OpFUnordEqual', 'OpFUnordNotEqual', 'OpFUnordLessThan',
        'OpFUnordGreaterThan', 'OpFUnordLessThanEqual',
        'OpFUnordGreaterThanEqual'
    ]

    for instruction in instructions:
        name = get_test_name_from_op(instruction) + '.spvasm'
        name = posixpath.join(args.output_dir, name.lower())

        with open(name, 'w') as f:
            f.write(HEADER + '''OpName %a "a"
               OpName %b "b"
               OpName %c "c"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
          %9 = OpConstant %float 424.424
       %bool = OpTypeBool
%_ptr_Function_bool = OpTypePointer Function %bool
       %main = OpFunction %void None %3
          %5 = OpLabel
          %a = OpVariable %_ptr_Function_float Function
          %b = OpVariable %_ptr_Function_float Function
          %c = OpVariable %_ptr_Function_bool Function
               OpStore %a %9
               OpStore %b %9
         %14 = OpLoad %float %b
         %15 = OpLoad %float %a
         %16 = {0} %bool %14 %15 ; testing this
               OpStore %c %16
               OpReturn
               OpFunctionEnd
'''.format(instruction))

        files.append(name)

    return files


def gen_long_conversion_kernels(args):
    ''' Generates kernels which test conversion instructions for long integer
        types, something not possible with glsl.

        Arguments:
            args: argument parser object setup by the script
    '''

    files = []

    long_conversions = {
        'int': 'long',
        'uint': 'ulong',
        'long': 'int',
        'ulong': 'uint'
    }

    for src_type in long_conversions:
        dst_type = long_conversions[src_type]

        name = 'op_convert_{0}_to_{1}'.format(src_type, dst_type) + '.spvasm'
        name = posixpath.join(args.output_dir, name)

        if 'u' in src_type:
            inst = 'OpUConvert'
        else:
            inst = 'OpSConvert'

        src_decl = TYPE_DECLARATIONS[src_type]
        dst_decl = TYPE_DECLARATIONS[dst_type]

        src_const = CONSTANTS[src_type]

        with open(name, 'w') as f:
            f.write(HEADER + '''OpName %a "a"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
        %{0} = {1}
        %{2} = {3}
    %ptr_{0} = OpTypePointer Function %{0}
    %ptr_{2} = OpTypePointer Function %{2}
          %9 = OpConstant %{0} {4}
       %main = OpFunction %void None %3
          %5 = OpLabel
          %a = OpVariable %ptr_{0} Function
          %b = OpVariable %ptr_{2} Function
               OpStore %a %9
         %13 = OpLoad %{0} %a
         %14 = {5} %{2} %13 ; testing this
               OpStore %b %14
               OpReturn
               OpFunctionEnd
'''.format(src_type, src_decl, dst_type, dst_decl, src_const, inst))

        files.append(name)

    return files


def gen_unary_kernels(args):
    ''' Generates SPIR-V kernels which test various unary operators

        Arguments:
            args: the argument parser object setup by the script
    '''

    instructions = {
        'OpLogicalNot': 'bool',
        'OpNot': 'int',
        # TODO: OpBitReverse when it is implemented
    }

    files = []

    for instruction in instructions:
        name = get_test_name_from_op(instruction) + '.spvasm'
        name = posixpath.join(args.output_dir, name.lower())

        ty = instructions[instruction]

        if ty == 'bool':
            const = 'OpConstantTrue %bool'
        else:
            const = 'OpConstant %{0} {1}'.format(ty, CONSTANTS[ty])

        with open(name, 'w') as f:
            f.write(HEADER + '''OpName %a "a"
               OpName %res "res"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
        %{0} = {1}
    %ptr_{0} = OpTypePointer Function %{0}
          %9 = {2}
       %main = OpFunction %void None %3
          %5 = OpLabel
          %a = OpVariable %ptr_{0} Function
        %res = OpVariable %ptr_{0} Function
               OpStore %a %9
         %11 = OpLoad %{0} %a
         %12 = {3} %{0} %11
               OpStore %res %12
               OpReturn
               OpFunctionEnd
'''.format(ty, TYPE_DECLARATIONS[ty], const, instruction))

        files.append(name)

    return files


def gen_forward_pointer_kernels(args):
    ''' Generates SPIR-V kernels that test the OpTypeForwardPointer instruction

        Arguments:
            args: argument parser object setup by the script
    '''

    files = []
    types = []

    for glsl_type in NUMERICAL_TYPES:
        types.append(glsl_type)

    types.append('Foo')

    for glsl_type in types:
        name = 'op_forward_pointer_{0}.spvasm'.format(glsl_type)
        name = posixpath.join(args.output_dir, name)

        if 'Foo' in glsl_type:
            type_decl = ''
            ptr_decl = ''
        else:
            type_decl = '%{0} = {1}'.format(glsl_type,
                                            TYPE_DECLARATIONS[glsl_type])
            ptr_decl = '%ptr_{0} = OpTypePointer Function %{0}'.format(
                glsl_type)

        with open(name, 'w') as f:
            f.write('''OpCapability Addresses
            ''' + HEADER + '''OpName %main "main"
               OpName %res "res"
               OpName %Foo "Foo"
               OpMemberName %Foo 0 "a"
               OpMemberName %Foo 1 "b"
               OpName %f "f"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
%struct_float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %struct_float
{0} ; this should be blank if type is Foo
          %9 = OpConstant %struct_float 42.42
 %struct_int = OpTypeInt 32 0
               OpTypeForwardPointer %ptr_{1} Function
        %Foo = OpTypeStruct %struct_int %struct_float %ptr_{1}
{2} ; this should be blank if type is Foo
    %ptr_Foo = OpTypePointer Function %Foo
         %14 = OpConstant %struct_int 1
       %main = OpFunction %void None %3
          %5 = OpLabel
        %res = OpVariable %_ptr_Function_float Function
          %f = OpVariable %ptr_Foo Function
         %15 = OpInBoundsAccessChain %_ptr_Function_float %f %14
         %16 = OpLoad %struct_float %15
         %17 = OpFAdd %struct_float %9 %16
               OpStore %res %17
               OpReturn
               OpFunctionEnd
'''.format(type_decl, glsl_type, ptr_decl))

        files.append(name)

    return files


def gen_branch_conditional_kernels(args):
    ''' Generates kernels that test the OpBranchConditional instruction

        Arguments:
            args: argument parser object setup by the script
    '''

    files = []

    values = ['True', 'False']

    for value in values:
        name = 'op_branch_conditional_{0}.spvasm'.format(value.lower())
        name = posixpath.join(args.output_dir, name)

        with open(name, 'w') as f:
            f.write(HEADER + '''OpName %cond "cond"
               OpName %a "a"
               OpName %res "res"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
       %bool = OpTypeBool
%_ptr_Function_bool = OpTypePointer Function %bool
          %9 = OpConstant{0} %bool
        %int = OpTypeInt 32 1
%_ptr_Function_int = OpTypePointer Function %int
         %13 = OpConstant %int 0
         %17 = OpConstant %int 24
         %19 = OpConstant %int 42
       %main = OpFunction %void None %3
          %5 = OpLabel
       %cond = OpVariable %_ptr_Function_bool Function
          %a = OpVariable %_ptr_Function_int Function
        %res = OpVariable %_ptr_Function_int Function
               OpStore %cond %9
               OpStore %a %13
         %14 = OpLoad %bool %cond
               OpSelectionMerge %16 None
               OpBranchConditional %14 %15 %18 ; testing this
         %15 = OpLabel
               OpStore %a %17
               OpBranch %16
         %18 = OpLabel
               OpStore %a %19
               OpBranch %16
         %16 = OpLabel
         %21 = OpLoad %int %a
         %22 = OpBitwiseXor %int %21 %19
               OpStore %res %22
               OpReturn
               OpFunctionEnd
'''.format(value))

        files.append(name)

    return files


def gen_ptr_int_conversion_kernels(args):
    ''' Generates kernels that test OpConvertPtrToU and OpConvertUToPtr

        Arguments:
            args: argument parser object setup by the script
    '''

    name = posixpath.join(args.output_dir, 'ptr_int_conversions.spvasm')

    with open(name, 'w') as f:
        f.write('''OpCapability Addresses
            ''' + HEADER + '''OpName %ptr "ptr"
               OpName %a "a"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
       %uint = OpTypeInt 32 0
          %7 = OpConstant %uint 2
%_arr_uint_7 = OpTypeArray %uint %7
%_ptr_Function__arr_uint_7 = OpTypePointer Function %_arr_uint_7
         %11 = OpConstant %uint 42
         %12 = OpConstantComposite %_arr_uint_7 %11 %11
%_ptr_Function_uint = OpTypePointer Function %uint
         %16 = OpConstant %uint 1
       %main = OpFunction %void None %3
          %5 = OpLabel
        %ptr = OpVariable %_ptr_Function__arr_uint_7 Function
          %a = OpVariable %_ptr_Function_uint Function
               OpStore %ptr %12
         %17 = OpConvertPtrToU %uint %ptr ; pointer to uint
         %18 = OpIAdd %uint %16 %17 ; increment
         %19 = OpConvertUToPtr %_ptr_Function_uint %18 ; convert back again
         %20 = OpLoad %uint %19
               OpStore %a %20
               OpReturn
               OpFunctionEnd
''')

    return [name]


def gen_spec_constant_op_kernels(args):
    ''' Generates kernels that test a few cases for OpSpecConstantOp that can't
        be generated from glsl (e.g. those that require the kernel capability)

        Arguments:
            args: the argument parser object setup by the script
    '''

    # TODO: 100% coverage of the possible instructions executed by
    # OpSpecConstantOp
    instructions = ['FAdd', 'FSub', 'FMul', 'FDiv']

    files = []

    for inst in instructions:
        for const_type in ['float', 'double']:
            name = 'op_spec_constant_op_{0}_{1}.spvasm'.format(
                inst.lower(), const_type)
            name = posixpath.join(args.output_dir, name)

            with open(name, 'w') as f:
                f.write('''OpCapability Kernel
                     ''' + HEADER + '''OpName %res "res"
               OpName %test "test"
               OpName %const "const"
               OpDecorate %test SpecId 0
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
        %{0} = {1}
%ptr_{0} = OpTypePointer Function %{0}
       %test = OpSpecConstant %{0} {2}
      %const = OpConstant %{0} {2}
         %11 = OpSpecConstantOp %{0} {3} %test %const
       %main = OpFunction %void None %3
          %5 = OpLabel
        %res = OpVariable %ptr_{0} Function
               OpStore %res %11
               OpReturn
               OpFunctionEnd
'''.format(const_type, TYPE_DECLARATIONS[const_type], CONSTANTS[const_type],
           inst))

            files.append(name)

    # also take this opportunity to generate a kernel that tests resolving
    # dependencies between OpSpecConstantOp and OpSpecConstantComposite
    # insructions

    name = 'op_spec_constant_deps.spvasm'
    name = posixpath.join(args.output_dir, name)

    with open(name, 'w') as f:
        f.write(HEADER + '''OpName %Foo "Foo"
               OpMemberName %Foo 0 "a"
               OpDecorate %11 SpecId 0
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
        %int = OpTypeInt 32 1
      %v2int = OpTypeVector %int 2
        %Foo = OpTypeStruct %v2int
%_ptr_Function_Foo = OpTypePointer Function %Foo
         %11 = OpSpecConstant %int 42
         %12 = OpSpecConstantComposite %v2int %11 %11
         %13 = OpConstant %int 42
         %14 = OpConstantComposite %v2int %13 %13
         %15 = OpSpecConstantOp %v2int IAdd %12 %14
         %16 = OpSpecConstantComposite %Foo %15
       %main = OpFunction %void None %3
          %5 = OpLabel
         %17 = OpVariable %_ptr_Function_Foo Function
               OpStore %17 %16
               OpReturn
               OpFunctionEnd
''')

    files.append(name)

    return files


def gen_null_constant_kernels(args):
    ''' Generates kernels that test the various usages of OpConstantNull

        Arguments:
            args: argument parser object setup by the script
    '''

    files = []

    for glsl_type in ['int', 'float']:
        name = 'op_constant_null_{0}.spvasm'.format(glsl_type)
        name = posixpath.join(args.output_dir, name)

        with open(name, 'w') as f:
            f.write(HEADER + '''OpName %a "a"
       %void = OpTypeVoid
          %5 = OpTypeFunction %void
        %{0} = {1}
    %ptr_{0} = OpTypePointer Function %{0}
          %8 = OpConstantNull %{0}
       %main = OpFunction %void None %5
          %9 = OpLabel
          %a = OpVariable %ptr_{0} Function
               OpStore %a %8
               OpReturn
               OpFunctionEnd
'''.format(glsl_type, TYPE_DECLARATIONS[glsl_type]))

        files.append(name)

    types = {
        'vec': 'OpTypeVector %int 3',
        'struct': 'OpTypeStruct %int',
        'ptr': 'OpTypePointer Function %int'
    }

    for ty in types:
        name = 'op_constant_null_{0}.spvasm'.format(ty)
        name = posixpath.join(args.output_dir, name)

        with open(name, 'w') as f:
            f.write(HEADER + '''OpName %a "a"
       %void = OpTypeVoid
          %5 = OpTypeFunction %void
        %int = OpTypeInt 32 0
        %{0} = {1}
    %ptr_{0} = OpTypePointer Function %{0}
          %8 = OpConstantNull %{0}
       %main = OpFunction %void None %5
          %9 = OpLabel
          %a = OpVariable %ptr_{0} Function
               OpStore %a %8
               OpReturn
               OpFunctionEnd
'''.format(ty, types[ty]))

        files.append(name)

    return files


def gen_composite_insert_kernels(args):
    ''' Generates kernels that test the OpCompositeInsert instruction

        Arguments:
            args: argument parser object setup by the script
    '''

    files = []

    types = {
        'vector': 'OpTypeVector %int 3',
        'struct': 'OpTypeStruct %int %int %int',
        'array': 'OpTypeArray %int %9'
    }

    for ty in types:
        name = 'op_composite_insert_{0}.spvasm'.format(ty)
        name = posixpath.join(args.output_dir, name)

        with open(name, 'w') as f:
            f.write(HEADER + '''OpName %res "res"
       %void = OpTypeVoid
          %5 = OpTypeFunction %void
        %int = OpTypeInt 32 0
          %9 = OpConstant %int 3
        %{0} = {1}
    %ptr_{0} = OpTypePointer Function %{0}
         %10 = OpConstant %int 42
         %11 = OpConstantComposite %{0} %9 %9 %9
       %main = OpFunction %void None %5
         %12 = OpLabel
        %res = OpVariable %ptr_{0} Function
         %13 = OpCompositeInsert %{0} %10 %11 0
               OpStore %res %13
               OpReturn
               OpFunctionEnd
'''.format(ty, types[ty]))

        files.append(name)

    return files


def gen_phi_kernel(args):
    ''' Generates a kernel which tests OpPhi '''
    name = 'op_phi.spvasm'
    name = posixpath.join(args.output_dir, name)

    with open(name, 'w') as f:
        f.write(HEADER + '''OpName %cond "cond"
               OpName %a "a"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
       %bool = OpTypeBool
%_ptr_Function_bool = OpTypePointer Function %bool
          %9 = OpConstantFalse %bool
        %int = OpTypeInt 32 1
%_ptr_Function_int = OpTypePointer Function %int
         %17 = OpConstant %int 24
         %19 = OpConstant %int 42
       %main = OpFunction %void None %3
          %5 = OpLabel
       %cond = OpVariable %_ptr_Function_bool Function
          %a = OpVariable %_ptr_Function_int Function
               OpStore %cond %9
         %14 = OpLoad %bool %cond
               OpSelectionMerge %16 None
               OpBranchConditional %14 %15 %18
         %15 = OpLabel
         %a1 = OpBitwiseXor %int %17 %19
               OpBranch %16
         %18 = OpLabel
         %a2 = OpBitwiseAnd %int %17 %19
               OpBranch %16
         %16 = OpLabel
       %aphi = OpPhi %int %a1 %15 %a2 %18
               OpStore %a %aphi
               OpReturn
               OpFunctionEnd
''')

    return [name]


def gen_vec_extract_kernels(args):
    ''' Generates kernels that test using OpCompositeExtract with vector operands
        (doing this in glsl tends to generate an OpAccessChain)

        Arguments:
            args: argument parser object setup by the script
    '''

    files = []

    for vec_width in range(2, 5):
        name = 'op_composite_extract_vec{0}.spvasm'.format(vec_width)
        name = posixpath.join(args.output_dir, name)

        with open(name, 'w') as f:
            f.write(HEADER + '''OpName %const "const"
               OpName %a "a"
               OpName %res "res"
       %void = OpTypeVoid
          %2 = OpTypeFunction %void
        %int = OpTypeInt 32 1
    %ptr_int = OpTypePointer Function %int
        %vec = OpTypeVector %int {0}
    %ptr_vec = OpTypePointer Function %vec
      %const = OpConstant %int {1}
          %7 = OpConstantComposite %vec {2}
       %main = OpFunction %void None %2
          %9 = OpLabel
          %a = OpVariable %ptr_vec Function
        %res = OpVariable %ptr_int Function
               OpStore %a %7
         %10 = OpLoad %vec %a
         %11 = OpCompositeExtract %int %10 1
               OpStore %res %11
               OpReturn
               OpFunctionEnd
'''.format(vec_width, CONSTANTS['int'], ''.join(['%const '] * vec_width)))

        files.append(name)

    return files


def gen_vector_shuffle_kernels(args):
    ''' Generate .spvasm files to test OpVectorShuffle

        Arguments:
            args: argument parser object setup by the script
    '''

    files = []

    for glsl_type in NUMERICAL_TYPES:
        name = 'op_vector_shuffle_{0}.spvasm'.format(glsl_type)
        name = posixpath.join(args.output_dir, name)

        with open(name, 'w') as f:
            f.write(HEADER + '''OpName %main "main"
               OpName %res "res"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
        %{0} = {1}
     %v3_{0} = OpTypeVector %{0} 3
 %ptr_v3_{0} = OpTypePointer Function %v3_{0}
         %10 = OpConstant %{0} {2}
         %11 = OpConstantComposite %v3_{0} %10 %10 %10
         %13 = OpConstant %{0} {2}
         %14 = OpConstantComposite %v3_{0} %13 %13 %13
       %main = OpFunction %void None %3
          %5 = OpLabel
        %res = OpVariable %ptr_v3_{0} Function
         %23 = OpVectorShuffle %v3_{0} %11 %14 0 3 4
               OpStore %res %23
               OpReturn
               OpFunctionEnd
'''.format(glsl_type, TYPE_DECLARATIONS[glsl_type], CONSTANTS[glsl_type]))

        files.append(name)

        # test swizzle functionality by passing an undef as the second operand

        name = 'op_vector_swizzle_{0}.spvasm'.format(glsl_type)
        name = posixpath.join(args.output_dir, name)

        with open(name, 'w') as f:
            f.write(HEADER + '''OpName %main "main"
               OpName %res "res"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
        %{0} = {1}
     %v3_{0} = OpTypeVector %{0} 3
 %ptr_v3_{0} = OpTypePointer Function %v3_{0}
         %10 = OpConstant %{0} {2}
         %11 = OpConstantComposite %v3_{0} %10 %10 %10
      %undef = OpUndef %v3_{0}
       %main = OpFunction %void None %3
          %5 = OpLabel
        %res = OpVariable %ptr_v3_{0} Function
         %23 = OpVectorShuffle %v3_{0} %11 %undef 0 0 1
               OpStore %res %23
               OpReturn
               OpFunctionEnd
'''.format(glsl_type, TYPE_DECLARATIONS[glsl_type], CONSTANTS[glsl_type]))

        files.append(name)

    return files


def gen_unreachable_kernel(args):
    ''' Generate a kernel to test OpUnreachable
    '''

    name = 'op_unreachable.spvasm'
    name = posixpath.join(args.output_dir, name)

    with open(name, 'w') as f:
        f.write(HEADER + '''OpName %a "a"
       %void = OpTypeVoid
          %5 = OpTypeFunction %void
        %int = OpTypeInt 32 1
%_ptr_Function_int = OpTypePointer Function %int
          %8 = OpConstant %int 42
       %main = OpFunction %void None %5
         %10 = OpLabel
          %a = OpVariable %_ptr_Function_int Function
               OpStore %a %8
         %11 = OpLoad %int %a
               OpSelectionMerge %12 None
               OpSwitch %11 %12 42 %13
         %13 = OpLabel
         %15 = OpLoad %int %a
         %16 = OpIAdd %int %15 %8
               OpStore %a %16
               OpBranch %17
         %12 = OpLabel
               OpUnreachable
         %17 = OpLabel
               OpReturn
               OpFunctionEnd
''')

    return [name]


def gen_nop_kernel(args):
    ''' Generates a SPIR-V kernel that makes use of OpNop

        Arguments:
            args: argument parser object setup by the script'''

    name = 'op_nop.spvasm'
    name = posixpath.join(args.output_dir, name)

    with open(name, 'w') as f:
        f.write(HEADER + '''%void = OpTypeVoid
          %3 = OpTypeFunction %void
       %main = OpFunction %void None %3
          %5 = OpLabel
               OpNop
               OpReturn
               OpFunctionEnd
''')

    return [name]


def gen_debug_kernel(args):
    ''' Generates a kernel which includes source level debug information via
    OpLine and OpString

    Arguments:
        args: argument parser object setup by the script
    '''
    name = posixpath.join(args.output_dir, 'debug_info.spvasm')
    with open(name, 'w') as f:
        f.write(
            HEADER.replace('OpName %main "main"', '') +
            '''%file = OpString "source/vk/test/Lit/tests/debug_info.spvasm"
               OpName %main "main"
               OpName %a "a"
               OpName %b "b"
               OpName %file "file"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
       %bool = OpTypeBool
%_ptr_Function_bool = OpTypePointer Function %bool
          %9 = OpConstantFalse %bool
        %int = OpTypeInt 32 1
%_ptr_Function_int = OpTypePointer Function %int
         %13 = OpConstant %int 42
       %main = OpFunction %void None %3
          %5 = OpLabel
               OpLine %file 4 5
          %a = OpVariable %_ptr_Function_bool Function
               OpStore %a %9
               OpLine %file 5 5
          %b = OpVariable %_ptr_Function_int Function
               OpStore %b %13
               OpLine %file 6 5
         %14 = OpLoad %bool %a
               OpSelectionMerge %16 None
               OpBranchConditional %14 %15 %16
         %15 = OpLabel
               OpLine %file 7 5
         %17 = OpLoad %int %b
         %18 = OpIAdd %int %17 %13
               OpStore %b %18
               OpLine %file 8 5
               OpBranch %16
         %16 = OpLabel
               OpNoLine
               OpReturn
               OpFunctionEnd
''')
    return [name]


def gen_loop_merge_kernel(args):
    ''' Generates a kernel which tests OpLoopMerge

        Arguments:
            args: argument parser object setup by the script
    '''

    name = posixpath.join(args.output_dir, 'op_loop_merge.spvasm')

    with open(name, 'w') as f:
        f.write(HEADER + '''OpName %a "a"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
        %int = OpTypeInt 32 1
%_ptr_Function_int = OpTypePointer Function %int
      %int_42 = OpConstant %int 42
      %int_0 = OpConstant %int 0
       %bool = OpTypeBool
      %int_1 = OpConstant %int 1
       %main = OpFunction %void None %3
          %5 = OpLabel
          %a = OpVariable %_ptr_Function_int Function
               OpStore %a %int_42
               OpBranch %10
         %10 = OpLabel
               OpLoopMerge %12 %13 Unroll
               OpBranch %14
         %14 = OpLabel
         %15 = OpLoad %int %a
         %18 = OpSGreaterThan %bool %15 %int_0
               OpBranchConditional %18 %11 %12 5 2
         %11 = OpLabel
         %19 = OpLoad %int %a
         %21 = OpISub %int %19 %int_1
               OpStore %a %21
               OpBranch %13
         %13 = OpLabel
               OpBranch %10
         %12 = OpLabel
               OpReturn
               OpFunctionEnd
''')

    return [name]


def gen_source_continued(args):
    ''' Generates a SPIR-V kernel with opSourceContinued instructions
            which cannot currently by generated by glslc

        Arguments:
            args: argument parser object setup by the script'''

    name = 'op_source_continued.spvasm'
    name = posixpath.join(args.output_dir, name)

    with open(name, 'w') as f:
        f.write(''' OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
      %file1 = OpString "fakeShaderName.comp"
      %file2 = OpString "otherShader.comp"
               OpSource GLSL 450 %file1 "Test A "
               OpSourceContinued "Test B"
               OpSource ESSL 100 %file2 ""
               OpSourceContinued "Test C"
               OpSourceExtension "GL_GOOGLE_cpp_style_line_directive"
               OpSourceExtension "GL_GOOGLE_include_directive"
               OpName %main "main"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
       %main = OpFunction %void None %3
          %5 = OpLabel
               OpReturn
               OpFunctionEnd
''')
    return [name]


def gen_kernel_builtin_insts(args):
    ''' Generates spvasm file that test the kernel capability
        instructions which use abacus builtins

        Arguments:
            args: argument parser object setup by the script
    '''
    files = []

    for inst in ['OpIsFinite', 'OpIsNormal', 'OpSignBitSet']:
        name = get_test_name_from_op(inst) + '.spvasm'
        name = posixpath.join(args.output_dir, name)

        with open(name, 'w') as f:
            f.write('''
               OpCapability Kernel
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpSource GLSL 450
               OpSourceExtension "GL_GOOGLE_cpp_style_line_directive"
               OpSourceExtension "GL_GOOGLE_include_directive"
               OpName %main "main"
               OpName %value "value"
               OpName %res "res"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
%float_42_42 = OpConstant %float 42.42
       %bool = OpTypeBool
%_ptr_Function_bool = OpTypePointer Function %bool
       %main = OpFunction %void None %3
          %5 = OpLabel
      %value = OpVariable %_ptr_Function_float Function
        %res = OpVariable %_ptr_Function_bool Function
               OpStore %value %float_42_42
         %13 = OpLoad %float %value
         %14 = {0} %bool %13
               OpStore %res %14
               OpReturn
               OpFunctionEnd
'''.format(inst))

        files.append(name)

    for inst in ['OpUnordered', 'OpOrdered', 'OpLessOrGreater']:
        name = get_test_name_from_op(inst) + '.spvasm'
        name = posixpath.join(args.output_dir, name)

        with open(name, 'w') as f:
            f.write('''
               OpCapability Kernel
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpSourceExtension "GL_GOOGLE_cpp_style_line_directive"
               OpSourceExtension "GL_GOOGLE_include_directive"
               OpName %main "main"
               OpName %x "x"
               OpName %y "y"
               OpName %res "res"
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
       %bool = OpTypeBool
%_ptr_Function_float = OpTypePointer Function %float
%_ptr_Function_bool = OpTypePointer Function %bool
%float_42_42 = OpConstant %float 42.42
       %main = OpFunction %void None %3
          %5 = OpLabel
          %x = OpVariable %_ptr_Function_float Function
          %y = OpVariable %_ptr_Function_float Function
        %res = OpVariable %_ptr_Function_bool Function
               OpStore %x %float_42_42
               OpStore %y %float_42_42
         %12 = OpLoad %float %x
         %13 = OpLoad %float %y
         %14 = {0} %bool %12 %13
               OpStore %res %14
               OpReturn
               OpFunctionEnd
'''.format(inst))
        files.append(name)

    return files


def edit_spvasm_file(inst, ty, f):
    ''' Generate .spvasm file to test the given instruction

        Arguments:
            inst(string): instruction to test
            ty(string): type of operands to be passed to the instruction
            f: file object to write the .spvasm to
    '''

    param_list = []

    for param in inst['operands']:
        param_list.append(param['name'].replace("'", ''))

    f.write(HEADER)

    for param in param_list:
        f.write('OpName %{0} "{0}"\n'.format(param))
    f.write('OpName %r "r"\n')

    f.write('''%void = OpTypeVoid
            %3 = OpTypeFunction %void
            %{0} = {1}
            %_ptr_Function_{0} = OpTypePointer Function %{0}
            %9 = OpConstant %{0} {2}
            %main = OpFunction %void None %3
            %5 = OpLabel
'''.format(ty, TYPE_DECLARATIONS[ty], CONSTANTS[ty]))

    # create a variable for each operand
    for param in param_list:
        f.write('%{0} = OpVariable %_ptr_Function_{1} Function\n'.format(
            param, ty))
    f.write('%r = OpVariable %_ptr_Function_' + ty + ' Function\n')

    # create a store for each operand
    for param in param_list:
        f.write('OpStore %' + param + ' %9\n')

    # create a load for each operand
    idc = 10
    for param in param_list:
        f.write('%{0} = OpLoad %{1} %{2}\n'.format(str(idc), ty, param))
        idc += 1

    # create the instruction being tested
    f.write(
        '%{0} = OpExtInst %{1} %1 {2} '.format(str(idc), ty, inst['opname']))
    idc = idc - len(param_list)
    for param in param_list:
        f.write('%' + str(idc) + ' ')
        idc += 1
    f.write('\n')

    # store the result and end the function
    idc = idc + len(inst['operands']) - 1
    f.write('''OpStore %r %{0}
            OpReturn
            OpFunctionEnd
'''.format(str(idc)))


def gen_modf_or_frexp_tests(args, inst):
    ''' Generates .spvasm files for the modf or frexp extended instruction.

        Arguments:
            args: argument parser object setup by the script
            inst: the name of the instruction, either "Frexp" or "Modf"
    '''
    files = []

    # Variables that differ between Frexp/Modf:
    outbuffer_member_names = []
    # scaler type or component type in vectors:
    outbuffer_member2_raw_type = ''

    assert (inst == 'Frexp' or inst == 'Modf'
            ), 'Did not pass Frexp or Modf as inst to gen_modf_or_frexp_tests!'

    if inst == "Frexp":
        outbuffer_member_names = ['significand', 'exponent']
        outbuffer_member2_raw_type = 'int'
    elif inst == "Modf":
        outbuffer_member_names = ['fract', 'whole']
        outbuffer_member2_raw_type = 'float'

    # Scalar types (with size)
    glsl_scalar_types = {'float': 32, 'double': 64}

    for type, size in glsl_scalar_types.items():
        if type == 'double':
            capability = 'OpCapability Float64'

        if inst == "Frexp":
            name = get_builtin_test_name('Frexp', [type, 'int*']) + '.spvasm'
        elif inst == "Modf":
            name = get_builtin_test_name('Modf', [type, type + '*'
                                                  ]) + '.spvasm'
        name = posixpath.join(args.output_dir, name)

        with open(name, 'w') as f:
            f.write('''                         OpCapability Shader
                         {1}
        %1             = OpExtInstImport "GLSL.std.450"
                         OpMemoryModel Logical GLSL450
                         OpEntryPoint GLCompute %main "main"
                         OpExecutionMode %main LocalSize 1 1 1
                         OpSource GLSL 450
                         OpName %main "main"
                         OpName %inBuff "inBuff"
                         OpName %inBuff_t "inBuff_t"
                         OpMemberName %inBuff_t 0 "x"
                         OpName %outBuff "outBuff"
                         OpName %outBuff_t "outBuff_t"
                         OpMemberName %outBuff_t 0 "{2}"
                         OpMemberName %outBuff_t 1 "{3}"
                         OpMemberDecorate %outBuff_t 0 Offset 0
                         OpMemberDecorate %outBuff_t 1 Offset {4}
                         OpDecorate %outBuff BufferBlock
                         OpDecorate %outBuff DescriptorSet 0
                         OpDecorate %outBuff Binding 1
                         OpMemberDecorate %inBuff_t 0 Offset 0
                         OpDecorate %inBuff BufferBlock
                         OpDecorate %inBuff DescriptorSet 0
                         OpDecorate %inBuff Binding 0
     %void_t           = OpTypeVoid
 %voidFnct_t           = OpTypeFunction %void_t
    %float_t           = OpTypeFloat {0}
%float_ptr_Uniform_t = OpTypePointer Uniform %float_t
     %int_t            = OpTypeInt 32 1
%int_ptr_Uniform_t     = OpTypePointer Uniform %int_t
  %inBuff_t            = OpTypeStruct %float_t
 %outBuff_t            = OpTypeStruct %float_t %{6}_t
%inBuff_ptr_Uniform_t  = OpTypePointer Uniform %inBuff_t
%outBuff_ptr_Uniform_t = OpTypePointer Uniform %outBuff_t
%int_0                 = OpConstant %int_t 0
%int_1                 = OpConstant %int_t 1
%inBuff                = OpVariable %inBuff_ptr_Uniform_t Uniform
%outBuff               = OpVariable %outBuff_ptr_Uniform_t Uniform
%main                  = OpFunction %void_t None %voidFnct_t
%mainEntry             = OpLabel
%ptrTox                = OpAccessChain %float_ptr_Uniform_t %inBuff %int_0
%x                     = OpLoad %float_t %ptrTox
%ptrToSecond           = OpAccessChain %{6}_ptr_Uniform_t %outBuff %int_1
%res                   = OpExtInst %float_t %1 {5} %x %ptrToSecond
%ptrToret              = OpAccessChain %float_ptr_Uniform_t %outBuff %int_0
                         OpStore %ptrToret %res
     OpReturn
     OpFunctionEnd'''.format(
                str(size), capability, outbuffer_member_names[
                    0], outbuffer_member_names[1], size / 8, inst,
                outbuffer_member2_raw_type))
        files += [name]

    # Vector version:
    vec_elem_sizes = {'vec': 32, 'dvec': 64}
    for type, elem_size in vec_elem_sizes.items():
        if type == 'double':
            capability = 'OpCapability Float64'

        for i in range(2, 5):
            if inst == "Frexp":
                name = get_builtin_test_name('Frexp', [
                    type + str(i), 'ivec{0}*'.format(str(i))
                ]) + '.spvasm'
            elif inst == "Modf":
                name = get_builtin_test_name(
                    'Modf', [type + str(i), type + str(i) + '*']) + '.spvasm'

            name = posixpath.join(args.output_dir, name)

            # Number of bits for type, including padding bytes inserted after
            # type
            size = 0
            # 3 component vectors have an additional padding component:
            if i == 3:
                size = 4 * elem_size
            else:
                size = i * elem_size

            with open(name, 'w') as f:
                f.write('''                         OpCapability Shader
                         {2}
        %1             = OpExtInstImport "GLSL.std.450"
                         OpMemoryModel Logical GLSL450
                         OpEntryPoint GLCompute %main "main"
                         OpExecutionMode %main LocalSize 1 1 1
                         OpSource GLSL 450
                         OpName %main "main"
                         OpName %inBuff "inBuff"
                         OpName %inBuff_t "inBuff_t"
                         OpMemberName %inBuff_t 0 "x"
                         OpName %outBuff "outBuff"
                         OpName %outBuff_t "outBuff_t"
                         OpMemberName %outBuff_t 0 "{3}"
                         OpMemberName %outBuff_t 1 "{4}"
                         OpMemberDecorate %outBuff_t 0 Offset 0
                         OpMemberDecorate %outBuff_t 1 Offset {5}
                         OpDecorate %outBuff BufferBlock
                         OpDecorate %outBuff DescriptorSet 0
                         OpDecorate %outBuff Binding 1
                         OpMemberDecorate %inBuff_t 0 Offset 0
                         OpDecorate %inBuff BufferBlock
                         OpDecorate %inBuff DescriptorSet 0
                         OpDecorate %inBuff Binding 0
     %void_t           = OpTypeVoid
 %voidFnct_t           = OpTypeFunction %void_t
    %float_t           = OpTypeFloat {0}
 %vec_float_t           = OpTypeVector %float_t {1}
%vec_float_ptr_Uniform_t = OpTypePointer Uniform %vec_float_t
     %int_t            = OpTypeInt 32 1
  %vec_int_t            = OpTypeVector %int_t {1}
%vec_int_ptr_Uniform_t  = OpTypePointer Uniform %vec_int_t
  %inBuff_t            = OpTypeStruct %vec_float_t
 %outBuff_t            = OpTypeStruct %vec_float_t %vec_{7}_t
%inBuff_ptr_Uniform_t  = OpTypePointer Uniform %inBuff_t
%outBuff_ptr_Uniform_t = OpTypePointer Uniform %outBuff_t
%int_0                 = OpConstant %int_t 0
%int_1                 = OpConstant %int_t 1
%inBuff                = OpVariable %inBuff_ptr_Uniform_t Uniform
%outBuff               = OpVariable %outBuff_ptr_Uniform_t Uniform
%main                  = OpFunction %void_t None %voidFnct_t
%mainEntry             = OpLabel
%ptrTox                = OpAccessChain %vec_float_ptr_Uniform_t %inBuff %int_0
%x                     = OpLoad %vec_float_t %ptrTox
%ptrToSecond           = OpAccessChain %vec_{7}_ptr_Uniform_t %outBuff %int_1
%res                   = OpExtInst %vec_float_t %1 {6} %x %ptrToSecond
%ptrToret              = OpAccessChain %vec_float_ptr_Uniform_t %outBuff %int_0
                         OpStore %ptrToret %res
                         OpReturn
                         OpFunctionEnd
'''.format(vec_elem_sizes[type],
           str(i), capability, outbuffer_member_names[
               0], outbuffer_member_names[1], size / 8, inst,
           outbuffer_member2_raw_type))
                files += [name]

    return files


def gen_frexpstruct_tests(args):
    ''' Generates .spvasm files for testing the frexpstruct instruction. This
        is required because there is not native frexpstruct instruction in
        GLSL.

        Arguments:
            args: argument parser object setup by the script
    '''
    files = []

    glsl_type_sizes = {'float': 32, 'double': 64}

    # Scalar
    for type, size in glsl_type_sizes.items():
        name = get_builtin_test_name('FrexpStruct', [type]) + '.spvasm'
        name = posixpath.join(args.output_dir, name)
        with open(name, 'w') as f:
            f.write('''               OpCapability Shader
               OpCapability Float64
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpSource GLSL 450
               OpMemberDecorate %inputBuffer 0 Offset 0
               ; ^ set offset of first element
               OpDecorate %inputBuffer BufferBlock

               ; v Bind variable to descriptor
               OpDecorate %inBufferVar DescriptorSet 0
               OpDecorate %inBufferVar Binding 0

               ; same for output buffer:
               OpMemberDecorate %outputBuffer 0 Offset 0
               OpDecorate %outputBuffer BufferBlock

               ; V Bind variable to descriptor
               OpDecorate %outBufferVar DescriptorSet 0
               OpDecorate %outBufferVar Binding 1

               ; Set up member offsets of ResType
               OpMemberDecorate %ResType 0 Offset 0
               OpMemberDecorate %ResType 1 Offset {1}

       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat {0}
%ptrToGfloat = OpTypePointer Uniform %float
%inputBuffer = OpTypeStruct %float
; ^ this is the type of the input buffer
%ptrInputBuf = OpTypePointer Uniform %inputBuffer
; declare input buffer:
%inBufferVar = OpVariable %ptrInputBuf Uniform
        %int = OpTypeInt 32 1
      %int_0 = OpConstant %int 0
    %ResType = OpTypeStruct %float %int
%ptrToResType = OpTypePointer Uniform %ResType
; declare type of output buffer:
%outputBuffer = OpTypeStruct %ResType
; declare output buffer:
%ptrOutBuffer = OpTypePointer Uniform %outputBuffer
%outBufferVar = OpVariable %ptrOutBuffer Uniform


       %main = OpFunction %void None %3
          %5 = OpLabel
          ; get input variable from descriptor:
 %ptrToInput = OpAccessChain %ptrToGfloat %inBufferVar %int_0
        %arg = OpLoad %float %ptrToInput
     %result = OpExtInst %ResType %1 FrexpStruct %arg
; store result in descriptor:
%ptrToResult = OpAccessChain %ptrToResType %outBufferVar %int_0
               OpStore %ptrToResult %result
               OpReturn
               OpFunctionEnd'''.format(size, size / 8))

            files += [name]

    vec_elem_sizes = {'vec': 32, 'dvec': 64}
    for type, elem_size in vec_elem_sizes.items():
        for i in range(2, 5):
            name = get_builtin_test_name('FrexpStruct', [type + str(i)
                                                         ]) + '.spvasm'
            name = posixpath.join(args.output_dir, name)

            vec_type_name = type + str(i)

            # Byte sizes of GLSL vector types with padding
            GLSL_VEC_SIZES = {
                'vec2': 8,
                'vec3': 16,
                'vec4': 16,
                'dvec2': 16,
                'dvec3': 32,
                'dvec4': 32,
            }

            # calculate size of resultant vector, accounting for padding
            result_vec_sz = GLSL_VEC_SIZES[vec_type_name]

            with open(name, 'w') as f:
                f.write('''
               OpCapability Shader
               OpCapability Float64
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpSource GLSL 450
               OpMemberDecorate %inputBuffer 0 Offset 0
               ; ^ set offset of first element
               OpDecorate %inputBuffer BufferBlock

               ; v Bind variable to descriptor
               OpDecorate %inBufferVar DescriptorSet 0
               OpDecorate %inBufferVar Binding 0

               ; same for output buffer:
               OpMemberDecorate %outputBuffer 0 Offset 0
               OpDecorate %outputBuffer BufferBlock

               ; V Bind variable to descriptor
               OpDecorate %outBufferVar DescriptorSet 0
               OpDecorate %outBufferVar Binding 1

               ; Set up member offsets of ResType
               OpMemberDecorate %ResType 0 Offset 0
               OpMemberDecorate %ResType 1 Offset {2}

       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat {0}
   %floatVec = OpTypeVector %float {1}
%ptrToGfvec  = OpTypePointer Uniform %floatVec
%inputBuffer = OpTypeStruct %floatVec
; ^ this is the type of the input buffer
%ptrInputBuf = OpTypePointer Uniform %inputBuffer
; declare input buffer:
%inBufferVar = OpVariable %ptrInputBuf Uniform
        %int = OpTypeInt 32 1
     %intVec = OpTypeVector %int {1}
      %int_0 = OpConstant %int 0
    %ResType = OpTypeStruct %floatVec %intVec
%ptrToResType = OpTypePointer Uniform %ResType
; declare type of output buffer:
%outputBuffer = OpTypeStruct %ResType
; declare output buffer:
%ptrOutBuffer = OpTypePointer Uniform %outputBuffer
%outBufferVar = OpVariable %ptrOutBuffer Uniform


       %main = OpFunction %void None %3
          %5 = OpLabel
          ; get input variable from descriptor:
 %ptrToInput = OpAccessChain %ptrToGfvec %inBufferVar %int_0
        %arg = OpLoad %floatVec %ptrToInput
     %result = OpExtInst %ResType %1 FrexpStruct %arg
; store result in descriptor:
%ptrToResult = OpAccessChain %ptrToResType %outBufferVar %int_0
               OpStore %ptrToResult %result
               OpReturn
               OpFunctionEnd
'''.format(elem_size, i, result_vec_sz))

                files += [name]

    return files


def gen_modfstruct_tests(args):
    ''' Generates .spvasm files for testing the modfstruct instruction. This
        is required because there is not native modfstruct instruction in GLSL.

        Arguments:
            args: argument parser object setup by the script
    '''
    files = []

    glsl_type_sizes = {'float': 32, 'double': 64}

    # Scalar
    for type, size in glsl_type_sizes.items():
        name = get_builtin_test_name('ModfStruct', [type]) + '.spvasm'
        name = posixpath.join(args.output_dir, name)
        with open(name, 'w') as f:
            f.write('''               OpCapability Shader
               OpCapability Float64
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpSource GLSL 450
               OpMemberDecorate %inputBuffer 0 Offset 0
               ; ^ set offset of first element
               OpDecorate %inputBuffer BufferBlock

               ; v Bind variable to descriptor
               OpDecorate %inBufferVar DescriptorSet 0
               OpDecorate %inBufferVar Binding 0

               ; same for output buffer:
               OpMemberDecorate %outputBuffer 0 Offset 0
               OpDecorate %outputBuffer BufferBlock

               ; V Bind variable to descriptor
               OpDecorate %outBufferVar DescriptorSet 0
               OpDecorate %outBufferVar Binding 1

               ; Set up member offsets of ResType
               OpMemberDecorate %ResType 0 Offset 0
               OpMemberDecorate %ResType 1 Offset {1}

       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat {0}
%ptrToGfloat = OpTypePointer Uniform %float
%inputBuffer = OpTypeStruct %float
; ^ this is the type of the input buffer
%ptrInputBuf = OpTypePointer Uniform %inputBuffer
; declare input buffer:
%inBufferVar = OpVariable %ptrInputBuf Uniform
        %int = OpTypeInt 32 1
      %int_0 = OpConstant %int 0
    %ResType = OpTypeStruct %float %float
%ptrToResType = OpTypePointer Uniform %ResType
; declare type of output buffer:
%outputBuffer = OpTypeStruct %ResType
; declare output buffer:
%ptrOutBuffer = OpTypePointer Uniform %outputBuffer
%outBufferVar = OpVariable %ptrOutBuffer Uniform


       %main = OpFunction %void None %3
          %5 = OpLabel
          ; get input variable from descriptor:
 %ptrToInput = OpAccessChain %ptrToGfloat %inBufferVar %int_0
        %arg = OpLoad %float %ptrToInput
     %result = OpExtInst %ResType %1 ModfStruct %arg
; store result in descriptor:
%ptrToResult = OpAccessChain %ptrToResType %outBufferVar %int_0
               OpStore %ptrToResult %result
               OpReturn
               OpFunctionEnd'''.format(size, size / 8))

            files += [name]

    vec_elem_sizes = {'vec': 32, 'dvec': 64}
    for type, elem_size in vec_elem_sizes.items():
        for i in range(2, 5):
            vec_type_name = type + str(i)

            name = get_builtin_test_name('ModfStruct', [vec_type_name
                                                        ]) + '.spvasm'
            name = posixpath.join(args.output_dir, name)

            # Byte sizes of GLSL vector types with padding
            GLSL_PADDED_VEC_SIZES = {
                'vec2': 8,
                'vec3': 16,
                'vec4': 16,
                'dvec2': 16,
                'dvec3': 32,
                'dvec4': 32,
            }

            # calculate size of resultant vector, accounting for padding
            result_vec_sz = GLSL_PADDED_VEC_SIZES[vec_type_name]

            with open(name, 'w') as f:
                f.write('''
               OpCapability Shader
               OpCapability Float64
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpSource GLSL 450
               OpMemberDecorate %inputBuffer 0 Offset 0
               ; ^ set offset of first element
               OpDecorate %inputBuffer BufferBlock

               ; v Bind variable to descriptor
               OpDecorate %inBufferVar DescriptorSet 0
               OpDecorate %inBufferVar Binding 0

               ; same for output buffer:
               OpMemberDecorate %outputBuffer 0 Offset 0
               OpDecorate %outputBuffer BufferBlock

               ; V Bind variable to descriptor
               OpDecorate %outBufferVar DescriptorSet 0
               OpDecorate %outBufferVar Binding 1

               ; Set up member offsets of ResType
               OpMemberDecorate %ResType 0 Offset 0
               OpMemberDecorate %ResType 1 Offset {2}

       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat {0}
   %floatVec = OpTypeVector %float {1}
%ptrToGfvec  = OpTypePointer Uniform %floatVec
%inputBuffer = OpTypeStruct %floatVec
; ^ this is the type of the input buffer
%ptrInputBuf = OpTypePointer Uniform %inputBuffer
; declare input buffer:
%inBufferVar = OpVariable %ptrInputBuf Uniform
        %int = OpTypeInt 32 1
      %int_0 = OpConstant %int 0
    %ResType = OpTypeStruct %floatVec %floatVec
%ptrToResType = OpTypePointer Uniform %ResType
; declare type of output buffer:
%outputBuffer = OpTypeStruct %ResType
; declare output buffer:
%ptrOutBuffer = OpTypePointer Uniform %outputBuffer
%outBufferVar = OpVariable %ptrOutBuffer Uniform


       %main = OpFunction %void None %3
          %5 = OpLabel
          ; get input variable from descriptor:
 %ptrToInput = OpAccessChain %ptrToGfvec %inBufferVar %int_0
        %arg = OpLoad %floatVec %ptrToInput
     %result = OpExtInst %ResType %1 ModfStruct %arg
; store result in descriptor:
%ptrToResult = OpAccessChain %ptrToResType %outBufferVar %int_0
               OpStore %ptrToResult %result
               OpReturn
               OpFunctionEnd
'''.format(elem_size, i, result_vec_sz))

                files += [name]

    return files


def gen_spvasm_files(args):
    '''Generates .spvasm files to test extended instructions parsed from a
    grammar and some other instructions that the glsl frontend wont generate

    Arguments:
        args: argument parser object containing at least an output directory
    '''

    global GRAMMAR  # pylint: disable=global-statement

    with open(args.extension) as ext_file:
        GRAMMAR = json.load(ext_file)['instructions']

    files = []

    files.extend(gen_bitcast_kernels(args))
    files.extend(gen_unary_kernels(args))
    files.extend(gen_memcpy_kernels(args))
    files.extend(gen_overflow_arithmetic(args))
    files.extend(gen_remainder_kernels(args))
    files.extend(gen_select_kernels(args))
    files.extend(gen_vec_kernels(args))
    files.extend(gen_unord_cmp_kernels(args))
    files.extend(gen_long_conversion_kernels(args))
    files.extend(gen_forward_pointer_kernels(args))
    files.extend(gen_branch_conditional_kernels(args))
    files.extend(gen_ptr_int_conversion_kernels(args))
    files.extend(gen_spec_constant_kernels(args))
    files.extend(gen_spec_constant_composite_kernels(args))
    files.extend(gen_spec_constant_op_kernels(args))
    files.extend(gen_null_constant_kernels(args))
    files.extend(gen_composite_insert_kernels(args))
    files.extend(gen_phi_kernel(args))
    files.extend(gen_vec_extract_kernels(args))
    files.extend(gen_vector_shuffle_kernels(args))
    files.extend(gen_unreachable_kernel(args))
    files.extend(gen_nop_kernel(args))
    files.extend(gen_debug_kernel(args))
    files.extend(gen_loop_merge_kernel(args))
    files.extend(gen_source_continued(args))
    files.extend(gen_kernel_builtin_insts(args))
    # Most extended instructions are generated by glsl_generator.py. Any
    # instructions which cannot be generated by glslc should be generated here.
    files.extend(gen_frexpstruct_tests(args))
    files.extend(gen_modfstruct_tests(args))
    files.extend(gen_modf_or_frexp_tests(args, "Modf"))
    files.extend(gen_modf_or_frexp_tests(args, "Frexp"))
    return files
