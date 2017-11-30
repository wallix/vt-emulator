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

#include "cxx/keyword.hpp"

namespace rvt_lib
{
    class TerminalEmulator;

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
using rvt_lib::OutputFormat;
using rvt_lib::TranscriptPrefix;
using rvt_lib::CreateFileMode;

/// \return  0 if success, -2 if bad argument (emu is null, bad format, bad size, etc), -1 if internal error with `errno` code to 0 (bad alloc, etc) and > 0 is an `errno` code
//@{
REDEMPTION_LIB_EXTERN char const * terminal_emulator_version() noexcept;

//BEGIN ctor/dtor
REDEMPTION_LIB_EXTERN TerminalEmulator * terminal_emulator_init(int lines, int columns) noexcept;
REDEMPTION_LIB_EXTERN int terminal_emulator_deinit(TerminalEmulator *) noexcept;
//END ctor/dtor

//BEGIN log
using LogFunction = void(*)(char const *);
using LogFunctionCtx = void(*)(void *, char const *);

REDEMPTION_LIB_EXTERN int terminal_emulator_set_log_function(
    TerminalEmulator *, LogFunction) noexcept;
REDEMPTION_LIB_EXTERN int terminal_emulator_set_log_function_ctx(
    TerminalEmulator *, LogFunctionCtx, void * ctx) noexcept;
//END log

//BEGIN emulator
REDEMPTION_LIB_EXTERN int terminal_emulator_set_title(TerminalEmulator *, char const * title) noexcept;
REDEMPTION_LIB_EXTERN int terminal_emulator_finish(TerminalEmulator *) noexcept;
REDEMPTION_LIB_EXTERN int terminal_emulator_feed(TerminalEmulator *, char const * s, int n) noexcept;
REDEMPTION_LIB_EXTERN int terminal_emulator_resize(TerminalEmulator *, int lines, int columns) noexcept;
//END emulator

//BEGIN buffer
REDEMPTION_LIB_EXTERN int terminal_emulator_buffer_prepare(
    TerminalEmulator *, OutputFormat, char const * extra_data) noexcept;
REDEMPTION_LIB_EXTERN int terminal_emulator_buffer_size(TerminalEmulator const *) noexcept;
REDEMPTION_LIB_EXTERN char const * terminal_emulator_buffer_data(TerminalEmulator const *) noexcept;
REDEMPTION_LIB_EXTERN int terminal_emulator_buffer_reset(TerminalEmulator *) noexcept;
//END buffer

//BEGIN write
REDEMPTION_LIB_EXTERN int terminal_emulator_write_buffer(
    TerminalEmulator const *, char const * filename, int mode, CreateFileMode) noexcept;
REDEMPTION_LIB_EXTERN int terminal_emulator_write_buffer_integrity(
    TerminalEmulator const *, char const * filename, char const * prefix_tmp_filename, int mode) noexcept;
/// \brief terminal_emulator_buffer_prepare then terminal_emulator_write_buffer
REDEMPTION_LIB_EXTERN int terminal_emulator_write(
    TerminalEmulator *, OutputFormat, char const * extra_data,
    char const * filename, int mode, CreateFileMode) noexcept;
/// \brief terminal_emulator_buffer_prepare then terminal_emulator_write_buffer_integrity
REDEMPTION_LIB_EXTERN int terminal_emulator_write_integrity(
    TerminalEmulator *, OutputFormat, char const * extra_data,
    char const * filename, char const * prefix_tmp_filename, int mode) noexcept;

/// \brief Generate a transcript file of session recorded by ttyrec
/// \param outfile  output file or stdout if empty/null
REDEMPTION_LIB_EXTERN int terminal_emulator_transcript_from_ttyrec(
    char const * infile, char const * outfile, int mode, CreateFileMode, TranscriptPrefix) noexcept;
//END write

//@}
