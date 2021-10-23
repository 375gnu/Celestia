// qtinfopanel.cpp
//
// Copyright (C) 2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Information panel for Qt4 UI.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celcompat/numbers.h>
#include <celestia/celestiacore.h>
#include <celengine/astro.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/greek.h>
#include <celengine/universe.h>
#include <QTextBrowser>
#include <QIODevice>
#include <QItemSelection>
#include "qtinfopanel.h"

using namespace std;
using namespace Eigen;
using namespace celmath;
using celestia::util::GetLogger;

InfoPanel::InfoPanel(CelestiaCore* _appCore, const QString& title, QWidget* parent) :
    QDockWidget(title, parent),
    appCore(_appCore)
{
    textBrowser = new QTextBrowser(this);
    textBrowser->setOpenExternalLinks(true);
    setWidget(textBrowser);
}


void InfoPanel::buildInfoPage(Selection sel,
                              Universe* universe,
                              double tdb)
{
    QString pageText;
    QTextStream stream(&pageText, QIODevice::WriteOnly);

    pageHeader(stream);

    if (sel.body() != nullptr)
    {
        buildSolarSystemBodyPage(sel.body(), tdb, stream);
    }
    else if (sel.star() != nullptr)
    {
        buildStarPage(sel.star(), universe, tdb, stream);
    }
    else if (sel.deepsky() != nullptr)
    {
        buildDSOPage(sel.deepsky(), universe, stream);
    }
    else
    {
        stream << QString(_("Error: no object selected!\n"));
    }

    pageFooter(stream);

    textBrowser->setHtml(pageText);
}


void InfoPanel::pageHeader(QTextStream& stream)
{
    stream << "<html><head><title>" << QString(_("Info")) << "</title></head><body>";
}


void InfoPanel::pageFooter(QTextStream& stream)
{
    stream << "</body></html>";
}


static QString anchor(const QString& href, const QString& text)
{
    return QString("<a href=\"%1\">%2</a>").arg(href, text);
}


