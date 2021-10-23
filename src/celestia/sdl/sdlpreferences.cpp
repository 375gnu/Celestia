#include <cstdint>

#include <celutil/gettext.h>
#include <celengine/glsupport.h>
#include <celengine/observer.h>
#include <celengine/render.h>
#include <celengine/simulation.h>
#include <celestia/celestiacore.h>

#include "sdlpreferences.h"

namespace celestia
{
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

SDL_Preferences::SDL_Preferences(CelestiaCore* appCore) :
    m_appCore(appCore)
{
}

void
SDL_Preferences::show()
{
    m_visible = true;
}

void
SDL_Preferences::render()
{
    if (!m_visible) { return; }
    if (ImGui::Begin(_("Preferences"), &m_visible, ImGuiWindowFlags_NoScrollbar))
    {
        if (ImGui::BeginTabBar("##preftabs"))
        {
            Renderer* renderer = m_appCore->getRenderer();
            auto renderFlags = renderer->getRenderFlags();
            auto oldRenderFlags = renderFlags;
            auto labelMode = renderer->getLabelMode();
            auto oldLabelMode = labelMode;
            objectsTab(renderFlags);
            guidesTab(renderFlags, labelMode);
            labelsTab(labelMode);
            renderTab(renderFlags);
            displayTab();
            if (renderFlags != oldRenderFlags) renderer->setRenderFlags(renderFlags);
            if (labelMode != oldLabelMode) renderer->setLabelMode(labelMode);
            ImGui::EndTabBar();
        }
        ImGui::End();
    }
}

void
SDL_Preferences::objectsTab(std::uint64_t& renderFlags)
{
    if (ImGui::BeginTabItem(_("Objects")))
    {
        if (ImGui::BeginTable("##objprefs", 2))
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Objects"));
            ImGui::BeginChild("objectpane", ImVec2(0, 0), true);
            bool flag = (renderFlags & Renderer::ShowStars) != 0;
            if (ImGui::Checkbox(_("Stars"), &flag)) { renderFlags ^= Renderer::ShowStars; }
            flag = (renderFlags & Renderer::ShowPlanets) != 0;
            if (ImGui::Checkbox(_("Planets"), &flag)) { renderFlags ^= Renderer::ShowPlanets; }
            flag = (renderFlags & Renderer::ShowDwarfPlanets) != 0;
            if (ImGui::Checkbox(_("Dwarf planets"), &flag)) { renderFlags ^= Renderer::ShowDwarfPlanets; }
            flag = (renderFlags & Renderer::ShowMoons) != 0;
            if (ImGui::Checkbox(_("Moons"), &flag)) { renderFlags ^= Renderer::ShowMoons; }
            flag = (renderFlags & Renderer::ShowMinorMoons) != 0;
            if (ImGui::Checkbox(_("Minor moons"), &flag)) { renderFlags ^= Renderer::ShowMinorMoons; }
            flag = (renderFlags & Renderer::ShowAsteroids) != 0;
            if (ImGui::Checkbox(_("Asteroids"), &flag)) { renderFlags ^= Renderer::ShowAsteroids; }
            flag = (renderFlags & Renderer::ShowComets) != 0;
            if (ImGui::Checkbox(_("Comets"), &flag)) { renderFlags ^= Renderer::ShowComets; }
            flag = (renderFlags & Renderer::ShowSpacecrafts) != 0;
            if (ImGui::Checkbox(_("Spacecraft"), &flag)) { renderFlags ^= Renderer::ShowSpacecrafts; }
            flag = (renderFlags & Renderer::ShowGalaxies) != 0;
            if (ImGui::Checkbox(_("Galaxies"), &flag)) { renderFlags ^= Renderer::ShowGalaxies; }
            flag = (renderFlags & Renderer::ShowNebulae) != 0;
            if (ImGui::Checkbox(_("Nebulae"), &flag)) { renderFlags ^= Renderer::ShowNebulae; }
            flag = (renderFlags & Renderer::ShowOpenClusters) != 0;
            if (ImGui::Checkbox(_("Open clusters"), &flag)) { renderFlags ^= Renderer::ShowOpenClusters; }
            flag = (renderFlags & Renderer::ShowGlobulars) != 0;
            if (ImGui::Checkbox(_("Globular clusters"), &flag)) { renderFlags ^= Renderer::ShowGlobulars; }
            ImGui::EndChild();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Features"));
            ImGui::BeginChild("featurespane", ImVec2(0, 0), true);
            flag = (renderFlags & Renderer::ShowAtmospheres) != 0;
            if (ImGui::Checkbox(_("Atmospheres"), &flag)) { renderFlags ^= Renderer::ShowAtmospheres; }
            flag = (renderFlags & Renderer::ShowCloudMaps) != 0;
            if (ImGui::Checkbox(_("Cloud maps"), &flag)) { renderFlags ^= Renderer::ShowCloudMaps; }
            flag = (renderFlags & Renderer::ShowCloudShadows) != 0;
            if (ImGui::Checkbox(_("Cloud shadows"), &flag)) { renderFlags ^= Renderer::ShowCloudShadows; }
            flag = (renderFlags & Renderer::ShowEclipseShadows) != 0;
            if (ImGui::Checkbox(_("Eclipse shadows"), &flag)) { renderFlags ^= Renderer::ShowEclipseShadows; }
            flag = (renderFlags & Renderer::ShowPlanetRings) != 0;
            if (ImGui::Checkbox(_("Ring systems"), &flag)) { renderFlags ^= Renderer::ShowPlanetRings; }
            flag = (renderFlags & Renderer::ShowRingShadows) != 0;
            if (ImGui::Checkbox(_("Ring shadows"), &flag)) { renderFlags ^= Renderer::ShowRingShadows; }
            flag = (renderFlags & Renderer::ShowNightMaps) != 0;
            if (ImGui::Checkbox(_("Nightside lights"), &flag)) { renderFlags ^= Renderer::ShowNightMaps; }
            flag = (renderFlags & Renderer::ShowCometTails) != 0;
            if (ImGui::Checkbox(_("Comet tails"), &flag)) { renderFlags ^= Renderer::ShowCometTails; }
            Observer* observer = m_appCore->getSimulation()->getActiveObserver();
            flag = observer->getDisplayedSurface() == "limit of knowledge";
            if (ImGui::Checkbox(_("Limit of knowledge textures"), &flag)) { observer->setDisplayedSurface(flag ? "limit of knowledge" : ""); }
            ImGui::EndChild();
            ImGui::EndTable();
        }
        ImGui::EndTabItem();
    }
}

