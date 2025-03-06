#!/usr/bin/env python
"""Generate a header based on an XML schema."""

# Copyright (c) 2015 Kenneth Benzie
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

from __future__ import print_function
import argparse
import xml.etree.ElementTree as XML
import sys

INDENT = '  '
PREFIX = ''
FUNCTION_PREFIX = ''
FUNCTIONS_ONLY = False
INCLUDES = []
STUB_INCLUDES = []
STUB_GUARDS_ON = False
STUB_PREFIX = ''
STUB_QUALIFIER = ''
STUB_VALUE = None
STUB_VOID = None
VARIABLES = []


class Variable:
    name = ''
    values = []

    def __init__(self, name, values):
        self.name = name
        self.values = values


class DoxygenParam:
    __name = None
    __text = None
    __form = None

    def __init__(self, name, node):
        if name is None:
            raise Exception('Parameter name must not be None')
        self.__name = name
        if not node is None:
            param = node.find('param')
            if not param is None:
                self.__text = param.text
                form = param.attrib.get('form')
                if form != '':
                    self.__form = form

    def output(self):
        """Generate Doxygen parameter output."""
        param = ''
        if not self.__text is None:
            param += '@param'
            if self.__form != '':
                param += '[' + self.__form + '] '
            else:
                param += ' '
            param += self.__name + ' ' + self.__text
        return param


class Doxygen:
    brief = None
    detail = None
    params = []
    ret = None
    see = None

    def __init__(self, node):
        self.init(node)

    def init(self, node):
        self.params = []
        if not node is None:
            brief = node.find('brief')
            if not brief is None:
                self.brief = brief.text
            else:
                self.brief = None
            detail = node.find('detail')
            if not detail is None:
                self.detail = detail.text
            else:
                self.detail = None
            ret = node.find('return')
            if not ret is None:
                self.ret = ret.text
            else:
                self.ret = None
            see = node.find('see')
            if not see is None:
                self.see = see.text
            else:
                self.see = None
        else:
            self.brief = None
            self.detail = None
            self.ret = None
            self.see = None

    def empty(self):
        if (self.brief is None and self.detail is None and
                len(self.params) == 0 and self.ret is None and
                self.see is None):
            return True
        return False

    def output(self):
        sections = []
        if not self.brief is None:
            sections.append('/// ' + '@brief ' + replace_prefixes(self.brief) +
                            '\n')
        if not self.detail is None:
            detail = ''
            for line in self.detail.split('\n'):
                detail += '/// ' + line + '\n'
            sections.append(replace_prefixes(detail))
        if len(self.params) != 0:
            param_section = ''
            for param in self.params:
                param_section += '/// ' + param + '\n'
            if param_section != '':
                sections.append(replace_prefixes(param_section))
        if not self.ret is None:
            sections.append('/// @return ' + replace_prefixes(self.ret) + '\n')
        if not self.see is None:
            sections.append('/// @see ' + replace_prefixes(self.see) + '\n')
        text = ''
        if len(sections) != 0:
            text = '///\n'.join(sections)
        return text.strip()


def is_identifier(identifier):
    """Checks if an identifier is a valid C identifier."""
    nondigits = ['_', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
                 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
                 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
                 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U',
                 'V', 'W', 'X', 'Y', 'Z']
    digits = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9']
    if not identifier[:1] in nondigits:
        return False
    characters = nondigits + digits
    for char in identifier[1:]:
        if not char in characters:
            return False
    return True


def replace_prefixes(text):
    text = replace_prefix(text)
    text = replace_function_prefix(text)
    return text


def replace_prefix(identifier):
    def vowel(c):
        return c.upper() in ['A', 'E', 'I', 'O', 'U']

    # Note that this will simply use "an" before words beginning with a vowel,
    # and "a" before words that don't (or neither if neither "an" nor "a" where
    # used originally).  The English language, however, is more complicated
    # than that.  "a" vs "an" is determined by sound, not simple vowels.  There
    # is even a Python package [1] to handle this, but I don't want to
    # introduce another dependency.
    # [1] https://pypi.python.org/pypi/inflect
    def vowel_aware_replace(ident, pattern, PREFIX):
        a = ("a", "an") if vowel(PREFIX[0]) else ("an", "a")
        A = ("A", "An") if vowel(PREFIX[0]) else ("An", "A")
        out = ident.replace("%s %s" % (a[0], pattern),
                            "%s %s" % (a[1], PREFIX))
        out = out.replace("%s %s" % (A[0], pattern), "%s %s" % (A[1], PREFIX))
        out = out.replace(pattern, PREFIX)
        return out

    output = vowel_aware_replace(identifier, "${prefix}", PREFIX)
    output = vowel_aware_replace(output, "${Prefix}", PREFIX.capitalize())
    output = vowel_aware_replace(output, "${PREFIX}", PREFIX.upper())
    return output


