// fontmanager.cpp
//
// Copyright (C) 2020, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <iostream>
#include <fmt/printf.h>
#include <celengine/render.h>
#include "fontmanager.h"
#include "truetypefont.h"

namespace celestia
{
namespace engine
{
namespace /* helpers */
{
// temporary while no fontconfig support
fs::path ParseFontName(const fs::path &filename, int &collectionIndex, int &size)
{
    // Format with font path/collection index(if any)/font size(if any)
    auto fn = filename.string();
    auto pos = fn.rfind(',');
    if (pos != string::npos)
    {
        size = (int) stof(fn.substr(pos + 1));
        auto rest = fn.substr(0, pos);

        pos = rest.rfind(',');
        if (pos != string::npos)
        {
            collectionIndex = stof(rest.substr(pos + 1));
            return rest.substr(0, pos);
        }
        else
        {
            return rest;
        }
    }
    else
    {
        size = 12;
        return filename;
    }
}

} // namespace

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
        return f->face;

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
    int psize = 0, pindex = 0;
    auto nameonly = ParseFontName(path, pindex, psize);
    if (index <= 0)
        index = pindex;
    if (size <= 0)
        size = psize;

    auto cmp = [&nameonly, index, size, dpi](const FontCacheEntry &e) {
        return e.path == nameonly && e.index == index && e.size == size && e.dpi == dpi;
    };
    auto f = std::find_if(begin(m_fonts), end(m_fonts), cmp);
    if (f != end(m_fonts))
        return f->font;

    auto face = getFontFace(nameonly, index);

    if (FT_Set_Char_Size(face, 0, size << 6, dpi, dpi) != 0)
    {
        fmt::fprintf(std::cout, "Could not font size %i\n", size);
        return nullptr;
    }

    auto font = TextureFont::Load(m_renderer, face, size, dpi);
    m_fonts.emplace_back(nameonly, index, size, dpi, font);

    return font;
}

}
}
