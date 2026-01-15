#include "features.h"
#include "../config/config.h"
#include <imgui.h>
#include <algorithm>
#include <cstring>
#include "../sdk/sdk.h"

namespace features {
    
    // ============================================================
    // Get all modules for display in the module list and menu
    // This is where we register all our features
    // ============================================================
    std::vector<ModuleInfo> GetAllModules() {
        std::vector<ModuleInfo> modules;
        
        // Visuals
        modules.push_back({ "ESP",           "Visuals",  &config::g_config.visuals.esp_enabled });
        modules.push_back({ "Chams",         "Visuals",  &config::g_config.visuals.chams_enabled });
        modules.push_back({ "Fullbright",    "Visuals",  &config::g_config.visuals.fullbright });
        modules.push_back({ "No Fog",        "Visuals",  &config::g_config.visuals.no_fog });
        modules.push_back({ "Crosshair",     "Visuals",  &config::g_config.visuals.crosshair });
        
        // Aimbot
        modules.push_back({ "Aimbot",        "Aimbot",   &config::g_config.aimbot.enabled });
        
        // Movement
        modules.push_back({ "Speed",         "Movement", &config::g_config.misc.speed_hack });
        modules.push_back({ "Fly",           "Movement", &config::g_config.misc.fly_hack });
        modules.push_back({ "NoClip",        "Movement", &config::g_config.misc.no_clip });
        
        // Player
        modules.push_back({ "God Mode",      "Player",   &config::g_config.misc.god_mode });
        modules.push_back({ "Inf. Stamina",  "Player",   &config::g_config.misc.infinite_stamina });
        modules.push_back({ "Inf. Ammo",     "Player",   &config::g_config.misc.infinite_ammo });
        
        // Misc
        modules.push_back({ "Teleport",      "Misc",     &config::g_config.misc.teleport_enabled });
        modules.push_back({ "Debug View",    "Misc",     &config::g_config.misc.debug_view });
        
        return modules;
    }
    
    std::vector<ModuleInfo> GetEnabledModules() {
        std::vector<ModuleInfo> enabled;
        auto all = GetAllModules();
        
        for (const auto& mod : all) {
            if (mod.IsEnabled()) {
                enabled.push_back(mod);
            }
        }
        
        // Sort by name length (longest first) for pyramid effect in module list
        std::sort(enabled.begin(), enabled.end(), [](const ModuleInfo& a, const ModuleInfo& b) {
            return strlen(a.name) > strlen(b.name);
        });
        
        return enabled;
    }
    
    void Initialize() {
        // Initialize all feature systems here
        // For now, nothing special needed
        sdk::Initialize();
    }
    
    void Shutdown() {
        // Cleanup all features
    }
    
    void OnTick() {
        // Called every frame - update all enabled features
        // TODO: Implement actual feature logic here
    }
    
    void OnRender() {
        // Called during render - for overlay drawing
        
        if (config::g_config.misc.debug_view) {
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);
            ImGui::Begin("Debug View (MiSide-Zero)", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
            {
                ImGui::TextColored(ImVec4(0.58f, 0.72f, 0.02f, 1.0f), "WallbangBros Game Analysis");
                ImGui::Separator();
                
                // Initialize SDK if not already
                static bool sdkInit = false;
                if (!sdkInit) {
                    sdkInit = sdk::Initialize();
                }
                
                if (!sdk::Initialize()) {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "SDK Initialize FAILED");
                } else {
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "WallbangBros Game Analysis");
                    
                    ImGui::TextWrapped("SDK Log: %s", sdk::GetLastLog());
                    ImGui::Separator();

                    // 1. Local Player Check
                    void* pm = sdk::game::GetPlayerManager();
                    void* mitaMgr = sdk::game::GetMitaManager();
                    ImGui::TextColored(ImVec4(1, 1, 0, 1), "[Local Player]");
                    ImGui::Text("Class: PlayerManager (TypeDef: 4211)");
                    ImGui::Text("Instance: %p", pm);
                    ImGui::Text("MitaManager: %p", mitaMgr);
                    
                    if (pm) {
                        sdk::Vector3 pos = sdk::game::GetPosition(pm); // Assuming PM is a component
                        ImGui::BulletText("Position: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
                    } else {
                        ImGui::BulletText("Position: N/A");
                    }
                    
                    ImGui::Spacing();
                    
                    // 2. Camera Check
                    void* cam = sdk::game::GetPlayerCamera();
                    ImGui::TextColored(ImVec4(1, 1, 0, 1), "[Camera]");
                    ImGui::Text("Main Camera: %p", cam);
                    
                    if (cam) {
                        sdk::Vector3 camPos = sdk::game::GetPosition(cam);
                        ImGui::BulletText("Cam Pos: %.2f, %.2f, %.2f", camPos.x, camPos.y, camPos.z);
                    }
                    
                    ImGui::Spacing();

                    // World To Screen Test
                    if (pm && cam) {
                         sdk::Vector3 worldPos = sdk::game::GetPosition(pm);
                         sdk::Vector3 screenPos = sdk::game::WorldToScreen(worldPos);
                         
                         // Fix Y for Debug View too
                         screenPos.y = ImGui::GetIO().DisplaySize.y - screenPos.y;
                         
                         ImGui::Text("W2S Test: Screen(%.1f, %.1f)", screenPos.x, screenPos.y);
                         
                         // Draw a circle at player root
                         if (screenPos.z > 0) {
                              ImGui::GetForegroundDrawList()->AddCircle(ImVec2(screenPos.x, screenPos.y), 5.0f, ImColor(255, 0, 0));
                         }
                    }
                }
                
                ImGui::Separator();
                ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
                
                if (ImGui::Button("Close Debug View")) {
                    config::g_config.misc.debug_view = false;
                }
            }
            ImGui::End();
        }


        // ESP Rendering
        if (config::g_config.visuals.esp_enabled) {
             void* mita = sdk::game::GetMitaManager();
             if (mita) {
                 sdk::Vector3 mitaPos = sdk::game::GetPosition(mita);
                 // Adjust position up slightly (head?)
                 mitaPos.y += 1.5f; 
                 
                 sdk::Vector3 screenPos = sdk::game::WorldToScreen(mitaPos);
                 
                 if (screenPos.z > 0) {
                      // Invert Y for ImGui
                      screenPos.y = ImGui::GetIO().DisplaySize.y - screenPos.y;

                      ImGui::GetForegroundDrawList()->AddText(ImVec2(screenPos.x, screenPos.y), ImColor(255, 0, 0, 255), "Mita");
                      ImGui::GetForegroundDrawList()->AddCircle(ImVec2(screenPos.x, screenPos.y + 10), 5.0f, ImColor(255, 0, 0, 255));
                 }
             } else {
                 // Debug: MitaManager not found
                 // ImGui::GetForegroundDrawList()->AddText(ImVec2(100, 100), ImColor(255, 0, 0, 255), "MitaManager NOT Found");
             }
        }
    }
}
