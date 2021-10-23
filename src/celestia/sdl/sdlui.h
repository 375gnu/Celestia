#pragma once

#include <memory>

#include <SDL.h>
#include <SDL_opengl.h>

#include <celengine/glsupport.h>
#include <celengine/selection.h>
#include <celestia/celestiacore.h>

namespace celestia
{
class SDL_Preferences;

enum class UIAction
{
    None,
    Quit,
    Copy,
    Paste,
    ToggleFullscreen,
};

class SDL_UI : public CelestiaCore::ContextMenuHandler
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

    void requestContextMenu(float x, float y, Selection sel) override;

    bool wantCaptureMouse() const;
    bool wantCaptureKeyboard() const;

private:
    SDL_Window* m_window;
    SDL_GLContext* m_glContext;
    CelestiaCore *m_appCore;
    bool *m_cursorVisible;
    bool *m_fullscreen;
    std::unique_ptr<SDL_Preferences> m_preferences;

    bool m_initialized{ false };
    bool m_openContext{ false };
    bool m_showGLInfo{ false };
    bool m_showAbout{ false };

    Selection m_selection { };

    void renderMainMenu(UIAction& action);
    void renderGLInfo();
    void renderAbout();
};
}
