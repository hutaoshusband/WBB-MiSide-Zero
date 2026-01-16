#include "debug_view.h"
#include "game_cache.h"
#include "debug_draw.h"
#include "../config/config.h"
#include "../sdk/sdk.h"
#include <imgui.h>

namespace features {
namespace debug_view {

    void Render() {
        if (!config::g_config.misc.debug_view) return;

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
        ImGui::Begin("Debug View (MiSide-Zero)", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
        {
            ImGui::TextColored(ImVec4(0.58f, 0.72f, 0.02f, 1.0f), "WallbangBros Game Analysis");
            ImGui::Separator();

            // Get cached debug info (thread-safe, no IL2CPP calls)
            game_cache::CachedDebugInfo debug = game_cache::GetDebugInfo();
            debug_draw::DebugDrawStats drawStats = debug_draw::GetStats();

            if (!sdk::IsReady()) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "SDK Not Ready");
                ImGui::TextDisabled("Waiting for main thread to initialize SDK...");
            } else {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "SDK Initialized");
                ImGui::TextWrapped("SDK Log: %s", sdk::GetLastLog());
                ImGui::Separator();

                // Local Player Info
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "[Local Player]");
                ImGui::Text("PlayerManager: %p", debug.playerManager);
                if (debug.playerManager) {
                    ImGui::BulletText("Pos: %.2f, %.2f, %.2f", 
                        debug.playerPos.x, debug.playerPos.y, debug.playerPos.z);
                }

                ImGui::Spacing();

                // Camera Info
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "[Camera System]");
                ImGui::Text("Camera.main: %p", debug.mainCamera);
                ImGui::Text("PlayerManager.playerCam: %p", debug.playerCamera);
                if (debug.mainCamera || debug.playerCamera) {
                    ImGui::BulletText("Camera Pos: %.2f, %.2f, %.2f",
                        debug.cameraPos.x, debug.cameraPos.y, debug.cameraPos.z);
                }

                ImGui::Spacing();

                // Mita Info
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "[Mita]");
                ImGui::Text("MitaManager: %p", debug.mitaManager);
                ImGui::Text("MitaAnimator: %p", debug.mitaAnimator);
                ImGui::Text("NavMeshAgent: %p", debug.navMeshAgent);
                if (debug.mitaManager) {
                    ImGui::BulletText("Pos: %.2f, %.2f, %.2f",
                        debug.mitaPos.x, debug.mitaPos.y, debug.mitaPos.z);
                    ImGui::Text("Screen: %.1f, %.1f (Z: %.1f)",
                        debug.mitaScreenPos.x, debug.mitaScreenPos.y, debug.mitaScreenPos.z);
                    if (debug.navMeshAgent) {
                        ImGui::Text("Agent Speed: %.2f", debug.agentSpeed);
                    }
                }

                // World To Screen Test
                if (debug.playerManager && (debug.mainCamera || debug.playerCamera)) {
                    ImGui::Text("Player W2S: %.1f, %.1f", debug.playerW2S.x, debug.playerW2S.y);
                }

                ImGui::Spacing();

                // Debug Draw Hooks
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "[Debug Draw Hooks]");
                ImGui::Text("Hooks Active: %s", drawStats.hook_active ? "YES" : "NO");
                ImGui::Text("Lines Captured: %d", drawStats.lines_captured);
                ImGui::Text("Rays Captured: %d", drawStats.rays_captured);
                ImGui::Text("Gizmos Called: %d", drawStats.gizmos_called);

                ImGui::Spacing();

                // Settings
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "[Settings]");
                ImGui::Text("Render Debug: %s", config::g_config.misc.debug_draw_render ? "YES" : "NO");
                ImGui::Text("Max Lines: %d", config::g_config.misc.debug_draw_max_lines);
                ImGui::Text("Max Rays: %d", config::g_config.misc.debug_draw_max_rays);

                ImGui::Spacing();

                // DOTween Stats
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "[DOTween]");
                ImGui::Text("Active Tweens: %d", debug.activeTweens);
                ImGui::Text("Path Tweens: %d", debug.pathTweens);
            }

            ImGui::Separator();
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

            if (ImGui::Button("Close Debug View")) {
                config::g_config.misc.debug_view = false;
            }
        }
        ImGui::End();
    }

} // namespace debug_view
} // namespace features
