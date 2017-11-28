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

#define BOOST_TEST_MODULE VtEmulator
#include "system/redemption_unit_tests.hpp"

#include "rvt/vt_emulator.hpp"
#include "rvt/utf8_decoder.hpp"
#include "rvt/text_rendering.hpp"

#include <fstream>

namespace rvt {

inline std::ostream & operator<<(std::ostream & out, CharacterColor const & ch_color)
{
    auto color = ch_color.color(rvt::color_table);
    return out << "Color("
        << color.red()+0 << ", "
        << color.green()+0 << ", "
        << color.blue()+0 << ")"
    ;
}

inline std::ostream & operator<<(std::ostream & out, Character const & ch)
{
    return out << "Ch("
        << ch.character << ", "
        << ch.foregroundColor << ", "
        << ch.backgroundColor << ", "
        << underlying_cast(ch.rendition)+0 << ", "
        << ch.isRealCharacter << ")"
    ;
}

}

BOOST_AUTO_TEST_CASE(TestEmulator)
{
    rvt::VtEmulator emulator(7, 20);
    rvt::Utf8Decoder text_decoder;

    auto send_ucs = [&emulator](rvt::ucs4_char ucs) { emulator.receiveChar(ucs); };
    auto send_zstring = [&emulator, &text_decoder, send_ucs](array_view_const_char av) {
        text_decoder.decode(av.first(av.size()-1), send_ucs);
    };

    for (int i = 0; i < 10; ++i) {
        send_zstring("abc");
    }
    send_zstring("\033[0B\033[31m");
    for (int i = 0; i < 10; ++i) {
        send_zstring("abc");
    }
    send_zstring("\033[44mée\xea\xb0\x80");
    text_decoder.end_decode(send_ucs);

    BOOST_CHECK_EQUAL_RANGES(emulator.getWindowTitle(), array_view<rvt::ucs4_char>());
    send_zstring("\033]2;abc\a");
    BOOST_CHECK_EQUAL_RANGES(emulator.getWindowTitle(), cstr_array_view("abc"));
    send_zstring("\033]0;abcd\a");
    BOOST_CHECK_EQUAL_RANGES(emulator.getWindowTitle(), cstr_array_view("abcd"));

    rvt::Screen const & screen = emulator.getCurrentScreen();
    auto const & lines = screen.getScreenLines();

    BOOST_REQUIRE_EQUAL(lines.size(), 7);

    rvt::Character a_ch('a');
    rvt::Character b_ch('b');
    rvt::Character c_ch('c');

    BOOST_CHECK_EQUAL(lines[0].size(), 20);
    BOOST_CHECK_EQUAL(lines[0][0], a_ch);
    BOOST_CHECK_EQUAL(lines[0][1], b_ch);
    BOOST_CHECK_EQUAL(lines[0][2], c_ch);
    BOOST_CHECK_EQUAL(lines[0][3], a_ch);
    BOOST_CHECK_EQUAL(lines[0][4], b_ch);
    BOOST_CHECK_EQUAL(lines[0][5], c_ch);
    BOOST_CHECK_EQUAL(lines[1].size(), 10);
    BOOST_CHECK_EQUAL(lines[1][0], c_ch);
    BOOST_CHECK_EQUAL(lines[1][1], a_ch);
    BOOST_CHECK_EQUAL(lines[1][2], b_ch);
    BOOST_CHECK_EQUAL(lines[1][3], c_ch);
    BOOST_CHECK_EQUAL(lines[1][4], a_ch);

    rvt::CharacterColor fg(rvt::ColorSpace::System, 1);
    a_ch.foregroundColor = fg;
    b_ch.foregroundColor = fg;
    c_ch.foregroundColor = fg;
    rvt::Character no_ch;

    BOOST_CHECK_EQUAL(lines[2].size(), 20);
    BOOST_CHECK_EQUAL(lines[2][0], no_ch);
    BOOST_CHECK_EQUAL(lines[2][1], no_ch);
    BOOST_CHECK_EQUAL(lines[2][10], a_ch);
    BOOST_CHECK_EQUAL(lines[2][11], b_ch);
    BOOST_CHECK_EQUAL(lines[3].size(), 20);
    BOOST_CHECK_EQUAL(lines[3][0], b_ch);
    BOOST_CHECK_EQUAL(lines[3][1], c_ch);

    rvt::CharacterColor bg(rvt::ColorSpace::System, 4);
    rvt::Character ch(0, fg, bg);
    rvt::Character no_real(0, fg, bg); no_real.isRealCharacter = false;

    BOOST_CHECK_EQUAL(lines[4].size(), 4);
    BOOST_CHECK_EQUAL(lines[4][0], rvt::Character(233, fg, bg));
    BOOST_CHECK_EQUAL(lines[4][1], rvt::Character('e', fg, bg));
    BOOST_CHECK_EQUAL(lines[4][2], rvt::Character(44032, fg, bg));
    BOOST_CHECK_EQUAL(lines[4][3], no_real);

    send_zstring("e");
    text_decoder.end_decode(send_ucs);
    BOOST_CHECK_EQUAL(lines[4].size(), 5);
    BOOST_CHECK_EQUAL(lines[4][3], no_real);
    BOOST_CHECK_EQUAL(lines[4][4], rvt::Character('e', fg, bg));
    emulator.receiveChar(0x311);
    BOOST_CHECK_EQUAL(lines[4][4], rvt::Character(0, fg, bg, rvt::Rendition::ExtendedChar));
    BOOST_CHECK_EQUAL(screen.extendedCharTable().size(), 1);
    BOOST_CHECK_EQUAL_RANGES(screen.extendedCharTable()[0], utils::make_array<rvt::ucs4_char>('e', 0x311));

    BOOST_CHECK_EQUAL(lines[5].size(), 0);
}

