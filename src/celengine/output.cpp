// output.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//               2019, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include "output.h"

namespace celestia
{
IOutput::IOutput() :
    ostream(&sbuf)
{
}

void IOutput::begin()
{
}

void IOutput::end()
{
}

void IOutput::setFont(TextureFont* f)
{
    if (f != font)
        font = f;
}

void IOutput::setColor(float r, float g, float b, float a) const
{
    glColor4f(r, g, b, a);
}

void IOutput::setColor(const Color& c) const
{
    glColor4f(c.red(), c.green(), c.blue(), c.alpha());
}

void IOutput::moveBy(float dx, float dy, float dz)
{
    glTranslatef(dx, dy, dz);
}

void IOutput::print(wchar_t c)
{

}

void IOutput::print(char c)
{
    switch (c)
    {
    case '\n':
        newline();
        break;
    default:
        if (column == nColumns)
            newline();
        text[row * (nColumns + 1) + column] = c;
        column++;
        break;
    }
}

void IOutput::print(const char* s)
{
    int length = strlen(s);
    bool validChar = true;
    int i = 0;

    while (i < length && validChar)
    {
        wchar_t ch = 0;
        validChar = UTF8Decode(s, i, length, ch);
        i += UTF8EncodedSize(ch);
        print(ch);
    }
}

//
// IOutputStreamBuf implementation
//
int IOutputStreamBuf::overflow(int c)
{
    if (m_output != nullptr)
    {
        switch (m_decodeState)
        {
        case UTF8DecodeStart:
            if (c < 0x80)
            {
                // Just a normal 7-bit character
                overlay->print((char) c);
            }
            else
            {
                unsigned int count;

                if ((c & 0xe0) == 0xc0)
                    count = 2;
                else if ((c & 0xf0) == 0xe0)
                    count = 3;
                else if ((c & 0xf8) == 0xf0)
                    count = 4;
                else if ((c & 0xfc) == 0xf8)
                    count = 5;
                else if ((c & 0xfe) == 0xfc)
                    count = 6;
                else
                    count = 1; // Invalid byte

                if (count > 1)
                {
                    unsigned int mask = (1 << (7 - count)) - 1;
                    m_decodeShift = (count - 1) * 6;
                    m_decodedChar = (c & mask) << decodeShift;
                    m_decodeState = UTF8DecodeMultibyte;
                }
                else
                {
                    // If the character isn't valid multibyte sequence head,
                    // silently skip it by leaving the decoder state alone.
                }
            }
            break;

        case UTF8DecodeMultibyte:
            if ((c & 0xc0) == 0x80)
            {
                // We have a valid non-head byte in the sequence
                m_decodeShift -= 6;
                m_decodedChar |= (c & 0x3f) << decodeShift;
                if (m_decodeShift == 0)
                {
                    output->print(m_decodedChar);
                    m_decodeState = UTF8DecodeStart;
                }
            }
            else
            {
                // Bad byte in UTF-8 encoded sequence; we'll silently ignore
                // it and reset the state of the UTF-8 decoder.
                m_decodeState = UTF8DecodeStart;
            }
            break;
        }
    }

    return c;
}
}
