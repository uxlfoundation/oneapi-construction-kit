#!/usr/bin/env python
# Copyright (C) Codeplay Software Limited. All Rights Reserved.

from argparse import ArgumentParser
from os import environ, mkdir, path
from re import match, findall, search, sub, finditer
from math import ceil
import sys
from subprocess import call, Popen, PIPE, STDOUT
from io import StringIO

TYPE_INT8 = 0
TYPE_INT16 = 1
TYPE_INT32 = 2
TYPE_INT64 = 3
TYPE_HALF = 4
TYPE_FLOAT = 5
TYPE_DOUBLE = 6

DEBUG_OCLC_PRE_DUMP = 1
DEBUG_eval_access_eval = 2
debug = DEBUG_eval_access_eval | DEBUG_OCLC_PRE_DUMP
irlines = []
type_lookup = {
    "i8": TYPE_INT8,
    "i16": TYPE_INT16,
    "i32": TYPE_INT32,
    "i64": TYPE_INT64,
    "float": TYPE_FLOAT,
    "double": TYPE_DOUBLE,
    "half": TYPE_HALF
}

# map of basic block label of for loop condition -> tuple of info for that loop
for_loop_bounds = dict()

# map of kernel argument name -> argument input value
fixed_params = dict()

# map of %arrayidx assignment line -> basic block label
arrayidx_bblock_map = dict()

# list of %arrayidx registers which are used to calculate other %arrayidx values
meta_arrayidxs = []

target_kernel = ""

max_work_dim_access = 0


class Param:
    """Contains information about a parameter"""

    def __init__(self, name, type, vec_size, is_buf=True):
        self.name = name
        self.type = type
        self.vec_size = vec_size
        self.is_buf = is_buf
        self.preset_val = None
        self.access = ""

    def doprint(self):
        vec_suffix = "" if self.vec_size == 1 else str(self.vec_size)
        print("name = ", self.name, "type = ", \
            str(self.type) + vec_suffix, "access = ", self.access)

    def get_name(self):
        return self.name

    def get_size_base_type(self):
        """
        Returns the size of the type of this parameter
        The size of vectors are ignored
        """

        if self.type == TYPE_INT8:
            return 1
        elif self.type == TYPE_INT16 or self.type == TYPE_HALF:
            return 2
        elif self.type == TYPE_INT32 or self.type == TYPE_FLOAT:
            return 4
        elif self.type == TYPE_INT64 or self.type == TYPE_DOUBLE:
            return 8


class BufferAccessEval:
    """Contains information about access to a buffer as a string"""

    def __init__(self, name, eval_string, access, bblock):
        self.name = name
        self.eval_string = eval_string
        self.access = access
        self.bblock = bblock

    def doprint(self):
        print("name = ", self.name, "eval = ", self.eval_string, "access = ",
              self.access)


def replaceArgsInScev(scev_dict, arg, params, arg_char):
    """Retrieve the related SCEV expression from an array index register"""
    out = arg
    find_list = findall(r"%([\w.]+)", out)
    while len(find_list) > 0:
        # Remove duplicates and sort on length
        find_list = list(set(find_list))
        find_list = sorted(find_list, key=len, reverse=True)
        CSD_count = 0
        for lookup in find_list:
            if lookup in scev_dict:
                repl = scev_dict[lookup]
                out = out.replace(arg_char + lookup, repl)
            else:
                out = out.replace(arg_char + lookup, "$" + lookup)
        find_list = findall(r"%([\w.]+)", out)
    return out


def strip_address_from_load_store(line):
    """Retrieve the address of in an array access IR line"""
    m = match(r".+(%[^,]*)", line.strip())
    if m:
        return m.group(1)
    else:
        return None


def eval_access_eval(buffers, hanging_vars, scev_dict, params, access):
    """
    Find kernel parameters in a SCEV expression, and replace it with 0
    0 being the base array offset
    """
    sys.stdout.flush()
    eval_string = replaceArgsInScev(scev_dict, access, params, '%')
    sys.stdout.flush()
    # we should hopefully have a single dollar one left - which is the param
    dollar_args = findall(r"\$([\w.]+)", eval_string)
    # Look for buffer arg
    buf_name = None
    for arg in dollar_args:
        for p in params:
            if p.is_buf and arg == p.get_name():
                # Check if buffer type
                buf_name = arg
    if buf_name:
        eval_string = sub(r"(.*)(\$" + buf_name + r")([^\w.])", r"\g<1>0\3",
                          eval_string)
        buffers.append(BufferAccessEval(
            buf_name, eval_string, access, arrayidx_bblock_map.get(access, "0")))
        # Check for any hanging variables
        dollar_args = findall(r"\$([\w.]+)", eval_string)
        hanging_vars += dollar_args
    return True


