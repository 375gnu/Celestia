#include <utility>
#include <vector>

#include <fmt/format.h>
#include <imgui.h>

#include <celengine/glsupport.h>
#include <celengine/body.h>
#include <celengine/marker.h>
#include <celengine/simulation.h>
#include <celengine/solarsys.h>
#include <celengine/universe.h>
#include <celestia/celestiacore.h>
#include <celestia/helper.h>
#include <celutil/gettext.h>

#include "sdlcontextmenu.h"

namespace celestia
{
namespace
{
std::vector<std::pair<std::string, MarkerRepresentation>>
getMarkerTypes()
{
    return {
        { _("Diamond"), MarkerRepresentation(MarkerRepresentation::Diamond, 10.0f, Color(0.0f, 1.0f, 0.0f, 0.9f)) },
        { _("Triangle"), MarkerRepresentation(MarkerRepresentation::Triangle, 10.0f, Color(0.0f, 1.0f, 0.0f, 0.9f)) },
        { _("Square"), MarkerRepresentation(MarkerRepresentation::Square, 10.0f, Color(0.0f, 1.0f, 0.0f, 0.9f)) },
        { _("Filled square"), MarkerRepresentation(MarkerRepresentation::FilledSquare, 10.0f, Color(0.0f, 1.0f, 0.0f, 0.9f)) },
        { _("Plus"), MarkerRepresentation(MarkerRepresentation::Plus, 10.0f, Color(0.0f, 1.0f, 0.0f, 0.9f)) },
        { _("Cross"), MarkerRepresentation(MarkerRepresentation::X, 10.0f, Color(0.0f, 1.0f, 0.0f, 0.9f)) },
        { _("Left arrow"), MarkerRepresentation(MarkerRepresentation::LeftArrow, 10.0f, Color(0.0f, 1.0f, 0.0f, 0.9f)) },
        { _("Right arrow"), MarkerRepresentation(MarkerRepresentation::RightArrow, 10.0f, Color(0.0f, 1.0f, 0.0f, 0.9f)) },
        { _("Up arrow"), MarkerRepresentation(MarkerRepresentation::UpArrow, 10.0f, Color(0.0f, 1.0f, 0.0f, 0.9f)) },
        { _("Down arrow"), MarkerRepresentation(MarkerRepresentation::DownArrow, 10.0f, Color(0.0f, 1.0f, 0.0f, 0.9f)) },
        { _("Circle"), MarkerRepresentation(MarkerRepresentation::Circle, 10.0f, Color(0.0f, 1.0f, 0.0f, 0.9f)) },
        { _("Disk"), MarkerRepresentation(MarkerRepresentation::Disk, 10.0f, Color(0.0f, 1.0f, 0.0f, 0.9f)) },
    };
}

std::vector<std::pair<std::string, std::string>>
getReferenceMarks()
{
    return {
        { _("Show body axes"), "body axes" },
        { _("Show frame axes"), "frame axes" },
        { _("Show sun direction"), "sun direction" },
        { _("Show velocity vector"), "velocity vector" },
        { _("Show spin vector"), "spin vector" },
        { _("Show planetographic grid"), "planetographic grid" },
        { _("Show terminator"), "terminator" },
    };
}

void
renderReferenceMarkMenu(CelestiaCore* appCore, const Selection& selection)
{
    if (ImGui::BeginMenu(_("Reference marks")))
    {
        static auto referenceMarks = getReferenceMarks();
        for (const auto& referenceMark : referenceMarks)
        {
            bool flag = appCore->referenceMarkEnabled(referenceMark.second, selection);
            if (ImGui::MenuItem(referenceMark.first.c_str(), nullptr, flag))
            {
                appCore->toggleReferenceMark(referenceMark.second, selection);
            }
        }
        Selection center = selection.body()->getOrbitFrame(appCore->getSimulation()->getTime())->getCenter();
        if (center.body() != nullptr)
        {
            // Only show the frame center menu item if the selection orbits another
            // a non-stellar object. If it orbits a star, this is generally identical
            // to the sun direction entry.
            std::string itemName = fmt::format(_("Show direction to {}"), center.body()->getName(true));
            bool flag = appCore->referenceMarkEnabled("frame center direction", selection);
            if (ImGui::MenuItem(itemName.c_str(), nullptr, flag))
            {
                appCore->toggleReferenceMark("frame center direction", selection);
            }
        }
        ImGui::EndMenu();
    }
}

void renderObjectsMenu(Simulation* simulation, const std::vector<Body*>& bodies, const char* name)
{
    if (!bodies.empty() && ImGui::BeginMenu(name))
    {
        for (Body* body : bodies)
        {
            if (ImGui::MenuItem(body->getName(true).c_str())) { simulation->setSelection(body); }
        }
        ImGui::EndMenu();
    }
}

void renderPlanetarySystemMenus(Simulation* simulation, const PlanetarySystem* planetarySystem)
{
    if (planetarySystem == nullptr) { return; }
    int nObjects = planetarySystem->getSystemSize();
    static std::vector<Body*> planets{};
    static std::vector<Body*> dwarfPlanets{};
    static std::vector<Body*> moons{};
    static std::vector<Body*> minorMoons{};
    static std::vector<Body*> asteroids{};
    static std::vector<Body*> comets{};
    static std::vector<Body*> spacecraft{};
    static std::vector<Body*> other{};
    planets.clear();
    dwarfPlanets.clear();
    moons.clear();
    minorMoons.clear();
    asteroids.clear();
    comets.clear();
    spacecraft.clear();
    other.clear();
    for (int i = 0; i < nObjects; ++i)
    {
        Body* body = planetarySystem->getBody(i);
        switch (body->getClassification())
        {
        case Body::Planet: planets.push_back(body); break;
        case Body::DwarfPlanet: dwarfPlanets.push_back(body); break;
        case Body::Moon: moons.push_back(body); break;
        case Body::MinorMoon: minorMoons.push_back(body); break;
        case Body::Asteroid: asteroids.push_back(body); break;
        case Body::Comet: comets.push_back(body); break;
        case Body::Spacecraft: spacecraft.push_back(body); break;
        default: other.push_back(body); break;
        }
    }

    renderObjectsMenu(simulation, planets, _("Planets"));
    renderObjectsMenu(simulation, dwarfPlanets, _("Dwarf planets"));
    renderObjectsMenu(simulation, moons, _("Moons"));
    renderObjectsMenu(simulation, minorMoons, _("Minor moons"));
    renderObjectsMenu(simulation, asteroids, _("Asteroids"));
    renderObjectsMenu(simulation, comets, _("Comets"));
    renderObjectsMenu(simulation, spacecraft, _("Spacecraft"));
    renderObjectsMenu(simulation, other, _("Other"));
}
}

void renderContextMenu(CelestiaCore* appCore, const Selection& selection)
{
    if (ImGui::BeginPopup("celcontext"))
    {
        Simulation* sim = appCore->getSimulation();
        Universe* univ = sim->getUniverse();
        if (selection.body() != nullptr)
        {
            ImGui::MenuItem(selection.body()->getName(true).c_str(), nullptr, false, false);
        }
        else if (selection.star() != nullptr)
        {
            ImGui::MenuItem(univ->getStarCatalog()->getStarName(*selection.star(), true).c_str(), nullptr, false, false);
        }
        else if (selection.deepsky() != nullptr)
        {
            ImGui::MenuItem(univ->getDSOCatalog()->getDSOName(selection.deepsky(), true).c_str(), nullptr, false, false);
        }
        ImGui::Separator();
        if (ImGui::MenuItem(_("Select"), "S")) { sim->setSelection(selection); }
        if (ImGui::MenuItem(_("Center"), "C")) { sim->setSelection(selection); appCore->charEntered('c'); }
        if (ImGui::MenuItem(_("Goto"), "G")) { sim->setSelection(selection); appCore->charEntered('g'); }
        if (ImGui::MenuItem(_("Follow"), "F")) { sim->setSelection(selection); appCore->charEntered('f'); }
        if (selection.star() == nullptr && selection.deepsky() == nullptr)
        {
            if (ImGui::MenuItem(_("Sync orbit"), "Y")) { sim->setSelection(selection); appCore->charEntered('y'); }
        }
        ImGui::MenuItem(_("Info"), nullptr, false, false); // TODO implement this
        if (selection.body() != nullptr)
        {
            bool isVisible = selection.body()->isVisible();
            if (ImGui::MenuItem(_("Visible"), nullptr, &isVisible)) { selection.body()->setVisible(isVisible); }
        }
        if (ImGui::BeginMenu(_("Markers")))
        {
            static auto markerTypes = getMarkerTypes();
            for (const auto& markerType : markerTypes)
            {
                if (ImGui::MenuItem(markerType.first.c_str())) { univ->markObject(selection, markerType.second, 1); }
            }
            ImGui::Separator();
            bool isMarked = univ->isMarked(selection, 1);
            if (ImGui::MenuItem(_("Unmark"), nullptr, false, isMarked)) { univ->unmarkObject(selection, 1); }
            ImGui::EndMenu();
        }
        if (selection.body() != nullptr)
        {
            renderReferenceMarkMenu(appCore, selection);
            if (Helper::hasPrimary(selection.body()))
            {
                if (ImGui::MenuItem(_("Select primary"))) { sim->setSelection(Helper::getPrimary(selection.body())); }
            }
            PlanetarySystem* planetarySystem = selection.body()->getSatellites();
            renderPlanetarySystemMenus(sim, planetarySystem);
        }
        else if (selection.star() != nullptr)
        {
            SolarSystemCatalog* catalog = univ->getSolarSystemCatalog();
            if (catalog != nullptr)
            {
                auto iter = catalog->find(selection.star()->getIndex());
                if (iter != catalog->end())
                {
                    SolarSystem* solarSystem = iter->second;
                    if (solarSystem != nullptr)
                    {
                        renderPlanetarySystemMenus(sim, solarSystem->getPlanets());
                    }
                }
            }
        }

        ImGui::EndPopup();
    }
}
}
