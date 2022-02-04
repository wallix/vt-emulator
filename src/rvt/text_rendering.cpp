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

#include "utils/sugar/numerics/safe_conversions.hpp"
#include "rvt/text_rendering.hpp"

#include "rvt/character.hpp"
#include "rvt/screen.hpp"

#include "rvt/ucs.hpp"
#include "rvt/utf8_decoder.hpp"

#include <charconv>

namespace rvt {

namespace
{

struct RenderingBuffer2
{
    RenderingBuffer2(RenderingBuffer buffer) noexcept
    : _p(buffer.buffer)
    , _ctx(buffer.ctx)
    , _start_p(buffer.buffer)
    , _end_p(buffer.buffer + buffer.length)
    , _allocate(buffer.allocate)
    , _set_final_buffer(buffer.set_final_buffer)
    {}

    void prepare_buffer(std::size_t min_remaining, std::size_t extra_capacity)
    {
        if (REDEMPTION_UNLIKELY(remaining() < min_remaining)) {
            allocate(extra_capacity);
        }
    }

    void set_final()
    {
        _set_final_buffer(_ctx, _start_p, buffer_length());
    }

    void allocate(std::size_t extra_capacity)
    {
        _p = _allocate(_ctx, extra_capacity, _start_p, buffer_length());
        _start_p = _p;
        _end_p = _p + extra_capacity;
    }

    std::size_t buffer_length() const noexcept
    {
        return checked_int(_p - _start_p);
    }

    std::size_t remaining() const noexcept
    {
        return checked_int(_end_p - _p);
    }

    void pop_c()
    {
        assert(_p > _start_p);
        --_p;
    }

    void unsafe_push_ucs(ucs4_char ucs)
    {
        // TODO optimize that
        switch (ucs) {
            case '\\': assert(remaining() >= 2); *_p++ = '\\'; *_p++ = '\\'; break;
            case '"': assert(remaining() >= 2); *_p++ = '\\'; *_p++ = '"'; break;
            default : assert(remaining() >= 4); _p += unsafe_ucs4_to_utf8(ucs, _p); break;
        }
    }

    void unsafe_push_character(Character const & ch, const rvt::ExtendedCharTable & extended_char_table, std::size_t extra_capacity)
    {
        if (ch.isRealCharacter) {
            if (REDEMPTION_UNLIKELY(ch.is_extended())) {
                this->push_ucs_array(extended_char_table[ch.character], extra_capacity);
            }
            else {
                this->unsafe_push_ucs(ch.character);
            }
        }
        else {
            // TODO only if tabulation character
            this->unsafe_push_c(' ');
        }
    }

    void unsafe_push_ucs_array(ucs4_carray_view ucs_array)
    {
        for (ucs4_char ucs : ucs_array) {
            unsafe_push_ucs(ucs);
        }
    }

    void push_ucs_array(ucs4_carray_view ucs_array, std::size_t extra_capacity)
    {
        prepare_buffer(ucs_array.size() * 4, std::max(extra_capacity, ucs_array.size() * 4u));
        this->unsafe_push_ucs_array(ucs_array);
    }

    void unsafe_push_s(chars_view str)
    {
        assert(remaining() >= str.size());
        memcpy(_p, str.data(), str.size());
        _p += str.size();
    }

    void unsafe_push_s(std::string_view str)
    {
        unsafe_push_s(chars_view{str.data(), str.size()});
    }

    void unsafe_push_c(ucs4_char c) = delete; // unused
    void unsafe_push_c(char c)
    {
        assert(remaining() >= 1);
        *_p++ = c;
    }

    template<class... Ts>
    void unsafe_push_values(Ts const&... xs)
    {
        (..., _unsafe_push_value(xs));
    }

    template<class... Ts>
    void push_values(Ts const&... xs)
    {
        std::size_t len = (... + _value_size(xs));
        prepare_buffer(len, std::max<std::size_t>(4096, len * 2));
        (..., _unsafe_push_value(xs));
    }

private:
    void _unsafe_push_value(int x)
    {
        auto r = std::to_chars(_p, _p + remaining(), x);
        assert(r.ec == std::errc());
        _p = r.ptr;
    }

    void _unsafe_push_value(uint32_t x)
    {
        auto r = std::to_chars(_p, _p + remaining(), x);
        assert(r.ec == std::errc());
        _p = r.ptr;
    }

    void _unsafe_push_value(char x)
    {
        unsafe_push_c(x);
    }

    void _unsafe_push_value(ucs4_carray_view x)
    {
        unsafe_push_ucs_array(x);
    }

    void _unsafe_push_value(chars_view x)
    {
        unsafe_push_s(x);
    }

