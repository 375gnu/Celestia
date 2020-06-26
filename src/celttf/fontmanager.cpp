// fontmanager.cpp
//
// Copyright (C) 2020, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <fmt/printf.h>
#include "fontmanager.h"
#include "truetypefont.h"

namespace celestia
{
namespace engine
{
static FontManager* fontManager = nullptr;

FontManager* FontManager::Initialize(const Renderer *renderer)
{
    if (fontManager != nullptr)
        return fontManager;

    FT_Library ft = nullptr;
    if (FT_Init_FreeType(&ft) != 0)
    {
        std::cout << "Could not initialize freetype library\n";
        return nullptr;
    }

    fontManager = new FontManager(renderer, ft);
    return fontManager;
}

FontManager* FontManager::Get()
{
    return fontManager;
}

FT_Face FontManager::getFontFace(const fs::path &path, int index)
{
    auto cmp = [&path, index](const FontFace &f) {
        return f.path == path && f.index == index;
    };
    auto f = std::find_if(begin(m_faces), end(m_faces), cmp);
    if (f != end(m_faces))
    {
        f->count++;
        return f->face;
    }

    FT_Face face;
    if (FT_New_Face(m_ft, path.c_str(), index, &face) != 0)
    {
        fmt::fprintf(std::cout, "Could not open font %s\n", path);
        return nullptr;
    }

    if (!FT_IS_SCALABLE(face))
    {
        fmt::fprintf(std::cout, "Font is not scalable: %s\n", path);
        FT_Done_Face(face);
        return nullptr;
    }

    m_faces.emplace_back(path, index, face);
    return face;
}

TextureFont* FontManager::getFont(const fs::path &path, int index, int size, int dpi)
{
    auto cmp = [&path, index, size, dpi](const FontCacheEntry &e)
    {
        return e.path == path && e.index == index && e.size == size && e.dpi == dpi;
    };
    auto f = std::find_if(begin(m_fonts), end(m_fonts), cmp);
    if (f != end(m_fonts))
    {
        f->count++;
        return f->font;
    }

    auto face = getFontFace(path, index);

    if (FT_Set_Char_Size(face, 0, size << 6, dpi, dpi) != 0)
    {
        fmt::fprintf(std::cout, "Could not font size %i\n", size);
        return nullptr;
    }

    auto font = TextureFont::Load(this, m_renderer, face, size, dpi);
    m_fonts.emplace_back(path, index, size, dpi, face, font);

    return font;
}

void FontManager::releaseFont(const TextureFont *font)
{
    auto cmp = [font](const FontCacheEntry &e) { return e.font == font; };
    auto f = std::find_if(begin(m_fonts), end(m_fonts), cmp);
    if (f == end(m_fonts))
    {
        fmt::fprintf(std::cerr, "Font %p is not found in cache!\n", (void*)font);
        return;
    }

    assert(f->count > 0);
    if (f->count == 1)
    {
        FT_Face face = f->face;
        m_fonts.erase(f);

        releaseFace(face);
    }
    else
    {
        f->count--;
    }
}

void FontManager::releaseFace(FT_Face face)
{
    auto cmp = [face](const FontFace &e) { return e.face == face; };
    auto f = std::find_if(begin(m_faces), end(m_faces), cmp);
    if (f == end(m_faces))
    {
        fmt::fprintf(std::cerr, "Font face %p is not found in cache!\n", (void*)face);
        return;
    }

    assert(f->count > 0);
    if (f->count == 1)
    {
        FT_Done_Face(face);
        m_faces.erase(f);
    }
    else
    {
        f->count--;
    }
}

}
}
