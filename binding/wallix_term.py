# -*- coding: utf-8 -*-

import ctypes
import ctypes.util

from ctypes import CFUNCTYPE, c_void_p, c_int, c_char_p, c_uint

RVT_OF_JSON = 0
RVT_OF_ANSI = 1

RVT_FILE_FAIL_IF_EXISTS = 0
RVT_FILE_FORCE_CREATE = 1

RVT_TRANSCRIPT_NO_PREFIX = 0
RVT_TRANSCRIPT_DATETIME_PREFIX = 1

try:
    # libpath = ctypes.util.find_library('wallix_term')
    # lib = ctypes.CDLL(libpath)
    lib = ctypes.CDLL("libwallix_term.so")

    # char const * terminal_emulator_version() noexcept
    lib.terminal_emulator_version.argtypes = []
    lib.terminal_emulator_version.restype = c_char_p

    # rvt_lib::TerminalEmulator * terminal_emulator_init(int lines, int columns)
    lib.terminal_emulator_init.argtypes = [c_int, c_int]
    lib.terminal_emulator_init.restype = c_void_p

    # int terminal_emulator_deinit(rvt_lib::TerminalEmulator *) noexcept
    lib.terminal_emulator_deinit.argtypes = [c_void_p]
    lib.terminal_emulator_deinit.restype = c_int

    # int terminal_emulator_finish(rvt_lib::TerminalEmulator *)
    lib.terminal_emulator_finish.argtypes = [c_void_p]
    lib.terminal_emulator_finish.restype = c_int

    # int terminal_emulator_set_title(rvt_lib::TerminalEmulator *,
    #     char const * title)
    lib.terminal_emulator_set_title.argtypes = [c_void_p, c_char_p]
    lib.terminal_emulator_set_title.restype = c_int

    # int terminal_emulator_set_log_function(rvt_lib::TerminalEmulator *,
    #     void(* log_func)(char const *))
    lib.terminal_emulator_set_log_function.argtypes = [
        c_void_p, CFUNCTYPE(None, c_char_p)]
    lib.terminal_emulator_set_log_function.restype = c_int

    # int terminal_emulator_set_log_function_ctx(rvt_lib::TerminalEmulator *,
    #     void(* log_func)(void *, char const *), void * ctx)
    lib.terminal_emulator_set_log_function_ctx.argtypes = [
        c_void_p, CFUNCTYPE(None, c_void_p, c_char_p), c_void_p]
    lib.terminal_emulator_set_log_function_ctx.restype = c_int

    # int terminal_emulator_feed(rvt_lib::TerminalEmulator *,
    #     char const * s, int n)
    lib.terminal_emulator_feed.argtypes = [c_void_p, c_char_p, c_int]
    lib.terminal_emulator_feed.restype = c_int

    # int terminal_emulator_resize(rvt_lib::TerminalEmulator *,
    #     int lines, int columns)
    lib.terminal_emulator_resize.argtypes = [c_void_p, c_int, c_int]
    lib.terminal_emulator_resize.restype = c_int

    # int terminal_emulator_buffer_prepare(TerminalEmulator *,
    #     OutputFormat, char const * extra_data)
    lib.terminal_emulator_buffer_prepare.argtypes = [c_void_p, c_int, c_char_p]
    lib.terminal_emulator_buffer_prepare.restype = c_int

    # int terminal_emulator_buffer_size(TerminalEmulator const *)
    lib.terminal_emulator_buffer_size.argtypes = [c_void_p]
    lib.terminal_emulator_buffer_size.restype = c_int

    # char const * terminal_emulator_buffer_data(TerminalEmulator const *)
    lib.terminal_emulator_buffer_data.argtypes = [c_void_p]
    lib.terminal_emulator_buffer_data.restype = c_char_p

    # int terminal_emulator_buffer_reset(TerminalEmulator *)
    lib.terminal_emulator_buffer_data.argtypes = [c_void_p]
    lib.terminal_emulator_buffer_data.restype = c_int

    # int terminal_emulator_write_buffer(TerminalEmulator const *,
    #     char const * filename, int mode, CreateFileMode)
    lib.terminal_emulator_write_buffer.argtypes = [c_void_p, c_char_p, c_int, c_int]
    lib.terminal_emulator_write_buffer.restype = c_int

    # int terminal_emulator_write_buffer_integrity(TerminalEmulator const *,
    #     char const * filename, char const * prefix_tmp_filename, int mode)
    lib.terminal_emulator_write_buffer_integrity.argtypes = [
        c_void_p, c_char_p, c_char_p, c_int]
    lib.terminal_emulator_write_buffer_integrity.restype = c_int

    # int terminal_emulator_write(rvt_lib::TerminalEmulator *,
    #     rvt_lib::OutputFormat, char const * extra_data,
    #     char const * filename, int mode, CreateFileMode)
    lib.terminal_emulator_write.argtypes = [
        c_void_p, c_int, c_char_p, c_char_p, c_int, c_int]
    lib.terminal_emulator_write.restype = c_int

    # int terminal_emulator_write_integrity(rvt_lib::TerminalEmulator *,
    #     rvt_lib::OutputFormat, char const * extra_data,
    #     char const * filename, char const * prefix_tmp_filename, int mode)
    lib.terminal_emulator_write_integrity.argtypes = [
        c_void_p, c_int, c_char_p, c_char_p, c_char_p, c_int]
    lib.terminal_emulator_write_integrity.restype = c_int

    ### brief Generate a transcript file of session recorded by ttyrec
    ### param outfile  output file or stdout if empty/null
    # int terminal_emulator_transcript_from_ttyrec(
    #   char const * infile, char const * outfile, int mode,
    #   CreateFileMode, TranscriptPrefix)
    lib.terminal_emulator_transcript_from_ttyrec.argtypes = [
        c_char_p, c_char_p, c_int, c_int, c_int]
    lib.terminal_emulator_transcript_from_ttyrec.restype = c_int

except AttributeError as e:
    lib = None
    import traceback
    raise ImportError('vt-emulator shared library not found'
                      ' or incompatible (%s)' % traceback.format_exc(e))
except Exception as e:
    lib = None
    import traceback
    raise ImportError('vt-emulator shared library not found.\n'
                      'you probably had not installed libwallix_term'
                      ' library.\n %s\n' % traceback.format_exc(e))
