#include <cstdint>

#include <imgui.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl.h>

#include <celengine/render.h>

#include "sdlui.h"

namespace celestia
{

SDL_UI::SDL_UI(SDL_Window* window,
               SDL_GLContext* glContext,
               CelestiaCore* appCore) :
    m_window(window),
    m_glContext(glContext),
    m_appCore(appCore)
{
}

SDL_UI::~SDL_UI()
{
    if (m_initialized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }
}

bool
SDL_UI::init()
{
    if (!IMGUI_CHECKVERSION()) { return false; }
    if (!ImGui::CreateContext()) { return false; }
    ImGui::StyleColorsDark();
    if (!ImGui_ImplSDL2_InitForOpenGL(m_window, m_glContext)) { return false; }
    if (!ImGui_ImplOpenGL3_Init("#version 100")) { return false; }
    m_initialized = true;
    return true;
}

void
SDL_UI::processEvent(const SDL_Event* event)
{
    ImGui_ImplSDL2_ProcessEvent(event);
}

bool
SDL_UI::update()
{
    bool keepRunning = true;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    keepRunning &= mainMenu();
    celestialBrowser();

    ImGui::Render();

    return keepRunning;
}

void
SDL_UI::display() const
{
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool
SDL_UI::wantCaptureMouse() const
{
    return ImGui::GetIO().WantCaptureMouse;
}

bool
SDL_UI::wantCaptureKeyboard() const
{
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool
SDL_UI::mainMenu()
{
    bool keepRunning = true;
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Grab image", "F10")) { }
            if (ImGui::MenuItem("Capture video", "SHIFT+F10", false, false)) { }
            if (ImGui::MenuItem("Copy image", "CTRL+SHIFT+C")) { }
            ImGui::Separator();
            if (ImGui::MenuItem("Open script..")) { }
            if (ImGui::BeginMenu("Scripts"))
            {
                if (ImGui::MenuItem("Toggle galactic grid")) { }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Preferences..")) { }
            if (ImGui::MenuItem("Exit", "CTRL+Q")) { keepRunning = false; }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Navigation"))
        {
            if (ImGui::MenuItem("Select Sun")) { m_appCore->charEntered('h'); }
            if (ImGui::MenuItem("Center selection")) { m_appCore->charEntered('c'); }
            if (ImGui::MenuItem("Goto selection")) { m_appCore->charEntered('g'); }
            if (ImGui::MenuItem("Goto object..")) { }
            if (ImGui::MenuItem("Copy URL/console text", "CTRL+C")) { }
            if (ImGui::MenuItem("Paste URL/console text", "CTRL+V")) { }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Time"))
        {
            if (ImGui::MenuItem("Set time..")) { }
            bool lightDelayActive = m_appCore->getLightDelayActive();
            if (ImGui::MenuItem("Light time delay", nullptr, &lightDelayActive))
            {
                m_appCore->setLightDelayActive(lightDelayActive);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Display"))
        {
            Renderer* renderer = m_appCore->getRenderer();
            auto renderFlags = renderer->getRenderFlags();
            auto oldRenderFlags = renderFlags;
            if (ImGui::MenuItem("Atmospheres", "CTRL+A", renderFlags & Renderer::ShowAtmospheres))
            {
                renderFlags ^= Renderer::ShowAtmospheres;
            }
            if (ImGui::MenuItem("Clouds", nullptr, renderFlags & Renderer::ShowCloudMaps))
            {
                renderFlags ^= Renderer::ShowCloudMaps;
            }
            if (ImGui::MenuItem("Comet tails", nullptr, renderFlags & Renderer::ShowCometTails))
            {
                renderFlags ^= Renderer::ShowCometTails;
            }
            if (ImGui::MenuItem("Nightside lights", nullptr, renderFlags & Renderer::ShowNightMaps))
            {
                renderFlags ^= Renderer::ShowNightMaps;
            }
            if (ImGui::BeginMenu("Deep sky objects"))
            {
                if (ImGui::MenuItem("Galaxies", nullptr, renderFlags & Renderer::ShowGalaxies))
                {
                    renderFlags ^= Renderer::ShowGalaxies;
                }
                if (ImGui::MenuItem("Globulars", nullptr, renderFlags & Renderer::ShowGlobulars))
                {
                    renderFlags ^= Renderer::ShowGlobulars;
                }
                if (ImGui::MenuItem("Open clusters", nullptr, renderFlags & Renderer::ShowOpenClusters))
                {
                    renderFlags ^= Renderer::ShowOpenClusters;
                }
                if (ImGui::MenuItem("Nebulae", "^", renderFlags & Renderer::ShowNebulae))
                {
                    renderFlags ^= Renderer::ShowNebulae;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Shadows"))
            {
                if (ImGui::MenuItem("Ring shadows", nullptr, renderFlags & Renderer::ShowRingShadows))
                {
                    renderFlags ^= Renderer::ShowRingShadows;
                }
                if (ImGui::MenuItem("Eclipse shadows", "CTRL+E", renderFlags & Renderer::ShowEclipseShadows))
                {
                    renderFlags ^= Renderer::ShowEclipseShadows;
                }
                if (ImGui::MenuItem("Cloud shadows", nullptr, renderFlags & Renderer::ShowCloudShadows))
                {
                    renderFlags ^= Renderer::ShowCloudShadows;
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Auto magnitude", "CTRL+Y", renderFlags & Renderer::ShowAutoMag))
            {
                renderFlags ^= Renderer::ShowAutoMag;
            }
            if (ImGui::BeginMenu("Star style"))
            {
                auto starStyle = renderer->getStarStyle();
                if (ImGui::MenuItem("Points", nullptr, starStyle == Renderer::PointStars))
                {
                    renderer->setStarStyle(Renderer::PointStars);
                }
                if (ImGui::MenuItem("Fuzzy points", nullptr, starStyle == Renderer::FuzzyPointStars))
                {
                    renderer->setStarStyle(Renderer::FuzzyPointStars);
                }
                if (ImGui::MenuItem("Scaled discs", nullptr, starStyle == Renderer::ScaledDiscStars))
                {
                    renderer->setStarStyle(Renderer::ScaledDiscStars);
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("Texture resolution"))
            {
                auto textureResolution = renderer->getResolution();
                if (ImGui::MenuItem("Low", nullptr, textureResolution == 0))
                {
                    renderer->setResolution(0);
                }
                if (ImGui::MenuItem("Medium", nullptr, textureResolution == 1))
                {
                    renderer->setResolution(1);
                }
                if (ImGui::MenuItem("High", nullptr, textureResolution == 2))
                {
                    renderer->setResolution(2);
                }
                ImGui::EndMenu();
            }

            if (renderFlags != oldRenderFlags)
            {
                renderer->setRenderFlags(renderFlags);
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Bookmarks"))
        {
            if (ImGui::MenuItem("Add bookmark..")) { }
            if (ImGui::MenuItem("Organize bookmarks..")) { }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Celestial browser", nullptr, &m_showCelestialBrowser);
            ImGui::Separator();
            if (ImGui::MenuItem("Full screen", "SHIFT+F11")) { }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Celestia manual")) { }
            ImGui::Separator();
            if (ImGui::MenuItem("OpenGL info")) { }
            if (ImGui::MenuItem("About Celestia")) { }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    return keepRunning;
}

void
SDL_UI::celestialBrowser()
{
    if (!m_showCelestialBrowser) { return; }
    ImGui::Begin("Celestial browser", &m_showCelestialBrowser);

    if (ImGui::BeginTabBar("##tabs"))
    {
        if (ImGui::BeginTabItem("Solar system"))
        {
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Stars"))
        {
            if (ImGui::BeginTable("##stars", 5, ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Distance (ly)");
                ImGui::TableSetupColumn("App. mag");
                ImGui::TableSetupColumn("Abs. mag");
                ImGui::TableSetupColumn("Type");
                ImGui::TableHeadersRow();

                ImGui::EndTable();
            }

            ImGui::Text("1000 objects found");

            ImGui::Separator();

            ImGui::RadioButton("Closest stars", true);
            ImGui::SameLine();
            ImGui::RadioButton("Brightest stars", false);

            ImGui::Separator();

            ImGui::Text("Filter");

            bool xyz = false;
            ImGui::Checkbox("With planets", &xyz);
            ImGui::SameLine();
            ImGui::Checkbox("Multiple stars", &xyz);
            ImGui::SameLine();
            ImGui::Checkbox("Barycenters", &xyz);

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Deep sky objects"))
        {
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Button("Refresh");
    ImGui::Separator();
    ImGui::Text("Markers");
    ImGui::Button("Mark selected");
    ImGui::SameLine();
    ImGui::Button("Unmark selected");
    ImGui::SameLine();
    ImGui::Button("Clear markers");

    ImGui::End();
}

}