def replace_function_prefix(identifier):
    output = identifier.replace("${function_prefix}", FUNCTION_PREFIX)
    output = output.replace("${Function_Prefix}", FUNCTION_PREFIX.capitalize())
    output = output.replace("${FUNCTION_PREFIX}", FUNCTION_PREFIX.upper())
    return output


def replace_stub_prefix(identifier, name=''):
    output = identifier.replace("${stub_prefix}", name)
    output = output.replace("${Stub_Prefix}", name.capitalize())
    output = output.replace("${STUB_PREFIX}", name.upper())
    return output


def replace_variables(text):
    start = text.find('${')
    while start != -1:
        end = text.find('}')
        var_name = text[start + 2:end]
        replaced = False
        for variable in VARIABLES:
            if variable.name.startswith(var_name):
                text = text.replace('${' + var_name + '}', variable.values[0])
                replaced = True
        if not replaced:
            raise Exception('could not replace:', var_name)
        start = text.find('${')
    return text


def replace_stub(text, name, return_type, arguments):
    stub = ''
    text = text.replace("${name}", name.replace("${prefix}", ""))
    capturing = False

    loop_variable = None
    loop_iterator = ''
    loop_lines = []

    for line in text.split('\n'):
        if '${foreach}' in line:
            capturing = True

            # Reset loop state
            loop_variable = None
            loop_iterator = ''
            loop_lines = []

            expr = line[line.find('(') + 1:line.find(')')]
            in_pos = expr.find('in')
            iter_name = expr[:in_pos].strip()
            var_name = expr[in_pos + 2:].strip()
            for variable in VARIABLES:
                if var_name == variable.name:
                    for value in variable.values:
                        loop_variable = variable
                        loop_iterator = iter_name

            if loop_variable is None:
                raise Exception('invalid ${foreach} variable', var_name)
        elif '${endforeach}' in line:
            # Write loop
            for value in loop_variable.values:
                for line in loop_lines:
                    stub += line.replace('${' + loop_iterator + '}',
                                         value) + '\n'
            capturing = False
        elif capturing:
            loop_lines.append(line)
        else:
            stub += line + '\n'

    stub = stub.replace("${forward}", ', '.join(arguments))
    # TODO: Support any numbered argument!
    stub = stub.replace("${0}", arguments[0])
    stub = replace_stub_prefix(stub, STUB_PREFIX)
    stub = replace_prefix(stub)
    stub = stub.replace("${function_prefix}", "")
    stub = stub.replace("${return_type}", replace_prefix(return_type))
    return replace_variables(stub)


def priority_is(priority, node):
    node_priority = node.attrib.get('priority')
    if priority == node_priority:
        return True
    return False


def include(node, newline):
    """Handle include generation."""
    if not FUNCTIONS_ONLY:
        if node.text is None:
            raise Exception('missing include file')
        name = replace_prefix(node.text.strip())
        include = '#' + node.tag + ' '
        form = node.attrib.get('form')
        if form is None or form == 'angle':
            include += '<' + name + '>'
        elif form == 'quote':
            include += '"' + name + '"'
        else:
            raise Exception('invalid include form: ' + form)
        if newline:
            include += '\n'
        print(include)
    else:
        for include_path in INCLUDES:
            INCLUDES.remove(include_path)  # Only include once
            print('#include <' + include_path + '>')


