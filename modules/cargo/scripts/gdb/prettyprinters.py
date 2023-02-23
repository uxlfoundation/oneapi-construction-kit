"""GDB pretty printers for cargo containers.

For more information about the ``gdb`` Python API visit the online
documentation.

https://sourceware.org/gdb/onlinedocs/gdb/Python-API.html#Python-API
"""

import gdb


class Value(object):
    """Base class for printing a value wrapper."""

    def to_string(self):
        """GDB will call this method to display the string representation of
        the value passed to the object's constructor."""
        raise NotImplementedError


class ArrayIterator(object):
    """Base class for printing array-like objects."""

    begin = None
    size = 0
    i = 0

    def __iter__(self):
        return self

    def __next__(self):
        if self.i == self.size:
            raise StopIteration
        result = ('[{}]'.format(self.i), (self.begin + self.i).dereference())
        self.i += 1
        return result

    def next(self):
        """Support Python 2, alias to __next__."""
        return self.__next__()

    def children(self):
        """GDB will call this method on a pretty-printer to compute the
        children of the pretty-printer's value."""
        return self

    def to_string(self):
        """GDB will call this method to display the string representation of
        the value passed to the object's constructor."""
        raise NotImplementedError

    def display_hint(self):  # pylint: disable=no-self-use
        """Indicate that the object being printed is 'array-like'."""
        return 'array'


class ArrayViewPrinter(ArrayIterator):
    """Print a cargo::array_view object."""

    def __init__(self, value):
        self.value = value
        self.T = value.type.template_argument(0)
        self.begin = value['Begin'].cast(self.T.pointer())
        self.size = value['End'].cast(self.T.pointer()) - self.begin

    def to_string(self):
        return 'cargo::array_view<{}> of size {}'.format(
            self.T, self.size)


class DynamicArrayPrinter(ArrayIterator):
    """Print a cargo::dynamic_array<T> object."""

    def __init__(self, value):
        self.value = value
        self.T = value.type.template_argument(0)
        self.begin = value['Begin'].cast(self.T.pointer())
        self.size = value['End'].cast(self.T.pointer()) - self.begin

    def to_string(self):
        return 'cargo::dynamic_array<{}> of size {}'.format(
            self.T, self.size)


class RingBufferPrinter(ArrayIterator):
    """Print a cargo::ring_buffer<T, N> object."""

    def __init__(self, value):
        self.value = value
        self.T = value.type.template_argument(0)
        self.size = value.type.template_argument(1)
        # Get pointer to the C array data member of std::array
        for field in self.value['payload'].type.fields():
            self.begin = self.value['payload'][field.name].cast(
                self.T.pointer())
            break
        enqueue_index = self.value['enqueue_index']
        self.i = self.value['dequeue_index']
        self.empty = self.value['empty']
        # Determine the number of elements in the ring buffer
        self.count = 0
        if enqueue_index == self.i:
            self.max = self.size
        elif enqueue_index > self.i:
            self.max = enqueue_index - self.i
        elif enqueue_index < self.i:
            self.max = self.size - self.i + enqueue_index

    def __next__(self):
        if self.empty or self.count == self.max:
            raise StopIteration
        result = ('[{}]'.format(self.i), (self.begin + self.i).dereference())
        self.i = 0 if (self.i + 1) == self.size else self.i + 1
        self.count += 1
        return result

    def to_string(self):
        return 'cargo::ring_buffer<{}, {}>{}'.format(
            self.T, self.size, ' is empty' if self.empty else '')


class SmallVectorPrinter(ArrayIterator):
    """Print a cargo::small_vector<T, N> object."""

    def __init__(self, value):
        self.value = value
        self.T = value.type.template_argument(0)
        self.N = value.type.template_argument(1)
        self.begin = value['Begin'].cast(self.T.pointer())
        self.size = value['End'].cast(self.T.pointer()) - self.begin
        self.capacity = value['Capacity']

    def to_string(self):
        return 'cargo::small_vector<{}, {}> of size {}, capacity {}'.format(
            self.T, self.N, self.size, self.capacity)


