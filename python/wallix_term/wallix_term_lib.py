# ./tools/cpp2ctypes/cpp2ctypes.lua 'src/rvt_lib/terminal_emulator.hpp' '-l' 'libwallix_term.so'

from ctypes import CDLL, CFUNCTYPE, POINTER, c_char, c_char_p, c_int, c_size_t, c_void_p
from enum import IntEnum

lib = CDLL("libwallix_term.so")

# enum class TerminalEmulatorOutputFormat : int {
#    json,
#    ansi
# }
class TerminalEmulatorOutputFormat(IntEnum):
    json = 0
    ansi = 1

    def from_param(self) -> int:
        return int(self)


# enum class TerminalEmulatorTranscriptPrefix : int {
#    noprefix,
#    datetime,
# }
class TerminalEmulatorTranscriptPrefix(IntEnum):
    noprefix = 0
    datetime = 1

    def from_param(self) -> int:
        return int(self)


# enum class TerminalEmulatorCreateFileMode : int {
#    fail_if_exists,
#    force_create,
# }
class TerminalEmulatorCreateFileMode(IntEnum):
    fail_if_exists = 0
    force_create = 1

    def from_param(self) -> int:
        return int(self)


# \return  0 if success ; -3 for bad_alloc ; -2 if bad argument (emu is null, bad format, bad size, etc) ; -1 if internal error with `errno` code to 0 (bad alloc, etc) ; > 0 is an `errno` code,
# @{
# char const * terminal_emulator_version() noexcept;
terminal_emulator_version = lib.terminal_emulator_version
terminal_emulator_version.argtypes = []
terminal_emulator_version.restype = c_char_p

# BEGIN ctor/dtor
# TerminalEmulator * terminal_emulator_new(int lines, int columns) noexcept;
terminal_emulator_new = lib.terminal_emulator_new
terminal_emulator_new.argtypes = [c_int, c_int]
terminal_emulator_new.restype = c_void_p

# int terminal_emulator_delete(TerminalEmulator * emu) noexcept;
terminal_emulator_delete = lib.terminal_emulator_delete
terminal_emulator_delete.argtypes = [c_void_p]
terminal_emulator_delete.restype = c_int

# END ctor/dtor
# BEGIN log
# str is zero-terminated
TerminalEmulatorLogFunction = CFUNCTYPE(None, c_char_p, c_size_t)

TerminalEmulatorLogFunctionCtx = CFUNCTYPE(None, c_void_p, c_char_p, c_size_t)

# int terminal_emulator_set_log_function(
#     TerminalEmulator * emu, TerminalEmulatorLogFunction * func) noexcept;
terminal_emulator_set_log_function = lib.terminal_emulator_set_log_function
terminal_emulator_set_log_function.argtypes = [c_void_p, c_void_p]
terminal_emulator_set_log_function.restype = c_int

# int terminal_emulator_set_log_function_ctx(
#     TerminalEmulator * emu, TerminalEmulatorLogFunctionCtx * func, void * ctx) noexcept;
terminal_emulator_set_log_function_ctx = lib.terminal_emulator_set_log_function_ctx
terminal_emulator_set_log_function_ctx.argtypes = [c_void_p, c_void_p, c_void_p]
terminal_emulator_set_log_function_ctx.restype = c_int

# END log
# BEGIN emulator
# int terminal_emulator_set_title(TerminalEmulator * emu, char const * title) noexcept;
terminal_emulator_set_title = lib.terminal_emulator_set_title
terminal_emulator_set_title.argtypes = [c_void_p, c_char_p]
terminal_emulator_set_title.restype = c_int

# int terminal_emulator_feed(TerminalEmulator * emu, uint8_t const * s, std::size_t len) noexcept;
terminal_emulator_feed = lib.terminal_emulator_feed
terminal_emulator_feed.argtypes = [c_void_p, POINTER(c_char), c_size_t]
terminal_emulator_feed.restype = c_int

# int terminal_emulator_finish(TerminalEmulator *) noexcept;
# int terminal_emulator_resize(TerminalEmulator * emu, int lines, int columns) noexcept;
terminal_emulator_resize = lib.terminal_emulator_resize
terminal_emulator_resize.argtypes = [c_void_p, c_int, c_int]
terminal_emulator_resize.restype = c_int

# END emulator
# BEGIN buffer
TerminalEmulatorBufferGetBufferFn = CFUNCTYPE(c_void_p, c_void_p, POINTER(c_size_t))

# \param extra_capacity_in_out  bytes required
# \param p                      original pointer of previous allocation (nullptr when no allocation)
# \param used_size              bytes consumed on \c p pointer
# \return nullptr when memory allocation error
TerminalEmulatorBufferExtraMemoryAllocatorFn = CFUNCTYPE(c_void_p, c_void_p, POINTER(c_size_t), POINTER(c_char), c_size_t)

TerminalEmulatorBufferSetFinalBufferFn = CFUNCTYPE(None, c_void_p, POINTER(c_char), c_size_t)

TerminalEmulatorBufferClearFn = CFUNCTYPE(None, c_void_p)

TerminalEmulatorBufferDeleteCtxFn = CFUNCTYPE(None, c_void_p)

# TerminalEmulatorBuffer * terminal_emulator_buffer_new() noexcept;
terminal_emulator_buffer_new = lib.terminal_emulator_buffer_new
terminal_emulator_buffer_new.argtypes = []
terminal_emulator_buffer_new.restype = c_void_p