def define(node, newline):
    """Handle define generation."""
    if not FUNCTIONS_ONLY or priority_is('high', node):
        docs = Doxygen(node.find('doxygen'))
        define = '#{0} {1}'.format(node.tag,
                                   replace_prefix(node.text.strip()).upper())
        params = node.findall('param')
        # TODO: Output nice diagnostics for unexpected input, use is_identifier()
        if len(params) > 0:
            param_names = []
            for param in params:
                param_names.append(replace_prefix(param.text.strip()))
            define += '(' + ', '.join(param_names) + ')'
        value = node.find('value')
        if not value is None:
            lines = value.text.split('\n')
            if len(lines) > 1:
                continuation = ' \\\n'
                define += (continuation + INDENT +
                           (continuation + INDENT).join(lines))
            elif len(lines) == 1:
                define += ' ' + lines[0]
        define = replace_prefix(define)
        define = replace_function_prefix(define)
        if newline:
            define += '\n'
        if not docs.empty():
            print(docs.output())
        print(define)


def struct(node, semicolon, newline):
    """Handle struct generation."""
    if not FUNCTIONS_ONLY:
        doxygen = Doxygen(node.find('doxygen'))
        struct = 'struct'
        if node.text:
            name = replace_prefix(node.text.strip())
            if not is_identifier(name):
                raise Exception('invalid struct name: ' + name)
            struct += ' ' + name
        scope = node.find('scope')
        # TODO Output nice diagnostics for unexpected input, use is_identifier()
        if not scope is None:
            members = scope.findall('member')
            if len(members) > 0:
                struct += ' {'
                member_decls = []
                for member in members:
                    if not member is None:
                        doxygen_member = Doxygen(member.find(
                            'doxygen')).output()
                        type = member.find('type')
                        if not type is None:
                            member_decl = ''
                            if doxygen_member != '':
                                doxygen_members = doxygen_member.split('\n')
                                doxygen_member = ''
                                for line in doxygen_members:
                                    doxygen_member += INDENT + line + '\n'
                                doxygen_member = doxygen_member.rstrip()
                                member_decl += doxygen_member + '\n'
                            member_decl += INDENT + replace_prefix(
                                type.text.strip())
                            if member.text:
                                member_decl += ' ' + \
                                        replace_prefix(member.text.strip())
                            member_decls.append(member_decl)
                        member_function = member.find('function')
                        if not member_function is None:
                            function_form = member_function.attrib.get('form')
                            if function_form != 'pointer':
                                raise Exception(
                                    'struct member function is not a function pointer')
                            member_decl = ''
                            if doxygen_member != '':
                                member_decl += INDENT + doxygen_member + '\n'
                            member_decl += INDENT + function(
                                member_function, False, False, False)
                            member_decls.append(member_decl)
                        member_union = member.find('union')
                        if not member_union is None:
                            union_decls = []
                            if doxygen_member != '':
                                union_decls.append(doxygen_member)
                            union_decls.extend(
                                union(member_union, False, False, False).split(
                                    '\n'))
                            union_decl = '\n'.join(
                                [INDENT + decl for decl in union_decls])
                            member_decls.append(union_decl)
                if len(member_decls) > 0:
                    struct += '\n' + ';\n'.join(member_decls) + ';\n'
                struct += '}'
        if semicolon:
            struct += ';'
        if newline:
            struct += '\n\n'
        docs = doxygen.output()
        if docs != '':
            print(docs)
        sys.stdout.write(struct)


def union(node, semicolon, newline, out=True):
    """Handle union generation."""
    if not FUNCTIONS_ONLY:
        union = 'union'
        if node.text:
            name = replace_prefix(node.text.strip())
            if not is_identifier(name):
                raise Exception('invalid union name: ' + name)
            union += ' ' + name
        scope = node.find('scope')
        if not scope is None:
            members = scope.findall('member')
            if len(members) > 0:
                union += ' {'
                member_decls = []
                for member in members:
                    if not member is None:
                        member_name = replace_prefix(member.text.strip())
                        type = member.find('type')
                        if not type is None:
                            member_decl = INDENT + replace_prefix(
                                type.text.strip())
                            if member.text is None:
                                raise Exception('union member has no name')
                            member_decl += ' ' + member_name
                            member_decls.append(member_decl)
                        struct = member.find('struct')
                        if not struct is None:
                            struct_name = replace_prefix(struct.text.strip())
                            if not struct_name is None:
                                member_decl = INDENT + 'struct ' + struct_name + \
                                        ' ' + member_name
                                member_decls.append(member_decl)
                if len(member_decls) > 0:
                    union += '\n' + ';\n'.join(member_decls) + ';\n'
                union += '}'
        if semicolon:
            union += ';'
        if newline:
            union += '\n\n'
        if out:
            sys.stdout.write(union)
        else:
            return union


