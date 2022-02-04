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

#include <cstddef>


namespace rvt_lib
{
    class TerminalEmulator;
    class TerminalEmulatorBuffer;

    enum class OutputFormat : int {
        json,
        ansi
    };

    enum class TranscriptPrefix : int {
        noprefix,
        datetime,
    };

    enum class CreateFileMode : int {
        fail_if_exists,
        force_create,
    };
}

using rvt_lib::TerminalEmulator;
using rvt_lib::TerminalEmulatorBuffer;
using rvt_lib::OutputFormat;
using rvt_lib::TranscriptPrefix;
using rvt_lib::CreateFileMode;

using BufferGetBufferFn = char*(void* ctx, std::size_t * output_len) noexcept;
using BufferExtraMemoryAllocatorFn = char*(void* ctx, std::size_t extra_capacity, char* p, std::size_t used_size);
using BufferSetFinalBufferFn = void(void* ctx, char* p, std::size_t used_size);
using BufferClearFn = void(void* ctx) noexcept;
using BufferDeleteCtxFn = void(void* ctx) noexcept;

/// \return  0 if success, -2 if bad argument (emu is null, bad format, bad size, etc), -1 if internal error with `errno` code to 0 (bad alloc, etc) and > 0 is an `errno` code
//@{
REDEMPTION_LIB_EXPORT
char const * terminal_emulator_version() noexcept;

//BEGIN ctor/dtor
REDEMPTION_LIB_EXPORT
TerminalEmulator * terminal_emulator_new(int lines, int columns) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_delete(TerminalEmulator *) noexcept;
//END ctor/dtor

//BEGIN log
using LogFunction = void(*)(char const *, std::size_t len);
using LogFunctionCtx = void(*)(void *, char const *, std::size_t len);

REDEMPTION_LIB_EXPORT
int terminal_emulator_set_log_function(
    TerminalEmulator *, LogFunction) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_set_log_function_ctx(
    TerminalEmulator *, LogFunctionCtx, void * ctx) noexcept;
//END log

//BEGIN emulator
REDEMPTION_LIB_EXPORT
int terminal_emulator_set_title(TerminalEmulator *, char const * title) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_feed(TerminalEmulator *, char const * s, std::size_t len) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_finish(TerminalEmulator *) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_resize(TerminalEmulator *, int lines, int columns) noexcept;
//END emulator

//BEGIN buffer
REDEMPTION_LIB_EXPORT
TerminalEmulatorBuffer * terminal_emulator_buffer_new() noexcept;

REDEMPTION_LIB_EXPORT
TerminalEmulatorBuffer * terminal_emulator_buffer_new_with_custom_allocator(
    void * ctx,
    BufferGetBufferFn * get_buffer_fn,
    BufferExtraMemoryAllocatorFn * extra_memory_allocator_fn,
    BufferSetFinalBufferFn * set_final_buffer_fn,
    BufferClearFn * clear_fn,
    BufferDeleteCtxFn * delete_ctx_fn) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_delete(TerminalEmulatorBuffer *) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_prepare(
    TerminalEmulatorBuffer *, TerminalEmulator *, OutputFormat) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_prepare2(
    TerminalEmulatorBuffer *, TerminalEmulator *,
    OutputFormat, char const * extra_data, std::size_t extra_data_len) noexcept;

REDEMPTION_LIB_EXPORT
char const * terminal_emulator_buffer_get_data(
    TerminalEmulatorBuffer const *, std::size_t * output_len) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_clear_data(TerminalEmulatorBuffer *) noexcept;
//END buffer

//BEGIN write
REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_write(
    TerminalEmulatorBuffer const *, char const * filename, int mode, CreateFileMode) noexcept;

REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_write_integrity(
    TerminalEmulatorBuffer const *, char const * filename,
    char const * prefix_tmp_filename, int mode) noexcept;

/// \brief Generate a transcript file of session recorded by ttyrec
/// \param outfile  output file when not null, otherwise stdout
REDEMPTION_LIB_EXPORT
int terminal_emulator_transcript_from_ttyrec(
    char const * infile, char const * outfile, int mode, CreateFileMode, TranscriptPrefix) noexcept;
//END write

//@}
