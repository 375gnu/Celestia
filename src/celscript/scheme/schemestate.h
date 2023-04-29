#pragma once

#include <iosfwd>
#include <string>
#include <celcompat/filesystem.h>
#include <chibi/eval.h>

class CelestiaCore;

namespace celestia::scripts
{

class SchemeState
{
public:
    SchemeState(CelestiaCore*);
    ~SchemeState();

    bool loadScript(const fs::path&);
    bool loadScript(const std::string&);

private:
    sexp m_ctx;
    CelestiaCore* m_appCore;
};

}
