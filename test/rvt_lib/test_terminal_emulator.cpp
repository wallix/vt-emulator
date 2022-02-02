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

#define BOOST_TEST_MODULE LibEmulator
#include "system/redemption_unit_tests.hpp"

#include "rvt_lib/terminal_emulator.hpp"

#include <memory>
#include <iostream>
#include <fstream>

#include <cstring>
#include <cerrno>

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

constexpr auto force_create = CreateFileMode::force_create;

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
    BOOST_CHECK_EQUAL(0, terminal_emulator_set_log_function(emu, [](char const *, std::size_t) { global_i = 2; }));
    BOOST_CHECK_EQUAL(global_i, 1);
    BOOST_CHECK_EQUAL(0, terminal_emulator_feed(emu, "\033[324a", 6));
    BOOST_CHECK_EQUAL(global_i, 2);
    int ctx_i = 1;
    BOOST_CHECK_EQUAL(0, terminal_emulator_set_log_function_ctx(emu, [](void * ctx, char const *, std::size_t) { *static_cast<int*>(ctx) = 3; }, &ctx_i));
    BOOST_CHECK_EQUAL(ctx_i, 1);
    BOOST_CHECK_EQUAL(0, terminal_emulator_feed(emu, "\033[324a", 6));
    BOOST_CHECK_EQUAL(ctx_i, 3);

    BOOST_CHECK_EQUAL(0, terminal_emulator_set_title(emu, "Lib test"));
    BOOST_CHECK_EQUAL(0, terminal_emulator_feed(emu, "ABC", 3));

    char const * filename = "/tmp/termemu-test.json";

    char const * contents = R"xxx({"x":3,"y":0,"lines":3,"columns":10,"title":"Lib test","style":{"r":0,"f":16777215,"b":0},"data":[[[{"s":"ABC"}]],[[{}]],[[{}]]]})xxx";

    BOOST_CHECK_EQUAL(0, terminal_emulator_buffer_prepare(emu, OutputFormat::json, nullptr));
    BOOST_CHECK_EQUAL(strlen(contents), terminal_emulator_buffer_size(emu));
    BOOST_CHECK_EQUAL(contents, terminal_emulator_buffer_data(emu));

    BOOST_CHECK_EQUAL(0, terminal_emulator_write_buffer_integrity(emu, filename, filename, 0664));
    BOOST_CHECK_EQUAL(contents, get_file_contents(filename));
    BOOST_CHECK_EQUAL(0, unlink(filename));

    BOOST_CHECK_EQUAL(0, terminal_emulator_write_buffer(emu, filename, 0664, force_create));
    BOOST_CHECK_EQUAL(contents, get_file_contents(filename));
    BOOST_CHECK_EQUAL(0, unlink(filename));


    BOOST_CHECK_EQUAL(0, terminal_emulator_buffer_reset(emu));
    BOOST_CHECK_EQUAL(0, terminal_emulator_buffer_size(emu));
    BOOST_CHECK_EQUAL("", terminal_emulator_buffer_data(emu));


    BOOST_CHECK_EQUAL(0, terminal_emulator_resize(emu, 2, 2));

    contents = R"xxx({"x":1,"y":0,"lines":2,"columns":2,"title":"Lib test","style":{"r":0,"f":16777215,"b":0},"data":[[[{"s":"AB"}]],[[{}]]]})xxx";

    BOOST_CHECK_EQUAL(0, terminal_emulator_buffer_prepare2(emu, OutputFormat::json, nullptr, 0));
    BOOST_CHECK_EQUAL(strlen(contents), terminal_emulator_buffer_size(emu));
    BOOST_CHECK_EQUAL(contents, terminal_emulator_buffer_data(emu));

    contents = R"xxx({"x":1,"y":0,"lines":2,"columns":2,"title":"Lib test","style":{"r":0,"f":16777215,"b":0},"data":[[[{"s":"AB"}]],[[{}]]],"extra":"plop"})xxx";

    BOOST_CHECK_EQUAL(0, terminal_emulator_buffer_prepare(emu, OutputFormat::json, "\"plop\""));
    BOOST_CHECK_EQUAL(strlen(contents), terminal_emulator_buffer_size(emu));
    BOOST_CHECK_EQUAL(contents, terminal_emulator_buffer_data(emu));

    contents = R"xxx({"y":-1,"lines":2,"columns":2,"title":"Lib test","style":{"r":0,"f":16777215,"b":0},"data":[[[{"s":"AB"}]],[[{}]]]})xxx";

    BOOST_CHECK_EQUAL(0, terminal_emulator_feed(emu, "\033[?25l", 6));
    BOOST_CHECK_EQUAL(0, terminal_emulator_buffer_prepare(emu, OutputFormat::json, nullptr));
    BOOST_CHECK_EQUAL(strlen(contents), terminal_emulator_buffer_size(emu));
    BOOST_CHECK_EQUAL(contents, terminal_emulator_buffer_data(emu));


    // check error code
    BOOST_CHECK_EQUAL(-2, terminal_emulator_set_log_function_ctx(nullptr, [](void *, char const *, std::size_t) {}, nullptr));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_set_log_function(nullptr, [](char const *, std::size_t) {}));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_set_title(nullptr, "Lib test"));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_feed(nullptr, "\033[324a", 6));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_write_buffer(nullptr, filename, 0664, force_create));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_write_buffer_integrity(nullptr, filename, filename, 0664));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_buffer_prepare(nullptr, OutputFormat::json, nullptr));
    BOOST_CHECK_EQUAL("", terminal_emulator_buffer_data(nullptr));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_buffer_size(nullptr));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_buffer_reset(nullptr));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_resize(emu, -3, 3));
    const unsigned very_big_size = (~0u>>1) - 1u; // -1u for inhibit integer overflow (uint -> int)
    BOOST_CHECK_EQUAL(ENOMEM, terminal_emulator_resize(emu, very_big_size, very_big_size)); // bad alloc

    BOOST_CHECK_LT(0, terminal_emulator_write_buffer_integrity(emu, "/a/a", filename, 0664));
    BOOST_CHECK_LT(0, terminal_emulator_write_buffer_integrity(emu, filename, "/a/a", 0664));
    BOOST_CHECK_LT(0, terminal_emulator_write_buffer(emu, "/a/a", 0664, force_create));
}

