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

enum class U8Color : uint8_t;

static constexpr char digit10_pairs[200] = {
    '0', '0', '0', '1', '0', '2', '0', '3', '0', '4', '0', '5', '0', '6',
    '0', '7', '0', '8', '0', '9', '1', '0', '1', '1', '1', '2', '1', '3',
    '1', '4', '1', '5', '1', '6', '1', '7', '1', '8', '1', '9', '2', '0',
    '2', '1', '2', '2', '2', '3', '2', '4', '2', '5', '2', '6', '2', '7',
    '2', '8', '2', '9', '3', '0', '3', '1', '3', '2', '3', '3', '3', '4',
    '3', '5', '3', '6', '3', '7', '3', '8', '3', '9', '4', '0', '4', '1',
    '4', '2', '4', '3', '4', '4', '4', '5', '4', '6', '4', '7', '4', '8',
    '4', '9', '5', '0', '5', '1', '5', '2', '5', '3', '5', '4', '5', '5',
    '5', '6', '5', '7', '5', '8', '5', '9', '6', '0', '6', '1', '6', '2',
    '6', '3', '6', '4', '6', '5', '6', '6', '6', '7', '6', '8', '6', '9',
    '7', '0', '7', '1', '7', '2', '7', '3', '7', '4', '7', '5', '7', '6',
    '7', '7', '7', '8', '7', '9', '8', '0', '8', '1', '8', '2', '8', '3',
    '8', '4', '8', '5', '8', '6', '8', '7', '8', '8', '8', '9', '9', '0',
    '9', '1', '9', '2', '9', '3', '9', '4', '9', '5', '9', '6', '9', '7',
    '9', '8', '9', '9',
};

struct RenderingBuffer2
{
    RenderingBuffer2(RenderingBuffer buffer, std::size_t consumed_len = 0) noexcept
    : _p(buffer.buffer + consumed_len)
    , _ctx(buffer.ctx)
    , _start_p(buffer.buffer)
    , _end_p(buffer.buffer + buffer.length)
    , _allocate(buffer.allocate)
    , _set_final_buffer(buffer.set_final_buffer)
    {
        assert(_p <= _end_p);
    }

    TranscriptPartialBuffer to_transcript_buffer() const
    {
        return {_start_p, buffer_length(), checked_int(_p - _start_p)};
    }

    void prepare_buffer(std::size_t min_remaining, std::size_t extra_capacity)
    {
        if (REDEMPTION_UNLIKELY(remaining() < min_remaining)) {
            allocate(extra_capacity);
        }
    }

    void prepare_buffer(std::size_t extra_capacity)
    {
        prepare_buffer(extra_capacity, extra_capacity);
    }

    uint8_t* start_buffer() const
    {
        return bytes_t(_start_p).to_u8p();
    }

    void set_final()
    {
        _set_final_buffer(_ctx, start_buffer(), buffer_length());
    }

    void allocate(std::size_t extra_capacity)
    {
        auto* p = _allocate(_ctx, extra_capacity, start_buffer(), buffer_length());
        if (REDEMPTION_UNLIKELY(not p)) {
            throw std::bad_alloc();
        }
        _p = bytes_t(p).to_charp();
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
        _p += unsafe_ucs4_to_utf8(ucs, _p);
    }

    void unsafe_push_quoted_ucs(ucs4_char ucs)
    {
        switch (ucs) {
            case '\\': assert(remaining() >= 2); *_p++ = '\\'; *_p++ = '\\'; break;
            case '"': assert(remaining() >= 2); *_p++ = '\\'; *_p++ = '"'; break;
            default : assert(remaining() >= 4); _p += unsafe_ucs4_to_utf8(ucs, _p); break;
        }
    }

    void unsafe_push_quoted_character(Character const & ch, const rvt::ExtendedCharTable & extended_char_table, std::size_t extra_capacity)
    {
        if (ch.isRealCharacter) {
            if (REDEMPTION_UNLIKELY(ch.is_extended())) {
                this->push_ucs_array(extended_char_table[ch.character], extra_capacity);
            }
            else {
                this->unsafe_push_quoted_ucs(ch.character);
            }
        }
        else {
            // TODO only if tabulation character
            this->unsafe_push_c(' ');
        }
    }

