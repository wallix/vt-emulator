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

#include "terminal_emulator.hpp"
#include "version.hpp"

#include "rvt/character_color.hpp"
#include "rvt/vt_emulator.hpp"
#include "rvt/utf8_decoder.hpp"
#include "rvt/text_rendering.hpp"

#include <memory>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


extern "C"
{

struct TerminalEmulator
{
    rvt::VtEmulator emulator;
    rvt::Utf8Decoder decoder;

    TerminalEmulator(int lines, int columns)
    : emulator(lines, columns)
    {}
};

struct TerminalEmulatorBuffer
{
    void * ctx;
    TerminalEmulatorBufferGetBufferFn * get_buffer_fn;
    TerminalEmulatorBufferExtraMemoryAllocatorFn * extra_memory_allocator_fn;
    TerminalEmulatorBufferSetFinalBufferFn * set_final_buffer_fn;
    TerminalEmulatorBufferClearFn * clear_fn;
    TerminalEmulatorBufferDeleteCtxFn * delete_ctx_fn;
    void(*delete_self)(TerminalEmulatorBuffer* self) noexcept;

    rvt::RenderingBuffer as_rendering_buffer()
    {
        std::size_t len = 0;
        uint8_t* data = get_buffer_fn(ctx, &len);
        return rvt::RenderingBuffer{
            ctx,
            bytes_t(data).to_charp(), len,
            extra_memory_allocator_fn,
            set_final_buffer_fn
        };
    }
};

struct TerminalEmulatorBufferWithVector : TerminalEmulatorBuffer
{
    struct Data
    {
        std::unique_ptr<uint8_t[]> buffer;
        std::size_t length;
        std::size_t capacity;
        std::size_t max_capacity;

        std::size_t count_before(uint8_t* p) const
        {
            return static_cast<std::size_t>(p - data());
        }

        uint8_t* data() const noexcept
        {
            return buffer.get();
        }
    };

    Data d;

    TerminalEmulatorBufferWithVector(std::size_t max_capacity)
        noexcept(noexcept(std::vector<uint8_t>()))
    : TerminalEmulatorBuffer{
        &d,
        // get buffer
        [](void* ctx, std::size_t * output_len) noexcept {
            auto& d = *static_cast<Data*>(ctx);
            *output_len = d.length;
            return d.data();
        },
        // alloc extra memory
        [](void* ctx, std::size_t* extra_capacity_in_out, uint8_t* p, std::size_t used_size) -> uint8_t* {
            assert(extra_capacity_in_out);

            auto& d = *static_cast<Data*>(ctx);

            std::size_t const extra_capacity = *extra_capacity_in_out;
            std::size_t const current_len = d.count_before(p) + used_size;
            assert(current_len <= d.length);

            // check max_capacity
            if (REDEMPTION_UNLIKELY(d.max_capacity - current_len <= extra_capacity)) {
                return nullptr;
            }

            // resize
            std::size_t new_size = current_len + extra_capacity;
            if (new_size > d.capacity) {
                new_size = d.capacity + std::max(d.capacity, new_size);
                new_size = std::min(d.max_capacity, new_size);
                auto* new_buffer = new(std::nothrow) uint8_t[new_size];
                if (!new_buffer) {
                    return nullptr;
                }
                if (d.data()) {
                    memcpy(new_buffer, d.data(), current_len);
                }
                d.buffer.reset(new_buffer);
                d.capacity = new_size;
            }

            *extra_capacity_in_out = d.capacity - current_len;
            d.length = d.capacity;
            return d.data() + current_len;
        },
        // set final buffer
        [](void* ctx, uint8_t* p, std::size_t used_size) {
            auto& d = *static_cast<Data*>(ctx);
            std::size_t const current_len = d.count_before(p) + used_size;
            assert(current_len <= d.length);
            d.length = current_len;
        },
        // clear
        [](void* ctx) noexcept {
            auto& d = *static_cast<Data*>(ctx);
            d.length = 0;
        },
        // delete
        [](void* /*ctx*/) noexcept {},
        // delete self
        [](TerminalEmulatorBuffer* self) noexcept {
            delete static_cast<TerminalEmulatorBufferWithVector*>(self);
        }
    }
    , d{{}, 0, 0, max_capacity == 0 ? ~std::size_t() : max_capacity}
    {}

    bool initial_size(std::size_t len)
    {
        auto* buf = new(std::nothrow) uint8_t[len];
        if (!buf) {
            return false;
        }
        d.buffer.reset(buf);
        d.length = len;
        d.capacity = len;
        return true;
    }
};

} // extern "C"