void InfoPanel::buildSolarSystemBodyPage(const Body* body,
                                         double t,
                                         QTextStream& stream)
{
    stream << QString("<h1>%1</h1>").arg(QString::fromStdString(body->getName(true)));

    if (!body->getInfoURL().empty())
    {
        QString infoURL = QString::fromStdString(body->getInfoURL());
        stream << QString(_("Web info: %1")).arg(anchor(infoURL, infoURL)) << "<br>\n";
    }

    stream << "<br>";

    bool isArtificial = body->getClassification() == Body::Spacecraft;

    QString units(_("km"));
    double radius = body->getRadius();
    if (radius < 1.0)
    {
        units = _("m");
        radius *= 1000.0;
    }

    if (body->isEllipsoid())
        stream << QString(_("<b>Equatorial radius:</b> %L1 %2")).arg(radius).arg(units) << "<br>\n";
    else
        stream << QString(_("<b>Size:</b> %L1 %2")).arg(radius).arg(units) << "<br>\n";

#if 0
    if (body->getOblateness() != 0.0f && body->isEllipsoid())
    {
        stream << QString(_("<b>Oblateness: ")) << body->getOblateness() << "<br>\n";
    }
#endif

    double orbitalPeriod = 0.0;
    const Orbit* orbit = body->getOrbit(t);
    if (orbit->isPeriodic())
        orbitalPeriod = orbit->getPeriod();

    // Show rotation information for natural, periodic rotators
    if (body->getRotationModel(t)->isPeriodic() && !isArtificial)
    {
        const RotationModel* rotationModel = body->getRotationModel(t);
        double rotPeriod = rotationModel->getPeriod();

        double dayLength = 0.0;
        bool prograde = false;
        if (orbitalPeriod > 0.0)
        {
            Vector3d axis = AngleAxisd(rotationModel->equatorOrientationAtTime(t)
                                       * body->getBodyFrame(t)->getOrientation(t)).axis();
            Vector3d orbitNormal = body->getOrbitFrame(t)->getOrientation(t)
                                   * orbit->positionAtTime(t).cross(orbit->velocityAtTime(t));
            prograde = axis.dot(orbitNormal) >= 0;
            double siderealDaysPerYear = orbitalPeriod / rotPeriod;
            double solarDaysPerYear = prograde ? siderealDaysPerYear - 1.0 : siderealDaysPerYear + 1.0;
            if (std::abs(solarDaysPerYear) > 0.0001)
            {
                dayLength = std::abs(orbitalPeriod / solarDaysPerYear);
            }
        }

        if (rotPeriod < 2.0)
        {
            rotPeriod *= 24.0;
            dayLength *= 24.0;
            units = _("hours");
        }
        else
        {
            units = _("days");
        }

        stream << QString(_("<b>Sidereal rotation period:</b> %L1 %2")).arg(rotPeriod).arg(units) << "<br>\n";
        if (orbitalPeriod > 0.0)
        {
            QString direction = prograde ? _("Prograde") : _("Retrograde");
            stream << QString(_("<b>Rotation direction:</b> %1")).arg(direction) << "<br>\n";
        }
        if (dayLength != 0.0)
        {
            stream << QString(_("<b>Length of day:</b> %L1 %2")).arg(dayLength).arg(units) << "<br>\n";
        }
    }

    // Orbit information
    if (orbitalPeriod != 0.0 || 1)
    {
        if (orbitalPeriod == 0.0)
            orbitalPeriod = 365.25;

        astro::OrbitalElements elements;
        orbit->calculateOsculatingElements(t,
                                           orbitalPeriod * 1.0e-6,
                                           &elements);
        if (orbitalPeriod < 2.0)
        {
            orbitalPeriod *= 24.0;
            units = _("hours");
        }
        else if (orbitalPeriod < 365.25 * 2.0)
        {
            units = _("days");
        }
        else
        {
            units = _("years");
            orbitalPeriod /= 365.25;
        }

        if (body->getRings() != nullptr)
            stream << QString(_("<b>Has rings</b>")) << "<br>\n";
        if (body->getAtmosphere() != nullptr)
            stream << QString(_("<b>Has atmosphere</b>")) << "<br>\n";

        // Start and end dates
        double startTime = 0.0;
        double endTime = 0.0;
        body->getLifespan(startTime, endTime);

        if (startTime > -1.0e9)
            stream << "<br>" << QString(_("<b>Start:</b> %1")).arg(astro::TDBtoUTC(startTime).toCStr()) << "<br>\n";

        if (endTime < 1.0e9)
            stream << "<br>" << QString(_("<b>End:</b> %1")).arg(astro::TDBtoUTC(endTime).toCStr()) << "<br>\n";

        stream << "<br><big><b>" << QString(_("Orbit information")) << "</b></big><br>\n";
        stream << QString(_("Osculating elements for %1")).arg(astro::TDBtoUTC(t).toCStr()) << "<br>\n";
        stream << "<br>\n";
//        stream << "<i>[ Orbit reference plane info goes here ]</i><br>\n";
        stream << QString(_("<b>Period:</b> %L1 %2")).arg(orbitalPeriod).arg(units) << "<br>\n";

        double sma = elements.semimajorAxis;
        if (sma > 2.5e7)
        {
            units = _("AU");
            sma = astro::kilometersToAU(sma);
        }
        else
        {
            units = _("km");
        }

        stream << QString(_("<b>Semi-major axis:</b> %L1 %2")).arg(sma).arg(units) << "<br>\n";
        stream << QString(_("<b>Eccentricity:</b> %L1")).arg(elements.eccentricity) << "<br>\n";
        stream << QString(_("<b>Inclination:</b> %L1%2")).arg(radToDeg(elements.inclination)).arg(QString::fromUtf8(UTF8_DEGREE_SIGN)) << "<br>\n";
        stream << QString(_("<b>Pericenter distance:</b> %L1 %2")).arg(sma * (1 - elements.eccentricity)).arg(units) << "<br>\n";
        stream << QString(_("<b>Apocenter distance:</b> %L1 %2")).arg(sma * (1 + elements.eccentricity)).arg(units) << "<br>\n";

        stream << QString(_("<b>Ascending node:</b> %L1%2")).arg(radToDeg(elements.longAscendingNode)).arg(QString::fromUtf8(UTF8_DEGREE_SIGN)) << "<br>\n";
        stream << QString(_("<b>Argument of periapsis:</b> %L1%2")).arg(radToDeg(elements.argPericenter)).arg(QString::fromUtf8(UTF8_DEGREE_SIGN)) << "<br>\n";
        stream << QString(_("<b>Mean anomaly:</b> %L1%2")).arg(radToDeg(elements.meanAnomaly)).arg(QString::fromUtf8(UTF8_DEGREE_SIGN)) << "<br>\n";
        stream << QString(_("<b>Period (calculated):</b> %L1 %2")).arg(elements.period).arg(_("days")) << "<br>\n";;
    }
}


Vector3d celToJ2000Ecliptic(const Vector3d& p)
{
    return Vector3d(p.x(), -p.z(), p.y());
}


