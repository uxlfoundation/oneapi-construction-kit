#! /usr/bin/env python3
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
"""
Create a target based off cookie cutter scripts
Note this needs cookiecutter installed e.g. pip install --user cookiecutter

Although other context information can be used for the target generation, a default
path is given for default values (<base_computeaorta_dir>/scripts/default_target_cookie.json)
The context information used for the generation of the new target is as follows:
Mandatory values:
        "target_name"               : target name e.g. refsi
        "llvm_name"                 : llvm name used for functions such as
                                      LLVMInitialize{{cookiecutter.llvm_name}}Target()
        "llvm_cpu"                  : llvm cpu name string e.g. generic-rv64.
                                      If ' is at start and end of string, then these will be
                                      replaced with " otherwise it is assumed as an expression.
        "llvm_triple"               : llvm target triple string to be used e.g.
                                      riscv64-unknown-elf.
        "llvm_cpu"                  : llvm cpu name string e.g. generic-rv64.
                                      If ' is at start and end of string, then these will be
                                      replaced with " otherwise it is assumed as an expression.
Optional overrides:
        "llvm_features"             : llvm features string as a comma separated list
                                      e.g. +f,+v ("" default).
                                      If ' is at start and end of string, then these will be
                                      replaced with " otherwise it is assumed as an expression.
        "link"                      : true if the final binary should be linked
                                      (true default).
        "link_shared"               : true if --shared should be used for the
                                      linker (false default).
        "scalable_vector"           : true if scalable vectorization is
                                      enabled (false default).
        "vlen"                      : vector length in bits, only relevant for scalable
                                      vectorization (1 default).
        "bit_width"                 : 32 or 64 bits.
        "capabilities_fp16"         : true if has fp16 capability else false.
        "capabilities_fp64"         : true if has fp64 capability else false.
        "feature"                   : A list of features (normally tutorials) to enable as a ';' separated string
                                      Tutorials can be part finished e.g. clmul_1 or just clmul
                                      Current list is "replace_mem", "refsi_wrapper_pass" and "clmul"
        "copyright_name"            : Optional copyright to be added to generated files

This assumes that under ComputeAorta there are cookie directories under
modules/compiler and modules/mux These should relate directly to target
information and will produce targets under modules/compiler/targets/<target_name>
and modules/mux/targets/<target_name>.

Although this script does not mandate what is in the cookie information, in the
current state of the cookies and input information, the assumptions are that a
derived HAL will be placed under
modules/mux/target/<target_name>/external/hal_<hal_name>. <hal_name> can be
anything and is not defined by the script inputs.

Other than that it is expected that it should be able to compile.
"""

import os
import json
from argparse import ArgumentParser
from argparse import RawTextHelpFormatter
from collections import OrderedDict
from sys import stderr, exit

try:
    from cookiecutter.config import get_user_config
    from cookiecutter.generate import generate_files, apply_overwrites_to_context
    from cookiecutter.repository import determine_repo_dir
    from cookiecutter.utils import rmtree
except:
    print(
        "Error: cookiecutter not installed. Please use pip install cookiecutter",
        file=stderr)
    os._exit(1)


def load_json(json_file):
    """
    load a json file into a dictionary
    :param json_file: filepath to json file
    """
    try:
        with open(json_file) as fp:
            return json.load(fp, object_pairs_hook=OrderedDict)
    except Exception as e:
        print('Error loading json file {0} : {1}"'.format(json_file,
                                                          e,
                                                          file=stderr))
        return None


def create_context(default_context, target_context, base_computeaorta_dir, override):
    """
    create a context dictionary for cookie cutter based on a default context
    and an additional context
    :param default_context: base context for setting defaults
    :param target_context: target specific context with overrides
    :param base_computeaorta_dir: path to ComputeAorta base
    :param override: An array of overrides for the dictionary
    """
    default_context_file_to_load = default_context
    target_context_file_to_load = target_context
    default_dict = load_json(default_context_file_to_load)
    context_data = load_json(target_context_file_to_load)
    if not default_dict or not context_data:
        return None
    context = OrderedDict([])
    context['cookiecutter'] = default_dict

    # Apply the context data from the target config over the default
    apply_overwrites_to_context(context['cookiecutter'], context_data)
    # Add some additional variables based on base target dir and
    # capitalisation of target_name
    context['cookiecutter']['target_name_capitals'] = context['cookiecutter'][
        'target_name'].upper()
    context['cookiecutter']['target_name_capitalize'] = context['cookiecutter'][
        'target_name'].capitalize()
    context['cookiecutter']['base_computeaorta_dir'] = base_computeaorta_dir
    for o in override:
        context['cookiecutter'][o[0]] = o[1]

    return context