static int errno_or_single_error() noexcept
{
    int errnum = errno;
    return errnum ? errnum : -1;
}

static int build_format_string(
    TerminalEmulatorBuffer & buffer, TerminalEmulator & emu,
    TerminalEmulatorOutputFormat format, std::string_view extra_data
) noexcept
{
    rvt::RenderingBuffer rendering_buffer = buffer.as_rendering_buffer();
    try {
        #define call_rendering(Format)                 \
            case TerminalEmulatorOutputFormat::Format: \
                rvt::Format##_rendering(               \
                    emu.emulator.getWindowTitle(),     \
                    emu.emulator.getCurrentScreen(),   \
                    rvt::xterm_color_table,            \
                    rendering_buffer,                  \
                    extra_data                         \
                ); return 0
        switch (format) {
            call_rendering(json);
            call_rendering(ansi);
        }
        #undef call_rendering
        return -2;
    }
    catch (...) {
        return errno_or_single_error();
    }
}

extern "C"
{

REDEMPTION_LIB_EXPORT
char const * terminal_emulator_version() noexcept
{
    return RVT_LIB_VERSION;
}

#define return_nullptr_if(x) do { if (REDEMPTION_UNLIKELY(x)) { return nullptr; } } while (0)
#define return_if(x) do { if (REDEMPTION_UNLIKELY(x)) { return -2; } } while (0)
#define Panic(expr, err) do { try { expr; } \
    catch (...) { return err; } } while (0)
#define Panic_errno(expr) do { try { expr; } \
    catch (std::bad_alloc const&) { return -3; } \
    catch (...) { return errno_or_single_error(); } } while (0)

REDEMPTION_LIB_EXPORT
TerminalEmulator * terminal_emulator_new(int lines, int columns) noexcept
{
    return_nullptr_if(lines <= 0 || columns <= 0);
    Panic(return new(std::nothrow) TerminalEmulator(lines, columns), nullptr);
}

REDEMPTION_LIB_EXPORT
int terminal_emulator_delete(TerminalEmulator * emu) noexcept
{
    delete emu;
    return 0;
}

REDEMPTION_LIB_EXPORT
int terminal_emulator_finish(TerminalEmulator * emu) noexcept
{
    return_if(!emu);

    auto send_fn = [emu](rvt::ucs4_char ucs) { emu->emulator.receiveChar(ucs); };
    Panic_errno(emu->decoder.end_decode(send_fn));
    return 0;
}


REDEMPTION_LIB_EXPORT
int terminal_emulator_set_title(TerminalEmulator * emu, char const * title) noexcept
{
    return_if(!emu);

    rvt::ucs4_char ucs_title[255];
    rvt::ucs4_char * p = std::begin(ucs_title);
    rvt::ucs4_char * e = std::end(ucs_title) - 4;

    std::size_t sz = strlen(title);

    auto send_fn = [&p](rvt::ucs4_char ucs) { *p = ucs; ++p; };
    while (sz && p < e) {
        std::size_t const consumed = std::min(sz, std::size_t(e-p));
        emu->decoder.decode(const_bytes_array(title, consumed), send_fn);
        sz -= consumed;
        title += consumed;
    }
    emu->decoder.end_decode(send_fn);

    emu->emulator.setWindowTitle({ucs_title, std::size_t(p-ucs_title)});
    return 0;
}

REDEMPTION_LIB_EXPORT
int terminal_emulator_set_log_function(
    TerminalEmulator * emu, TerminalEmulatorLogFunction * log_func
) noexcept
{
    return_if(!emu);

    Panic_errno(emu->emulator.setLogFunction(log_func));
    return 0;
}

REDEMPTION_LIB_EXPORT
int terminal_emulator_set_log_function_ctx(
    TerminalEmulator * emu, TerminalEmulatorLogFunctionCtx * log_func, void * ctx
) noexcept
{
    return_if(!emu);

    Panic_errno(emu->emulator.setLogFunction([log_func, ctx](char const * s, std::size_t len) {
        log_func(ctx, s, len);
    }));
    return 0;
}

REDEMPTION_LIB_EXPORT
int terminal_emulator_feed(TerminalEmulator * emu, uint8_t const * s, std::size_t len) noexcept
{
    return_if(!emu);

    auto send_fn = [emu](rvt::ucs4_char ucs) { emu->emulator.receiveChar(ucs); };
    Panic_errno(emu->decoder.decode(const_bytes_array(s, len), send_fn));
    return 0;
}

