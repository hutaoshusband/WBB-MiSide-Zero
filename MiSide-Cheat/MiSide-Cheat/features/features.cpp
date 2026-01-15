#include "features.h"
#include "../config/config.h"
#include <algorithm>
#include <cstring>

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
        // ESP, crosshair, etc. will be drawn here
    }
}
