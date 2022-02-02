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
*   Author(s): Jonathan Poelen;
*
*   Based on Konsole, an X terminal
*/

#include "rvt_lib/terminal_emulator.hpp"

#include <string_view>
#include <charconv>
#include <memory>

#include <cstring>
#include <cstdio>

#include <unistd.h>

namespace
{

bool parse_size(std::string_view str, int& result_columns, int& result_lines)
{
    uint16_t lines = 0;
    uint16_t columns = 0;

    auto
    r = std::from_chars(str.begin(), str.end(), columns);
    if (r.ec != std::errc() || r.ptr == str.end())
    {
        return false;
    }

    r = std::from_chars(r.ptr+1, str.end(), lines);
    if (r.ec != std::errc() || r.ptr != str.end())
    {
        return false;
    }

    result_columns = static_cast<int>(columns);
    result_lines = static_cast<int>(lines);
    return true;
}

struct Cli
{
    int lines = 68;
    int columns = 117;
    char const* filename = "screen.json";

    bool parse(int ac, char ** av)
    {
        if (ac > 1)
        {
            std::string_view p1 = av[1];
            if (p1 == "-h" || p1 == "--help")
            {
                return false;
            }

            if ('0' <= av[1][0] && av[1][0] <= '9' && parse_size(p1, columns, lines))
            {
                if (ac > 2)
                {
                    filename = av[2];
                }
            }
            else
            {
                filename = av[1];
            }
        }

        return true;
    }
};

} // anonymous namespace

using rvt_lib::TerminalEmulator;
using rvt_lib::OutputFormat;

struct TerminalEmulatorDeleter
{
    void operator()(TerminalEmulator * p) noexcept
    {
        terminal_emulator_deinit(p);
    }
};

int main(int ac, char ** av)
{
    Cli cli;

    if (!cli.parse(ac, av))
    {
        printf("Usage: %s ${COLUMNS}x${LINES} [json_filename]\n", av[0]);
        return 0;
    }

    std::unique_ptr<TerminalEmulator, TerminalEmulatorDeleter> uptr{
        terminal_emulator_init(cli.lines, cli.columns)
    };

    auto emu = uptr.get();
    terminal_emulator_set_title(emu, "No title");
    terminal_emulator_set_log_function(emu, [](char const * s, std::size_t /*len*/) {
        puts(s);
    });

    #define PError(x) do { if (int err = (x))                        \
        fprintf(stderr, "internal error: %s on " #x, strerror(err)); \
    } while (0)

    for (;;)
    {
        constexpr std::size_t buflen = 4096;
        char buf[buflen];
        ssize_t result = read(0, buf, buflen);
        if (result > 0)
        {
            PError(terminal_emulator_feed(emu, buf, std::size_t(result)));
            PError(terminal_emulator_write_integrity(
                emu, OutputFormat::json, nullptr, cli.filename, cli.filename, 0660
            ));
        }
        else
        {
            PError(terminal_emulator_finish(emu));
            break;
        }
    }
}
