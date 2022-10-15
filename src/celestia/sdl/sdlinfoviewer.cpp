#include <algorithm>
#include <cmath>
#include <vector>

#include <SDL.h>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <imgui.h>

#include <celengine/body.h>
#include <celengine/deepskyobj.h>
#include <celengine/glsupport.h>
#include <celengine/star.h>
#include <celengine/universe.h>
#include <celephem/orbit.h>
#include <celephem/rotation.h>
#include <celestia/celestiacore.h>
#include <celmath/mathlib.h>
#include <celutil/gettext.h>
#include <celutil/utf8.h>

#include "sdlinfoviewer.h"

namespace celestia
{
namespace
{
enum class RotationSense
{
    Unknown,
    Prograde,
    Retrograde,
};

void
renderOrbit(const astro::OrbitalElements& elements)
{
    double orbitalPeriod = elements.period;
    const char* unit;
    if (orbitalPeriod < 2.0)
    {
        orbitalPeriod *= 24.0;
        unit = _("hours");
    }
    else if (orbitalPeriod < 365.0 * 2.0)
    {
        unit = _("days");
    }
    else
    {
        orbitalPeriod /= 365.25;
        unit = _("years");
    }
    ImGui::Separator();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted("Period");
    ImGui::TableNextColumn();
    ImGui::Text("%f %s", orbitalPeriod, unit);

    double sma = elements.semimajorAxis;
    if (sma > 2.5e7)
    {
        sma = astro::kilometersToAU(sma);
        unit = _("AU");
    }
    else
    {
        unit = _("km");
    }
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(_("Semimajor axis"));
    ImGui::TableNextColumn();
    ImGui::Text("%f %s", sma, unit);
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(_("Eccentricity"));
    ImGui::TableNextColumn();
    ImGui::Text("%f", elements.eccentricity);
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(_("Pericenter distance"));
    ImGui::TableNextColumn();
    ImGui::Text("%f %s", sma * (1.0 - elements.eccentricity), unit);
    if (elements.eccentricity < 1.0)
    {
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(_("Apocenter distance"));
        ImGui::TableNextColumn();
        ImGui::Text("%f %s", sma * (1.0 + elements.eccentricity), unit);
    }
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(_("Inclination"));
    ImGui::TableNextColumn();
    ImGui::Text("%f" UTF8_DEGREE_SIGN, celmath::radToDeg(elements.inclination));
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(_("Ascending node"));
    ImGui::TableNextColumn();
    ImGui::Text("%f" UTF8_DEGREE_SIGN, celmath::radToDeg(elements.longAscendingNode));
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(_("Arg of pericenter"));
    ImGui::TableNextColumn();
    ImGui::Text("%f" UTF8_DEGREE_SIGN, celmath::radToDeg(elements.argPericenter));
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(_("Mean anomaly"));
    ImGui::TableNextColumn();
    ImGui::Text("%f" UTF8_DEGREE_SIGN, celmath::radToDeg(elements.meanAnomaly));
}

void
renderBody(const Body* body, double tdb)
{
    ImGui::TextUnformatted(body->getName(true).c_str());
#if SDL_VERSION_ATLEAST(2, 0, 14)
    const std::string& url = body->getInfoURL();
    if (!url.empty())
    {
        ImGui::Separator();
        ImGui::TextUnformatted(_("Web info:"));
        if (ImGui::SmallButton(url.c_str())) { SDL_OpenURL(url.c_str()); }
    }
    ImGui::Separator();
#endif
    if (ImGui::BeginTable("##bodyinfo", 2))
    {
        ImGui::TableNextRow();
        float radius = body->getRadius();
        const char* unit;
        if (radius < 1.0f)
        {
            radius *= 1000.0f;
            unit = _("m");
        }
        else
        {
            unit = _("km");
        }
        ImGui::TableNextColumn();
        if (body->isEllipsoid())
        {
            ImGui::TextUnformatted(_("Equatorial radius"));
        }
        else
        {
            ImGui::TextUnformatted(_("Size"));
        }
        ImGui::TableNextColumn();
        ImGui::Text("%f %s", radius, unit);
        double orbitalPeriod = 0.0;
        const Orbit* orbit = body->getOrbit(tdb);
        if (orbit->isPeriodic())
        {
            orbitalPeriod = orbit->getPeriod();
        }
        const RotationModel* rotationModel = body->getRotationModel(tdb);
        if (rotationModel != nullptr && rotationModel->isPeriodic())
        {
            double rotationPeriod = rotationModel->getPeriod();
            double dayLength = 0.0;
            RotationSense sense = RotationSense::Unknown;
            if (orbitalPeriod > 0.0)
            {
                Eigen::Vector3d axis = Eigen::AngleAxisd(rotationModel->equatorOrientationAtTime(tdb)
                                                         * body->getBodyFrame(tdb)->getOrientation(tdb)).axis();
                Eigen::Vector3d orbitNormal = body->getOrbitFrame(tdb)->getOrientation(tdb)
                                              * orbit->positionAtTime(tdb).cross(orbit->velocityAtTime(tdb));
                sense = axis.dot(orbitNormal) >= 0.0 ? RotationSense::Prograde : RotationSense::Retrograde;

                // For now we only calculate day length for objects in direct stellar orbit. Moons would require
                // determination of the parent directly orbiting the star, then determining the rotation sense
                // with respect to that orbit.
                if (body->getSystem() != nullptr && body->getSystem()->getPrimaryBody() == nullptr)
                {
                    double siderealDaysPerYear = orbitalPeriod / rotationPeriod;
                    double solarDaysPerYear = sense == RotationSense::Prograde ? siderealDaysPerYear - 1.0 : siderealDaysPerYear + 1.0;
                    if (std::abs(solarDaysPerYear) > 0.0001)
                    {
                        dayLength = std::abs(orbitalPeriod / solarDaysPerYear);
                    }
                }
            }

            if (rotationPeriod < 2.0)
            {
                rotationPeriod *= 24.0;
                dayLength *= 24.0;
                unit = _("hours");
            }
            else
            {
                unit = _("days");
            }
            ImGui::Separator();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Sidereal rotation"));
            ImGui::TableNextColumn();
            ImGui::Text("%f %s", rotationPeriod, unit);

            if (sense != RotationSense::Unknown)
            {
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(_("Rotation direction"));
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(sense == RotationSense::Prograde ? _("Prograde") : _("Retrograde"));
            }

            if (dayLength > 0.0)
            {
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(_("Day length"));
                ImGui::TableNextColumn();
                ImGui::Text("%f %s", dayLength, unit);
            }
        }
        ImGui::Separator();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(_("Ring system?"));
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(body->getRings() == nullptr ? _("No") : _("Yes"));
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(_("Atmosphere?"));
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(body->getAtmosphere() == nullptr ? _("No") : _("Yes"));
        double startTime;
        double endTime;
        body->getLifespan(startTime, endTime);
        if (startTime > -1.0e9 || endTime < 1.0e9)
        {
            ImGui::Separator();
            if (startTime > -1.0e9)
            {
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(_("Lifetime start"));
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(astro::TDBtoUTC(startTime).toCStr());
            }
            if (endTime < 1.0e9)
            {
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(_("Lifetime end"));
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(astro::TDBtoUTC(endTime).toCStr());
            }
        }

        astro::OrbitalElements elements;
        orbit->calculateOsculatingElements(tdb, orbitalPeriod == 0.0 ? 365.25e-6 : orbitalPeriod * 1.0e-6, &elements);
        renderOrbit(elements);

        ImGui::EndTable();
    }
}

void
renderStar(const Star* star, const Universe* universe, double tdb)
{
    ImGui::TextUnformatted(universe->getStarCatalog()->getStarName(*star, true).c_str());
#if SDL_VERSION_ATLEAST(2, 0, 14)
    const std::string& url = star->getInfoURL();
    if (!url.empty())
    {
        ImGui::Separator();
        ImGui::TextUnformatted(_("Web info:"));
        if (ImGui::SmallButton(url.c_str())) { SDL_OpenURL(url.c_str()); }
    }
    ImGui::Separator();
#endif
    if (ImGui::BeginTable("##starinfo", 2))
    {
        ImGui::TableNextRow();
        Eigen::Vector3d celPos = star->getPosition(tdb).offsetFromKm(UniversalCoord::Zero());
        Eigen::Vector3d eqPos = astro::eclipticToEquatorial(Eigen::Vector3d(celPos.x(), -celPos.z(), celPos.y()));
        double distance = eqPos.norm();
        double ra = celmath::radToDeg(std::atan2(eqPos.y(), eqPos.x()));
        if (ra < 0.0) { ra += 360.0; }
        double dec = celmath::radToDeg(std::asin(eqPos.z() / distance));
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(_("Right ascension"));
        ImGui::TableNextColumn();
        int hourdeg;
        int minutes;
        double seconds;
        astro::decimalToHourMinSec(ra, hourdeg, minutes, seconds);
        ImGui::Text("%dh %dm %fs", hourdeg, minutes, seconds);
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(_("Declination"));
        ImGui::TableNextColumn();
        astro::decimalToDegMinSec(dec, hourdeg, minutes, seconds);
        ImGui::Text("%d" UTF8_DEGREE_SIGN " %d" UTF8_PRIME " %f" UTF8_DOUBLE_PRIME, hourdeg, minutes, seconds);
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(_("Distance from Sol"));
        ImGui::TableNextColumn();
        distance = astro::kilometersToLightYears(distance);
        if (distance < 0.0001) { distance = 0.0; }
        ImGui::Text(_("%f ly"), distance);
        if (star->getVisibility())
        {
            ImGui::Separator();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Spectral type"));
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(star->getSpectralType());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Temperature"));
            ImGui::TableNextColumn();
            ImGui::Text(_("%f K"), star->getTemperature());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Radius"));
            ImGui::TableNextColumn();
            ImGui::Text(_("%f km"), star->getRadius());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(_("Absolute magnitude"));
            ImGui::TableNextColumn();
            ImGui::Text("%f", star->getAbsoluteMagnitude());
        }

        Orbit* orbit = star->getOrbit();
        if (orbit != nullptr)
        {
            double sampleTime = orbit->isPeriodic() ? orbit->getPeriod() * 1.0e-6 : 0.0;
            if (sampleTime == 0.0) { sampleTime = 365.25e-6; }
            astro::OrbitalElements elements;
            orbit->calculateOsculatingElements(tdb, sampleTime, &elements);
            // display semimajor axis with respect to paired star, if there is no paired star
            // then don't display - avoids confusing output for Sol
            const Star* barycenter = star->getOrbitBarycenter();
            if (barycenter != nullptr)
            {
                const std::vector<Star*>* stars = barycenter->getOrbitingStars();
                if (stars != nullptr && stars->size() == 2)
                {
                    auto iter = std::find_if(stars->cbegin(), stars->cend(), [=](Star* s) { return s != star; });
                    if (iter != stars->cend())
                    {
                        astro::OrbitalElements secondaryElements;
                        (*iter)->getOrbit()->calculateOsculatingElements(tdb, sampleTime, &secondaryElements);
                        elements.semimajorAxis += secondaryElements.semimajorAxis;
                        renderOrbit(elements);
                    }
                }
            }
        }
        ImGui::EndTable();
    }
}

void
renderDso(const DeepSkyObject* dso, const Universe* universe)
{
    ImGui::TextUnformatted(universe->getDSOCatalog()->getDSOName(dso).c_str());
#if SDL_VERSION_ATLEAST(2, 0, 14)
    const std::string& url = dso->getInfoURL();
    if (!url.empty())
    {
        ImGui::Separator();
        ImGui::TextUnformatted(_("Web info:"));
        if (ImGui::SmallButton(url.c_str())) { SDL_OpenURL(url.c_str()); }
    }
    ImGui::Separator();
#endif
    if (ImGui::BeginTable("##dsoInfo", 2))
    {
        ImGui::TableNextRow();
        Eigen::Vector3d eclPos = dso->getPosition();
        Eigen::Vector3d eqPos = astro::eclipticToEquatorial(Eigen::Vector3d(eclPos.x(), -eclPos.z(), eclPos.y()));
        double distance = eqPos.norm();
        double ra = celmath::radToDeg(std::atan2(eqPos.y(), eqPos.x()));
        if (ra < 0.0) { ra += 360.0; }
        double dec = celmath::radToDeg(std::asin(eqPos.z() / distance));
        int hoursDeg;
        int minutes;
        double seconds;
        astro::decimalToHourMinSec(ra, hoursDeg, minutes, seconds);
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(_("Right ascension"));
        ImGui::TableNextColumn();
        ImGui::Text("%dh %dm %fs", hoursDeg, minutes, seconds);
        ImGui::TableNextColumn();
        astro::decimalToDegMinSec(dec, hoursDeg, minutes, seconds);
        ImGui::TextUnformatted(_("Declination"));
        ImGui::TableNextColumn();
        ImGui::Text("%d" UTF8_DEGREE_SIGN " %d" UTF8_PRIME " %f" UTF8_DOUBLE_PRIME, hoursDeg, minutes, seconds);
        Eigen::Vector3d galPos = astro::equatorialToGalactic(eqPos);
        double lon = celmath::radToDeg(std::atan2(galPos.y(), galPos.x()));
        if (lon < 0.0) { lon += 360.0; }
        double lat = celmath::radToDeg(std::asin(eqPos.z() / eqPos.norm()));
        astro::decimalToDegMinSec(lon, hoursDeg, minutes, seconds);
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(_("Galactic longitude"));
        ImGui::TableNextColumn();
        ImGui::Text("%d" UTF8_DEGREE_SIGN " %d" UTF8_PRIME " %f" UTF8_DOUBLE_PRIME, hoursDeg, minutes, seconds);
        astro::decimalToDegMinSec(lat, hoursDeg, minutes, seconds);
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(_("Galactic latitude"));
        ImGui::TableNextColumn();
        ImGui::Text("%d" UTF8_DEGREE_SIGN " %d" UTF8_PRIME " %f" UTF8_DOUBLE_PRIME, hoursDeg, minutes, seconds);
        const char* unit;
        double radius = dso->getRadius();
        if (radius > astro::parsecsToLightYears(1.0e6))
        {
            radius = astro::lightYearsToParsecs(radius) / 1.0e6;
            unit = _("Mpc");
        }
        else if (radius > astro::parsecsToLightYears(0.5e3))
        {
            radius = astro::lightYearsToParsecs(radius) / 1.0e3;
            unit = _("kpc");
        }
        else
        {
            unit = _("ly");
        }
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(_("Radius"));
        ImGui::TableNextColumn();
        ImGui::Text("%f %s", radius, unit);
        if (distance > astro::parsecsToLightYears(1.0e6))
        {
            distance = astro::lightYearsToParsecs(distance) / 1.0e6;
            unit = _("Mpc");
        }
        else if (distance > astro::parsecsToLightYears(0.5e3))
        {
            distance = astro::lightYearsToParsecs(distance) / 1.0e3;
            unit = _("kpc");
        }
        else
        {
            unit = _("ly");
        }
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(_("Distance from Sol"));
        ImGui::TableNextColumn();
        ImGui::Text("%f %s", distance, unit);
        ImGui::EndTable();
    }
}
}

SDL_InfoViewer::SDL_InfoViewer(CelestiaCore* appCore) :
    m_appCore(appCore)
{
}

void
SDL_InfoViewer::show(const Selection& selection)
{
    m_selection = selection;
    m_visible = true;
}

void
SDL_InfoViewer::render()
{
    if (!m_visible || m_selection.empty()) { return; }
    double tdb = m_appCore->getSimulation()->getTime();
    if (ImGui::Begin(_("Object information"), &m_visible))
    {
        if (m_selection.body() != nullptr)
        {
            renderBody(m_selection.body(), tdb);
        }
        else if (m_selection.star() != nullptr)
        {
            renderStar(m_selection.star(), m_appCore->getSimulation()->getUniverse(), tdb);
        }
        else if (m_selection.deepsky() != nullptr)
        {
            renderDso(m_selection.deepsky(), m_appCore->getSimulation()->getUniverse());
        }
        ImGui::End();
    }
}
}