class ErrorOrPrinter(Value):
    """Print a cargo::error_or<T> object."""

    def __init__(self, value):
        self.value = value
        self.T = value.type.template_argument(0)
        self.result = gdb.lookup_type('cargo::result')

    def to_string(self):
        return 'cargo::error_or<{}> = {}'.format(
            self.T,
            self.value['ErrorStorage'] if self.value['HasError'] else
            self.value['ValueStorage'])


class OptionalPrinter(Value):
    """Print a cargo::optional<T> object."""

    def __init__(self, value):
        self.value = value
        self.T = value.type.template_argument(0)
        self.has_value = self.value['m_has_value']

    def to_string(self):
        return 'cargo::optional<{}> = {}'.format(
            self.T,
            self.value['m_value'] if self.has_value else 'cargo::nullopt')


class ExpectedPrinter(Value):
    """Print a cargo::expected<T, E> object."""

    def __init__(self, value):
        self.value = value
        self.T = value.type.template_argument(0)
        self.E = value.type.template_argument(1)
        self.has_value = self.value['m_has_val']
        if self.has_value:
            # If T is a reference, unpack std::reference_wrapper. `_M_data` is
            # a libstdc++ thing, so this won't work for libc++.
            if gdb.TYPE_CODE_REF == self.T.code:
                self.held_obj = self.value['m_val']['_M_data']
            else:
                self.held_obj = self.value['m_val']
        else:
            self.held_obj = self.value['m_unexpect']['m_val']
        # Stringify the contained object. try-except is needed, because expected
        # objects sometimes appear in invalid states when returned from
        # functions (possibly because of RVO). Without the `except`,
        # `(gdb) finish` can crash.
        if (gdb.TYPE_CODE_PTR == self.held_obj.type.code
                or gdb.TYPE_CODE_MEMBERPTR == self.held_obj.type.code):
            try:
                self.print_value = str(self.held_obj.dereference())
            except Exception:
                self.print_value = "{nullptr (possible RVO artifact)}"
        else:
            self.print_value = str(self.held_obj)

    def to_string(self):
        return 'cargo::expected<{}, {}> = ({}) {}'.format(
            self.T, self.E,
            'expected' if self.has_value else 'unexpected',
            self.print_value)


class StringViewPrinter(Value):
    """Print a cargo::string_view object."""

    def __init__(self, value):
        self.value = value;
        self.size = int(value['Size'])
        if value['Begin']:
            # string() returns up to `\0`. We only want `size` characters.
            self.str = value['Begin'].string()[:self.size]
            # str() stringifies the pointee as well. We only want the address.
            self.Begin = str(value['Begin']).split()[0]
        else:
            self.str = ""
            self.Begin = 'nullptr'

    def to_string(self):
        return self.str

    def display_hint(self):  # pylint: disable=no-self-use
        """Indicate that the object being printed is 'string-like'."""
        return 'string'

    def children(self):
        """GDB will print these key-value pairs after the string."""
        return [('type', 'cargo::string_view'),
            ('size', self.size),
            ('Begin', self.Begin)]


PP = gdb.printing.RegexpCollectionPrettyPrinter("cargo")

PP.add_printer('cargo::array_view', '^cargo::array_view<.*>$',
               ArrayViewPrinter)
PP.add_printer('cargo::dynamic_array', '^cargo::dynamic_array<.*>$>',
               DynamicArrayPrinter)
PP.add_printer('cargo::error_or', '^cargo::error_or<.*>$', ErrorOrPrinter)
PP.add_printer('cargo::expected', '^cargo::expected<.*>$', ExpectedPrinter)
PP.add_printer('cargo::optional', '^cargo::optional<.*>$', OptionalPrinter)
PP.add_printer('cargo::ring_buffer', '^cargo::ring_buffer<.*>$',
               RingBufferPrinter)
PP.add_printer('cargo::small_vector', '^cargo::small_vector<.*>$',
               SmallVectorPrinter)
PP.add_printer('cargo::string_view', '^cargo::string_view$', StringViewPrinter)

gdb.printing.register_pretty_printer(gdb.current_objfile(), PP)
