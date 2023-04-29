// schemescript.cpp
//
// Copyright (C) 2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <fstream>
#include <istream>
#include <celestia/celestiacore.h>
#include <celutil/gettext.h>
#include "schemescript.h"
#include "schemestate.h"


namespace celestia::scripts
{

//
// SchemeScript
//
SchemeScript::SchemeScript(CelestiaCore* appCore) :
    m_appCore(appCore),
    m_state(std::make_unique<SchemeState>())
{
}

bool
SchemeScript::load(std::istream &scriptfile, const fs::path &path, std::string &errorMsg)
{
    if (m_state->loadScript(scriptfile, path) != 0)
    {
        //errorMsg = m_state->getErrorMessage();
        return false;
    }
    return true;
}

bool
SchemeScript::tick(double dt)
{
    return m_state->tick(dt);
}

//
// SchemeScriptPlugin
//
bool
SchemeScriptPlugin::isOurFile(const fs::path &p) const
{
    return p.extension() == ".cels";
}

std::unique_ptr<IScript>
SchemeScriptPlugin::loadScript(const fs::path &path)
{
    std::ifstream scriptfile(path);
    if (!scriptfile.good())
    {
        appCore()->fatalError(_("Error opening script file."));
        return nullptr;
    }

    auto script = std::make_unique<SchemeScript>(appCore());
    if (std::string errorMsg; !script->load(scriptfile, path, errorMsg))
    {
        if (errorMsg.empty())
            errorMsg = _("Unknown error loading script");
        appCore()->fatalError(errorMsg);
        return nullptr;
    }
    return script;
}

} // end namespace celestia::scripts
