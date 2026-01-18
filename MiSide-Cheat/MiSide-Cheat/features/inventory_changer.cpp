#include "inventory_changer.h"
#include "../sdk/sdk.h"
#include <imgui.h>
#include <algorithm>

namespace features {
namespace inventory {

    std::vector<std::string> known_items = {
        "Card",
        "Key",
        "Phone",
        "Flashlight",
        "Cassette",
        "Coin",
        "Money",
        "Doll",
        "Knife",
        "Gun",
        "Ticket",
        "Map",
        "Fuse"
    };
    
    bool show_window = false;
    static bool safe_mode = true;

    static char custom_item_buffer[128] = "";

    void RenderWindow() {
        if (!show_window) return;
        
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Inventory Changer", &show_window)) {

            ImGui::Checkbox("Safe Mode (Bypass UI)", &safe_mode);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Disables Inventory UI during addition to prevent crashes with invalid items.");
            }
            ImGui::Separator();

            // Custom Item Input
            ImGui::InputText("Item ID", custom_item_buffer, sizeof(custom_item_buffer));
            ImGui::SameLine();
            if (ImGui::Button("Add Custom")) {
                sdk::game::AddItemToInventory(custom_item_buffer, safe_mode);
            }

            ImGui::Separator();
            
            if (ImGui::Button("Scan For New Items in Scene")) {
                 std::vector<std::string> sceneItems = sdk::game::GetAllItemPickups();
                 for (const auto& sceneItem : sceneItems) {
                     bool known = false;
                     for (const auto& k : known_items) {
                         if (k == sceneItem) {
                             known = true;
                             break;
                         }
                     }
                     if (!known) {
                         known_items.push_back(sceneItem);
                     }
                 }
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Scans all 'ItemPickup' objects in the current scene and adds their IDs to the list below.");
            }

            ImGui::Separator();

            // Split view: Known Items vs Current Inventory
            ImGui::Columns(2, "inventory_columns");
            
            // Left Column: Known Items
            ImGui::Text("Known Items");
            ImGui::Separator();
            
            ImGui::BeginChild("known_items_list", ImVec2(0, 300), true);
            for (const auto& item : known_items) {
                if (ImGui::Button(("Add##" + item).c_str())) {
                    sdk::game::AddItemToInventory(item.c_str(), safe_mode);
                }
                ImGui::SameLine();
                ImGui::Text("%s", item.c_str());
            }
            ImGui::EndChild();

            ImGui::NextColumn();

            // Right Column: Current Inventory
            ImGui::Text("Your Inventory");
            ImGui::SameLine();
            if (ImGui::Button("Refresh")) {
                // Refresh happens automatically in Update loop for caching, 
                // but we can force it or just rely on the next frame
            }
            ImGui::Separator();

            std::vector<std::string> current_items = sdk::game::GetInventoryItems();
            
            ImGui::BeginChild("current_inventory_list", ImVec2(0, 300), true);
            for (const auto& item : current_items) {
                if (ImGui::Button(("Remove##" + item).c_str())) {
                    sdk::game::RemoveItemFromInventory(item.c_str());
                }
                ImGui::SameLine();
                ImGui::Text("%s", item.c_str());
            }
            ImGui::EndChild();

            ImGui::Columns(1);
        }
        ImGui::End();
    }

    void Update() {
        // Optional: Periodic logic if needed
    }

}
}