def enum(node, semicolon, newline):
    """Handle enum generation."""
    if not FUNCTIONS_ONLY:
        enum = 'enum'
        doxygen = Doxygen(node.find('doxygen')).output()
        if doxygen != '':
            enum = doxygen + '\n' + enum
        if node.text:
            name = node.text.strip()
            if name != '':
                enum += ' ' + replace_prefix(name)
        enum += ' {'
        scope = node.find('scope')
        # TODO: Output nice diagnostics for unexpected input, use is_identifier()
        if scope is None:
            raise Exception("missing enum scope tag")
        constants = scope.findall('constant')
        if len(constants) > 0:
            enum += '\n'
            constant_decls = []
            for constant in constants:
                if not constant is None:
                    decl = ''
                    doxygen = Doxygen(constant.find('doxygen')).output()
                    if doxygen != '':
                        for line in doxygen.split('\n'):
                            decl = INDENT + line + '\n'
                    if constant.text is None:
                        raise Exception("invalid enum constant")
                    decl += INDENT + replace_prefix(constant.text.strip())
                    value = constant.find('value')
                    if not value is None:
                        decl += ' = ' + replace_prefix(value.text.strip())
                    constant_decls.append(decl)
            enum += ',\n'.join(constant_decls) + '\n'
        enum += '}'
        if semicolon:
            enum += ';'
        if newline:
            enum += '\n\n'
        sys.stdout.write(enum)


def typedef(node, newline):
    """Handle typedef generation."""
    if not FUNCTIONS_ONLY:
        docs = Doxygen(node.find('doxygen'))
        name = replace_prefix(node.text.strip())
        if name is None:
            raise Exception('missing typedef type name')
        type = node.find('type')
        if type is None:
            raise Exception('missing typedef type')
        if docs != '':
            print(docs.output())
        print('typedef')
        generate(type, False, False)
        if type.text is not None:
            sys.stdout.write(replace_prefix(type.text.strip()))
        sys.stdout.write(' ' + name)
        print(';\n')


def function(node, semicolon, newline, out=True):
    """Handle function generation."""
    doxygen = Doxygen(node.find('doxygen'))
    if node.text is None:
        raise Exception('missing function name')
    return_type = node.find('return')
    if return_type is None:
        raise Exception('missing function return')
    if return_type.text is None:
        raise Exception("missing function return type name")
    doxygen_return = return_type.find('doxygen')
    if not doxygen_return is None:
        tag = doxygen_return.find('return')
        if not tag is None:
            doxygen.ret = tag.text
    function_str = replace_prefix(return_type.text.strip()) + ' '
    prefix_name = node.text.strip()
    name = replace_prefix(replace_stub_prefix(prefix_name, STUB_PREFIX))
    name = replace_function_prefix(name)
    prefix_name = replace_stub_prefix(prefix_name)
    form = node.attrib.get('form')
    if not form is None:
        if form == 'pointer':
            function_str += '(*' + name + ')('
        else:
            raise Exception('invalid function form: ' + form)
    else:
        function_str += name + '('
    params = node.findall('param')
    doxygen.params = []
    param_names = []
    if len(params) > 0:
        param_decls = []
        for param in params:
            param_type = param.find('type')
            if param_type is None:
                raise Exception('missing function parameter type')
            param_function = param_type.find('function')
            if not param_function is None:
                param_function_form = param_function.attrib.get('form')
                if param_function_form != 'pointer':
                    raise Exception('function parameter of type function is '
                                    'not a function pointer')
                param_decls.append(
                    function(param_function, False, False, False))
                param_names.append(param_function.text.strip())
                doxygen_param = DoxygenParam(param_function.text.strip(),
                                             param.find('doxygen')).output()
                if doxygen_param != '':
                    doxygen.params.append(doxygen_param.strip())
                continue
            elif param_type.text is None:
                raise Exception('missing function parameter type name')
            decl = replace_prefix(param_type.text.strip())
            if not param.text is None:
                decl += ' ' + replace_prefix(param.text.strip())
                doxygen_param = DoxygenParam(param.text,
                                             param.find('doxygen')).output()
                if doxygen_param != '':
                    doxygen.params.append(doxygen_param)
                param_names.append(param.text)
            param_decls.append(decl)
        function_str += ', '.join(param_decls)
    function_str += ')'
    if semicolon:
        function_str += ';'
    if newline:
        function_str += '\n'
    if not doxygen.empty() and (STUB_VALUE is None or STUB_VOID is None):
        function_str = doxygen.output() + '\n' + function_str
    if out:
        print(function_str)
        if STUB_VALUE is not None and STUB_VOID is not None:
            STUB = STUB_VOID if return_type.text == 'void' else STUB_VALUE
            print('{\n' + replace_stub(STUB.text, prefix_name, return_type.text, param_names) +
                  '\n}\n')
    else:
        return function_str