void
SDL_Preferences::guidesTab(std::uint64_t& renderFlags, int& labelMode)
{
    if (ImGui::BeginTabItem(_("Guides")))
    {
        if (ImGui::BeginTable("##guideprefs", 2))
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Orbits"));
            ImGui::BeginChild("orbitspane", ImVec2(0, 0), true);
            bool flag = (renderFlags & Renderer::ShowOrbits) != 0;
            if (ImGui::Checkbox(_("Show orbits"), &flag)) { renderFlags ^= Renderer::ShowOrbits; }
            flag = (renderFlags & Renderer::ShowFadingOrbits) != 0;
            if (ImGui::Checkbox(_("Fading orbits"), &flag)) { renderFlags ^= Renderer::ShowFadingOrbits; }
            flag = (renderFlags & Renderer::ShowPartialTrajectories) != 0;
            if (ImGui::Checkbox(_("Partial trajectories"), &flag)) { renderFlags ^= Renderer::ShowPartialTrajectories; }
            ImGui::Separator();
            auto orbitMask = m_appCore->getRenderer()->getOrbitMask();
            auto oldOrbitMask = orbitMask;
            flag = (orbitMask & Body::Stellar) != 0;
            if (ImGui::Checkbox(_("Stars"), &flag)) { orbitMask ^= Body::Stellar; }
            flag = (orbitMask & Body::Planet) != 0;
            if (ImGui::Checkbox(_("Planets"), &flag)) { orbitMask ^= Body::Planet; }
            flag = (orbitMask & Body::DwarfPlanet) != 0;
            if (ImGui::Checkbox(_("Dwarf planets"), &flag)) { orbitMask ^= Body::DwarfPlanet; }
            flag = (orbitMask & Body::Moon) != 0;
            if (ImGui::Checkbox(_("Moons"), &flag)) { orbitMask ^= Body::Moon; }
            flag = (orbitMask & Body::MinorMoon) != 0;
            if (ImGui::Checkbox(_("Minor moons"), &flag)) { orbitMask ^= Body::MinorMoon; }
            flag = (orbitMask & Body::Asteroid) != 0;
            if (ImGui::Checkbox(_("Asteroids"), &flag)) { orbitMask ^= Body::Asteroid; }
            flag = (orbitMask & Body::Comet) != 0;
            if (ImGui::Checkbox(_("Comets"), &flag)) { orbitMask ^= Body::Comet; }
            flag = (orbitMask & Body::Spacecraft) != 0;
            if (ImGui::Checkbox(_("Spacecraft"), &flag)) { orbitMask ^= Body::Spacecraft; }
            if (orbitMask != oldOrbitMask) { m_appCore->getRenderer()->setOrbitMask(orbitMask); }
            ImGui::EndChild();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Navigation"));
            ImGui::BeginChild("guidespane", ImVec2(0, 0), true);
            ImGui::TextUnformatted(_("Grids"));
            flag = (renderFlags & Renderer::ShowCelestialSphere) != 0;
            if (ImGui::Checkbox(_("Equatorial"), &flag)) { renderFlags ^= Renderer::ShowCelestialSphere; }
            flag = (renderFlags & Renderer::ShowEclipticGrid) != 0;
            if (ImGui::Checkbox(_("Ecliptic"), &flag)) { renderFlags ^= Renderer::ShowEclipticGrid; }
            flag = (renderFlags & Renderer::ShowGalacticGrid) != 0;
            if (ImGui::Checkbox(_("Galactic"), &flag)) { renderFlags ^= Renderer::ShowGalacticGrid; }
            flag = (renderFlags & Renderer::ShowHorizonGrid) != 0;
            if (ImGui::Checkbox(_("Horizontal"), &flag)) { renderFlags ^= Renderer::ShowHorizonGrid; }
            ImGui::Separator();
            ImGui::TextUnformatted(_("Constellations"));
            flag = (renderFlags & Renderer::ShowDiagrams) != 0;
            if (ImGui::Checkbox(_("Diagrams"), &flag)) { renderFlags ^= Renderer::ShowDiagrams; }
            flag = (renderFlags & Renderer::ShowBoundaries) != 0;
            if (ImGui::Checkbox(_("Boundaries"), &flag)) { renderFlags ^= Renderer::ShowBoundaries; }
            flag = (labelMode & Renderer::I18nConstellationLabels) == 0; // true if no i18n
            if (ImGui::Checkbox(_("Latin names"), &flag)) { labelMode ^= Renderer::I18nConstellationLabels; }
            ImGui::Separator();
            ImGui::TextUnformatted(_("Miscellaneous"));
            flag = (renderFlags & Renderer::ShowMarkers) != 0;
            if (ImGui::Checkbox(_("Markers"), &flag)) { renderFlags ^= Renderer::ShowMarkers; }
            flag = (renderFlags & Renderer::ShowEcliptic) != 0;
            if (ImGui::Checkbox(_("Ecliptic line"), &flag)) { renderFlags ^= Renderer::ShowEcliptic; }
            ImGui::EndChild();
            ImGui::EndTable();
        }
        ImGui::EndTabItem();
    }
}