BOOST_AUTO_TEST_CASE(TestEmulatorReplay1)
{
    rvt::VtEmulator emulator(57, 104);
    rvt::Utf8Decoder text_decoder;
    std::filebuf in;
    in.open("test/data/typescript1", std::ios::in);

    char buf[4096];
    std::streamsize len;
    while ((len = in.sgetn(buf, sizeof(buf)))) {
        text_decoder.decode({buf, buf+len}, [&emulator](rvt::ucs4_char ucs) {
            emulator.receiveChar(ucs);
        });
    }

    auto s = ansi_rendering(
        {},
        emulator.getCurrentScreen(),
        rvt::color_table,
        nullptr
    );

    BOOST_CHECK_EQUAL(s.size(), 4327u);
    BOOST_CHECK_EQUAL(s, u8""
        "\033]\a│       ├── \033[0;4;38;2;95;135;215mcxx\n"
        "\033[0;38;2;238;238;238m│       │   ├── \033[0;38;2;95;215;255mattributes.hpp\n"
        "\033[0;38;2;238;238;238m│       │   ├── \033[0;38;2;95;215;255mdiagnostic.hpp\n"
        "\033[0;38;2;238;238;238m│       │   ├── \033[0;38;2;95;215;255mfeatures.hpp\n"
        "\033[0;38;2;238;238;238m│       │   └── \033[0;38;2;95;215;255mkeyword.hpp\n"
        "\033[0;38;2;238;238;238m│       ├── \033[0;4;38;2;95;135;215msystem\n"
        "\033[0;38;2;238;238;238m│       │   └── \033[0;4;38;2;95;135;215mlinux\n"
        "\033[0;38;2;238;238;238m│       │       └── \033[0;4;38;2;95;135;215msystem\n"
        "\033[0;38;2;238;238;238m│       │           └── \033[0;38;2;95;215;255mredemption_unit_tests.hpp\n"
        "\033[0;38;2;238;238;238m│       └── \033[0;4;38;2;95;135;215mutils\n"
        "\033[0;38;2;238;238;238m│           └── \033[0;4;38;2;95;135;215msugar\n"
        "\033[0;38;2;238;238;238m│               ├── \033[0;38;2;95;215;255marray.hpp\n"
        "\033[0;38;2;238;238;238m│               ├── \033[0;38;2;95;215;255marray_view.hpp\n"
        "\033[0;38;2;238;238;238m│               ├── \033[0;38;2;95;215;255mbytes_t.hpp\n"
        "\033[0;38;2;238;238;238m│               ├── \033[0;38;2;95;215;255menum_flags_operators.hpp\n"
        "\033[0;38;2;238;238;238m│               └── \033[0;38;2;95;215;255munderlying_cast.hpp\n"
        "\033[0;38;2;238;238;238m├── \033[0;4;38;2;95;135;215msrc\n"
        "\033[0;38;2;238;238;238m│   ├── \033[0;4;38;2;95;135;215mrvt\n"
        "\033[0;38;2;238;238;238m│   │   ├── \033[0;38;2;95;215;255mcharacter_color.hpp\n"
        "\033[0;38;2;238;238;238m│   │   ├── \033[0;38;2;95;215;255mcharacter.hpp\n"
        "\033[0;38;2;238;238;238m│   │   ├── \033[0;38;2;95;215;255mchar_class.hpp\n"
        "\033[0;38;2;238;238;238m│   │   ├── \033[0;38;2;95;215;255mcharsets.hpp\n"
        "\033[0;38;2;238;238;238m│   │   ├── \033[0;38;2;95;215;255mcolor.hpp\n"
        "\033[0;38;2;238;238;238m│   │   ├── \033[0;38;2;175;135;0mscreen.cpp\n"
        "\033[0;38;2;238;238;238m│   │   ├── \033[0;38;2;95;215;255mscreen.hpp\n"
        "\033[0;38;2;238;238;238m│   │   ├── \033[0;38;2;175;135;0mtext_rendering.cpp\n"
        "\033[0;38;2;238;238;238m│   │   ├── \033[0;38;2;95;215;255mtext_rendering.hpp\n"
        "\033[0;38;2;238;238;238m│   │   ├── \033[0;38;2;95;215;255mucs.hpp\n"
        "\033[0;38;2;238;238;238m│   │   ├── \033[0;38;2;95;215;255mutf8_decoder.hpp\n"
        "\033[0;38;2;238;238;238m│   │   ├── \033[0;38;2;175;135;0mvt_emulator.cpp\n"
        "\033[0;38;2;238;238;238m│   │   └── \033[0;38;2;95;215;255mvt_emulator.hpp\n"
        "\033[0;38;2;238;238;238m│   └── \033[0;4;38;2;95;135;215mrvt_lib\n"
        "\033[0;38;2;238;238;238m│       ├── \033[0;38;2;175;135;0mterminal_emulator.cpp\n"
        "\033[0;38;2;238;238;238m│       ├── \033[0;38;2;95;215;255mterminal_emulator.hpp\n"
        "\033[0;38;2;238;238;238m│       └── \033[0;38;2;95;215;255mversion.hpp\n"
        "\033[0;38;2;238;238;238m├── \033[0;4;38;2;95;135;215mtest\n"
        "\033[0;38;2;238;238;238m│   ├── \033[0;4;38;2;95;135;215mrvt\n"
        "\033[0;38;2;238;238;238m│   │   ├── \033[0;38;2;175;135;0mtest_character_color.cpp\n"
        "\033[0;38;2;238;238;238m│   │   ├── \033[0;38;2;175;135;0mtest_character.cpp\n"
        "\033[0;38;2;238;238;238m│   │   ├── \033[0;38;2;175;135;0mtest_char_class.cpp\n"
        "\033[0;38;2;238;238;238m│   │   ├── \033[0;38;2;175;135;0mtest_screen.cpp\n"
        "\033[0;38;2;238;238;238m│   │   ├── \033[0;38;2;175;135;0mtest_utf8_decoder.cpp\n"
        "\033[0;38;2;238;238;238m│   │   ├── \033[0;38;2;175;135;0mtest_vt_emulator.cpp\n"
        "\033[0;38;2;238;238;238m│   │   └── typescript\n"
        "│   └── \033[0;4;38;2;95;135;215mrvt_lib\n"
        "\033[0;38;2;238;238;238m│       └── \033[0;38;2;175;135;0mtest_terminal_emulator.cpp\n"
        "\033[0;38;2;238;238;238m├── \033[0;4;38;2;95;135;215mtools\n"
        "\033[0;38;2;238;238;238m│   ├── \033[0;1;38;2;215;95;0mtagger.sh\n"
        "\033[0;38;2;238;238;238m│   └── \033[0;38;2;175;135;0mterminal_browser.cpp\n"
        "\033[0;38;2;238;238;238m├── typescript\n"
        "└── vt-emulator.kdev4\n"
        "\n"
        "23 directories, 57 files\n"
        "\033[0;38;2;178;104;24m[2]\033[0;38;2;24;178;178;48;2;0;0;0m~/projects/vt-emulator\033[0;38;2;238;238;238;48;2;51;51;51m!\033[0;1;38;2;104;104;104m4891\033[0;1;38;2;84;255;84m$\033[0;1;38;2;84;84;255m(master)\033[0;1;38;2;255;255;84m✗\033[0;38;2;238;238;238m                                         \033[0;38;2;178;24;24m~/projects/vt-emulator\n"
        "\n"
        "\033[0;38;2;238;238;238mScript done on 2017-11-28 11:33:08+0100\n"
        "\n");
}