def process_ir_param(p):
    """Retrieve details of a kernel parameter from the IR"""

    # example: float addrspace(1)* %mean, float addrspace(1)* %std,
    # float addrspace(1)* %data, float %float_n, i32 %m, i32 %n

    # first check for addrspace(1)* (__global *)
    # split string on space
    s = p.strip().split(" ")
    if s[-1][0] != '%':
        # this means the param is not a named kernel argument, so ignore it
        return None
    is_vec = s[0][0] == '<' and s[0][1].isdigit()
    is_buf = (((is_vec and len(s) == 5) or (not is_vec and len(s) == 3)) and
              "addrspace" in p)
    vec_size = int(s[0][1:]) if is_vec else 1
    name = s[-1][1:]
    type_str = s[2][:-1] if is_vec else s[0]
    return Param(name, type_lookup[type_str], vec_size, is_buf)


def getForLoops():
    """Collects information about each for loop in the kernel"""
    for index, line in enumerate(irlines):
        if "<label>:" in line:
            label = line.split(':')[1]
            if "preds = " in line:
                preds = line.split('%')[1:]
                for pred in preds:
                    pred.strip(" ,")
                if len(preds) == 2:
                    phiIdx = index + 1
                    while "%storemerge" not in irlines[phiIdx] \
                        and "<label>" not in irlines[phiIdx] \
                        and phiIdx + 1 < len(irlines):
                        phiIdx += 1
                    phiLine = irlines[phiIdx].split()
                    if len(phiLine) > 2 and phiLine[2] == "phi":
                        register = phiLine[0]
                        counterType = phiLine[3]
                        phiVals = [(phiLine[5].strip(','),
                                    phiLine[6].strip('%')),
                                   (phiLine[9].strip(','),
                                    phiLine[10].strip('%'))]
                        # assume startvals is the value set from the
                        # lower of the two predecessors to the phi node
                        startVal = phiVals[0][0] if (
                            int(phiVals[0][1]) <
                            int(phiVals[1][1])) else phiVals[1][0]

                        counter = phiVals[0][0] if (
                            int(phiVals[0][1]) >=
                            int(phiVals[1][1])) else phiVals[1][0]

                        # if it's a for loop, we will find an icmp and
                        # a counter increment, in no particular order
                        forIdx = phiIdx + 1
                        while "icmp" not in irlines[forIdx] \
                            and "<label>" not in irlines[forIdx] \
                            and counter + " =" not in irlines[forIdx] \
                            and phiIdx + 1 < len(irlines):
                            forIdx += 1

                        cmpFound = False
                        ctrFound = False
                        if "icmp" in irlines[forIdx]:
                            cmpVal = irlines[forIdx].split()[-1]
                            brLine = irlines[forIdx+1].split()
                            # the labels of the for loop, and the block after
                            if  brLine[0] == "br" and brLine[1] == "i1":
                                brLabels = (brLine[4].strip("%,"),
                                            brLine[6].strip("%,"))
                            cmpFound = True
                        elif counter + " =" in irlines[forIdx]:
                            stride = irlines[forIdx].split()[-1]
                            ctrFound = True
                        else:
                            continue

                        forIdx += 1
                        while (
                            ctrFound or counter + " =" not in irlines[forIdx]) \
                            and (cmpFound or "icmp" not in irlines[forIdx]) \
                            and forIdx + 1 < len(irlines):
                            forIdx += 1
                        if "icmp" in irlines[forIdx] \
                            or counter + " =" in irlines[forIdx]:
                            if "icmp" in irlines[forIdx]:
                                cmpVal = irlines[forIdx].split()[-1]
                                brLine = irlines[forIdx+1].split()
                                if  brLine[0] == "br" and brLine[1] == "i1":
                                    brLabels = (brLine[4].strip("%,"),
                                                brLine[6].strip("%,"))
                            else:
                                stride = irlines[forIdx].split()[-1]
                            for_loop_bounds[label] = (startVal, cmpVal,
                                                      counterType, stride,
                                                      register, brLabels)


def getMaxSize(func, dimension):
    """
    Returns the maximum value of a local/global/group id/size function
    Default values of global & local size = 4 across all dimensions
    """
    global_size = 4
    local_size = 4
    num_groups = global_size / local_size

    call_dict = {
        "_Z15get_global_sizej": [global_size,   global_size,   global_size],
        "_Z14get_local_sizej":  [local_size,    local_size,    local_size],
        "_Z13get_global_idj":   [global_size-1, global_size-1, global_size-1],
        "_Z12get_local_idj":    [local_size-1,  local_size-1,  local_size-1],
        "_Z12get_group_idj":    [num_groups-1,  num_groups-1,  num_groups-1],
        "_Z14get_num_groupsj":  [num_groups,    num_groups,    num_groups]
    }
    if dimension < 0 or dimension > 2 or func not in call_dict:
        print("error: bad arguments for getMaxSize() (" \
            + str(index) + ", " + func + ")")
        return -1
    return call_dict[func][dimension]


