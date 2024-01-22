#pragma once
#include <cstdint>
#include <cstdio>

// #define CONSOLE_COLORS_FORCE_ASCII

#if defined _WIN32 && !defined PRINT_COLORED_FORCE_ASCII
#define USE_WIN32_API
#endif

#if defined(USE_WIN32_API)

namespace win32_lite
{
typedef void* HANDLE;
typedef uint32_t DWORD;

#define WIN32_LITE_STD_INPUT_HANDLE (static_cast<win32_lite::DWORD>(-10))
#define WIN32_LITE_STD_OUTPUT_HANDLE (static_cast<win32_lite::DWORD>(-11))
#define WIN32_LITE_STD_ERROR_HANDLE (static_cast<win32_lite::DWORD>(-12))

#define WIN32_LITE_ENABLE_VIRTUAL_TERMINAL_PROCESSING (4)

#define WIN32_LITE_DECLSPEC_IMPORT __declspec(dllimport)

#define WIN32_LITE_WINAPI __stdcall

typedef short SHORT;
typedef unsigned short WORD;
typedef int WINBOOL;

extern "C"
{
    WIN32_LITE_DECLSPEC_IMPORT WINBOOL WIN32_LITE_WINAPI GetConsoleMode(HANDLE hConsole, DWORD* dwMode);
    WIN32_LITE_DECLSPEC_IMPORT WINBOOL WIN32_LITE_WINAPI SetConsoleMode(HANDLE hConsole, DWORD dwMode);
    WIN32_LITE_DECLSPEC_IMPORT HANDLE WIN32_LITE_WINAPI GetStdHandle(DWORD nStdHandle);
    WIN32_LITE_DECLSPEC_IMPORT WINBOOL WIN32_LITE_WINAPI SetConsoleTextAttribute(HANDLE hConsoleOutput,
                                                                                 WORD wAttributes);
}
} // namespace win32_lite

#endif

namespace console_colors
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
    text_color get(console_buffer = ConsoleStdOutput) { return saved_color(); }

    void set(text_color new_color, console_buffer console = ConsoleStdOutput)
    {
#ifdef USE_WIN32_API
        win32_lite::SetConsoleTextAttribute(win32_lite::GetStdHandle(console == ConsoleStdOutput
                                                                         ? WIN32_LITE_STD_OUTPUT_HANDLE
                                                                         : WIN32_LITE_STD_ERROR_HANDLE),
                                            static_cast<win32_lite::WORD>(new_color));
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
            std::fprintf(console == ConsoleStdOutput ? stdout : stderr, "\x1B[%d;%dm", tnum, bnum);
        }
        else
        {
            std::fprintf(console == ConsoleStdOutput ? stdout : stderr, "\x1B[0m");
        }
#endif
        saved_color() = new_color;
    }

    text_color m_old;
    console_buffer m_console;
    static text_color& saved_color()
    {
        static text_color color = Normal;
        return color;
    }
};

template <text_color color, console_buffer console = ConsoleStdOutput>
struct console_color_tpl : public console_color
{
public:
    console_color_tpl() : console_color(color, console) {}

private:
};

typedef console_color_tpl<DarkBlue> darkblue_text;
typedef console_color_tpl<DarkGreen> darkgreen_text;
typedef console_color_tpl<DarkCyan> darkcyan_text;
typedef console_color_tpl<DarkRed> darkred_text;
typedef console_color_tpl<DarkMagenta> darkmagenta_text;
typedef console_color_tpl<DarkYellow> darkyellow_text;
typedef console_color_tpl<LightGrey> lightgrey_text;
typedef console_color_tpl<Gray> gray_text;
typedef console_color_tpl<Blue> blue_text;
typedef console_color_tpl<Green> green_text;
typedef console_color_tpl<Cyan> cyan_text;
typedef console_color_tpl<Red> red_text;
typedef console_color_tpl<Magenta> magenta_text;
typedef console_color_tpl<Yellow> yellow_text;
typedef console_color_tpl<White> white_text;
} // namespace console_colors
