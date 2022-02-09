# ./tools/cpp2ctypes/cpp2ctypes.lua 'src/rvt_lib/terminal_emulator.hpp' '-c'

from wallix_term.wallix_term_lib import lib
from collections import namedtuple
from enum import Enum
from typing import Callback
from ctypes import byref, c_size_t, c_char, c_void_p, Array
import errno


class TerminalEmulatorOutputFormat(Enum):
    json = 0
    ansi = 1


class TerminalEmulatorTranscriptPrefix(Enum):
    noprefix = 0
    datetime = 1


class TerminalEmulatorCreateFileMode(Enum):
    fail_if_exists = 0
    force_create = 1


TerminalEmulatorAllocator = namedtuple('TerminalEmulatorAllocator',
                                       ['ctx',
                                        'get_buffer_fn',
                                        'extra_memory_allocator_fn',
                                        'set_final_buffer_fn',
                                        'clear_fn',
                                        'delete_ctx_fn'])


class TerminalEmulatorException(Exception):
    pass


def _check_positive(result:int):
    if result < 0:
        raise TerminalEmulatorException("allocation error")
    return result

def _check_errnum(errnum) -> None:
    if errnum == 0:
        pass

    if errnum == -2:
        raise TerminalEmulatorException("bad argument(s)")

    if errnum < 0:
        raise TerminalEmulatorException("internal error")

    raise TerminalEmulatorException(os.strerror(errnum))


class TerminalEmulator:
    __slot__ = ('_ctx')

    def __init__(self, lines:int, columns:int allocator:TerminalEmulatorAllocator=None) -> None:
        if allocator:
            self._ctx = lib.terminal_emulator_buffer_new_with_custom_allocator(
                allocator.ctx,
                allocator.get_buffer_fn,
                allocator.extra_memory_allocator_fn,
                allocator.set_final_buffer_fn,
                allocator.clear_fn,
                allocator.delete_ctx_fn)
        else:
            self._ctx = lib.terminal_emulator_new(lines, columns)

        if not self._ctx:
            raise TerminalEmulatorException("malloc error")

    def __del__(self) -> None:
        lib.terminal_emulator_delete(self._ctx)

    def set_log_function(self, func:Callback[[bytes, int], None]) -> None:
        _check_errnum(lib.terminal_emulator_set_log_function(self._ctx, func))

    def set_log_function_ctx(self, func:, ctx:) -> None:
        _check_errnum(lib.terminal_emulator_set_log_function_ctx(self._ctx, func, ctx))

    def set_title(self, title:str) -> None:
        _check_errnum(lib.terminal_emulator_set_title(self._ctx, title))

    def feed(self, s:bytes, len:int) -> None:
        _check_errnum(lib.terminal_emulator_feed(self._ctx, s, len))

    def resize(self, lines:int, columns:int) -> None:
        _check_errnum(lib.terminal_emulator_resize(self._ctx, lines, columns))


class TerminalEmulatorBuffer:
    __slot__ = ('_ctx')

    def __init__(self) -> None:
        self._ctx = lib.terminal_emulator_buffer_new()
        if not self._ctx:
            raise Exception("malloc error")

    def __del__(self) -> None:
        lib.terminal_emulator_buffer_delete(self._ctx)

    def prepare(self, emu:TerminalEmulator, format:int) -> None:
        _check_errnum(lib.terminal_emulator_buffer_prepare(self._ctx, emu, format))

    def prepare2(self, emu:TerminalEmulator, format:int,
                 extra_data:bytes, extra_data_len:int) -> None:
        _check_errnum(lib.terminal_emulator_buffer_prepare2(
            self._ctx, emu, format, extra_data, extra_data_len))

    def get_data(self, output_len:) -> Array:
        n = c_size_t()
        p = lib.terminal_emulator_buffer_get_data(self._ctx, byref(n))
        if not p:
            raise TerminalEmulatorException('invalid buffer')
        return CTypeBuffer(p, n.value)

    def write(self, filename:str, mode:int, create_mode:int) -> None:
        _check_errnum(lib.terminal_emulator_buffer_write(self._ctx, filename, mode, create_mode))

    def write_integrity(self, filename:str, mode:int, prefix_tmp_filename:str=None) -> None:
        _check_errnum(lib.terminal_emulator_buffer_write_integrity(
            self._ctx, filename, prefix_tmp_filename or filename, mode))