def replaceSizes():
    """Replaces local/global/group ids/sizes with their maximum values"""
    for index in range(len(irlines)):
        m = match("(.*)call spir_func *i\d* @([\w]+)\(*i\d* (\d)",
                  irlines[index].strip())
        if m:
            global max_work_dim_access
            max_work_dim_access = max(max_work_dim_access, int(m.group(3)))
            irlines[index] = m.group(1) + "add i32 0, " + str(
                getMaxSize(m.group(2), int(m.group(3))))


def process_kernel(file, kernel_name, oclc, params_str_list):
    """Retrieves information about load and store instructions from a kernel"""
    scev_dict = {}
    params = []
    loads = []
    stores = []
    hanging_vars = []
    global_ids_used = 0

    environ['CA_PASS_PRE_DUMP'] = "host_dma"
    cmd = oclc + " -enqueue " + kernel_name + " " + file
    p = Popen(
        cmd,
        shell=True,
        stdin=PIPE,
        stdout=PIPE,
        stderr=PIPE,
        universal_newlines=True)
    stdout, stderr = p.communicate()
    ir = stdout + stderr
    global irlines
    irlines = ir.split('\n')
    print(ir)
    bblock = "0"
    global arrayidx_bblock_map
    for line in irlines:
        if "<label>:" in line:
            bblock = line.split(':')[1]
        if line.strip().startswith("%arrayidx"):
            arrayidx_bblock_map[line.split()[0]] = bblock
        if "define spir_kernel" in line:
            m = match("define spir_kernel.*@(\w+)\(([^\#]*)", line)
            name = m.group(1)
            ir_params_str_list = m.group(2)[:-2].split(",")
            for p in ir_params_str_list:
                param = process_ir_param(p)
                if param:
                    params.append(param)

    environ['CA_PASS_SCEV_PRE_DUMP'] = "host_dma"
    cmd = oclc + " -enqueue " + kernel_name + " " + file
    p = Popen(
        cmd,
        shell=True,
        stdin=PIPE,
        stdout=PIPE,
        stderr=PIPE,
        universal_newlines=True)
    stdout, stderr = p.communicate()
    buf = stdout + stderr
    insideScevFunc = False
    expecting_scev_line = False
    for line in buf.split("\n"):
        if "SCEV Func" in line:
            m = match("SCEV Func :: (\w+)", line)
            if m and m.group(1) == kernel_name:
                insideScevFunc = True
        else:
            if insideScevFunc:
                if not expecting_scev_line or "=" in line or "store " in line:
                    if "load " in line.strip():
                        loads.append(
                            strip_address_from_load_store(line.strip()))
                    elif "store " in line.strip():
                        stores.append(
                            strip_address_from_load_store(line.strip()))
                    else:
                        m = match(r"\s*%([\w.]+)\s*=\s*(.*)", line)
                        if m:
                            expecting_scev_line = True
                            last_name = m.group(1)
                            last_instruction = m.group(2)
                elif expecting_scev_line:
                    expecting_scev_line = False
                    if "-> " in line:
                        call_dict = {
                            "_Z13get_global_idj": "@global_id_",
                            "_Z15get_global_sizej": "@global_size_",
                            "_Z12get_local_idj": "@local_id_",
                            "_Z14get_local_sizej": "@local_size_",
                            "_Z12get_group_idj": "@group_id_",
                            "_Z14get_num_groupsj": "@group_size_"
                        }
                        m = match("call spir_func *i\d* @([\w]+)\(*i\d* (\d)",
                                  last_instruction)
                        if m:
                            call_lookup = call_dict[m.group(1)]
                            scev_dict[last_name] = call_lookup + m.group(2)
                            global max_work_dim_access
                            max_work_dim_access = max(max_work_dim_access,
                                                      int(m.group(2)))
                        else:
                            # get scev output and remove <nsw>
                            m = match("\s*->\s*(.*)", line.strip())
                            if m:
                                scev = m.group(1).replace("<nsw>", "")
                                if last_name == scev[1:]:
                                    print(
                                        "WARNING: Unable to work out scev line",
                                        last_instruction, " :: ", scev)
                                else:
                                    scev_dict[last_name] = scev
    # Look for any variables left that need to be set

    load_buffers = []
    store_buffers = []
    for l in loads:
        if l:
            sys.stdout.flush()
            if not eval_access_eval(load_buffers, hanging_vars, scev_dict,
                                    params, l):
                return None

    for s in stores:
        if s:
            sys.stdout.flush()
            if not eval_access_eval(store_buffers, hanging_vars, scev_dict,
                                    params, s):
                return None

    hanging_vars = list(set(hanging_vars))
    return load_buffers, store_buffers, params, hanging_vars


