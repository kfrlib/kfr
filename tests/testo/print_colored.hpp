#pragma once
#include <cstdint>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace print_colored
{

enum text_color : uint32_t
{
    Black         = 0x00,
    DarkBlue      = 0x01,
    DarkGreen     = 0x02,
    DarkCyan      = 0x03,
    DarkRed       = 0x04,
    DarkMagenta   = 0x05,
    DarkYellow    = 0x06,
    LightGrey     = 0x07,
    Gray          = 0x08,
    Blue          = 0x09,
    Green         = 0x0A,
    Cyan          = 0x0B,
    Red           = 0x0C,
    Magenta       = 0x0D,
    Yellow        = 0x0E,
    White         = 0x0F,
    BgBlack       = 0x00,
    BgDarkBlue    = 0x10,
    BgDarkGreen   = 0x20,
    BgDarkCyan    = 0x30,
    BgDarkRed     = 0x40,
    BgDarkMagenta = 0x50,
    BgDarkYellow  = 0x60,
    BgLightGrey   = 0x70,
    BgGray        = 0x80,
    BgBlue        = 0x90,
    BgGreen       = 0xA0,
    BgCyan        = 0xB0,
    BgRed         = 0xC0,
    BgMagenta     = 0xD0,
    BgYellow      = 0xE0,
    BgWhite       = 0xF0,

    Normal = BgBlack | LightGrey
};

enum console_buffer
{
    ConsoleStdOutput,
    ConsoleStdError
};

#if defined(_WIN32)
typedef HANDLE console_handle_t;

inline console_handle_t console_handle(console_buffer console = ConsoleStdOutput)
{
    static HANDLE con_out = ::GetStdHandle(STD_OUTPUT_HANDLE);
    static HANDLE con_err = ::GetStdHandle(STD_ERROR_HANDLE);
    return console == ConsoleStdOutput ? con_out : con_err;
}

#endif

struct console_color
{
public:
    console_color(text_color c, console_buffer console = ConsoleStdOutput)
        : m_old(get(console)), m_console(console)
    {
        set(c, m_console);
    }

    ~console_color() { set(m_old, m_console); }

private:
    text_color get(console_buffer console = ConsoleStdOutput)
    {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO info;
        ::GetConsoleScreenBufferInfo(console_handle(console), &info);
        return static_cast<text_color>(info.wAttributes & 0xFF);
#else
        return static_color();
#endif
    }

    void set(text_color new_color, console_buffer console = ConsoleStdOutput)
    {
#ifdef _WIN32
        ::SetConsoleTextAttribute(console_handle(console), static_cast<WORD>(new_color));
#else
        if (new_color != Normal)
        {
            uint8_t t    = new_color & 0xF;
            uint8_t b    = (new_color & 0xF0) >> 4;
            uint8_t tnum = 30 + ((t & 1) << 2 | (t & 2) | (t & 4) >> 2);
            uint8_t bnum = 40 + ((b & 1) << 2 | (b & 2) | (b & 4) >> 2);
            if (t & 8)
                tnum += 60;
            if (b & 8)
                bnum += 60;
            printf("\x1B[%d;%dm", tnum, bnum);
        }
        else
        {
            printf("\x1B[0m");
        }
        static_color() = new_color;
#endif
    }

    text_color m_old;
    console_buffer m_console;
#ifndef _WIN32
    static text_color& static_color()
    {
        static text_color color = Normal;
        return color;
    }
#endif
};

template <text_color color, console_buffer console = ConsoleStdOutput>
struct colored_text_tpl : public console_color
{
public:
    colored_text_tpl() : console_color(color, console) {}

private:
};

typedef colored_text_tpl<DarkBlue> darkblue_text;
typedef colored_text_tpl<DarkGreen> darkgreen_text;
typedef colored_text_tpl<DarkCyan> darkcyan_text;
typedef colored_text_tpl<DarkRed> darkred_text;
typedef colored_text_tpl<DarkMagenta> darkmagenta_text;
typedef colored_text_tpl<DarkYellow> darkyellow_text;
typedef colored_text_tpl<LightGrey> lightgrey_text;
typedef colored_text_tpl<Gray> gray_text;
typedef colored_text_tpl<Blue> blue_text;
typedef colored_text_tpl<Green> green_text;
typedef colored_text_tpl<Cyan> cyan_text;
typedef colored_text_tpl<Red> red_text;
typedef colored_text_tpl<Magenta> magenta_text;
typedef colored_text_tpl<Yellow> yellow_text;
typedef colored_text_tpl<White> white_text;
}
