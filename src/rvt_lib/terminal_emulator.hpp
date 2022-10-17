/*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*   Product name: redemption, a FLOSS RDP proxy
*   Copyright (C) Wallix 2010-2016
*   Author(s): Jonathan Poelen
*/

#pragma once

#include "cxx/cxx.hpp"

#include <cstdint>
#include <cstddef>

extern "C"
{

class TerminalEmulator;
class TerminalEmulatorBuffer;

enum class TerminalEmulatorOutputFormat : int {
    json,
    ansi
};

enum class TerminalEmulatorTranscriptPrefix : int {
    noprefix,
    datetime,
};

enum class TerminalEmulatorCreateFileMode : int {
    fail_if_exists,
    force_create,
};


/// \return  0 if success ; -3 for bad_alloc ; -2 if bad argument (emu is null, bad format, bad size, etc) ; -1 if internal error with `errno` code to 0 (bad alloc, etc) ; > 0 is an `errno` code,
//@{
REDEMPTION_LIB_EXPORT
char const * terminal_emulator_version() noexcept;

//BEGIN ctor/dtor
REDEMPTION_LIB_EXPORT
TerminalEmulator * terminal_emulator_new(int lines, int columns) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_delete(TerminalEmulator * emu) noexcept;
//END ctor/dtor

//BEGIN log
// str is zero-terminated
using TerminalEmulatorLogFunction = void(char const * str, std::size_t len);
using TerminalEmulatorLogFunctionCtx = void(void *, char const * str, std::size_t len);

REDEMPTION_LIB_EXPORT
int terminal_emulator_set_log_function(
    TerminalEmulator * emu, TerminalEmulatorLogFunction * func) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_set_log_function_ctx(
    TerminalEmulator * emu, TerminalEmulatorLogFunctionCtx * func, void * ctx) noexcept;
//END log

//BEGIN emulator
REDEMPTION_LIB_EXPORT
int terminal_emulator_set_title(TerminalEmulator * emu, char const * title) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_feed(TerminalEmulator * emu, uint8_t const * s, std::size_t len) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_finish(TerminalEmulator *) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_resize(TerminalEmulator * emu, int lines, int columns) noexcept;
//END emulator

//BEGIN buffer
using TerminalEmulatorBufferGetBufferFn
  = uint8_t*(void * ctx, std::size_t * output_len) noexcept;
/// \param extra_capacity_in_out  bytes required
/// \param p                      original pointer of previous allocation (nullptr when no allocation)
/// \param used_size              bytes consumed on \c p pointer
/// \return nullptr when memory allocation error
using TerminalEmulatorBufferExtraMemoryAllocatorFn
  = uint8_t*(void * ctx, std::size_t * extra_capacity_in_out, uint8_t * p, std::size_t used_size);
using TerminalEmulatorBufferSetFinalBufferFn
  = void(void * ctx, uint8_t * p, std::size_t used_size);
using TerminalEmulatorBufferClearFn = void(void* ctx) noexcept;
using TerminalEmulatorBufferDeleteCtxFn = void(void* ctx) noexcept;

REDEMPTION_LIB_EXPORT
TerminalEmulatorBuffer * terminal_emulator_buffer_new() noexcept;

REDEMPTION_LIB_EXPORT
TerminalEmulatorBuffer * terminal_emulator_buffer_new_with_max_capacity(
    std::size_t max_capacity, std::size_t pre_alloc_len) noexcept;

REDEMPTION_LIB_EXPORT
TerminalEmulatorBuffer * terminal_emulator_buffer_new_with_custom_allocator(
    void * ctx,
    TerminalEmulatorBufferGetBufferFn * get_buffer_fn,
    TerminalEmulatorBufferExtraMemoryAllocatorFn * extra_memory_allocator_fn,
    TerminalEmulatorBufferSetFinalBufferFn * set_final_buffer_fn,
    TerminalEmulatorBufferClearFn * clear_fn,
    TerminalEmulatorBufferDeleteCtxFn * delete_ctx_fn) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_delete(TerminalEmulatorBuffer * buffer) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_prepare(
    TerminalEmulatorBuffer * buffer, TerminalEmulator * emu,
    TerminalEmulatorOutputFormat format) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_prepare2(
    TerminalEmulatorBuffer * buffer, TerminalEmulator * emu,
    TerminalEmulatorOutputFormat format, uint8_t const * extra_data,
    std::size_t extra_data_len) noexcept;

REDEMPTION_LIB_EXPORT
uint8_t const * terminal_emulator_buffer_get_data(
    TerminalEmulatorBuffer const * buffer, std::size_t * output_len) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_clear_data(TerminalEmulatorBuffer *) noexcept;
//END buffer

//BEGIN read
/// Construct a transcript buffer of session recorded by ttyrec.
REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_prepare_transcript_from_ttyrec(
    TerminalEmulatorBuffer * buffer,
    char const * infile,
    TerminalEmulatorTranscriptPrefix prefix_type) noexcept;
//END read

//BEGIN write
REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_write(
    TerminalEmulatorBuffer const * buffer, char const * filename,
    int mode, TerminalEmulatorCreateFileMode create_mode) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_write_integrity(
    TerminalEmulatorBuffer const * buffer, char const * filename,
    char const * prefix_tmp_filename, int mode) noexcept;

/// Generate a transcript file of session recorded by ttyrec.
/// \param outfile  output file when not null, otherwise stdout
REDEMPTION_LIB_EXPORT
int terminal_emulator_transcript_from_ttyrec(
    char const * infile, char const * outfile, int mode,
    TerminalEmulatorCreateFileMode create_mode,
    TerminalEmulatorTranscriptPrefix prefix_type) noexcept;
//END write

//@}

}