def replaceInBufferList(expression, from_val, to_val, var_char):
    """Replaces an IR expression"""
    expression.eval_string = expression.eval_string.replace(
        var_char + from_val, to_val)


def replaceRegex(expression, from_val, to_val):
    """Replaces an IR expression using regular expressions"""
    expression.eval_string = sub(from_val, to_val, expression.eval_string)


def resolveSizeChange(lb):
    """Removes extension / truncation information from an SCEV expression"""
    replaceRegex(lb, r"trunc ", r" ")
    replaceRegex(lb, r"sext ", r" ")
    replaceRegex(lb, r"zext ", r" ")
    replaceRegex(lb, r" to ", r"  ")
    replaceRegex(lb, r" i4", r" ")
    replaceRegex(lb, r" i8", r" ")
    replaceRegex(lb, r" i16", r" ")
    replaceRegex(lb, r" i32", r" ")
    replaceRegex(lb, r" i64", r" ")
    replaceRegex(lb, r" i1", r" ")
    return lb


def getInputVal(input, size):
    """Assigns a fixed value to a kernel argument (currently 10 x `size`)"""
    fixed_params[input[1:]] = "\"repeat(" + str(size) + ",10)\""
    return "10"


def loadToExpr(ins, vecIdx):
    """Evaluates an llvm `load` instruction"""
    """If that value is an input buffer access, we return the value that that"""
    """access would return (fixed to 0 by default)"""
    """If it is a bitcasted vector, we find the original vector and access the"""
    """required element"""
    ins = ins.strip()
    if "%arrayidx" in ins[1:]:
        # beg=1 because we don't want to find the LHS, if that's an '%addridxN'
        arrayidxidx = ins.find("%arrayidx", 1)
        arrayidxend = ins.find(",", arrayidxidx)
        arrayidx = ins[arrayidxidx:arrayidxend]
        meta_arrayidxs.append(arrayidx)
        return "0"
    elif vecIdx != "-1":
        validStoreArea = False
        for line in reversed(irlines):
            if validStoreArea:
                components = line.split()
                # look for a store from a register with a vector type
                if components[0] == "store" and "<" in components[1] \
                    and "%" in components[4]:
                    dstRegAsgn = getAssignment(components[8].strip(",")).split()
                    # look for a bitcast instruction on the `load`ed register
                    if dstRegAsgn[2] == "bitcast" \
                        and dstRegAsgn[6] == ins.split()[9].strip(","):
                        return toExpr(components[4].strip(","), vecIdx)
            elif ins == line.strip():
                validStoreArea = True
    raise Exception("unsupported load operation")