    void unsafe_push_quoted_ucs_array(ucs4_carray_view ucs_array)
    {
        for (ucs4_char ucs : ucs_array) {
            unsafe_push_quoted_ucs(ucs);
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
        this->unsafe_push_quoted_ucs_array(ucs_array);
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
        unsafe_push_quoted_ucs_array(x);
    }

    void _unsafe_push_value(chars_view x)
    {
        unsafe_push_s(x);
    }

    void _unsafe_push_value(U8Color x)
    {
        uint8_t value { underlying_cast(x) };
        // TODO remove condition
        if (value < 100) {
            if (value < 10) {
                unsafe_push_c(char('0' + value));
            }
            else {
                unsafe_push_c(digit10_pairs[value * 2]);
                unsafe_push_c(digit10_pairs[value * 2 + 1]);
            }
        }
        else {
            unsafe_push_c(char('0' + value / 100));
            unsafe_push_c(digit10_pairs[value % 100 * 2]);
            unsafe_push_c(digit10_pairs[value % 100 * 2 + 1]);
        }
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
    buf.unsafe_push_quoted_ucs_array(title);
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
                        int const r = (0
                            | (bool(ch.rendition & rvt::Rendition::Bold)      ? 1 : 0)
                            | (bool(ch.rendition & rvt::Rendition::Italic)    ? 2 : 0)
                            | (bool(ch.rendition & rvt::Rendition::Underline) ? 4 : 0)
                            | (bool(ch.rendition & rvt::Rendition::Blink)     ? 8 : 0)
                        );
                        if (r < 10) {
                            buf.unsafe_push_values("\"r\":"_av, char(r + '0'), ',');
                        }
                        else {
                            buf.unsafe_push_values("\"r\":"_av, '1', char(r - 10 + '0'), ',');
                        }
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

                buf.unsafe_push_quoted_character(ch, screen.extendedCharTable(), 4096);

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
        buf.unsafe_push_values(';', cmd, '8', ';', '2', ';',
                               U8Color(color.red()), ';',
                               U8Color(color.green()), ';',
                               U8Color(color.blue()));
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

            buf.unsafe_push_quoted_character(ch, screen.extendedCharTable(), 4096);

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


TranscriptPartialBuffer transcript_partial_rendering(
    Screen const & screen, size_t y, size_t yend,
    RenderingBuffer buffer, std::size_t consumed_buffer
) {
    RenderingBuffer2 buf{buffer, consumed_buffer};

    using Line = array_view<const rvt::Character>;

    auto write_line_impl = [&](Line line){
        std::size_t nb_byte_for_ascii_line = line.size() * 4u;
        buf.prepare_buffer(nb_byte_for_ascii_line);
        for (auto const& ch : line) {
            if (REDEMPTION_UNLIKELY(ch.is_extended())) {
                auto chars = screen.extendedCharTable()[ch.character];
                buf.prepare_buffer(chars.size() * 4);
                buf.unsafe_push_ucs_array(chars);
                buf.prepare_buffer(checked_int((line.end() - &ch) * 4), nb_byte_for_ascii_line);
            }
            else {
                buf.unsafe_push_ucs(ch.character);
            }
        }
    };

    auto const&& lines = screen.getScreenLines();
    auto const&& lineProperties = screen.getLineProperties();
    constexpr auto wrapped = rvt::LineProperty::Wrapped;
    while (y && bool(lineProperties[y-1] & wrapped)) {
        --y;
    }
    while (y < yend) {
        write_line_impl(lines[y]);
        if (bool(lineProperties[y] & wrapped)) {
            while (++y < lines.size()) {
                write_line_impl(lines[y]);
                if (!bool(lineProperties[y] & wrapped)) {
                    break;
                }
            }
        }
        ++y;

        buf.prepare_buffer(1);
        buf.unsafe_push_c('\n');
    }

    return buf.to_transcript_buffer();
}

namespace
{

template<class T>
RenderingBuffer vector_to_rendering_buffer(std::vector<T>& v)
{
    auto allocate = [](void* ctx, std::size_t extra_capacity, uint8_t* p, std::size_t used_size) {
        auto v = static_cast<std::vector<T>*>(ctx);
        std::size_t current_len = static_cast<std::size_t>(p - bytes_t(v->data()).to_u8p()) + used_size;
        v->resize(current_len + extra_capacity);
        return bytes_t(v->data()).to_u8p() + current_len;
    };

    auto set_final = [](void* ctx, uint8_t* p, std::size_t used_size) {
        auto v = static_cast<std::vector<T>*>(ctx);
        std::size_t current_len = static_cast<std::size_t>(p - bytes_t(v->data()).to_u8p()) + used_size;
        v->resize(current_len);
    };

    return RenderingBuffer{&v, bytes_t(v.data()).to_charp(), v.size(), allocate, set_final};
}

}

RenderingBuffer RenderingBuffer::from_vector(std::vector<char>& v)
{
    return vector_to_rendering_buffer(v);
}

RenderingBuffer RenderingBuffer::from_vector(std::vector<uint8_t>& v)
{
    return vector_to_rendering_buffer(v);
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
