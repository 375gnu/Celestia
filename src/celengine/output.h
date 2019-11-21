// output.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//               2019, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <iosfwd>

class TextureFont;

namespace celestia
{
class IOutput : public std::ostream
{
 public:
    IOutput();
    virtual ~IOutput() = default;

    void virtual begin();
    void virtual end();
    void setFont(TextureFont* f);
    virtual void print(wchar_t c);
    virtual void print(char c);
    virtual void print(const char* s);
    void setColor(float r, float g, float b, float a) const;
    void setColor(const Color& c) const;
    void moveBy(float dx, float dy, float dz) const;

 private:
    IOutputStreamBuf sbuf;
};

class IOutputStreamBuf : public std::streambuf
{
 public:
    IOutputStreamBuf() = default;
    ~IOutputStreamBuf() = default;
    int overflow(int c = EOF);

 private:
    IOutput *m_output;

    enum UTF8DecodeState
    {
        UTF8DecodeStart     = 0,
        UTF8DecodeMultibyte = 1,
    };
    UTF8DecodeState m_decodeState{ UTF8DecodeStart };
    wchar_t m_decodedChar{ 0 };
    unsigned int m_decodeShift{ 0 };
};
}
