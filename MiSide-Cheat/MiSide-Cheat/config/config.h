#pragma once
#include <string>
#include <vector>
#include <Windows.h>

// ============================================================
// Config Structure for MiSide-Zero
// All cheat settings in one place for easy expansion
// ============================================================

namespace config {
    
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
        bool esp_enabled = false;
        bool esp_box = true;
        bool esp_name = true;
        bool esp_health = true;
        bool esp_distance = false;
        float esp_max_distance = 500.0f;
        float esp_box_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
        
        // Chams (for future)
        bool chams_enabled = false;
        int chams_type = 0;  // 0=Flat, 1=Textured, etc.
        float chams_color[4] = { 1.0f, 0.0f, 1.0f, 1.0f };
        
        // World
        bool fullbright = false;
        bool no_fog = false;
        
        // Other
        bool crosshair = false;
        int crosshair_type = 0;
        float crosshair_color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    };
    
    // ============================================================
    // Aimbot Category (for shooter-like games)
    // ============================================================
    struct AimbotSettings {
        bool enabled = false;
        int key = VK_RBUTTON;
        int key_mode = 0;  // 0=Hold, 1=Toggle, 2=Always
        
        float fov = 10.0f;
        float smooth = 5.0f;
        int bone = 0;  // Target bone
        
        bool visible_only = true;
        bool team_check = true;
    };
    
    // ============================================================
    // Misc Category
    // ============================================================
    struct MiscSettings {
        // Player
        bool speed_hack = false;
        float speed_multiplier = 1.5f;
        int speed_key = VK_SHIFT;
        
        bool fly_hack = false;
        int fly_key = VK_SPACE;
        float fly_speed = 10.0f;
        
        bool no_clip = false;
        int no_clip_key = VK_CONTROL;
        
        // Game
        bool infinite_stamina = false;
        bool god_mode = false;
        bool infinite_ammo = false;
        
        // Teleport
        bool teleport_enabled = false;
        int teleport_key = VK_F5;

        // Developer / Debug
        bool debug_view = true;
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
    
    // Save/Load functions (to be implemented)
    bool Save(const char* filename);
    bool Load(const char* filename);
    void Reset();
}
