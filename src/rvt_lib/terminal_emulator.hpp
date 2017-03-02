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
}

/// \return  0 if success, -2 if bad argument (emu is null, bad format, bad size, etc), -1 if internal error (bad alloc, etc) and > 0 is an `errno` code
//@{
REDEMPTION_LIB_EXTERN char const * terminal_emulator_version() noexcept;
REDEMPTION_LIB_EXTERN rvt_lib::TerminalEmulator * terminal_emulator_init(int lines, int columns) noexcept;
REDEMPTION_LIB_EXTERN int terminal_emulator_deinit(rvt_lib::TerminalEmulator *) noexcept;

REDEMPTION_LIB_EXTERN int terminal_emulator_finish(rvt_lib::TerminalEmulator *) noexcept;

REDEMPTION_LIB_EXTERN int terminal_emulator_set_title(rvt_lib::TerminalEmulator *, char const * title) noexcept;
REDEMPTION_LIB_EXTERN int terminal_emulator_set_log_function(
    rvt_lib::TerminalEmulator *, void(* log_func)(char const *)) noexcept;
REDEMPTION_LIB_EXTERN int terminal_emulator_set_log_function_ctx(
    rvt_lib::TerminalEmulator *, void(* log_func)(void *, char const *), void * ctx) noexcept;

REDEMPTION_LIB_EXTERN int terminal_emulator_feed(rvt_lib::TerminalEmulator *, char const * s, int n) noexcept;
REDEMPTION_LIB_EXTERN int terminal_emulator_resize(rvt_lib::TerminalEmulator *, int lines, int columns) noexcept;

REDEMPTION_LIB_EXTERN int terminal_emulator_write(
    rvt_lib::TerminalEmulator *, rvt_lib::OutputFormat, char const * filename, int mode) noexcept;
REDEMPTION_LIB_EXTERN int terminal_emulator_write_integrity(
    rvt_lib::TerminalEmulator *, rvt_lib::OutputFormat,
    char const * filename, char const * prefix_tmp_filename, int mode) noexcept;
REDEMPTION_LIB_EXTERN int terminal_emulator_write_in_buffer(
    rvt_lib::TerminalEmulator *, rvt_lib::OutputFormat) noexcept;

REDEMPTION_LIB_EXTERN int terminal_emulator_buffer_size(rvt_lib::TerminalEmulator const *) noexcept;
REDEMPTION_LIB_EXTERN char const * terminal_emulator_buffer_data(rvt_lib::TerminalEmulator const *) noexcept;

REDEMPTION_LIB_EXTERN int terminal_emulator_buffer_reset(rvt_lib::TerminalEmulator *) noexcept;

//@}