REDEMPTION_LIB_EXPORT
int terminal_emulator_resize(TerminalEmulator * emu, int lines, int columns) noexcept
{
    return_if(!emu);
    return_if(lines <= 0 || columns <= 0);

    if (lines > 4096 || columns > 4096) {
        return ENOMEM;
    }

    Panic_errno(emu->emulator.setScreenSize(lines, columns));
    return 0;
}



REDEMPTION_LIB_EXPORT
TerminalEmulatorBuffer* terminal_emulator_buffer_new() noexcept
{
    return terminal_emulator_buffer_new_with_max_capacity(4u /*Go*/ * 1024 * 1024 * 1024, 0);
}

REDEMPTION_LIB_EXPORT
TerminalEmulatorBuffer* terminal_emulator_buffer_new_with_max_capacity(
    std::size_t max_capacity, std::size_t pre_alloc_len) noexcept
{
    static_assert(noexcept(TerminalEmulatorBufferWithVector(max_capacity)));
    auto* res = new(std::nothrow) TerminalEmulatorBufferWithVector{max_capacity};
    if (res && pre_alloc_len) {
        if (!res->initial_size(pre_alloc_len)) {
            delete res;
            return nullptr;
        }
    }
    return res;
}

REDEMPTION_LIB_EXPORT
TerminalEmulatorBuffer * terminal_emulator_buffer_new_with_custom_allocator(
    void * ctx,
    TerminalEmulatorBufferGetBufferFn * get_buffer_fn,
    TerminalEmulatorBufferExtraMemoryAllocatorFn * extra_memory_allocator_fn,
    TerminalEmulatorBufferSetFinalBufferFn * set_final_buffer_fn,
    TerminalEmulatorBufferClearFn * clear_fn,
    TerminalEmulatorBufferDeleteCtxFn * delete_ctx_fn) noexcept
{
    return new(std::nothrow) TerminalEmulatorBuffer{
        ctx,
        get_buffer_fn,
        extra_memory_allocator_fn,
        set_final_buffer_fn,
        clear_fn,
        delete_ctx_fn,
        [](TerminalEmulatorBuffer* self) noexcept { delete self; }
    };
}

REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_delete(TerminalEmulatorBuffer * buffer) noexcept
{
    return_if(!buffer);
    buffer->delete_ctx_fn(buffer->ctx);
    buffer->delete_self(buffer);
    return 0;
}

REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_prepare(
    TerminalEmulatorBuffer * buffer, TerminalEmulator * emu,
    TerminalEmulatorOutputFormat format
) noexcept
{
    return_if(!buffer || !emu);

    return build_format_string(*buffer, *emu, format, {});
}

REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_prepare2(
    TerminalEmulatorBuffer * buffer, TerminalEmulator * emu,
    TerminalEmulatorOutputFormat format,
    uint8_t const * extra_data, std::size_t extra_data_len
) noexcept
{
    return_if(!buffer || !emu);

    std::string_view extra = {const_bytes_t(extra_data).to_charp(), extra_data_len};
    return build_format_string(*buffer, *emu, format, extra);
}

REDEMPTION_LIB_EXPORT
uint8_t const * terminal_emulator_buffer_get_data(
    TerminalEmulatorBuffer const * buffer, std::size_t * output_len) noexcept
{
    if (REDEMPTION_UNLIKELY(!buffer)) {
        if (output_len) {
            *output_len = 0;
        }
        return nullptr;
    }

    std::size_t output_len2;
    return buffer->get_buffer_fn(buffer->ctx, output_len ? output_len : &output_len2);
}

REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_clear_data(TerminalEmulatorBuffer * buffer) noexcept
{
    return_if(!buffer);
    buffer->clear_fn(buffer->ctx);
    return 0;
}

namespace
{
    struct TranscryptRender
    {
        rvt::RenderingBuffer rendering_buffer;
        std::size_t consumed_buffer = 0;
        time_t time = 0;

        void write_line(rvt::Screen const& screen, size_t y, size_t yend)
        {
            auto partial_buf = transcript_partial_rendering(screen, y, yend, rendering_buffer, consumed_buffer);
            rendering_buffer.buffer = partial_buf.buffer;
            rendering_buffer.length = partial_buf.capacity;
            consumed_buffer = partial_buf.length;
        }

