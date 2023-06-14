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

import re
import os
from subprocess import check_output
from platform import system, machine

from lit.llvm import llvm_config
from lit.llvm.subst import ToolSubst, FindTool

def last_substitution_by_key(li, x):
    for i in reversed(range(len(li))):
      if li[i][0] == x:
          return i
    return -1

def is_native_exe(path):
    """Determines if an executable is native."""
    # We currently do not support cross compilation and emulated testing on
    # windows meaning on windows builds everything is native.
    if 'Windows' == system():
        return True
    output = str(check_output(['file', path]))
    # If the BuildID hash happens to end up having '386' in it then it can
    # cause a false match below, so just blank out any BuildID.
    cleaned = re.sub(r"BuildID\[[\w\s]*\]=[A-Fa-f0-9]*, ","", output)
    if 'x86' in machine() or '386' in machine():
        if 'x86' in cleaned or '386' in cleaned:
            return True
        return False
    return True

def add_ca_tool_substitutions(config, lit_config, llvm_config, tools):
    # Check at runtime if environmental variables have been set with paths to
    # helper tools. If so, set the 'command' flag, which tells lit to treat the
    # tool command as an exact path.
    for tool in tools:
        env_var = os.environ.get(f'LIT_{tool.key.upper().replace("-", "_")}')
        if env_var:
            tool.command = env_var

    llvm_config.add_tool_substitutions(tools, config.tool_search_dirs)

    # Handle emulation of non-native tools by preprending any substitution with the
    # emulator command.
    for tool in config.tools:
        if not tool.was_resolved:
            continue
        idx = last_substitution_by_key(config.substitutions, tool.regex)
        if idx < 0:
            continue
        subst_key = config.substitutions[idx][0]
        subst_path = config.substitutions[idx][1]
        if 'emulator' in lit_config.params and not is_native_exe(subst_path):
            config.substitutions[idx]= (subst_key, f'{lit_config.params["emulator"]} {subst_path}')

def get_unresolved_tools(tools):
    return [tool.key for tool in tools if not tool.was_resolved]
