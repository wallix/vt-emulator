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

#include "rvt/color.hpp"

#include "cxx/diagnostic.hpp"
#include "utils/sugar/array.hpp"

#include <cassert>
#include <array>
#include <type_traits>

namespace rvt
{

// Attributed Character Representations ///////////////////////////////

// Colors

constexpr int BASE_COLORS = 2+8;
constexpr int INTENSITIES = 2;
constexpr int TABLE_COLORS = INTENSITIES * BASE_COLORS;

constexpr int DEFAULT_FORE_COLOR = 0;
constexpr int DEFAULT_BACK_COLOR = 1;

/* CharacterColor is a union of the various color spaces.

   Assignment is as follows:

   Type  - Space        - Values

   0     - Undefined   - u:  0,      v:0        w:0
   1     - Default     - u:  0..1    v:intense  w:0
   2     - System      - u:  0..7    v:intense  w:0
   3     - Index(256)  - u: 16..255  v:0        w:0
   4     - RGB         - u:  0..255  v:0..255   w:0..255

   Default color space has two separate colors, namely
   default foreground and default background color.
*/

enum class ColorSpace : uint8_t
{
    Undefined,
    Default,
    System,
    Index256,
    RGB,
};

constexpr std::array<Color, TABLE_COLORS>
color_table = utils::make_array(
    Color(0xEE, 0xEE, 0xEE), // Dfore
    Color(0x33, 0x33, 0x33), // Dback
    Color(0x00, 0x00, 0x00), // Black
    Color(0xB2, 0x18, 0x18), // Red
    Color(0x18, 0xB2, 0x18), // Green
    Color(0xB2, 0x68, 0x18), // Yellow
    Color(0x18, 0x18, 0xB2), // Blue
    Color(0xB2, 0x18, 0xB2), // Magenta
    Color(0x18, 0xB2, 0xB2), // Cyan
    Color(0xB2, 0xB2, 0xB2), // White
    // intensive versions
    Color(0xFF, 0xFF, 0xFF), // Dfore
    Color(0x00, 0x00, 0x00), // Dback
    Color(0x68, 0x68, 0x68), // Black
    Color(0xFF, 0x54, 0x54), // Red
    Color(0x54, 0xFF, 0x54), // Green
    Color(0xFF, 0xFF, 0x54), // Yellow
    Color(0x54, 0x54, 0xFF), // Blue
    Color(0xFF, 0x54, 0xFF), // Magenta
    Color(0x54, 0xFF, 0xFF), // Cyan
    Color(0xFF, 0xFF, 0xFF)  // White
);

constexpr std::array<Color, TABLE_COLORS>
xterm_color_table = utils::make_array(
    Color(0xFF, 0xFF, 0xFF), // Dfore
    Color(0x00, 0x00, 0x00), // Dback
    Color(0x00, 0x00, 0x00), // Black
    Color(0xCD, 0x00, 0x00), // Red
    Color(0x00, 0xCD, 0x00), // Green
    Color(0xCD, 0xCD, 0x00), // Yellow
    Color(0x00, 0x00, 0xEE), // Blue
    Color(0xCD, 0x00, 0xCD), // Magenta
    Color(0x00, 0xCD, 0xCD), // Cyan
    Color(0xE5, 0xE5, 0xE5), // White
    // intensive versions
    Color(0xFF, 0xFF, 0xFF), // Dfore
    Color(0x00, 0x00, 0x00), // Dback
    Color(0x7F, 0x7F, 0x7F), // Black
    Color(0xFF, 0x00, 0x00), // Red
    Color(0x00, 0xFF, 0x00), // Green
    Color(0xFF, 0xFF, 0x00), // Yellow
    Color(0x5c, 0x5c, 0xFF), // Blue
    Color(0xFF, 0x00, 0xFF), // Magenta
    Color(0x00, 0xFF, 0xFF), // Cyan
    Color(0xFF, 0xFF, 0xFF)  // White
);


// TODO utils/fixed_array_view.hpp
template<class T, std::size_t N>
struct fixed_array_view
{
    fixed_array_view(std::array<typename std::remove_const<T>::type, N> const & a) noexcept
    : p(a.data())
    {}

    fixed_array_view(std::array<T, N> const & a) noexcept
    : p(a.data())
    {}

    fixed_array_view(T const (&a)[N]) noexcept
    : p(a)
    {}

    T const & operator[](std::size_t i) const noexcept
    {
        assert(i < N);
        return p[i];
    }

private:
    T const * p;
};

using ColorTableView = fixed_array_view<const Color, TABLE_COLORS>;

/**
 * Describes the color of a single character in the terminal.
 */
class CharacterColor
{
public:
    /** Constructs a new CharacterColor whose color and color space are undefined. */
    CharacterColor()
    : _colorSpaceWithDim(ColorSpace::Undefined)
    {}

