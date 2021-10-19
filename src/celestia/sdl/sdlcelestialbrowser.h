#pragma once

class CelestiaCore;

namespace celestia
{
class SDL_CelestialBrowser
{
public:
    bool update();
private:
    CelestiaCore* m_appCore;
};
}
