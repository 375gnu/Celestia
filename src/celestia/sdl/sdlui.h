#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_opengl.h>

class CelestiaCore;
struct ScriptMenuItem;

namespace celestia
{
enum class UIAction
{
    None,
    Quit,
    Copy,
    Paste,
    ToggleFullscreen,
};

enum class ImGuiColorScheme
{
    Dark,
    Light,
    Classic,
};

class SDL_UI
{
public:
    SDL_UI(SDL_Window* window,
           SDL_GLContext* glContext,
           CelestiaCore* appCore,
           bool* cursorVisible,
           bool* isFullscreen);
    ~SDL_UI();

    bool init();
    void processEvent(const SDL_Event* event);
    UIAction update();
    void display() const;

    bool wantCaptureMouse() const;
    bool wantCaptureKeyboard() const;

private:
    SDL_Window* m_window;
    SDL_GLContext* m_glContext;
    CelestiaCore *m_appCore;
    bool *m_cursorVisible;
    bool *m_fullscreen;

    bool m_initialized{ false };
    bool m_showPreferences{ false };
    bool m_showGLInfo{ false };
    bool m_showAbout{ false };
    bool m_showCelestialBrowser{ false };
    ImGuiColorScheme m_colorScheme{ ImGuiColorScheme::Dark };

    void renderMainMenu(UIAction& action);
    void renderGLInfo();
    void renderAbout();
    void renderPreferences();

    std::map<std::string, std::string> m_glInfo{ };
    std::vector<std::string> m_glExtensions{ };
    std::unique_ptr<std::vector<ScriptMenuItem>> m_scripts{ nullptr };
};
}
