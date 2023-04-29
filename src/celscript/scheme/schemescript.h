// schemescript.h
//
// Copyright (C) 2023, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <iosfwd>
#include <memory>
#include <string>

#include <celcompat/filesystem.h>

#include <celscript/common/script.h>

namespace celestia::scripts
{

class SchemeScript : public IScript
{
 public:
    SchemeScript(CelestiaCore*);
    ~SchemeScript() override = default;

    bool load(std::istream&, const fs::path&, std::string&);

    bool tick(double) override;

 private:
    CelestiaCore *m_appCore;
    std::unique_ptr<Execution> m_runningScript;
    std::unique_ptr<ExecutionEnvironment> m_execEnv;

    friend class SchemeScriptPlugin;
};

class SchemeScriptPlugin : public IScriptPlugin
{
 public:
    SchemeScriptPlugin() = delete;
    SchemeScriptPlugin(CelestiaCore *appCore) : IScriptPlugin(appCore) {};
    ~SchemeScriptPlugin() override = default;
    SchemeScriptPlugin(const SchemeScriptPlugin&) = delete;
    SchemeScriptPlugin(SchemeScriptPlugin&&) = delete;
    SchemeScriptPlugin& operator=(const SchemeScriptPlugin&) = delete;
    SchemeScriptPlugin& operator=(SchemeScriptPlugin&&) = delete;

    bool isOurFile(const fs::path&) const override;
    std::unique_ptr<IScript> loadScript(const fs::path&) override;
};

} // end namespace celestia::scripts
