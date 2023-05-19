#!/usr/bin/env python3.6

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
''' This file generates the declarations of the subset of C11 required
 by the OpenCL-3.0 spec.
'''

import sys

type_pairs = {
    'atomic_int': 'int',
    'atomic_uint': 'uint',
    'atomic_float': 'float',
}

address_spaces = ['__local', '__global']


def generate_init():
    '''Generates the atomic_init builtins.'''
    return [
        f'void atomic_init(volatile {AS} {A} *obj, {C} value);'
        for AS in address_spaces for A, C in type_pairs.items()
    ]


def generate_fence():
    '''
    Generate the atomic_work_item_fence builtin.
    '''
    return [
        'void atomic_work_item_fence(cl_mem_fence_flags flags, memory_order order, memory_scope scope);'
    ]


def generate_st():
    '''Generate the atomic_store_explicit builtins.'''
    return [
        f'void atomic_store_explicit(volatile {AS} {A} *object, {C} desired, memory_order order, memory_scope scope);'
        for AS in address_spaces for A, C in type_pairs.items()
    ]


def generate_ld():
    '''
    Generate the atomic_load_explicit builtins..
    '''
    return [
        f'{C} atomic_load_explicit(volatile {AS} {A} *object, memory_order order, memory_scope scope);'
        for AS in address_spaces for A, C in type_pairs.items()
    ]


def generate_exc():
    '''
    Generate atomic_exchange_explicit builtlins.
    '''
    return [
        f'{C} atomic_exchange_explicit(volatile {AS} {A} *object, {C} desired, memory_order order, memory_scope scope);'
        for AS in address_spaces for A, C in type_pairs.items()
    ]


def generate_cmp_exc():
    '''
    Generate the ate_atomic_compare_exchange_(strong|weak)_explicit builtins.
    '''
    all_address_spaces = address_spaces + ['__private']
    scopes = ['', ', memory_scope scope']
    strengths = ['strong', 'weak']

    return [
        f'bool atomic_compare_exchange_{strength}_explicit(volatile {AS1} {A} *object, {AS2} {C} *expected, {C} desired, memory_order success, memory_order failure{scope});'
        for strength in strengths for AS1 in address_spaces
        for A, C in type_pairs.items() for AS2 in all_address_spaces
        for scope in scopes
    ]


def generate_fetch_key():
    '''
    Generate the atomic_fetch_key_explicit overloads.
    Fetch operations are only defined for integer types.
    '''
    keys = ['add', 'sub', 'or', 'xor', 'and', 'min', 'max']

    def getM(key, atomic_type_name):
        '''
        Get the M parameter corresponding to the type of A.
        '''
        if (key == 'add'
                or key == 'sub') and (atomic_type_name == 'atomic_inptr_t' or
                                      atomic_type_name == 'atomic_uintptr_t'):
            return 'ptrdiff_t'
        else:
            return type_pairs[atomic_type_name]

    return [
        f'{C} atomic_fetch_{key}_explicit(volatile {AS} {A} *object, {getM(key,A)} operand, memory_order order, memory_scope scope);'
        for A, C in type_pairs.items()
        if A != 'atomic_float' and A != 'atomic_double' for key in keys
        for AS in address_spaces
    ]


def generate_flag():
    '''
    Generate the atomic_flag_test_and_set_explicit and
    atomic_flag_clear_explicit overloads.
    '''
    names = ['test_and_set_explicit', 'clear_explicit']
    return [
        f'bool atomic_flag_{name}(volatile {AS} atomic_flag *object, memory_order order, memory_scope scope);'
        for name in names for AS in address_spaces
    ]


def main():
    '''Wrapper for core logic'''

    print('#if __OPENCL_C_VERSION__ >= 300')
    print(
        'typedef enum { memory_order_relaxed, memory_order_acquire, memory_order_release, memory_order_acq_rel } memory_order;'
    )
    print(
        'typedef enum { memory_scope_work_group, memory_scope_work_item } memory_scope;'
    )
    declarations = generate_init()
    declarations += generate_fence()
    declarations += generate_st()
    declarations += generate_ld()
    declarations += generate_exc()
    declarations += generate_cmp_exc()
    declarations += generate_fetch_key()
    declarations += generate_flag()
    for declaration in declarations:
        idx = declaration.find(' ')
        new_decl = declaration[:
                               idx] + ' __CL_BUILTIN_ATTRIBUTES' + declaration[idx:]
        print(new_decl)

    print('#endif')


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(130)