# TerminalEmulatorBuffer * terminal_emulator_buffer_new_with_max_capacity(
#     std::size_t max_capacity, std::size_t pre_alloc_len) noexcept;
terminal_emulator_buffer_new_with_max_capacity = lib.terminal_emulator_buffer_new_with_max_capacity
terminal_emulator_buffer_new_with_max_capacity.argtypes = [c_size_t, c_size_t]
terminal_emulator_buffer_new_with_max_capacity.restype = c_void_p

# TerminalEmulatorBuffer * terminal_emulator_buffer_new_with_custom_allocator(
#     void * ctx,
#     TerminalEmulatorBufferGetBufferFn * get_buffer_fn,
#     TerminalEmulatorBufferExtraMemoryAllocatorFn * extra_memory_allocator_fn,
#     TerminalEmulatorBufferSetFinalBufferFn * set_final_buffer_fn,
#     TerminalEmulatorBufferClearFn * clear_fn,
#     TerminalEmulatorBufferDeleteCtxFn * delete_ctx_fn) noexcept;
terminal_emulator_buffer_new_with_custom_allocator = lib.terminal_emulator_buffer_new_with_custom_allocator
terminal_emulator_buffer_new_with_custom_allocator.argtypes = [c_void_p, c_void_p, c_void_p, c_void_p, c_void_p, c_void_p]
terminal_emulator_buffer_new_with_custom_allocator.restype = c_void_p

# int terminal_emulator_buffer_delete(TerminalEmulatorBuffer * buffer) noexcept;
terminal_emulator_buffer_delete = lib.terminal_emulator_buffer_delete
terminal_emulator_buffer_delete.argtypes = [c_void_p]
terminal_emulator_buffer_delete.restype = c_int

# int terminal_emulator_buffer_prepare(
#     TerminalEmulatorBuffer * buffer, TerminalEmulator * emu,
#     TerminalEmulatorOutputFormat format) noexcept;
terminal_emulator_buffer_prepare = lib.terminal_emulator_buffer_prepare
terminal_emulator_buffer_prepare.argtypes = [c_void_p, c_void_p, c_int]
terminal_emulator_buffer_prepare.restype = c_int

# int terminal_emulator_buffer_prepare2(
#     TerminalEmulatorBuffer * buffer, TerminalEmulator * emu,
#     TerminalEmulatorOutputFormat format, uint8_t const * extra_data,
#     std::size_t extra_data_len) noexcept;
terminal_emulator_buffer_prepare2 = lib.terminal_emulator_buffer_prepare2
terminal_emulator_buffer_prepare2.argtypes = [c_void_p, c_void_p, c_int, POINTER(c_char), c_size_t]
terminal_emulator_buffer_prepare2.restype = c_int

# uint8_t const * terminal_emulator_buffer_get_data(
#     TerminalEmulatorBuffer const * buffer, std::size_t * output_len) noexcept;
terminal_emulator_buffer_get_data = lib.terminal_emulator_buffer_get_data
terminal_emulator_buffer_get_data.argtypes = [c_void_p, POINTER(c_size_t)]
terminal_emulator_buffer_get_data.restype = POINTER(c_char)

# int terminal_emulator_buffer_clear_data(TerminalEmulatorBuffer *) noexcept;
# END buffer
# BEGIN read
# Construct a transcript buffer of session recorded by ttyrec.
# int terminal_emulator_buffer_prepare_transcript_from_ttyrec(
#     TerminalEmulatorBuffer * buffer,
#     char const * infile,
#     TerminalEmulatorTranscriptPrefix prefix_type) noexcept;
terminal_emulator_buffer_prepare_transcript_from_ttyrec = lib.terminal_emulator_buffer_prepare_transcript_from_ttyrec
terminal_emulator_buffer_prepare_transcript_from_ttyrec.argtypes = [c_void_p, c_char_p, c_int]
terminal_emulator_buffer_prepare_transcript_from_ttyrec.restype = c_int

# END read
# BEGIN write
# int terminal_emulator_buffer_write(
#     TerminalEmulatorBuffer const * buffer, char const * filename,
#     int mode, TerminalEmulatorCreateFileMode create_mode) noexcept;
terminal_emulator_buffer_write = lib.terminal_emulator_buffer_write
terminal_emulator_buffer_write.argtypes = [c_void_p, c_char_p, c_int, c_int]
terminal_emulator_buffer_write.restype = c_int

# int terminal_emulator_buffer_write_integrity(
#     TerminalEmulatorBuffer const * buffer, char const * filename,
#     char const * prefix_tmp_filename, int mode) noexcept;
terminal_emulator_buffer_write_integrity = lib.terminal_emulator_buffer_write_integrity
terminal_emulator_buffer_write_integrity.argtypes = [c_void_p, c_char_p, c_char_p, c_int]
terminal_emulator_buffer_write_integrity.restype = c_int

# Generate a transcript file of session recorded by ttyrec.
# \param outfile  output file when not null, otherwise stdout
# int terminal_emulator_transcript_from_ttyrec(
#     char const * infile, char const * outfile, int mode,
#     TerminalEmulatorCreateFileMode create_mode,
#     TerminalEmulatorTranscriptPrefix prefix_type) noexcept;
terminal_emulator_transcript_from_ttyrec = lib.terminal_emulator_transcript_from_ttyrec
terminal_emulator_transcript_from_ttyrec.argtypes = [c_char_p, c_char_p, c_int, c_int, c_int]
terminal_emulator_transcript_from_ttyrec.restype = c_int

# END write
# @}
