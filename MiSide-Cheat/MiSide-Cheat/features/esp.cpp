#include "esp.h"
#include "game_cache.h"
#include "../config/config.h"
#include <imgui.h>

namespace features {
namespace esp {

    // Helper: Expand bounding box with a screen point
    static void ExpandBounds(const sdk::Vector3& p, float& minX, float& maxX, float& minY, float& maxY) {
        if (p.z > 0) { // Only if visible (in front of camera)
            if (p.x < minX) minX = p.x;
            if (p.x > maxX) maxX = p.x;
            if (p.y < minY) minY = p.y;
            if (p.y > maxY) maxY = p.y;
        }
    }

    void RenderMitaESP() {
        if (!config::g_config.visuals.esp.IsActive()) return;

        // Get cached Mita data (thread-safe)
        game_cache::CachedEntity mita = game_cache::GetMita();
        
        if (!mita.valid) return;
        
        // Check if Mita is visible on screen
        if (mita.screenPos.z <= 0) return;
        
        // Determine box thickness based on state
        float thickness = 2.0f;
        if (mita.state == 1 || mita.moveState >= 2) {
            thickness = 4.0f; // Thicker when running
        }

        if (mita.hasBones) {
            // Full bone-based ESP
            if (mita.headScreen.z <= 0 && mita.leftFootScreen.z <= 0 && mita.rightFootScreen.z <= 0) {
                return; // No visible bones
            }

            // Calculate bounding box from bones
            float minX = mita.headScreen.x, maxX = mita.headScreen.x;
            float minY = mita.headScreen.y, maxY = mita.headScreen.y;

            ExpandBounds(mita.hipsScreen, minX, maxX, minY, maxY);
            ExpandBounds(mita.leftFootScreen, minX, maxX, minY, maxY);
            ExpandBounds(mita.rightFootScreen, minX, maxX, minY, maxY);
            ExpandBounds(mita.leftHandScreen, minX, maxX, minY, maxY);
            ExpandBounds(mita.rightHandScreen, minX, maxX, minY, maxY);
            ExpandBounds(mita.leftShoulderScreen, minX, maxX, minY, maxY);
            ExpandBounds(mita.rightShoulderScreen, minX, maxX, minY, maxY);

            // Add padding above head for hair/accessories
            float height = maxY - minY;
            minY -= height * 0.25f;

            // Add horizontal padding for arms/body width
            float width = maxX - minX;
            float paddingX = width * 0.15f;
            minX -= paddingX;
            maxX += paddingX;

            // Minimum size check
            if ((maxY - minY) < 10.0f) {
                maxY = minY + 10.0f;
            }
            if ((maxX - minX) < 10.0f) {
                float center = (minX + maxX) / 2.0f;
                minX = center - 5.0f;
                maxX = center + 5.0f;
            }

            // Draw Box
            if (config::g_config.visuals.esp_box) {
                ImColor boxColor(
                    config::g_config.visuals.esp_box_color[0],
                    config::g_config.visuals.esp_box_color[1],
                    config::g_config.visuals.esp_box_color[2],
                    config::g_config.visuals.esp_box_color[3]
                );
                ImGui::GetForegroundDrawList()->AddRect(
                    ImVec2(minX, minY),
                    ImVec2(maxX, maxY),
                    boxColor,
                    0.0f,
                    0,
                    thickness
                );
            }

            // Draw Name
            if (config::g_config.visuals.esp_name) {
                float textWidth = ImGui::CalcTextSize(mita.name.c_str()).x;
                ImGui::GetForegroundDrawList()->AddText(
                    ImVec2(minX + (maxX - minX) / 2.0f - textWidth / 2.0f, minY - 18),
                    ImColor(255, 255, 255, 255),
                    mita.name.c_str()
                );
            }
        } else {
            // Simple position-based ESP (fallback)
            float height = mita.screenPos.y - mita.headScreen.y;
            if (height < 0) height = -height;
            if (height < 5.0f) height = 50.0f; // Default height if calculation fails

            float width = height * 0.5f;
            float x = mita.headScreen.x - (width / 2);
            float y = mita.headScreen.y;

            if (config::g_config.visuals.esp_box) {
                ImColor boxColor(
                    config::g_config.visuals.esp_box_color[0],
                    config::g_config.visuals.esp_box_color[1],
                    config::g_config.visuals.esp_box_color[2],
                    config::g_config.visuals.esp_box_color[3]
                );
                ImGui::GetForegroundDrawList()->AddRect(
                    ImVec2(x, y),
                    ImVec2(x + width, y + height),
                    boxColor,
                    0.0f,
                    0,
                    thickness
                );
            }

            if (config::g_config.visuals.esp_name) {
                ImGui::GetForegroundDrawList()->AddText(
                    ImVec2(x + (width / 2) - 10, y - 15),
                    ImColor(255, 255, 255, 255),
                    mita.name.c_str()
                );
            }
        }
    }

    void RenderCollectiblesESP() {
        if (!config::g_config.visuals.esp_collectibles) return;

        // Get cached collectibles (thread-safe)
        std::vector<game_cache::CachedCollectible> collectibles = game_cache::GetCollectibles();

        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        for (const auto& col : collectibles) {
            if (!col.valid) continue;
            if (col.screenPos.z <= 0) continue; // Behind camera

            // Filter by type based on config
            bool shouldDraw = false;
            ImColor color(200, 200, 200);
            std::string label = "Item";

            if (col.type == "Card") {
                if (config::g_config.visuals.esp_col_cards) {
                    shouldDraw = true;
                    color = ImColor(0, 255, 255); // Cyan
                    label = "Card";
                }
            } else if (col.type == "Cassette") {
                if (config::g_config.visuals.esp_col_cassettes) {
                    shouldDraw = true;
                    color = ImColor(255, 165, 0); // Orange
                    label = "Cassette";
                }
            } else if (col.type == "Coin") {
                if (config::g_config.visuals.esp_col_coins) {
                    shouldDraw = true;
                    color = ImColor(255, 215, 0); // Gold
                    label = "Coin";
                }
            } else {
                // Generic items
                shouldDraw = true;
                label = col.name;
            }

            if (shouldDraw) {
                if (config::g_config.visuals.esp_box) {
                    ImColor boxColor(
                        config::g_config.visuals.esp_col_color[0],
                        config::g_config.visuals.esp_col_color[1],
                        config::g_config.visuals.esp_col_color[2],
                        config::g_config.visuals.esp_col_color[3]
                    );
                    drawList->AddCircle(
                        ImVec2(col.screenPos.x, col.screenPos.y),
                        5.0f,
                        boxColor
                    );
                }
                drawList->AddText(
                    ImVec2(col.screenPos.x + 8, col.screenPos.y - 8),
                    color,
                    label.c_str()
                );
            }
        }
    }

    void RenderAll() {
        RenderMitaESP();
        RenderCollectiblesESP();
    }

} // namespace esp
} // namespace features