Vector3d rectToSpherical(const Vector3d& v)
{
    double r = v.norm();
    double theta = atan2(v.y(), v.x());
    if (theta < 0)
        theta = theta + 2 * celestia::numbers::pi;
    double phi = asin(v.z() / r);

    return Vector3d(theta, phi, r);
}


void InfoPanel::buildStarPage(const Star* star, const Universe* universe, double tdb, QTextStream& stream)
{
    string name = ReplaceGreekLetterAbbr(universe->getStarCatalog()->getStarName(*star, true));
    stream << QString("<h1>%1</h1>").arg(QString::fromStdString(name));

    // Compute the star's position relative to the Solar System Barycenter. Note that
    // this will ignore the effect of parallax in the star's position.
    // TODO: Use either the observer's position or the Earth's position as the
    // origin instead.
    Vector3d celPos = star->getPosition(tdb).offsetFromKm(UniversalCoord::Zero());
    Vector3d eqPos = astro::eclipticToEquatorial(celToJ2000Ecliptic(celPos));
    Vector3d sph = rectToSpherical(eqPos);

    int hours = 0;
    int minutes = 0;
    double seconds = 0;
    astro::decimalToHourMinSec(radToDeg(sph.x()), hours, minutes, seconds);
    stream << QString(_("<b>RA:</b> %L1h %L2m %L3s")).arg(hours).arg(abs(minutes)).arg(abs(seconds)) << "<br>\n";

    int degrees = 0;
    astro::decimalToDegMinSec(radToDeg(sph.y()), degrees, minutes, seconds);
    stream << QString(_("<b>Dec:</b> %L1%2 %L3' %L4\"")).arg(degrees).arg(QString::fromUtf8(UTF8_DEGREE_SIGN))
                                                        .arg(abs(minutes)).arg(abs(seconds)) << "<br>\n";
}


void InfoPanel::buildDSOPage(const DeepSkyObject* dso,
                             const Universe* universe,
                             QTextStream& stream)
{
    string name = universe->getDSOCatalog()->getDSOName(dso, true);
    stream << QString("<h1>%1</h1>").arg(QString::fromStdString(name));

    Vector3d eqPos = astro::eclipticToEquatorial(celToJ2000Ecliptic(dso->getPosition()));
    Vector3d sph = rectToSpherical(eqPos);

    int hours = 0;
    int minutes = 0;
    double seconds = 0;
    astro::decimalToHourMinSec(radToDeg(sph.x()), hours, minutes, seconds);
    stream << QString(_("<b>RA:</b> %L1h %L2m %L3s")).arg(hours).arg(abs(minutes)).arg(abs(seconds)) << "<br>\n";

    int degrees = 0;
    astro::decimalToDegMinSec(radToDeg(sph.y()), degrees, minutes, seconds);
    stream << QString(_("<b>Dec:</b> %L1%2 %L3' %L4\"")).arg(degrees).arg(QString::fromUtf8(UTF8_DEGREE_SIGN))
                                                        .arg(abs(minutes)).arg(abs(seconds)) << "<br>\n";

    Vector3d galPos = astro::equatorialToGalactic(eqPos);
    sph = rectToSpherical(galPos);

    astro::decimalToDegMinSec(radToDeg(sph.x()), degrees, minutes, seconds);
    // TRANSLATORS: Galactic longitude
    stream << QString(_("<b>L:</b> %L1%2 %L3' %L4\"")).arg(degrees).arg(QString::fromUtf8(UTF8_DEGREE_SIGN))
                                                      .arg(abs(minutes)).arg(abs(seconds)) << "<br>\n";
    astro::decimalToDegMinSec(radToDeg(sph.y()), degrees, minutes, seconds);
    // TRANSLATORS: Galactic latitude
    stream << QString(_("<b>B:</b> %L1%2 %L3' %L4\"")).arg(degrees).arg(QString::fromUtf8(UTF8_DEGREE_SIGN))
                                                      .arg(abs(minutes)).arg(abs(seconds)) << "<br>\n";
}


void InfoPanel::updateHelper(ModelHelper* model, const QItemSelection& newSel, const QItemSelection& oldSel)
{
    if (!isVisible() || newSel == oldSel || newSel.indexes().length() == 0)
        return;

    Selection sel = model->itemForInfoPanel(newSel.indexes().at(0));
    if (!sel.empty())
    {
        buildInfoPage(sel,
                      appCore->getSimulation()->getUniverse(),
                      appCore->getSimulation()->getTime());
    }
}
