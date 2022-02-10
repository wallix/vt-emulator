# ./tools/cpp2ctypes/cpp2ctypes.lua 'src/rvt_lib/terminal_emulator.hpp' '-c'

from .wallix_term_lib import lib
from collections import namedtuple
from ctypes import byref, c_size_t, c_char, c_void_p, Array
from enum import Enum
from os import fsencode, strerror, PathLike
from typing import Callback, Any, Optional, Union


PathLikeObject = Union[str, bytes, PathLike]


# OutputFormat.json = 0
# OutputFormat.ansi = 1
OutputFormat = lib.TerminalEmulatorOutputFormat

# TranscriptPrefix.noprefix = 0
# TranscriptPrefix.datetime = 1
TranscriptPrefix = lib.TerminalEmulatorTranscriptPrefix

# CreateFileMode.fail_if_exists = 0
# CreateFileMode.force_create = 1
CreateFileMode = lib.TerminalEmulatorCreateFileMode


Allocator = namedtuple('TerminalEmulatorAllocator',
                        ['ctx',
                         'get_buffer_fn',
                         'extra_memory_allocator_fn',
                         'set_final_buffer_fn',
                         'clear_fn',
                         'delete_ctx_fn'])


class TerminalEmulatorException(Exception):
    pass


def _check_errnum(errnum) -> None:
    if errnum == 0:
        pass

    if errnum == -2:
        raise TerminalEmulatorException("bad argument(s)")

    if errnum < 0:
        raise TerminalEmulatorException("internal error")

    raise TerminalEmulatorException(strerror(errnum))


class TerminalEmulator:
    __slot__ = ('_ctx')

    def __init__(self, lines:int, columns:int, title:Optional[str]=None, allocator:Allocator=None) -> None:
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

        if title:
            self.set_title(title)

        if not self._ctx:
            raise TerminalEmulatorException("malloc error")

    def __del__(self) -> None:
        lib.terminal_emulator_delete(self._ctx)

    def set_log_function(self, func:Callback[[Array], None]) -> None:
        _check_errnum(lib.terminal_emulator_set_log_function(
            self._ctx, lambda p,n: func((c_char * n).from_address(p))))

    def set_log_raw_function(self, func:Callback[[bytes, int], None]) -> None:
        _check_errnum(lib.terminal_emulator_set_log_function(self._ctx, func))

    def set_title(self, title:str) -> None:
        _check_errnum(lib.terminal_emulator_set_title(self._ctx, title))

    def feed(self, s:bytes) -> None:
        _check_errnum(lib.terminal_emulator_feed(self._ctx, s, len(s)))

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

    def prepare(self, emu:TerminalEmulator, format:OutputFormat, extra_data:Optional[bytes]=None) -> None:
        if extra_data:
            _check_errnum(lib.terminal_emulator_buffer_prepare2(
                self._ctx, emu, format, extra_data, len(extra_data)))
        else:
            _check_errnum(lib.terminal_emulator_buffer_prepare(self._ctx, emu, format))

    def get_data(self) -> Array:
        n = c_size_t()
        p = lib.terminal_emulator_buffer_get_data(self._ctx, byref(n))
        if not p:
            raise TerminalEmulatorException('invalid buffer')
        return (c_char * n).from_address(p)

    def write(self, filename:PathLikeObject,
              mode:int=0o664,
              create_mode:CreateFileMode=CreateFileMode.fail_if_exists) -> None:
        _check_errnum(lib.terminal_emulator_buffer_write(self._ctx, fsencode(filename), mode, create_mode))

    def write_integrity(self,
                        filename:PathLikeObject,
                        mode:int=0o664,
                        prefix_tmp_filename:Optional[PathLikeObject]=None) -> None:
        filename = fsencode(filename)
        _check_errnum(lib.terminal_emulator_buffer_write_integrity(
            self._ctx, filename, fsencode(prefix_tmp_filename) if prefix_tmp_filename else filename, mode))


def transcript_from_ttyrec(infile:PathLikeObject,
                           outfile:Optional[PathLikeObject]=None,
                           mode:int=0o664,
                           create_mode:CreateFileMode=CreateFileMode.fail_if_exists,
                           prefix_type:TranscriptPrefix=TranscriptPrefix.datetime) -> None:
    """
    Generate a transcript file of session recorded by ttyrec
    """
    _check_errnum(lib.terminal_emulator_transcript_from_ttyrec(fsencode(infile),
                                                               fsencode(outfile) if outfile else None,
                                                               mode, create_mode, prefix_type))
