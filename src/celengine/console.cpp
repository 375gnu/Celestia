// console.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstring>
#include <cstdarg>
#include <cassert>
#include <algorithm>
#include <celutil/utf8.h>
#include <celmath/geomutil.h>
#include <GL/glew.h>
#include "vecgl.h"
#include "console.h"

using namespace std;
using namespace celmath;

static int pmod(int n, int m)
{
    return n >= 0 ? n % m : m - (-(n + 1) % m) - 1;
}


Console::Console(int _nRows, int _nColumns) :
    IOutput(&sbuf),
    nRows(_nRows),
    nColumns(_nColumns)
{
    sbuf.setConsole(this);
    text = new wchar_t[(nColumns + 1) * nRows];
    for (int i = 0; i < nRows; i++)
        text[(nColumns + 1) * i] = '\0';
}


Console::~Console()
{
    delete[] text;
}


/*! Resize the console log to use the specified number of rows.
 *  Old long entries are preserved in the resize. setRowCount()
 *  returns true if it was able to successfully allocate a new
 *  buffer, and false if there was a problem (out of memory.)
 */
bool Console::setRowCount(int _nRows)
{
    if (_nRows == nRows)
        return true;

    auto* newText = new wchar_t[(nColumns + 1) * _nRows];

    for (int i = 0; i < _nRows; i++)
    {
        newText[(nColumns + 1) * i] = '\0';
    }

    std::copy(newText, newText + (nColumns + 1) * min(_nRows, nRows), text);

    delete[] text;
    text = newText;
    nRows = _nRows;

    return true;
}


void Console::begin()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrix(Ortho2D(0.0f, (float)xscale, 0.0f, (float)yscale));
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0.125f, 0.125f, 0);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void Console::end()
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}


void Console::render(int rowHeight)
{
    if (font == nullptr)
        return;

    glEnable(GL_TEXTURE_2D);
    font->bind();
    glPushMatrix();
    for (int i = 0; i < rowHeight; i++)
    {
        //int r = (nRows - rowHeight + 1 + windowRow + i) % nRows;
        int r = pmod(row + windowRow + i, nRows);
        for (int j = 0; j < nColumns; j++)
        {
            wchar_t ch = text[r * (nColumns + 1) + j];
            if (ch == '\0')
                break;
            font->render(ch);
        }

        // advance to the next line
        glPopMatrix();
        glTranslatef(0.0f, -(1.0f + font->getHeight()), 0.0f);
        glPushMatrix();
    }
    glPopMatrix();
}


void Console::setScale(int w, int h)
{
    xscale = w;
    yscale = h;
}

void Console::newline()
{
    assert(column <= nColumns);
    assert(row < nRows);

    text[row * (nColumns + 1) + column] = '\0';
    row = (row + 1) % nRows;
    column = 0;

    if (autoScroll)
        windowRow = -windowHeight;
}

int Console::getRow() const
{
    return row;
}

int Console::getColumn() const
{
    return column;
}

int Console::getWindowRow() const
{
    return windowRow;
}

void Console::setWindowRow(int _row)
{
    windowRow = _row;
}

void Console::setWindowHeight(int _height)
{
    windowHeight = _height;
}

int Console::getWidth() const
{
    return nColumns;
}

int Console::getHeight() const
{
    return nRows;
}

//
// ConsoleStreamBuf implementation
//
void ConsoleStreamBuf::setConsole(Console* c)
{
    console = c;
}
