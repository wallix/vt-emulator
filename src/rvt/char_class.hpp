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

#pragma once

#include "rvt/ucs.hpp"

// CharClass for vt_emulator.cpp
namespace rvt {

namespace {
    // Character Class flags used while decoding
    constexpr int CTL =  1;  // Control character
    constexpr int CHR =  2;  // Printable character
    constexpr int CPN =  4;  // TODO: Document me
    constexpr int DIG =  8;  // Digit
    constexpr int SCS = 16;  // Select Character Set
    constexpr int GRP = 32;  // TODO: Document me
    constexpr int CPS = 64;  // Character which indicates end of window resize
}

    // C++14 version (does not working with gcc-4.9)
    struct CharClass
    {
        /*constexpr*/ CharClass() noexcept
        : charClass_{}
        {
            using u8 = unsigned char;

            for (int i = 0; i < 32; ++i)
                charClass_[i] |= CTL;
            for (int i = 32; i < 256; ++i)
                charClass_[i] |= CHR;
            for (auto s = "@ABCDGHILMPSTXZcdfry"; *s; ++s)
                charClass_[u8(*s)] |= CPN;
            // resize = \e[8;<row>;<col>t
            for (auto s = "t"; *s; ++s)
                charClass_[u8(*s)] |= CPS;
            for (auto s = "0123456789"; *s; ++s)
                charClass_[u8(*s)] |= DIG;
            for (auto s = "()+*%"; *s; ++s)
                charClass_[u8(*s)] |= SCS;
            for (auto s = "()+*#[]%"; *s; ++s)
                charClass_[u8(*s)] |= GRP;
        }

        /*constexpr*/ char operator[](ucs4_char cc) const noexcept
        {
            return charClass_[cc];
        }

        char const * begin() const noexcept
        {
            return charClass_;
        }

        char const * end() const noexcept
        {
            return charClass_ + sizeof(charClass_);
        }

    private:
        char charClass_[256];
    };

namespace {
    constexpr char charClass[256] {
        CTL, CTL, CTL, CTL, CTL, CTL, CTL, CTL, CTL, CTL, CTL, CTL, CTL, CTL,
        CTL, CTL, CTL, CTL, CTL, CTL, CTL, CTL, CTL, CTL, CTL, CTL, CTL, CTL,
        CTL, CTL, CTL, CTL, CHR, CHR, CHR, CHR|GRP, CHR, CHR|SCS|GRP, CHR,
        CHR, CHR|SCS|GRP, CHR|SCS|GRP, CHR|SCS|GRP, CHR|SCS|GRP, CHR, CHR,
        CHR, CHR, CHR|DIG, CHR|DIG, CHR|DIG, CHR|DIG, CHR|DIG, CHR|DIG,
        CHR|DIG, CHR|DIG, CHR|DIG, CHR|DIG, CHR, CHR, CHR, CHR, CHR, CHR,
        CHR|CPN, CHR|CPN,CHR|CPN, CHR|CPN, CHR|CPN, CHR, CHR, CHR|CPN,
        CHR|CPN, CHR|CPN, CHR, CHR, CHR|CPN, CHR|CPN, CHR, CHR, CHR|CPN,
        CHR, CHR, CHR|CPN, CHR|CPN, CHR, CHR, CHR, CHR|CPN, CHR, CHR|CPN,
        CHR|GRP, CHR, CHR|GRP, CHR, CHR, CHR, CHR, CHR, CHR|CPN, CHR|CPN,
        CHR, CHR|CPN, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR,
        CHR|CPN, CHR, CHR|CPS, CHR, CHR, CHR, CHR, CHR|CPN, CHR, CHR, CHR, CHR,
        CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR,
        CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR,
        CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR,
        CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR,
        CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR,
        CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR,
        CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR,
        CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR,
        CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR, CHR,
        CHR, CHR, CHR, CHR
    };
}

}
