#pragma once
#include <string>
#include <vector>
#include <Windows.h>
#include "../bind.h"

// ============================================================
// Config Structure for MiSide-Zero
// All cheat settings in one place for easy expansion
// ============================================================

namespace config {
    
    // ============================================================
    // Common Types
    // ============================================================
    // Bind moved to bind.h

    // ============================================================
    // Menu Settings
    // ============================================================
    struct MenuSettings {
        int toggle_key = VK_INSERT;
        float animation_speed = 8.0f;
        int dpi_scale_index = 1;  // 0=75%, 1=100%, 2=125%, 3=150%
        float accent_color[4] = { 0.584f, 0.722f, 0.024f, 1.0f };  // Lime green
        bool show_watermark = true;
        bool show_module_list = true;
    };
    
    // ============================================================
    // Visuals Category
    // ============================================================
    struct VisualsSettings {
        // ESP
        Bind esp;
        bool esp_box = false;
        bool esp_name = false;
        bool esp_distance = false;
        float esp_max_distance = 500.0f;
        float esp_box_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
        
        // Chams
        Bind chams;
        int chams_type = 0;  // 0=Flat, 1=Textured, 2=Glow
        float chams_color[4] = { 1.0f, 0.0f, 1.0f, 1.0f };
        
        // World
        Bind fullbright;
        Bind no_fog;
        
        // Other
        Bind crosshair;
        int crosshair_type = 0;
        float crosshair_color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
        
        // Re-mapping for old feature naming
        bool esp_enabled = false; // Legacy, will be kept for compatibility if needed
    };
    
    // ============================================================
    // Aimbot Category
    // ============================================================
    struct AimbotSettings {
        Bind aimbot;
        float fov = 10.0f;
        float smooth = 5.0f;
        int bone = 0;  // Target bone
        
        bool visible_only = true;
        bool team_check = true;

        bool enabled = false; // Legacy
    };
    
    // ============================================================
    // Misc Category
    // ============================================================
    struct MiscSettings {
        // Player
        Bind speed_hack;
        float speed_multiplier = 1.5f;
        
        Bind fly_hack;
        float fly_speed = 10.0f;
        
        Bind no_clip;
        
        // Game
        Bind infinite_stamina;
        Bind god_mode;
        Bind infinite_ammo;
        
        // Teleport
        Bind teleport;

        // Developer / Debug
        bool debug_view = false;

        // Game Modifiers
        bool mita_speed_enabled = false;
        float mita_speed = 3.5f; // Default Mita speed usually around 3.5

        bool speed_hack_enabled = false; // Legacy
        bool fly_hack_enabled = false; // Legacy
        bool no_clip_enabled = false; // Legacy
    };
    
    // ============================================================
    // Global Config Instance
    // ============================================================
    struct GlobalConfig {
        MenuSettings menu;
        VisualsSettings visuals;
        AimbotSettings aimbot;
        MiscSettings misc;
    };
    
    inline GlobalConfig g_config;
    
    // Save/Load functions
    bool Save(const char* filename);
    bool Load(const char* filename);
    void Reset();
    std::string GetConfigDirectory();
}