def toExpr(reg, vecIdx="-1"):
    """Recursively converts a line of IR to a python expression"""
    if reg[0] != '%':
        return reg
    asgn = getAssignment(reg)
    values = asgn.replace(',', '').split()
    if len(values) < 3:
        return asgn
    if (values[2] == "add"):
        return binOpToExpr(values, '+', vecIdx)
    elif (values[2] == "sub"):
        return binOpToExpr(values, '-', vecIdx)
    elif (values[2] == "mul"):
        return binOpToExpr(values, '*', vecIdx)
    elif (values[2] == "fdiv"):
        return binOpToExpr(values, '/', vecIdx)
    elif (values[2] == "sdiv"):
        return binOpToExpr(values, '/', vecIdx)
    elif (values[2] == "udiv"):
        return binOpToExpr(values, '/', vecIdx)
    elif (values[2] == "frem"):
        return binOpToExpr(values, '%', vecIdx)
    elif (values[2] == "srem"):
        return binOpToExpr(values, '%', vecIdx)
    elif (values[2] == "urem"):
        return binOpToExpr(values, '%', vecIdx)
    elif (values[2] == "or"):
        return binOpToExpr(values, '|', vecIdx)
    elif (values[2] == "and"):
        return binOpToExpr(values, '&', vecIdx)
    elif (values[2] == "xor"):
        return binOpToExpr(values, '^', vecIdx)
    elif (values[2] == "shl"):
        return binOpToExpr(values, '<<', vecIdx)
    elif (values[2] == "ashr"):
        return binOpToExpr(values, '>>', vecIdx)
    elif (values[2] == "lshr"):
        # add 2^32 to LHS of shift if it's negative, as
        # python doesn't support logical shift right
        return toExpr(values[-2].strip(",")) + " >> (" + toExpr(values[-1]) \
            + ") if (" + toExpr(values[-2].strip(",")) + ") >= 0 else ((" \
            + toExpr(values[-2].strip(",")) + ") + 0x100000000) >> (" \
            + toExpr(values[-1]) + ")"
    elif (values[2] == "phi" and "%storemerge" not in values[0]):
        return "max(" + toExpr(values[5].strip(',')) + "," \
            + toExpr(values[9].strip(',')) + ")"
    elif (values[2] == "icmp"):
        return cmpToExpr(values)
    elif (values[2] == "trunc"):
        return toExpr(values[4])
    elif (values[2] == "sext"):
        return toExpr(values[4])
    elif (values[2] == "zext"):
        return toExpr(values[4])
    elif (values[2] == "fptosi"):
        return "int(" + toExpr(values[4]) + ")"
    elif (values[2] == "sitofp"):
        return "float(" + toExpr(values[4]) + ")"
    elif (values[2] == "uitofp"):
        return "float(" + toExpr(values[4]) + ")"
    elif (values[2] == "select"):
        return "(" + toExpr(values[-3].strip(',')) + " if " + toExpr(
            values[-5].strip(',')) + " == 1 else " + toExpr(values[-1]) + ")"
    elif (values[2] == "load"):
        return loadToExpr(asgn, vecIdx)
    elif (values[2] == "insertelement"):
        if values[10] == vecIdx:
            return toExpr(values[8].strip(','))
        else:
            return toExpr(values[6].strip(','), vecIdx)
    elif (values[2] == "extractelement"):
        return toExpr(values[6].strip(','), values[8])

    sizeMatch = match(r".*call spir_func *i\d* @([\w]+)\(*i\d* (\d).*", asgn)
    if sizeMatch:
        global max_work_dim_access
        max_work_dim_access = max(max_work_dim_access, int(sizeMatch.group(2)))
        return str(getMaxSize(sizeMatch.group(1), int(sizeMatch.group(2))))

    clampMatch = match(r".*call spir_func *i\d* @_Z5clampjjj\((.+)\).*", asgn)
    if clampMatch:
        clampArgs = clampMatch.group(1).replace(',', '').split()
        return "(min(max(" + toExpr(clampArgs[1]) + ", " + toExpr(
            clampArgs[3]) + "), " + toExpr(clampArgs[5]) + "))"

    clampMatchVec = match(r".*call spir_func <[^>]+> @_Z5clampDv" \
                            + r"[^>]+> ((?!, <).+), <[^>]+> ((?!, <).+), <" \
                            + r"[^>]+> ([^)]+).*", asgn)
    if clampMatchVec:
        vals = ["", "", ""]
        args = clampMatchVec.groups()
        for i in range(3):
            if args[i][0] == '%':
                vals[i] = toExpr(args[i], vecIdx)
            elif args[i] == "zeroinitializer":
                vals[i] = "0"
            elif args[i][0] == "<":
                vals[i] = args[i].split()[2*vecIdx+1].strip(",>")
        return "(min(max(" + vals[0] + ", " + vals[1] + "), " + vals[2] + "))"

    ceilMatch = match(r".* call spir_func \w+ @_Z4ceil.\(([^)]+)\).*", asgn)
    if ceilMatch:
        ceilArg = ceilMatch.group(1).split()[1]
        return "int((" + toExpr(ceilArg) + ") + 1)"

    if "%storemerge" in reg:
        for key, val in for_loop_bounds.items():
            if val[4] == reg:
                return "(" + toExpr(val[1]) + " - " + toExpr(val[3]) + ")"

    return reg


def getAssignment(reg):
    """Returns the line where a register is first assigned"""
    """If the register is a kernel argument, it's assigned value is returned"""
    funcname = ""
    for line in irlines:
        if line.strip().startswith("define spir_"):
            funcname = line[line.find('@') + 1:line.find('(')]
        if line.strip().startswith("define spir_kernel") \
            and reg in line \
            and "* " + reg + "," not in line \
            and funcname == target_kernel:
            if "> " + reg + ","in line:
                kernelDef = line.split()
                regIdx = kernelDef.index(reg + ",")
                size = kernelDef[regIdx-3].strip("<")
            else:
                size = "1"
            return getInputVal(reg, size)
        if line.strip().startswith(reg) and funcname == target_kernel:
            return line
        if line.strip().startswith("define spir_kernel") \
            and "* " + reg + "," in line and target_kernel == funcname:
            return "0"
    return reg


def cmpToExpr(ins):
    """Converts an IR icmp operation to a python expression"""
    operator_map = {
        "eq": " == ",
        "ne": " != ",
        "ugt": " > ",
        "uge": " >= ",
        "ult": " <",
        "ule": " <= ",
        "sgt": " > ",
        "sge": " >= ",
        "slt": " < ",
        "sle": " <= "
    }
    return "(1 if (" + toExpr(ins[-2].strip(',')) \
        + operator_map[ins[3]] + toExpr(ins[-1].strip(',')) + ") else 0)"