BOOST_AUTO_TEST_CASE(TestEmulatorTranscript)
{
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);          // for localtime_r
    char const * outfile = "/tmp/emu_transcript.txt";
    BOOST_CHECK_EQUAL(ENOENT, terminal_emulator_transcript_from_ttyrec("aaa", outfile, 0664, force_create, TranscriptPrefix::datetime));
    BOOST_CHECK_EQUAL(0, terminal_emulator_transcript_from_ttyrec("test/data/ttyrec1", outfile, 0664, force_create, TranscriptPrefix::datetime));
    BOOST_CHECK_EQUAL(get_file_contents(outfile), ""
        "2017-11-29 17:29:05 [2]~/projects/vt-emulator!4902$(nomove)✗ l               ~/projects/vt-emulator\n"
        "2017-11-29 17:29:05 binding/  jam/     LICENSE   packaging/  redemption/  test/   typescript\n"
        "2017-11-29 17:29:05 browser/  Jamroot  out_text  README.md   src/         tools/  vt-emulator.kdev4\n"
        "2017-11-29 17:29:06 [2]~/projects/vt-emulator!4903$(nomove)✗                 ~/projects/vt-emulator\n");
    BOOST_CHECK_EQUAL(EEXIST, terminal_emulator_transcript_from_ttyrec("test/data/ttyrec1", outfile, 0664, CreateFileMode::fail_if_exists, TranscriptPrefix::datetime));
}

BOOST_AUTO_TEST_CASE(TestEmulatorTranscriptBigFile)
{
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);          // for localtime_r
    char const * outfile = "/tmp/emu_transcript.txt";
    BOOST_CHECK_EQUAL(0, terminal_emulator_transcript_from_ttyrec(
        "test/data/debian.ttyrec", outfile, 0664, force_create, TranscriptPrefix::datetime));
    auto const ref = get_file_contents("test/data/debian.transcript.txt");
    auto const gen = get_file_contents(outfile);
    auto hash = std::hash<std::string>{};
    BOOST_CHECK_EQUAL(hash(ref), hash(gen));
}