void
SDL_Preferences::labelsTab(int& labelMode)
{
    if (ImGui::BeginTabItem(_("Labels")))
    {
        if (ImGui::BeginTable("##labelprefs", 2))
        {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 2.0f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 3.0f);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Labels"));
            ImGui::BeginChild("labelspane", ImVec2(0, 0), true);
            bool flag = (labelMode & Renderer::StarLabels) != 0;
            if (ImGui::Checkbox(_("Stars"), &flag)) { labelMode ^= Renderer::StarLabels; }
            flag = (labelMode & Renderer::PlanetLabels) != 0;
            if (ImGui::Checkbox(_("Planets"), &flag)) { labelMode ^= Renderer::PlanetLabels; }
            flag = (labelMode & Renderer::DwarfPlanetLabels) != 0;
            if (ImGui::Checkbox(_("Dwarf planets"), &flag)) { labelMode ^= Renderer::DwarfPlanetLabels; }
            flag = (labelMode & Renderer::MoonLabels) != 0;
            if (ImGui::Checkbox(_("Moons"), &flag)) labelMode ^= Renderer::MoonLabels;
            flag = (labelMode & Renderer::MinorMoonLabels) != 0;
            if (ImGui::Checkbox(_("Minor moons"), &flag)) labelMode ^= Renderer::MinorMoonLabels;
            flag = (labelMode & Renderer::AsteroidLabels) != 0;
            if (ImGui::Checkbox(_("Asteroids"), &flag)) labelMode ^= Renderer::AsteroidLabels;
            flag = (labelMode & Renderer::CometLabels) != 0;
            if (ImGui::Checkbox(_("Comets"), &flag)) labelMode ^= Renderer::CometLabels;
            flag = (labelMode & Renderer::SpacecraftLabels) != 0;
            if (ImGui::Checkbox(_("Spacecraft"), &flag)) labelMode ^= Renderer::SpacecraftLabels;
            flag = (labelMode & Renderer::GalaxyLabels) != 0;
            if (ImGui::Checkbox(_("Galaxies"), &flag)) labelMode ^= Renderer::GalaxyLabels;
            flag = (labelMode & Renderer::NebulaLabels) != 0;
            if (ImGui::Checkbox(_("Nebulae"), &flag)) labelMode ^= Renderer::NebulaLabels;
            flag = (labelMode & Renderer::OpenClusterLabels) != 0;
            if (ImGui::Checkbox(_("Open clusters"), &flag)) labelMode ^= Renderer::OpenClusterLabels;
            flag = (labelMode & Renderer::GlobularLabels) != 0;
            if (ImGui::Checkbox(_("Globular clusters"), &flag)) labelMode ^= Renderer::GlobularLabels;
            flag = (labelMode & Renderer::ConstellationLabels) != 0;
            if (ImGui::Checkbox(_("Constellations"), &flag)) labelMode ^= Renderer::ConstellationLabels;
            ImGui::EndChild();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Locations"));
            ImGui::BeginChild("locspane", ImVec2(0, 0), true);
            flag = (labelMode & Renderer::LocationLabels) != 0;
            if (ImGui::Checkbox(_("Show locations"), &flag)) labelMode ^= Renderer::LocationLabels;
            ImGui::Separator();
            ImGui::TextUnformatted(_("Location types"));
            if (ImGui::BeginTable("##loctypes", 2))
            {
                Observer* observer = m_appCore->getSimulation()->getActiveObserver();
                auto locFilter = observer->getLocationFilter();
                auto oldLocFilter = locFilter;
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                flag = (locFilter & Location::City) != 0;
                if (ImGui::Checkbox(_("Cities"), &flag)) locFilter ^= Location::City;
                flag = (locFilter & Location::Observatory) != 0;
                if (ImGui::Checkbox(_("Observatories"), &flag)) locFilter ^= Location::Observatory;
                flag = (locFilter & Location::LandingSite) != 0;
                if (ImGui::Checkbox(_("Landing sites"), &flag)) locFilter ^= Location::LandingSite;
                flag = (locFilter & Location::Mons) != 0;
                if (ImGui::Checkbox(_("Montes (mountains)"), &flag)) locFilter ^= Location::Mons;
                flag = (locFilter & Location::Mare) != 0;
                if (ImGui::Checkbox(_("Maria (seas)"), &flag)) locFilter ^= Location::Mare;
                flag = (locFilter & Location::Crater) != 0;
                ImGui::TableNextColumn();
                if (ImGui::Checkbox(_("Craters"), &flag)) locFilter ^= Location::Crater;
                flag = (locFilter & Location::Vallis) != 0;
                if (ImGui::Checkbox(_("Valles (valleys)"), &flag)) locFilter ^= Location::Vallis;
                flag = (locFilter & Location::Terra) != 0;
                if (ImGui::Checkbox(_("Terrae (land masses)"), &flag)) locFilter ^= Location::Terra;
                flag = (locFilter & Location::EruptiveCenter) != 0;
                if (ImGui::Checkbox(_("Volcanoes"), &flag)) locFilter ^= Location::EruptiveCenter;
                flag = (locFilter & FilterOtherLocations) != 0;
                if (ImGui::Checkbox(_("Other features"), &flag))
                    locFilter = (locFilter & ~FilterOtherLocations) | (flag ? FilterOtherLocations : 0);
                if (locFilter != oldLocFilter) { observer->setLocationFilter(locFilter); }
                ImGui::EndTable();
            }
            ImGui::Separator();
            ImGui::TextUnformatted(_("Minimum labelled feature size"));
            float featureSize = m_appCore->getRenderer()->getMinimumFeatureSize();
            if (ImGui::SliderFloat(_("pixels"), &featureSize, 0.01f, 1000.0f, "%.2f", ImGuiSliderFlags_Logarithmic))
            {
                m_appCore->getRenderer()->setMinimumFeatureSize(featureSize);
            }
            ImGui::EndChild();
            ImGui::EndTable();
        }
        ImGui::EndTabItem();
    }
}

