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
#include <stdlib.h> // mkostemps
#include <fcntl.h> // O_* flags

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

static bool write_all(int fd, const void * data, size_t len) noexcept
{
    size_t remaining_len = len;
    size_t total_sent = 0;
    while (remaining_len) {
        ssize_t ret = ::write(fd, static_cast<const char*>(data) + total_sent, remaining_len);
        if (ret <= 0){
            if (errno == EINTR){
                continue;
            }
            return false;
        }
        remaining_len -= ret;
        total_sent += ret;
    }
    return true;
}

static bool write_all(int fd, TerminalEmulatorBuffer const * buffer) noexcept
{
    std::size_t len = 0;
    uint8_t const* data = terminal_emulator_buffer_get_data(buffer, &len);
    return write_all(fd, data, len);
}

int terminal_emulator_buffer_write_integrity(
    TerminalEmulatorBuffer const * buffer, char const * filename,
    char const * prefix_tmp_filename
) noexcept
{
    char tmpfilename[4096];
    tmpfilename[0] = 0;
    if (prefix_tmp_filename == nullptr) {
        prefix_tmp_filename = filename;
    }
    int n = std::snprintf(tmpfilename, sizeof(tmpfilename) - 1, "%s-teremu-XXXXXX.tmp", prefix_tmp_filename);
    tmpfilename[n < 0 ? 0 : n] = 0;

    const int fd = ::mkostemps(tmpfilename, 4, O_WRONLY | O_CREAT);
    if (fd == -1) {
        return -1;
    }

    if (!write_all(fd, buffer) || rename(tmpfilename, filename) == -1) {
        auto const err = errno;
        close(fd);
        unlink(tmpfilename);
        return err ? err : -1;
    }

    close(fd);

    return 0;
}

} // anonymous namespace

using OutputFormat = TerminalEmulatorOutputFormat;

struct TerminalEmulatorDeleter
{
    void operator()(TerminalEmulator * p) noexcept
    {
        terminal_emulator_delete(p);
    }
};

struct TerminalEmulatorBufferDeleter
{
    void operator()(TerminalEmulator * p) noexcept
    {
        terminal_emulator_delete(p);
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

    auto emu = terminal_emulator_new(cli.lines, cli.columns);
    auto emu_buffer = terminal_emulator_buffer_new();

    terminal_emulator_set_title(emu, "No title");
    terminal_emulator_set_log_function(emu, [](char const * s, std::size_t /*len*/) {
        puts(s);
    });

    #define PError(x) do { if (int err = (x))                        \
        fprintf(stderr, "internal error: %s on " #x, strerror(err)); \
    } while (0)

    constexpr std::size_t input_buf_len = 4096;
    uint8_t input_buf[input_buf_len];
    ssize_t result;
    while ((result = read(0, input_buf, input_buf_len)) > 0)
    {
        PError(terminal_emulator_feed(emu, input_buf, std::size_t(result)));
        PError(terminal_emulator_buffer_prepare(
            emu_buffer, emu, TerminalEmulatorOutputFormat::json));
        PError(terminal_emulator_buffer_write_integrity(
            emu_buffer, cli.filename, cli.filename));
    }

    PError(terminal_emulator_finish(emu));
    PError(terminal_emulator_buffer_prepare(emu_buffer, emu, TerminalEmulatorOutputFormat::json));
    PError(terminal_emulator_buffer_write_integrity(emu_buffer, cli.filename, cli.filename));

    terminal_emulator_buffer_delete(emu_buffer);
    terminal_emulator_delete(emu);
}
