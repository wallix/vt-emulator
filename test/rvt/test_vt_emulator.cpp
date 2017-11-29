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
#include <iostream>

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
    std::string out;
    auto line_saver = [&out](rvt::Screen const& screen, array_view<const rvt::Character> line){
        for (auto const& ch : line) {
            char buf[4];
            if (ch.is_extended()) {
                for (auto ucs : screen.extendedCharTable()[ch.character]) {
                    out.append(buf, rvt::unsafe_ucs4_to_utf8(ucs, buf));
                }
            }
            else {
                out.append(buf, rvt::unsafe_ucs4_to_utf8(ch.character, buf));
            }
        }
        out += '\n';
    };
    rvt::VtEmulator emulator(57, 104, line_saver);
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

    BOOST_CHECK_EQUAL(out.size(), 3197u);
    BOOST_CHECK_EQUAL(out, u8""
        "Script started on 2017-11-28 11:32:57+0100\n"
        "                                          %"
        "                                                             \n"
        "[2]~/projects/vt-emulator!4890$(master)✗  t                                      ~/projects/vt-emulator\n"
        ".\n"
        "├── binding\n"
        "│   └── wallix_term.py\n"
        "├── browser\n"
        "│   ├── jquery.js\n"
        "│   ├── screen.json\n"
        "│   ├── tty-emulator\n"
        "│   │   ├── html_rendering.js\n"
        "│   │   └── player.css\n"
        "│   └── view_browser.html\n"
        "├── jam\n"
        "│   ├── cxxflags.jam\n"
        "│   └── testing.jam\n"
        "├── Jamroot\n"
        "├── LICENSE\n"
        "├── packaging\n"
        "│   ├── targets\n"
        "│   │   ├── debian\n"
        "│   │   └── ubuntu\n"
        "│   └── template\n"
        "│       └── debian\n"
        "│           ├── changelog\n"
        "│           ├── compat\n"
        "│           ├── control\n"
        "│           ├── copyright\n"
        "│           ├── rules\n"
        "│           └── vt-emulator.install\n"
        "├── README.md\n"
        "├── redemption\n"
        "│   └── src\n"
        "│       ├── cxx\n"
        "│       │   ├── attributes.hpp\n"
        "│       │   ├── diagnostic.hpp\n"
        "│       │   ├── features.hpp\n"
        "│       │   └── keyword.hpp\n"
        "│       ├── system\n"
        "│       │   └── linux\n"
        "│       │       └── system\n"
        "│       │           └── redemption_unit_tests.hpp\n"
        "│       └── utils\n"
        "│           └── sugar\n"
        "│               ├── array.hpp\n"
        "│               ├── array_view.hpp\n"
        "│               ├── bytes_t.hpp\n"
        "│               ├── enum_flags_operators.hpp\n"
        "│               └── underlying_cast.hpp\n"
        "├── src\n"
        "│   ├── rvt\n"
        "│   │   ├── character_color.hpp\n"
        "│   │   ├── character.hpp\n"
        "│   │   ├── char_class.hpp\n"
        "│   │   ├── charsets.hpp\n"
        "│   │   ├── color.hpp\n"
        "│   │   ├── screen.cpp\n"
        "│   │   ├── screen.hpp\n"
        "│   │   ├── text_rendering.cpp\n"
        "│   │   ├── text_rendering.hpp\n"
        "│   │   ├── ucs.hpp\n"
        "│   │   ├── utf8_decoder.hpp\n"
        "│   │   ├── vt_emulator.cpp\n"
        "│   │   └── vt_emulator.hpp\n"
        "│   └── rvt_lib\n"
        "│       ├── terminal_emulator.cpp\n"
        "│       ├── terminal_emulator.hpp\n"
        "│       └── version.hpp\n"
        "├── test\n"
        "│   ├── rvt\n"
        "│   │   ├── test_character_color.cpp\n"
        "│   │   ├── test_character.cpp\n"
        "│   │   ├── test_char_class.cpp\n"
        "│   │   ├── test_screen.cpp\n"
        "│   │   ├── test_utf8_decoder.cpp\n"
        "│   │   ├── test_vt_emulator.cpp\n"
        "│   │   └── typescript\n"
        "│   └── rvt_lib\n"
        "│       └── test_terminal_emulator.cpp\n"
        "├── tools\n"
        "│   ├── tagger.sh\n"
        "│   └── terminal_browser.cpp\n"
        "├── typescript\n"
        "└── vt-emulator.kdev4\n"
        "\n"
        "23 directories, 57 files\n"
        "[2]~/projects/vt-emulator!4891$(master)✗                                         ~/projects/vt-emulator\n"
        "\n"
        "Script done on 2017-11-28 11:33:08+0100\n");
}