def comment(node, newline):
    """Handle comment generation."""
    comment = '// '
    if node.text:
        lines = node.text.split('\n')
        comment += '\n// '.join(lines)
    if newline:
        comment += '\n'
    print(comment)


def block(node):
    """Handle block generation."""
    generate(node, False, False)
    print()


def scope(node, semicolon, newline):
    """Handle scope generation."""
    scope = ''
    open = True
    close = True
    form = node.attrib.get('form')
    if form:
        if form == 'open':
            close = False
        elif form == 'close':
            open = False
        else:
            raise Exception('invalid scope form: ' + form)
    name = ''
    if node.text:
        name = replace_prefix(node.text.strip())
    if open:
        if name == '':
            print('{')
        else:
            print(name + ' {')
    generate(node, semicolon, newline)
    if close:
        if name == '':
            print('}')
        else:
            print('}  // ' + name)
    if newline:
        print()


def guard(node, semicolon, newline):
    """Handle guard generation."""
    if not node.text:
        raise Exception('missing guard name')
    name = replace_prefix(replace_stub_prefix(node.text.strip(), STUB_PREFIX))
    name = replace_function_prefix(name)
    form = node.attrib.get('form')
    if form == 'include':
        print('#ifndef ' + name)
        print('#define ' + name + '\n')
        generate(node, semicolon, True)
        print('#endif  // ' + name)
    elif form == 'defined':
        print('#ifdef ' + name)
        generate(node, semicolon, False)
        print('#endif  // ' + name)
    else:
        print('#ifndef ' + name)
        generate(node, semicolon, False)
        print('#endif  // ' + name)
    if newline:
        print()


def code(node):
    """Handle core generation."""
    form = node.attrib.get('form')
    if not FUNCTIONS_ONLY or (form and form == 'always'):
        if node.text:
            print(replace_prefixes(node.text))


def includes_stubs():
    """Handle stub include generation."""
    if not includes_stubs.ALREADY_INCLUDED:
        for stub_include in STUB_INCLUDES:
            if '${foreach}' in stub_include:
                loop = stub_include[stub_include.find('(') + 1:
                                    stub_include.find(')')].split(' ')
                elem = loop[0]
                var_name = loop[2]
                expr = stub_include[stub_include.find(')') + 1:
                                    stub_include.find('${endforeach}')]
                for variable in VARIABLES:
                    if var_name == variable.name:
                        for value in variable.values:
                            print('#include <' + expr.replace(
                                '${' + elem + '}', value) + '>')
            else:
                print(replace_prefix('#include <' + stub_include + '>'))
        print()
        includes_stubs.ALREADY_INCLUDED = True


includes_stubs.ALREADY_INCLUDED = False


