#ifndef CELESTIA_CELESTIA_SDL_SDLUI_H
#define CELESTIA_CELESTIA_SDL_SDLUI_H

#include <celengine/glsupport.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include <celestia/celestiacore.h>

namespace celestia
{
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
    bool m_showCelestialBrowser{ false };

    bool mainMenu();
    void celestialBrowser();
};
}

#endif
