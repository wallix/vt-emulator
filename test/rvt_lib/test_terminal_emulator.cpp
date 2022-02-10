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
#include "utils/sugar/bytes_t.hpp"

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


using OutputFormat = TerminalEmulatorOutputFormat;
using CreateFileMode = TerminalEmulatorCreateFileMode;
using TranscriptPrefix = TerminalEmulatorTranscriptPrefix;

constexpr auto force_create = CreateFileMode::force_create;

template<>
struct std::default_delete<TerminalEmulator>
{
    void operator()(TerminalEmulator * p) noexcept
    { BOOST_CHECK_EQUAL(0, terminal_emulator_delete(p)); }
};


template<>
struct std::default_delete<TerminalEmulatorBuffer>
{
    void operator()(TerminalEmulatorBuffer * p) noexcept
    { BOOST_CHECK_EQUAL(0, terminal_emulator_buffer_delete(p)); }
};

static uint8_t const* to_u8p(char const* p) noexcept
{
    return const_bytes_t(p).to_u8p();
}

static std::string_view get_data(TerminalEmulatorBuffer * buf)
{
    std::size_t len = 0;
    uint8_t const* data = terminal_emulator_buffer_get_data(buf, &len);
    return {const_bytes_t(data).to_charp(), len};
}

BOOST_AUTO_TEST_CASE(TestTermEmu)
{
    std::unique_ptr<TerminalEmulator> uemu{terminal_emulator_new(3, 10)};
    std::unique_ptr<TerminalEmulatorBuffer> uemubuf{terminal_emulator_buffer_new()};
    auto emu = uemu.get();
    auto emubuf = uemubuf.get();

    static std::string global_s;

    BOOST_CHECK(emu);
    BOOST_CHECK(emubuf);
    BOOST_CHECK_EQUAL(0, terminal_emulator_set_log_function(emu, [](char const * s, std::size_t n) { global_s.assign(s, n); }));
    BOOST_CHECK_EQUAL(global_s, "");
    BOOST_CHECK_EQUAL(0, terminal_emulator_feed(emu, to_u8p("\033[324a"), 6));
    BOOST_CHECK_EQUAL(global_s, "Undecodable sequence: \\x1b[324a");
    int ctx_i = 1;
    BOOST_CHECK_EQUAL(0, terminal_emulator_set_log_function_ctx(emu, [](void * ctx, char const *, std::size_t) { *static_cast<int*>(ctx) = 3; }, &ctx_i));
    BOOST_CHECK_EQUAL(ctx_i, 1);
    BOOST_CHECK_EQUAL(0, terminal_emulator_feed(emu, to_u8p("\033[324a"), 6));
    BOOST_CHECK_EQUAL(ctx_i, 3);

    BOOST_CHECK_EQUAL(0, terminal_emulator_set_title(emu, "Lib test"));
    BOOST_CHECK_EQUAL(0, terminal_emulator_feed(emu, to_u8p("ABC"), 3));

    char const * filename = "/tmp/termemu-test.json";

    std::string_view contents = R"xxx({"x":3,"y":0,"lines":3,"columns":10,"title":"Lib test","style":{"r":0,"f":16777215,"b":0},"data":[[[{"s":"ABC"}]],[[{}]],[[{}]]]})xxx";

    BOOST_CHECK_EQUAL(0, terminal_emulator_buffer_prepare(emubuf, emu, OutputFormat::json));
    BOOST_CHECK_EQUAL(contents.size(), get_data(emubuf).size());
    BOOST_CHECK_EQUAL(contents, get_data(emubuf));

    BOOST_CHECK_EQUAL(0, terminal_emulator_buffer_write_integrity(emubuf, filename, filename, 0664));
    BOOST_CHECK_EQUAL(contents, get_file_contents(filename));
    BOOST_CHECK_EQUAL(0, unlink(filename));

    BOOST_CHECK_EQUAL(0, terminal_emulator_buffer_write(emubuf, filename, 0664, force_create));
    BOOST_CHECK_EQUAL(contents, get_file_contents(filename));
    BOOST_CHECK_EQUAL(0, unlink(filename));


    BOOST_CHECK_EQUAL(contents.size(), get_data(emubuf).size());
    BOOST_CHECK_EQUAL(contents, get_data(emubuf));


    BOOST_CHECK_EQUAL(0, terminal_emulator_resize(emu, 2, 2));

    contents = R"xxx({"x":1,"y":0,"lines":2,"columns":2,"title":"Lib test","style":{"r":0,"f":16777215,"b":0},"data":[[[{"s":"AB"}]],[[{}]]]})xxx";

    BOOST_CHECK_EQUAL(0, terminal_emulator_buffer_prepare(emubuf, emu, OutputFormat::json));
    BOOST_CHECK_EQUAL(contents.size(), get_data(emubuf).size());
    BOOST_CHECK_EQUAL(contents, get_data(emubuf));

    contents = R"xxx({"x":1,"y":0,"lines":2,"columns":2,"title":"Lib test","style":{"r":0,"f":16777215,"b":0},"data":[[[{"s":"AB"}]],[[{}]]],"extra":"plop"})xxx";

    BOOST_CHECK_EQUAL(0, terminal_emulator_buffer_prepare2(emubuf, emu, OutputFormat::json, to_u8p("\"plop\""), 6));
    BOOST_CHECK_EQUAL(contents.size(), get_data(emubuf).size());
    BOOST_CHECK_EQUAL(contents, get_data(emubuf));

    contents = R"xxx({"y":-1,"lines":2,"columns":2,"title":"Lib test","style":{"r":0,"f":16777215,"b":0},"data":[[[{"s":"AB"}]],[[{}]]]})xxx";

    BOOST_CHECK_EQUAL(0, terminal_emulator_feed(emu, to_u8p("\033[?25l"), 6));
    BOOST_CHECK_EQUAL(0, terminal_emulator_buffer_prepare(emubuf, emu, OutputFormat::json));
    BOOST_CHECK_EQUAL(contents.size(), get_data(emubuf).size());
    BOOST_CHECK_EQUAL(contents, get_data(emubuf));


    // check error code
    BOOST_CHECK_EQUAL(-2, terminal_emulator_set_log_function_ctx(nullptr, [](void *, char const *, std::size_t) {}, nullptr));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_set_log_function(nullptr, [](char const *, std::size_t) {}));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_set_title(nullptr, "Lib test"));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_feed(nullptr, to_u8p("\033[324a"), 6));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_buffer_write(nullptr, filename, 0664, force_create));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_buffer_write_integrity(nullptr, filename, filename, 0664));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_buffer_prepare(nullptr, emu, OutputFormat::json));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_buffer_prepare(emubuf, nullptr, OutputFormat::json));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_buffer_prepare(nullptr, nullptr, OutputFormat::json));
    BOOST_CHECK_NE(nullptr, terminal_emulator_buffer_get_data(emubuf, nullptr));
    BOOST_CHECK_EQUAL(nullptr, terminal_emulator_buffer_get_data(nullptr, nullptr));
    std::size_t len{};
    BOOST_CHECK_EQUAL(nullptr, terminal_emulator_buffer_get_data(nullptr, &len));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_buffer_clear_data(nullptr));
    BOOST_CHECK_EQUAL(-2, terminal_emulator_resize(emu, -3, 3));
    const unsigned very_big_size = (~0u>>1) - 1u; // -1u for inhibit integer overflow (uint -> int)
    BOOST_CHECK_EQUAL(ENOMEM, terminal_emulator_resize(emu, very_big_size, very_big_size)); // bad alloc

    BOOST_CHECK_LT(0, terminal_emulator_buffer_write_integrity(emubuf, "/a/a", filename, 0664));
    BOOST_CHECK_LT(0, terminal_emulator_buffer_write_integrity(emubuf, filename, "/a/a", 0664));
    BOOST_CHECK_LT(0, terminal_emulator_buffer_write(emubuf, "/a/a", 0664, force_create));
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