        void write_time()
        {
            if (REDEMPTION_UNLIKELY(rendering_buffer.length - consumed_buffer < 21)) {
                std::size_t capacity = 4 * 1024;
                auto p = start_buffer();
                p = rendering_buffer.allocate(rendering_buffer.ctx, &capacity, p, consumed_buffer);
                if (REDEMPTION_UNLIKELY(not p)) {
                    throw std::bad_alloc();
                }
                rendering_buffer.buffer = bytes_t(p).to_charp();
                rendering_buffer.length = capacity;
            }

            char* p = rendering_buffer.buffer;
            struct tm tm;
            p += strftime(p, 21, "%Y-%m-%d %H:%M:%S ", localtime_r(&time, &tm));
            consumed_buffer = checked_int(p - rendering_buffer.buffer);
        }

        void finalize()
        {
            rendering_buffer.set_final_buffer(rendering_buffer.ctx, start_buffer(), consumed_buffer);
        }

        uint8_t* start_buffer() const
        {
            return bytes_t(rendering_buffer.buffer).to_u8p();
        }
    };
}

REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_prepare_transcript_from_ttyrec_buffer(
    TerminalEmulatorBuffer * buffer,
    uint8_t const * data, std::size_t data_len,
    TerminalEmulatorTranscriptPrefix prefix_type) noexcept
{
    return_if(!buffer);

    TranscryptRender render{buffer->as_rendering_buffer()};

    auto line_saver = [&render](rvt::Screen const& screen, size_t y, size_t yend){
        render.write_line(screen, y, yend);
    };
    auto line_saver_with_datetime = [&render](rvt::Screen const& screen, size_t y, size_t yend){
        render.write_time();
        render.write_line(screen, y, yend);
    };

    auto run = [&]{
        rvt::VtEmulator emu(20, 80,
            (prefix_type == TerminalEmulatorTranscriptPrefix::datetime)
            ? rvt::Screen::LineSaver(line_saver_with_datetime)
            : rvt::Screen::LineSaver(line_saver));
        rvt::Utf8Decoder decoder;
        auto ucs_receiver = [&emu](rvt::ucs4_char ucs) { emu.receiveChar(ucs); };

        auto* data_end = data + data_len;

        auto read_tty_u32 = [](uint8_t const* p) -> uint32_t {
            return p[0] | uint32_t(p[1] << 8) | uint32_t(p[2] << 16) | uint32_t(p[3] << 24);
        };

        while (data != data_end && data_end - data > 12) {
            uint32_t const sec  = read_tty_u32(data);
            //uint32_t const usec = read_tty_u32(data() + 4);
            uint32_t frame_len  = read_tty_u32(data + 8);
            render.time = sec /*+ (usec / 1000000)*/;
            data += 12;

            if (frame_len > data_end - data) {
                break;
            }
            decoder.decode({data, frame_len}, ucs_receiver);
            data += frame_len;
        }

        decoder.end_decode(ucs_receiver);
        render.finalize();

        if (data != data_end) {
            return -1;
        }

        return 0;
    };

    Panic_errno(return run());
}

REDEMPTION_LIB_EXPORT
int terminal_emulator_buffer_prepare_transcript_from_ttyrec_file(
    TerminalEmulatorBuffer * buffer,
    char const * infile,
    TerminalEmulatorTranscriptPrefix prefix_type) noexcept
{
    return_if(!buffer || !infile);

    int fd_in { open(infile, O_RDONLY) };
    if (fd_in == -1) {
        return errno_or_single_error();
    }

    struct FileCloser
    {
        ~FileCloser()
        {
            close(fd);
        }

        int fd;
    };
    FileCloser _file_closer{fd_in};

    struct stat s;
    int status = fstat(fd_in, & s);
    if (status == -1) {
        return errno_or_single_error();
    }

    auto data_len = static_cast<std::size_t>(s.st_size);

    if (data_len == 0) {
        TranscryptRender{buffer->as_rendering_buffer()}.finalize();
        return 0;
    }

    auto* data = static_cast<uint8_t*>(mmap(nullptr, data_len, PROT_READ, MAP_PRIVATE, fd_in, 0));
    if (data == MAP_FAILED) {
        return errno_or_single_error();
    }

    int res = terminal_emulator_buffer_prepare_transcript_from_ttyrec_buffer(
        buffer, data, data_len, prefix_type
    );
    munmap(data, data_len);
    return res;
}

} // extern "C"