def binOpToExpr(ins, op, vecIdx):
    """Converts an IR arithmetic / logic binary operation to a python expression"""
    operands = ins[3:]
    if operands[0] == "nuw":
        operands = operands[1:]
    if operands[0] == "nsw":
        operands = operands[1:]

    if vecIdx == "-1":
        # scalar operation
        lhs = toExpr(operands[1])
        rhs = toExpr(operands[2])
    elif "<" not in operands[3] and "<" not in operands[4]:
        # vector operation - register `op` register
        lhs = toExpr(operands[3].strip(","), vecIdx)
        rhs = toExpr(operands[4].strip(","), vecIdx)
    elif "<" not in operands[3] and "<" in operands[4]:
        # vector operation - register `op` literal
        lhs = toExpr(operands[3].strip(","), vecIdx)
        rhs = operands[5+(2*int(vecIdx))].strip(",>")
    elif "<" in operands[3] and ">" not in operands[-1]:
        # vector operation - literal `op` register
        lhs = ""
        rhs = toExpr(operands[-1].strip(","), vecIdx)
    else:
        print("error: unsupported binary operation (" + " ".join(ins) + ")")
        return None

    return "(" + lhs + " " + op + " " + rhs + ")"


def convertRegister(reg):
    """Swaps the leading $/% in `reg` for the other one"""
    if reg[0] == '%':
        return '$' + reg[1:]
    if reg[0] == '$':
        return '%' + reg[1:]
    return reg


def resolveRegisters(lb):
    """Replaces a reg in a SCEV expression with the max value it can hold"""
    m = sub(r"(\$(\w|\.)+)", lambda m: toExpr(convertRegister(m.group(0))),
            lb.eval_string)
    lb.eval_string = m
    return lb


def resolveForLoop(lb):
    """Converts a SCEV expression for a for loop to a python expression"""
    while True:
        labelIndex = lb.eval_string.find("<$") + 2
        if labelIndex == 1:
            return lb
        labelCloseIndex = lb.eval_string[labelIndex:].find(">")
        label = lb.eval_string.strip()[labelIndex:labelIndex + labelCloseIndex]
        if not label.isdigit():
            return lb
        start, end, forType, \
        stride, storemerge, brLabels = for_loop_bounds[label]
        startVal = eval(toExpr(start))
        endVal = eval(toExpr(end))
        highestVal = max(startVal, endVal)

        if brLabels[0].isdigit() and brLabels[1].isdigit() \
            and int(brLabels[0]) > int(brLabels[1]):
            loopLimit = highestVal
        else:
            loopLimit = highestVal - eval(toExpr(stride))

        maxIdx = "0"
        if brLabels[0].isdigit() and brLabels[1].isdigit() \
            and min(int(brLabels[0]), int(brLabels[1])) == int(lb.bblock):
            # this arrayidx is inside the loop body
            maxIdx = str(loopLimit - startVal)
        else:
            maxIdx = toExpr(str(stride)) + str(loopLimit - startVal)

        replaceRegex(lb, "{([^{}]+),\+,([^{},]+)}<\$([^{},]+)>", \
            r"max((\1) + "+ r"int(ceil(float(" + maxIdx + r") / (" \
            + toExpr(str(stride)) + r"))) * (\2), (\1))")


def evalMaxBufferVals(param, buffer_access_list):
    """Verify that estimated buffer sizes are at least 0"""
    max_val = 0
    for b in buffer_access_list:
        if b.name == param.name:
            val = eval(b.eval_string)
            max_val = max(val, max_val)
    return max_val


class ParamValue:
    """Contains information about a (possibly random) parameter value"""

    def __init__(self, rand_start, rand_end, value=None):
        self.rand_start = rand_start
        self.rand_end = rand_end
        self.value = value


class ParamSetting:
    """Contains information about a parameter setting"""

    def __init__(self, param, num_els, is_input, is_output, value=None):
        self.param = param
        self.num_elements = num_els
        self.is_input = is_input
        self.is_output = is_output
        self.value = value


