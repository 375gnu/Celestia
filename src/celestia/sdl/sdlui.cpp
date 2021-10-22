#include <climits>
#include <cstdint>

#include <SDL_version.h>

#include <imgui.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl.h>

#include <celengine/glsupport.h>
#include <celengine/astro.h>
#include <celengine/body.h>
#include <celengine/location.h>
#include <celengine/observer.h>
#include <celengine/render.h>
#include <celengine/starcolors.h>
#include <celestia/celestiacore.h>
#include <celestia/scriptmenu.h>
#include <celutil/gettext.h>

#include "sdlui.h"

namespace
{
constexpr uint64_t FilterOtherLocations =
    ~(Location::City |
      Location::Observatory |
      Location::LandingSite |
      Location::Mons |
      Location::Mare |
      Location::Crater |
      Location::Vallis |
      Location::Terra |
      Location::EruptiveCenter);
}

namespace celestia
{

SDL_UI::SDL_UI(SDL_Window* window,
               SDL_GLContext* glContext,
               CelestiaCore* appCore,
               bool* cursorVisible,
               bool* isFullscreen) :
    m_window(window),
    m_glContext(glContext),
    m_appCore(appCore),
    m_cursorVisible(cursorVisible),
    m_fullscreen(isFullscreen)
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
    m_initialized = true;
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
    if (m_showPreferences) renderPreferences();

    if (!*m_cursorVisible) { ImGui::SetMouseCursor(ImGuiMouseCursor_None); }

    ImGui::Render();

    return action;
}