def generate(parent, semicolon=True, newline=True):
    """Main entry point for generation, dispatches to other generate functions."""

    def dispatch(tag):
        """Dispatch to generation function."""
        {'block': block, 'code': code}[tag](node)

    def dispatch_newline(tag):
        """Dispatch to generation function with newline."""
        {'include': include,
         'define': define,
         'typedef': typedef,
         'comment': comment}[tag](node, newline)

    def dispatch_semicolon_newline(tag):
        """Dispatch to generation fucntion with semicolon and newline."""
        {'struct': struct,
         'union': union,
         'enum': enum,
         'function': function,
         'scope': scope,
         'guard': guard}[tag](node, semicolon, newline)

    def dont_dispatch(_):
        """Don't dispatch, skip."""
        pass

    if STUB_VALUE is None or STUB_VOID is None:
        for node in parent:
            {'include': dispatch_newline,
             'define': dispatch_newline,
             'struct': dispatch_semicolon_newline,
             'union': dispatch_semicolon_newline,
             'enum': dispatch_semicolon_newline,
             'typedef': dispatch_newline,
             'function': dispatch_semicolon_newline,
             'comment': dispatch_newline,
             'block': dispatch,
             'scope': dispatch_semicolon_newline,
             'guard': dispatch_semicolon_newline,
             'code': dispatch,
             'stubs': dont_dispatch}[node.tag](node.tag)
    else:
        for node in parent:
            if node.tag == 'comment':
                comment(node, True)
            elif node.tag == 'guard':
                if STUB_GUARDS_ON:
                    guard(node, True, True)
                else:
                    for guard_node in node:
                        if guard_node.tag == 'function':
                            function(guard_node, False, False)
            elif node.tag == 'scope':
                scope(node, True, False)
            elif node.tag == 'function':
                stub_attrib = node.attrib.get('stub')
                if stub_attrib != 'none':
                    if STUB_QUALIFIER != '':
                        sys.stdout.write(STUB_QUALIFIER + ' ')
                    function(node, False, False)
            elif node.tag == 'block':
                # TODO: This is a hack to place include's correctly, it is not
                # a general solution as the INCLUDES will be inserted more
                # than once if there is more than <block></block> in the schema.
                # This should be replaced with a general solution if it causes
                # any problems.
                includes_stubs()


def main():
    global INDENT
    global PREFIX
    global INCLUDES
    global FUNCTION_PREFIX
    global FUNCTIONS_ONLY
    global STUB_GUARDS_ON
    global STUB_INCLUDES
    global STUB_PREFIX
    global STUB_QUALIFIER
    global STUB_VALUE
    global STUB_VOID

    parser = argparse.ArgumentParser(
        description='Generate C from an XML schema.')
    parser.add_argument('schema', help='XML scheme file')
    parser.add_argument(
        '-p', '--prefix', default='', help='identifier to be prefixed')
    parser.add_argument('-s', '--stub', help='output function stub')
    parser.add_argument(
        '-v',
        '--variables',
        action='append',
        help='add user variables <variable>:<value>')
    parser.add_argument(
        '-f', '--function-prefix', help='function prefix identifier')
    parser.add_argument(
        '-F',
        '--functions-only',
        action='store_true',
        help='only output functions')
    parser.add_argument(
        '-g', '--stub-guards', action='store_true', help='enable stub guards')
    parser.add_argument(
        '-i',
        '--includes',
        action='append',
        help='header to included, may be specified multiple time')

    args = parser.parse_args()

    tree = XML.parse(args.schema)
    interface = tree.getroot()

    if not is_identifier(args.prefix):
        raise Exception('Invalid C identifier prefix: {0}'.format(args.prefix))
    PREFIX = args.prefix

    if args.stub:
        stubs = interface.find('stubs')
        for node in stubs:
            if node.tag == 'stub':
                if args.stub == node.attrib.get('name'):
                    STUB_VALUE = node.find('return-value')
                    STUB_VOID = node.find('return-void')
                    if STUB_VALUE is None: STUB_VALUE = node
                    if STUB_VOID is None: STUB_VOID = node
                    prefix_stub = node.attrib.get('prefix')
                    if prefix_stub != '':
                        STUB_PREFIX = prefix_stub
                    qual = node.attrib.get('qualifier')
                    if qual != '':
                        STUB_QUALIFIER = qual
            elif node.tag == 'include':
                STUB_INCLUDES.append(node.text)
        if STUB_VOID is None or STUB_VALUE is None:
            raise Exception('could not find stub named: {0}'.format(args.stub))

    if args.variables:
        for variable in args.variables:
            name, values = variable.split(':')
            VARIABLES.append(Variable(name, values.split(',')))

    if not is_identifier(args.function_prefix):
        raise Exception('invalid C function prefix: {0}'.format(
            args.function_prefix))
    FUNCTION_PREFIX = args.function_prefix

    FUNCTIONS_ONLY = args.functions_only

    STUB_GUARDS_ON = args.stub_guards

    if args.includes:
        INCLUDES = args.includes

    generate(interface)


if __name__ == '__main__':
    main()