def createParamSettings(loads, stores, params, debug):
    """Decides the sizes and values of each kernel parameter"""
    param_settings = []
    getForLoops()

    for lb in stores + loads:
        if debug: print "\n==============\n"
        if debug: print lb.name
        if debug: print lb.eval_string

        for dim in range(3):
            replaceInBufferList(lb, "global_id_" + str(dim),
                                str(getMaxSize("_Z13get_global_idj", dim)), "@")
            replaceInBufferList(lb, "global_size_" + str(dim),
                                str(getMaxSize("_Z15get_global_sizej", dim)), "@")
            replaceInBufferList(lb, "local_id_" + str(dim),
                                str(getMaxSize("_Z12get_local_idj", dim)), "@")
            replaceInBufferList(lb, "local_size_" + str(dim),
                                str(getMaxSize("_Z14get_local_sizej", dim)), "@")
            replaceInBufferList(lb, "group_id_" + str(dim),
                                str(getMaxSize("_Z12get_group_idj", dim)), "@")
            replaceInBufferList(lb, "group_size_" + str(dim),
                                str(getMaxSize("_Z14get_num_groupsj", dim)), "@")

        # SCEV "no [unsigned] wrap" tags can be discarded
        replaceRegex(lb, r"<nw>", r"")
        replaceRegex(lb, r"<nuw>", r"")
        # SCEV representation of a udiv instruction
        replaceRegex(lb, r" /u ", r" / ")


        lb = resolveSizeChange(lb)
        if debug: print lb.eval_string
        lb = resolveForLoop(lb)
        if debug: print lb.eval_string
        lb = resolveRegisters(lb)
        element_size = "4"
        for p in params:
            if p.name == lb.name:
                element_size = str(p.vec_size * p.get_size_base_type())
        lb.eval_string += " + " + element_size
        if debug: print lb.eval_string



    for p in params:
        for lb in loads + stores:
            if p.name == lb.name:
                p.access = lb.access
        if p.is_buf:
            # check loads and stores and eval the ones related
            size_load = evalMaxBufferVals(p, loads) / p.get_size_base_type()
            size_store = evalMaxBufferVals(p, stores) / p.get_size_base_type()
            is_load = size_load > 0
            is_store = size_store > 0
            if is_load:
                if p.type == TYPE_FLOAT or p.type == TYPE_DOUBLE:
                    param_settings.append(
                        ParamSetting(p,
                                     max(size_load, size_store), True, is_store,
                                     ParamValue(0, 1.0)))
                    print("AddInputBuffer(", p.name, ", ", max(
                        size_load, size_store), ", rand(0,1.0));")
                elif p.access in meta_arrayidxs:
                    param_settings.append(
                        ParamSetting(p,
                                     max(size_load, size_store), True, is_store,
                                     ParamValue(0, 0, 0)))
                    print("AddInputBuffer(", p.name, ", ", max(
                        size_load, size_store), ", 0);")
                else:
                    param_settings.append(
                        ParamSetting(p,
                                     max(size_load, size_store), True, is_store,
                                     ParamValue(0, 255)))
                    print("AddInputBuffer(", p.name, ", ", max(
                        size_load, size_store), ", randint(0,255));")
            elif is_store:
                if p.type == TYPE_FLOAT or p.type == TYPE_DOUBLE:
                    param_settings.append(
                        ParamSetting(p, size_store, False, True))
                    print("AddOutputBuffer(", p.name, ", ", size_store,
                          ", rand(0,1.0));")
                else:
                    param_settings.append(
                        ParamSetting(p, size_store, False, True))
                    print("AddOutputBuffer(", p.name, ", ", size_store,
                          ", randint(0,255));")
        else:
            # check if preset otherwise float (0:1.0), int (0:10)
            if p.preset_val:
                param_settings.append(
                    ParamSetting(p, 1, True, False,
                                 ParamValue(0, 0, p.preset_val)))
                print("AddPrimitive(", p.name, ", ", p.preset_val, ");")
            elif p.name in fixed_params:
                param_settings.append(
                    ParamSetting(p, 1, True, False,
                                 ParamValue(0, 0, fixed_params[p.name])))
                print("AddPrimitive(", p.name, ", ", fixed_params[p.name], ");")
            else:
                if p.type == TYPE_FLOAT or p.type == TYPE_DOUBLE:
                    param_settings.append(
                        ParamSetting(p, 1, True, False, ParamValue(0, 1.0)))
                    print("AddPrimitive(", p.name, ", rand(0,1.0));")
                else:
                    param_settings.append(
                        ParamSetting(p, 1, True, False, ParamValue(0, 10)))
                    print("AddPrimitive(", p.name, ", randint(0,255));")
    return param_settings


