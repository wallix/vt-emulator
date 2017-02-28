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


#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestRect
#include "system/redemption_unit_tests.hpp"

#include "rvt_lib/terminal_emulator.hpp"

#include <memory>
#include <iostream>
#include <fstream>

#include <cstring>

#include <unistd.h>

inline std::string get_file_contents(const char * name)
{
    std::string s;
    std::filebuf buf;

    char c;
    buf.pubsetbuf(&c, 1);

    if (buf.open(name, std::ios::in)) {
        const std::streamsize sz = buf.in_avail();
        if (sz != std::streamsize(-1)) {
            s.resize(std::size_t(sz));
            const std::streamsize n = buf.sgetn(&s[0], std::streamsize(s.size()));
            if (sz != n) {
                s.resize(std::size_t(n));
            }
        }
    }

    return s;
}


using rvt_lib::TerminalEmulator;
using rvt_lib::OutputFormat;

struct TerminalEmulatorDeleter
{
    void operator()(TerminalEmulator * p) noexcept
    { BOOST_CHECK_EQUAL(0, terminal_emulator_deinit(p)); }
};

static int global_i = 1;

BOOST_AUTO_TEST_CASE(TestTermEmu)
{
    std::unique_ptr<TerminalEmulator, TerminalEmulatorDeleter> uemu{terminal_emulator_init(3, 10)};
    auto emu = uemu.get();

    BOOST_CHECK(emu);
    BOOST_CHECK_EQUAL(0, terminal_emulator_set_log_function(emu, [](char const *) { global_i = 2; }));
    BOOST_CHECK_EQUAL(global_i, 1);
    BOOST_CHECK_EQUAL(0, terminal_emulator_feed(emu, "\033[324a", 6));
    BOOST_CHECK_EQUAL(global_i, 2);
    int ctx_i = 1;
    BOOST_CHECK_EQUAL(0, terminal_emulator_set_log_function_ctx(emu, [](void * ctx, char const *) { *static_cast<int*>(ctx) = 3; }, &ctx_i));
    BOOST_CHECK_EQUAL(ctx_i, 1);
    BOOST_CHECK_EQUAL(0, terminal_emulator_feed(emu, "\033[324a", 6));
    BOOST_CHECK_EQUAL(ctx_i, 3);

    BOOST_CHECK_EQUAL(0, terminal_emulator_set_title(emu, "Lib test"));
    BOOST_CHECK_EQUAL(0, terminal_emulator_feed(emu, "ABC", 3));

    char const * filename = "/tmp/termemu-test.json";

    char const * contents = R"xxx({"lines":3,"columns":10,"title":"Lib test","style":{"r":0,"f":15658734,"b":3355443},"data":[[[{"s":"ABC"}]],[[{}]],[[{}]]]})xxx";

    BOOST_CHECK_EQUAL(0, terminal_emulator_write_integrity(emu, OutputFormat::json, filename, filename, 0664));
    BOOST_CHECK_EQUAL(contents, get_file_contents(filename));
    BOOST_CHECK_EQUAL(0, unlink(filename));

    BOOST_CHECK_EQUAL(0, terminal_emulator_write(emu, OutputFormat::json, filename, 0664));
    BOOST_CHECK_EQUAL(contents, get_file_contents(filename));
    BOOST_CHECK_EQUAL(0, unlink(filename));


    BOOST_CHECK_EQUAL(strlen(contents), terminal_emulator_buffer_size(emu));
    BOOST_CHECK_EQUAL(contents, terminal_emulator_buffer_data(emu));
    BOOST_CHECK_EQUAL(0, terminal_emulator_buffer_reset(emu));
    BOOST_CHECK_EQUAL(0, terminal_emulator_buffer_size(emu));
    BOOST_CHECK_EQUAL("", terminal_emulator_buffer_data(emu));
    BOOST_CHECK_EQUAL(0, terminal_emulator_write_in_buffer(emu, OutputFormat::json));
    BOOST_CHECK_EQUAL(strlen(contents), terminal_emulator_buffer_size(emu));
    BOOST_CHECK_EQUAL(contents, terminal_emulator_buffer_data(emu));


    BOOST_CHECK_EQUAL(0, terminal_emulator_resize(emu, 2, 2));

    contents = R"xxx({"lines":2,"columns":2,"title":"Lib test","style":{"r":0,"f":15658734,"b":3355443},"data":[[[{"s":"AB"}]],[[{}]]]})xxx";

    BOOST_CHECK_EQUAL(0, terminal_emulator_write_in_buffer(emu, OutputFormat::json));
    BOOST_CHECK_EQUAL(strlen(contents), terminal_emulator_buffer_size(emu));
    BOOST_CHECK_EQUAL(contents, terminal_emulator_buffer_data(emu));


    // check error code
    BOOST_CHECK_EQUAL(-2, terminal_emulator_set_log_function_ctx(nullptr, [](void *, char const *) {}, nullptr));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_set_log_function(nullptr, [](char const *) {}));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_set_title(nullptr, "Lib test"));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_feed(nullptr, "\033[324a", 6));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_write(nullptr, OutputFormat::json, filename, 0664));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_write_integrity(nullptr, OutputFormat::json, filename, filename, 0664));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_write_in_buffer(nullptr, OutputFormat::json));
    BOOST_CHECK_EQUAL("", terminal_emulator_buffer_data(nullptr));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_buffer_size(nullptr));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_buffer_reset(nullptr));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_resize(emu, -3, 3));
    BOOST_CHECK_EQUAL(-1, terminal_emulator_resize(emu, ~0u>>1, ~0u>>1)); // bad alloc

    BOOST_CHECK(0 < terminal_emulator_write_integrity(emu, OutputFormat::json, "/a/a", filename, 0664));
    BOOST_CHECK(0 < terminal_emulator_write_integrity(emu, OutputFormat::json, filename, "/a/a", 0664));
    BOOST_CHECK(0 < terminal_emulator_write(emu, OutputFormat::json, "/a/a", 0664));
}
