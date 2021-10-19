#include <imgui.h>

#include <celengine/glsupport.h>
#include <celestia/celestiacore.h>

#include "sdlcelestialbrowser.h"

namespace celestia
{
bool
SDL_CelestialBrowser::update()
{
    bool close = true;
    if (ImGui::Begin("Celestial browser", &close))
    {
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
    return close;
}

}