class KernelAnalyze:
    """Retrieves information about a kernel using a given oclc binary"""

    def __init__(self, oclc):
        self.oclc = oclc
        self.kernel_name = ""

    def analyze(self, filename, kernel, debug):
        """Finds the parameters to a kernel, and """
        self.kernel_name = kernel

        param_replace = {}
        f = open(filename, 'r').readlines()
        for index, line in enumerate(f):
            if "kernel" in line:
                kernel_line = line.strip()
                line_offset = 1
                while "{" not in kernel_line:
                    kernel_line += " " + f[index + line_offset].strip()
                    line_offset += 1
                kernel_line = kernel_line[:kernel_line.find('{')].strip()
                name_params = match(
                    r"(__)?kernel\s+(__attribute__\(\(.*)?" \
                    + r"void\s+(\w+)\s*\(([^)]*.*)",
                    kernel_line)
                if name_params:
                    hanging_vars = []
                    self.kernel_name = name_params.group(3)
                    params_as_string = name_params.group(4)
                    if kernel is None or self.kernel_name == kernel:
                        global target_kernel
                        target_kernel = self.kernel_name
                        params = findall("([^,^\)]+)+", params_as_string)
                        loads, stores, params, hanging_vars = process_kernel(
                            filename, self.kernel_name, self.oclc, params)
                        if len(hanging_vars):
                            # check if we can replace then with args passed in
                            for h in hanging_vars:
                                if h in param_replace:
                                    replaceInBufferList(loads, h,
                                                        param_replace[h], "$")
                                    replaceInBufferList(stores, h,
                                                        param_replace[h], "$")
                                    for p in params:
                                        if p.name == h:
                                            p.preset_val = param_replace[h]
                        print("LOAD:")
                        map(lambda x: x.doprint(), loads)
                        print("STORE:")
                        map(lambda x: x.doprint(), stores)
                        param_settings = createParamSettings(
                            loads, stores, params, debug)
                        return param_settings

    def create_output(self, param_settings, filename):
        """Converts kernel parameter sizes and values to oclc arguments"""
        global_size = str(getMaxSize("_Z15get_global_sizej", 0))
        local_size = str(getMaxSize("_Z14get_local_sizej", 0))
        for i in range(0, max_work_dim_access):
            global_size += "," + str(getMaxSize("_Z15get_global_sizej", i))
            local_size += "," + str(getMaxSize("_Z14get_local_sizej", i))
        out = " -execute -ulp-error 128 -char-error 1 -global " + global_size \
            + " -local " + local_size + " -enqueue " + self.kernel_name + " "
        for p in param_settings:
            if p.is_input:
                if p.param.is_buf:
                    if p.value.value:
                        out += "-arg \"" + p.param.name + ",repeat(" + str(
                            int(p.num_elements)) + "," + str(
                                p.value.value) + ")\" "
                    elif p.param.type == TYPE_FLOAT \
                        or p.param.type == TYPE_DOUBLE:
                        out += "-arg \"" + p.param.name + ",repeat(" + str(
                            int(p.num_elements)) + ",rand(0,1.0))\" "
                    else:
                        out += "-arg \"" + p.param.name + ",repeat(" + str(
                            int(p.num_elements)) + ",randint(0,255))\" "
                elif p.value.value:
                    out += "-arg " + p.param.name + "," + str(p.value.value)
                elif p.param.type == TYPE_FLOAT or p.param.type == TYPE_DOUBLE:
                    out += "-arg \"" + p.param.name + "," + "repeat(" + str(
                        int(p.param.vec_size)) + ",rand(0,1.0))\" "
                else:
                    out += "-arg \"" + p.param.name + "," + "repeat(" + str(
                        int(p.param.vec_size)) + ",randint(0,255))\" "
            if p.is_output:
                out += "-print \"" + p.param.name + "," + str(
                    int(p.num_elements)) + "\" "
                if not p.is_input:
                    out += "-arg \"" + p.param.name + ",repeat(" + str(
                        int(p.num_elements)) + ",73)\" "
            out += " "
        out += filename
        return out


def main():
    """Main entry point to the script."""
    parser = ArgumentParser(description='Generate OCLC arguments for a kernel.')
    parser.add_argument(
        'file', help='Name of the file where the kernel is found.')
    parser.add_argument(
        '--binary_dir', help='Path to OCLC binary.')
    parser.add_argument(
        '--kernel', help='Kernel name.')
    parser.add_argument(
        '--debug',
        help= 'Print buffer access values at various stages of evaluation.')
    parser.add_argument(
        '--alt_filename',
        help=
        'File kernel is found in when lit runs, not neccessarily same as `file`.'
    )
    args = parser.parse_args()
    kernel = args.kernel
    if args.binary_dir:
        args.binary_dir = path.abspath(args.binary_dir)
        oclc = path.join(args.binary_dir, "oclc")
    else:
        print("Need --binary_dir to be set to find oclc")

    analyzer = KernelAnalyze(oclc)
    param_settings = analyzer.analyze(args.file, args.kernel, args.debug)
    if args.alt_filename:
        print(" %p/" + args.alt_filename)
    print(analyzer.create_output(param_settings, args.file))


if __name__ == '__main__':
    main()