    // static std::size_t _value_size(int)
    // {
    //     return std::numeric_limits<int>::digits10 + 2;
    // }
    //
    // static std::size_t _value_size(uint32_t)
    // {
    //     return std::numeric_limits<uint32_t>::digits10 + 2;
    // }

    static std::size_t _value_size(char)
    {
        return 1;
    }

    static std::size_t _value_size(ucs4_carray_view ucs_array)
    {
        return ucs_array.size() * 4u;
    }

    // static std::size_t _value_size(chars_view av)
    // {
    //     return av.size();
    // }

    char * _p;
    void * _ctx;
    char * _start_p;
    char * _end_p;
    RenderingBuffer::ExtraMemoryAllocator * _allocate;
    RenderingBuffer::SetFinalBuffer * _set_final_buffer;
};

}

// format = "{
//      $cursor,
//      lines: %d,
//      columns: %d,
//      title: %s,
//      style: {$render $foreground $background},
//      data: [ $line... ]
//      extra: extra_data // if extra_data != nullptr
// }"
// $line = "[ {" $render? $foreground? $background? "s: %s } ]"
// $cursor = "x: %d, y: %d" | "y: -1"
// $render = "r: %d
//      flags:
//      1 -> bold
//      2 -> italic
//      4 -> underline
//      8 -> blink
// $foreground = "f: $color"
// $background = "b: $color"
// $color = %d
//      decimal rgb
void json_rendering(
    ucs4_carray_view title,
    Screen const & screen,
    ColorTableView palette,
    RenderingBuffer buffer,
    std::string_view extra_data
) {
    auto color2int = [](rvt::Color const & color){
        return uint32_t((color.red() << 16) | (color.green() << 8) |  (color.blue() << 0));
    };

    RenderingBuffer2 buf{buffer};

    buf.prepare_buffer(4096, std::max(title.size() * 4 + 512, std::size_t(4096)));

    if (screen.hasCursorVisible()) {
        buf.unsafe_push_values("{\"x\":"_av, screen.getCursorX(),
                               ",\"y\":"_av, screen.getCursorY());
    }
    else {
        buf.unsafe_push_s(R"({"y":-1)"_av);
    }
    buf.unsafe_push_values(",\"lines\":"_av, screen.getLines(),
                           ",\"columns\":"_av, screen.getColumns(),
                           ",\"title\":\""_av);
    buf.unsafe_push_ucs_array(title);
    buf.unsafe_push_values("\",\"style\":{\"r\":0"
                           ",\"f\":"_av, color2int(palette[0]),
                           ",\"b\":"_av, color2int(palette[1]), "},\"data\":["_av);

    constexpr std::size_t max_size_by_loop = 111; // approximate

    if (screen.getColumns() && screen.getLines()) {
        rvt::Character const default_ch; // Default format
        rvt::Character const* previous_ch = &default_ch;

        for (auto const & line : screen.getScreenLines()) {
            buf.unsafe_push_s("[[{"_av);

            bool is_s_enable = false;
            for (rvt::Character const & ch : line) {
                buf.prepare_buffer(max_size_by_loop, 4096);

                constexpr auto rendition_flags
                    = rvt::Rendition::Bold
                    | rvt::Rendition::Italic
                    | rvt::Rendition::Underline
                    | rvt::Rendition::Blink;
                bool const is_same_bg = ch.backgroundColor == previous_ch->backgroundColor;
                bool const is_same_fg = ch.foregroundColor == previous_ch->foregroundColor;
                bool const is_same_rendition
                    = (ch.rendition & rendition_flags) == (previous_ch->rendition & rendition_flags);
                bool const is_same_format = is_same_bg & is_same_fg & is_same_rendition;
                if (!is_same_format) {
                    if (is_s_enable) {
                        buf.unsafe_push_s("\"},{"_av);
                    }
                    if (!is_same_rendition) {
                        // TODO table of u8
                        int const r = (0
                            | (bool(ch.rendition & rvt::Rendition::Bold)      ? 1 : 0)
                            | (bool(ch.rendition & rvt::Rendition::Italic)    ? 2 : 0)
                            | (bool(ch.rendition & rvt::Rendition::Underline) ? 4 : 0)
                            | (bool(ch.rendition & rvt::Rendition::Blink)     ? 8 : 0)
                        );
                        buf.unsafe_push_values("\"r\":"_av, r, ',');
                    }

                    if (!is_same_fg) {
                        buf.unsafe_push_values("\"f\":"_av,
                            color2int(ch.foregroundColor.color(palette)), ',');
                    }
                    if (!is_same_bg) {
                        buf.unsafe_push_values("\"b\":"_av,
                            color2int(ch.backgroundColor.color(palette)), ',');
                    }

                    is_s_enable = false;
                }

                if (!is_s_enable) {
                    is_s_enable = true;
                    buf.unsafe_push_s(R"("s":")"_av);
                }

                buf.unsafe_push_character(ch, screen.extendedCharTable(), 4096);

                previous_ch = &ch;
            }

            buf.prepare_buffer(max_size_by_loop, 4096);
            if (is_s_enable) {
                buf.unsafe_push_c('"');
            }
            buf.unsafe_push_s("}]],"_av);
        }

        buf.pop_c();
    }

    if (!extra_data.empty()) {
        buf.unsafe_push_s("],\"extra\":"_av);
        buf.prepare_buffer(extra_data.size() + 1u, extra_data.size() + 1u);
        buf.unsafe_push_s(extra_data);
        buf.unsafe_push_c('}');
    }
    else {
        buf.unsafe_push_s("]}"_av);
    }

    buf.set_final();
}


void ansi_rendering(
    ucs4_carray_view title,
    Screen const & screen,
    ColorTableView palette,
    RenderingBuffer buffer,
    std::string_view extra_data
) {
    auto write_color = [palette](RenderingBuffer2 & buf, char cmd, rvt::CharacterColor const & ch_color) {
        auto color = ch_color.color(palette);
        // TODO table of color
        buf.unsafe_push_values(';', cmd, '8', ';', '2', ';',
                               color.red()+0, ';',
                               color.green()+0, ';',
                               color.blue()+0);
    };

    RenderingBuffer2 buf{buffer};

    buf.prepare_buffer(4096, 4096);
    buf.push_values('\033', ']', title, '\a');

    rvt::Character const default_ch; // Default format
    rvt::Character const* previous_ch = &default_ch;

    constexpr std::size_t max_size_by_loop = 64; // approximate

    for (auto const & line : screen.getScreenLines()) {
        for (rvt::Character const & ch : line) {
            buf.prepare_buffer(max_size_by_loop, 4096);

            bool const is_same_bg = ch.backgroundColor == previous_ch->backgroundColor;
            bool const is_same_fg = ch.foregroundColor == previous_ch->foregroundColor;
            bool const is_same_rendition = ch.rendition == previous_ch->rendition;
            bool const is_same_format = is_same_bg & is_same_fg & is_same_rendition;
            if (!is_same_format) {
                buf.unsafe_push_s("\033[0"_av);
                if (!is_same_format) {
                    auto const r = ch.rendition;
                    if (bool(r & rvt::Rendition::Bold))     { buf.unsafe_push_s(";1"_av); }
                    if (bool(r & rvt::Rendition::Italic))   { buf.unsafe_push_s(";3"_av); }
                    if (bool(r & rvt::Rendition::Underline)){ buf.unsafe_push_s(";4"_av); }
                    if (bool(r & rvt::Rendition::Blink))    { buf.unsafe_push_s(";5"_av); }
                    if (bool(r & rvt::Rendition::Reverse))  { buf.unsafe_push_s(";6"_av); }
                }
                if (!is_same_fg) write_color(buf, '3', ch.foregroundColor);
                if (!is_same_bg) write_color(buf, '4', ch.backgroundColor);
                buf.unsafe_push_c('m');
            }

            buf.unsafe_push_character(ch, screen.extendedCharTable(), 4096);

            previous_ch = &ch;
        }

        buf.prepare_buffer(max_size_by_loop, 4096);
        buf.unsafe_push_c('\n');
    }

    if (!extra_data.empty()) {
        buf.prepare_buffer(extra_data.size(), extra_data.size());
        buf.unsafe_push_s(extra_data);
    }

    buf.set_final();
}


RenderingBuffer RenderingBuffer::from_vector(std::vector<char>& v)
{
    auto allocate = [](void* ctx, std::size_t extra_capacity, char* p, std::size_t used_size){
        auto v = static_cast<std::vector<char>*>(ctx);
        std::size_t current_len = static_cast<std::size_t>(p - v->data()) + used_size;
        v->resize(current_len + extra_capacity);
        return v->data() + current_len;
    };

    auto set_final = [](void* ctx, char* p, std::size_t used_size) {
        auto v = static_cast<std::vector<char>*>(ctx);
        std::size_t current_len = static_cast<std::size_t>(p - v->data()) + used_size;
        v->resize(current_len);
    };

    return RenderingBuffer{&v, v.data(), v.size(), allocate, set_final};
}

std::vector<char> json_rendering(
    ucs4_carray_view title, Screen const & screen,
    ColorTableView palette, std::string_view extra_data
)
{
    std::vector<char> v;
    json_rendering(title, screen, palette, RenderingBuffer::from_vector(v), extra_data);
    return v;
}

std::vector<char> ansi_rendering(
    ucs4_carray_view title, Screen const & screen,
    ColorTableView palette, std::string_view extra_data
)
{
    std::vector<char> v;
    ansi_rendering(title, screen, palette, RenderingBuffer::from_vector(v), extra_data);
    return v;
}

}