void
SDL_Preferences::renderTab(std::uint64_t& renderFlags)
{
    if (ImGui::BeginTabItem(_("Render")))
    {
        if (ImGui::BeginTable("##rendertable", 2))
        {
            Renderer* renderer = m_appCore->getRenderer();
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Quality"));
            ImGui::BeginChild("qualitypane", ImVec2(0, 0), true);
            ImGui::TextUnformatted(_("Texture resolution"));
            auto resolution = renderer->getResolution();
            auto oldResolution = resolution;
            if (ImGui::RadioButton(_("Low"), resolution == 0)) resolution = 0;
            if (ImGui::RadioButton(_("Medium"), resolution == 1)) resolution = 1;
            if (ImGui::RadioButton(_("High"), resolution == 2)) resolution = 2;
            if (resolution != oldResolution) renderer->setResolution(resolution);
            ImGui::Separator();
            ImGui::TextUnformatted(_("Illumination"));
            float ambientLight = renderer->getAmbientLightLevel();
            if (ImGui::SliderFloat(_("Ambient"), &ambientLight, 0.0, 1.0)) renderer->setAmbientLightLevel(ambientLight);
            bool flag = (renderFlags & Renderer::ShowTintedIllumination) != 0;
            if (ImGui::Checkbox(_("Tinted illumination"), &flag)) renderFlags ^= Renderer::ShowTintedIllumination;
            ImGui::Separator();
            ImGui::TextUnformatted(_("Antialiasing"));
            flag = (renderFlags & Renderer::ShowSmoothLines) != 0;
            if (ImGui::Checkbox(_("Antialiased lines"), &flag)) renderFlags ^= Renderer::ShowSmoothLines;
            ImGui::EndChild();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Stars"));
            ImGui::BeginChild("starspane", ImVec2(0, 0), true);
            ImGui::TextUnformatted(_("Style"));
            auto starStyle = renderer->getStarStyle();
            if (ImGui::RadioButton(_("Points"), starStyle == Renderer::PointStars)) renderer->setStarStyle(Renderer::PointStars);
            if (ImGui::RadioButton(_("Fuzzy points"), starStyle == Renderer::FuzzyPointStars)) renderer->setStarStyle(Renderer::FuzzyPointStars);
            if (ImGui::RadioButton(_("Scaled discs"), starStyle == Renderer::ScaledDiscStars)) renderer->setStarStyle(Renderer::ScaledDiscStars);
            ImGui::Separator();
            flag = (renderFlags & Renderer::ShowAutoMag) != 0;
            if (ImGui::Checkbox(_("Auto-magnitude"), &flag)) renderFlags ^= Renderer::ShowAutoMag;
            ImGui::Separator();
            ImGui::TextUnformatted(_("Star colors"));
            auto starColorTable = renderer->getStarColorTable();
            auto starColorsD65 = GetStarColorTable(ColorTable_Blackbody_D65);
            auto starColorsClassic = GetStarColorTable(ColorTable_Enhanced);
            if (ImGui::RadioButton(_("Blackbody D65"), starColorTable == starColorsD65)) renderer->setStarColorTable(starColorsD65);
            if (ImGui::RadioButton(_("Classic"), starColorTable == starColorsClassic)) renderer->setStarColorTable(starColorsClassic);
            ImGui::EndChild();
            ImGui::EndTable();
        }
        ImGui::EndTabItem();
    }
}