    /**
     * Constructs a new CharacterColor using the specified @p colorSpace and with
     * color value @p co
     *
     * The meaning of @p co depends on the @p colorSpace used.
     *
     * TODO : Document how @p co relates to @p colorSpace
     *
     * TODO : Add documentation about available color spaces.
     */
    CharacterColor(ColorSpace colorSpace, int32_t co)
    : _colorSpaceWithDim(colorSpace)
    {
        REDEMPTION_DIAGNOSTIC_PUSH()
        REDEMPTION_DIAGNOSTIC_GCC_IGNORE("-Wconversion")
        switch (colorSpace) {
        case ColorSpace::Default:
            _u = co & 1;
            break;
        case ColorSpace::System:
            _u = co & 7;
            _v = (co >> 3) & 1;
            break;
        case ColorSpace::Index256:
            _u = co & 255;
            break;
        case ColorSpace::RGB:
            _u = co >> 16;
            _v = co >> 8;
            _w = co;
            break;
        case ColorSpace::Undefined:
            break;
        }
        REDEMPTION_DIAGNOSTIC_POP()
    }

    /**
     * Returns true if this character color entry is valid.
     */
    bool isValid() const {
        return _colorSpaceWithDim.colorSpace() != ColorSpace::Undefined;
    }

    /**
     * Set this color as an intensive system color.
     *
     * This is only applicable if the color is using the ColorSpace::Default or ColorSpace::System
     * color spaces.
     */
    void setIntensive();

    /**
     * Set this color as an dim color.
     */
    void setDim();

    /**
     * Returns the color within the specified color @p palette
     *
     * The @p palette is only used if this color is one of the 16 system colors, otherwise
     * it is ignored.
     */
    Color color(ColorTableView palette) const;

    /**
     * Compares two colors and returns true if they represent the same color value and
     * use the same color space.
     */
    friend bool operator == (const CharacterColor& a, const CharacterColor& b);
    /**
     * Compares two colors and returns true if they represent different color values
     * or use different color spaces.
     */
    friend bool operator != (const CharacterColor& a, const CharacterColor& b);

private:
    class ColorSpaceWithDim
    {
        using IntColorSpace = std::underlying_type<ColorSpace>::type;
        IntColorSpace _intColorSpace;
        static const uint8_t _colorSpaceDimFlag = 0x8;

    public:
        explicit ColorSpaceWithDim(ColorSpace colorSpace) noexcept
          : _intColorSpace(IntColorSpace(colorSpace))
        {}

        bool operator == (ColorSpaceWithDim const& other) const noexcept
        {
            return _intColorSpace == other._intColorSpace;
        }

        ColorSpace colorSpace() const noexcept
        {
            return static_cast<ColorSpace>(_intColorSpace & IntColorSpace(~_colorSpaceDimFlag));
        }

        void setDim() noexcept
        {
            _intColorSpace |= _colorSpaceDimFlag;
        }

        bool isDim() const noexcept
        {
            return bool(_intColorSpace & _colorSpaceDimFlag);
        }
    };
    ColorSpaceWithDim _colorSpaceWithDim;

    // bytes storing the character color
    uint8_t _u = 0;
    uint8_t _v = 0;
    uint8_t _w = 0;
};

inline bool operator == (const CharacterColor& a, const CharacterColor& b)
{
    return
        a._colorSpaceWithDim == b._colorSpaceWithDim &&
        a._u == b._u &&
        a._v == b._v &&
        a._w == b._w;
}
inline bool operator != (const CharacterColor& a, const CharacterColor& b)
{
    return !operator==(a, b);
}

inline Color color256(uint8_t u, fixed_array_view<const Color, TABLE_COLORS> base)
{
    REDEMPTION_DIAGNOSTIC_PUSH()
    REDEMPTION_DIAGNOSTIC_GCC_IGNORE("-Wconversion")
    //   0.. 16: system colors
    if (u < 8) {
        return base[u + 2];
    }
    u -= 8;
    if (u < 8) {
        return base[u + 2 + BASE_COLORS];
    }
    u -= 8;

    //  16..231: 6x6x6 rgb color cube
    if (u < 216) {
        return Color(((u / 36) % 6) ? (40 * ((u / 36) % 6) + 55) : 0,
                     ((u / 6) % 6) ? (40 * ((u / 6) % 6) + 55) : 0,
                     ((u / 1) % 6) ? (40 * ((u / 1) % 6) + 55) : 0);
    }
    u -= 216;

    // 232..255: gray, leaving out black and white
    uint8_t gray = u * 10 + 8;
    REDEMPTION_DIAGNOSTIC_POP()

    return Color(gray, gray, gray);
}

inline Color CharacterColor::color(fixed_array_view<const Color, TABLE_COLORS> base) const
{
    Color const color = [this, &base]{
        switch (_colorSpaceWithDim.colorSpace()) {
        case ColorSpace::Default:
            return base[_u + 0 + (_v ? BASE_COLORS : 0)];
        case ColorSpace::System:
            return base[_u + 2 + (_v ? BASE_COLORS : 0)];
        case ColorSpace::Index256:
            return color256(_u, base);
        case ColorSpace::RGB:
            return Color(_u, _v, _w);
        case ColorSpace::Undefined:
            return Color();
        }

        assert(false); // invalid color space

        return Color();
    }();

    return _colorSpaceWithDim.isDim()
    ? Color(
        color.red()   * 2 / 3,
        color.green() * 2 / 3,
        color.blue()  * 2 / 3
    ) : color;
}

inline void CharacterColor::setIntensive()
{
    auto const colorSpace = _colorSpaceWithDim.colorSpace();
    if (colorSpace == ColorSpace::System || colorSpace == ColorSpace::Default) {
        _v = 1;
    }
}

inline void CharacterColor::setDim()
{
    _colorSpaceWithDim.setDim();
}

}
