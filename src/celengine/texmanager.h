// texmanager.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _TEXMANAGER_H_
#define _TEXMANAGER_H_

#include <tuple> // for std::tie
#include <celutil/resmanager.h>
#include <celengine/texture.h>
#include "multitexture.h"

class TextureInfo : public ResourceInfo<Texture>
{
 public:
    fs::path source;
    fs::path path;
    unsigned flags;
    float bumpHeight;
    unsigned resolution;

    enum
    {
        WrapTexture      = 0x1,
        CompressTexture  = 0x2,
        NoMipMaps        = 0x4,
        AutoMipMaps      = 0x8,
        AllowSplitting   = 0x10,
        BorderClamp      = 0x20,
        sRGBTexture      = 0x40,
    };

    TextureInfo(const fs::path &source,
                const fs::path &path,
                float bumpHeight,
                unsigned flags,
                unsigned resolution = medres) :
        source(source),
        path(path),
        flags(flags),
        bumpHeight(bumpHeight),
        resolution(resolution)
    {
    }

    TextureInfo(const fs::path &source,
                const fs::path &path,
                unsigned flags,
                unsigned resolution = medres) :
    TextureInfo(source, path, 0.0f, flags, resolution)
    {
    }

    TextureInfo(const fs::path &source,
                unsigned flags,
                unsigned resolution = medres) :
        TextureInfo(source, {}, 0.0f, flags, resolution)
    {
    }

    fs::path resolve(const fs::path&) override;
    Texture* load(const fs::path&) override;
};

inline bool operator<(const TextureInfo& ti0, const TextureInfo& ti1)
{
    return std::tie(ti0.resolution, ti0.source, ti0.path)
         < std::tie(ti1.resolution, ti1.source, ti1.path);
}

typedef ResourceManager<TextureInfo> TextureManager;

TextureManager* GetTextureManager();

#endif // _TEXMANAGER_H_
