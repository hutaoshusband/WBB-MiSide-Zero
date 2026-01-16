#include "collectibles.h"
#include "../sdk/sdk.h"
#include "../config/config.h"
#include <imgui.h>
#include <algorithm>
#include <string>

namespace features {

    // Cache to avoid scanning every frame
    static std::vector<void*> storedCollectibles;
    static float updateTimer = 0.0f;

    void RenderCollectiblesESP() {
        if (!config::g_config.visuals.esp_collectibles) return;

        updateTimer += ImGui::GetIO().DeltaTime;
        if (updateTimer > 1.0f || storedCollectibles.empty()) { // Update every 1 sec
            updateTimer = 0.0f;
            storedCollectibles.clear();
            
            // Speed up by scanning only ItemPickup components instead of all Transforms
            std::vector<void*> objects = sdk::game::FindObjectsOfTypeAll("ItemPickup");
            
            for (void* obj : objects) {
                if (obj) storedCollectibles.push_back(obj);
            }
        }

        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        for (void* obj : storedCollectibles) {
            if (!sdk::IsValidPtr(obj)) continue;

            // GetPosition handles components correctly by calling get_transform automatically
            sdk::Vector3 pos = sdk::game::GetPosition(obj);
            sdk::Vector3 screenPos = sdk::game::WorldToScreen(pos);

            if (screenPos.z > 0) {
                const char* name = sdk::game::GetObjectName(obj);
                if (!name) continue;
                
                std::string nameStr = name;
                std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);

                bool shouldDraw = false;
                ImColor color = ImColor(200, 200, 200); // Default gray
                std::string label = "Item";

                // Filter based on configuration
                if (nameStr.find("card") != std::string::npos) {
                    if (config::g_config.visuals.esp_col_cards) {
                        shouldDraw = true;
                        color = ImColor(0, 255, 255); // Cyan
                        label = "Card";
                    }
                }
                else if (nameStr.find("cassette") != std::string::npos || nameStr.find("tape") != std::string::npos || nameStr.find("cartridge") != std::string::npos) {
                    if (config::g_config.visuals.esp_col_cassettes) {
                        shouldDraw = true;
                        color = ImColor(255, 165, 0); // Orange
                        label = "Cassette";
                    }
                }
                else if (nameStr.find("coin") != std::string::npos || nameStr.find("money") != std::string::npos) {
                     if (config::g_config.visuals.esp_col_coins) {
                        shouldDraw = true;
                        color = ImColor(255, 215, 0); // Gold
                        label = "Coin";
                    }
                }
                else {
                    // Fallback for other ItemPickups not matching categories
                    // If user only wants specific types, valid logic is to NOT draw unknown ones?
                    // Or maybe draw them as generic items?
                    // Let's draw them as generic items for now but maybe add a toggle later.
                    shouldDraw = true;
                    label = name; // Use full name
                }

                if (shouldDraw) {
                    if (config::g_config.visuals.esp_box) {
                         // Use user defined box color for collectibles
                         ImColor boxColor = ImColor(config::g_config.visuals.esp_col_color[0], 
                                                    config::g_config.visuals.esp_col_color[1], 
                                                    config::g_config.visuals.esp_col_color[2], 
                                                    config::g_config.visuals.esp_col_color[3]);
                         drawList->AddCircle({screenPos.x, screenPos.y}, 5.0f, boxColor);
                    }
                    drawList->AddText({screenPos.x + 8, screenPos.y - 8}, color, label.c_str());
                }
            }
        }
    }
}