def create_subtarget(subtarget_in_base,
                     subtarget_out_base,
                     context,
                     overwrite_if_exists=False,
                     skip_if_file_exists=False):
    """
    create a subtarget based on parameters for a subsection of the whole target.

    :param subtarget_in_base: Base directory path for subtarget e.g. path to
    compiler/targets
    :param subtarget_out_base: Base directory path for subtarget output e.g. path to
    compiler
    :param context: Dictionary to use for cookie cutter replacement
    :param overwrite_if_exists: Overwrite the contents of output directory
        if it exists
    :param skip_if_file_exists: Skip if file exists
    """

    template = os.path.join(subtarget_in_base, 'cookie')

    config_dict = get_user_config(
        config_file=None,
        default_config=None,
    )

    repo_dir, cleanup = determine_repo_dir(
        template=template,
        abbreviations=config_dict['abbreviations'],
        clone_to_dir=config_dict['cookiecutters_dir'],
        checkout=None,
        no_input=True)

    context['cookiecutter']['_template'] = template

    # Create project from context and subtarget template.
    try:
        result = generate_files(
            repo_dir=repo_dir,
            context=context,
            overwrite_if_exists=overwrite_if_exists,
            skip_if_file_exists=skip_if_file_exists,
            output_dir=subtarget_out_base
        )
    except Exception as e:
        print(str(e), file=stderr)
        return None

    # Cleanup (if required)
    if cleanup:
        rmtree(repo_dir)

    return result

def update_string_values(context, fields):
    for f in fields:
        val = context['cookiecutter'][f]
        if val.startswith("'") and val.endswith("'"):
            val = '"' + val[1:-1] + '"'

        context['cookiecutter'][f] = val

def main():
    parser = ArgumentParser(description=__doc__, formatter_class=RawTextHelpFormatter)
    parser.add_argument('base_computeaorta_dir',
                        help='compiler template directory')
    parser.add_argument(
        '--base-config',
        help='base json description of target (default '
        '<base_computeaorta_dir>/scripts/default_target_cookie.json>')
    parser.add_argument('--overwrite-if-file-exists',
                        action='store_true',
                        help='overwrite existing files')
    parser.add_argument('--external-dir',
                        default=os.path.abspath("."),
                        help='compiler template directory')
    parser.add_argument('--override',
                        default = [],
                        action='append',nargs=2,
                        help='override name value pair (may be repeated)')
    parser.add_argument('config', help='json override description of target')
    args = parser.parse_args()
    base_computeaorta_dir = os.path.abspath(args.base_computeaorta_dir)
    external_dir = os.path.abspath(args.external_dir)

    if args.base_config:
        base_config = os.path.abspath(args.base_config)
    else:
        base_config = os.path.join(
            os.path.join(base_computeaorta_dir, "scripts"),
            "default_target_cookie.json")

    # create mux/compiler paths and create subtarget
    modules_dir_in_base = os.path.join(base_computeaorta_dir, 'modules')
    compiler_dir_in_base = os.path.join(modules_dir_in_base, 'compiler')
    mux_dir_in_base = os.path.join(modules_dir_in_base, 'mux')

    compiler_dir_out_base = os.path.join(external_dir, 'compiler')
    mux_dir_out_base = os.path.join(external_dir, 'mux')
    external_dir_in_base = os.path.join(base_computeaorta_dir, 'external')


    context = create_context(base_config, args.config, base_computeaorta_dir, args.override)
    if context:
        context['cookiecutter']['external_dir'] = external_dir
        update_string_values(context, ["llvm_cpu", "llvm_features", "llvm_triple"])
        result = create_subtarget(
            compiler_dir_in_base,
            compiler_dir_out_base,
            context,
            overwrite_if_exists=args.overwrite_if_file_exists)

        if result:
            result = create_subtarget(
                mux_dir_in_base,
                mux_dir_out_base,
                context,
                overwrite_if_exists=args.overwrite_if_file_exists)
        if result:
            result = create_subtarget(
                external_dir_in_base,
                external_dir,
                context,
                overwrite_if_exists=True)                
        if not result:
            os._exit(1)

    else:
        print("Unable to create context for target creation", file=stderr)
        os._exit(1)


if __name__ == '__main__':
    main()