void
SDL_UI::display() const
{
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Grab image", "F10")) { }
            if (ImGui::MenuItem("Capture video", "SHIFT+F10", false, false)) { }
            ImGui::Separator();
            if (ImGui::MenuItem("Open script..")) { }
            if (ImGui::BeginMenu("Scripts"))
            {
                if (m_scripts == nullptr)
                {
                    m_scripts = std::unique_ptr<std::vector<ScriptMenuItem>>(ScanScriptsDirectory("scripts", false));
                }

                for (const ScriptMenuItem& item : *m_scripts)
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
            if (ImGui::MenuItem("Preferences..")) { m_showPreferences = true; }
            if (ImGui::MenuItem("Exit", "CTRL+Q")) { action = UIAction::Quit; }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Navigation"))
        {
            if (ImGui::MenuItem("Select Sun")) { m_appCore->charEntered('h'); }
            if (ImGui::MenuItem("Center selection")) { m_appCore->charEntered('c'); }
            if (ImGui::MenuItem("Goto selection")) { m_appCore->charEntered('g'); }
            if (ImGui::MenuItem("Goto object..")) { }
            bool targetUrl = m_appCore->getTextEnterMode() == CelestiaCore::KbNormal;
            if (ImGui::MenuItem(targetUrl ? "Copy URL##copyitem" : "Copy console text##copyitem", "CTRL+C"))
            {
                action = UIAction::Copy;
            }
            if (ImGui::MenuItem(targetUrl ? "Paste URL##pasteitem" : "Paste console text##pasteitem", "CTRL+V"))
            {
                action = UIAction::Paste;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Time"))
        {
            if (ImGui::MenuItem("Set time..")) { }
            bool lightDelayActive = m_appCore->getLightDelayActive();
            if (ImGui::MenuItem("Light time delay", nullptr, &lightDelayActive))
            {
                m_appCore->setLightDelayActive(lightDelayActive);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Bookmarks"))
        {
            if (ImGui::MenuItem("Add bookmark..")) { }
            if (ImGui::MenuItem("Organize bookmarks..")) { }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Celestial browser", nullptr, &m_showCelestialBrowser);
            ImGui::Separator();
            if (ImGui::MenuItem("Full screen", nullptr, *m_fullscreen)) { action = UIAction::ToggleFullscreen; }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Celestia manual")) { SDL_OpenURL("https://en.wikibooks.org/wiki/Celestia"); }
            ImGui::Separator();
            if (ImGui::MenuItem("OpenGL info")) { m_showGLInfo = true; }
            if (ImGui::MenuItem("About Celestia")) { m_showAbout = true; }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    m_appCore->setSafeAreaInsets(0, ImGui::GetMainViewport()->WorkPos.y, 0, 0);
}

void
SDL_UI::renderGLInfo()
{
    if (ImGui::Begin("OpenGL Info", &m_showGLInfo))
    {
        if (m_glInfo.empty())
        {
            m_appCore->getRenderer()->getInfo(m_glInfo);
            if (m_glInfo.count("Extensions") > 0)
            {
                std::string& extensions = m_glInfo["Extensions"];
                std::string::size_type old = 0;
                std::string::size_type pos = extensions.find(' ');
                while (pos != std::string::npos)
                {
                    m_glExtensions.push_back(extensions.substr(old, pos - old));
                    old = pos + 1;
                    pos = extensions.find(' ', old);
                }
            }
        }
        if (m_glInfo.count("API") > 0 && m_glInfo.count("APIVersion") > 0)
        {
            ImGui::Text("%s version: %s", m_glInfo["API"].c_str(), m_glInfo["APIVersion"].c_str());
        }
        if (m_glInfo.count("Vendor") > 0)
        {
            ImGui::Text("Vendor: %s", m_glInfo["Vendor"].c_str());
        }
        if (m_glInfo.count("Renderer") > 0)
        {
            ImGui::Text("Renderer: %s", m_glInfo["Renderer"].c_str());
        }
        if (m_glInfo.count("Language") > 0 && m_glInfo.count("LanguageVersion") > 0)
        {
            ImGui::Text("%s version: %s", m_glInfo["Language"].c_str(), m_glInfo["LanguageVersion"].c_str());
        }
        if (m_glInfo.count("MaxTextureUnits") > 0)
        {
            ImGui::Text("Max simultaneous textures: %s", m_glInfo["MaxTextureUnits"].c_str());
        }
        if (m_glInfo.count("MaxTextureSize") > 0)
        {
            ImGui::Text("Max texture size: %s", m_glInfo["MaxTextureSize"].c_str());
        }
        if (m_glInfo.count("PointSizeMax") > 0 && m_glInfo.count("PointSizeMin") > 0)
        {
            ImGui::Text("Point size range: %s - %s", m_glInfo["PointSizeMin"].c_str(), m_glInfo["PointSizeMax"].c_str());
        }
        if (m_glInfo.count("PointSizeGran") > 0)
        {
            ImGui::Text("Point size granularity: %s", m_glInfo["PointSizeGran"].c_str());
        }
        if (m_glInfo.count("MaxCubeMapSize") > 0)
        {
            ImGui::Text("Max cube map size: %s", m_glInfo["MaxCubeMapSize"].c_str());
        }
        if (m_glInfo.count("MaxVaryingFloats") > 0)
        {
            ImGui::Text("Number of interpolators: %s", m_glInfo["MaxVaryingFloats"].c_str());
        }
        if (m_glInfo.count("MaxAnisotropy") > 0)
        {
            ImGui::Text("Max anisotropy filtering: %s", m_glInfo["MaxAnisotropy"].c_str());
        }

        if (!m_glExtensions.empty())
        {
            ImGui::Separator();
            ImGui::TextUnformatted("Supported extensions:");
            for (const std::string& extension : m_glExtensions)
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
    if (ImGui::Begin("About Celestia", &m_showAbout))
    {
        ImGui::TextUnformatted(
            "Celestia 1.7\n"
            "Development snapshot, commit " GIT_COMMIT);
        ImGui::Separator();
        SDL_version version;
        SDL_GetVersion(&version);
        ImGui::Text(
            "Built for %zu bit CPU\n"
#if defined(_MSC_VER)
            "Using MSVC %d\n"
#elif defined(__clang_version__)
            "Using Clang %s\n"
#elif defined(__GNUC__)
            "Using GNU GCC %s\n"
#else
            "Using unknown compiler\n"
#endif
            "Built against SDL %d.%d.%d\n"
#if defined(USE_SPICE)
            "NAIF kernels are supported\n"
#else
            "NAIF kernels are not supported\n"
#endif
#if defined(USE_LIBAVIF)
            "AVIF images are supported\n"
#else
            "AVIF images are not supported\n"
#endif
            "Runtime SDL version %d.%d.%d",
            sizeof(void*) * CHAR_BIT,
#if defined(_MSC_VER)
            _MSC_FULL_VER,
#elif defined(__clang_version__)
            __clang_version__,
#elif defined(__GNUC__)
            __VERSION__,
#endif
            SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
            +version.major, +version.minor, +version.patch);

        ImGui::TextWrapped("Copyright (c) 2001-2021 by the Celestia Development Team.");
        ImGui::TextWrapped(
            "Celestia is free software. You can redistribute it and/or modify "
            "it under the terms of the GNU General Public License as published "
            "by the Free Software Foundation; either version 2 of the License, "
            "or (at your option) any later version."
        );

        ImGui::End();
    }
}

void
SDL_UI::renderPreferences()
{
    if (ImGui::Begin("Preferences", &m_showPreferences, ImGuiWindowFlags_NoScrollbar))
    {
        Renderer* renderer = m_appCore->getRenderer();
        Observer* observer = m_appCore->getSimulation()->getActiveObserver();
        if (ImGui::BeginTabBar("##preftabs"))
        {
            auto renderFlags = renderer->getRenderFlags();
            auto oldRenderFlags = renderFlags;
            auto labelMode = renderer->getLabelMode();
            auto oldLabelMode = labelMode;
            if (ImGui::BeginTabItem("Objects"))
            {
                if (ImGui::BeginTable("##objprefs", 2))
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Objects");
                    ImGui::BeginChild("objectpane", ImVec2(0, 0), true);
                    bool flag = (renderFlags & Renderer::ShowStars) != 0;
                    if (ImGui::Checkbox("Stars", &flag)) { renderFlags ^= Renderer::ShowStars; }
                    flag = (renderFlags & Renderer::ShowPlanets) != 0;
                    if (ImGui::Checkbox("Planets", &flag)) { renderFlags ^= Renderer::ShowPlanets; }
                    flag = (renderFlags & Renderer::ShowDwarfPlanets) != 0;
                    if (ImGui::Checkbox("Dwarf planets", &flag)) { renderFlags ^= Renderer::ShowDwarfPlanets; }
                    flag = (renderFlags & Renderer::ShowMoons) != 0;
                    if (ImGui::Checkbox("Moons", &flag)) { renderFlags ^= Renderer::ShowMoons; }
                    flag = (renderFlags & Renderer::ShowMinorMoons) != 0;
                    if (ImGui::Checkbox("Minor moons", &flag)) { renderFlags ^= Renderer::ShowMinorMoons; }
                    flag = (renderFlags & Renderer::ShowAsteroids) != 0;
                    if (ImGui::Checkbox("Asteroids", &flag)) { renderFlags ^= Renderer::ShowAsteroids; }
                    flag = (renderFlags & Renderer::ShowComets) != 0;
                    if (ImGui::Checkbox("Comets", &flag)) { renderFlags ^= Renderer::ShowComets; }
                    flag = (renderFlags & Renderer::ShowSpacecrafts) != 0;
                    if (ImGui::Checkbox("Spacecraft", &flag)) { renderFlags ^= Renderer::ShowSpacecrafts; }
                    flag = (renderFlags & Renderer::ShowGalaxies) != 0;
                    if (ImGui::Checkbox("Galaxies", &flag)) { renderFlags ^= Renderer::ShowGalaxies; }
                    flag = (renderFlags & Renderer::ShowNebulae) != 0;
                    if (ImGui::Checkbox("Nebulae", &flag)) { renderFlags ^= Renderer::ShowNebulae; }
                    flag = (renderFlags & Renderer::ShowOpenClusters) != 0;
                    if (ImGui::Checkbox("Open clusters", &flag)) { renderFlags ^= Renderer::ShowOpenClusters; }
                    flag = (renderFlags & Renderer::ShowGlobulars) != 0;
                    if (ImGui::Checkbox("Globular clusters", &flag)) { renderFlags ^= Renderer::ShowGlobulars; }
                    ImGui::EndChild();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Features");
                    ImGui::BeginChild("featurespane", ImVec2(0, 0), true);
                    flag = (renderFlags & Renderer::ShowAtmospheres) != 0;
                    if (ImGui::Checkbox("Atmospheres", &flag)) { renderFlags ^= Renderer::ShowAtmospheres; }
                    flag = (renderFlags & Renderer::ShowCloudMaps) != 0;
                    if (ImGui::Checkbox("Cloud maps", &flag)) { renderFlags ^= Renderer::ShowCloudMaps; }
                    flag = (renderFlags & Renderer::ShowCloudShadows) != 0;
                    if (ImGui::Checkbox("Cloud shadows", &flag)) { renderFlags ^= Renderer::ShowCloudShadows; }
                    flag = (renderFlags & Renderer::ShowEclipseShadows) != 0;
                    if (ImGui::Checkbox("Eclipse shadows", &flag)) { renderFlags ^= Renderer::ShowEclipseShadows; }
                    flag = (renderFlags & Renderer::ShowPlanetRings) != 0;
                    if (ImGui::Checkbox("Ring systems", &flag)) { renderFlags ^= Renderer::ShowPlanetRings; }
                    flag = (renderFlags & Renderer::ShowRingShadows) != 0;
                    if (ImGui::Checkbox("Ring shadows", &flag)) { renderFlags ^= Renderer::ShowRingShadows; }
                    flag = (renderFlags & Renderer::ShowNightMaps) != 0;
                    if (ImGui::Checkbox("Nightside lights", &flag)) { renderFlags ^= Renderer::ShowNightMaps; }
                    flag = (renderFlags & Renderer::ShowCometTails) != 0;
                    if (ImGui::Checkbox("Comet tails", &flag)) { renderFlags ^= Renderer::ShowCometTails; }
                    flag = observer->getDisplayedSurface() == "limit of knowledge";
                    if (ImGui::Checkbox("Limit of knowledge textures", &flag))
                    {
                        observer->setDisplayedSurface(flag ? "limit of knowledge" : "");
                    }
                    ImGui::EndChild();
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Guides"))
            {
                if (ImGui::BeginTable("##guideprefs", 2))
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Orbits");
                    ImGui::BeginChild("orbitspane", ImVec2(0, 0), true);
                    bool flag = (renderFlags & Renderer::ShowOrbits) != 0;
                    if (ImGui::Checkbox("Show orbits", &flag)) { renderFlags ^= Renderer::ShowOrbits; }
                    flag = (renderFlags & Renderer::ShowFadingOrbits) != 0;
                    if (ImGui::Checkbox("Fading orbits", &flag)) { renderFlags ^= Renderer::ShowFadingOrbits; }
                    flag = (renderFlags & Renderer::ShowPartialTrajectories) != 0;
                    if (ImGui::Checkbox("Partial trajectories", &flag)) { renderFlags ^= Renderer::ShowPartialTrajectories; }
                    ImGui::Separator();
                    auto orbitMask = renderer->getOrbitMask();
                    auto oldOrbitMask = orbitMask;
                    flag = (orbitMask & Body::Stellar) != 0;
                    if (ImGui::Checkbox("Stars", &flag)) { orbitMask ^= Body::Stellar; }
                    flag = (orbitMask & Body::Planet) != 0;
                    if (ImGui::Checkbox("Planets", &flag)) { orbitMask ^= Body::Planet; }
                    flag = (orbitMask & Body::DwarfPlanet) != 0;
                    if (ImGui::Checkbox("Dwarf planets", &flag)) { orbitMask ^= Body::DwarfPlanet; }
                    flag = (orbitMask & Body::Moon) != 0;
                    if (ImGui::Checkbox("Moons", &flag)) { orbitMask ^= Body::Moon; }
                    flag = (orbitMask & Body::MinorMoon) != 0;
                    if (ImGui::Checkbox("Minor moons", &flag)) { orbitMask ^= Body::MinorMoon; }
                    flag = (orbitMask & Body::Asteroid) != 0;
                    if (ImGui::Checkbox("Asteroids", &flag)) { orbitMask ^= Body::Asteroid; }
                    flag = (orbitMask & Body::Comet) != 0;
                    if (ImGui::Checkbox("Comets", &flag)) { orbitMask ^= Body::Comet; }
                    flag = (orbitMask & Body::Spacecraft) != 0;
                    if (ImGui::Checkbox("Spacecraft", &flag)) { orbitMask ^= Body::Spacecraft; }
                    if (orbitMask != oldOrbitMask) { renderer->setOrbitMask(orbitMask); }
                    ImGui::EndChild();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Navigation");
                    ImGui::BeginChild("guidespane", ImVec2(0, 0), true);
                    ImGui::TextUnformatted("Grids");
                    flag = (renderFlags & Renderer::ShowCelestialSphere) != 0;
                    if (ImGui::Checkbox("Equatorial", &flag)) { renderFlags ^= Renderer::ShowCelestialSphere; }
                    flag = (renderFlags & Renderer::ShowEclipticGrid) != 0;
                    if (ImGui::Checkbox("Ecliptic", &flag)) { renderFlags ^= Renderer::ShowEclipticGrid; }
                    flag = (renderFlags & Renderer::ShowGalacticGrid) != 0;
                    if (ImGui::Checkbox("Galactic", &flag)) { renderFlags ^= Renderer::ShowGalacticGrid; }
                    flag = (renderFlags & Renderer::ShowHorizonGrid) != 0;
                    if (ImGui::Checkbox("Horizontal", &flag)) { renderFlags ^= Renderer::ShowHorizonGrid; }
                    ImGui::Separator();
                    ImGui::TextUnformatted("Constellations");
                    flag = (renderFlags & Renderer::ShowDiagrams) != 0;
                    if (ImGui::Checkbox("Diagrams", &flag)) { renderFlags ^= Renderer::ShowDiagrams; }
                    flag = (renderFlags & Renderer::ShowBoundaries) != 0;
                    if (ImGui::Checkbox("Boundaries", &flag)) { renderFlags ^= Renderer::ShowBoundaries; }
                    flag = (labelMode & Renderer::I18nConstellationLabels) == 0; // true if no i18n
                    if (ImGui::Checkbox("Latin names", &flag)) { labelMode ^= Renderer::I18nConstellationLabels; }
                    ImGui::Separator();
                    ImGui::TextUnformatted("Miscellaneous");
                    flag = (renderFlags & Renderer::ShowMarkers) != 0;
                    if (ImGui::Checkbox("Markers", &flag)) { renderFlags ^= Renderer::ShowMarkers; }
                    flag = (renderFlags & Renderer::ShowEcliptic) != 0;
                    if (ImGui::Checkbox("Ecliptic line", &flag)) { renderFlags ^= Renderer::ShowEcliptic; }
                    ImGui::EndChild();
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Labels"))
            {
                if (ImGui::BeginTable("##labelprefs", 2))
                {
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 2.0f);
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 3.0f);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Labels");
                    ImGui::BeginChild("labelspane", ImVec2(0, 0), true);
                    bool flag = (labelMode & Renderer::StarLabels) != 0;
                    if (ImGui::Checkbox("Stars", &flag)) { labelMode ^= Renderer::StarLabels; }
                    flag = (labelMode & Renderer::PlanetLabels) != 0;
                    if (ImGui::Checkbox("Planets", &flag)) { labelMode ^= Renderer::PlanetLabels; }
                    flag = (labelMode & Renderer::DwarfPlanetLabels) != 0;
                    if (ImGui::Checkbox("Dwarf planets", &flag)) { labelMode ^= Renderer::DwarfPlanetLabels; }
                    flag = (labelMode & Renderer::MoonLabels) != 0;
                    if (ImGui::Checkbox("Moons", &flag)) labelMode ^= Renderer::MoonLabels;
                    flag = (labelMode & Renderer::MinorMoonLabels) != 0;
                    if (ImGui::Checkbox("Minor moons", &flag)) labelMode ^= Renderer::MinorMoonLabels;
                    flag = (labelMode & Renderer::AsteroidLabels) != 0;
                    if (ImGui::Checkbox("Asteroids", &flag)) labelMode ^= Renderer::AsteroidLabels;
                    flag = (labelMode & Renderer::CometLabels) != 0;
                    if (ImGui::Checkbox("Comets", &flag)) labelMode ^= Renderer::CometLabels;
                    flag = (labelMode & Renderer::SpacecraftLabels) != 0;
                    if (ImGui::Checkbox("Spacecraft", &flag)) labelMode ^= Renderer::SpacecraftLabels;
                    flag = (labelMode & Renderer::GalaxyLabels) != 0;
                    if (ImGui::Checkbox("Galaxies", &flag)) labelMode ^= Renderer::GalaxyLabels;
                    flag = (labelMode & Renderer::NebulaLabels) != 0;
                    if (ImGui::Checkbox("Nebulae", &flag)) labelMode ^= Renderer::NebulaLabels;
                    flag = (labelMode & Renderer::OpenClusterLabels) != 0;
                    if (ImGui::Checkbox("Open clusters", &flag)) labelMode ^= Renderer::OpenClusterLabels;
                    flag = (labelMode & Renderer::GlobularLabels) != 0;
                    if (ImGui::Checkbox("Globular clusters", &flag)) labelMode ^= Renderer::GlobularLabels;
                    flag = (labelMode & Renderer::ConstellationLabels) != 0;
                    if (ImGui::Checkbox("Constellations", &flag)) labelMode ^= Renderer::ConstellationLabels;
                    ImGui::EndChild();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Locations");
                    ImGui::BeginChild("locspane", ImVec2(0, 0), true);
                    flag = (labelMode & Renderer::LocationLabels) != 0;
                    if (ImGui::Checkbox("Show locations", &flag)) labelMode ^= Renderer::LocationLabels;
                    ImGui::Separator();
                    ImGui::TextUnformatted("Location types");
                    if (ImGui::BeginTable("##loctypes", 2))
                    {
                        auto locFilter = observer->getLocationFilter();
                        auto oldLocFilter = locFilter;
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        flag = (locFilter & Location::City) != 0;
                        if (ImGui::Checkbox("Cities", &flag)) locFilter ^= Location::City;
                        flag = (locFilter & Location::Observatory) != 0;
                        if (ImGui::Checkbox("Observatories", &flag)) locFilter ^= Location::Observatory;
                        flag = (locFilter & Location::LandingSite) != 0;
                        if (ImGui::Checkbox("Landing sites", &flag)) locFilter ^= Location::LandingSite;
                        flag = (locFilter & Location::Mons) != 0;
                        if (ImGui::Checkbox("Montes (mountains)", &flag)) locFilter ^= Location::Mons;
                        flag = (locFilter & Location::Mare) != 0;
                        if (ImGui::Checkbox("Maria (seas)", &flag)) locFilter ^= Location::Mare;
                        flag = (locFilter & Location::Crater) != 0;
                        ImGui::TableNextColumn();
                        if (ImGui::Checkbox("Craters", &flag)) locFilter ^= Location::Crater;
                        flag = (locFilter & Location::Vallis) != 0;
                        if (ImGui::Checkbox("Valles (valleys)", &flag)) locFilter ^= Location::Vallis;
                        flag = (locFilter & Location::Terra) != 0;
                        if (ImGui::Checkbox("Terrae (land masses)", &flag)) locFilter ^= Location::Terra;
                        flag = (locFilter & Location::EruptiveCenter) != 0;
                        if (ImGui::Checkbox("Volcanoes", &flag)) locFilter ^= Location::EruptiveCenter;
                        flag = (locFilter & FilterOtherLocations) != 0;
                        if (ImGui::Checkbox("Other features", &flag))
                            locFilter = (locFilter & ~FilterOtherLocations) | (flag ? FilterOtherLocations : 0);
                        if (locFilter != oldLocFilter) observer->setLocationFilter(locFilter);
                        ImGui::EndTable();
                    }
                    ImGui::Separator();
                    ImGui::TextUnformatted("Minimum labelled feature size");
                    float featureSize = renderer->getMinimumFeatureSize();
                    if (ImGui::SliderFloat("pixels", &featureSize, 0.01f, 1000.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) renderer->setMinimumFeatureSize(featureSize);
                    ImGui::EndChild();
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Render"))
            {
                if (ImGui::BeginTable("##rendertable", 2))
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Quality");
                    ImGui::BeginChild("qualitypane", ImVec2(0, 0), true);
                    ImGui::TextUnformatted("Texture resolution");
                    auto resolution = renderer->getResolution();
                    auto oldResolution = resolution;
                    if (ImGui::RadioButton("Low", resolution == 0)) resolution = 0;
                    if (ImGui::RadioButton("Medium", resolution == 1)) resolution = 1;
                    if (ImGui::RadioButton("High", resolution == 2)) resolution = 2;
                    if (resolution != oldResolution) renderer->setResolution(resolution);
                    ImGui::Separator();
                    ImGui::TextUnformatted("Illumination");
                    float ambientLight = renderer->getAmbientLightLevel();
                    if (ImGui::SliderFloat("Ambient", &ambientLight, 0.0, 1.0)) renderer->setAmbientLightLevel(ambientLight);
                    bool flag = (renderFlags & Renderer::ShowTintedIllumination) != 0;
                    if (ImGui::Checkbox("Tinted illumination", &flag)) renderFlags ^= Renderer::ShowTintedIllumination;
                    ImGui::Separator();
                    ImGui::TextUnformatted("Antialiasing");
                    flag = (renderFlags & Renderer::ShowSmoothLines) != 0;
                    if (ImGui::Checkbox("Antialiased lines", &flag)) renderFlags ^= Renderer::ShowSmoothLines;
                    ImGui::EndChild();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Stars");
                    ImGui::BeginChild("starspane", ImVec2(0, 0), true);
                    ImGui::TextUnformatted("Style");
                    auto starStyle = renderer->getStarStyle();
                    if (ImGui::RadioButton("Points", starStyle == Renderer::PointStars)) renderer->setStarStyle(Renderer::PointStars);
                    if (ImGui::RadioButton("Fuzzy points", starStyle == Renderer::FuzzyPointStars)) renderer->setStarStyle(Renderer::FuzzyPointStars);
                    if (ImGui::RadioButton("Scaled discs", starStyle == Renderer::ScaledDiscStars)) renderer->setStarStyle(Renderer::ScaledDiscStars);
                    ImGui::Separator();
                    flag = (renderFlags & Renderer::ShowAutoMag) != 0;
                    if (ImGui::Checkbox("Auto-magnitude", &flag)) renderFlags ^= Renderer::ShowAutoMag;
                    ImGui::Separator();
                    ImGui::TextUnformatted("Star colors");
                    auto starColorTable = renderer->getStarColorTable();
                    auto starColorsD65 = GetStarColorTable(ColorTable_Blackbody_D65);
                    auto starColorsClassic = GetStarColorTable(ColorTable_Enhanced);
                    if (ImGui::RadioButton("Blackbody D65", starColorTable == starColorsD65)) renderer->setStarColorTable(starColorsD65);
                    if (ImGui::RadioButton("Classic", starColorTable == starColorsClassic)) renderer->setStarColorTable(starColorsClassic);
                    ImGui::EndChild();
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Display"))
            {
                ImGui::TextUnformatted("Date display format");
                auto timeFormat = m_appCore->getDateFormat();
#ifndef _WIN32
                if (ImGui::RadioButton("Local format", timeFormat == astro::Date::Locale)) m_appCore->setDateFormat(astro::Date::Locale);
#endif
                if (ImGui::RadioButton("Time zone name", timeFormat == astro::Date::TZName)) m_appCore->setDateFormat(astro::Date::TZName);
                if (ImGui::RadioButton("UTC offset", timeFormat == astro::Date::UTCOffset)) m_appCore->setDateFormat(astro::Date::UTCOffset);
                ImGui::Separator();
                ImGui::TextUnformatted("Color scheme");
                if (ImGui::RadioButton("Dark", m_colorScheme == ImGuiColorScheme::Dark))
                {
                    m_colorScheme = ImGuiColorScheme::Dark;
                    ImGui::StyleColorsDark();
                }
                if (ImGui::RadioButton("Light", m_colorScheme == ImGuiColorScheme::Light))
                {
                    m_colorScheme = ImGuiColorScheme::Light;
                    ImGui::StyleColorsLight();
                }
                if (ImGui::RadioButton("Classic", m_colorScheme == ImGuiColorScheme::Classic))
                {
                    m_colorScheme = ImGuiColorScheme::Classic;
                    ImGui::StyleColorsClassic();
                }
                ImGui::EndTabItem();
            }
            if (renderFlags != oldRenderFlags) renderer->setRenderFlags(renderFlags);
            if (labelMode != oldLabelMode) renderer->setLabelMode(labelMode);
            ImGui::EndTabBar();
        }
        ImGui::End();
    }
}
}
