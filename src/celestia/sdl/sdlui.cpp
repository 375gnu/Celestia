#include <cstdint>

#include <SDL_clipboard.h>

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
#include <celestia/celestiastate.h>
#include <celestia/scriptmenu.h>
#include <celestia/url.h>
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
               CelestiaCore* appCore) :
    m_window(window),
    m_glContext(glContext),
    m_appCore(appCore)
{
}

SDL_UI::~SDL_UI()
{
    delete m_scripts;

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

bool
SDL_UI::update()
{
    bool keepRunning = true;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    keepRunning &= mainMenu();
    if (m_showPreferences) preferences();

    ImGui::Render();

    return keepRunning;
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

bool
SDL_UI::mainMenu()
{
    bool keepRunning = true;
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
                    m_scripts = ScanScriptsDirectory("scripts", false);
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
            if (ImGui::MenuItem("Exit", "CTRL+Q")) { keepRunning = false; }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Navigation"))
        {
            if (ImGui::MenuItem("Select Sun")) { m_appCore->charEntered('h'); }
            if (ImGui::MenuItem("Center selection")) { m_appCore->charEntered('c'); }
            if (ImGui::MenuItem("Goto selection")) { m_appCore->charEntered('g'); }
            if (ImGui::MenuItem("Goto object..")) { }
            if (ImGui::MenuItem("Copy URL/console text", "CTRL+C"))
            {
                CelestiaState appState(m_appCore);
                appState.captureState();
                Url url(appState);
                SDL_SetClipboardText(url.getAsString().c_str());
                m_appCore->flash(_("Copied URL"));
            }
            if (ImGui::MenuItem("Paste URL/console text", "CTRL+V"))
            {
                char* clipboardText = SDL_GetClipboardText();
                if (clipboardText != nullptr && clipboardText[0] != '\0')
                {
                    m_appCore->goToUrl(std::string(clipboardText));
                    m_appCore->flash(_("Pasting URL"));
                }
                SDL_free(clipboardText);
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
            if (ImGui::MenuItem("Full screen", "SHIFT+F11")) { }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Celestia manual")) { }
            ImGui::Separator();
            if (ImGui::MenuItem("OpenGL info")) { }
            if (ImGui::MenuItem("About Celestia")) { }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    m_appCore->setSafeAreaInsets(0, ImGui::GetMainViewport()->WorkPos.y, 0, 0);
    return keepRunning;
}

void
SDL_UI::preferences()
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
                    ImGui::Text("Objects");
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
                    ImGui::Text("Features");
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
                    ImGui::Text("Orbits");
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
                    ImGui::Text("Navigation");
                    ImGui::BeginChild("guidespane", ImVec2(0, 0), true);
                    ImGui::Text("Grids");
                    flag = (renderFlags & Renderer::ShowCelestialSphere) != 0;
                    if (ImGui::Checkbox("Equatorial", &flag)) { renderFlags ^= Renderer::ShowCelestialSphere; }
                    flag = (renderFlags & Renderer::ShowEclipticGrid) != 0;
                    if (ImGui::Checkbox("Ecliptic", &flag)) { renderFlags ^= Renderer::ShowEclipticGrid; }
                    flag = (renderFlags & Renderer::ShowGalacticGrid) != 0;
                    if (ImGui::Checkbox("Galactic", &flag)) { renderFlags ^= Renderer::ShowGalacticGrid; }
                    flag = (renderFlags & Renderer::ShowHorizonGrid) != 0;
                    if (ImGui::Checkbox("Horizontal", &flag)) { renderFlags ^= Renderer::ShowHorizonGrid; }
                    ImGui::Separator();
                    ImGui::Text("Constellations");
                    flag = (renderFlags & Renderer::ShowDiagrams) != 0;
                    if (ImGui::Checkbox("Diagrams", &flag)) { renderFlags ^= Renderer::ShowDiagrams; }
                    flag = (renderFlags & Renderer::ShowBoundaries) != 0;
                    if (ImGui::Checkbox("Boundaries", &flag)) { renderFlags ^= Renderer::ShowBoundaries; }
                    flag = (labelMode & Renderer::I18nConstellationLabels) == 0; // true if no i18n
                    if (ImGui::Checkbox("Latin names", &flag)) { labelMode ^= Renderer::I18nConstellationLabels; }
                    ImGui::Separator();
                    ImGui::Text("Miscellaneous");
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
                    ImGui::Text("Labels");
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
                    ImGui::Text("Locations");
                    ImGui::BeginChild("locspane", ImVec2(0, 0), true);
                    flag = (labelMode & Renderer::LocationLabels) != 0;
                    if (ImGui::Checkbox("Show locations", &flag)) labelMode ^= Renderer::LocationLabels;
                    ImGui::Separator();
                    ImGui::Text("Location types");
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
                    ImGui::Text("Minimum labelled feature size");
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
                    ImGui::Text("Quality");
                    ImGui::BeginChild("qualitypane", ImVec2(0, 0), true);
                    ImGui::Text("Texture resolution");
                    auto resolution = renderer->getResolution();
                    auto oldResolution = resolution;
                    if (ImGui::RadioButton("Low", resolution == 0)) resolution = 0;
                    if (ImGui::RadioButton("Medium", resolution == 1)) resolution = 1;
                    if (ImGui::RadioButton("High", resolution == 2)) resolution = 2;
                    if (resolution != oldResolution) renderer->setResolution(resolution);
                    ImGui::Separator();
                    ImGui::Text("Illumination");
                    float ambientLight = renderer->getAmbientLightLevel();
                    if (ImGui::SliderFloat("Ambient", &ambientLight, 0.0, 1.0)) renderer->setAmbientLightLevel(ambientLight);
                    bool flag = (renderFlags & Renderer::ShowTintedIllumination) != 0;
                    if (ImGui::Checkbox("Tinted illumination", &flag)) renderFlags ^= Renderer::ShowTintedIllumination;
                    ImGui::Separator();
                    ImGui::Text("Antialiasing");
                    flag = (renderFlags & Renderer::ShowSmoothLines) != 0;
                    if (ImGui::Checkbox("Antialiased lines", &flag)) renderFlags ^= Renderer::ShowSmoothLines;
                    ImGui::EndChild();
                    ImGui::TableNextColumn();
                    ImGui::Text("Stars");
                    ImGui::BeginChild("starspane", ImVec2(0, 0), true);
                    ImGui::Text("Style");
                    auto starStyle = renderer->getStarStyle();
                    if (ImGui::RadioButton("Points", starStyle == Renderer::PointStars)) renderer->setStarStyle(Renderer::PointStars);
                    if (ImGui::RadioButton("Fuzzy points", starStyle == Renderer::FuzzyPointStars)) renderer->setStarStyle(Renderer::FuzzyPointStars);
                    if (ImGui::RadioButton("Scaled discs", starStyle == Renderer::ScaledDiscStars)) renderer->setStarStyle(Renderer::ScaledDiscStars);
                    ImGui::Separator();
                    flag = (renderFlags & Renderer::ShowAutoMag) != 0;
                    if (ImGui::Checkbox("Auto-magnitude", &flag)) renderFlags ^= Renderer::ShowAutoMag;
                    ImGui::Separator();
                    ImGui::Text("Star colors");
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
                ImGui::Text("Date display format");
                auto timeFormat = m_appCore->getDateFormat();
#ifndef _WIN32
                if (ImGui::RadioButton("Local format", timeFormat == astro::Date::Locale)) m_appCore->setDateFormat(astro::Date::Locale);
#endif
                if (ImGui::RadioButton("Time zone name", timeFormat == astro::Date::TZName)) m_appCore->setDateFormat(astro::Date::TZName);
                if (ImGui::RadioButton("UTC offset", timeFormat == astro::Date::UTCOffset)) m_appCore->setDateFormat(astro::Date::UTCOffset);
                ImGui::Separator();
                ImGui::Text("Color scheme");
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