void
SDL_Preferences::displayTab()
{
    if (ImGui::BeginTabItem(_("Display")))
    {
        ImGui::TextUnformatted(_("Date display format"));
        auto timeFormat = m_appCore->getDateFormat();
#ifndef _WIN32
        if (ImGui::RadioButton(_("Local format"), timeFormat == astro::Date::Locale)) m_appCore->setDateFormat(astro::Date::Locale);
#endif
        if (ImGui::RadioButton(_("Time zone name"), timeFormat == astro::Date::TZName)) m_appCore->setDateFormat(astro::Date::TZName);
        if (ImGui::RadioButton(_("UTC offset"), timeFormat == astro::Date::UTCOffset)) m_appCore->setDateFormat(astro::Date::UTCOffset);
        ImGui::Separator();
        ImGui::TextUnformatted(_("Color scheme"));
        if (ImGui::RadioButton(_("Dark"), m_colorScheme == ImGuiColorScheme::Dark))
        {
            m_colorScheme = ImGuiColorScheme::Dark;
            ImGui::StyleColorsDark();
        }
        if (ImGui::RadioButton(_("Light"), m_colorScheme == ImGuiColorScheme::Light))
        {
            m_colorScheme = ImGuiColorScheme::Light;
            ImGui::StyleColorsLight();
        }
        if (ImGui::RadioButton(_("Classic"), m_colorScheme == ImGuiColorScheme::Classic))
        {
            m_colorScheme = ImGuiColorScheme::Classic;
            ImGui::StyleColorsClassic();
        }
        ImGui::EndTabItem();
    }
}

}
