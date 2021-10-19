#pragma once

#include <vector>

#include <SDL.h>
#include <SDL_opengl.h>

class CelestiaCore;
struct ScriptMenuItem;

namespace celestia
{
enum class ImGuiColorScheme
{
    Dark,
    Light,
    Classic,
};

class SDL_UI
{
public:
    SDL_UI(SDL_Window* window, SDL_GLContext* glContext, CelestiaCore* appCore);
    ~SDL_UI();

    bool init();
    void processEvent(const SDL_Event* event);
    bool update();
    void display() const;

    bool wantCaptureMouse() const;
    bool wantCaptureKeyboard() const;

private:
    SDL_Window* m_window;
    SDL_GLContext* m_glContext;
    CelestiaCore *m_appCore;

    bool m_initialized{ false };
    bool m_showPreferences{ false };
    bool m_showCelestialBrowser{ false };
    ImGuiColorScheme m_colorScheme{ ImGuiColorScheme::Dark };

    bool mainMenu();
    void preferences();

    std::vector<ScriptMenuItem>* m_scripts{ nullptr };
};
}
