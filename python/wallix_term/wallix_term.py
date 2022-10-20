# ./tools/cpp2ctypes/cpp2ctypes.lua 'src/rvt_lib/terminal_emulator.hpp' '-c'

from .wallix_term_lib import (lib,
                              TerminalEmulatorLogFunction,
                              TerminalEmulatorBufferGetBufferFn,
                              TerminalEmulatorBufferExtraMemoryAllocatorFn,
                              TerminalEmulatorBufferSetFinalBufferFn,
                              TerminalEmulatorBufferClearFn,
                              TerminalEmulatorBufferDeleteCtxFn,
                              TerminalEmulatorOutputFormat as OutputFormat,
                              TerminalEmulatorTranscriptPrefix as TranscriptPrefix,
                              TerminalEmulatorCreateFileMode as CreateFileMode
                              )
from collections import namedtuple
from ctypes import byref, cast, c_size_t, c_char, c_void_p, Array, addressof
from enum import Enum
from os import fsencode, strerror, PathLike
from typing import Callable, Any, Optional, Union, Tuple, NamedTuple


PathLikeObject = Union[str, bytes, PathLike]


# OutputFormat.json = 0
# OutputFormat.ansi = 1

# TranscriptPrefix.noprefix = 0
# TranscriptPrefix.datetime = 1

# CreateFileMode.fail_if_exists = 0
# CreateFileMode.force_create = 1


class Allocator(NamedTuple):
    ctx: Any
    get_buffer_fn: Any  # :Callable[[void*,size_t*], char*]
    extra_memory_allocator_fn: Any  # :Callable[[void*,size_t,char*,size_t], char*]
    set_final_buffer_fn: Any  # :Callable[[void*,char*,size_t], None]
    clear_fn: Any  # :Callable[[void*], None]
    delete_ctx_fn: Any  # :Callable[[void*], None]


class TerminalEmulatorException(Exception):
    pass


def _check_errnum(errnum: int) -> None:
    if errnum == 0:
        return

    if errnum == -2:
        raise TerminalEmulatorException("bad argument(s)")

    if errnum == -3:
        raise TerminalEmulatorException("bad alloc")

    if errnum < 0:
        raise TerminalEmulatorException("internal error")

    raise TerminalEmulatorException(strerror(errnum))


class TerminalEmulator:
    __slot__ = ('_ctx')

    def __init__(self, lines: int, columns: int, title: Optional[str] = None) -> None:
        self._ctx = lib.terminal_emulator_new(lines, columns)

        if title:
            self.set_title(title)

        if not self._ctx:
            raise TerminalEmulatorException("malloc error")

    def __del__(self) -> None:
        lib.terminal_emulator_delete(self._ctx)

    def set_log_function(self, func: Callable[[str], None]) -> None:
        log_func = lambda p,n: func(p.decode())
        _check_errnum(lib.terminal_emulator_set_log_function(
            self._ctx, TerminalEmulatorLogFunction(log_func)))
        # extend lifetime
        self._log_func = log_func

    def set_title(self, title: str) -> None:
        _check_errnum(lib.terminal_emulator_set_title(self._ctx, title.encode()))

    def feed(self, s: bytes) -> None:
        _check_errnum(lib.terminal_emulator_feed(self._ctx, s, len(s)))

    def resize(self, lines: int, columns: int) -> None:
        _check_errnum(lib.terminal_emulator_resize(self._ctx, lines, columns))


class TerminalEmulatorBuffer:
    __slot__ = ('_ctx', '_allocator')

    def __init__(self, allocator: Allocator = None) -> None:
        self._allocator = allocator

        if allocator:
            self._ctx = lib.terminal_emulator_buffer_new_with_custom_allocator(
                allocator.ctx,
                allocator.get_buffer_fn,
                allocator.extra_memory_allocator_fn,
                allocator.set_final_buffer_fn,
                allocator.clear_fn,
                allocator.delete_ctx_fn)
        else:
            self._ctx = lib.terminal_emulator_buffer_new()

        if not self._ctx:
            raise Exception("malloc error")

    def __del__(self) -> None:
        lib.terminal_emulator_buffer_delete(self._ctx)

    def prepare(self, emu: TerminalEmulator, format: OutputFormat, extra_data: Optional[bytes] = None) -> None:
        if extra_data:
            _check_errnum(lib.terminal_emulator_buffer_prepare2(
                self._ctx, emu._ctx, int(format), extra_data, len(extra_data)))
        else:
            _check_errnum(lib.terminal_emulator_buffer_prepare(self._ctx, emu._ctx, int(format)))

    def prepare_transcript_from_ttyrec(self,
                                       infile: PathLikeObject,
                                       prefix_type: TranscriptPrefix = TranscriptPrefix.datetime) -> None:
        _check_errnum(lib.terminal_emulator_buffer_prepare_transcript_from_ttyrec(
            self._ctx, fsencode(infile), prefix_type))

    def as_bytes(self) -> bytes:
        return self.get_data().raw

    def get_data(self) -> Array:
        p, n = self.get_raw_data()
        return (c_char * n).from_address(p)

    def get_raw_data(self) -> Tuple[int, int]:
        """
        Return address and size
        """
        n = c_size_t()
        p = lib.terminal_emulator_buffer_get_data(self._ctx, byref(n))
        if not p:
            raise TerminalEmulatorException('invalid buffer')
        return (addressof(p.contents), n.value)

    def write(self, filename: PathLikeObject,
              mode: int = 0o664,
              create_mode: CreateFileMode=CreateFileMode.fail_if_exists) -> None:
        _check_errnum(lib.terminal_emulator_buffer_write(self._ctx, fsencode(filename), mode, create_mode))

    def write_integrity(self,
                        filename: PathLikeObject,
                        mode: int = 0o664,
                        prefix_tmp_filename: Optional[PathLikeObject] = None) -> None:
        filename = fsencode(filename)
        _check_errnum(lib.terminal_emulator_buffer_write_integrity(
            self._ctx, filename, fsencode(prefix_tmp_filename) if prefix_tmp_filename else filename, mode))


def transcript_from_ttyrec(infile: PathLikeObject,
                           outfile: Optional[PathLikeObject] = None,
                           mode: int = 0o664,
                           create_mode: CreateFileMode = CreateFileMode.fail_if_exists,
                           prefix_type: TranscriptPrefix = TranscriptPrefix.datetime) -> None:
    """
    Generate a transcript file of session recorded by ttyrec
    """
    _check_errnum(lib.terminal_emulator_transcript_from_ttyrec(fsencode(infile),
                                                               fsencode(outfile) if outfile else None,
                                                               mode, create_mode, prefix_type))
