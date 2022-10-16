#include <climits>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <SDL_version.h>

#include <fmt/format.h>

#include <imgui.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl.h>

#include <celengine/astro.h>
#include <celengine/render.h>
#include <celestia/scriptmenu.h>
#include <celutil/gettext.h>

#include "sdlcontextmenu.h"
#include "sdlinfoviewer.h"
#include "sdlpreferences.h"
#include "sdlui.h"

namespace celestia
{
namespace
{
std::string
getCompilerVersion()
{
#if defined(_MSC_VER)
    return fmt::format("MSVC {}.{}.{}", _MSC_VER / 100, _MSC_VER % 100, _MSC_FULL_VER % 10000);
#elif defined(__clang_version__)
    return "Clang " __clang_version__;
#elif defined(__GNUC__)
    return "GNU GCC " __VERSION__;
#else
    return _("Unknown compiler");
#endif
}

std::map<std::string, std::string> getGLInfo(CelestiaCore* appCore)
{
    std::map<std::string, std::string> glInfo{};
    appCore->getRenderer()->getInfo(glInfo);
    return glInfo;
}

std::vector<std::string> getGLExtensions(const std::map<std::string, std::string>& glInfo)
{
    std::vector<std::string> glExtensions{};
    auto iter = glInfo.find("Extensions");
    if (iter != glInfo.end())
    {
        const std::string& extensions = iter->second;
        std::string::size_type old = 0;
        std::string::size_type pos = extensions.find(' ');
        while (pos != std::string::npos)
        {
            glExtensions.push_back(extensions.substr(old, pos - old));
            old = pos + 1;
            pos = extensions.find(' ', old);
        }
    }
    return glExtensions;
}

std::string
getBuildInfo()
{
    std::string s = fmt::format(_("Built for {}-bit CPU)\n"), sizeof(void*) * CHAR_BIT);
    s.append(fmt::format(_("Compiled with {}\n"), getCompilerVersion()));
    s.append(fmt::format(_("Built against SDL {}.{}.{}\n"), SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL));
#if defined(USE_SPICE)
    s.append(_("NAIF kernels are supported\n"));
#else
    s.append(_("NAIF kernels are not supported\n"));
#endif
#if defined(USE_LIBAVIF)
    s.append(_("AVIF images are supported\n"));
#else
    s.append(_("AVIF images are not supported\n"));
#endif
    SDL_version sdlVersion;
    SDL_GetVersion(&sdlVersion);
    s.append(fmt::format(_("Runtime SDL version {}.{}.{}"), +sdlVersion.major, +sdlVersion.minor, +sdlVersion.patch));
    return s;
}
}

SDL_UI::SDL_UI(SDL_Window* window,
               SDL_GLContext* glContext,
               CelestiaCore* appCore,
               bool* cursorVisible,
               bool* isFullscreen) :
    m_window(window),
    m_glContext(glContext),
    m_appCore(appCore),
    m_cursorVisible(cursorVisible),
    m_fullscreen(isFullscreen),
    m_infoViewer(std::make_unique<SDL_InfoViewer>(appCore)),
    m_preferences(std::make_unique<SDL_Preferences>(appCore))
{
}

SDL_UI::~SDL_UI()
{
    if (m_initialized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }
}

bool
SDL_UI::init()
{
    if (!IMGUI_CHECKVERSION()) { return false; }
    if (!ImGui::CreateContext()) { return false; }
    ImGui::StyleColorsDark();
    if (!ImGui_ImplSDL2_InitForOpenGL(m_window, m_glContext)) { return false; }
    if (!ImGui_ImplOpenGL3_Init("#version 100")) { return false; }
    ImGuiIO& io = ImGui::GetIO();
    static const ImWchar ranges[] = {
        0x0020, 0x00ff, // Latin-1
        0x0391, 0x03c9, // Greek letters
        0x0400, 0x04ff, // Cyrillic letters
        0x2032, 0x2033, // Prime, double prime
        0x2070, 0x2079, // Superscript digits
        0,
    };
    ImFontConfig font_cfg;
    font_cfg.GlyphRanges = ranges;
    io.Fonts->AddFontFromFileTTF("fonts/DejaVuSans.ttf", 15.0, nullptr, ranges);
    m_initialized = true;
    m_appCore->setContextMenuHandler(this);
    return true;
}

void
SDL_UI::processEvent(const SDL_Event* event)
{
    ImGui_ImplSDL2_ProcessEvent(event);
}

UIAction
SDL_UI::update()
{
    UIAction action = UIAction::None;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    renderMainMenu(action);
    if (m_showGLInfo) renderGLInfo();
    if (m_showAbout) renderAbout();
    m_preferences->render();

    if (!*m_cursorVisible) { ImGui::SetMouseCursor(ImGuiMouseCursor_None); }

    if (m_openContext) {
        ImGui::OpenPopup("celcontext");
        m_openContext = false;
    }
    m_infoViewer->render();
    renderContextMenu(m_appCore, m_selection, *m_infoViewer);

    ImGui::Render();
    return action;
}

void
SDL_UI::display() const
{
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void
SDL_UI::requestContextMenu(float x, float y, Selection sel)
{
    // can't render the menu here, so trigger it on next loop
    m_selection = sel;
    m_openContext = true;
}

bool
SDL_UI::wantCaptureMouse() const
{
    return ImGui::GetIO().WantCaptureMouse;
}

bool
SDL_UI::wantCaptureKeyboard() const
{
    return ImGui::GetIO().WantCaptureKeyboard;
}

void
SDL_UI::renderMainMenu(UIAction& action)
{
    bool outputViewport = false;
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu(_("File")))
        {
            if (ImGui::MenuItem(_("Grab image"), "F10")) { }
            if (ImGui::MenuItem(_("Capture video"), "SHIFT+F10", false, false)) { }
            ImGui::Separator();
            if (ImGui::MenuItem(_("Open script.."))) { }
            static std::unique_ptr<std::vector<ScriptMenuItem>> scripts(ScanScriptsDirectory("scripts", false));
            if (scripts != nullptr && ImGui::BeginMenu(_("Scripts")))
            {
                for (const ScriptMenuItem& item : *scripts)
                {
                    if (ImGui::MenuItem(item.title.c_str()))
                    {
                        m_appCore->cancelScript();
                        m_appCore->runScript(item.filename);
                    }
                }

                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem(_("Preferences.."))) { m_preferences->show(); }
            if (ImGui::MenuItem(_("Exit"), "CTRL+Q")) { action = UIAction::Quit; }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(_("Navigation")))
        {
            if (ImGui::MenuItem(_("Select Sun"), "H")) { m_appCore->charEntered('h'); }
            if (ImGui::MenuItem(_("Center selection"), "C")) { m_appCore->charEntered('c'); }
            if (ImGui::MenuItem(_("Goto selection"), "G")) { m_appCore->charEntered('g'); }
            if (ImGui::MenuItem(_("Goto object.."))) { }
            bool targetUrl = m_appCore->getTextEnterMode() == CelestiaCore::KbNormal;
            static std::string copyUrl = fmt::format("{}##copyitem", _("Copy URL"));
            static std::string copyConsole = fmt::format("{}##copyitem", _("Copy console text"));
            static std::string pasteUrl = fmt::format("{}##pasteitem", _("Paste URL"));
            static std::string pasteConsole = fmt::format("{}##pasteitem", _("Paste console text"));
            if (ImGui::MenuItem((targetUrl ? copyUrl : copyConsole).c_str(), "CTRL+C"))
            {
                action = UIAction::Copy;
            }
            if (ImGui::MenuItem((targetUrl ? pasteUrl : pasteConsole).c_str(), "CTRL+V"))
            {
                action = UIAction::Paste;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(_("Time")))
        {
            if (ImGui::MenuItem(_("Set time.."))) { }
            bool lightDelayActive = m_appCore->getLightDelayActive();
            if (ImGui::MenuItem(_("Light time delay"), nullptr, &lightDelayActive))
            {
                m_appCore->setLightDelayActive(lightDelayActive);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(_("Bookmarks")))
        {
            if (ImGui::MenuItem(_("Add bookmark.."))) { }
            if (ImGui::MenuItem(_("Organize bookmarks.."))) { }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(_("View")))
        {
            ImGui::MenuItem(_("Celestial browser"), nullptr, false);
            ImGui::MenuItem(_("Eclipse finder"), nullptr, false);
            ImGui::Separator();
            if (ImGui::MenuItem(_("Full screen"), nullptr, *m_fullscreen)) { action = UIAction::ToggleFullscreen; }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(_("Help")))
        {
#if SDL_VERSION_ATLEAST(2, 0, 14)
            if (ImGui::MenuItem(_("Celestia manual"))) { SDL_OpenURL("https://en.wikibooks.org/wiki/Celestia"); }
            ImGui::Separator();
#endif
            if (ImGui::MenuItem(_("OpenGL info"))) { m_showGLInfo = true; }
            if (ImGui::MenuItem(_("About Celestia"))) { m_showAbout = true; }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    m_appCore->setSafeAreaInsets(0, ImGui::GetMainViewport()->WorkPos.y, 0, 0);
}

void
SDL_UI::renderGLInfo()
{
    if (ImGui::Begin(_("OpenGL Info"), &m_showGLInfo))
    {
        static std::map<std::string, std::string> glInfo = getGLInfo(m_appCore);
        static std::vector<std::string> glExtensions = getGLExtensions(glInfo);
        if (glInfo.count("API") > 0 && glInfo.count("APIVersion") > 0)
        {
            ImGui::Text(_("%s version: %s"), glInfo["API"].c_str(), glInfo["APIVersion"].c_str());
        }
        if (glInfo.count("Vendor") > 0)
        {
            ImGui::Text(_("Vendor: %s"), glInfo["Vendor"].c_str());
        }
        if (glInfo.count("Renderer") > 0)
        {
            ImGui::Text(_("Renderer: %s"), glInfo["Renderer"].c_str());
        }
        if (glInfo.count("Language") > 0 && glInfo.count("LanguageVersion") > 0)
        {
            ImGui::Text(_("%s version: %s"), glInfo["Language"].c_str(), glInfo["LanguageVersion"].c_str());
        }
        if (glInfo.count("MaxTextureUnits") > 0)
        {
            ImGui::Text(_("Max simultaneous textures: %s"), glInfo["MaxTextureUnits"].c_str());
        }
        if (glInfo.count("MaxTextureSize") > 0)
        {
            ImGui::Text(_("Max texture size: %s"), glInfo["MaxTextureSize"].c_str());
        }
        if (glInfo.count("PointSizeMax") > 0 && glInfo.count("PointSizeMin") > 0)
        {
            ImGui::Text(_("Point size range: %s - %s"), glInfo["PointSizeMin"].c_str(), glInfo["PointSizeMax"].c_str());
        }
        if (glInfo.count("PointSizeGran") > 0)
        {
            ImGui::Text(_("Point size granularity: %s"), glInfo["PointSizeGran"].c_str());
        }
        if (glInfo.count("MaxCubeMapSize") > 0)
        {
            ImGui::Text(_("Max cube map size: %s"), glInfo["MaxCubeMapSize"].c_str());
        }
        if (glInfo.count("MaxVaryingFloats") > 0)
        {
            ImGui::Text(_("Number of interpolators: %s"), glInfo["MaxVaryingFloats"].c_str());
        }
        if (glInfo.count("MaxAnisotropy") > 0)
        {
            ImGui::Text(_("Max anisotropy filtering: %s"), glInfo["MaxAnisotropy"].c_str());
        }

        if (!glExtensions.empty())
        {
            ImGui::Separator();
            ImGui::TextUnformatted(_("Supported extensions:"));
            for (const std::string& extension : glExtensions)
            {
                ImGui::Text("- %s", extension.c_str());
            }
        }

        ImGui::End();
    }
}

void
SDL_UI::renderAbout()
{
    if (ImGui::Begin(_("About Celestia"), &m_showAbout))
    {
        ImGui::Text(_("Celestia 1.7\nDevelopment snapshot, commit %s"), GIT_COMMIT);
        ImGui::Separator();
        static std::string buildInfo = getBuildInfo();
        ImGui::TextUnformatted(buildInfo.c_str());

        ImGui::TextWrapped(_("Copyright (c) 2001-2021 by the Celestia Development Team."));
        ImGui::TextWrapped(
            _("Celestia is free software. You can redistribute it and/or modify "
              "it under the terms of the GNU General Public License as published "
              "by the Free Software Foundation; either version 2 of the License, "
              "or (at your option) any later version.")
        );

        ImGui::End();
    }
}


}
