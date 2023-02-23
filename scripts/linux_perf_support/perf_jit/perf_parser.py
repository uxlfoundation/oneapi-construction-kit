#!/usr/bin/python3
# Copyright (c) Codeplay Software Ltd. All rights reserved.

import re
import sys
from . import command_line as cmd
from . import utils as util
from . import disassembly as dis_asm


class LineParser(object):
    """Base class to hold and parse lines from a file
       -to be subclassed in the perf-log parser"""
    def __init__(self, file_fd):
        self._file_handle = file_fd
        self._line = None  # most likely holding byte stream
        self._lineno = 0
        self._eof = False

    @property
    def file_handle(self):
        return self._file_handle

    @file_handle.setter
    def file_handle(self, fd):
        self._file_handle = fd

    @property
    def line(self):
        return self._line.decode("utf-8")

    @line.setter
    def line(self, content):
        self._line = content

    @property
    def lineno(self):
        return self._lineno

    @lineno.setter
    def lineno(self, line_num):
        self._lineno = line_num

    @property
    def eof(self):
        return self._eof

    @eof.setter
    def eof(self, flag):
        assert self._eof is not None
        self._eof = flag

    def read_line(self):
        nextline = self._file_handle.readline()
        if not nextline:
            self._line = ""
            self._eof = True
        else:
            self._lineno += 1
        self._line = nextline.rstrip(b'\r\n')

    def lookahead(self):
        assert self._line is not None
        return self.line #internally converts self._line to text

    def consume(self):
        assert self._line is not None
        current = self.line  #internally converts self._line to text
        self.read_line()
        return current


