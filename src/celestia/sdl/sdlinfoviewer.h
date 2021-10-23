#pragma once

#include <celengine/selection.h>

class CelestiaCore;

namespace celestia
{
class SDL_InfoViewer
{
public:
    SDL_InfoViewer(CelestiaCore* appCore);
    void show(const Selection& selection);
    void render();

private:
    CelestiaCore* m_appCore;
    Selection m_selection{};
    bool m_visible{ false };
};
}
