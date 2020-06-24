// fontmanager.h
//
// Copyright (C) 2020, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <memory> // shared_ptr
#include <vector>
#include <celcompat/filesystem.h>
#include <celutil/util.h>

class Renderer;
class TextureFont;

namespace celestia
{
namespace engine
{

class FontManager : public util::noncopyable
{
 public:
    static FontManager* Initialize(const Renderer*);
    static FontManager* Get();

    TextureFont* getFont(const fs::path &path, int index = 0, int size = 0, int dpi = 96);

 private:
    FontManager() = delete;
    FontManager(const Renderer *renderer, FT_Library ft) :
        m_renderer(renderer),
        m_ft(ft)
    {
    };

    FT_Face getFontFace(const fs::path &path, int index);

    const Renderer *m_renderer  { nullptr };
    FT_Library      m_ft        { nullptr };

    struct FontFace
    {
        FontFace(const fs::path &path,
                 int index,
                 FT_Face face) :
            path(path),
            index(index),
            face(face)
        {}
        fs::path    path;
        int         index;
        FT_Face     face;
    };
    struct FontCacheEntry
    {
        FontCacheEntry(const fs::path &path,
                       int index,
                       int size,
                       int dpi,
                       TextureFont *font) :
            path(path),
            index(index),
            size(size),
            dpi(dpi),
            font(font)
        {}
        fs::path    path;
        int         index;
        int         size;
        int         dpi;
        TextureFont *font;
    };
    std::vector<FontFace>       m_faces;
    std::vector<FontCacheEntry> m_fonts;
};

}
}
