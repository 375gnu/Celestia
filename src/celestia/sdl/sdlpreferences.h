#pragma once

#include <imgui.h>

class CelestiaCore;

namespace celestia
{
enum class ImGuiColorScheme
{
    Dark,
    Light,
    Classic,
};

class SDL_Preferences
{
public:
    SDL_Preferences(CelestiaCore* appCore);
    void show();
    void render();

private:
    CelestiaCore* m_appCore;
    bool m_visible{ false };
    ImGuiColorScheme m_colorScheme{ ImGuiColorScheme::Dark };

    void objectsTab(std::uint64_t& renderFlags);
    void guidesTab(std::uint64_t& renderFlags, int& labelMode);
    void labelsTab(int& labelMode);
    void renderTab(std::uint64_t& renderFlags);
    void displayTab();
};
}