class PerfParser(LineParser):
    """Parse Perf logs, and match up event samples @addresses
       against functions and addresses found in the map and object
       files"""

    def __init__(self, in_file, events, symbols, sym_map, obj_files):
        if sys.version[0] == '3':
            super().__init__(in_file)
        elif sys.version[0] == '2':
            super(PerfParser, self).__init__(in_file)

        # list of symbols and events to search for - from user
        self._events = events
        self._symbols = symbols

        #event table to hold statistic samples
        #map  { event-name : { func-name : [ ] }}
        self._event_table = {}

        #table to hold disassembly
        #map  { function-name : [ lines of assembly] }
        self._disassembly = {}

        #table to hold jit-function addressess, sizes and names
        # map { func-name : (abs-addr, size) }
        # Should be set
        self.symbol_map = sym_map

        #list of object files to search for symbols
        #This will be lazily evaluated and parsed on demand
        #only if a symbol encountered in the perf log file
        #has not been found so far in the list of user specified
        #object files
        self.obj_file_iter = util.LazyEval(obj_files)

    @property
    def symbol_map(self):
        return self._symbol_map

    @symbol_map.setter
    def symbol_map(self, sym_map):
        if type(sym_map) is not dict:
            print("JIT symbol map should be a dictionary of format")
            print(
                "{ symbol-name : (absolute-address, sizeof(txt section of symbol))}"
            )
            sys.exit("JIT Symbol resolution error")

        self._symbol_map = sym_map

    @property
    def obj_file_iter(self):
        return self._obj_file_iter.items()

    @obj_file_iter.setter
    def obj_file_iter(self, lazy_iter_obj):
        self._obj_file_iter = lazy_iter_obj

    def read_line(self):
        if sys.version[0] == '3':
            return super().read_line()
        elif sys.version[0] == '2':
            return super(PerfParser, self).read_line()

    def _is_relevant_event(self, event):
        """if cmdline option was not given, it defaults to all.
		   Hence return any event as relevant. Otherwise if event
		   found while parsing is within the list passed on command-line
		   return True"""
        if type(self._events) is list \
           and len(self._events) > 0 and  self._events[0] == "all" :
            return True

        if event in self._events:
            return True

        return False

    def _is_relevant_symbol(self, sym):
        """if cmdline option was not given, it defaults to all.
		   Hence return any event as relevant. Otherwise if event
		   found while parsing is within the list passed on command-line
		   return True"""
        if type(self._symbols) is list \
           and len(self._symbols) > 0 and  self._symbols[0] == "all" :
            return True

        if sym in self._symbols:
            return True

        return False

    def extract_event(self):
        """A perf event log record reads as <offset> <symbol-name> (<dso>)
		   dso - dynamic shared object, a map file is also considered as a dso by perf
		"""
        record_chain = []
        while not self.eof and self.lookahead():
            _record = self.consume()
            record_chain.append(_record)

        return record_chain

    #parse an event record and return the call chain
    def parse_event_record(self, event_rec):
        call_chain = []
        callchain_re = re.compile(
            r'^\s*(?P<address>[0-9a-fA-F]+)\s+(?P<symbol>.*)\s+\((?P<module>[^)]*)\)\s*$'
        )
        for call in event_rec:
            matched_rec = callchain_re.match(call)
            if not matched_rec:
                continue

            call_chain.append( (int(matched_rec.group("address"),16) \
                               , matched_rec.group("symbol") \
                               , matched_rec.group("module")) )

        if len(call_chain) == 0:
            return None

        return call_chain

    #Search for events and check if the event found is within user filter
    def search_event(self):
        # search for event header using regular expression matching
        event_re = re.compile(
            r'^\s*(?P<prog>[^\s]*)\s+(?P<pid>[0-9]*)\s+(?P<timestamp>[0-9]+.[0-9]+):\s+(?P<unknown>[^\s]+)\s+(?P<event>[^\s]*):.*$'
        )

        while not self.eof:
            current_line = self.consume()

            is_event = event_re.match(current_line)
            if not is_event:
                continue

            _prog = is_event.group("prog")
            _pid = is_event.group("pid")
            _timestamp = is_event.group("timestamp")
            _unknown = is_event.group("unknown")
            _event = is_event.group("event")

            if not self._is_relevant_event(_event):
                continue

            event_rec = self.extract_event()
            if not event_rec:
                continue

            formatted_event = self.parse_event_record(event_rec)

            # create a dictionary for each symbol that we are interested in
            # if for the current event, in the call-chain, we have already
            # visited the symbol(ie some form of recursion), then go to
            # the next interesting symbol
            already_found = {key: False for key in self._symbols}
            for cchain in formatted_event:
                _addr, _func, _dso = cchain
                if not self._is_relevant_symbol(_func):
                    continue

                if already_found[_func] is True:
                    continue

                _func_start_addr, _func_size = self.symbol_map.get(
                    _func, (None, None))
                if _func_start_addr is None:
                    print("Could not find symbol [{0}] in symbol-map".format(
                        _func))
                    print("symbol-map = {0}".format(self.symbol_map))
                    continue

                already_found[_func] = True
                #insert the event enountered into the event-map
                self._insert_event(_event, _func, _addr)

    # given an object file name, disassemble it
    # and if applicable, add to current map of disassembled code
    # also return a list of symbols (not explicitly pruned)
    # NOTE : Will raise an Assertion exception if duplicates found
    def _cache_disassembly(self, obj_file):
        _xtrct_asm = dis_asm.Extract_Asm(obj_file)
        asm = _xtrct_asm.copy_to_mem(_xtrct_asm.asm_file)
        _xtrct_asm.pickle_asm()
        _obj_functions = _xtrct_asm.function_list()
        _pruned_func_list = filter(
            lambda func: False if cmd.to_prune(func) else True, _obj_functions)

        #append to disassembly map
        for func in _pruned_func_list:
            # check that we do not have duplicates of functions
            assert func not in self._disassembly.keys()
            addr, is_offset, _txt = _xtrct_asm.asm_units[func]
            self._disassembly[func] = _txt

        return [item for item in self._disassembly.keys()]

    #insert relevant event into the event-map
    #while doing this we ensure that the requisite
    #disassembly is loaded on demand so as to save on
    #start-up time and memory
    def _insert_event(self, event, func, addr):
        #ensure we have the function within our disassembly map
	#if it is not within our disassembly map, dynamically obtain it
        if not self._disassembly.get(func, None):
            for obj in self.obj_file_iter:
                new_func_list = self._cache_disassembly(obj)
                if func in new_func_list:
                    break
            else:
                print(
                    "Cannot Locate symbol-<{0}> disassembly amongst provided object files".
                    format(func))
                sys.exit(-1)

        # if control flow reaches here, we have already cached disassembly
        sym_num_lines = len(self._disassembly[func])

        #If we have not already encountered the event, create the entry in the map
        #event-map has the following format
        #{ event :  {func-name : (total-samples-function, {phys-off:asm-txt-offset},[sample-counter for each function ]) }}
        _func_offset_map = self._event_table.get(event, None)
        if not _func_offset_map:
            #first time that current function has been
            #seen for current event hence obtain dis-assembly
            #and create sample statistics buffer
            self._event_table[event] = {func: self._construct_event(func)}
            _func_offset_map = self._event_table[event]

        # ensure function-offset-map contains our required function
        # if not insert into function-offset-map
        _rec = _func_offset_map.get( func, (None, None, None))
        _total, _offset_map, _samples = _rec
        if _samples is None:
            _func_offset_map[func] = self._construct_event(func)
            _rec = _func_offset_map.get( func, (None, None, None))
            _total, _offset_map, _samples = _rec

        # If control has reached here :
        # 1) function is in disassembly map
        # 2) event exists in event-map
        # 3) perf samples map exists for relevant event-function combination

        #look-up assembly to find offset and insert into function-offset-map
        _offset = self._symbol_offset(func, addr)
        _total += 1
        _samples[_offset_map[_offset]] += 1

        _func_offset_map[func] = self._update_stat(_rec, _total, 0)


    def _symbol_offset(self, func, abs_addr):
        _start, _size = self.symbol_map.get(func, 0)
        return abs_addr - _start

    def _update_stat(self, rec, val, idx):
        """utility function - updates value within a tuple
           Needed because Python tuples are actually immutable"""
        return rec[:idx] + (val, ) + rec[idx + 1:]

    def _construct_event(self, func, init_total=0, buff_size=0, fill=0):
        """assumes that the disassembly for the function
		   has already been written into self._disassembly
		   Helper function for _insert_event()
		   creates a tuple ( init_total
		                     , { phys-offset : dis-assembly-txt-offset }
		                     , [ perf-sample-array ] )
		   """
        _dis = self._disassembly.get(func, None)
        if not _dis:
            return None

        # probe and find out length of required sample-statistic buffer
        if buff_size == 0:
            buff_size = len(_dis)

        #for each-line in the disassembly, extract line offset
        offset_map = {}
        for txt_off, line in enumerate(_dis):
            offset_map[int(line.split(':')[0].lstrip(), 16)] = txt_off

        return (init_total, offset_map,
                [fill for val in range(0, buff_size, 1)])

    def parse(self):
        self.read_line()
        while not self.eof:
            self.search_event()

    # Final annotation display function
    def annotate_jit_asm(self):
        def display_hdr(event, func, total_samples):
            _start, _size = self.symbol_map.get(func, (0, 0))
            print("================================================")
            print("Perf-Event:[{0}],Samples=[{1}]".format(
                event, total_samples))
            print("================================================")
            print("Function:[{0}] \tAddr:[0x{1:x}]".format(func, _start))

        def display_body(func, samples):
            body = self._disassembly.get(func, None)
            if not body:
                return None

            for idx, lines in enumerate(body):
                prefix = lambda idx : "{0:s}".format("      ") if samples[idx] == 0 \
                                      else "{0: 3d} ->".format(samples[idx])

                print("{0}{1}".format(prefix(idx), body[idx]))

        for event in self._event_table.keys():
            _sample_map = self._event_table.get(event, None)
            if _sample_map is None:
                print(
                    "Error querying offset map for event [{0}]".format(event))
                continue

            for func in _sample_map.keys():
                _tot, _offset_map, _samples = _sample_map.get(
                    func, (None, None, None))
                display_hdr(event, func, _tot)
                display_body(func, _samples)

    def display_log(self):
        print("=======================================")
        print("perf log")
        print("=======================================")
        self.read_line()
        while not self.eof:
            print("{0}".format(self.consume()))


def main():
    """Main program"""
    perf_process = subprocess.Popen(
        ['perf', 'script'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    perf_parse = PerfParser(perf_process.stdout, cmdline.get_option('event'),
                            cmdline.get_option('func'),
                            cmdline.unified_jit_map(),
                            cmdline.get_option('obj'))
    perf_parse.parse()
    perf_parse.annotate_jit_asm()

if __name__ == '__main__':
    main()
