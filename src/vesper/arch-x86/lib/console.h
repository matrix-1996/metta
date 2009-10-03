//
// Copyright 2007 - 2009, Stanislav Karchebnyy <berkus+metta@madfire.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "panic.h"

#define endl console_t::eol

enum Color {
    BLACK = 0,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHTGRAY,
    DARKGRAY=8,
    LIGHTBLUE,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTRED,
    LIGHTMAGENTA,
    YELLOW,
    WHITE
};

/*!
* Abstract base class for console output.
*/
class console_t
{
public:
    static const char eol;

    virtual void set_color(Color col) = 0;
    virtual void set_background(Color col) = 0;
    virtual void set_attr(Color fore, Color back) = 0;// e.g. GREEN on BLACK

    virtual void clear() = 0;
    virtual void locate(int row, int col) = 0;
    virtual void scroll_up() = 0;
    virtual void newline() = 0;

    virtual void print_int(int n) = 0;
    virtual void print_char(char ch) = 0;
    virtual void print_byte(unsigned char n) = 0;
    virtual void print_hex(uint32_t n) = 0;
    virtual void print_hex8(uint64_t n) = 0;
    virtual void print_str(const char *s) = 0;

    // Wrappers for template version
    inline void print(int n) { print_int(n); }
    inline void print(char ch) { print_char(ch); }
    inline void print(unsigned char n) { print_byte(n); }
    inline void print(unsigned int n) { print_hex(n); }
    inline void print(void *p) { print((unsigned int)p); }
    inline void print(unsigned long long n) { print_hex8(n); }
    inline void print(const char* str) { print_str(str); }
    template<typename T, typename... Args>
    void print(const char* str, T value, Args... args);

    virtual void wait_ack() = 0;

    virtual void debug_log(const char *str, ...) = 0;
    virtual void checkpoint(const char *str) = 0; // FIXME: move to debugger_t?

protected:
    console_t();
};

// Define stream io on console.
inline console_t& operator << (console_t& con, Color data)
{
    con.set_color(data);
    return con;
}

inline console_t& operator << (console_t& con, const char* data)
{
    con.print_str(data);
    return con;
}

inline console_t& operator << (console_t& con, int data)
{
    con.print_int(data);
    return con;
}

inline console_t& operator << (console_t& con, unsigned int data)
{
    con.print_hex(data);
    return con;
}

inline console_t& operator << (console_t& con, char data)
{
    con.print_char(data);
    return con;
}

inline console_t& operator << (console_t& con, unsigned char data)
{
    con.print_byte(data);
    return con;
}

inline console_t& operator << (console_t& con, const void* data)
{
    con.print_hex((uint32_t)data);
    return con;
}

template<typename T, typename... Args>
void console_t::print(const char* str, T value, Args... args)
{
    while (*str)
    {
        if (*str == '%' && *(++str) != '%')
        {
            print(value);
            print(*str ? ++str : str, args...); // call even when *s == 0 to detect extra arguments
            return;
        }
        print(*str++);
    }
    PANIC("console: extra arguments provided to print");
}

// kate: indent-width 4; replace-tabs on;
// vim: set et sw=4 ts=4 sts=4 cino=(4 :